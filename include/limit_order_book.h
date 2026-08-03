#pragma once
#include "price_level.h"
#include "trade.h"
#include "pool_allocator.h"
#include <map>
#include <functional>
#include <algorithm>

namespace hft
{
    struct LimitOrderBook
    {
        using TradeCallback = std::function<void(const Trade&)>;
        
        TradeCallback on_trade_; // 成员变量：成交时的回调钩子

        void registerTradeCallback(TradeCallback cb)
        {
            on_trade_ = cb;
        }

        LimitOrderBook(size_t max_orders = 0)
            : bids_pool_(96, 1024)
            , asks_pool_(96, 1024)
            , bids_(std::greater<Price>(), PoolAllocator<std::pair<const Price, PriceLevel>>(bids_pool_))
            , asks_(PoolAllocator<std::pair<const Price, PriceLevel>>(asks_pool_)) 
        {
            if (max_orders > 0) 
            {
                order_map_.resize(max_orders);
                // 预热：一次性触发所有物理页分配
                std::fill(order_map_.begin(), order_map_.end(), OrderLocation{});
            }
        }
        
        struct OrderLocation
        {
            Price price;
            Side side;
            size_t index; // 订单在 vector 中的下标
            bool valid;
            OrderLocation() : valid(false){}
            OrderLocation(Price p, Side s, size_t idx) : price(p), side(s), index(idx), valid(true) {}
        };
        
        MemoryPool bids_pool_;
        MemoryPool asks_pool_;
        std::map<Price, PriceLevel, std::greater<Price>, PoolAllocator<std::pair<const Price, PriceLevel>>> bids_;
        std::map<Price, PriceLevel, std::less<Price>, PoolAllocator<std::pair<const Price, PriceLevel>>> asks_;

        std::vector<OrderLocation> order_map_; //某id订单在的具体位置

