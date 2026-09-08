#pragma once
#include "types.h" // 假设里面定义了 Price, Qty, OrderId 等

namespace hft
{
struct Trade
{
    Price price;           // 成交价格
    Quantity qty;          // 成交数量
    OrderId buy_order_id;  // 买方订单 ID
    OrderId sell_order_id; // 卖方订单 ID
};
} // namespace hft