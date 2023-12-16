#ifndef FLEXMINGEN_H
#define FLEXMINGEN_H

using namespace std;

#include <vector>
#define FLEXMINGEN_SIZE 128

struct FlexMINgen{
    vector<unsigned int> liveness_intervals; // 存储活跃区间的向量
    uint64_t num_cache; // 缓存命中次数
    uint64_t access; // 访问次数
    uint64_t cache_size; // 缓存大小

    // 初始化值
    void init(uint64_t size){
        num_cache = 0;
        access = 0;
        cache_size = size;
        liveness_intervals.resize(FLEXMINGEN_SIZE, 0);
    }

    // 返回缓存命中次数
    uint64_t get_flexmingen_hits(){
        return num_cache;
    }

    // 设置访问
    void set_access(uint64_t val){
        access++;
        liveness_intervals[val] = 0;
    }

    // 设置预取
    void set_prefetch(uint64_t val){
        liveness_intervals[val] = 0;
    }

    // 判断是否缓存命中
    bool is_cache(uint64_t val, uint64_t endVal, uint32_t type, uint32_t led){
        
        // mark 新增
        if(type == PREFETCH){
            if((val - endVal) >= led){
                return false;
            }
        }

        bool cache = true;
        unsigned int count = endVal;
        while (count != val){
            if(liveness_intervals[count] >= cache_size){
                cache = false;
                break;
            }
            count = (count+1) % liveness_intervals.size();
        }

        if(cache){
            count = endVal;
            while(count != val){
                liveness_intervals[count]++;
                count = (count+1) % liveness_intervals.size();
            }
            num_cache++;
        }
        return cache;
    }
};

#endif