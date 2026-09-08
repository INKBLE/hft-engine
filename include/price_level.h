#pragma once
#include "order.h"
#include "types.h"
#include <vector>

namespace hft
{
// 特定价格的排队队列
struct PriceLevel
{
    Price price;
    Quantity total_qty;
    size_t head_index;
    std::vector<Order> orders;
    PriceLevel(Price price) : price(price), total_qty(0), head_index(0) {}
};
} // namespace hft
