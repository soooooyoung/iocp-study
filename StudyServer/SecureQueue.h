#pragma once
#include <concurrent_queue.h>

template<typename T>
class SecureQueue {
private:
    concurrency::concurrent_queue<T> mObjects;

public:
    bool IsEmpty() const {
        return mObjects.empty();
    }

    void Push(T object) {
        mObjects.push(object);
    }

    T Pop() {
        T object;
        mObjects.try_pop(object);
        return object;
    }

    void Clear() {
        while (!mObjects.empty()) {
            T object;
            mObjects.try_pop(object);
        }
    }
};
