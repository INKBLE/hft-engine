#include <iostream>
#include "limit_order_book.h" 

int main()
{
    // std::cout << "map node size: " << sizeof(std::map<hft::Price, hft::PriceLevel>::value_type) << std::endl;
    // std::cout << "map node alignment: " << alignof(std::map<hft::Price, hft::PriceLevel>::value_type) << std::endl;

    
    hft::LimitOrderBook book(2000000);

    // 1. 注册成交回调函数（在这里打印成交回报）
    book.registerTradeCallback([](const hft::Trade& trade) {
        std::cout << "【成交回报】 "
                  << "价格: " << trade.price 
                  << ", 数量: " << trade.qty 
                  << ", 买单ID: " << trade.buy_order_id 
                  << ", 卖单ID: " << trade.sell_order_id 
                  << std::endl;
    });

    std::cout << "=== 测试 1: 挂单不成交 ===" << std::endl;
    // 投递一个卖单：价格 100，数量 10，ID = 1
    // 修改处 1：使用你 order.h 中定义的标准构造函数
    hft::Order sell_order1(1, 100, 10, hft::Side::SELL);
    book.addOrder(sell_order1);
    std::cout << "卖单 1 已挂入账簿，当前 asks_ 档位数: " << book.asks_.size() << "\n\n";

    std::cout << "=== 测试 2: 买单进场，部分成交并剩余挂单 ===" << std::endl;
    // 投递一个买单：价格 100，数量 15（比上面的卖单多 5 个），ID = 2
    // 修改处 2：使用标准构造函数
    hft::Order buy_order1(2, 100, 15, hft::Side::BUY);
    book.addOrder(buy_order1);
    std::cout << "买单 2 撮合完毕。吃掉卖单 1 的 10 个数量，自身剩余 5 个挂入 bids_。" << std::endl;
    std::cout << "当前买单簿 bids_ 档位数: " << book.bids_.size() << "\n\n";

    std::cout << "=== 测试 3: 撤单测试 ===" << std::endl;
    // 买单 2 剩余的 5 个数量此时在 bids_ 里，其订单 ID 是 2
    // 我们现在将它撤销
    book.cancelOrder(2);
    std::cout << "已执行撤单 ID: 2" << std::endl;
    
    // 修改处 3：将 book.bids_[100] 替换为 book.bids_.at(100)
    std::cout << "撤单后，该价格档位的 total_qty 总量应为 0：" << book.bids_.at(100).total_qty << std::endl;
    
    return 0;
}