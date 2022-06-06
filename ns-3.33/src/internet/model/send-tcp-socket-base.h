/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-22 10:10:00
 * @LastEditTime: 2022-04-22 10:23:22
 * @LastEditors: Zhang Bochun
 * @Description: Sender Tcp Socket Base
 * @FilePath: /ns-3.33/src/tracer/model/send-tcp-socket-base.h
 */

#ifndef SEND_TCP_SOCKET_BASE_H
#define SEND_TCP_SOCKET_BASE_H

#include <stdint.h>
#include <queue>
#include "ns3/traced-value.h"
#include "ns3/tcp-socket.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/timer.h"
#include "ns3/sequence-number.h"
#include "ns3/data-rate.h"
#include "ns3/node.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/tcp-socket-base.h"

namespace ns3 {

class Ipv4EndPoint;
class Ipv6EndPoint;
class Node;
class Packet;
class TcpL4Protocol;
class TcpHeader;
class TcpCongestionOps;
class TcpRecoveryOps;
class RttEstimator;
class TcpRxBuffer;
class TcpTxBuffer;
class TcpOption;
class Ipv4Interface;
class Ipv6Interface;
class TcpRateOps;
class RttHistory;

/**
 * 继承与 TcpSocketBase, 在其中添加了部分 TraceSource 等,
 * 避免了直接修改 TcpSocketBase, 影响其他内容;
 */
class SendTcpSocketBase : public TcpSocketBase
{
public:
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId () const;

    SendTcpSocketBase (void);
    SendTcpSocketBase (const SendTcpSocketBase& sock);
    virtual ~SendTcpSocketBase (void);

    virtual void SetNode (Ptr<Node> node);
    virtual void SetTcp (Ptr<TcpL4Protocol> tcp);
    virtual void SetRtt (Ptr<RttEstimator> rtt);
    
    // Necessary implementations of null functions from ns3::Socket
    virtual enum SocketErrno GetErrno (void) const;    // returns m_errno
    virtual enum SocketType GetSocketType (void) const; // returns socket type
    virtual Ptr<Node> GetNode (void) const;            // returns m_node
    virtual int Bind (void);    // Bind a socket by setting up endpoint in TcpL4Protocol
    virtual int Bind6 (void);    // Bind a socket by setting up endpoint in TcpL4Protocol
    virtual int Bind (const Address &address);         // ... endpoint of specific addr or port
    virtual int Connect (const Address &address);      // Setup endpoint and call ProcessAction() to connect
    virtual int Listen (void);  // Verify the socket is in a correct state and call ProcessAction() to listen
    virtual int Close (void);   // Close by app: Kill socket upon tx buffer emptied
    virtual int ShutdownSend (void);    // Assert the m_shutdownSend flag to prevent send to network
    virtual int ShutdownRecv (void);    // Assert the m_shutdownRecv flag to prevent forward to app
    virtual int Send (Ptr<Packet> p, uint32_t flags);  // Call by app to send data to network
    virtual int SendTo (Ptr<Packet> p, uint32_t flags, const Address &toAddress); // Same as Send(), toAddress is insignificant
    virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags); // Return a packet to be forwarded to app
    virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress); // ... and write the remote address at fromAddress
    virtual uint32_t GetTxAvailable (void) const; // Available Tx buffer size
    virtual uint32_t GetRxAvailable (void) const; // Available-to-read data size, i.e. value of m_rxAvailable
    virtual int GetSockName (Address &address) const; // Return local addr:port in address
    virtual int GetPeerName (Address &address) const;
    virtual void BindToNetDevice (Ptr<NetDevice> netdevice); // NetDevice with my m_endPoint

protected:
    // Implementing ns3::TcpSocket -- Attribute get/set
    // inherited, no need to doc

    virtual void     SetSndBufSize (uint32_t size);
    virtual uint32_t GetSndBufSize (void) const;
    virtual void     SetRcvBufSize (uint32_t size);
    virtual uint32_t GetRcvBufSize (void) const;
    virtual void     SetSegSize (uint32_t size);
    virtual uint32_t GetSegSize (void) const;
    virtual void     SetInitialSSThresh (uint32_t threshold);
    virtual uint32_t GetInitialSSThresh (void) const;
    virtual void     SetInitialCwnd (uint32_t cwnd);
    virtual uint32_t GetInitialCwnd (void) const;
    virtual void     SetConnTimeout (Time timeout);
    virtual Time     GetConnTimeout (void) const;
    virtual void     SetSynRetries (uint32_t count);
    virtual uint32_t GetSynRetries (void) const;
    virtual void     SetDataRetries (uint32_t retries);
    virtual uint32_t GetDataRetries (void) const;
    virtual void     SetDelAckTimeout (Time timeout);
    virtual Time     GetDelAckTimeout (void) const;
    virtual void     SetDelAckMaxCount (uint32_t count);
    virtual uint32_t GetDelAckMaxCount (void) const;
    virtual void     SetTcpNoDelay (bool noDelay);
    virtual bool     GetTcpNoDelay (void) const;
    virtual void     SetPersistTimeout (Time timeout);
    virtual Time     GetPersistTimeout (void) const;
    virtual bool     SetAllowBroadcast (bool allowBroadcast);
    virtual bool     GetAllowBroadcast (void) const;

    virtual void CompleteFork (Ptr<Packet> p, const TcpHeader& tcpHeader,
                             const Address& fromAddress, const Address& toAddress);
    virtual void DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
                             const Address &toAddress);
    virtual uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck);
    virtual void SendEmptyPacket (uint8_t flags);
    virtual uint32_t UnAckDataCount (void) const;
    virtual uint32_t BytesInFlight (void) const;
    virtual uint32_t Window (void) const;
    virtual uint32_t AvailableWindow (void) const;
    virtual uint16_t AdvertisedWindowSize (bool scale = true) const;
    virtual Ptr<TcpSocketBase> Fork (void);
    virtual void ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader);
    virtual void ProcessAck (const SequenceNumber32 &ackNumber, bool scoreboardUpdated,
                           uint32_t currentDelivered, const SequenceNumber32 &oldHeadSequence);
    virtual void ReceivedData (Ptr<Packet> packet, const TcpHeader& tcpHeader);
    virtual void EstimateRtt (const TcpHeader& tcpHeader);

    virtual void UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
                                  bool isRetransmission);
    virtual void NewAck (SequenceNumber32 const& seq, bool resetRTO);
    virtual void ReTxTimeout (void);
    virtual void DelAckTimeout (void);
    virtual void LastAckTimeout (void);
    virtual void PersistTimeout (void);
};

} // namespace ns3

#endif