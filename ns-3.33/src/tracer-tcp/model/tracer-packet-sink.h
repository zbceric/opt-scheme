/*
 * @Author: Zhang Bochun
 * @Date: 2022-03-17 16:42:21
 * @LastEditTime: 2022-04-11 22:41:27
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/scratch/tracer-tcp/tracer-packet-sink.h
 */

#ifndef TRACER_PACKET_SINK_H
#define TRACER_PACKET_SINK_H

#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <functional>
#include <unordered_map>

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/seq-ts-size-header.h"

#include "tracer.h"

namespace ns3 {

class Address;
class Socket;
class Packet;


class TracerPacketSink : public Application 
{
public:
    static TypeId GetTypeId (void);
    
    TracerPacketSink ();
    virtual ~TracerPacketSink ();

    uint64_t GetTotalRx () const;
    Ptr<Socket> GetListeningSocket (void) const;
    std::list<Ptr<Socket> > GetAcceptedSockets (void) const;


    typedef void (* SeqTsSizeCallback)(Ptr<const Packet> p, const Address &from, const Address & to,
                                   const SeqTsSizeHeader &header);

protected:
    virtual void DoDispose (void);

private:
    // inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop

    void HandleRead (Ptr<Socket> socket);
    void HandleAccept (Ptr<Socket> socket, const Address& from);
    void HandlePeerClose (Ptr<Socket> socket);
    void HandlePeerError (Ptr<Socket> socket);

    void PacketReceived (const Ptr<Packet> &p, const Address &from, const Address &localAddress);

    struct AddressHash
    {
        size_t operator() (const Address &x) const
        {
        NS_ABORT_IF (!InetSocketAddress::IsMatchingType (x));
        InetSocketAddress a = InetSocketAddress::ConvertFrom (x);
        return std::hash<uint32_t>()(a.GetIpv4 ().Get ());
        }
    };

    std::unordered_map<Address, Ptr<Packet>, AddressHash> m_buffer; //!< Buffer for received packets

    // In the case of TCP, each socket accept returns a new socket, so the
    // listening socket is stored separately from the accepted sockets
    Ptr<Socket>     m_socket;       //!< Listening socket
    std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets

    Address         m_local;        //!< Local address to bind to
    uint64_t        m_totalRx;      //!< Total bytes received
    TypeId          m_tid;          //!< Protocol TypeId

    bool            m_enableSeqTsSizeHeader {false}; //!< Enable or disable the export of SeqTsSize header 

    /// Traced Callback: received packets, source address.
    TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;
    /// Callback for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
    /// Callbacks for tracing the packet Rx events, includes source, destination addresses, and headers
    TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader&> m_rxTraceWithSeqTsSize;

private:
    uint16_t    m_traceflag;
    Ptr<Tracer> m_tracer;
    std::string m_foldername;
    std::list<Ptr<Tracer>> m_tracerList; //!< the accepted sockets
};

} // namespace ns3

#endif /* PACKET_SINK_H */

