/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-15 16:43:01
 * @LastEditTime: 2022-04-25 22:21:51
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/internet/model/simulation-tcp-socket.cc
 */

#include "ns3/abort.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/simulation-singleton.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/data-rate.h"
#include "ns3/object.h"
#include "tcp-l4-protocol.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include "ipv6-l3-protocol.h"
#include "tcp-tx-buffer.h"
#include "tcp-rx-buffer.h"
#include "rtt-estimator.h"
#include "tcp-header.h"
#include "tcp-option-winscale.h"
#include "tcp-option-ts.h"
#include "tcp-option-sack-permitted.h"
#include "tcp-option-sack.h"
#include "tcp-congestion-ops.h"
#include "tcp-recovery-ops.h"
#include "ns3/tcp-rate-ops.h"
#include "simulation-tcp-socket.h"

#include <math.h>
#include <algorithm>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimulationTcpSocket");

NS_OBJECT_ENSURE_REGISTERED (SimulationTcpSocket);

TypeId SimulationTcpSocket::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SimulationTcpSocket")
        .SetParent<Object> ()
        .SetGroupName ("Internet")
        .AddConstructor<SimulationTcpSocket> ()
        .AddTraceSource ("CongestionWindow",
                         "The TCP connection's congestion window",
                         MakeTraceSourceAccessor (&SimulationTcpSocket::m_cWndTrace),
                         "ns3::TracedValueCallback::Uint32")
        .AddTraceSource ("CongState",
                         "TCP Congestion machine state",
                         MakeTraceSourceAccessor (&SimulationTcpSocket::m_congStateTrace),
                         "ns3::TcpSocketState::TcpCongStatesTracedValueCallback")
        .AddTraceSource ("Tx",
                         "Send tcp packet to IP protocol",
                         MakeTraceSourceAccessor (&SimulationTcpSocket::m_txTrace),
                         "ns3::TcpSocketBase::TcpTxRxTracedCallback")
        ;

  return tid;
}

TypeId SimulationTcpSocket::GetInstanceTypeId () const
{
    return SimulationTcpSocket::GetTypeId ();
}


SimulationTcpSocket::SimulationTcpSocket ()
{
    m_txBuffer = CreateObject<TcpTxBuffer> ();
    m_txBuffer->SetRWndCallback (MakeCallback (&SimulationTcpSocket::GetRWnd, this));
    m_tcb = CreateObject<TcpSocketState> ();
    m_tcb->m_rxBuffer = CreateObject<TcpRxBuffer> ();
    m_congestionControl = CreateObject<TcpCubic> ();
    m_recoveryOps = CreateObject<TcpClassicRecovery> ();
    m_rateOps = CreateObject <TcpRateLinux> ();

    m_tcb->TraceConnectWithoutContext ("CongestionWindow",
                                       MakeCallback (&SimulationTcpSocket::UpdateCwnd, this));
    m_tcb->TraceConnectWithoutContext ("CongState",
                                       MakeCallback (&SimulationTcpSocket::UpdateCongState, this));                                   
}


SimulationTcpSocket::SimulationTcpSocket (const PaceTcpSocketBase& sock)
  : m_rto (sock.m_rto),
    m_minRto (sock.m_minRto),
    m_clockGranularity (sock.m_clockGranularity),
    // m_noDelay (true),
    m_delAckTimeout (sock.m_delAckTimeout),
    m_persistTimeout (sock.m_persistTimeout),
    m_cnTimeout (sock.m_cnTimeout),
    m_state (sock.m_state),
    m_closeNotified (sock.m_closeNotified),
    m_closeOnEmpty (sock.m_closeOnEmpty),
    m_shutdownSend (sock.m_shutdownSend),
    m_shutdownRecv (sock.m_shutdownRecv),
    m_connected (sock.m_connected),
    m_msl (sock.m_msl),
    m_maxWinSize (sock.m_maxWinSize),
    m_bytesAckedNotProcessed (sock.m_bytesAckedNotProcessed),
    m_rWnd (sock.m_rWnd),
    m_highRxMark (sock.m_highRxMark),
    m_highRxAckMark (sock.m_highRxAckMark),
    m_dupAckCount (sock.m_dupAckCount),
    m_delAckCount (0),
    m_delAckMaxCount (sock.m_delAckMaxCount),
    m_synCount (sock.m_synCount),
    m_synRetries (sock.m_synRetries),
    m_dataRetrCount (sock.m_dataRetrCount),
    m_dataRetries (sock.m_dataRetries),
    m_sackEnabled (sock.m_sackEnabled),
    m_winScalingEnabled (sock.m_winScalingEnabled),
    m_rcvWindShift (sock.m_rcvWindShift),
    m_sndWindShift (sock.m_sndWindShift),
    m_timestampEnabled (sock.m_timestampEnabled),
    m_timestampToEcho (sock.m_timestampToEcho),
    m_isFirstPartialAck (sock.m_isFirstPartialAck),
    m_recover (sock.m_recover),
    m_recoverActive (sock.m_recoverActive),
    m_retxThresh (sock.m_retxThresh),
    m_limitedTx (sock.m_limitedTx)
{
    m_txBuffer = CreateObject<TcpTxBuffer> ();
    m_txBuffer->SetRWndCallback (MakeCallback (&SimulationTcpSocket::GetRWnd, this));

    m_tcb = CopyObject<TcpSocketState> (sock.m_tcb);
    m_tcb->m_rxBuffer = CopyObject (sock.m_tcb->m_rxBuffer);
    m_txBuffer->m_segmentSize = m_tcb->m_segmentSize;
    
    m_congestionControl = CreateObject<TcpCubic> ();
    m_recoveryOps = CreateObject<TcpClassicRecovery> ();
    m_rateOps = CreateObject <TcpRateLinux> ();

    m_txBuffer->SetDupAckThresh (3);
    m_txBuffer->SetMaxBufferSize (1048576);

    if (sock.m_rtt)
    {
        m_rtt = sock.m_rtt->Copy ();
    }

    m_tcb->TraceConnectWithoutContext ("CongestionWindow",
                                       MakeCallback (&SimulationTcpSocket::UpdateCwnd, this));
    m_tcb->TraceConnectWithoutContext ("CongState",
                                       MakeCallback (&SimulationTcpSocket::UpdateCongState, this));      
}


