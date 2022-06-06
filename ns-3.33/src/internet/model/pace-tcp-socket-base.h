/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-22 10:10:43
 * @LastEditTime: 2022-06-07 01:06:46
 * @LastEditors: Zhang Bochun
 * @Description: Receiver Pace ACK Tcp Socket Base
 * @FilePath: /ns-3.33/src/internet/model/pace-tcp-socket-base.h
 */

#ifndef PACE_TCP_SOCKET_BASE_H
#define PACE_TCP_SOCKET_BASE_H

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

#include "tcp-rate-ops.h"
#include "tcp-tx-buffer.h"
#include "tcp-option-ts.h"
#include "windowed-filter.h"

#include <fstream>

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

class TcpSocketState;
class SimulationTcpSocket;

class PaceTcpSocketBase : public TcpSocketBase
{
public:
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId () const;

    PaceTcpSocketBase (void);
    PaceTcpSocketBase (const PaceTcpSocketBase& sock);
    virtual ~PaceTcpSocketBase (void);

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

protected:
    uint8_t index;
    static uint8_t Index;

    std::fstream maxRateFile;
    std::fstream dataRateFile;
    std::fstream targetRateFile;

    friend class SimulationTcpSocket;
    // friend class PaceTcpSocketBase;

    class TcpPacket
    {
    public:
        TcpTxItem*  m_item;
        TcpHeader   m_head;
    };

    // typedef enum
    // {
    //     PC_BEGIN,       // 处于慢启动开始状态
    //     PC_BGREC,
    //     PC_BGPRB,       // 慢启动结束, 借助慢启动的突发探测最大带宽
    //     PC_BGDRN,       // 慢启动过后的排空阶段
    //     PC_CONGE,       // 处于拥塞状态, 开始丢包了
    //     PC_DRAIN,       // 处于排空状态, 记录上一个Round的发生速率, 如果这个Round测量的接收速率还要高于这个
    //     PC_NORML,
    //     PC_RECOV,
    //     PC_STABL,
    // } PacingState;


    typedef enum
    {
        PC_BEGIN,           // 处于慢启动开始状态
        PC_BGREC,           // 恢复
        PC_PROBE,           // 寻找收敛点
        PC_STABL,           // 稳定状态
        PC_RECVR,
        PC_DRAIN,
    } PacingState;
    
    typedef std::list<TcpPacket*> TcpPacketList; //!< container for data stored in the buffer
    typedef WindowedFilter<DataRate, MaxFilter<DataRate>, uint32_t, uint32_t> MaxBandwidthFilter_t; //!< Definition of max bandwidth filter.

    bool    m_isSimulationEnable {true};
    bool    m_isAckPacingEnable  {true};
    Ptr<SimulationTcpSocket>    m_peerSocket;


    TcpPacketList   m_ackList;
    TcpPacketList   m_ackSentList;
    EventId         m_ackSentEvent {};

    // uint32_t    m_probeRound {0};
    uint32_t    m_noIncreaseRoundCnt {0};
    DataRate    m_roundMaxBW[3] {0, 0, 0};      // 上上个 round的最大bw, 上个round的最大bw, 这个round的最大bw
    DataRate    m_lastRndPCR[3] {0, 0, 0};      // 对应的是最大 pacing 速度

    Time        m_recentRtt[3] = {Time (0), Time (0), Time (0)};

    TracedValue<Time>   m_lastSendRtt;
    TracedValue<Time>   m_lastReceRtt;
    Ptr<RttEstimator>   m_SendRttEstimator;
    Ptr<RttEstimator>   m_ReceRttEstimator;
    

    Time    m_lastRxTime {"0s"};
    Time    m_minRxRTT {Time::Max ()};
    Time    m_maxRxRTT {0};
    Time    m_lastRoundRtt {0};
    Time    m_currRoundRtt {0};
    Time    m_delayLow;
    Time    m_delayHigh;
    Time    m_timestampOffset;
    double  m_alpha {0.05}, m_beta {0.20};
    DataRate m_step = DataRate ("0.2Mbps");

