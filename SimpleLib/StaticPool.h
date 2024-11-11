#pragma once
#include "SecureQueue.h"
#include "PoolObject.h"
#include <memory>

template<typename T>
class StaticPool {
private:
	SecureQueue<std::shared_ptr<T>> mPool;

	void Release(std::shared_ptr<T> object) {
		if (nullptr == object)
			return;

		object->Release();
		mPool.Push(object);
	}

	struct Deleter {
		void operator()(T* object) {
			if (nullptr == object)
				return;

			std::shared_ptr<T> ptr{ object, [](T*) {} };
			StaticPool<T>::GetInstance()->Release(ptr);
		}
	};

public:

	static StaticPool<T>& GetInstance() {
		static StaticPool<T> instance;
		return instance;
	}

	virtual ~StaticPool() {
		Clear();
	}

	void Reserve(int size) {
		for (int i = 0; i < size; i++) {
			mPool.Push(std::shared_ptr<T>(new T(), Deleter()));
		}
	}

	std::shared_ptr<T> Pop() {
		if (mPool.IsEmpty()) {
			return std::shared_ptr<T>(new T(), Deleter());
		}
		else {
			return mPool.Pop();
		}
	}

	void Clear() {
		while (!mPool.IsEmpty()) {
			auto obj = mPool.Pop();
			if (nullptr != obj)
			{
				obj->Release();
				obj.reset();
			}
		}
	}
};
