/*
 * 日志类
 * 用于输出日志
 */
#ifndef LOGGER_H
#define LOGGER_H

// 避免宏展开处缺头文件
#include <iostream>

#include "singleton.h"

#define PRINT_LOCATION std::cout << __FILE__ << "第" << __LINE__ << "行（" << __FUNCTION__ << "）：";
#define LOG(...) PRINT_LOCATION Logger::get_instance().print_info(__VA_ARGS__) // 外层不可省略{}

class Logger : public Singleton<Logger>
{
public:
	// C++17
	template <typename... Types>
	inline void print_info(const Types... args)
	{
		(std::cout << ... << args) << std::endl;
	}
};

#endif // !LOGGER_H
