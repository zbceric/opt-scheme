/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-15 16:08:33
 * @LastEditTime: 2022-06-24 22:57:51
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/internet/model/simulation-tcp-socket.h
 */

#ifndef TRACER_SIMULATION_SOCKET_H
#define TRACER_SIMULATION_SOCKET_H

#include <stdint.h>
#include <queue>
#include <iostream>
#include <fstream>
#include "ns3/traced-value.h"
#include "ns3/tcp-socket.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/timer.h"
#include "ns3/sequence-number.h"
#include "ns3/data-rate.h"
#include "ns3/node.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/tcp-rate-ops.h"
#include "ns3/tcp-cubic.h"

#include "pace-tcp-socket-base.h"

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
class TcpSocketBase;
class TcpCubic;
class PaceTcpSocketBase;

class SimulationTcpSocket : public Object
{
public:
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId () const;

    SimulationTcpSocket ();  
    SimulationTcpSocket (const PaceTcpSocketBase& sock);
    virtual ~SimulationTcpSocket (void);


    // Transmission Control Block
    Ptr<TcpSocketState>    m_tcb;               //!< Congestion control information
    Ptr<TcpCongestionOps>  m_congestionControl; //!< Congestion control
    Ptr<TcpRecoveryOps>    m_recoveryOps;       //!< Recovery Algorithm
    Ptr<TcpRateOps>        m_rateOps;           //!< Rate operations

    Ptr<TcpTxBuffer> m_txBuffer;                //!< Tx buffer

    // Timeouts
    TracedValue<Time> m_rto     {Seconds (0.0)}; //!< Retransmit timeout
    Time              m_minRto  {Time::Max ()};   //!< minimum value of the Retransmit timeout
    Time              m_clockGranularity {Seconds (0.001)}; //!< Clock Granularity used in RTO calcs
    Time              m_delAckTimeout    {Seconds (0.0)};   //!< Time to delay an ACK
    Time              m_persistTimeout   {Seconds (0.0)};   //!< Time between sending 1-byte probes
    Time              m_cnTimeout        {Seconds (0.0)};   //!< Timeout for connection retry

    uint32_t          m_initCwnd  {0};
    uint32_t          m_firstTime {0};

    // State-related attributes
    TcpSocket::TcpStates_t   m_state         {TcpSocket::TcpStates_t::CLOSED};         //!< TCP state
    bool                     m_closeNotified {false};  //!< Told app to close socket
    bool                     m_closeOnEmpty  {false};  //!< Close socket upon tx buffer emptied
    bool                     m_shutdownSend  {false};  //!< Send no longer allowed
    bool                     m_shutdownRecv  {false};  //!< Receive no longer allowed
    bool                     m_connected     {false};  //!< Connection established
    double                   m_msl           {0.0};    //!< Max segment lifetime

    // Window management
    uint16_t         m_maxWinSize              {0};  //!< Maximum window size to advertise
    uint32_t         m_bytesAckedNotProcessed  {0};  //!< Bytes acked, but not processed
    SequenceNumber32 m_highTxAck               {0};  //!< Highest ack sent
    TracedValue<uint32_t> m_rWnd               {0};  //!< Receiver window (RCV.WND in RFC793)
    TracedValue<uint32_t> m_advWnd             {0};  //!< Advertised Window size
    TracedValue<SequenceNumber32> m_highRxMark {0};  //!< Highest seqno received
    TracedValue<SequenceNumber32> m_highRxAckMark {0}; //!< Highest ack received

    // ACK management
    uint32_t          m_dupAckCount {0};     //!< Dupack counter
    uint32_t          m_delAckCount {0};     //!< Delayed ACK counter
    uint32_t          m_delAckMaxCount {0};  //!< Number of packet to fire an ACK before delay timeout

    // Retries
    uint32_t          m_synCount     {0}; //!< Count of remaining connection retries
    uint32_t          m_synRetries   {0}; //!< Number of connection attempts
    uint32_t          m_dataRetrCount {0}; //!< Count of remaining data retransmission attempts
    uint32_t          m_dataRetries  {0}; //!< Number of data retransmission attempts

    Ptr<RttEstimator> m_rtt; //!< Round trip time estimator