    DataRate m_lastRoundPacingRate;
    DataRate m_ltltRoundPacingRate;


    PacingState     m_pacingState {PC_BEGIN};

    bool            m_exitRecovery {false};
    bool            m_enterDrain   {true};
    bool            m_exitDrain   {false};

    // PacingState     m_pacingState {PC_SLOWSTART};
    DataRate        m_bandwidth    {"0Mbps"};               // 最新计算得到的 receive rate
    DataRate        m_maxBandwidth {"0Mbps"};               // 自连接创建以来的最大 bw
    DataRate        m_constAckRate  {DataRate("0Mbps")};
    DataRate        m_pacingAckRate {DataRate("0Mbps")};   
    DataRate        m_lastRoundMxBw {DataRate("0Mbps")};
    DataRate        m_ltltRoundMxBw {DataRate("0Mbps")};
    MaxBandwidthFilter_t   m_maxBwFilter;                          //!< Maximum bandwidth filter

    uint32_t        m_round {0};
    uint32_t        m_recordRound {0};
    uint32_t        m_lastEcho {0};
    // uint32_t        m_windowLength {8};
    uint128_t       m_lastRxBytes;
    uint128_t       m_totalRxBytes;
    bool            m_roundStart {false};
    double          m_factor {0.95};
    double          m_gradient {0};
    uint128_t       m_nextRound {0};

    Time                m_recordlastSendRtt;
    Time                m_recordlastReceRtt;
    uint32_t            m_recordSendCwnd;

    SequenceNumber32 m_currentMaxReceived {0};

    // SequenceNumber32 m_lastMarkSequence;
    // SequenceNumber32 m_currMarkSequence;


    void Update (void);
    void UpdateSendRtt  (Time oldValue, Time newValue);
    void UpdateReceRtt  (Time oldValue, Time newValue);
    void UpdateSendCwnd (uint32_t oldValue, uint32_t newValue);
    void UpdateSendCongState (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue);
    void TraceSimSendPacketTx (Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase>);
    void AppendAck (Ptr<Packet> packet, TcpHeader& header);
    void SendAck ();
    void PaceAndScheduleAck (Ptr<Packet> packet, TcpHeader header, SequenceNumber32 seq);
    void DestoryTcpPacket (TcpPacket* tcpPacket);

    
    TcpSocketState::TcpCongState_t m_lastCongState;
    TracedCallback<Time, Time> m_lastSendRttTrace;
    TracedCallback<Time, Time> m_lastReceRttTrace;
    TracedCallback<uint32_t, uint32_t> m_SendCwndTrace;
    TracedCallback<TcpSocketState::TcpCongState_t, TcpSocketState::TcpCongState_t> m_sendCongStateTrace;
    TracedCallback<Ptr<const Packet>, const TcpHeader&, Ptr<const TcpSocketBase> > m_simTxTrace; //!< Trace of transmitted packets


    void Initialization (void);
    void ResetRound ();
    void CalculateRound (TcpPacket* tcpPacket);
    void CalculateBandwidth (Ptr<Packet> packet);
    void CalculateGradient ();
    // void UpdateParameter ();
    void UpdateAckPacingRate ();
    void ProcessOptionTimestamp (const Ptr<const TcpOption> option, const SequenceNumber32 &seq);

    static const int CYCLE = 4;
    // double FactorBaseCycle[CYCLE] = {1.25, 0.75, 1, 1, 1, 1, 1, 1};
    int cycleoffset = 0;
    // double FactorBaseCycle[CYCLE] = {1, 1, 1, 1, 1, 1, 1, 1};
    // double FactorBaseCycle[CYCLE] = {1.15, 0.85, 1, 1, 1, 1, 1, 1};
    // double FactorBaseCycle[CYCLE] = {1.05, 0.90, 0.96, 0.96, 0.96, 0.96, 0.96, 0.96};
    // double FactorBaseCycle[CYCLE] = {1.05, 0.90, 1, 1, 1, 1, 1, 1};

    // double FactorBaseCycle[CYCLE] = {1.05, 0.90, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95};

    int flags [CYCLE] = {1, 0, 0, 0};

};

}

#endif