SimulationTcpSocket::~SimulationTcpSocket (void)
{

}


/**
 * ??? Sender, ??????????????????????????????, ?????? TcpSocketBase ?????? fromAddress ??? toAddress,
 * ???????????????????????????, ????????????????????????
 */
uint32_t SimulationTcpSocket::DoForwardUp (Ptr<Packet> packet, const TcpHeader& tcpHeader, Time rtt, Time offset)
{
    // SequenceNumber32 seq = tcpHeader.GetSequenceNumber ();

    // std::cout << "ACK number: " << tcpHeader.GetAckNumber () << std::endl;

    /**
     * TcpHeader ????????? SYN, ????????????:
     * Window scale, SACK enabled, Timestamps
     */
    if (tcpHeader.GetFlags () & TcpHeader::SYN)
    {
        m_rWnd = tcpHeader.GetWindowSize ();

        if (tcpHeader.HasOption (TcpOption::WINSCALE) && m_winScalingEnabled)
        {
            ProcessOptionWScale (tcpHeader.GetOption (TcpOption::WINSCALE));
        }
        else
        {
            m_winScalingEnabled = false;
        }

        if (tcpHeader.HasOption (TcpOption::SACKPERMITTED) && m_sackEnabled)
        {
            // do nothing
        }
        else
        {
            m_sackEnabled = false;
            m_txBuffer->SetSackEnabled (false);
        }

        if (tcpHeader.HasOption (TcpOption::TS) && m_timestampEnabled)
        {
            ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS), tcpHeader.GetSequenceNumber ());
            m_firstTime = m_firstTime ? m_firstTime : m_tcb->m_rcvTimestampValue;
            m_initCwnd = 0;
        }
        else
        {
            m_timestampEnabled = false;

        }

        // ????????? cwnd
        // m_tcb->m_cWnd = GetInitialCwnd () * GetSegSize ();
        // m_tcb->m_cWndInfl = m_tcb->m_cWnd;
        // m_tcb->m_ssThresh = GetInitialSSThresh ();

        m_tcb->m_cWnd = 0 * GetSegSize ();
        m_tcb->m_cWndInfl = m_tcb->m_cWnd;
        m_tcb->m_ssThresh = GetInitialSSThresh ();

        if (tcpHeader.GetFlags () & TcpHeader::ACK)
        {
            // EstimateRtt (rtt);
            m_highRxAckMark = tcpHeader.GetAckNumber ();
        }
    }

    /**
     * ACK - ?????? RTT (??? timestamp ??? timestamp, ?????????????????? m_history), ????????? Receive Window size
     */
    else if (tcpHeader.GetFlags () & TcpHeader::ACK)
    {
        if (m_timestampEnabled)
        {
            if (!tcpHeader.HasOption (TcpOption::TS))
            {
                return 0;
            }
            else
            {
              ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS), tcpHeader.GetSequenceNumber ());
            }
        }

        EstimateRtt (rtt, offset);
        UpdateWindowSize (tcpHeader);
    }

    uint32_t npacket;
    switch (m_state)
    {
        case TcpSocket::TcpStates_t::ESTABLISHED:
            npacket = ProcessEstablished (packet, tcpHeader);
            break;

        case TcpSocket::TcpStates_t::SYN_SENT:
            npacket = ProcessSynSent (packet, tcpHeader);
            break;
        default: // mute compiler
            npacket = 0;
            break;
    }

    // ?????? sender ??????, ???????????????????????????
    if (m_rWnd.Get () != 0 && m_persistEvent.IsRunning ())
    {
        m_persistEvent.Cancel ();
        // SendPendingData (m_connected);
    }

    if (m_lastCwnd == m_tcb->m_cWnd)
    {
        UpdateCwnd (m_lastCwnd, m_lastCwnd);
    }
    else
    {
        m_lastCwnd = m_tcb->m_cWnd;
    }
    return npacket;
}



// Process blocks according to connection state

/**
 * SYN_SENT
 * ?????? SYN + ACK packet
 */