    // Options
    bool    m_sackEnabled       {true}; //!< RFC SACK option enabled
    bool    m_winScalingEnabled {true}; //!< Window Scale option enabled (RFC 7323)
    uint8_t m_rcvWindShift      {0};    //!< Window shift to apply to outgoing segments
    uint8_t m_sndWindShift      {0};    //!< Window shift to apply to incoming segments
    bool     m_timestampEnabled {true}; //!< Timestamp option enabled
    uint32_t m_timestampToEcho  {0};    //!< Timestamp to echo

    EventId           m_retxEvent     {}; //!< Retransmission event
    EventId           m_lastAckEvent  {}; //!< Last ACK timeout event
    EventId           m_delAckEvent   {}; //!< Delayed ACK timeout event
    EventId           m_persistEvent  {}; //!< Persist event: Send 1 byte to probe for a non-zero Rx window
    EventId           m_timewaitEvent {}; //!< TIME_WAIT expiration event: Move this socket to CLOSED state

    TracedCallback<uint32_t, uint32_t> m_cWndTrace;
    TracedCallback<TcpSocketState::TcpCongState_t, TcpSocketState::TcpCongState_t> m_congStateTrace;

    void UpdateCwnd (uint32_t oldValue, uint32_t newValue);
    void UpdateCongState (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue);

    // Guesses over the other connection end
    bool m_isFirstPartialAck {true}; //!< First partial ACK during RECOVERY

    // Fast Retransmit and Recovery
    SequenceNumber32       m_recover    {0};   //!< Previous highest Tx seqnum for fast recovery (set it to initial seq number)
    bool                   m_recoverActive {false}; //!< Whether "m_recover" has been set/activated
                                                //!< It is used to avoid comparing with the old m_recover value
                                                //!< which was set for handling previous congestion event.
    uint32_t               m_retxThresh {3};   //!< Fast Retransmit threshold
    bool                   m_limitedTx  {true}; //!< perform limited transmit


public:
    Time        m_lastTimestamp {Time::Max ()};     // 预测 Initial Cwnd 所使用
    uint32_t    m_recoverTimestamp {0};
    uint32_t    m_dataEchoTimestamp {0};
    uint16_t    m_sameTimestampCount {0};           // 连续多少个时间戳相同
    uint32_t    m_lastCwnd {0};
    Ptr<Packet> m_packet {0};

    uint32_t GetRWnd (void) const;
    uint32_t GetSegSize (void) const;
    uint32_t GetInitialSSThresh (void) const;

    
    uint32_t DoForwardUp    (Ptr<Packet> packet, const TcpHeader& tcpHeader, Time rtt, Time offset);
    uint32_t FillDataPacket (Ptr<Packet> packet, const TcpHeader& tcpHeader);

    uint32_t ProcessSynSent     (Ptr<Packet> packet, const TcpHeader& tcpHeader);
    uint32_t ProcessEstablished (Ptr<Packet> packet, const TcpHeader& tcpHeader);
    uint32_t ReceivedAck    (Ptr<Packet> packet, const TcpHeader& tcpHeader);

    void ProcessAck         (const SequenceNumber32 &ackNumber, bool scoreboardUpdated, 
                                uint32_t currentDelivered, const SequenceNumber32 &oldHeadSequence);

    void ReadOptions (const TcpHeader &tcpHeader, uint32_t *bytesSacked);
    void ProcessOptionWScale    (const Ptr<const TcpOption> option);
    void ProcessOptionTimestamp (const Ptr<const TcpOption> option, const SequenceNumber32 &seq);
    uint32_t ProcessOptionSack  (const Ptr<const TcpOption> option);


    void EstimateRtt (Time& time, Time& offset);
    void UpdateWindowSize (const TcpHeader &header);    


    
    uint32_t SendPendingData (bool withAck);
    uint32_t BytesInFlight () const;
    uint32_t SafeSubtraction (uint32_t a, uint32_t b);
    void DoRetransmit ();
    void NewAck (SequenceNumber32 const& ack, bool resetRTO);
    void DupAck (uint32_t currentDelivered);
    void EnterRecovery (uint32_t currentDelivered);
    uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck);
    uint32_t AvailableWindow () const;
    uint32_t Window (void) const;
    uint32_t UnAckDataCount () const;
    void ReTxTimeout ();


    TcpTxItem* FillPacket (SequenceNumber32 seq, uint32_t size,Ptr<Packet> packet, bool& listEdited);
    TracedCallback<Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase> > m_txTrace; //!< Trace of transmitted packets

};

} // namespace ns3

#endif
