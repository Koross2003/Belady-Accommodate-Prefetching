/*  Harmony with Belady's Algorithm Replacement Policy
    Code for Harmony configurations of 1 and 2 in Champsim */
/* Harmony使用Belady的替换算法
适用于Champsim中Harmony配置1和2的代码 */

#include "../inc/champsim_crc2.h"
#include <map>
#include <math.h>

#define NUM_CORE 1
#define LLC_SETS NUM_CORE*2048
#define LLC_WAYS 16

//3-bit RRIP counter
//3-bit RRIP 计数器
#define MAXRRIP 7
uint32_t rrip[LLC_SETS][LLC_WAYS];

// mark 新增四个计数器
uint64_t dtotal[NUM_CORE]; // demand-miss间隔总长
uint32_t dcount[NUM_CORE]; // demand-miss间隔个数
uint64_t ptotal[NUM_CORE]; // 
uint32_t pcount[NUM_CORE];

// mark 计算demand miss
uint64_t demand_access = 0;
uint64_t demand_hit = 0;

#include "harmony_predictor.h"
#include "flexmingen.h"
#include "helper_function.h"

//Harmony predictors for demand and prefetch requests
// Harmony对需求和预取请求的预测器
Harmony_Predictor* predictor_demand;    // 2K条目，每个条目5位计数器
Harmony_Predictor* predictor_prefetch;  //2K entries, 5-bit counter per each entry

FlexMINgen flexmingen_occup_vector[LLC_SETS];   //64 vecotrs, 128 entries each

//Prefectching
bool prefetching[LLC_SETS][LLC_WAYS];

//Sampler components tracking cache history
//采样器组件跟踪缓存历史
#define SAMPLER_ENTRIES 2800 * NUM_CORE
#define SAMPLER_HIST 8
#define SAMPLER_SETS SAMPLER_ENTRIES/SAMPLER_HIST
vector<map<uint64_t, HISTORY>> cache_history_sampler;  //2800 entries, 4-bytes per each entry //2800条目，每个条目4字节
uint64_t sample_signature[LLC_SETS][LLC_WAYS];

//History time
#define TIMER_SIZE 1024
uint64_t set_timer[LLC_SETS];   // 64个组，每个组使用1个定时器

//Mathmatical functions needed for sampling set
#define bitmask(l) (((l) == 64) ? (unsigned long long)(-1LL) : ((1LL << (l))-1LL))
#define bits(x, i, l) (((x) >> (i)) & bitmask(l))
//Helper function to sample 64 sets for each core
// 前6位和后6位相同的组是采样集
#define SAMPLED_SET(set) (bits(set, 0 , 6) == bits(set, ((unsigned long long)log2(LLC_SETS) - 6), 6) )


// Initialize replacement state
void InitReplacementState()
{
    cout << "Initialize Harmony replacement policy state" << endl;

    for (int i=0; i<LLC_SETS; i++) {
        for (int j=0; j<LLC_WAYS; j++) {
            rrip[i][j] = MAXRRIP;
            sample_signature[i][j] = 0;
            prefetching[i][j] = false;

            // // mark 新增
            // dtotal[i][j] = 0;
            // dcount[i][j] = 0;
            // ptotal[i][j] = 0;
            // pcount[i][j] = 0;
        }
        set_timer[i] = 0;
        flexmingen_occup_vector[i].init(LLC_WAYS-2);
    }

    // mark 新增
    for(int i=0;i<NUM_CORE;i++){
        dtotal[i] = 0;
        dcount[i] = 0;
        ptotal[i] = 0;
        pcount[i] = 0;
    }

    cache_history_sampler.resize(SAMPLER_SETS);
    for(int i = 0; i < SAMPLER_SETS; i++){
        cache_history_sampler[i].clear();
    }

    predictor_prefetch = new Harmony_Predictor();
    predictor_demand = new Harmony_Predictor();

    cout << "Finished initializing Harmony replacement policy state" << endl;
}

// Find replacement victim
// Return value should be 0 ~ 15 or 16 (bypass)(16表示跳过不替换?)
uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    //Find the line with RRPV of 7 in that set
    // 如果找到rrpv = 7 的行,直接返回victim(因为是cache-averse的)
    for(uint32_t i = 0; i < LLC_WAYS; i++){
        if(rrip[set][i] == MAXRRIP){
            return i;
        }
    }

    //If no RRPV of 7, then we find next highest RRPV value (oldest cache-friendly line)
    uint32_t max_rrpv = 0;
    int32_t victim = -1;
    for(uint32_t i = 0; i < LLC_WAYS; i++){
        if(rrip[set][i] >= max_rrpv){
            max_rrpv = rrip[set][i];
            victim = i;
        }
    }

    //Predictor will be trained negaively on evictions
    // sample set是采样集
    // 驱逐时,预测器会被负面训练
    if(SAMPLED_SET(set)){
        if(prefetching[set][victim]){
            predictor_prefetch->decrease(sample_signature[set][victim]);
        }
        else{
            predictor_demand->decrease(sample_signature[set][victim]);
        }
    }

    return victim;
}

