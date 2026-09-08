#include "limit_order_book.h"
#include <benchmark/benchmark.h>
#include <random>

using namespace hft;

// ── 场景0：空簿挂单基准 ─────────────────────────────────
static void BM_AddMakerOrder_EmptyBook(benchmark::State& state)
{
    LimitOrderBook book(2200000);
    OrderId id = 1;
    for (auto _ : state)
    {
        book.addOrder(Order(id++, 100, 10, Side::BUY));
    }
}
BENCHMARK(BM_AddMakerOrder_EmptyBook)->Iterations(100000);

// ── 场景1：稳态挂单（同一价格） ──────────────────────────
static void BM_AddMakerOrder_SamePrice(benchmark::State& state)
{
    LimitOrderBook book(2200000);
    for (int i = 0; i < 1000; ++i)
    {
        book.addOrder(Order(i, 100, 10, Side::BUY));
    }
    OrderId new_id = 1000000;
    for (auto _ : state)
    {
        book.addOrder(Order(new_id++, 100, 10, Side::BUY));
    }
}
BENCHMARK(BM_AddMakerOrder_SamePrice)->Iterations(100000);

// ── 场景2：随机价格挂单 ─────────────────────────────────
static void BM_AddMakerOrder_RandomPrice(benchmark::State& state)
{
    LimitOrderBook book(2200000);
    std::mt19937 rng(42);
    std::uniform_int_distribution<Price> dist(95, 105);
    for (int i = 0; i < 1000; ++i)
    {
        book.addOrder(Order(i, dist(rng), 10, Side::BUY));
    }
    OrderId new_id = 1000000;
    for (auto _ : state)
    {
        book.addOrder(Order(new_id++, dist(rng), 10, Side::BUY));
    }
}
BENCHMARK(BM_AddMakerOrder_RandomPrice)->Iterations(100000);

// ── 场景3：撮合测试 ─────────────────────────────────────
static void BM_MatchTakerOrder(benchmark::State& state)
{
    LimitOrderBook book(2200000);
    book.registerTradeCallback([](const Trade&) {});
    for (int i = 0; i < 1000; ++i)
    {
        book.addOrder(Order(i, 100, 10, Side::BUY));
    }
    OrderId sell_id = 2000000;
    for (auto _ : state)
    {
        book.addOrder(Order(sell_id++, 100, 10, Side::SELL));
    }
}
BENCHMARK(BM_MatchTakerOrder)->Iterations(100000);

BENCHMARK_MAIN();
