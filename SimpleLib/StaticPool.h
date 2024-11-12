#pragma once
#include <memory>
#include <type_traits>
#include "SecureQueue.h"

template<typename T>
class StaticPool
{
private:
	StaticPool() = default;
	SecureQueue<std::shared_ptr<T>> mPool;

	struct Deleter {
		void operator()(T* object) {
			if (nullptr == object)
				return;

			std::shared_ptr<T> ptr{ object, [](T*) {} };
			StaticPool<T>::GetInstance().Release(ptr);
		}
	};

public:
	static StaticPool<T>& GetInstance() {
		static StaticPool<T> instance;
		return instance;
	}

	void Release(std::shared_ptr<T> object) {
		if (nullptr == object)
			return;

		object->Release();
		mPool.Push(std::move(object));
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

		return mPool.Pop();
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


template<typename T>
class PoolObject : public std::enable_shared_from_this<T>
{
public:
	virtual ~PoolObject() {};
	virtual void Release() {};

	std::shared_ptr<T> GetSharedPtr() {
		return this->shared_from_this();
	}

	std::weak_ptr<T> GetWeakPtr() {
		return this->shared_from_this();
	}


protected:
	friend class StaticPool<T>;
};


