/*
 * 公共常量和函数
 */
#ifndef COMMON_H
#define COMMON_H

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <deque>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;
using std::chrono::steady_clock;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::nanoseconds;
namespace fs = std::filesystem;

using uint   = unsigned int;
using ulong  = unsigned long;
using ullong = unsigned long long;
using llong  = long long;

extern const double kInfinitDouble;
extern const double kPI;
extern const double kEpsilon;    // 比较浮点数的阈值
extern const char*  kLoadPath;   // 加载文件的位置
extern const char*  kOutputPath; // 保存输出的位置

extern std::atomic_ullong hit_count;      // 击中次数统计
extern std::atomic_ullong sample_count;   // 采样次数统计
extern std::atomic_bool   tracing;        // 标志是否正在渲染
extern std::atomic_bool   stop_rastering; // 标志是否需要停止光栅化，光追过程中和光追完成但没有Clear结果时需要停止光栅化

// 返回dir目录下（含递归目录）所有文件名满足rule的文件的路径
std::vector<fs::path> traverse_path(std::string dir, std::regex rule);

enum RasteringModeFlags // 光栅化模式
{
    RasteringModeFlags_None = 0,
    RasteringModeFlags_Coordinate_System = 1 << 0,
    RasteringModeFlags_Wireframe = 1 << 1,
    RasteringModeFlags_Depth = 1 << 2,
    RasteringModeFlags_Shade = 1 << 3,
};

enum MaterialTypeFlags // 材质类型
{
    MaterialTypeFlags_None = 0,
    MaterialTypeFlags_Lambert = 1 << 0,
    MaterialTypeFlags_Microfacet = 1 << 2,
};

#define BASE_COLOR_DEFAULT make_shared<SolidColor>(Color3(0, 1, 0))
#define METALLIC_DEFAULT   make_shared<SolidColor>(Color3(0, 0, 0))
#define ROUGHNESS_DEFAULT  make_shared<SolidColor>(Color3(.01, .01, .01))
#define NORMAL_DEFAULT     make_shared<SolidColor>(Color3(0, 0, 1))

#define CHOOSE_MATERIAL(material)                      \
shared_ptr<Material> material;                         \
if (material_type & MaterialTypeFlags_Lambert)         \
    material = material_lambert;                       \
else if (material_type & MaterialTypeFlags_Microfacet) \
    material = material_microfacet

#define ARRAY3_ASSIGN(array3, x, y, z)                    \
static_assert((sizeof(array3) / sizeof(array3[0])) == 3); \
array3[0] = x, array3[1] = y, array3[2] = z

double degrees_to_radians(double degrees);

double random_double();
double random_double(double min, double max);
int    random_int(int min, int max);

// 给输出数字加入千位分隔符
std::string operator ""_sep(ullong num);
std::string format_num(ullong num);

// 避免minwindef.h中宏max和std::max的冲突
#undef max
#undef min

inline double max3(double x, double y, double z)
{
    return std::max(x, std::max(y, z));
}

inline double min3(double x, double y, double z)
{
    return std::min(x, std::min(y, z));
}

#define STR(x) std::to_string(x)

std::string operator ""_str(const char*, size_t);

void add_info(std::string new_info);
const char* get_info(ullong i);
ullong get_info_size();

#endif // !COMMON_H