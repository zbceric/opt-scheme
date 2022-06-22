/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-10 15:10:03
 * @LastEditTime: 2022-06-09 22:34:57
 * @LastEditors: Zhang Bochun
 * @Description: 中心为2个交换机, 左侧连接到若干 sender, 右侧连接到若干 receiver
 * @FilePath: /ns-3.33/scratch/loss/topology.cc
 */


#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <memory>
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/stats-module.h"
#include "ns3/tracer-tcp-module.h"

#include "random-loss.h"

using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("TracerTcpCubic");

static const uint32_t PACKET_SIZE  = 1500;

int main (int argc, char *argv[])
{
    LogComponentEnable("TracerTcpCubic", LOG_LEVEL_ALL);


    std::string cc("cubic");                                    // 默认使用Cubic算法
    std::string flow("2");                                      // flow 数量 
    std::string foldname("default"); 
    std::string loss_str("0");

    CommandLine cmd;
    cmd.AddValue ("cc", "congestion algorithm", cc);            // cmd --cc=xxx 设置拥塞算法
    cmd.AddValue ("flow", "num of flow", flow);
    cmd.AddValue ("foldname", "folder name to collect data", foldname);  // cmd --foldername==xxx 设置数据保存的文件夹
    cmd.AddValue ("lo", "loss rate (integer)", loss_str);
    cmd.Parse(argc, argv);   
    

    int num = std::stoi(flow);
    num = num > 0 ? num : 1;
    foldname += "/" + cc + "_simulation" + "/" + std::to_string(num) + "flow";
    int loss_level = std::stoi (loss_str);
    double loss_rate = loss_level * 1.0 / 10000;
    foldname += " - optimal - " + std::to_string(loss_rate);


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


    bottleneckLink.SetQueue ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("50p")));
    sendedgeLink.SetQueue   ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("5p")));
    receedgeLink.SetQueue   ("ns3::DropTailQueue",  "MaxSize", QueueSizeValue (QueueSize ("5p")));


    // Create NetDevice containers
    NetDeviceContainer r1r2 = bottleneckLink.Install (routers.Get (0), routers. Get (1));
    std::unique_ptr<TriggerRandomLoss> triggerloss = nullptr;
    if (loss_level != 0)
    {
        Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (loss_rate));
        Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));
        Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (loss_rate));
        Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));
        
        triggerloss.reset(new TriggerRandomLoss());
        triggerloss->Start(Time ("10s"));

        triggerloss->RegisterDevice (r1r2.Get (1));
    }

    vector<NetDeviceContainer> sendEdgeList, receEdgeList;
    for (int i = 0; i < num; ++i)
    {
        NetDeviceContainer sendEdge = sendedgeLink.Install (sender. Get (i), routers. Get (0));
        NetDeviceContainer receEdge = receedgeLink.Install (routers.Get (1), receiver.Get (i));
        sendEdgeList.push_back (sendEdge);
        receEdgeList.push_back (receEdge);
    }

    TrafficControlHelper pfifoHelper;
    uint16_t handle = pfifoHelper.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue ("50p"));
    pfifoHelper.AddInternalQueues (handle, 1, "ns3::DropTailQueue", "MaxSize",StringValue ("50p"));
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
    std::string receSocketFactory = "ns3::PaceTcpSocketFactory";

    double startTime [5] = {10.0, 20.0, 40.0, 60.0, 80.0};
    double endTime   [5] = {100.0, 120.0, 140.0, 160.0, 180.0}; 


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


        TracerPacketSinkHelper dest (receSocketFactory, addr_listen, Tracer::E_TRACE_RECE_ALL);
        dest.SetAttribute ("Foldername", StringValue (foldname));

        ApplicationContainer destApps = dest.Install (receiver.Get (i));
        destApps.Start (Seconds (10.0));
        destApps.Stop  (Seconds (295.0));
    }

        
    Simulator::Stop(Seconds(300.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "end!" << std::endl;

    return 0;
}