        //增加订单逻辑
        void addOrder(Order order)
        {
            if(order.side == Side::BUY) //是买单
            {
                //撮合循环
                if(!asks_.empty())
                {
                    auto it = asks_.begin();
                    while(it != asks_.end() && it->first <= order.price)
                    {
                        auto& cur_price_level = it->second;
                        if(cur_price_level.total_qty != 0)
                        {
                            size_t cur_count = cur_price_level.orders.size();
                            for(size_t i = cur_price_level.head_index; i < cur_count; ++i)
                            {
                                if(!cur_price_level.orders[i].is_canceled)
                                {
                                    Quantity matched_qty = 0;
                                    if(order.qty >= cur_price_level.orders[i].qty) //买单需要的比当前找到的卖单多，即没消化完
                                    {
                                        matched_qty = cur_price_level.orders[i].qty; //记录成交量
                                        order.qty -= cur_price_level.orders[i].qty;
                                        cur_price_level.orders[i].is_canceled = true;
                                        cur_price_level.total_qty -= cur_price_level.orders[i].qty;
                                        if(cur_price_level.head_index == i)
                                        {
                                            while(cur_price_level.head_index < cur_count &&
                                                cur_price_level.orders[cur_price_level.head_index].is_canceled)
                                            {
                                                cur_price_level.head_index++;
                                            }
                                            i = cur_price_level.head_index - 1;
                                        }
                                    }
                                    else //买单需要的比当前找到的卖单少
                                    {
                                        matched_qty = order.qty; // 记录成交量
                                        cur_price_level.orders[i].qty -= order.qty;
                                        cur_price_level.total_qty -= order.qty;
                                        order.qty = 0;
                                    }
                                    if(on_trade_)
                                    {
                                        on_trade_(Trade
                                        {
                                            .price = it->first,
                                            .qty = matched_qty,
                                            .buy_order_id = order.id,
                                            .sell_order_id = cur_price_level.orders[i].id
                                        });
                                    }
                                    if(order.qty == 0)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        if(order.qty == 0) return;
                        ++it;
                    }
                }

                //在订单簿中添加订单
                auto [it, inserted] = bids_.try_emplace(order.price, order.price);
                it->second.orders.push_back(order);
                it->second.total_qty += order.qty;
                if(order.id >= order_map_.size())
                {
                    order_map_.resize(order.id + 1);
                }
                order_map_[order.id] = {order.price, order.side, it->second.orders.size() - 1};
            }
            else // 是卖单
            {
                //撮合循环
                if(!bids_.empty())
                {
                    auto it = bids_.begin();
                    // 卖单吃买单：只要买单簿最高价大于等于当前卖价，就一直吃
                    while(it != bids_.end() && it->first >= order.price)
                    {
                        auto& cur_price_level = it->second;
                        if(cur_price_level.total_qty != 0)
                        {
                            size_t cur_count = cur_price_level.orders.size();
                            // 从游标处开始遍历，干掉无效扫描
                            for(size_t i = cur_price_level.head_index; i < cur_count; ++i)
                            {
                                if(!cur_price_level.orders[i].is_canceled)
                                {
                                    Quantity matched_qty = 0;

                                    if(order.qty >= cur_price_level.orders[i].qty)
                                    {
                                        matched_qty = cur_price_level.orders[i].qty; // 记录成交量
                                        order.qty -= cur_price_level.orders[i].qty;
                                        cur_price_level.orders[i].is_canceled = true;
                                        cur_price_level.total_qty -= cur_price_level.orders[i].qty;

                                        // 游标推进：仅当当前处理的正是队首时才推进
                                        if(cur_price_level.head_index == i)
                                        {
                                            while(cur_price_level.head_index < cur_count &&
                                                  cur_price_level.orders[cur_price_level.head_index].is_canceled)
                                            {
                                                cur_price_level.head_index++;
                                            }
                                            // 修正 i，让 for 循环的 ++i 落在正确的最新游标上
                                            i = cur_price_level.head_index - 1;
                                        }
                                    }
                                    else
                                    {
                                        matched_qty = order.qty;
                                        cur_price_level.orders[i].qty -= order.qty;
                                        cur_price_level.total_qty -= order.qty;
                                        order.qty = 0;
                                    }

                                    // 成交回调：移到 break 之前，确保 partial fill 也能触发
                                    if(on_trade_)
                                    {
                                        on_trade_(Trade{
                                            .price = it->first,
                                            .qty = matched_qty,
                                            .buy_order_id = cur_price_level.orders[i].id, // 躺在账簿里的 Maker 是买单
                                            .sell_order_id = order.id                     // 主动吃单的 Taker 是卖单
                                        });
                                    }
                                    
                                    // 当前卖单已被完全消化，跳出该价格档位的内循环
                                    if(order.qty == 0) break;
                                }
                            }
                        }
                        if(order.qty == 0) return;
                        ++it;
                    }
                }
                
                // 如果经历撮合后仍有剩余数量（或者根本没碰到能撮合的买单），挂入卖单簿
                auto [it, inserted] = asks_.try_emplace(order.price, order.price);
                it->second.orders.push_back(order);
                it->second.total_qty += order.qty;
                order_map_[order.id] = {order.price, order.side, it->second.orders.size() - 1};
            }
        }
        
        //删除订单逻辑
        void cancelOrder(OrderId id)
        {
            if(id >= order_map_.size()) return;
            auto& loc = order_map_[id];
            if(!loc.valid) return;
            if(loc.side == Side::BUY)
            {
                auto& level = bids_.at(loc.price);
                auto& tar_order = level.orders[loc.index];
                tar_order.is_canceled = true;
                level.total_qty -= tar_order.qty;
            }
            else
            {
                auto& level = asks_.at(loc.price);
                auto& tar_order = level.orders[loc.index];
                tar_order.is_canceled = true;
                level.total_qty -= tar_order.qty; 
            }
            loc.valid = false;
        }
    };
}