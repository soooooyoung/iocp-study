#pragma once
#include <memory>

template<typename T>
class PoolObject : public std::enable_shared_from_this<T>
{
public:
    virtual ~PoolObject() = default;

	std::shared_ptr<T> GetSharedPtr() {
		return this->shared_from_this();
	}

	std::weak_ptr<T> GetWeakPtr() {
		return this->shared_from_this();
	}

	virtual void Release() = 0;

protected:
	PoolObject() = default;
	friend class StaticPool<T>;
};