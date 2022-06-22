/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-05 20:31:40
 * @LastEditTime: 2022-06-20 17:49:56
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/model/tracer.cc
 */


#include "tracer.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Tracer");

NS_OBJECT_ENSURE_REGISTERED (Tracer);


TypeId Tracer::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Tracer")
        .SetParent<Object> ()
        .SetGroupName("TracerTcp")
        .AddConstructor<Tracer> ()
    ;
    return tid;
}


Tracer::Tracer (Ptr<Socket> socket, uint16_t flag, std::string& foldername)
{
    m_socket = socket;
    m_traceFlag = flag;
    m_foldername = foldername;

    m_minTxRTT = Time("20ms");
    m_minRxRTT = Time("20ms");

    m_socket->GetSockName (m_local);
    m_socket->GetPeerName (m_peer );
}


/* 注册 Trace 函数 */
void Tracer::RegisterTraceFunctions ()
{
    if (m_socket == nullptr )
    {
        return;
    }
    
    std::string delimiter="_";
    InetSocketAddress  local_sock_addr = InetSocketAddress::ConvertFrom (m_local);
    InetSocketAddress remote_sock_addr = InetSocketAddress::ConvertFrom (m_peer );

    uint32_t ip1 =  local_sock_addr.GetIpv4 ().Get ();
    uint32_t ip2 = remote_sock_addr.GetIpv4 ().Get ();
    uint16_t port1 =  local_sock_addr.GetPort ();
    uint16_t port2 = remote_sock_addr.GetPort ();

    std::string ip1_str = TracerUtils::StandardIpFormatString (ip1);
    std::string ip2_str = TracerUtils::StandardIpFormatString (ip2);
    std::string port1_str = std::to_string (port1);
    std::string port2_str = std::to_string (port2);


    char buf[FILENAME_MAX];
    std::string path = std::string (getcwd (buf, FILENAME_MAX))+ "/traces/";        /* 绝对路径 */
    path = path + m_foldername;
    if (!TracerUtils::IsDirExist (path))
    {
        TracerUtils::MakePath (path);
    }

    /* 绝对路径/trace/foldername/filename(ip+port)_suffix  */
    m_send_filename = path + "/" + ip1_str + delimiter + port1_str + delimiter + "to" + delimiter
                             + ip2_str + delimiter + port2_str + "_at_sender";
    m_rece_filename = path + "/" + ip2_str + delimiter + port2_str + delimiter + "to" + delimiter
                             + ip1_str + delimiter + port1_str + "_at_receiver";
                             
    UintegerValue mss;
    m_socket->GetAttribute("SegmentSize", mss);
    m_mss = mss.Get();

    if (m_traceFlag & E_TRACE_SEND_CWND)
    {
        this->OpenSendCwndTraceFile (m_send_filename);
        m_socket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&Tracer::TraceSendCwndCallback, this));
    }

    if (m_traceFlag & E_TRACE_SEND_RTT)
    {
        this->OpenSendRttTraceFile (m_send_filename);
        m_socket->TraceConnectWithoutContext ("RTT", MakeCallback (&Tracer::TraceSendRttCallback, this));
        // m_socket->TraceConnectWithoutContext ("LocalUpRTT", MakeCallback (&Tracer::TraceSendUpRttCallback, this));
        // m_socket->TraceConnectWithoutContext ("LocalDownRTT", MakeCallback (&Tracer::TraceSendDownRttCallback, this));
    }

    if (m_traceFlag & E_TRACE_SEND_INFLIGHT)
    {
        this->OpenSendInflightTraceFile (m_send_filename);
        m_socket->TraceConnectWithoutContext ("BytesInFlight", MakeCallback (&Tracer::TraceSendInflightCallback, this));
    }

    if (m_traceFlag & E_TRACE_SEND_DELIVERED)
    {
        this->OpenSendDeliveredTraceFile (m_send_filename);
        m_socket->TraceConnectWithoutContext ("TcpRateSampleUpdated", MakeCallback (&Tracer::TraceSendDeliveredCallback, this));
    }
    
    if (m_traceFlag & E_TRACE_SEND_SENDRATE)
    {
        this->OpenSendRateTraceFile  (m_send_filename);
        this->OpenSendTxRxTracerFile (m_send_filename);
        m_socket->TraceConnectWithoutContext ("Tx", MakeCallback (&Tracer::TraceSendPacketTxCallback, this));
        m_socket->TraceConnectWithoutContext ("Rx", MakeCallback (&Tracer::TraceSendPacketRxCallback, this));
    }

    if (m_traceFlag & E_TRACE_SEND_ALLRTT)
    {
        this->OpenSendAllRttTraceFile  (m_send_filename);
        m_socket->TraceConnectWithoutContext ("AllRTT", MakeCallback (&Tracer::TraceSendAllRttCallback, this));
    }

    if (m_traceFlag & E_TRACE_SEND_CONGSTATE)
    {
        this->OpenSendCongStateTraceFile (m_send_filename);
        m_socket->TraceConnectWithoutContext ("CongState", MakeCallback (&Tracer::TraceSendCongStateCallback, this));
    }


    if (m_traceFlag & E_TRACE_RECE_CWND)
    {
        this->OpenReceCwndTraceFile (m_rece_filename);
        m_socket->TraceConnectWithoutContext ("SendCwnd", MakeCallback (&Tracer::TraceReceCwndCallback, this));
    }

    if (m_traceFlag & E_TRACE_RECE_RTT)
    {
        this->OpenReceRttTraceFile (m_rece_filename);
        m_socket->TraceConnectWithoutContext ("ReceRTT", MakeCallback (&Tracer::TraceReceRttCallback, this));
        // m_socket->TraceConnectWithoutContext ("PeerUpRTT", MakeCallback (&Tracer::TraceReceUpRttCallback, this));
        // m_socket->TraceConnectWithoutContext ("PeerDownRTT", MakeCallback (&Tracer::TraceReceDownRttCallback, this));
    }

    if (m_traceFlag & E_TRACE_RECE_RECERATE)
    {
        this->OpenReceRateTraceFile  (m_rece_filename);
        this->OpenReceTxRxTracerFile (m_rece_filename);
        m_socket->TraceConnectWithoutContext ("Tx", MakeCallback (&Tracer::TraceRecePacketTxCallback, this));
        m_socket->TraceConnectWithoutContext ("Rx", MakeCallback (&Tracer::TraceRecePacketRxCallback, this));
    }

    if (m_traceFlag & E_TRACE_RECE_CONGSTATE)
    {
        this->OpenReceCongStateTraceFile (m_rece_filename);
        m_socket->TraceConnectWithoutContext ("SendCongState", MakeCallback (&Tracer::TraceReceCongStateCallback, this));
    }

    if (m_traceFlag & E_TRACE_RECE_SIMTX)
    {
        this->OpenReceSimPacketTxCallback (m_rece_filename);
        m_socket->TraceConnectWithoutContext ("SimTx", MakeCallback (&Tracer::TraceReceSimPacketRxCallback, this));
    }

}



