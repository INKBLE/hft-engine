#pragma once
#include "memory_pool.h"
#include <cstddef>

namespace hft
{
// 前向声明，避免循环依赖
class MemoryPool;

template <typename T>
struct PoolAllocator
{
    using value_type = T;

    MemoryPool* pool_; // 指向池子，不拥有所有权

    template <typename U>
    struct rebind
    {
        using other = PoolAllocator<U>;
    };

    // 构造函数：接受 MemoryPool 引用
    PoolAllocator(MemoryPool& pool) : pool_(&pool) {}

    // 模板拷贝构造
    template <typename U>
    PoolAllocator(const PoolAllocator<U>& other) : pool_(other.pool_)
    {
    }

    // 分配 n 个对象（忽略 n，因为 std::map 一次只请求 1）
    T* allocate(std::size_t n)
    {
        (void)n; // 消除“未使用参数”的警告
        return static_cast<T*>(pool_->allocate());
    }

    // 释放 n 个对象
    void deallocate(T* p, std::size_t n)
    {
        (void)n;
        pool_->deallocate(p);
    }
};

// 比较两个分配器：同一个池就是相等
template <typename T, typename U>
bool operator==(const PoolAllocator<T>& a, const PoolAllocator<U>& b)
{ return a.pool_ == b.pool_; }

template <typename T, typename U>
bool operator!=(const PoolAllocator<T>& a, const PoolAllocator<U>& b)
{ return !(a == b); }
} // namespace hft
