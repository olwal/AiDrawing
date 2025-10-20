#pragma once

#include <list>
#include <mutex>
#include <vector>
#include <algorithm>
#include <functional>

template <typename T>
class ThreadSafeList {
public:
    ThreadSafeList() = default;
    ~ThreadSafeList() = default;

    // Prevent copying or moving
    ThreadSafeList(const ThreadSafeList&) = delete;
    ThreadSafeList& operator=(const ThreadSafeList&) = delete;
    ThreadSafeList(ThreadSafeList&&) = delete;
    ThreadSafeList& operator=(ThreadSafeList&&) = delete;

    // Add element at the end
    void push_back(const T& value);
    void push_back(T&& value);

    // Add element at the beginning
    void push_front(const T& value);
    void push_front(T&& value);

    // Remove first element
    void pop_front();

    // Remove last element
    void pop_back();

    // Get a copy of all elements (thread-safe)
    std::list<T> getAll() const;

    // Get a copy as vector for faster random access
    std::vector<T> getAllAsVector() const;

    // Clear the list
    void clear();

    // Check if empty
    bool empty() const;

    // Get size
    size_t size() const;

    // Apply a function to each element (thread-safe)
    void forEach(const std::function<void(const T&)>& func) const;

    // Remove elements that match a predicate (thread-safe)
    template <typename Predicate>
    void removeIf(Predicate pred);

    // Find an element (returns a copy if found, or default T if not found)
    template <typename Predicate>
    T findIf(Predicate pred) const;

    // Check if an element exists
    template <typename Predicate>
    bool exists(Predicate pred) const;

    // Get first element (returns a copy)
    T front() const;

    // Get last element (returns a copy)
    T back() const;

private:
    mutable std::mutex mutex;
    std::list<T> data;
};

// Template implementation
#include "ThreadSafeList.inl"