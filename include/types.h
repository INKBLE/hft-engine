#pragma once
#include <cstdint>
namespace hft
{
    //定义核心金融字段类型，使用无符号64位整数避免负数异常
    using Price = uint64_t;
    using Quantity = uint64_t;
    using OrderId = uint64_t;

    //订单方向
    enum class Side : uint8_t
    {
        BUY = 0,
        SELL = 1
    };
}