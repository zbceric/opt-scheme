/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-10 15:10:03
 * @LastEditTime: 2022-06-07 01:03:14
 * @LastEditors: Zhang Bochun
 * @Description: 中心为2个交换机, 左侧连接到若干 sender, 右侧连接到若干 receiver
 * @FilePath: /ns-3.33/scratch/tracer/topology.cc
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

    Packet::EnablePrinting ();
    
    int num = std::stoi(flow);
    num = num > 0 ? num : 1;
    foldname += "/" + cc + "_simulation" + "/" + std::to_string(num) + "flow";


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
    sender.Create (1);
    receiver.Create (1);
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


    bottleneckLink.SetQueue ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("50p")));
    sendedgeLink.SetQueue   ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("5p")));
    receedgeLink.SetQueue   ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("5p")));


    // Create NetDevice containers
    NetDeviceContainer r1r2 =       bottleneckLink.Install (routers.Get (0), routers. Get (1));
    NetDeviceContainer senderEdge =   sendedgeLink.Install (sender. Get (0), routers. Get (0));
    NetDeviceContainer receiverEdge = receedgeLink.Install (routers.Get (1), receiver.Get (0));

    TrafficControlHelper pfifoHelper;
    uint16_t handle = pfifoHelper.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue ("50p"));
    pfifoHelper.AddInternalQueues (handle, 1, "ns3::DropTailQueue", "MaxSize",StringValue ("50p"));
    pfifoHelper.Install(r1r2);  


    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i2 = address.Assign (r1r2);
    address.NewNetwork ();
    Ipv4InterfaceContainer is1  = address.Assign (senderEdge);
    address.NewNetwork ();
    Ipv4InterfaceContainer ir1  = address.Assign (receiverEdge);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // std::string netdevicefolder = "netdevice";
    // Ptr<TracerNetDevice> r1r2_1         = CreateObject<TracerNetDevice> (r1r2.Get(0),        i1i2.GetAddress (0), netdevicefolder);
    // Ptr<TracerNetDevice> r1r2_2         = CreateObject<TracerNetDevice> (r1r2.Get(1),        i1i2.GetAddress (1), netdevicefolder);
    // Ptr<TracerNetDevice> senderEdge_1   = CreateObject<TracerNetDevice> (senderEdge.Get(0),   is1.GetAddress (0), netdevicefolder);
    // Ptr<TracerNetDevice> senderEdge_2   = CreateObject<TracerNetDevice> (senderEdge.Get(1),   is1.GetAddress (1), netdevicefolder);
    // Ptr<TracerNetDevice> receiverEdge_1 = CreateObject<TracerNetDevice> (receiverEdge.Get(0), ir1.GetAddress (0), netdevicefolder);
    // Ptr<TracerNetDevice> receiverEdge_2 = CreateObject<TracerNetDevice> (receiverEdge.Get(1), ir1.GetAddress (1), netdevicefolder);
    
    

    // Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    // em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
    // receiverEdge.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));


    uint16_t sinkPort = 8080;
    std::string sendSocketFactory = "ns3::SendTcpSocketFactory";
    InetSocketAddress addr_target = InetSocketAddress (ir1.GetAddress (1), sinkPort);
    InetSocketAddress addr_listen = InetSocketAddress (Ipv4Address::GetAny(), sinkPort);


    TracerBulkSendHelper sour (sendSocketFactory, addr_target, Tracer::E_TRACE_SEND_ALL | Tracer::E_TRACE_SEND_ALLRTT);
    sour.SetAttribute ("CongestionAlgo", StringValue (cc));
    sour.SetAttribute ("MaxBytes", UintegerValue (0));                      // 数据量不设置上限
    sour.SetAttribute ("Foldername", StringValue (foldname));


    list<ApplicationContainer>   list_app;
    for (int i = 0; i < num; ++i)
    {
        ApplicationContainer sourApps = sour.Install (sender.Get(0));
        list_app.push_back (sourApps);
        sourApps.Start (Seconds (10.0 + 10 * i));                              // 设置Application启动时间
        sourApps.Stop  (Seconds (40.0)); 
    }
        

    // 在 node 1 中安装接收端应用
    std::string receSocketFactory = "ns3::PaceTcpSocketFactory";
    // std::string receSocketFactory = "ns3::TcpSocketFactory";
    TracerPacketSinkHelper dest (receSocketFactory, addr_listen, Tracer::E_TRACE_RECE_ALL);
    dest.SetAttribute ("Foldername", StringValue (foldname));


    ApplicationContainer destApps = dest.Install (receiver.Get (0));
    destApps.Start (Seconds (10.0));
    destApps.Stop  (Seconds (105.0));


    Simulator::Stop(Seconds(110.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "end!" << std::endl;

    return 0;
}
