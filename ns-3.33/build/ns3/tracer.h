/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-05 20:21:41
 * @LastEditTime: 2022-04-23 21:19:25
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/model/tracer.h
 */


#ifndef TRACER_H
#define TRACER_H


#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "tracer-utils.h"

namespace ns3 {

class Tracer : public Object
{
public:
    static TypeId GetTypeId (void);

    enum TraceEnable: uint16_t {       // 枚举变量 用来控制trace哪些属性
        E_TRACE_SEND_CWND      = 0x0001 << 0,    // cwnd
        E_TRACE_SEND_INFLIGHT  = 0x0001 << 2,    // inflight
        E_TRACE_SEND_RTT       = 0x0001 << 3,    // Rtt
        E_TRACE_SEND_SENDRATE  = 0x0001 << 4,    // SendRate
        E_TRACE_SEND_DELIVERED = 0x0001 << 5,
        E_TRACE_SEND_ALLRTT    = 0x0001 << 6,
        E_TRACE_SEND_CONGSTATE = 0x0001 << 7,
        
        E_TRACE_RECE_RTT       = 0x0001 << 8,
        E_TRACE_RECE_CWND      = 0x0001 << 9,
        E_TRACE_RECE_RECERATE  = 0x0001 << 10,
        E_TRACE_RECE_CONGSTATE = 0x0001 << 11,
        E_TRACE_RECE_SIMTX     = 0x0001 << 12,

        E_TRACE_SEND_ALL = E_TRACE_SEND_CWND | E_TRACE_SEND_RTT | E_TRACE_SEND_SENDRATE | E_TRACE_SEND_INFLIGHT  | E_TRACE_SEND_DELIVERED | E_TRACE_SEND_CONGSTATE,
        E_TRACE_RECE_ALL = E_TRACE_RECE_CWND | E_TRACE_RECE_RTT | E_TRACE_RECE_RECERATE | E_TRACE_RECE_CONGSTATE | E_TRACE_RECE_SIMTX,

    };

    Tracer (void) = default;
    Tracer (Ptr<Socket> socket, uint16_t flag, std::string& foldername);
    
    void RegisterTraceFunctions ();

protected:
    virtual void DoDispose (void);

private:
    Ptr<Socket>          m_socket;
    
    Time                 m_minTxRTT;
    Time                 m_minRxRTT;
    Time                 m_lastTxTime;
    Time                 m_lastRxTime;
    Time                 m_gapTime;
    
    Address              m_local;
    Address              m_peer;
    
    uint16_t             m_traceFlag;
    uint32_t             m_mss;
    
    uint128_t            m_lastTxBytes;
    uint128_t            m_lastRxBytes;
    uint128_t            m_totalTxBytes;
    uint128_t            m_totalRxBytes;


    std::string          m_foldername;
    std::string          m_send_filename;
    std::string          m_rece_filename;

    std::fstream         m_send_cwnd;
    std::fstream         m_send_rtt;
    std::fstream         m_send_rtt_up;
    std::fstream         m_send_rtt_down;
    std::fstream         m_send_all_rtt;
    std::fstream         m_send_inflight;
    std::fstream         m_send_sendrate;
    std::fstream         m_send_delivered;

    std::fstream         m_send_rx_trace;
    std::fstream         m_send_tx_trace;

    std::fstream         m_send_tcpstate;
    std::fstream         m_send_congstate;

    std::fstream         m_rece_cwnd;
    std::fstream         m_rece_rtt;
    std::fstream         m_rece_rtt_up;
    std::fstream         m_rece_rtt_down;
    std::fstream         m_rece_inflight;
    std::fstream         m_rece_recerate;
    std::fstream         m_rece_delivered;

    std::fstream         m_rece_rx_trace;
    std::fstream         m_rece_tx_trace;
    std::fstream         m_rece_sim_tx_trace;

    std::fstream         m_rece_tcpstate;
    std::fstream         m_rece_congstate;

    std::queue<std::fstream*> m_filelist;


    // sender
    void OpenSendCwndTraceFile    (std::string filename);
    void TraceSendCwndCallback    (uint32_t oldValue, uint32_t newValue);

    void OpenSendRttTraceFile     (std::string filename);
    void TraceSendRttCallback     (Time oldValue, Time newValue);
    void TraceSendUpRttCallback   (Time oldValue, Time newValue);
    void TraceSendDownRttCallback (Time oldValue, Time newValue);
    void OpenSendAllRttTraceFile  (std::string filename);
    void TraceSendAllRttCallback  (Time oldValue, Time newValue);   

    void OpenSendInflightTraceFile  (std::string filename);
    void TraceSendInflightCallback  (uint32_t oldValue, uint32_t newValue);

    void OpenSendDeliveredTraceFile (std::string filename);
    void TraceSendDeliveredCallback (const TcpRateOps::TcpRateSample &sample);

    void OpenSendRateTraceFile      (std::string filename);
    void OpenSendTxRxTracerFile     (std::string filename);
    void TraceSendPacketTxCallback  (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base);
    void TraceSendPacketRxCallback  (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base);

    void OpenSendTcpStateTraceFile      (std::string filename);
    void OpenSendCongStateTraceFile     (std::string filename);
    void TraceSendTcpStateCallback  (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue);
    void TraceSendCongStateCallback (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue);


    // TraceSendPacketTxCallback 记录 data packet 发送信息, 并统计 sendrate;
    // TraceSendPacketRxCallback 记录 pure ACK 接收信息;

    // receiver
    void OpenReceCwndTraceFile      (std::string filename);
    void TraceReceCwndCallback      (uint32_t oldValue, uint32_t newValue);

    void OpenReceRttTraceFile      (std::string filemame);
    void TraceReceRttCallback      (Time oldValue, Time newValue);
    void TraceReceUpRttCallback    (Time oldValue, Time newValue);
    void TraceReceDownRttCallback  (Time oldValue, Time newValue);

    void OpenReceRateTraceFile     (std::string filename);
    void OpenReceTxRxTracerFile    (std::string filename);
    void TraceRecePacketRxCallback (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base);
    void TraceRecePacketTxCallback (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base);

    void OpenReceTcpStateTraceFile  (std::string filename);
    void OpenReceCongStateTraceFile (std::string filename);
    void TraceReceTcpStateCallback  (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue);
    void TraceReceCongStateCallback (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue);


    void OpenReceSimPacketTxCallback  (std::string filename);
    void TraceReceSimPacketRxCallback (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base);

    // TraceRecePacketRxCallback 记录 data packet 接收信息, 并统计 recerate;
    // TraceRecePacketTxCallback 记录 pure ACK 发送信息;
    
    // 公共处理函数
    void CalculateWithPacket (Ptr<const Packet>& packet, std::fstream& file, uint128_t& lastBytes, uint128_t& totalBytes, Time& time, Time& minRTT);

    typedef std::list<TcpTxItem*> PacketList; //!< container for data stored in the buffer

    std::string MapCongState (TcpSocketState::TcpCongState_t value);
    std::string ReadOptions (const TcpHeader &tcpHeader);
    std::string ProcessOptionSack (const Ptr<const TcpOption> option);


};

}

#endif