void Tracer::DoDispose (void)
{
    NS_LOG_FUNCTION (this);

    while (! m_filelist.empty ())
    {
        auto& file = m_filelist.front ();
        if (!file->is_open ())
        {
            file->close ();
        }
        m_filelist.pop ();
    }
    Object::DoDispose ();
}


// Trace helpers
void Tracer::OpenSendCwndTraceFile (std::string filename)
{
    if (!m_send_cwnd.is_open ())
    {
        filename += "_cwnd.txt";
        m_send_cwnd.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_send_cwnd);
    }
}


void Tracer::OpenSendRttTraceFile (std::string filename)
{
    if (!m_send_rtt.is_open ())
    {
        m_send_rtt.open ((filename + "_rtt.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_send_rtt);
    }
    // if (!m_send_rtt_up.is_open ())
    // {
    //     m_send_rtt_up.open ((filename + "_rtt_up.txt").c_str (), std::fstream::out);
    //     m_filelist.push (&m_send_rtt_up);
    // }
    // if (!m_send_rtt_down.is_open ())
    // {
    //     m_send_rtt_down.open ((filename + "_rtt_down.txt").c_str (), std::fstream::out);
    //     m_filelist.push (&m_send_rtt_down);
    // }
}


void Tracer::OpenSendInflightTraceFile (std::string filename)
{
    if (!m_send_inflight.is_open ())
    {
        filename += "_inflight.txt";
        m_send_inflight.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_send_inflight);
    }
}


void Tracer::OpenSendDeliveredTraceFile (std::string filename)
{
    if (!m_send_delivered.is_open ())
    {
        filename += "_delivered.txt";
        m_send_delivered.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_send_delivered);
    }
}


void Tracer::OpenSendRateTraceFile (std::string filename)
{
    if (!m_send_sendrate.is_open ())
    {
        filename += "_sendrate.txt";
        m_send_sendrate.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_send_sendrate);
    }
}