//Helper function for "UpdateReplacementState" to update cache history
// 更新cache历史
void update_cache_history(unsigned int sample_set, unsigned int currentVal){
    for(map<uint64_t, HISTORY>::iterator it = cache_history_sampler[sample_set].begin(); it != cache_history_sampler[sample_set].end(); it++){
        if((it->second).lru < currentVal){
            (it->second).lru++;
        }
    }

}

// Called on every cache hit and cache fill
// cache hit 或者 cache fill 时调用
void UpdateReplacementState (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    paddr = (paddr >> 6) << 6; // 低6位清零

    //Ignore all types that are writebacks
    if(type == WRITEBACK){
        return;
    }

    if(type == PREFETCH){
        if(!hit){
            prefetching[set][way] = true;
        }
    }
    else{
        prefetching[set][way] = false;
    }

    //Only if we are using sampling sets for FlexMINgen
    if(SAMPLED_SET(set)){
        uint64_t sample_tag = CRC(paddr >> 12) % 256;
        uint32_t sample_set = (paddr >> 6) % SAMPLER_SETS;
        uint64_t currentVal = set_timer[set] % FLEXMINGEN_SIZE;
        uint64_t previousVal = cache_history_sampler[sample_set][sample_tag].previousVal % FLEXMINGEN_SIZE;
        uint64_t interval_len = currentVal - previousVal;
        // uint64_t led = (dtotal[cpu] / dcount[cpu]) / (ptotal[cpu] / pcount[cpu]);
        uint64_t led = 2.5 * DIV(DIV(dtotal[cpu], dcount[cpu]), DIV(ptotal[cpu], pcount[cpu]));


        
        // *-D的间隔
        if((type != PREFETCH) && (cache_history_sampler[sample_set].find(sample_tag) != cache_history_sampler[sample_set].end())){

            demand_access++;

            unsigned int current_time = set_timer[set];
            if(current_time < cache_history_sampler[sample_set][sample_tag].previousVal){
                current_time += TIMER_SIZE;
            }
            //这个warp实际上是判断time是否走到下一轮的(是否发生时间轮回)
            bool isWrap = (current_time - cache_history_sampler[sample_set][sample_tag].previousVal) > FLEXMINGEN_SIZE;

            //Train predictor positvely for last PC value that was prefetched
            // 没有发生时间轮回且这一行在cache中
            if(!isWrap && flexmingen_occup_vector[set].is_cache(currentVal, previousVal, type, led)){
                demand_hit++;
                if(cache_history_sampler[sample_set][sample_tag].prefetching){//判断这一行是不是被预取进来的(即是不是P-D)
                    predictor_prefetch->increase(cache_history_sampler[sample_set][sample_tag].PCval);
                }
                else{//D-D
                    predictor_demand->increase(cache_history_sampler[sample_set][sample_tag].PCval);
                }
            }
            //Train predictor negatively since OPT did not cache this line
            //没命中,负训练
            else{
                if(!isWrap){
                    dtotal[cpu] += interval_len;
                    dcount[cpu]++;
                }

                if(cache_history_sampler[sample_set][sample_tag].prefetching){
                    predictor_prefetch->decrease(cache_history_sampler[sample_set][sample_tag].PCval);
                }
                else{
                    predictor_demand->decrease(cache_history_sampler[sample_set][sample_tag].PCval);
                }
            }
            
            flexmingen_occup_vector[set].set_access(currentVal);
            //Update cache history
            update_cache_history(sample_set, cache_history_sampler[sample_set][sample_tag].lru);
            //Mark prefetching as false since demand access
            cache_history_sampler[sample_set][sample_tag].prefetching = false;
        }
        //If line has not been used before, mark as prefetch or demand
        else if(cache_history_sampler[sample_set].find(sample_tag) == cache_history_sampler[sample_set].end()){
            //If sampling, find victim from cache
            if(cache_history_sampler[sample_set].size() == SAMPLER_HIST){
                //Replace the element in the cache history
                uint64_t addr_val = 0;
                for(map<uint64_t, HISTORY>::iterator it = cache_history_sampler[sample_set].begin(); it != cache_history_sampler[sample_set].end(); it++){
                    if((it->second).lru == (SAMPLER_HIST-1)){
                        addr_val = it->first;
                        break;
                    }
                }
                cache_history_sampler[sample_set].erase(addr_val);
            }

            //Create new entry
            cache_history_sampler[sample_set][sample_tag].init();
            //If preftech, mark it as a prefetching or if not, just set the demand access
            if(type == PREFETCH){
                cache_history_sampler[sample_set][sample_tag].set_prefetch();
                flexmingen_occup_vector[set].set_prefetch(currentVal);
            }
            else{
                flexmingen_occup_vector[set].set_access(currentVal);
            }

            //Update cache history
            update_cache_history(sample_set, SAMPLER_HIST-1);
        }
        //If line is neither of the two above options, then it is a prefetch line
        else{//*-P
            if(set_timer[set] - cache_history_sampler[sample_set][sample_tag].previousVal < 5*NUM_CORE){
                if(flexmingen_occup_vector[set].is_cache(currentVal, previousVal, type, led)){
                    bool isWrap = (set_timer[set] - cache_history_sampler[sample_set][sample_tag].previousVal) > FLEXMINGEN_SIZE;
                    if(!isWrap){
                        ptotal[cpu] += interval_len;
                        pcount[cpu]++;
                    }

                    if(cache_history_sampler[sample_set][sample_tag].prefetching){
                        predictor_prefetch->increase(cache_history_sampler[sample_set][sample_tag].PCval);
                    }
                    else{
                        predictor_demand->increase(cache_history_sampler[sample_set][sample_tag].PCval);
                    }
                }
                else if(interval_len>=led){//这里可以考虑更加激进的惩罚(多惩罚几下)
                    if(cache_history_sampler[sample_set][sample_tag].prefetching){
                        predictor_prefetch->decrease(cache_history_sampler[sample_set][sample_tag].PCval);
                        
                    }
                    else{
                        predictor_demand->decrease(cache_history_sampler[sample_set][sample_tag].PCval);
                        
                    }
                }
            }
            cache_history_sampler[sample_set][sample_tag].set_prefetch();
            flexmingen_occup_vector[set].set_prefetch(currentVal);
            //Update cache history
            update_cache_history(sample_set, cache_history_sampler[sample_set][sample_tag].lru);

        }   
        //Update the sample with time and PC
        cache_history_sampler[sample_set][sample_tag].update(set_timer[set], PC);
        cache_history_sampler[sample_set][sample_tag].lru = 0;
        set_timer[set] = (set_timer[set] + 1) % TIMER_SIZE;
    }

    //Retrieve Harmony's prediction for line
    bool prediction = predictor_demand->get_prediction(PC);
    if(type == PREFETCH){
        prediction = predictor_prefetch->get_prediction(PC);
    }
    
    sample_signature[set][way] = PC;
    //Fix RRIP counters with correct RRPVs and age accordingly
    if(!prediction){
        rrip[set][way] = MAXRRIP;
    }
    else{
        rrip[set][way] = 0;
        if(!hit){
            //Verifying RRPV of lines has not saturated
            bool isMaxVal = false;
            for(uint32_t i = 0; i < LLC_WAYS; i++){
                if(rrip[set][i] == MAXRRIP-1){
                    isMaxVal = true;
                }
            }

            //Aging cache-friendly lines that have not saturated
            for(uint32_t i = 0; i < LLC_WAYS; i++){
                if(!isMaxVal && rrip[set][i] < MAXRRIP-1){
                    rrip[set][i]++;
                }
            }
        }
        rrip[set][way] = 0;
    }
}

