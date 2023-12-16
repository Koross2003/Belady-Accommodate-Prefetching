#ifndef HOMONY_PREDICTOR_H
#define HOMONY_PREDICTOR_H

using namespace std;
#include <vector>
#include <map>
#include "helper_function.h"

#define MAX_PCMAP 31
#define PCMAP_SIZE 2048

class Homony_Predictor{
private:
	map<uint64_t, int> PC_Map; // 存储PC和预测结果的映射表

public:
	// 获取PC地址的预测结果
	bool get_prediction(uint64_t PC){
		uint64_t result = CRC(PC) % PCMAP_SIZE; // 计算哈希值
		if(PC_Map.find(result) != PC_Map.end() && PC_Map[result] < ((MAX_PCMAP+1)/2)){
			return false; // 返回预测结果为cache-averse
		}
		return true; // 返回预测结果为cache-friendly
	}

	// 增加PC地址的预测结果
	void increase(uint64_t PC){
		uint64_t result = CRC(PC) % PCMAP_SIZE; // 计算哈希值
		if(PC_Map.find(result) == PC_Map.end()){
			PC_Map[result] = (MAX_PCMAP + 1)/2; // 初始化预测结果为中间值
		}

		if(PC_Map[result] < MAX_PCMAP){
			PC_Map[result] = PC_Map[result]+1; // 预测结果加1
		}
		else{
			PC_Map[result] = MAX_PCMAP; // 预测结果达到最大值
		}
	}

	// 减少PC地址的预测结果
	void decrease(uint64_t PC){
		uint64_t result = CRC(PC) % PCMAP_SIZE; // 计算哈希值
		if(PC_Map.find(result) == PC_Map.end()){
			PC_Map[result] = (MAX_PCMAP + 1)/2; // 初始化预测结果为中间值
		}
		if(PC_Map[result] != 0){
			PC_Map[result] = PC_Map[result] - 1; // 预测结果减1
		}
	}

};

#endif