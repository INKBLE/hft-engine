# HFT Engine

一个以 C++20 实现的限价订单簿原型，聚焦价格—时间优先撮合路径中的低延迟内存管理。项目提供可执行演示、GoogleTest 正确性测试和 Google Benchmark 微基准，适合用于研究订单簿基本数据结构与延迟敏感的分配策略。

## 核心特性

- **限价订单撮合**：买方按最高价、卖方按最低价维护价格档位；可成交订单按价格交叉条件逐档撮合。
- **成交回调**：每次成交以 `Trade`（价格、数量、买卖订单 ID）同步通知调用方。
- **部分成交与剩余挂单**：主动单可消耗多个订单；未成交余量进入对应价格档位。
- **快速撤单**：订单 ID 映射到 `(价格、方向、档位内下标)`，撤单无需扫描整个订单簿。
- **预分配节点内存**：买卖两侧各自使用固定槽位空闲链表，并通过 `PoolAllocator` 供 `std::map` 节点使用；构造时写入预分配区以提前触发物理页分配。
- **可重复验证**：GTest 覆盖挂单、部分成交与墓碑撤单；Google Benchmark 提供空簿/同价/随机价挂单及吃单场景。

## 架构与设计原理

```text
Order -> LimitOrderBook::addOrder()
             |-- bids_ : 价格降序 map<Price, PriceLevel>
             |-- asks_ : 价格升序 map<Price, PriceLevel>
             |-- order_map_[OrderId] -> OrderLocation
             `-- on_trade_(Trade)
```

### 订单簿与队列

- `Price`、`Quantity` 和 `OrderId` 都是 `uint64_t`，方向由 `Side` 表示。
- `PriceLevel` 保存聚合数量、`std::vector<Order>` 和 `head_index`。成交或撤销的历史订单以 `is_canceled` 标记为墓碑；撮合时从 `head_index` 前进，避免在热路径中移动 vector 元素。
- 买单扫描最低卖价，卖单扫描最高买价。成交价格取账簿中 Maker 所在档位的价格。

### 内存管理

`MemoryPool` 在初始化时分配连续字节区，将固定大小槽位串为单链表；`allocate()`/`deallocate()` 只在空闲链表头部摘取或归还节点。两侧 `std::map` 分别持有自己的池，避免买卖档位竞争同一分配器。传入 `LimitOrderBook(max_orders)` 会提前建立并写入 `order_map_`，以降低首次访问页面缺页带来的抖动。

> 当前实现的池容量由构造参数固定（每侧 1,024 个、每槽 96 字节），`PoolAllocator` 假设 map 每次请求一个对象；将其用于其他容器或超过池容量的档位数前，应先扩展容量与边界检查策略。

## 环境与构建命令

**要求**：支持 C++20 的编译器、CMake ≥ 3.28，以及可访问 GitHub 的网络连接。CMake 会通过 `FetchContent` 下载 GoogleTest `v1.14.0` 和 Google Benchmark `v1.8.3`。

```bash
cd /home/inkble/my_projects/hft-engine
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

项目在 CMake 中启用 `-O3 -march=native -Wall -Wextra`；`-march=native` 生成的二进制仅保证在构建机兼容。若要分发到其他 CPU，请在 `CMakeLists.txt` 中替换该选项。

### 验证标准

```bash
# 功能演示
./build/hft_main

# GTest：断言档位聚合量、成交回报及撤单墓碑状态
./build/hft_test

# 微基准：每个已登记场景固定 100,000 次迭代
./build/hft_bench
```

仓库当前未配置 `enable_testing()`/`add_test()`，因此应直接运行 `hft_test`，而不是依赖 `ctest`。基准结果受 CPU、编译器、频率调度和 `-march=native` 影响；请在目标机器上运行后再比较数值。

## 使用示例

```cpp
#include "limit_order_book.h"
#include <iostream>

int main() {
    hft::LimitOrderBook book(2'000'000);
    book.registerTradeCallback([](const hft::Trade& trade) {
        std::cout << trade.price << " x " << trade.qty << '\n';
    });

    book.addOrder(hft::Order{1, 100, 10, hft::Side::SELL});
    book.addOrder(hft::Order{2, 100, 15, hft::Side::BUY}); // 成交 10，余量 5 挂入 bids_
    book.cancelOrder(2);
}
```

完整演示见 [`src/main.cpp`](src/main.cpp)，测试和基准分别位于 [`tests/test_basic.cpp`](tests/test_basic.cpp) 与 [`bench/bench_basic.cpp`](bench/bench_basic.cpp)。