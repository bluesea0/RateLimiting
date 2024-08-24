#include<iostream>
#include<atomic>
#include<thread>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

class RateLimiterSimpleWindow {
public:
    RateLimiterSimpleWindow(int qps, long timeWindow)
        : QPS(qps), TIME_WINDOWS(timeWindow), REQ_COUNT(0), START_TIME(std::chrono::steady_clock::now()) {}
    
    // 优化: 没有单独开一个线程去每隔 1 秒重置计数器，而是在每次调用时进行时间间隔计算来确定是否先重置计数器。
    bool tryAcquire() {
        auto now = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - START_TIME).count();

        if (elapsedTime > TIME_WINDOWS) {
            REQ_COUNT = 0;
            START_TIME = now;
        }

        return REQ_COUNT.fetch_add(1) < QPS;
    }

private:
    int QPS; // 阈值
    long TIME_WINDOWS; // 时间窗口（毫秒）
    std::atomic<int> REQ_COUNT; // 计数器
    std::chrono::time_point<std::chrono::steady_clock> START_TIME; // 开始时间
};

int main() {
    RateLimiterSimpleWindow rateLimiter(2, 1000); // QPS = 2, TIME_WINDOWS = 1000ms

    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);

        if (!rateLimiter.tryAcquire()) {
            std::cout << std::put_time(now_tm, "%H:%M:%S") << " 被限流" << std::endl;
        } else {
            std::cout << std::put_time(now_tm, "%H:%M:%S") << " 做点什么" << std::endl;
        }
    }

    cout<<endl;
    return 0;
}

// 运行结果
// 22:46:54 做点什么
// 22:46:55 做点什么
// 22:46:55 被限流
// 22:46:55 被限流
// 22:46:55 做点什么
// 22:46:56 做点什么
// 22:46:56 被限流
// 22:46:56 被限流
// 22:46:56 被限流
// 22:46:57 做点什么