uint32_t SimulationTcpSocket::ProcessSynSent (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
    uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

    if (tcpflags & (TcpHeader::SYN | TcpHeader::ACK) && m_tcb->m_nextTxSequence + SequenceNumber32 (1) == tcpHeader.GetAckNumber ())
    {   
        // Handshake completed
        m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
        m_tcb->m_congState = TcpSocketState::CA_OPEN;
        m_state = TcpSocket::TcpStates_t::ESTABLISHED;
        m_connected = true;
        m_retxEvent.Cancel ();
        m_tcb->m_rxBuffer->SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
        m_tcb->m_highTxMark = ++m_tcb->m_nextTxSequence;
        m_txBuffer->SetHeadSequence (m_tcb->m_nextTxSequence);

        m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);
        if (m_retxEvent.IsExpired ())
        {
            m_retxEvent = Simulator::Schedule (m_rto, &SimulationTcpSocket::ReTxTimeout, this);
        }
    }

    m_tcb->m_ecnState = TcpSocketState::ECN_DISABLED;

    return 0;
}


/**
 * ESTABLISHED
 * ?????? pure ACK, ????????? ACK ??????????????????????????????, ????????? ReceivedAck ?????????;
 */
uint32_t SimulationTcpSocket::ProcessEstablished (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
    uint32_t res = 0;
    uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG | TcpHeader::CWR | TcpHeader::ECE);

    if (tcpflags == TcpHeader::ACK)
    {
        if (tcpHeader.GetAckNumber () >= m_txBuffer->HeadSequence () && tcpHeader.GetAckNumber () <= m_tcb->m_highTxMark)
        {
            res = ReceivedAck (packet, tcpHeader);
        }
    }

    return res;
}


/**
 * ?????? ACK
 * ???????????????????????????????????????, ???????????????????????? ????????????
 * ??? txBuffer ???????????????
 * ?????? ProcessAck;
 * ???????????? SendPendingData ???????????? (??????????????????????????????????????????)
 */
uint32_t SimulationTcpSocket::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
    // ???????????????????????? rate ?????????, ??????????????????
    // uint32_t previousLost = m_txBuffer->GetLost ();             // ?????????????????? ACK ?????????????????? segment ??????
    // uint32_t priorInFlight = m_tcb->m_bytesInFlight.Get ();     // ?????????????????? ACK ??? inflight ????????????

    uint32_t bytesSacked = 0;
    uint64_t previousDelivered = m_rateOps->GetConnectionRate ().m_delivered;   // ?????????????????? ACK ??? delivered ????????????
    ReadOptions (tcpHeader, &bytesSacked);                                      // ?????? sack ????????????

    SequenceNumber32 ackNumber = tcpHeader.GetAckNumber ();
    SequenceNumber32 oldHeadSequence = m_txBuffer->HeadSequence ();

    // if (ackNumber.GetValue () == 5782501)
    // {
    //     std::cout << "here!" << std::endl;
    // }

    // ?????? ACK ????????????????????????????????????
    if (ackNumber < oldHeadSequence)
    {
        return 0;
    }

    if (m_tcb->m_cWnd == GetSegSize ())
    {
        std::cout << "ACK:" << ackNumber.GetValue () << std::endl; 
    }

    // ?????? ACK ????????????????????? ????????? recover ???
    if ((ackNumber > oldHeadSequence) && (ackNumber < m_recover) && (m_tcb->m_congState == TcpSocketState::CA_RECOVERY))
    {
        uint32_t segAcked = (ackNumber - oldHeadSequence)/m_tcb->m_segmentSize;
        for (uint32_t i = 0; i < segAcked; i++)
        {
            // ??????????????? retransmit data acked
            if (m_txBuffer->IsRetransmittedDataAcked (ackNumber - (i * m_tcb->m_segmentSize)))
            {
                m_tcb->m_isRetransDataAcked = true;
            }
        }
    }

    // ??? txBuffer ????????????
    m_txBuffer->DiscardUpTo (ackNumber, MakeCallback (&TcpRateOps::SkbDelivered, m_rateOps));   // SkbDelivered

    // ???????????? ACK ??? delivered ???????????????
    uint32_t currentDelivered = static_cast<uint32_t> (m_rateOps->GetConnectionRate ().m_delivered - previousDelivered);

    if (m_tcb->m_congState == TcpSocketState::CA_CWR && (ackNumber > m_recover))
    {
        m_tcb->m_congState = TcpSocketState::CA_OPEN;
        if (!m_congestionControl->HasCongControl ())
        {
            m_tcb->m_cWnd = m_tcb->m_ssThresh.Get ();
            m_recoveryOps->ExitRecovery (m_tcb);
            m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_COMPLETE_CWR);
        }
    }

    BytesInFlight ();

    ProcessAck (ackNumber, (bytesSacked > 0), currentDelivered, oldHeadSequence);
    m_tcb->m_isRetransDataAcked = false;

    // ????????????????????? packet
    uint32_t npacket = SendPendingData (m_connected);
    return npacket;
}


/**
 * ??????????????????????????? DupAck, ???????????????????????????, ????????????????????? EnterRecovery;
 * ????????????????????????;
 * 
 * ????????????????????? newAck
 */