// 使用此函数在每个heartbeat时打印出您自己的统计信息
void PrintStats_Heartbeat()
{
    int hits = 0;
    int access = 0;
    for(int i = 0; i < LLC_SETS; i++){
        hits += flexmingen_occup_vector[i].get_flexmingen_hits();
        access += flexmingen_occup_vector[i].access;
    }

    cout<< "Demand Hit Rate: " << 100 * ( (double)demand_hit/(double)demand_access )<< endl;
    cout<< "FlenMINGen Hits: " << hits << endl;
    cout<< "FlenMINGen Access: " << access << endl;
    cout<< "FLEXMINGEN Hit Rate: " << 100 * ( (double)hits/(double)access )<< endl;
}

// 使用此函数在模拟结束时打印出您自己的统计信息
void PrintStats()
{
    int hits = 0;
    int access = 0;
    for(int i = 0; i < LLC_SETS; i++){
        hits += flexmingen_occup_vector[i].get_flexmingen_hits();
        access += flexmingen_occup_vector[i].access;
    }

    cout<< "Demand Hit Rate: " << 100 * ( (double)demand_hit/(double)demand_access )<< endl;
    cout<< "Final FlenMINGen Hits: " << hits << endl;
    cout<< "Final FlenMINGen Access: " << access << endl;
    cout<< "Final FLEXMINGEN Hit Rate: " << 100 * ( (double)hits/(double)access )<< endl;

}