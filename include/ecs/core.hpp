#pragma once

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <functional>
#include <algorithm>
#include <cassert>
#include <string>
#include <random>
#include <iomanip>
#include <sstream>

namespace parallelstone {
namespace ecs {

// ==================== CORE TYPES ====================

/**
 * @brief Unique identifier for entities
 */
using Entity = std::uint32_t;

/**
 * @brief Special entity value representing null/invalid entity
 */
constexpr Entity NULL_ENTITY = 0;

/**
 * @brief Component ID type for type identification
 */
using ComponentId = std::uint32_t;

// ==================== COMPONENT STORAGE ====================

/**
 * @brief Base class for component storage
 */
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void entity_destroyed(Entity entity) = 0;
    virtual size_t size() const = 0;
    virtual bool has_entity(Entity entity) const = 0;
};

/**
 * @brief Template component storage array
 */
template<typename T>
class ComponentArray : public IComponentArray {
private:
    std::vector<T> components_;
    std::unordered_map<Entity, size_t> entity_to_index_;
    std::unordered_map<size_t, Entity> index_to_entity_;
    size_t size_ = 0;

public:
    void insert_data(Entity entity, T component) {
        assert(entity_to_index_.find(entity) == entity_to_index_.end() && "Component added to same entity more than once");

        // Put new entry at end and update the maps
        size_t new_index = size_;
        entity_to_index_[entity] = new_index;
        index_to_entity_[new_index] = entity;
        
        if (new_index >= components_.size()) {
            components_.resize(new_index + 1);
        }
        components_[new_index] = std::move(component);
        ++size_;
    }

    void remove_data(Entity entity) {
        assert(entity_to_index_.find(entity) != entity_to_index_.end() && "Removing non-existent component");

        // Copy element at end into deleted element's place to maintain density
        size_t index_of_removed_entity = entity_to_index_[entity];
        size_t index_of_last_element = size_ - 1;
        components_[index_of_removed_entity] = std::move(components_[index_of_last_element]);

        // Update map to point to moved spot
        Entity entity_of_last_element = index_to_entity_[index_of_last_element];
        entity_to_index_[entity_of_last_element] = index_of_removed_entity;
        index_to_entity_[index_of_removed_entity] = entity_of_last_element;

        entity_to_index_.erase(entity);
        index_to_entity_.erase(index_of_last_element);

        --size_;
    }

    T& get_data(Entity entity) {
        assert(entity_to_index_.find(entity) != entity_to_index_.end() && "Retrieving non-existent component");
        return components_[entity_to_index_[entity]];
    }

    const T& get_data(Entity entity) const {
        assert(entity_to_index_.find(entity) != entity_to_index_.end() && "Retrieving non-existent component");
        return components_[entity_to_index_[entity]];
    }

    bool has_data(Entity entity) const {
        return entity_to_index_.find(entity) != entity_to_index_.end();
    }

    void entity_destroyed(Entity entity) override {
        if (entity_to_index_.find(entity) != entity_to_index_.end()) {
            remove_data(entity);
        }
    }

    size_t size() const override {
        return size_;
    }

    bool has_entity(Entity entity) const override {
        return has_data(entity);
    }

    // Iterator support for system queries
    auto begin() { return components_.begin(); }
    auto end() { return components_.begin() + size_; }
    auto begin() const { return components_.begin(); }
    auto end() const { return components_.begin() + size_; }

    // Get all entities with this component
    std::vector<Entity> get_entities() const {
        std::vector<Entity> entities;
        entities.reserve(size_);
        for (const auto& [entity, index] : entity_to_index_) {
            entities.push_back(entity);
        }
        return entities;
    }
};

// ==================== COMPONENT MANAGER ====================

/**
 * @brief Manages all component types and storage
 */
class ComponentManager {
private:
    std::unordered_map<std::type_index, ComponentId> component_types_;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> component_arrays_;
    ComponentId next_component_type_ = 0;

