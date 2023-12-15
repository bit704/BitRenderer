/*
* 日志类
* 用于输出日志
*/
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <memory>

#include "singleton.h"

#define PRINT_LOCATION std::cout << __FILE__ << "第" << __LINE__ << "行（" << __FUNCTION__ << "）：";
#define LOG(X) PRINT_LOCATION Logger::getInstance().printInfo(X);

class Logger : public Singleton<Logger>
{
public:
	// 输出信息
	inline void printInfo(std::string info)
	{
		std::cout << info << std::endl;
	}
};

#endif // !LOGGER_H
