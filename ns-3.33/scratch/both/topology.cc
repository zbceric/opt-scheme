/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-10 15:10:03
 * @LastEditTime: 2022-06-22 10:58:21
 * @LastEditors: Zhang Bochun
 * @Description: 中心为2个交换机, 左侧连接到若干 sender, 右侧连接到若干 receiver
 * @FilePath: /ns-3.33/scratch/both/topology.cc
 */


#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/stats-module.h"
#include "ns3/tracer-tcp-module.h"

using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("TracerTcpCubic");

static const uint32_t PACKET_SIZE  = 1500;

int main (int argc, char *argv[])
{
    LogComponentEnable("TracerTcpCubic", LOG_LEVEL_ALL);
    // LogComponentEnable("PaceTcpSocketBase", LOG_LEVEL_ALL);
    // LogComponentEnable("SendTcpSocketBase", LOG_LEVEL_ALL);


    std::string cc("cubic");                                    // 默认使用Cubic算法
    std::string flow("2");                                      // flow 数量 
    std::string foldname("default"); 


    CommandLine cmd;
    cmd.AddValue ("cc", "congestion algorithm", cc);            // cmd --cc=xxx 设置拥塞算法
    cmd.AddValue ("flow", "num of flow", flow);
    cmd.AddValue ("foldname", "folder name to collect data", foldname);  // cmd --foldername==xxx 设置数据保存的文件夹
    cmd.Parse(argc, argv);   
    

    int num = std::stoi(flow);
    num = num > 0 ? num : 1;
    foldname += "/" + cc + "_simulation" + "/" + std::to_string(num) + "flow - coexist - RTT = 100ms";


    Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType", TypeIdValue (TcpClassicRecovery::GetTypeId()));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize",  UintegerValue (1 << 20));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize",  UintegerValue (1 << 20));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (uint32_t(2)));    // 不进行delay
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (PACKET_SIZE));
    Config::SetDefault ("ns3::TcpSocketBase::Sack",    BooleanValue  (true));
    Config::SetDefault ("ns3::TcpSocket::TcpNoDelay", BooleanValue (false));
    Config::SetDefault ("ns3::TcpCubic::Beta", DoubleValue(0.7));

    NodeContainer sender, receiver;
    NodeContainer routers;
    sender.Create (num);
    receiver.Create (num);
    routers.Create (2);

    // Install Stack
    InternetStackHelper internet;
    internet.Install (sender);
    internet.Install (receiver);
    internet.Install (routers);

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute  ("DataRate", StringValue ("24Mbps"));
    bottleneckLink.SetChannelAttribute ("Delay", StringValue ("40ms"));
    PointToPointHelper sendedgeLink;
    sendedgeLink.SetDeviceAttribute  ("DataRate", StringValue ("50Mbps"));
    sendedgeLink.SetChannelAttribute ("Delay", StringValue ("5ms"));
    PointToPointHelper receedgeLink;
    receedgeLink.SetDeviceAttribute  ("DataRate", StringValue ("50Mbps"));
    receedgeLink.SetChannelAttribute ("Delay", StringValue ("5ms"));


    bottleneckLink.SetQueue ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("200p")));
    sendedgeLink.SetQueue   ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("5p")));
    receedgeLink.SetQueue   ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("5p")));


    // Create NetDevice containers
    NetDeviceContainer r1r2 = bottleneckLink.Install (routers.Get (0), routers. Get (1));

    std::string filename = "flow-alldata";
    NetDeviceTracer netdeviceTrace (foldname, filename);

    NetDeviceTracer* netdeviceTraces[10];

    vector<NetDeviceContainer> sendEdgeList, receEdgeList;
    for (int i = 0; i < num; ++i)
    {
        NetDeviceContainer sendEdge = sendedgeLink.Install (sender. Get (i), routers. Get (0));
        NetDeviceContainer receEdge = receedgeLink.Install (routers.Get (1), receiver.Get (i));

        netdeviceTrace.ReceTraceNetDevice (sendEdge.Get(1));
        netdeviceTrace.SendTraceNetDevice (receEdge.Get(0));

        sendEdgeList.push_back (sendEdge);
        receEdgeList.push_back (receEdge);

        std::string filename = "flow-" + to_string(i+1);
        NetDeviceTracer* endNetdeviceTrace = new NetDeviceTracer (foldname, filename);
        endNetdeviceTrace->ReceTraceNetDevice (receEdge.Get(1));
        netdeviceTraces[i] = endNetdeviceTrace;
    }

    TrafficControlHelper pfifoHelper;
    uint16_t handle = pfifoHelper.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue ("200p"));
    pfifoHelper.AddInternalQueues (handle, 1, "ns3::DropTailQueue", "MaxSize",StringValue ("200p"));
    pfifoHelper.Install(r1r2);  


    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    vector<Ipv4InterfaceContainer> islist, irlist;

    Ipv4InterfaceContainer i1i2 = address.Assign (r1r2);
    for (int i = 0; i < num; ++i)
    {
        address.NewNetwork ();
        Ipv4InterfaceContainer is = address.Assign (sendEdgeList[i]);
        islist.push_back (is);
    }
    for (int i = 0; i < num; ++i)
    {
        address.NewNetwork ();
        Ipv4InterfaceContainer ir = address.Assign (receEdgeList[i]);
        irlist.push_back (ir);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    uint16_t sinkPort = 8080;
    std::string sendSocketFactory = "ns3::SendTcpSocketFactory";
    std::string receSocketFactory[3] = {"ns3::PaceTcpSocketFactory", "ns3::PaceTcpSocketFactory", "ns3::PaceTcpSocketFactory"};

    double startTime [5] = { 10.0, 30.0, 310.0, 60.0, 80.0};
    double endTime   [5] = {150.0, 200.0, 410.0, 160.0, 180.0}; 


    vector<ApplicationContainer> sendapplist, receapplist;
    for (int i = 0; i < num; ++i)
    {
        InetSocketAddress addr_target = InetSocketAddress (irlist[i].GetAddress (1), sinkPort);
        InetSocketAddress addr_listen = InetSocketAddress (Ipv4Address::GetAny (),   sinkPort);

        TracerBulkSendHelper sour (sendSocketFactory, addr_target, Tracer::E_TRACE_SEND_ALL);
        sour.SetAttribute ("CongestionAlgo", StringValue (cc));
        sour.SetAttribute ("MaxBytes", UintegerValue (0));                      // 数据量不设置上限
        sour.SetAttribute ("Foldername", StringValue (foldname));
        
        ApplicationContainer sourApps = sour.Install (sender.Get(i));
        sendapplist.push_back (sourApps);
        sourApps.Start (Seconds (startTime[i]));                              // 设置Application启动时间
        sourApps.Stop  (Seconds (endTime[i])); 


        TracerPacketSinkHelper dest (receSocketFactory[i], addr_listen, Tracer::E_TRACE_RECE_ALL);
        dest.SetAttribute ("Foldername", StringValue (foldname));

        ApplicationContainer destApps = dest.Install (receiver.Get (i));
        destApps.Start (Seconds (10.0));
        destApps.Stop  (Seconds (1005.0));
    }

        
    Simulator::Stop(Seconds(1100.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "end!" << std::endl;

    return 0;
}
