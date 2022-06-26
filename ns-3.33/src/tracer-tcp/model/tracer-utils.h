/*
 * @Author: Zhang Bochun
 * @Date: 2022-03-13 17:42:06
 * @LastEditTime: 2022-06-24 19:57:29
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/model/tracer-utils.h
 */



#ifndef TRACER_UTILES_H
#define TRACER_UTILES_H


#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sys/stat.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/stats-module.h"

namespace ns3
{

class TracerUtils
{
public:
    /**
     * @brief convert ip (uint32_t) to standard ip format (std::string)
     *        e.g. "10.0.0.1"
     * 
     * @param ip 
     * @return std::string 
     */
    static std::string StandardIpFormatString (uint32_t ip);

    /**
     * @brief check if the path exists
     * 
     * @param path 
     * @return true     exist
     * @return false    not exist
     */
    static bool IsDirExist (const std::string& path);


    /**
     * @brief make path
     * 
     * @param path 
     * @return true     exist
     * @return false    not exist
     */
    static bool MakePath (const std::string& path);


    /**
     * @brief map cc name to TypeId
     * 
     * @param cc congestion control algorithm name. (e.g. cubic)
     * @return TypeId the typeId of congestion control algorithm. (e.g. TcpCubic.GetTypeId ())
     */
    static TypeId GetCongestionTypeIdFromName (std::string cc, bool& flag);

private:
    TracerUtils()  = default;
    ~TracerUtils() = default;
};



inline std::string TracerUtils::StandardIpFormatString(uint32_t ip)
{
    uint32_t ip_1 = (ip >> 24) & 0x00FF;
    uint32_t ip_2 = (ip >> 16) & 0x00FF;
    uint32_t ip_3 = (ip >> 8 ) & 0x00FF;
    uint32_t ip_4 = (ip      ) & 0x00FF;
    std::string delimiter(".");
    std::string ret = std::to_string(ip_1) + delimiter + std::to_string(ip_2) + delimiter
                    + std::to_string(ip_3) + delimiter + std::to_string(ip_4);
    return ret;
}


inline bool TracerUtils::IsDirExist(const std::string& path)
{
#if defined(_WIN32)
    struct _stat info;
    if (_stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else 
    struct stat info;                                           // stat用来获取指定路径的文件或者文件夹的信息
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;                       // 检查st_mode
#endif
}


inline bool TracerUtils::MakePath(const std::string& path)      // 设置路径
{
#if defined(_WIN32)                                             // 在windows平台则运行A部分代码,否则在执行B部分代码
    int ret = _mkdir(path.c_str());
#else
    mode_t mode = 0755;                                         // 0755 八进制 表示文件权限 rwxrw-rw-
    int ret = mkdir(path.c_str(), mode);                        // 若目录创建成功，则返回0；否则返回-1，并将错误记录到全局变量errno中。
#endif
    if (ret == 0)                                               // 建立成功 直接返回
        return true;

    switch (errno)                                              // 出现了错误
    {
    case ENOENT:
        // parent didn't exist, try to create it                // 父目录不存在
        {
            size_t pos = path.find_last_of('/');                // 在linux上 找到最后一个/ 该字符前面的就是父路径
            if (pos == std::string::npos)                       // 没有找到 npos表示一个不存在的位置 即不存在父路径 就是失败了
#if defined(_WIN32)
                pos = path.find_last_of('\\');
            if (pos == std::string::npos)
#endif
                return false;                                   // 返回 false 表示创建失败
            if (!TracerUtils::MakePath( path.substr(0, pos) ))   // 传入父路径创建父路径 若返回值为true 则表示成功
                return false;                                   // 返回值为false 表示创建失败 返回false
        }
        // now, try to create again
#if defined(_WIN32)
        return 0 == _mkdir(path.c_str());
#else 
        return 0 == mkdir(path.c_str(), mode);                  // 执行到这里是 父路径成功创建 重新创建路径看是否成功
#endif
    case EEXIST:
        // done!
        return TracerUtils::IsDirExist(path);            // 路径已经存在 返回查询结果

    default:
        return false;
    }
}


inline TypeId TracerUtils::GetCongestionTypeIdFromName (std::string cc, bool& pacing)
{
    TypeId id;
    pacing = false;
    if (cc.compare("cubic") == 0)
    {
        id = TcpCubic::GetTypeId ();
    }
    else if (cc.compare("bbr") == 0)
    {
        id = TcpBbr::GetTypeId ();
        pacing = true;
    }
    else if (cc.compare("copa") == 0)
    {
        id = TcpCopa::GetTypeId ();
        pacing = true;
    }
    else
    {
        id = TcpCubic::GetTypeId ();    // default
    }
    return id;
}


}

#endif