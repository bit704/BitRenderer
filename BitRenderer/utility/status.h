/*
 * 获取系统状态的函数
 */
#ifndef STATUS_H
#define STATUS_H

#include <windows.h> // status.h

#include "common.h"

static MEMORYSTATUSEX statex;

// 正在使用的内存的近似百分比
inline long long get_memory_load()
{
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.dwMemoryLoad;
}

// CPU占用率
inline double get_cpu_usage(FILETIME &cpu_idle_prev, FILETIME &cpu_kernel_prev, FILETIME &cpu_user_prev)
{
	FILETIME cpu_idle, cpu_kernel, cpu_user;
	GetSystemTimes(&cpu_idle, &cpu_kernel, &cpu_user);

	auto ft2ull = [](FILETIME ft)-> ULONGLONG
	{
		return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
	};

	ULONGLONG cpu_idle_delta = ft2ull(cpu_idle) - ft2ull(cpu_idle_prev);
	ULONGLONG cpu_kernel_delta = ft2ull(cpu_kernel) - ft2ull(cpu_kernel_prev);
	ULONGLONG cpu_user_delta = ft2ull(cpu_user) - ft2ull(cpu_user_prev);

	double cpu_usage = 100.f * (cpu_kernel_delta + cpu_user_delta - cpu_idle_delta) 
		/ (cpu_kernel_delta + cpu_user_delta);

	cpu_idle_prev = cpu_idle;
	cpu_kernel_prev = cpu_kernel;
	cpu_user_prev = cpu_user;

	return cpu_usage;
}

#endif // !STATUS_H