#include <gtest/gtest.h>
#include <vector>
// 假设你的头文件放在 include 目录下，或者和当前文件同级
// 如果报错找不到头文件，请检查路径是否正确
#include "limit_order_book.h" 

using namespace hft;

// 创建一个测试夹具（Test Fixture），为每个测试用例提供干净的环境
class LimitOrderBookTest : public ::testing::Test {
protected:
    LimitOrderBook book;
    std::vector<Trade> trades; // 用来存放每次测试产生的所有成交回报

    // SetUp 会在每个 TEST_F 运行前自动执行，相当于初始化
    void SetUp() override {
        // 注册回调，把产生的成交明细存进 trades 数组里方便后续断言检查
        book.registerTradeCallback([this](const Trade& t) {
            trades.push_back(t);
        });
    }
};

// 测试用例 1：挂入一个纯买单（Maker），不应该产生任何成交
TEST_F(LimitOrderBookTest, AddMakerOrder) {
    Order o1(1, 100, 10, Side::BUY);
    book.addOrder(o1);

    EXPECT_EQ(book.bids_.size(), 1);               // 确保买单簿有 1 个档位
    EXPECT_EQ(book.bids_.at(100).total_qty, 10);      // 确保该档位总数量是 10
    EXPECT_TRUE(book.asks_.empty());               // 卖单簿必须是空的
    EXPECT_TRUE(trades.empty());                   // 绝不能触发成交回调
}

// 测试用例 2：买卖单撮合匹配，测试部分成交
TEST_F(LimitOrderBookTest, MatchTakerOrder) {
    // 挂入买单 10 个
    Order o1(1, 100, 10, Side::BUY);
    book.addOrder(o1);

    // 挂入卖单 5 个去砸盘
    Order o2(2, 100, 5, Side::SELL);
    book.addOrder(o2);

    // 验证成交回调
    ASSERT_EQ(trades.size(), 1);                   // 应该恰好产生一笔成交
    EXPECT_EQ(trades[0].qty, 5);                   // 成交数量应该是 5
    EXPECT_EQ(trades[0].price, 100);               // 成交价 100
    EXPECT_EQ(trades[0].buy_order_id, 1);
    EXPECT_EQ(trades[0].sell_order_id, 2);

    // 验证单簿残余状态
    EXPECT_EQ(book.bids_.at(100).total_qty, 5);       // 买单被吃掉 5 个，还剩 5 个
    EXPECT_TRUE(book.asks_.empty());               // 卖单全被吃光，不应该挂入卖单簿
}

// 测试用例 3：极速撤单功能验证
TEST_F(LimitOrderBookTest, CancelOrder) {
    Order o1(1, 100, 10, Side::BUY);
    book.addOrder(o1);

    // 撤销订单 ID 1
    book.cancelOrder(1);
    
    // 验证撤单状态
    EXPECT_EQ(book.bids_.at(100).total_qty, 0);       // 库存总数必须扣减为 0
    EXPECT_TRUE(book.bids_.at(100).orders[0].is_canceled); // 墓碑标记必须为 true
}