    template<typename T>
    ComponentArray<T>* get_component_array() {
        std::type_index type_index = std::type_index(typeid(T));
        
        assert(component_types_.find(type_index) != component_types_.end() && "Component not registered before use");
        
        return static_cast<ComponentArray<T>*>(component_arrays_[type_index].get());
    }

public:
    template<typename T>
    void register_component() {
        std::type_index type_index = std::type_index(typeid(T));
        
        assert(component_types_.find(type_index) == component_types_.end() && "Registering component type more than once");
        
        component_types_[type_index] = next_component_type_;
        component_arrays_[type_index] = std::make_unique<ComponentArray<T>>();
        
        ++next_component_type_;
    }

    template<typename T>
    ComponentId get_component_type() {
        std::type_index type_index = std::type_index(typeid(T));
        
        assert(component_types_.find(type_index) != component_types_.end() && "Component not registered before use");
        
        return component_types_[type_index];
    }

    template<typename T>
    void add_component(Entity entity, T component) {
        get_component_array<T>()->insert_data(entity, std::move(component));
    }

    template<typename T>
    void remove_component(Entity entity) {
        get_component_array<T>()->remove_data(entity);
    }

    template<typename T>
    T& get_component(Entity entity) {
        return get_component_array<T>()->get_data(entity);
    }

    template<typename T>
    const T& get_component(Entity entity) const {
        return const_cast<ComponentManager*>(this)->get_component_array<T>()->get_data(entity);
    }

    template<typename T>
    bool has_component(Entity entity) const {
        std::type_index type_index = std::type_index(typeid(T));
        if (component_types_.find(type_index) == component_types_.end()) {
            return false;
        }
        return get_component_array<T>()->has_data(entity);
    }

    void entity_destroyed(Entity entity) {
        for (auto& pair : component_arrays_) {
            pair.second->entity_destroyed(entity);
        }
    }

    template<typename T>
    std::vector<Entity> get_entities_with_component() const {
        return const_cast<ComponentManager*>(this)->get_component_array<T>()->get_entities();
    }
};

// ==================== ENTITY MANAGER ====================

/**
 * @brief Manages entity creation and destruction
 */
class EntityManager {
private:
    std::vector<Entity> available_entities_;
    Entity living_entity_count_ = 0;
    Entity next_entity_id_ = 1;  // Start from 1, 0 is NULL_ENTITY

public:
    Entity create_entity() {
        Entity id;
        
        if (!available_entities_.empty()) {
            id = available_entities_.back();
            available_entities_.pop_back();
        } else {
            id = next_entity_id_++;
        }
        
        ++living_entity_count_;
        return id;
    }

    void destroy_entity(Entity entity) {
        assert(entity < next_entity_id_ && "Entity out of range");
        
        available_entities_.push_back(entity);
        --living_entity_count_;
    }

    Entity get_living_entity_count() const {
        return living_entity_count_;
    }

    bool is_valid_entity(Entity entity) const {
        return entity != NULL_ENTITY && 
               entity < next_entity_id_ && 
               std::find(available_entities_.begin(), available_entities_.end(), entity) == available_entities_.end();
    }
};

// ==================== SYSTEM BASE CLASS ====================

/**
 * @brief Base class for all systems
 */
class System {
public:
    virtual ~System() = default;
    virtual void update(class Registry& registry, float delta_time) = 0;
    virtual void init(class Registry& registry) {}
    virtual void shutdown(class Registry& registry) {}
};

// ==================== VIEW FOR COMPONENT QUERIES ====================

/**
 * @brief View for querying entities with specific components
 */
template<typename... Components>
class View {
private:
    ComponentManager& component_manager_;
    std::vector<Entity> entities_;

public:
    explicit View(ComponentManager& manager) : component_manager_(manager) {
        // Get intersection of all entities that have all required components
        if constexpr (sizeof...(Components) > 0) {
            auto first_entities = get_first_component_entities();
            
            for (Entity entity : first_entities) {
                if ((component_manager_.has_component<Components>(entity) && ...)) {
                    entities_.push_back(entity);
                }
            }
        }
    }

private:
    std::vector<Entity> get_first_component_entities() {
        using FirstComponent = std::tuple_element_t<0, std::tuple<Components...>>;
        return component_manager_.get_entities_with_component<FirstComponent>();
    }

public:
    // Iterator support
    auto begin() { return entities_.begin(); }
    auto end() { return entities_.end(); }
    auto begin() const { return entities_.begin(); }
    auto end() const { return entities_.end(); }

