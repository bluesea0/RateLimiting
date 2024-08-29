#include <iostream>
#include <chrono>
#include <mutex>
#include <algorithm>

class LeakyBucket {
private:
    long capacity;    // 桶的容量
    long rate;        // 漏桶出水速率
    long water;       // 当前桶中的水量
    long lastLeakTimestamp; // 上次漏水时间戳
    std::mutex mtx;   // 互斥锁用于线程安全

public:
    LeakyBucket(long capacity, long rate) 
        : capacity(capacity), rate(rate), water(0) {
        lastLeakTimestamp = getCurrentTimeMillis();
    }

    /**
     * 尝试向桶中放入一定量的水，如果桶中还有足够的空间，则返回 true，否则返回 false。
     */
    bool tryConsume(long waterRequested) {
        std::lock_guard<std::mutex> lock(mtx); // 自动管理锁的生命周期
        leak();
        if (water + waterRequested <= capacity) {
            water += waterRequested;
            return true;
        } else {
            return false;
        }
    }

private:
    /**
     * 漏水，根据当前时间和上次漏水时间戳计算出应该漏出的水量，
     * 然后更新桶中的水量和漏水时间戳等状态。
     */
    void leak() {
        long now = getCurrentTimeMillis();
        long elapsedTime = now - lastLeakTimestamp;
        long leakedWater = elapsedTime * rate / 1000;
        if (leakedWater > 0) {
            water = std::max(0L, water - leakedWater);
            lastLeakTimestamp = now;
        }
    }

    /**
     * 获取当前时间的毫秒数
     */
    long getCurrentTimeMillis() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
};

int main() {
    LeakyBucket bucket(10, 1); // 创建一个容量为10，出水速率为1的漏桶

    // 模拟请求
    if (bucket.tryConsume(5)) {
        std::cout << "Request 1 succeeded." << std::endl;
    } else {
        std::cout << "Request 1 failed." << std::endl;
    }

    // 模拟延迟后再进行请求
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (bucket.tryConsume(6)) {
        std::cout << "Request 2 succeeded." << std::endl;
    } else {
        std::cout << "Request 2 failed." << std::endl;
    }

    return 0;
}

// 运行结果
// Request 1 succeeded.
// Request 2 succeeded.
