class Request {
public:
    int code;
    std::time_t launchTime;
    std::time_t handleTime;

    Request(int c, std::time_t lt) : code(c), launchTime(lt) {}

    void setHandleTime(std::time_t ht) {
        handleTime = ht;
    }

    std::time_t getLaunchTime() {
        return launchTime;
    }

    std::time_t getHandleTime() {
        return handleTime;
    }

    int getCode() {
        return code;
    }
};

class TokenBucketLimiter {
private:
    int capacity;      // 令牌桶容量
    int rate;          // 令牌生成速率
    int tokenAmount;   // 令牌数量
    std::mutex mtx;    // 线程同步用的互斥量

public:
    TokenBucketLimiter(int capacity, int rate) : capacity(capacity), rate(rate), tokenAmount(capacity) {
        std::thread([this]() {
            // 以恒定速率放令牌
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / this->rate));
                std::lock_guard<std::mutex> lock(mtx);
                this->tokenAmount++;
                if (this->tokenAmount > this->capacity) {
                    this->tokenAmount = this->capacity;
                }
            }
        }).detach();
    }

    bool tryAcquire(Request* request) {
        std::lock_guard<std::mutex> lock(mtx);
        if (tokenAmount > 0) {
            tokenAmount--;
            handleRequest(request);
            return true;
        } else {
            return false;
        }
    }

    void handleRequest(Request* request) {
        std::time_t now = std::time(nullptr);
        request->setHandleTime(now);
        // std::cout << request->getCode() << "号请求被处理，请求发起时间："
        //           << std::ctime(request->getLaunchTime()) << ", 请求处理时间："
        //           << std::ctime(request->getHandleTime()) << ", 处理耗时："
        //           << (request->getHandleTime() - request->getLaunchTime()) << "秒" << std::endl;
    }
};


int main() {
    TokenBucketLimiter tokenBucketLimiter(5, 2);
    for (int i = 1; i <= 10; i++) {
        Request* request = new Request(i, std::time(nullptr));
        if (tokenBucketLimiter.tryAcquire(request)) {
            std::cout << i << "号请求被接受" << std::endl;
        } else {
            std::cout << i << "号请求被拒绝" << std::endl;
        }
        delete request;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟请求到达的时间间隔
    }

    cout<<endl;
    return 0;
}
// 运行结果
// 1号请求被接受
// 2号请求被接受
// 3号请求被接受
// 4号请求被接受
// 5号请求被接受
// 6号请求被接受
// 7号请求被拒绝
// 8号请求被拒绝
// 9号请求被拒绝
// 10号请求被拒绝
