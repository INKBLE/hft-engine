#pragma once
#include <cstddef>
#include <new>
#include <cstring>

namespace hft
{
    class MemoryPool 
    {
    public:
        // 构造池子
        MemoryPool(size_t slot_size, size_t num_slots) : slot_size_(slot_size), num_slots_(num_slots)
        {
            pool_memory_ = new char[slot_size * num_slots];
            
            // 强制操作系统立刻分配物理页消除 minor page fault
            std::memset(pool_memory_, 0, slot_size_ * num_slots_);

            // 串起前 num_slots-1 个槽位
            for (size_t i = 0; i < num_slots_ - 1; ++i)
            {
                char* cur_slot  = pool_memory_ + i * slot_size_;
                char* next_slot = pool_memory_ + (i + 1) * slot_size_;
                reinterpret_cast<FreelistNode*>(cur_slot)->next = reinterpret_cast<FreelistNode*>(next_slot);
            }

            // 最后一个槽位的 next 指向空
            char* last_slot = pool_memory_ + (num_slots_ - 1) * slot_size_;
            reinterpret_cast<FreelistNode*>(last_slot)->next = nullptr;

            // 头指针指向第一个槽位
            free_head_ = reinterpret_cast<FreelistNode*>(pool_memory_); 
        }

        //释放池子
        ~MemoryPool()
        {
            delete[] pool_memory_;
        }

        // 摘取空闲链表头部节点，返回 void*
        void* allocate()
        {
            void* free_head = reinterpret_cast<void*>(free_head_);
            free_head_ = free_head_->next;
            return free_head;
        }
        
        //把传入的 void* 重新插回链表头部
        void deallocate(void* p)
        {
            FreelistNode* free_head = reinterpret_cast<FreelistNode*>(p);
            free_head->next = free_head_;
            free_head_ = free_head;
        }


    private:
        union FreelistNode
        {
            FreelistNode* next; 
            char data[1];  // 占位符，只为让 union 合法表达字节视图
        };

        char* pool_memory_;       // 原始内存块
        FreelistNode* free_head_; // 空闲链表头
        size_t slot_size_;        // 每个槽位的大小
        size_t num_slots_;        // 槽位总数
    };
}