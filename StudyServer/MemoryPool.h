#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include <functional>

template <typename T>
class MemoryPool {
public:
	using UniquePtr = std::unique_ptr<T, std::function<void(T*)>>;

    MemoryPool(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            pool_.push_back(std::make_unique<T>());
        }
    }

    UniquePtr Acquire() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (pool_.empty()) {
			return std::unique_ptr<T, std::function<void(T*)>>(new T(), [this](T* ptr) {
				this->Release(ptr);
				});
        }

        auto obj = std::move(pool_.back());
        pool_.pop_back();
        return std::unique_ptr<T, std::function<void(T*)>>(obj.release(), [this](T* ptr) {
            this->Release(ptr);
            });
    }

private:
    void Release(T* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push_back(std::unique_ptr<T>(ptr));
    }

    std::vector<std::unique_ptr<T>> pool_;
    std::mutex mutex_;
};
