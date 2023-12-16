#ifndef FLEXMINGEN_H
#define FLEXMINGEN_H

using namespace std;

#include <iostream>
#include <vector>
#include <set>
#define FLEXMINGEN_SIZE 128

unsigned int cache1 = 1;
struct DInterval{
    uint64_t start;
    uint64_t end;
    uint64_t PC_val;

    // 重载 < 操作符,用于塞入set
    bool operator<(const DInterval& other) const {
        return end < other.end ||
               (end == other.end && start < other.start) ||
               (start == other.start && end == other.end && PC_val < other.PC_val);
    }
};

struct PInterval{
    uint64_t start;
    uint64_t end;
    uint32_t counter;
    set<DInterval> hurt_d;

    bool is_inval(uint64_t val){
        if(val >= start && val <= end){
            return true;
        }
        return false;
    }
};

// is_cache返回值
struct ret_ic{
    bool cache;
    bool is_reset;
    set<uint64_t> PC_vals;
};

vector<PInterval> p_intervals;

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
    ret_ic is_cache(uint64_t val, uint64_t endVal, uint32_t type, uint32_t led, uint64_t PC_val){
        
        ret_ic r_ic;

        if(type == PREFETCH){// 这里是harmony的策略。实际测试发现，如果综合使用harmony和homony的策略，效果可能会更好
            if((val - endVal) >= led){
                return {false,false};
            }
        }

        cache1 = 1;
        unsigned int count = endVal;
        while (count != val){
            if(liveness_intervals[count] >= cache_size){
                if(type != PREFETCH){
                    for(auto it = p_intervals.begin(); it != p_intervals.end(); it++){
                        if(++it->counter > 10){
                            p_intervals.erase(it);
                            continue;
                        }
                        if(it->is_inval(count)){
                            DInterval d;
                            d.start = endVal;
                            d.end = val;
                            d.PC_val = PC_val;
                            it->hurt_d.insert(d);
                        }
                    }
                }
                cache1 = 0;
            }
            count = (count+1) % liveness_intervals.size();
        }

        if(cache1){
            count = endVal;
            while(count != val){
                liveness_intervals[count]++;
                count = (count+1) % liveness_intervals.size();
            }
            num_cache++;
            if(type == PREFETCH){
                
                PInterval p = {endVal, val, 0};
                p_intervals.push_back(p);
            }
            return {true, false};
        }
        else{
            if(type != PREFETCH){
                for(auto it = p_intervals.begin(); it != p_intervals.end(); it++){
                    if(it->hurt_d.size() >= 2){
                        // 恢复状态
                        count = it->start;
                        while(count != it->end){
                            liveness_intervals[count]--;
                            count = (count+1) % liveness_intervals.size();
                        }
                        num_cache--;

                        r_ic.PC_vals = reset_DIntervals(it->hurt_d);
                        p_intervals.erase(it);
                    }
                }
                r_ic.cache = false;
                r_ic.is_reset = true;
                return r_ic;
            }
        }
    }

    set<uint64_t> reset_DIntervals(set<DInterval> intervals){
        set<uint64_t> PC_vals;
        for(auto it = intervals.begin(); it != intervals.end(); it++){
            bool cache = true;
            unsigned int count = it->start;
            unsigned int endval = it->end;
            while (count != endval){
                if(liveness_intervals[count] >= cache_size){
                    cache = false;
                    break;
                }
                count = (count+1) % liveness_intervals.size();
            }
            if(cache){
                count = it->start;
                while(count != endval){
                    liveness_intervals[count]++;
                    count = (count+1) % liveness_intervals.size();
                }
                num_cache++;
                PC_vals.insert(it->PC_val);
            }
        }
        return PC_vals;
    }
};

#endif