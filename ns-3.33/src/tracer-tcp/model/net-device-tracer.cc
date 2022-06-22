/*
 * @Author: Zhang Bochun
 * @Date: 2022-06-19 19:31:16
 * @LastEditTime: 2022-06-20 15:12:13
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/model/net-device-tracer.cc
 */

#include "net-device-tracer.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NetDeviceTracer");

NS_OBJECT_ENSURE_REGISTERED (NetDeviceTracer);

TypeId NetDeviceTracer::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::NetDeviceTracer")
        .SetParent<Object> ()
        .SetGroupName("TracerTcp")
        .AddConstructor<NetDeviceTracer> ()
    ;
    return tid;
}


void NetDeviceTracer::DoDispose (void)
{
    NS_LOG_FUNCTION (this);

    if (m_sendrate.is_open ())
    {
        m_sendrate.close ();        
    }

    if (m_recerate.is_open ())
    {
        m_recerate.close ();        
    }

    Object::DoDispose ();
}


NetDeviceTracer::NetDeviceTracer (std::string& foldername)
{
    char buf[FILENAME_MAX];
    std::string path = std::string (getcwd (buf, FILENAME_MAX))+ "/traces/";        /* 绝对路径 */
    path = path + foldername;

    if (!TracerUtils::IsDirExist (path))
    {
        TracerUtils::MakePath (path);
    }
    
    m_filename = path + "/alldate";
    OpenReceRateTracerFile (m_filename);
    OpenSendRateTracerFile (m_filename);
}

NetDeviceTracer::NetDeviceTracer (std::string& foldername, std::string& filename)
{
    char buf[FILENAME_MAX];
    std::string path = std::string (getcwd (buf, FILENAME_MAX))+ "/traces/";        /* 绝对路径 */
    path = path + foldername;

    if (!TracerUtils::IsDirExist (path))
    {
        TracerUtils::MakePath (path);
    }
    
    m_filename = path + "/" + filename;
    OpenReceRateTracerFile (m_filename);
    OpenSendRateTracerFile (m_filename);
}


void NetDeviceTracer::OpenReceRateTracerFile (std::string filename)
{
    if (!m_sendrate.is_open ())
    {
        filename += "_sendrate.txt";
        m_sendrate.open(filename.c_str(), std::fstream::out);
    }
}

void NetDeviceTracer::OpenSendRateTracerFile (std::string filename)
{
    if (!m_recerate.is_open ())
    {
        filename += "_recerate.txt";
        m_recerate.open(filename.c_str(), std::fstream::out);
    }
}


void NetDeviceTracer::SendTraceNetDevice (Ptr<NetDevice> netdevice)
{
    m_netdevice.push_back (netdevice);
    netdevice->TraceConnectWithoutContext ("MacTx", MakeCallback (&NetDeviceTracer::TracePacketTxCallback, this));

    // std::cout << "band" << std::endl;
}


void NetDeviceTracer::ReceTraceNetDevice (Ptr<NetDevice> netdevice)
{
    m_netdevice.push_back (netdevice);
    netdevice->TraceConnectWithoutContext ("MacRx", MakeCallback (&NetDeviceTracer::TracePacketRxCallback, this));
}


void NetDeviceTracer::TracePacketRxCallback (Ptr<const Packet> packet)
{
    Time now = Simulator::Now();

    if (m_recerate.is_open())
    {
        m_totalRxBytes += packet->GetSize();

        if (m_RxTimes.empty ())
        {
            m_RxTimes.push_back (now);
            m_RxBytes.push_back (m_totalRxBytes);
        }
        else
        {
            if (now >= m_RxTimes.back () + Time ("1ms"))
            {
                m_RxTimes.push_back (now);
                m_RxBytes.push_back (m_totalRxBytes);
            }

            if (now >= m_RxTimes.front() + m_minGapTime)
            {
                double bps = 1.0 * (m_totalRxBytes - m_RxBytes.front ()) * 8 / (now - m_RxTimes.front()).GetSeconds();
                double mbps = bps / 1000 / 1000;

                m_RxBytes.pop_front ();
                m_RxTimes.pop_front ();

                m_recerate << now.GetSeconds() << "\t" << mbps << std::endl;
            }
        }
    }
}

void NetDeviceTracer::TracePacketTxCallback (Ptr<const Packet> packet)
{
    Time now = Simulator::Now();

    if (m_sendrate.is_open())
    {
        m_totalTxBytes += packet->GetSize();

        if (m_TxTimes.empty ())
        {
            m_TxTimes.push_back (now);
            m_TxBytes.push_back (m_totalTxBytes);
        }
        else
        {
            if (now >= m_TxTimes.back () + Time ("1ms"))
            {
                m_TxTimes.push_back (now);
                m_TxBytes.push_back (m_totalTxBytes);
            }

            if (now >= m_TxTimes.front() + m_minGapTime)
            {
                double bps = 1.0 * (m_totalTxBytes - m_TxBytes.front ()) * 8 / (now - m_TxTimes.front()).GetSeconds();
                double mbps = bps / 1000 / 1000;

                m_TxBytes.pop_front ();
                m_TxTimes.pop_front ();

                m_sendrate << now.GetSeconds() << "\t" << mbps << std::endl;
            }
        }
    }
}

};