void SimulationTcpSocket::ProcessAck(const SequenceNumber32 &ackNumber, bool scoreboardUpdated,
                                     uint32_t currentDelivered, const SequenceNumber32 &oldHeadSequence)
{
    bool exitedFastRecovery = false;
    uint32_t oldDupAckCount = m_dupAckCount; // remember the old value
    m_tcb->m_lastAckedSeq = ackNumber; // Update lastAckedSeq
    uint32_t bytesAcked = 0;

    bool isDupack = m_sackEnabled ?
                    scoreboardUpdated :
                    ackNumber == oldHeadSequence && ackNumber < m_tcb->m_highTxMark;

    if (isDupack)
    {
        DupAck (currentDelivered);
    }

    if (ackNumber == oldHeadSequence && ackNumber == m_tcb->m_highTxMark)
    {
        return;
    }
    else if (ackNumber == oldHeadSequence && ackNumber > m_tcb->m_highTxMark)
    {
        m_tcb->m_nextTxSequence = ackNumber;
    }
    else if (ackNumber == oldHeadSequence)
    {
        m_congestionControl->PktsAcked (m_tcb, 1, m_tcb->m_lastRtt);
    }
    else if (ackNumber > oldHeadSequence)
    {
        bytesAcked = ackNumber - oldHeadSequence;
        uint32_t segsAcked  = bytesAcked / m_tcb->m_segmentSize;
        m_bytesAckedNotProcessed += bytesAcked % m_tcb->m_segmentSize;
        bytesAcked -= bytesAcked % m_tcb->m_segmentSize;

        if (m_bytesAckedNotProcessed >= m_tcb->m_segmentSize)
        {
            segsAcked += 1;
            bytesAcked += m_tcb->m_segmentSize;
            m_bytesAckedNotProcessed -= m_tcb->m_segmentSize;
        }

        if (!isDupack)
        {
            m_dupAckCount = 0;
        }

        if (ackNumber < m_recover && m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
        {
            if (!m_sackEnabled)
            {
                m_txBuffer->MarkHeadAsLost ();
            }

            if (!m_congestionControl->HasCongControl () && segsAcked >= 1)
            {
                m_recoveryOps->DoRecovery (m_tcb, currentDelivered);
            }

            if (!m_txBuffer->IsRetransmittedDataAcked (ackNumber + m_tcb->m_segmentSize))
            {
                DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
                m_tcb->m_cWndInfl = SafeSubtraction (m_tcb->m_cWndInfl, bytesAcked);
            }

            m_congestionControl->PktsAcked (m_tcb, 1, m_tcb->m_lastRtt);
            NewAck (ackNumber, m_isFirstPartialAck);

            if (m_isFirstPartialAck)
            {
                m_isFirstPartialAck = false;
            }
        }
        else if (ackNumber < m_recover && m_tcb->m_congState == TcpSocketState::CA_LOSS)
        {
            m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
            m_congestionControl->IncreaseWindow (m_tcb, segsAcked);

            NewAck (ackNumber, true);
        }
        else if (m_tcb->m_congState == TcpSocketState::CA_CWR)
        {
        }
        else
        {
            if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
            {
                m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
            }
            else if (m_tcb->m_congState == TcpSocketState::CA_DISORDER)
            {
                if (segsAcked >= oldDupAckCount)
                {
                  m_congestionControl->PktsAcked (m_tcb, segsAcked - oldDupAckCount, m_tcb->m_lastRtt);
                }

                if (!isDupack)
                {
                    m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
                    m_tcb->m_congState = TcpSocketState::CA_OPEN;
                }

            }
            else if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
            {
                m_isFirstPartialAck = true;
                
                segsAcked = static_cast<uint32_t>(ackNumber - oldHeadSequence) / m_tcb->m_segmentSize;
                m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
                m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_COMPLETE_CWR);
                m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
                m_tcb->m_congState = TcpSocketState::CA_OPEN;
                exitedFastRecovery = true;
                m_dupAckCount = 0; // From recovery to open, reset dupack
            }
            else if (m_tcb->m_congState == TcpSocketState::CA_LOSS)
            {
                m_isFirstPartialAck = true;

                segsAcked = (ackNumber - m_recover) / m_tcb->m_segmentSize;

                m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);

                m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
                m_tcb->m_congState = TcpSocketState::CA_OPEN;
            }

            if (ackNumber >= m_recover)
            {
              m_recoverActive = false;
            }

            if (exitedFastRecovery)
            {
                NewAck (ackNumber, true);
                m_tcb->m_cWnd = m_tcb->m_ssThresh.Get ();
                // std::cout << "cwnd = " << m_tcb->m_cWnd / GetSegSize () << " -> ssthresh" << std::endl;
                m_recoveryOps->ExitRecovery (m_tcb);
            }
            if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
            {
                m_congestionControl->IncreaseWindow (m_tcb, segsAcked);   // ??????????????????
                m_tcb->m_cWndInfl = m_tcb->m_cWnd;
                // std::cout << "cwnd = " << m_tcb->m_cWnd / GetSegSize () << " -> IncreaseWindow" << std::endl;
                NewAck (ackNumber, true);
            }
        }
    }
}





// Tcp Option

/**
 * ??????????????? Window Scale Option
 * 
 * m_sndWindShift ?????? incoming ????????? (??????)
 * m_rcvWindShift ?????? outgoing ????????? (??????)
 * 
 * ?????????????????? receiver ??? m_sndWindShift ????????? simulation ?????? m_rcvWindShift
 * ???????????? m_sndWindShift
 */
void SimulationTcpSocket::ProcessOptionWScale (const Ptr<const TcpOption> option)
{
    Ptr<const TcpOptionWinScale> ws = DynamicCast<const TcpOptionWinScale> (option);

    m_sndWindShift = ws->GetScale ();

    if (m_sndWindShift > 14)
    {
        m_sndWindShift = 14;
    }
}


/**
 * ?????? & ?????? ?????????
 * ??? timestamp ?????? m_tcb
 */
