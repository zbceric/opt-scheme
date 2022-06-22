/*
 * @Author: Zhang Bochun
 * @Date: 2022-06-19 19:20:27
 * @LastEditTime: 2022-06-20 15:12:52
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/model/net-device-tracer.h
 */
#ifndef NET_DEVICE_TRACER_H
#define NET_DEVICE_TRACER_H


#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "tracer-utils.h"
#include "vector"
#include "list"

namespace ns3 {

class NetDeviceTracer : public Object
{
public:
    static TypeId GetTypeId (void);

    // enum TraceEnable: uint16_t {       // 枚举变量 用来控制trace哪些属性
    //     E_TRACE_SENDRATE  = 0x0001 << 1,    // SendRate
    //     E_TRACE_RECERATE  = 0x0001 << 2,
        
    //     E_TRACE_ALL = E_TRACE_SENDRATE | E_TRACE_RECERATE,
    // };

    NetDeviceTracer (void) = default;
    NetDeviceTracer (std::string& foldername);
    NetDeviceTracer (std::string& foldername, std::string& filename);
  
    void SendTraceNetDevice (Ptr<NetDevice> netdevice);
    void ReceTraceNetDevice (Ptr<NetDevice> netdevice);

protected:
    virtual void DoDispose (void);

private:
    std::vector<Ptr<NetDevice>> m_netdevice;
    
    Time                 m_minGapTime {Time("60ms")};
    std::list<Time>      m_TxTimes {Time("0s")};
    std::list<Time>      m_RxTimes {Time("0s")};
    std::list<uint128_t> m_TxBytes {0};
    std::list<uint128_t> m_RxBytes {0};

    uint128_t            m_totalTxBytes {0};
    uint128_t            m_totalRxBytes {0};

    uint16_t             m_traceFlag;
    uint64_t             m_mss;

    std::string          m_foldername;
    std::string          m_filename;

    std::fstream         m_sendrate;
    std::fstream         m_recerate;

    // sender
    void OpenReceRateTracerFile (std::string filename);
    void OpenSendRateTracerFile (std::string filename);
    void TracePacketRxCallback  (Ptr<const Packet> packet);
    void TracePacketTxCallback  (Ptr<const Packet> packet);

};

}

#endif
