#pragma once
#include "types.h"

namespace hft
{
    //定义订单结构体
    struct Order
    {
        OrderId id;
        Price price;
        Quantity qty;
        Side side;
        bool is_canceled;

        //无参初始化
        Order() : id(0), price(0), qty(0), side(Side::BUY), is_canceled(false) {}
        
        //有参初始化
        Order(OrderId id, Price price, Quantity qty, Side side)
            : id(id), price(price), qty(qty), side(side), is_canceled(false) {}
    };

}