void SimulationTcpSocket::ProcessOptionTimestamp (const Ptr<const TcpOption> option, const SequenceNumber32 &seq)
{
    Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);

    /**
     * ?????????????????????????????????????????????, ???????????? 50days ????????????
     */
    if (m_tcb->m_rcvTimestampValue > ts->GetTimestamp ())
    {
        return;
    }

    /* ???????????? timestamp value ??? echo value, ???????????? m_tcb (TcpSocketState) */
    m_tcb->m_rcvTimestampValue = ts->GetTimestamp ();
    m_tcb->m_rcvTimestampEchoReply = ts->GetEcho ();

    /* ?????? echo - ????????????????????????????????????????????? RTT */
    if (seq == m_tcb->m_rxBuffer->NextRxSequence () && seq <= m_highTxAck)
    {
        m_timestampToEcho = ts->GetTimestamp ();
    }
}


/**
 * ?????? SACK
 */
uint32_t SimulationTcpSocket::ProcessOptionSack (const Ptr<const TcpOption> option)
{
    Ptr<const TcpOptionSack> s = DynamicCast<const TcpOptionSack> (option);
    return m_txBuffer->Update (s->GetSackList (), MakeCallback (&TcpRateOps::SkbDelivered, m_rateOps));
//     // return m_txBuffer->Update (s->GetSackList (), MakeNullCallback <void, TcpTxItem*> ());
}


/**
 * ?????? RTT
 * ???????????????????????? ...
 */
void SimulationTcpSocket::EstimateRtt (Time& rtt, Time& offset)
{
    Time Rtt = rtt + offset;
    m_rateOps->UpdateRtt(Rtt);               // ?????? RTT.
    m_rtt->Measurement (Rtt);                // Log the measurement
    m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);
    m_tcb->m_lastRtt = m_rtt->GetEstimate ();
    m_tcb->m_minRtt = std::min (m_tcb->m_lastRtt.Get (), m_tcb->m_minRtt);
}


/**
 * ?????? Window size
 */
void SimulationTcpSocket::UpdateWindowSize (const TcpHeader &header)
{

    uint32_t receivedWindow = header.GetWindowSize ();
    receivedWindow <<= m_sndWindShift;
  
    if (m_state < TcpSocket::TcpStates_t::ESTABLISHED)
    {
        m_rWnd = receivedWindow;
        return;
    }


    bool update = false;
    if (header.GetAckNumber () == m_highRxAckMark && receivedWindow > m_rWnd)
    {
        update = true;
    }
    if (header.GetAckNumber () > m_highRxAckMark)
    {
        m_highRxAckMark = header.GetAckNumber ();
        update = true;
    }
    if (header.GetSequenceNumber () > m_highRxMark)
    {
        m_highRxMark = header.GetSequenceNumber ();
        update = true;
    }
    if (update == true)
    {
        m_rWnd = receivedWindow;
    }
}



/**
 * ?????? Option
 */
void SimulationTcpSocket::ReadOptions (const TcpHeader &tcpHeader, uint32_t *bytesSacked)
{
    TcpHeader::TcpOptionList::const_iterator it;
    const TcpHeader::TcpOptionList options = tcpHeader.GetOptionList ();    // ?????? option list

    for (it = options.begin (); it != options.end (); ++it)
    {
        const Ptr<const TcpOption> option = (*it);
        
        switch (option->GetKind ())
        {
            case TcpOption::SACK:
                *bytesSacked = ProcessOptionSack (option);
            break;
            default:
            continue;
        }
    }
}





uint32_t SimulationTcpSocket::GetSegSize (void) const
{
    return m_tcb->m_segmentSize;
}


uint32_t SimulationTcpSocket::GetInitialSSThresh (void) const
{
  return m_tcb->m_initialSsThresh;
}


uint32_t SimulationTcpSocket::GetRWnd (void) const
{
  return m_rWnd.Get ();
}



uint32_t SimulationTcpSocket::BytesInFlight () const
{
    uint32_t bytesInFlight = m_txBuffer->BytesInFlight ();
    m_tcb->m_bytesInFlight = bytesInFlight;
    return bytesInFlight;
}



uint32_t SimulationTcpSocket::SafeSubtraction (uint32_t a, uint32_t b)
{
  if (a > b)
    {
      return a-b;
    }

  return 0;
}


/**
 * ????????????
 * ????????????????????????, ??????????????? SendDataPacket ??????, ??????????????????
 */
void SimulationTcpSocket::DoRetransmit ()
{
    bool res;
    SequenceNumber32 seq;
    SequenceNumber32 seqHigh;
    uint32_t maxSizeToSend;

    res = m_txBuffer->NextSeg (&seq, &seqHigh, false);
    if (!res)
    {
        seq = m_txBuffer->HeadSequence ();
        maxSizeToSend = m_tcb->m_segmentSize;
    }
    else
    {
        maxSizeToSend = static_cast<uint32_t> (seqHigh - seq);
    }

    m_tcb->m_nextTxSequence = seq;

    m_txBuffer->Add(m_packet);
    uint32_t sz = SendDataPacket (m_tcb->m_nextTxSequence, maxSizeToSend, true);

    // std::cout << "This is a transmit packet. size = " << sz << std::endl;
    std::cout << "sim: Retransmitting seq = " << seq << ", size = " << sz << " = " << sz/ 1500 << "p" << std::endl;

}


