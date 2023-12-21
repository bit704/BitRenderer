/*
* 日志类
* 用于输出日志
*/
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

#include "singleton.h"

#define PRINT_LOCATION std::cout << __FILE__ << "第" << __LINE__ << "行（" << __FUNCTION__ << "）：";
#define LOG(X) PRINT_LOCATION Logger::get_instance().print_info(X);

class Logger : public Singleton<Logger>
{
public:
	// 输出信息
	inline void print_info(std::string info)
	{
		std::cout << info << std::endl;
	}
};

#endif // !LOGGER_H
