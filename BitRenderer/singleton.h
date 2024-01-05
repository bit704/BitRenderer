/*
 * 单例模板类
 * Meyers' Singleton，确保对象被使用前已先被初始化（Effective C++ 条款4）。
 * 把static单例对象定义在static函数内部作为局部static变量。
 * C++只能保证在同一个文件中声明的static变量的初始化顺序与其变量声明的顺序一致，但不能保证不同的文件中的static变量的初始化顺序。
 */
#ifndef SINGLETON_H
#define SINGLETON_H

template<typename T>
class Singleton
{
public:

    static T& get_instance()
    {
        static T instance;
        return instance;
    }

protected:

    Singleton() = default;
    ~Singleton() = default;
    
private:

    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

#endif // !SINGLETON_H