/**
 * ??? ACK ????????????, ?????? m_tcb->m_nextTxSequence
 */
void SimulationTcpSocket::NewAck (SequenceNumber32 const& ack, bool resetRTO)
{
    m_dataRetrCount = m_dataRetries;

    if (m_state != TcpSocket::TcpStates_t::SYN_RCVD && resetRTO)
    { 
        m_retxEvent.Cancel ();
        m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);
        m_retxEvent = Simulator::Schedule (m_rto, &SimulationTcpSocket::ReTxTimeout, this);
    }

    if (ack > m_tcb->m_nextTxSequence)
    {
        m_tcb->m_nextTxSequence = ack; // If advanced
    }

}


/**
 * SendDataPacket ???????????????
 * ??? SimulationTcpSocket ??? receiver ???????????? packet ????????? SimulationTcpSocket ???
 * Put into SimulationTcpSocket's txBuffer
 */
uint32_t SimulationTcpSocket::FillDataPacket (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
    // bool isStartOfTransmission = BytesInFlight () == 0U;
    
    /**
     * ?????? TxBuffer ?????????????????????????????????
     * ?????? seq == m_highTxMark ????????????????????????????????? packet, ???????????? txBuffer
     */
    uint32_t sz = packet->GetSize ();                           // ?????? packet ??????
    SequenceNumber32 seq = tcpHeader.GetSequenceNumber();       // ?????? seq ????????????
    SequenceNumber32 tail = seq + sz;

    // std::cout << "seq: " << seq.GetValue () << ", tail: " << tail.GetValue() << std::endl;

    Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (tcpHeader.GetOption (TcpOption::TS));
    m_dataEchoTimestamp = ts->GetEcho ();
    if (m_dataEchoTimestamp == m_firstTime)
    {
        if (m_packet == nullptr)
        {
            m_packet = packet->Copy();
        }
        m_initCwnd += 1;
        m_tcb->m_cWnd += GetSegSize();

        SendPendingData (true);

    }


    // ?????????, ????????????
    if (seq < m_txBuffer->m_firstByteSeq)
    {}
    // ???????????????????????????
    else if (seq >= m_txBuffer->m_firstByteSeq && tail <= m_txBuffer->m_firstByteSeq + m_txBuffer->m_size)
    {
        bool listEdited;
        FillPacket (seq, sz, packet, listEdited);
    }
    else
    {
        bool flag = m_txBuffer->Add(m_packet);
        if (!flag)
        {
            std::cout << "sentlist: from " << m_txBuffer->m_firstByteSeq << " to " <<  m_txBuffer->m_firstByteSeq + m_txBuffer->m_sentSize << std::endl;
            std::cout << "applist: from " << m_txBuffer->m_firstByteSeq +  m_txBuffer->m_sentSize<< " to " <<  m_txBuffer->m_firstByteSeq + m_txBuffer->m_size << std::endl;
            std::cout << "max size: " << m_txBuffer->Available () << ", size: " << m_txBuffer->m_size << std::endl;
            std::cout << "Rejected. Not enough room to buffer packet." << std::endl;
        }

        return FillDataPacket (packet, tcpHeader);  // ????????????
    }
    
    // bool isRetransmission = outItem->IsRetrans ();        // ????????????????????????

    // UpdateRttHistory (seq, sz, isRetransmission);


    return sz;
}




void SimulationTcpSocket::DupAck (uint32_t currentDelivered)
{

    if (m_tcb->m_congState == TcpSocketState::CA_LOSS)
    {
    }

    if (m_tcb->m_congState != TcpSocketState::CA_RECOVERY)
    {
        ++m_dupAckCount;
    }

    if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
    {
        m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_DISORDER);
        m_tcb->m_congState = TcpSocketState::CA_DISORDER;
    }

    if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
    {
        if (!m_sackEnabled)
        {
            m_txBuffer->AddRenoSack ();
        }
        if (!m_congestionControl->HasCongControl ())
        {
            m_recoveryOps->DoRecovery (m_tcb, currentDelivered);
        }
    }
    else if (m_tcb->m_congState == TcpSocketState::CA_DISORDER)
    {
        if ((m_dupAckCount == m_retxThresh) && ((m_highRxAckMark >= m_recover) || (!m_recoverActive)))
        {
            EnterRecovery (currentDelivered);
        }
        else if (m_txBuffer->IsLost (m_highRxAckMark + m_tcb->m_segmentSize))
        {
            EnterRecovery (currentDelivered);
        }
        else
        {

            if (!m_sackEnabled && m_limitedTx)
            {
                m_txBuffer->AddRenoSack ();
            }
        }
    }
}


void SimulationTcpSocket::EnterRecovery (uint32_t currentDelivered)
{
    if (!m_sackEnabled)
    {
        m_txBuffer->AddRenoSack ();
        m_txBuffer->MarkHeadAsLost ();
    }
    else
    {
        if (!m_txBuffer->IsLost (m_txBuffer->HeadSequence ()))
        {
            m_txBuffer->MarkHeadAsLost ();
        }
    }
    
    m_recover = m_tcb->m_highTxMark;
    m_recoverActive = true;

    if (m_recover.GetValue () == 5781001)
    {
        std::cout << "stop" << std::endl;
    }

    m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_RECOVERY);
    m_tcb->m_congState = TcpSocketState::CA_RECOVERY;

    uint32_t bytesInFlight = m_sackEnabled ? BytesInFlight () : BytesInFlight () + m_tcb->m_segmentSize;

    m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb, bytesInFlight);

    if (!m_congestionControl->HasCongControl ())
    {
      m_recoveryOps->EnterRecovery (m_tcb, m_dupAckCount, m_tcb->m_highTxMark - m_txBuffer->HeadSequence (), currentDelivered);
    }

    // std::cout << "cwnd = " << m_tcb->m_cWnd / m_tcb->m_segmentSize << " -> GetSsThresh (EnterRecovery)" << std::endl;
    m_recoverTimestamp = m_tcb->m_rcvTimestampValue;

    DoRetransmit ();
}


