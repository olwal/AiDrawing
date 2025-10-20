#pragma once

// Implementation file for ThreadSafeList (included by ThreadSafeList.h)

template <typename T>
void ThreadSafeList<T>::push_back(const T& value) {
    std::lock_guard<std::mutex> lock(mutex);
    data.push_back(value);
}

template <typename T>
void ThreadSafeList<T>::push_back(T&& value) {
    std::lock_guard<std::mutex> lock(mutex);
    data.push_back(std::move(value));
}

template <typename T>
void ThreadSafeList<T>::push_front(const T& value) {
    std::lock_guard<std::mutex> lock(mutex);
    data.push_front(value);
}

template <typename T>
void ThreadSafeList<T>::push_front(T&& value) {
    std::lock_guard<std::mutex> lock(mutex);
    data.push_front(std::move(value));
}

template <typename T>
void ThreadSafeList<T>::pop_front() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!data.empty()) {
        data.pop_front();
    }
}

template <typename T>
void ThreadSafeList<T>::pop_back() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!data.empty()) {
        data.pop_back();
    }
}

template <typename T>
std::list<T> ThreadSafeList<T>::getAll() const {
    std::lock_guard<std::mutex> lock(mutex);
    return data;
}

template <typename T>
std::vector<T> ThreadSafeList<T>::getAllAsVector() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::vector<T>(data.begin(), data.end());
}

template <typename T>
void ThreadSafeList<T>::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    data.clear();
}

template <typename T>
bool ThreadSafeList<T>::empty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return data.empty();
}

template <typename T>
size_t ThreadSafeList<T>::size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return data.size();
}

template <typename T>
void ThreadSafeList<T>::forEach(const std::function<void(const T&)>& func) const {
    std::list<T> copy;
    {
        std::lock_guard<std::mutex> lock(mutex);
        copy = data;
    }

    // Apply function to the copy outside the lock
    for (const auto& item : copy) {
        func(item);
    }
}

template <typename T>
template <typename Predicate>
void ThreadSafeList<T>::removeIf(Predicate pred) {
    std::lock_guard<std::mutex> lock(mutex);
    data.remove_if(pred);
}

template <typename T>
template <typename Predicate>
T ThreadSafeList<T>::findIf(Predicate pred) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = std::find_if(data.begin(), data.end(), pred);
    if (it != data.end()) {
        return *it;
    }
    return T();
}

template <typename T>
template <typename Predicate>
bool ThreadSafeList<T>::exists(Predicate pred) const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::find_if(data.begin(), data.end(), pred) != data.end();
}

template <typename T>
T ThreadSafeList<T>::front() const {
    std::lock_guard<std::mutex> lock(mutex);
    if (!data.empty()) {
        return data.front();
    }
    return T();
}

template <typename T>
T ThreadSafeList<T>::back() const {
    std::lock_guard<std::mutex> lock(mutex);
    if (!data.empty()) {
        return data.back();
    }
    return T();
}