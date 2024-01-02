/*
* 蒙特卡洛积分示例
* 以y=x^2在[0,2]的积分为例
*/
#include <iostream>
#include <iomanip>
#include <cmath>
#include "common.h"

// f即pdf的积分cdf的反函数
double f1(double d)
{
    return 2 * d;
}

// pdf在积分区间上积分必定为1
double pdf1(double x)
{
    return .5;
}

double f2(double d)
{
    return sqrt(4. * d);
}

double pdf2(double x)
{
    return x / 2.;
}

double f3(double d)
{
    return 8. * pow(d, 1. / 3.);
}

double pdf3(double x)
{
    return (3. / 8.) * x * x;
}

void integral_lab()
{
    // 函数指针数组
    double (*f[])(double) = { f1, f2, f3 };
    double (*pdf[])(double) = { pdf1, pdf2, pdf3 };

    for (int k = 0; k < 3; ++k)
    {
        int N = 1e6;
        double sum = 0;
        for (int i = 0; i < N; ++i)
        {
            auto x = f[k](random_double());
            auto tmp = x * x / pdf[k](x);
            if (!std::isnan(tmp)) // 避免nan
                sum += tmp;
        }
        std::cout << std::fixed << std::setprecision(12);
        std::cout << "integral: " << sum / N << std::endl;
        std::cout << "error: " << sum / N - 8. / 3 << std::endl;
    }
}