void SimulationTcpSocket::UpdateCwnd (uint32_t oldValue, uint32_t newValue)
{
    m_cWndTrace (oldValue, newValue);
}


uint32_t SimulationTcpSocket::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{
    // bool isStartOfTransmission = BytesInFlight () == 0U;
    TcpTxItem *outItem = m_txBuffer->CopyFromSequence (maxSize, seq);       // m_sendsize += sz;

    // m_rateOps->SkbSent(outItem, isStartOfTransmission&&(m_tcb->m_highTxMark==m_tcb->m_nextTxSequence));

    // bool isRetransmission = outItem->IsRetrans ();        // ????????????????????????
    Ptr<Packet> p = outItem->GetPacketCopy ();            // ??? TcpTxItem ???????????? packet
    uint32_t sz = p->GetSize (); // Size of packet        // ?????? packet ??????
    uint8_t flags = withAck ? TcpHeader::ACK : 0;         // ?????? flag, ????????? ACK
    // uint32_t remainingData = m_txBuffer->SizeFromSequence (seq + SequenceNumber32 (sz));  // ???????????? packet ???, ????????????????????????

    // std::cout << "send: seq = " << seq << ", end = " << seq + sz << ", size = " << sz << std::endl;
    TcpHeader header;
    header.SetFlags (flags);
    header.SetSequenceNumber (seq);
    header.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
  
    // if (withAck)
    // {
    //     m_delAckEvent.Cancel ();
    //     m_delAckCount = 0;
    // }

    if (m_retxEvent.IsExpired ())
    {
        m_retxEvent = Simulator::Schedule (m_rto, &SimulationTcpSocket::ReTxTimeout, this);
    }

    m_txTrace (p, header, (TcpSocketBase*)this);

    // UpdateRttHistory (seq, sz, isRetransmission);

    if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
    {
        m_recoveryOps->UpdateBytesSent (sz);
    }

    // Update highTxMark
    m_tcb->m_highTxMark = std::max (seq + sz, m_tcb->m_highTxMark.Get ());
    if (m_tcb->m_highTxMark.Get().GetValue () % 10 != 1)
    {
        std::cout << "here" << std::endl;
    }

    return sz;
}


/**
 * ????????????????????? packet ??????
 */
uint32_t SimulationTcpSocket::SendPendingData (bool withAck)
{
  /**
   * m_txBuffer->m_size ???????????? appList + sentList ??????
   * ???????????????????????? appList ?????????????????????, sentList ??????????????????????????????
   * ?????? receiver ????????????, ?????? SimulationTcpSocket ????????? sender ?????????, ???????????????
   */
//   if (m_txBuffer->Size () == 0)
//     {
//       return false;                           // Nothing to send
//     }


    uint32_t nPacketsSent = 0;
    uint32_t availableWindow = AvailableWindow ();

    while (availableWindow > 0)
    {
        SequenceNumber32 next;
        SequenceNumber32 nextHigh;
        bool enableRule3 = m_sackEnabled && m_tcb->m_congState == TcpSocketState::CA_RECOVERY;
        
        uint32_t leftspace = m_txBuffer->Available ();
        if (leftspace < GetSegSize ())
        {
            Ptr<Packet> p = m_packet->CreateFragment (0, leftspace);
            m_txBuffer->Add(p);
        }
        else
        {
            m_txBuffer->Add(m_packet);
        }
        
        if (!m_txBuffer->NextSeg (&next, &nextHigh, enableRule3))     // NextSeg ?????? false, ??????
        {
            break;
        }
        else
        {
            uint32_t availableData = m_txBuffer->SizeFromSequence (next);       // ?????? next ?????????????????????

            if (availableWindow < m_tcb->m_segmentSize && availableData > availableWindow)
            {
                break; // No more
            }
            if (!false && UnAckDataCount () > 0 && availableData < m_tcb->m_segmentSize)
            {
                break;
            }

            uint32_t s = std::min (availableWindow, m_tcb->m_segmentSize);
            uint32_t maxSizeToSend = static_cast<uint32_t> (nextHigh - next);
            s = std::min (s, maxSizeToSend);

            if (m_tcb->m_nextTxSequence != next)
            {
                m_tcb->m_nextTxSequence = next;           /* ??????????????????????????? seq, ??????????????? next (??????) */
            }
            if (m_tcb->m_bytesInFlight.Get () == 0)       /* ????????? inflight ????????? */
            { 
                m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_TX_START);
            }
            uint32_t sz = SendDataPacket (m_tcb->m_nextTxSequence, s, withAck);   // SendDataPacket
            m_tcb->m_nextTxSequence += sz;    // sz ??? SendDataPacket() ????????????????????????, ??????????????????????????? (????????????, ???????????????????????? next)
            ++nPacketsSent;                   // ????????? packet ?????? +1, ????????????????????????????????? MSS ????????????
        }

      availableWindow = AvailableWindow ();   // ?????? cwnd - pipe

    } // end of while loop!

    // if (nPacketsSent > 0)
    // {
    //     if (!m_sackEnabled)
    //     {
    //         if (!m_limitedTx)
    //         {
    //           NS_ASSERT (m_tcb->m_congState != TcpSocketState::CA_DISORDER);
    //         }
    //     }

    return nPacketsSent;
}


