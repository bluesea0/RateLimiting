class SlidingWindowRateLimiterMinute {
public:
    SlidingWindowRateLimiterMinute(int subCycle, int thresholdPerMin)
        : SUB_CYCLE(subCycle), thresholdPerMin(thresholdPerMin) {}

    // 滑动窗口时间算法实现
    bool slidingWindowsTryAcquire() {        
        // 获取当前时间在哪个小周期窗口
        auto now = std::chrono::system_clock::now();
        auto currentWindowTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / SUB_CYCLE * SUB_CYCLE;
        int currentWindowNum = countCurrentWindow(currentWindowTime); // 当前窗口总请求数

        // 超过阀值限流
        if (currentWindowNum >= thresholdPerMin) {
            return false;
        }

        // 计数器+1
        counters[currentWindowTime]++;
        return true;
    }

private:
    // 统计当前窗口的请求数
    // 时间复杂度是O(NlogN)，删除元素是O(logN),最坏是删除N个元素
    int countCurrentWindow(long currentWindowTime) {
        // 计算窗口开始位置，总的大窗口开始的位置，相较于现在减去50s，现在一定是在第6个窗口
        // long startTime = currentWindowTime - SUB_CYCLE * (60 / SUB_CYCLE - 1); // 按分钟来控制
        
        // 按s来控制qps，那么分隔的时间单位需要是ms，获取到本大窗口开头的那个时间
        long startTime = currentWindowTime - SUB_CYCLE * (1000 / SUB_CYCLE - 1);

        int count = 0;

        // 遍历存储的计数器
        auto it = counters.begin();
        while (it != counters.end()) {
            if (it->first < startTime) {
                // 删除无效过期的子窗口计数器
                it = counters.erase(it);
            } else {
                // 累加当前窗口的所有计数器之和
                count += it->second;
                ++it;
            }
        }
        return count;
    }

    const int SUB_CYCLE; // 单位时间划分的小周期（单位时间是1分钟，10s一个小格子窗口，一共6个格子）
    const int thresholdPerMin; // 每分钟限流请求数
    std::map<long, int> counters; // 计数器, key为当前窗口的开始时间值秒，value为当前窗口的计数 // 关键1: 用map来存储时间戳->请求个数
};


// 优化版本，只使用一个数组保存，不使用map保存，循环数组的方式来限制
class CounterSlidingWindowLimiter {
private:
    int windowSize;  // 窗口大小，毫秒为单位
    int limit;  // 窗口内限流大小
    int splitNum;  // 切分小窗口的数目大小
    std::vector<int> counters;  // 每个小窗口的计数数组
    int index;  // 当前小窗口计数器的索引
    long long startTime;  // 窗口开始时间

public:
    CounterSlidingWindowLimiter(int windowSize, int limit, int splitNum) 
        : windowSize(windowSize), limit(limit), splitNum(splitNum), 
          counters(splitNum, 0), index(0), 
          startTime(std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1)) {}

    // 请求到达后先调用本方法，若返回true，则请求通过，否则限流
    bool tryAcquire() {
        // 当前的ms时间戳
        long long curTime = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
        // 假设windowSize=1000, splitNum=2, startTime=0,  限制每秒qps最大是2
        // curTime = 1时，windowsNum=0;
        // curTime = 501时，windowsNum= (501-0)/500=1
        // curTime = 502时，windowsNum = (502-500)/500=0 ,返回失败，被限制
        // curTime = 1001时，windowsNum = (1001-500)/500=1，更新起始时间戳，返回成功
        // curTime = 2001时，windowsNum = （2001-500）/500 = 3，更新起始时间戳，返回成功
        long long windowsNum = std::max(curTime - startTime, 0LL) / (windowSize / splitNum);// 计算在第几个小窗口中
        slideWindow(windowsNum);

        int count = 0;
        // 计算大窗口下的请求量
        for (int i = 0; i < splitNum; i++) {
            count += counters[i];
        }

        if (count >= limit) {
            return false;
        } else {
            counters[index]++;
            return true;
        }
    }

private:
    void slideWindow(long long windowsNum) {
        if (windowsNum == 0) {
            return;
        }

        long long slideNum = std::min(windowsNum, static_cast<long long>(splitNum));
        // index到目前的slideNum的距离的时间都过去了
        for (int i = 0; i < slideNum; i++) {
            index = (index + 1) % splitNum;
            counters[index] = 0;
        }

        startTime += windowsNum * (windowSize / splitNum);// 触发窗口向右移动
    }
};

