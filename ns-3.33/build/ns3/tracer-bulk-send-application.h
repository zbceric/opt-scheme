/*
 * @Author: Zhang Bochun
 * @Date: 2022-03-12 22:09:18
 * @LastEditTime: 2022-04-07 15:17:43
 * @LastEditors: Zhang Bochun
 * @Description: Same as BulkSendApplication, add function like tracing attributions.
 *               and as a tcp sender, we trace attributions such as cwnd, sending rate, inflight and RTT.
 * @FilePath: /ns-3.33/scratch/tracer-tcp/tracer-bulk-send-application.h
 */

#ifndef TRACER_BULK_SEND_APPLICATION_H
#define TRACER_BULK_SEND_APPLICATION_H


#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sys/stat.h>

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/tcp-socket-base.h"

#include "tracer.h"

namespace ns3 {

class Address;
class Socket;

class TracerBulkSendApplication : public Application
{
public:
    static TypeId GetTypeId (void);

    TracerBulkSendApplication ();
    virtual ~TracerBulkSendApplication ();

    void SetMaxBytes (uint64_t maxBytes);
    Ptr<Socket> GetSocket (void) const;

protected:
    virtual void DoDispose (void);

private:
    // inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop

    void SendData (const Address &from, const Address &to);

    Ptr<Socket>     m_socket;       //!< Associated socket
    Address         m_peer;         //!< Peer address
    Address         m_local;        //!< Local address to bind to
    bool            m_connected;    //!< True if connected
    uint32_t        m_sendSize;     //!< Size of data to send each time
    uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
    uint64_t        m_totBytes;     //!< Total bytes sent so far
    TypeId          m_tid;          //!< The type of protocol to use.
    uint32_t        m_seq {0};      //!< Sequence
    Ptr<Packet>     m_unsentPacket; //!< Variable to cache unsent packet
    bool            m_enableSeqTsSizeHeader {false}; //!< Enable or disable the SeqTsSizeHeader

    /// Traced Callback: sent packets
    TracedCallback<Ptr<const Packet> > m_txTrace;
    /// Callback for tracing the packet Tx events, includes source, destination,  the packet sent, and header
    TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader &> m_txTraceWithSeqTsSize;


    void ConnectionSucceeded (Ptr<Socket> socket);
    void ConnectionFailed (Ptr<Socket> socket);
    void DataSend (Ptr<Socket>, uint32_t); // for socket's SetSendCallback

private:
    uint16_t    m_traceflag;
    Ptr<Tracer> m_tracer;
    std::string m_foldername;
    std::string m_ccalgoname;
    void SetCongestionAlgo (void);
    void RegisterTraceFunctions (void);
};

}

#endif /* TRACER_BULK_SEND_APPLICATION_H */