uint32_t SimulationTcpSocket::AvailableWindow () const
{
  uint32_t win = Window ();             // Number of bytes allowed to be outstanding
  uint32_t inflight = BytesInFlight (); // Number of outstanding bytes

//   std::cout << "wind = " << win / GetSegSize () << ", inflight = " << inflight / GetSegSize () << std::endl;
  return (inflight > win) ? 0 : win - inflight;
}


uint32_t SimulationTcpSocket::Window (void) const
{
  return std::min (m_rWnd.Get (), m_tcb->m_cWnd.Get ());
}


uint32_t SimulationTcpSocket::UnAckDataCount () const
{
  return m_tcb->m_highTxMark - m_txBuffer->HeadSequence ();
}


TcpTxItem* SimulationTcpSocket::FillPacket (SequenceNumber32 seq, uint32_t size, Ptr<Packet> packet, bool& listEdited)
{
    TcpTxItem* item;
    packet = packet == nullptr ? m_packet : packet->Copy();

    SequenceNumber32 tail = seq + size;
    // ????????????
    if (m_txBuffer->m_firstByteSeq > seq)
    {
        return nullptr;
    }
    // ?????? sentList
    else if (m_txBuffer->m_firstByteSeq + m_txBuffer->m_sentSize >= tail)
    {
        item = m_txBuffer->GetPacketFromList (m_txBuffer->m_sentList, m_txBuffer->m_firstByteSeq, size, seq, &listEdited);
    }
    // ?????? appList
    else if (m_txBuffer->m_firstByteSeq + m_txBuffer->m_size > tail)
    {
        item = m_txBuffer->GetPacketFromList (m_txBuffer->m_appList, m_txBuffer->m_firstByteSeq + m_txBuffer->m_sentSize, size, seq, &listEdited);
    }

    else if (m_txBuffer->m_firstByteSeq + m_txBuffer->m_size < tail)
    {
        item = new TcpTxItem ();
        item->m_startSeq = seq;
        item->m_lost = false;
        item->m_retrans = false;
        
        m_txBuffer->m_sentList.insert (m_txBuffer->m_sentList.end (), item);
        m_txBuffer->m_sentSize += size;
        m_txBuffer->m_size += size;           // ??? DiscardUpTo ????????????, ????????? 0 ???????????????
    }
    else
    {
        return nullptr;
    }

    item->m_packet = packet;
    m_tcb->m_highTxMark = std::max (seq + size, m_tcb->m_highTxMark.Get ());

    // ???????????? Recovery ??????, m_recover ????????????

    // if (seq.GetValue () == 5787001)
    // {
    //     std::cout << "here" << std::endl;
    // }

    if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY && m_dataEchoTimestamp <= m_recoverTimestamp && seq + size > m_recover)
    {
        m_recover = std::max (seq + size, m_recover);
    }
    return item;
}

void SimulationTcpSocket::ReTxTimeout ()
{
    if (m_state == TcpSocket::TcpStates_t::CLOSED || m_state == TcpSocket::TcpStates_t::TIME_WAIT || m_state == TcpSocket::TcpStates_t::SYN_SENT || m_txBuffer->Size () == 0)
    {
        return;
    }

    if (m_state <= TcpSocket::TcpStates_t::ESTABLISHED && m_txBuffer->HeadSequence () >= m_tcb->m_highTxMark && m_txBuffer->Size () == 0)
    {
        return;
    }

    if (m_dataRetrCount == 0)
    {
        return;
    }
    else
    {
        --m_dataRetrCount;
    }

    uint32_t inFlightBeforeRto = BytesInFlight ();
    bool resetSack = !m_sackEnabled;

    m_dupAckCount = 0;
    if (!m_sackEnabled)
    {
        m_txBuffer->ResetRenoSack ();
    }

    m_txBuffer->SetSentListLost (resetSack);

    m_recover = m_tcb->m_highTxMark;
    m_recoverActive = true;

    Time doubledRto = m_rto + m_rto;
    m_rto = Min (doubledRto, Time::FromDouble (60,  Time::S));

    if (m_tcb->m_congState != TcpSocketState::CA_LOSS || !m_txBuffer->IsHeadRetransmitted ())
    {
        m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb, inFlightBeforeRto);
    }

    m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_LOSS);
    m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_LOSS);
    m_tcb->m_congState = TcpSocketState::CA_LOSS;
    m_tcb->m_cWnd = m_tcb->m_segmentSize;
    m_tcb->m_cWndInfl = m_tcb->m_cWnd;

    std::cout << "Loss Event occur at time: " << Simulator::Now ().GetSeconds () << std::endl; 

    SendPendingData (m_connected);
}


void
SimulationTcpSocket::UpdateCongState (TcpSocketState::TcpCongState_t oldValue,
                                TcpSocketState::TcpCongState_t newValue)
{
  m_congStateTrace (oldValue, newValue);
}

}