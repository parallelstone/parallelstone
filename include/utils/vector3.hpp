#pragma once

#include <cstdint>
#include <cmath>

namespace parallelstone {
namespace utils {

/**
 * @brief 3D vector template for integer and floating-point coordinates
 */
template<typename T>
struct Vector3 {
    T x, y, z;
    
    constexpr Vector3() noexcept : x(0), y(0), z(0) {}
    constexpr Vector3(T x, T y, T z) noexcept : x(x), y(y), z(z) {}
    
    // Arithmetic operators
    constexpr Vector3 operator+(const Vector3& other) const noexcept {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }
    
    constexpr Vector3 operator-(const Vector3& other) const noexcept {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }
    
    constexpr Vector3 operator*(T scalar) const noexcept {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    
    constexpr Vector3 operator/(T scalar) const noexcept {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }
    
    // Compound assignment operators
    Vector3& operator+=(const Vector3& other) noexcept {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }
    
    Vector3& operator-=(const Vector3& other) noexcept {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }
    
    Vector3& operator*=(T scalar) noexcept {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }
    
    Vector3& operator/=(T scalar) noexcept {
        x /= scalar; y /= scalar; z /= scalar;
        return *this;
    }
    
    // Comparison operators
    constexpr bool operator==(const Vector3& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
    
    constexpr bool operator!=(const Vector3& other) const noexcept {
        return !(*this == other);
    }
    
    // Helper methods
    constexpr T dot(const Vector3& other) const noexcept {
        return x * other.x + y * other.y + z * other.z;
    }
    
    Vector3 cross(const Vector3& other) const noexcept requires std::is_floating_point_v<T> {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    T length_squared() const noexcept {
        return x * x + y * y + z * z;
    }
    
    T length() const noexcept requires std::is_floating_point_v<T> {
        return std::sqrt(length_squared());
    }
    
    Vector3 normalized() const noexcept requires std::is_floating_point_v<T> {
        T len = length();
        return len > 0 ? *this / len : Vector3();
    }
    
    // Distance functions
    T distance_squared_to(const Vector3& other) const noexcept {
        return (*this - other).length_squared();
    }
    
    T distance_to(const Vector3& other) const noexcept requires std::is_floating_point_v<T> {
        return (*this - other).length();
    }
    
    // Minecraft-specific helpers for integer vectors
    Vector3 offset(int dx, int dy, int dz) const noexcept requires std::is_integral_v<T> {
        return Vector3(x + dx, y + dy, z + dz);
    }
    
    Vector3 above() const noexcept requires std::is_integral_v<T> {
        return Vector3(x, y + 1, z);
    }
    
    Vector3 below() const noexcept requires std::is_integral_v<T> {
        return Vector3(x, y - 1, z);
    }
    
    Vector3 north() const noexcept requires std::is_integral_v<T> {
        return Vector3(x, y, z - 1);
    }
    
    Vector3 south() const noexcept requires std::is_integral_v<T> {
        return Vector3(x, y, z + 1);
    }
    
    Vector3 west() const noexcept requires std::is_integral_v<T> {
        return Vector3(x - 1, y, z);
    }
    
    Vector3 east() const noexcept requires std::is_integral_v<T> {
        return Vector3(x + 1, y, z);
    }
};

// Common type aliases
using Vector3i = Vector3<std::int32_t>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;

// Block coordinate helpers
using BlockPos = Vector3i;
using ChunkPos = Vector3<std::int32_t>;

} // namespace utils
} // namespace parallelstone

// Hash specialization for use in unordered containers
namespace std {
    template<typename T>
    struct hash<parallelstone::utils::Vector3<T>> {
        std::size_t operator()(const parallelstone::utils::Vector3<T>& v) const noexcept {
            std::size_t h1 = std::hash<T>{}(v.x);
            std::size_t h2 = std::hash<T>{}(v.y);
            std::size_t h3 = std::hash<T>{}(v.z);
            
            // Combine hashes using a standard technique
            h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
            h1 ^= h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
            
            return h1;
        }
    };
}