    size_t size() const { return entities_.size(); }
    bool empty() const { return entities_.empty(); }

    // Get component for entity (assumes entity is in this view)
    template<typename T>
    T& get(Entity entity) {
        return component_manager_.get_component<T>(entity);
    }

    template<typename T>
    const T& get(Entity entity) const {
        return component_manager_.get_component<T>(entity);
    }
};

// ==================== MAIN REGISTRY ====================

/**
 * @brief Main ECS registry that coordinates entities, components, and systems
 */
class Registry {
private:
    EntityManager entity_manager_;
    ComponentManager component_manager_;
    std::vector<std::unique_ptr<System>> systems_;

public:
    // Entity management
    Entity create() {
        return entity_manager_.create_entity();
    }

    void destroy(Entity entity) {
        component_manager_.entity_destroyed(entity);
        entity_manager_.destroy_entity(entity);
    }

    bool valid(Entity entity) const {
        return entity_manager_.is_valid_entity(entity);
    }

    Entity get_living_entity_count() const {
        return entity_manager_.get_living_entity_count();
    }

    // Component management
    template<typename T>
    void register_component() {
        component_manager_.register_component<T>();
    }

    template<typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args) {
        T component(std::forward<Args>(args)...);
        component_manager_.add_component<T>(entity, std::move(component));
        return component_manager_.get_component<T>(entity);
    }

    template<typename T>
    void remove(Entity entity) {
        component_manager_.remove_component<T>(entity);
    }

    template<typename T>
    T& get(Entity entity) {
        return component_manager_.get_component<T>(entity);
    }

    template<typename T>
    const T& get(Entity entity) const {
        return component_manager_.get_component<T>(entity);
    }

    template<typename T>
    bool has(Entity entity) const {
        return component_manager_.has_component<T>(entity);
    }

    // View creation for component queries
    template<typename... Components>
    View<Components...> view() {
        return View<Components...>(component_manager_);
    }

    // System management
    template<typename T, typename... Args>
    T& add_system(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& system_ref = *system;
        systems_.push_back(std::move(system));
        system_ref.init(*this);
        return system_ref;
    }

    void update_systems(float delta_time) {
        for (auto& system : systems_) {
            system->update(*this, delta_time);
        }
    }

    void shutdown_systems() {
        for (auto& system : systems_) {
            system->shutdown(*this);
        }
        systems_.clear();
    }

    // Utility functions
    void clear() {
        shutdown_systems();
        // Destroy all entities
        auto all_entities = get_all_entities();
        for (Entity entity : all_entities) {
            destroy(entity);
        }
    }

private:
    std::vector<Entity> get_all_entities() {
        std::vector<Entity> entities;
        for (Entity id = 1; id < entity_manager_.get_living_entity_count() + available_entities_size() + 1; ++id) {
            if (entity_manager_.is_valid_entity(id)) {
                entities.push_back(id);
            }
        }
        return entities;
    }

    size_t available_entities_size() const {
        // This is a simplified approach. In practice, you'd want to expose this from EntityManager
        return 0;
    }
};

// ==================== UTILITY FUNCTIONS ====================

/**
 * @brief Generate UUID for offline mode (simple hash-based approach)
 */
inline std::string generate_uuid(const std::string& username) {
    // Simplified offline UUID generation based on username
    std::hash<std::string> hasher;
    auto hash = hasher(username);
    
    // Format as UUID-like string
    char uuid_str[37];
    std::snprintf(uuid_str, sizeof(uuid_str), 
                  "%08x-%04x-%04x-%04x-%012x",
                  static_cast<uint32_t>(hash & 0xFFFFFFFF),
                  static_cast<uint16_t>((hash >> 32) & 0xFFFF),
                  static_cast<uint16_t>((hash >> 48) & 0x0FFF) | 0x3000,  // Version 3
                  static_cast<uint16_t>((hash >> 16) & 0x3FFF) | 0x8000,  // Variant bits
                  static_cast<uint64_t>(hash) >> 16);
    
    return std::string(uuid_str);
}

} // namespace ecs
} // namespace parallelstone