/**
 * @file queue.hpp
 * @brief Lock-free queue implementation for high-performance concurrent processing
 * 
 * This file provides a lock-free SPSC (Single Producer Single Consumer) queue
 * implementation optimized for high-throughput scenarios such as HTTP request
 * processing. The implementation uses atomic operations to ensure thread safety
 * without the overhead of traditional locks.
 * 
 * @author [@logpacket](https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025/01/08
 * 
 * @copyright Copyright (c) 2025 ParallelStone Project
 * 
 * @details SC (Single Producer Single Consumer) queue for HTTP requests
 * @details Uses atomic operations for thread-safe enqueueing and dequeueing
 * 
 * This implementation provides a high-performance queue suitable for scenarios
 * where one thread produces items and another thread consumes them. The lock-free
 * design eliminates contention and provides predictable performance characteristics.
 * 
 * @tparam T The type of items stored in the queue
 * 
 * @note This queue is designed for SPSC usage only. Using it with multiple
 *       producers or consumers may result in undefined behavior.
 */
template <typename T>
class LockFreeQueue {
 private:
  /**
   * @brief Internal node structure for the linked list
   * 
   * Each node contains atomic pointers to the data and next node,
   * ensuring thread-safe access without locks.
   */
  struct Node {
    std::atomic<T*> data;       ///< Pointer to the stored data
    std::atomic<Node*> next;    ///< Pointer to the next node
    
    /**
     * @brief Default constructor initializes pointers to null
     */
    Node() : data(nullptr), next(nullptr) {}
  };

  std::atomic<Node*> head_;  ///< Pointer to the head of the queue
  std::atomic<Node*> tail_;  ///< Pointer to the tail of the queue

 public:
  /**
   * @brief Constructor initializes the queue with a dummy node
   * 
   * The queue starts with a single dummy node to simplify the
   * enqueue and dequeue operations and avoid special cases.
   */
  LockFreeQueue() {
    Node* dummy = new Node;
    head_.store(dummy);
    tail_.store(dummy);
  }

  /**
   * @brief Destructor cleans up all remaining nodes
   * 
   * Iterates through the linked list and deletes all nodes,
   * ensuring no memory leaks occur.
   */
  ~LockFreeQueue() {
    while (Node* oldHead = head_.load()) {
      head_.store(oldHead->next);
      delete oldHead;
    }
  }

  /**
   * @brief Enqueue an item to the queue
   * @param item The item to enqueue (will be moved)
   * 
   * Creates a new node with the given item and atomically updates
   * the tail pointer to maintain queue consistency. This operation
   * is lock-free and safe for single-producer scenarios.
   */
  void enqueue(T item) {
    Node* newNode = new Node;
    T* data = new T(std::move(item));
    newNode->data.store(data);

    Node* prevTail = tail_.exchange(newNode);
    prevTail->next.store(newNode);
  }

  /**
   * @brief Dequeue an item from the queue
   * @param result Reference to store the dequeued item
   * @return true if successful, false if queue is empty
   * 
   * Attempts to remove and return the front item from the queue.
   * If the queue is empty, returns false and leaves result unchanged.
   * This operation is lock-free and safe for single-consumer scenarios.
   */
  bool dequeue(T& result) {
    Node* head = head_.load();
    Node* next = head->next.load();

    if (next == nullptr) {
      return false;
    }

    T* data = next->data.load();
    if (data == nullptr) {
      return false;
    }

    result = std::move(*data);
    delete data;
    head_.store(next);
    delete head;

    return true;
  }

  /**
   * @brief Check if queue is empty
   * @return true if empty, false otherwise
   * 
   * Checks if there are any items available for dequeuing.
   * Note that this is a snapshot view and the state may change
   * immediately after the call returns in concurrent scenarios.
   */
  bool empty() const {
    Node* head = head_.load();
    Node* next = head->next.load();
    return next == nullptr;
  }
};