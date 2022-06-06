/*
 * @Author: Zhang Bochun
 * @Date: 2022-05-05 20:42:36
 * @LastEditTime: 2022-05-06 11:46:14
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/model/tracer-netdevice.cc
 */


#include "tracer-netdevice.h"

#include "ns3/point-to-point-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TracerNetDevice");

NS_OBJECT_ENSURE_REGISTERED (TracerNetDevice);


TypeId TracerNetDevice::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TracerNetDevice")
        .SetParent<Object> ()
        .SetGroupName("TracerTcp")
        .AddConstructor<TracerNetDevice> ()
    ;
    return tid;
}


TracerNetDevice::TracerNetDevice (Ptr<NetDevice> netdevice, Ipv4Address addr, std::string& foldername)
{
    m_addr = addr;
    m_netdevice = netdevice;
    m_foldername = foldername;

    RegisterTraceFunctions ();
}

void TracerNetDevice::RegisterTraceFunctions ()
{
    if (m_netdevice == nullptr )
    {
        return;
    }
    
    std::string delimiter="_";
    Ipv4Address addr = Ipv4Address::ConvertFrom (m_addr);

    uint32_t ip = addr.Get ();

    std::string ip_str = TracerUtils::StandardIpFormatString (ip);

    char buf[FILENAME_MAX];
    std::string path = std::string (getcwd (buf, FILENAME_MAX))+ "/traces/";        /* 绝对路径 */
    path = path + m_foldername;
    if (!TracerUtils::IsDirExist (path))
    {
        TracerUtils::MakePath (path);
    }

    /* 绝对路径/trace/foldername/filename(ip+port)_suffix  */
    std::string filename = path + "/" + ip_str;
    this->OpenTxTracerFile (filename);
    m_netdevice->TraceConnectWithoutContext ("PhyTxBegin", MakeCallback (&TracerNetDevice::TraceTxCallback, this));
    this->OpenRxTracerFile (filename);
    m_netdevice->TraceConnectWithoutContext ("PhyRxEnd",   MakeCallback (&TracerNetDevice::TraceRxCallback, this));
    this->OpenInOutTracerFile (filename);

    m_queue = m_netdevice->GetObject<PointToPointNetDevice> ()->GetQueue ();
    m_queue->TraceConnectWithoutContext ("Enqueue", MakeCallback (&TracerNetDevice::TraceEnqueueCallback, this));
    m_queue->TraceConnectWithoutContext ("Dequeue", MakeCallback (&TracerNetDevice::TraceDequeueCallback, this));
    m_queue->TraceConnectWithoutContext ("Drop",    MakeCallback (&TracerNetDevice::TraceDropCallback,    this));
    m_queue->TraceConnectWithoutContext ("DropBeforeEnqueue", MakeCallback (&TracerNetDevice::TraceDropBfECallback, this));
    m_queue->TraceConnectWithoutContext ("DropAfterDequeue",  MakeCallback (&TracerNetDevice::TraceDropAfDCallback, this));
}

void TracerNetDevice::DoDispose (void)
{
    NS_LOG_FUNCTION (this);

    while (! m_filelist.empty ())
    {
        auto& file = m_filelist.front ();
        if (!file->is_open ())
        {
            file->close ();
        }
        m_filelist.pop ();
    }
    Object::DoDispose ();
}

void TracerNetDevice::OpenRxTracerFile (std::string filename)
{
    if (!m_rxTrace.is_open ())
    {
        filename += "_RX.txt";
        m_rxTrace.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_rxTrace);
    }
}


void TracerNetDevice::OpenTxTracerFile (std::string filename)
{
    if (!m_txTrace.is_open ())
    {
        filename += "_TX.txt";
        m_txTrace.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_txTrace);
    }
}


void TracerNetDevice::OpenInOutTracerFile (std::string filename)
{
    if (!m_inoutTrace.is_open ())
    {
        filename += "_InOut.txt";
        m_inoutTrace.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_inoutTrace);
    }
}


void TracerNetDevice::TraceTxCallback  (Ptr<const Packet> packet)
{
    if (m_txTrace.is_open ())
    {
        PrintToFile (packet, m_txTrace);
    }
}


void TracerNetDevice::TraceRxCallback  (Ptr<const Packet> packet)
{
    if (m_rxTrace.is_open ())
    {
        PrintToFile (packet, m_rxTrace);
    }
}


void TracerNetDevice::TraceEnqueueCallback  (Ptr<const Packet> packet)
{
    if (m_inoutTrace.is_open ())
    {
        m_inoutTrace << "Enqueue:\tn:" << m_queue->GetNPackets () << std::endl;
        PrintToFile (packet, m_inoutTrace);
    }
}

void TracerNetDevice::TraceDequeueCallback  (Ptr<const Packet> packet)
{
    if (m_inoutTrace.is_open ())
    {
        m_inoutTrace << "Dequeue:\tn:" << m_queue->GetNPackets () << std::endl;
        PrintToFile (packet, m_inoutTrace);
    }
}

void TracerNetDevice::TraceDropCallback     (Ptr<const Packet> packet)
{
    if (m_inoutTrace.is_open ())
    {
        m_inoutTrace << "Drop:\tn:" << m_queue->GetNPackets () << std::endl;
        PrintToFile (packet, m_inoutTrace);
    }
}
void TracerNetDevice::TraceDropBfECallback  (Ptr<const Packet> packet)
{
    if (m_inoutTrace.is_open ())
    {
        m_inoutTrace << "DropBeforeEnqueue:\tn:" << m_queue->GetNPackets () << std::endl;
        PrintToFile (packet, m_inoutTrace);
    }
}
void TracerNetDevice::TraceDropAfDCallback  (Ptr<const Packet> packet)
{
    if (m_inoutTrace.is_open ())
    {
        m_inoutTrace << "DropAfterDequeue:\tn:" << m_queue->GetNPackets () << std::endl;
        PrintToFile (packet, m_inoutTrace);
    }
}


void TracerNetDevice::PrintToFile (Ptr<const Packet> packet, std::fstream& file)
{
    PacketMetadata::ItemIterator metadataIterator = packet->BeginItem();

    PacketMetadata::Item item;
    while (metadataIterator.HasNext())
    {
        item = metadataIterator.Next();
        if(item.tid == TcpHeader::GetTypeId ())
        {
            Callback<ObjectBase *> constr = item.tid.GetConstructor();
            ObjectBase *instance = constr();
            TcpHeader* header = dynamic_cast<TcpHeader*> (instance);
            header->Deserialize(item.current);

            Ptr<const TcpOption> option = header->GetOption (TcpOption::TS);

            if (option != nullptr)
            {
                Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);

                file << Simulator::Now ().GetSeconds ()    << "\t" 
                    << "Seq: "  << header->GetSequenceNumber()  << "\t" 
                    << "ACK: "  << header->GetAckNumber()       << "\t"
                    << "size: " << packet->GetSize ()           << "\t"
                    << "time: " << ts->GetTimestamp()           << "\t"
                    << "echo: " << ts->GetEcho () << std::endl;
            }
            else
            {
                file << Simulator::Now ().GetSeconds ()    << "\t" 
                    << "Seq: "  << header->GetSequenceNumber()  << "\t" 
                    << "ACK: "  << header->GetAckNumber()       << "\t"
                    << "size: " << packet->GetSize () <<  std::endl;
            }
        
            break;
        }
    }
}

}