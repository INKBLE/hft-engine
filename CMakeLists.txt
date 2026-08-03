cmake_minimum_required(VERSION 3.28)
project(LowLatencyMatchingEngine VERSION 1.0)

# 1. 核心编译配置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

include_directories(${CMAKE_SOURCE_DIR}/include)

# 2. 编译主程序
add_executable(hft_main src/main.cpp)

# ==========================================
# 3. 引入测试框架 (FetchContent 魔法)
# 面试考点：用 FetchContent 替代手动安装，保证任何人在任何电脑上 clone 代码后都能一键编译。
# ==========================================
include(FetchContent)

# 拉取 GoogleTest (用于验证撮合逻辑是否正确)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# 拉取 Google Benchmark (用于测试纳秒级延迟)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE) # 关掉框架自带的测试，加快编译
FetchContent_MakeAvailable(googlebenchmark)

# ==========================================
# 4. 配置测试可执行文件
# ==========================================
# 编译功能单元测试
add_executable(hft_test tests/test_basic.cpp)
target_link_libraries(hft_test GTest::gtest_main)

# 编译性能基准测试
add_executable(hft_bench bench/bench_basic.cpp)
target_link_libraries(hft_bench benchmark::benchmark benchmark::benchmark_main)