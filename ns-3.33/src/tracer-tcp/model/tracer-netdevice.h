/*
 * @Author: Zhang Bochun
 * @Date: 2022-05-05 20:42:24
 * @LastEditTime: 2022-05-06 11:43:36
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/model/tracer-netdevice.h
 */

#ifndef TRACER_NETDEVICE_H
#define TRACER_NETDEVICE_H


#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "tracer-utils.h"

namespace ns3 {

class TracerNetDevice : public Object
{
public:
    static TypeId GetTypeId (void);

    TracerNetDevice (void) = default;
    TracerNetDevice (Ptr<NetDevice> netdevice, Ipv4Address addr, std::string& foldername);

    void RegisterTraceFunctions ();

protected:
    virtual void DoDispose (void);

    private:
    Ipv4Address     m_addr;
    Ptr<NetDevice>  m_netdevice;
    std::fstream    m_txTrace;
    std::fstream    m_rxTrace;
    std::fstream    m_inoutTrace;
    std::string     m_foldername;
    Ptr<Queue<Packet>> m_queue;

    std::queue<std::fstream*> m_filelist;

    void OpenRxTracerFile    (std::string filename);
    void OpenTxTracerFile    (std::string filename);
    void OpenInOutTracerFile (std::string filename);
    void TraceTxCallback       (Ptr<const Packet> packet);
    void TraceRxCallback       (Ptr<const Packet> packet);
    void TraceEnqueueCallback  (Ptr<const Packet> packet);
    void TraceDequeueCallback  (Ptr<const Packet> packet);
    void TraceDropCallback     (Ptr<const Packet> packet);
    void TraceDropBfECallback  (Ptr<const Packet> packet);
    void TraceDropAfDCallback  (Ptr<const Packet> packet);

    void PrintToFile (Ptr<const Packet> packet, std::fstream& file);

};

}

#endif