void Tracer::OpenSendTxRxTracerFile (std::string filename)
{
    if (!m_send_tx_trace.is_open ())
    {
        m_send_tx_trace.open((filename + "_Tx.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_send_tx_trace);
    }
    if (!m_send_rx_trace.is_open ())
    {
        m_send_rx_trace.open((filename + "_Rx.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_send_rx_trace);
    }
}


void Tracer::OpenSendAllRttTraceFile (std::string filename)
{
    if (!m_send_all_rtt.is_open ())
    {
        m_send_all_rtt.open ((filename + "_allrtt.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_send_all_rtt);
    }
}


void Tracer::OpenSendCongStateTraceFile (std::string filename)
{
    if (!m_send_congstate.is_open ())
    {
        m_send_congstate.open ((filename + "_congstate.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_send_congstate);
    }
}



void Tracer::OpenReceCwndTraceFile (std::string filename)
{
    if (!m_rece_cwnd.is_open ())
    {
        filename += "_cwnd.txt";
        m_rece_cwnd.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_rece_cwnd);
    }
}


void Tracer::OpenReceRttTraceFile (std::string filename)
{
    if (!m_rece_rtt.is_open ())
    {
        m_rece_rtt.open ((filename + "_rtt.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_rece_rtt);
    }
    // if (!m_rece_rtt_up.is_open ())
    // {
    //     m_rece_rtt_up.open ((filename + "_rtt_up.txt").c_str (), std::fstream::out);
    //     // m_filelist.push_back (m_rece_rtt_up);
    // }
    // if (!m_rece_rtt_down.is_open ())
    // {
    //     m_rece_rtt_down.open ((filename + "_rtt_down.txt").c_str (), std::fstream::out);
    //     // m_filelist.push_back (m_rece_rtt_down);
    // }
}


void Tracer::OpenReceRateTraceFile (std::string filename)
{
    if (!m_rece_recerate.is_open ())
    {
        filename += "_receiverate.txt";
        m_rece_recerate.open(filename.c_str(), std::fstream::out);
        m_filelist.push (&m_rece_recerate);
    }
}


void Tracer::OpenReceTxRxTracerFile (std::string filename)
{
    if (!m_rece_rx_trace.is_open ())
    {
        m_rece_rx_trace.open((filename + "_Rx.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_rece_rx_trace);
    }
    if (!m_rece_tx_trace.is_open ())
    {
        m_rece_tx_trace.open((filename + "_Tx.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_rece_tx_trace);
    }
}

void Tracer::OpenReceCongStateTraceFile (std::string filename)
{
    if (!m_rece_congstate.is_open ())
    {
        m_rece_congstate.open ((filename + "_congstate.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_rece_congstate);
    }
}


void Tracer::OpenReceSimPacketTxCallback  (std::string filename)
{
    if (!m_rece_sim_tx_trace.is_open ())
    {
        m_rece_sim_tx_trace.open ((filename + "_simTx.txt").c_str(), std::fstream::out);
        m_filelist.push (&m_rece_sim_tx_trace);
    }
}



void Tracer::TraceSendCwndCallback (uint32_t oldValue, uint32_t newValue)
{
    if (m_send_cwnd.is_open())
    {
        Time now = Simulator::Now ();
        uint32_t packets = newValue / m_mss;
        m_send_cwnd << now.GetSeconds() << "\t" << packets << std::endl;
    }
}


void Tracer::TraceSendRttCallback (Time oldValue, Time newValue)
{
    m_minTxRTT = m_minTxRTT < newValue ? m_minTxRTT : newValue;
    if ((m_traceFlag & E_TRACE_SEND_RTT) && m_send_rtt.is_open())
    {
        Time now = Simulator::Now ();
        m_send_rtt << now.GetSeconds() << "\t" << newValue.GetMicroSeconds() / 1000.0 << std::endl;
    }
}


void Tracer::TraceSendUpRttCallback (Time oldValue, Time newValue)
{
    if ((m_traceFlag & E_TRACE_SEND_RTT) && m_send_rtt_up.is_open())
    {
        Time now = Simulator::Now ();
        m_send_rtt_up << now.GetSeconds() << "\t" << newValue.GetSeconds() << std::endl;
    }
}


void Tracer::TraceSendDownRttCallback (Time oldValue, Time newValue)
{
    if ((m_traceFlag & E_TRACE_SEND_RTT) && m_send_rtt_down.is_open())
    {
        Time now = Simulator::Now ();
        m_send_rtt_down << now.GetSeconds() << "\t" << newValue.GetSeconds() << std::endl;
    }
}

void Tracer::TraceSendAllRttCallback (Time oldValue, Time newValue)
{
    if ((m_traceFlag & E_TRACE_SEND_ALLRTT) && m_send_all_rtt.is_open())
    {
        Time now = Simulator::Now ();
        m_send_all_rtt << now.GetSeconds() << "\t" << newValue.GetSeconds() << std::endl;
    }
}


void Tracer::TraceSendInflightCallback (uint32_t oldValue, uint32_t newValue)
{
    if (m_send_inflight.is_open())
    {
        Time now = Simulator::Now ();
        uint32_t packets = newValue / m_mss;
        m_send_inflight << now.GetSeconds() << "\t" << packets << std::endl;
    }
}


void Tracer::TraceSendDeliveredCallback (const TcpRateOps::TcpRateSample &sample)
{
    if (m_send_delivered.is_open())
    {
        Time now = Simulator::Now ();
        double mbps = sample.m_deliveryRate.GetBitRate () * 1.0 / 1000 / 1000;
        m_send_delivered << now.GetSeconds() << "\t" << mbps << std::endl;
    }
}


void Tracer::TraceSendPacketTxCallback (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base)
{
    if (m_send_tx_trace.is_open ())
    {
        Time now = Simulator::Now ();
        Ptr<const TcpOption> option = header.GetOption (TcpOption::TS);
        Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);


        m_send_tx_trace << now.GetSeconds ()            << "\t" 
            << "Seq: "  << header.GetSequenceNumber()   << "\t" 
            << "ACK: "  << header.GetAckNumber()        << "\t"
            << "size: " << packet->GetSize ()           << "\t"
            << "time: " << ts->GetTimestamp()           << "\t"
            << "echo: " << ts->GetEcho () << std::endl;
    }
    CalculateWithPacket (packet, m_send_sendrate, m_lastTxBytes, m_totalTxBytes, m_lastTxTime, m_minTxRTT);
}


void Tracer::TraceSendCongStateCallback (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue)
{
    if (m_send_congstate.is_open())
    {
        Time now = Simulator::Now ();
        m_send_congstate << now.GetSeconds () << "\t" << MapCongState (newValue) << std::endl;
    }
}


void Tracer::TraceSendPacketRxCallback (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base)
{
    if (m_send_rx_trace.is_open ())
    {
        Time now = Simulator::Now ();
        Ptr<const TcpOption> option = header.GetOption (TcpOption::TS);
        Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);


        m_send_rx_trace << now.GetSeconds () << "\t" 
            << "Seq: " << header.GetSequenceNumber()  << "\t" 
            << "ACK: " << header.GetAckNumber() << "\t" 
            << "time: " << ts->GetTimestamp() << "\t"
            << "echo: " << ts->GetEcho () 
            << ReadOptions (header) << std::endl;
    }
}


void Tracer::TraceReceCwndCallback (uint32_t oldValue, uint32_t newValue)
{
    if (m_rece_cwnd.is_open())
    {
        Time now = Simulator::Now ();
        uint32_t packets = newValue / m_mss;
        m_rece_cwnd << now.GetSeconds() << "\t" << packets << std::endl;
    }
}


void Tracer::TraceReceRttCallback (Time oldValue, Time newValue)
{
    m_minRxRTT = m_minRxRTT < newValue ? m_minRxRTT : newValue;
    if ((m_traceFlag & E_TRACE_RECE_RTT) && m_rece_rtt.is_open())
    {
        Time now = Simulator::Now ();
        m_rece_rtt << now.GetSeconds() << "\t" << newValue.GetMicroSeconds() / 1000.0 << std::endl;
    }
}


void Tracer::TraceReceUpRttCallback (Time oldValue, Time newValue)
{
    if ((m_traceFlag & E_TRACE_RECE_RTT) && m_rece_rtt_up.is_open())
    {
        Time now = Simulator::Now ();
        m_rece_rtt_up << now.GetSeconds() << "\t" << newValue.GetSeconds() << std::endl;
    }
}


void Tracer::TraceReceDownRttCallback (Time oldValue, Time newValue)
{
    if ((m_traceFlag & E_TRACE_RECE_RTT) && m_rece_rtt_down.is_open())
    {
        Time now = Simulator::Now ();
        m_rece_rtt_down << now.GetSeconds() << "\t" << newValue.GetSeconds() << std::endl;
    }
}


void Tracer::TraceRecePacketTxCallback (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base)
{
    if (m_rece_tx_trace.is_open ())
    {
        Time now = Simulator::Now ();
        Ptr<const TcpOption> option = header.GetOption (TcpOption::TS);
        Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);


        m_rece_tx_trace << now.GetSeconds () << "\t" 
            << "Seq: " << header.GetSequenceNumber()  << "\t" 
            << "ACK: " << header.GetAckNumber() << "\t" 
            << "time: " << ts->GetTimestamp() << "\t"
            << "echo: " << ts->GetEcho () 
            << ReadOptions (header) << std::endl;
    }
}


void Tracer::TraceRecePacketRxCallback (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base)
{
    if (m_rece_rx_trace.is_open ())
    {
        Time now = Simulator::Now ();
        Ptr<const TcpOption> option = header.GetOption (TcpOption::TS);
        Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);

        m_rece_rx_trace << now.GetSeconds () << "\t" 
            << "Seq: " << header.GetSequenceNumber()  << "\t" 
            << "ACK: " << header.GetAckNumber() << "\t" 
            << "time: " << ts->GetTimestamp() << "\t"
            << "echo: " << ts->GetEcho () << std::endl;
    }
    CalculateWithPacket (packet, m_rece_recerate, m_lastRxBytes, m_totalRxBytes, m_lastRxTime, m_minRxRTT);
}


void Tracer::TraceReceCongStateCallback (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue)
{
    if (m_rece_congstate.is_open())
    {
        Time now = Simulator::Now ();
        m_rece_congstate << now.GetSeconds () << "\t" <<  MapCongState (newValue) << std::endl;
    }
}



void Tracer::CalculateWithPacket (Ptr<const Packet>& packet, std::fstream& file, uint128_t& lastBytes, uint128_t& totalBytes, Time& time, Time& minRTT)
{
    Time now = Simulator::Now();

    if (file.is_open())
    {
        totalBytes += packet->GetSize();
        if (time.IsZero())
        {
            time = now;
            lastBytes = totalBytes;
        }

        /**
         * 每 3个 RTT 测量一次 sendrate. ... 有问题, m_minRTT 溢出...
         */
        if (now >= time + minRTT * 3)
        {
            double bps = 1.0 * (totalBytes - lastBytes) * 8 / (now - time).GetSeconds();
            double mbps = bps / 1000 / 1000;
            time = now;
            lastBytes = totalBytes;
            file << now.GetSeconds() << "\t" << mbps << std::endl;
        }

    }
}


void Tracer::TraceReceSimPacketRxCallback (Ptr<const Packet> packet, const TcpHeader& header,Ptr<const TcpSocketBase> base)
{
    if (m_rece_sim_tx_trace.is_open ())
    {
        Time now = Simulator::Now ();
        Ptr<const TcpOption> option = header.GetOption (TcpOption::TS);
        Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);
        m_rece_sim_tx_trace << now.GetSeconds () << "\t" 
            << "Seq: " << header.GetSequenceNumber()  << "\t" 
            << "ACK: " << header.GetAckNumber()
            << ReadOptions (header) << std::endl;
    }
}


std::string Tracer::MapCongState (TcpSocketState::TcpCongState_t value)
{
    std::string ret;
    switch (value)
    {
        case TcpSocketState::TcpCongState_t::CA_OPEN:
            ret = "CA_OPEN";
            break;
        case TcpSocketState::TcpCongState_t::CA_DISORDER:
            ret = "CA_DISORDER";
            break;
        case TcpSocketState::TcpCongState_t::CA_CWR:
            ret = "CA_CWR";
            break;
        case TcpSocketState::TcpCongState_t::CA_RECOVERY:
            ret = "CA_RECOVERY";
            break;
        case TcpSocketState::TcpCongState_t::CA_LOSS:
            ret = "CA_LOSS";
            break;
        case TcpSocketState::TcpCongState_t::CA_LAST_STATE:
            ret = "CA_LAST_STATE";
            break;
        default:
            ret = "Default";
            break;
    }
    
    return ret;
}


std::string Tracer::ReadOptions (const TcpHeader &tcpHeader)
{
    TcpHeader::TcpOptionList::const_iterator it;
    const TcpHeader::TcpOptionList options = tcpHeader.GetOptionList ();

    std::string ret = std::string (" ");
    for (it = options.begin (); it != options.end (); ++it)
    {
        const Ptr<const TcpOption> option = (*it);

        switch (option->GetKind ())
        {
            case TcpOption::SACK:
                ret = ProcessOptionSack (option);
                break;
            default:
                continue;
        }
    }
    return ret;
}

std::string Tracer::ProcessOptionSack (const Ptr<const TcpOption> option)
{
    Ptr<const TcpOptionSack> s = DynamicCast<const TcpOptionSack> (option);

    auto list = s->GetSackList ();
    std::string ret = std::string(" -> SACK List: ");

    for (auto option_it = list.begin (); option_it != list.end (); ++option_it)
    {
        ret += std::to_string((*option_it).first.GetValue ());
        ret += std::string (" to ");
        ret += std::to_string((*option_it).second.GetValue ());
        ret += std::string ("; ");
    }

    return ret;
}

}
