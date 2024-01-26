#include "common.h"

const double kInfinitDouble = std::numeric_limits<double>::infinity();
const double kPI = 3.1415926535897932385;
const double kEpsilon = 1e-8;
const char* kLoadPath   = ".\\load\\";
const char* kOutputPath = ".\\output\\";

std::vector<fs::path> traverse_path(std::string dir, std::regex rule)
{
    std::vector<fs::path> founds = { "None" };
    for (const auto& file : fs::recursive_directory_iterator(dir))
    {
        auto path = file.path();
        std::string name = path.filename().string();
        if (std::regex_match(name, rule))
            founds.push_back(path);
    }
    return founds;
}

double degrees_to_radians(double degrees)
{
    return degrees * kPI / 180.;
}

double random_double()
{
    return rand() / (RAND_MAX + 1.); // [0,1)
}

double random_double(double min, double max)
{
    return min + (max - min) * random_double(); // [min,max)
}

int random_int(int min, int max)
{
    return static_cast<int>(random_double(min, max + 1));
}

std::string format_num(long long num)
{
    static std::ostringstream oss;
    oss.str("");
    oss.imbue(std::locale("en_US.UTF-8"));
    oss << num;
    return oss.str();
}