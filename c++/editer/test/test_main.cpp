#include "test_framework.hpp"
#include <iostream>
#include <chrono>

int main(int argc, char* argv[]) {
    // TODO: 打印测试开始信息
    // 提示：显示程序名称、时间戳等
    std::cout << "================================\n";
    std::cout << "Editer Test Suite\n";
    std::cout << "================================\n\n";

    // TODO: 记录开始时间
    // 提示：使用 std::chrono 测量执行时间
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // TODO: 运行所有测试
    // 提示：调用 ::editer::test::run_all()
    int result = ::editer::test::run_all();
    
    // TODO: 记录结束时间并计算耗时
    // 提示：std::chrono::high_resolution_clock::now() - start_time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // TODO: 打印测试总耗时
    // 提示：显示用时（毫秒）
    std::cout << "Total time: " << duration.count() << " ms\n\n";
    
    // TODO: 根据结果打印最终消息
    // 提示：成功返回 0，失败返回非 0
    if (result == 0) {
        std::cout << "[SUCCESS] All tests passed!\n";
    } else {
        std::cout << "[FAILURE] Some tests failed!\n";
    }
    
    return result;
}