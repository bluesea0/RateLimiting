#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <ctime>

class Request {
public:
    int code;
    std::time_t launchTime;
    std::time_t handleTime;

    Request(int code, std::time_t launchTime) 
        : code(code), launchTime(launchTime) {}

    void setHandleTime(std::time_t handleTime) {
        this->handleTime = handleTime;
    }
};

class LeakyBucketLimiter {
private:
    int capacity;  // 漏斗容量
    int rate;      // 漏斗速率
    int left;      // 剩余容量
    std::queue<Request*> requestQueue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    LeakyBucketLimiter(int capacity, int rate) 
        : capacity(capacity), rate(rate), left(capacity) {
        // 开启一个定时线程，以固定的速率将漏斗中的请求流出，进行处理
        std::thread([this]() {
            while (true) {
                std::unique_lock<std::mutex> lock(this->mtx);
                cv.wait_for(lock, std::chrono::milliseconds(1000 / rate), [this] {
                    return !requestQueue.empty();
                });

                if (!requestQueue.empty()) {
                    Request* request = requestQueue.front();
                    requestQueue.pop();
                    handleRequest(request);
                }
            }
        }).detach();
    }

    // 尝试添加请求到漏斗
    bool tryAcquire(Request* request) {
        std::lock_guard<std::mutex> lock(mtx);
        if (left <= 0) {
            return false;
        } else {
            left--;
            requestQueue.push(request);
            cv.notify_all();
            return true;
        }
    }

private:
    // 处理请求
    void handleRequest(Request* request) {
        request->setHandleTime(std::time(nullptr));
        std::cout << request->code << "号请求被处理，请求发起时间："
                  << std::ctime(&request->launchTime)
                  << "请求处理时间：" << std::ctime(&request->handleTime)
                  << "处理耗时：" << difftime(request->handleTime, request->launchTime) 
                  << "秒" << std::endl;
    }
};

int main() {
    LeakyBucketLimiter leakyBucketLimiter(5, 2);

    for (int i = 1; i <= 10; ++i) {
        Request* request = new Request(i, std::time(nullptr));
        if (leakyBucketLimiter.tryAcquire(request)) {
            std::cout << i << "号请求被接受" << std::endl;
        } else {
            std::cout << i << "号请求被拒绝" << std::endl;
            delete request;  // 被拒绝的请求需要手动删除，以防止内存泄漏
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟请求间隔
    }

    // 延迟主线程结束，等待处理所有请求
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
// 这个跑出来的结果有点问题，没按规则限流住，可能是wait_for那里时间等待的有问题。但总体思路是这样的。开启单独的一个线程定时处理。



——————————————————————————————————————————————————————————
// 我认为这个版本的实现不是漏桶，实际还是按照tryConsume是批量放入，并不是匀速的。
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
