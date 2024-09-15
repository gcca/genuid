cmake_minimum_required(VERSION 3.30)
project(genuid VERSION 1.0 LANGUAGES CXX)

add_library(genuid OBJECT genuid.cpp)

find_package(benchmark REQUIRED)
add_executable(genuid-bench genuid-bench.cc)
target_link_libraries(genuid-bench benchmark::benchmark_main genuid)

find_package(GTest REQUIRED)
add_executable(genuid-test genuid-test.cc)
target_link_libraries(genuid-test GTest::gtest_main genuid)
enable_testing()
gtest_discover_tests(genuid-test)
