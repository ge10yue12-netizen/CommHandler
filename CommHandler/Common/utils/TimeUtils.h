#pragma once

#include <chrono>
#include <ctime>

// 自定义时间结构体
typedef struct times
{
    int Year;
    int Mon;
    int Day;
    int Hour;
    int Min;
    int Second;
} Times;

class TimeUtils
{
public:
    // 获取当前时间对应的纳秒
    static uint64_t getNanosecond()
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }

    // 获取当前时间对应的微秒
    static uint64_t getMicrosecond()
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    }

    // 获取当前时间对应的毫秒
    static unsigned long long getMillisecond()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }

    // 获取当前时间对应的秒
    static unsigned long long getSecond()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    }

    // 将时间戳转换为标准时间结构体
    static Times stamp_to_standard(uint64_t stampTime)
    {
        Times time;
        auto time_t_stamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(std::chrono::microseconds(stampTime)));
        tm* tm_info = std::localtime(&time_t_stamp);

        time.Year = tm_info->tm_year + 1900;
        time.Mon = tm_info->tm_mon + 1;
        time.Day = tm_info->tm_mday;
        time.Hour = tm_info->tm_hour;
        time.Min = tm_info->tm_min;
        time.Second = tm_info->tm_sec;

        return time;
    }
};
