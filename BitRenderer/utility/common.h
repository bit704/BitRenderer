/*
 * 公共常量和函数
 */
#ifndef COMMON_H
#define COMMON_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <d3d12.h>   // ui_helper.h
#include <dxgi1_4.h> // ui_helper.h
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <regex>
#include <sstream>
#include <string>
#include <tchar.h>   // ui_helper.h
#include <thread>
#include <vector>
#include <windows.h> // status.h

using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;
using std::chrono::steady_clock;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
namespace fs = std::filesystem;

using uint = unsigned int;

extern const double kInfinitDouble;
extern const double kPI;
extern const double kEpsilon;    // 比较浮点数的阈值
extern const char*  kLoadPath;   // 加载文件的位置
extern const char*  kOutputPath; // 保存输出的位置

// 返回dir目录下（含递归目录）所有文件名满足rule的文件的路径
std::vector<fs::path> traverse_path(std::string dir, std::regex rule);

double degrees_to_radians(double degrees);

double random_double();
double random_double(double min, double max);
int    random_int(int min, int max);

// 给输出数字加入千位分隔符
std::string format_num(long long num);

// 避免minwindef.h中宏max和std::max的冲突
#undef max
#undef min

inline double max3(double x, double y, double z)
{
    return std::max(x, std::max(y, z));
}

inline double min3(double x, double y, double z)
{
    return std::min(z, std::min(y, z));
}

#endif // !COMMON_H