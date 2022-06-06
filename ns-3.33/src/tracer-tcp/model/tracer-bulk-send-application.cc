/*
 * @Author: Zhang Bochun
 * @Date: 2022-03-12 22:59:30
 * @LastEditTime: 2022-04-25 12:46:55
 * @LastEditors: Zhang Bochun
 * @Description: tracer-bulk-send-application.cc
 * @FilePath: /ns-3.33/src/tracer-tcp/model/tracer-bulk-send-application.cc
 */

#include "tracer-bulk-send-application.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TracerBulkSendApplication");

NS_OBJECT_ENSURE_REGISTERED (TracerBulkSendApplication);


TypeId TracerBulkSendApplication::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TracerBulkSendApplication")
    .SetParent<Application> ()
    .SetGroupName("TracerTcp") 
    .AddConstructor<TracerBulkSendApplication> ()
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&TracerBulkSendApplication::m_sendSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&TracerBulkSendApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("Local",
                   "The Address on which to bind the socket. If not set, it is generated automatically.",
                   AddressValue (),
                   MakeAddressAccessor (&TracerBulkSendApplication::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. "
                   "Once these bytes are sent, "
                   "no data  is sent again. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TracerBulkSendApplication::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&TracerBulkSendApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("EnableSeqTsSizeHeader",
                   "Add SeqTsSizeHeader to each packet",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TracerBulkSendApplication::m_enableSeqTsSizeHeader),
                   MakeBooleanChecker ())

    .AddAttribute ("Foldername",
                   "Folder to store Trace information",
                   StringValue (std::string("default")),
                   MakeStringAccessor (&TracerBulkSendApplication::m_foldername),
                   MakeStringChecker ())
    .AddAttribute ("CongestionAlgo",
                   "Congestion Algorithm",
                   StringValue (std::string ("cubic")),
                   MakeStringAccessor (&TracerBulkSendApplication::m_ccalgoname),
                   MakeStringChecker ())

    .AddAttribute ("Traceflag",
                   "Trace flag",
                   UintegerValue (Tracer::E_TRACE_SEND_ALL),
                   MakeUintegerAccessor (&TracerBulkSendApplication::m_traceflag),
                   MakeUintegerChecker<uint16_t> ())
             
    .AddTraceSource ("Tx", "A new packet is sent",
                     MakeTraceSourceAccessor (&TracerBulkSendApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxWithSeqTsSize", "A new packet is created with SeqTsSizeHeader",
                     MakeTraceSourceAccessor (&TracerBulkSendApplication::m_txTraceWithSeqTsSize),
                     "ns3::PacketSink::SeqTsSizeCallback")
    ;
    return tid;
}


TracerBulkSendApplication::TracerBulkSendApplication ()
    : m_socket (0),
      m_connected (false),
      m_totBytes (0),
      m_unsentPacket (0)
{
    NS_LOG_FUNCTION (this);
}


TracerBulkSendApplication::~TracerBulkSendApplication ()
{
    NS_LOG_FUNCTION (this);
}


void TracerBulkSendApplication::SetMaxBytes (uint64_t maxBytes)
{
    NS_LOG_FUNCTION (this << maxBytes);
    m_maxBytes = maxBytes;
}


Ptr<Socket> TracerBulkSendApplication::GetSocket (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socket;
}


void TracerBulkSendApplication::DoDispose (void)
{
    NS_LOG_FUNCTION (this);

    m_socket = 0;
    m_unsentPacket = 0;

    // chain up
    Application::DoDispose ();    /* 清除资源占用 */
}


void TracerBulkSendApplication::StartApplication (void)
{
    NS_LOG_FUNCTION (this);
    Address from;

    // Create the socket if not already
    if (!m_socket)
    { 
        /* 尚未 create socket, m_tid: The type of protocol to use, defalut: TcpSocketFactory */
        m_socket = Socket::CreateSocket (GetNode (), m_tid);

        std::cout << "Send: factory -> " << m_tid << std::endl;

        int ret = -1;

        // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
        if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
            m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
            NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                            "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                            "In other words, use TCP instead of UDP.");
        }


        if (! m_local.IsInvalid())
        {
            NS_ABORT_MSG_IF ((Inet6SocketAddress::IsMatchingType (m_peer) && InetSocketAddress::IsMatchingType (m_local)) ||
                            (InetSocketAddress::IsMatchingType (m_peer) && Inet6SocketAddress::IsMatchingType (m_local)),
                            "Incompatible peer and local address IP version");
            /* 由 TcpL4Protocol 分配一个本地 Endpoint 并交付给 TcpSocketBase */
            ret = m_socket->Bind (m_local);
        }
        else
        {
            if (Inet6SocketAddress::IsMatchingType (m_peer))
            {
                ret = m_socket->Bind6 ();
            }
            else if (InetSocketAddress::IsMatchingType (m_peer))
            {
                ret = m_socket->Bind ();
            }
        }

        if (ret == -1)
        {
            NS_FATAL_ERROR ("Failed to bind socket");
        }



        m_socket->Connect (m_peer);     /* 原语操作, 绑定对端 */
        m_socket->ShutdownRecv ();      /* 关闭接收功能 */
        m_socket->SetConnectCallback (  /* 绑定连接成功, 失败时触发的函数 */
            MakeCallback (&TracerBulkSendApplication::ConnectionSucceeded, this),
            MakeCallback (&TracerBulkSendApplication::ConnectionFailed,    this));
        m_socket->SetSendCallback (     /* 通知 Buffer 空闲, 可以调用 Send() 进行原语发送 */
            MakeCallback (&TracerBulkSendApplication::DataSend, this));

        SetCongestionAlgo();
        RegisterTraceFunctions ();

    }
    if (m_connected)                    /* 连接建立成功 */
    {
        m_socket->GetSockName (from);   /* 从 socket 绑定的 Endpoint 获取本地 IP Address */
        SendData (from, m_peer);        /* 从 from 向 peer 发送分组, 直到 L4 层已满 */
    }

}



void TracerBulkSendApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();             /* 执行 Socket 原语关闭连接 */
      m_connected = false;            /* 标记 connected 状态为 false */
    }
  else
    {
      NS_LOG_WARN ("BulkSendApplication found null socket to close in StopApplication");
    }
}


// Private helpers

void TracerBulkSendApplication::SendData (const Address &from, const Address &to)
{
    NS_LOG_FUNCTION (this);

    while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { 
        // Time to send more
        // uint64_t to allow the comparison later.
        // the result is in a uint32_t range anyway, because
        // m_sendSize is uint32_t.
        uint64_t toSend = m_sendSize;
      
        // Make sure we don't send too many
        if (m_maxBytes > 0)
        {
            toSend = std::min (toSend, m_maxBytes - m_totBytes);
        }

        NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());

        Ptr<Packet> packet;
        if (m_unsentPacket)
        {
          packet = m_unsentPacket;
          toSend = packet->GetSize ();
        }
        else if (m_enableSeqTsSizeHeader)
        {
            SeqTsSizeHeader header;
            header.SetSeq (m_seq++);
            header.SetSize (toSend);
            NS_ABORT_IF (toSend < header.GetSerializedSize ());
            packet = Create<Packet> (toSend - header.GetSerializedSize ());
            // Trace before adding header, for consistency with PacketSink
            m_txTraceWithSeqTsSize (packet, from, to, header);
            packet->AddHeader (header);
        }
        else
        {
            packet = Create<Packet> (toSend);
        }

        int actual = m_socket->Send (packet);
        if ((unsigned) actual == toSend)
        {
            m_totBytes += actual;
            m_txTrace (packet);
            m_unsentPacket = 0;
        }
        else if (actual == -1)
        {
            // We exit this loop when actual < toSend as the send side
            // buffer is full. The "DataSent" callback will pop when
            // some buffer space has freed up.
            NS_LOG_DEBUG ("Unable to send packet; caching for later attempt");
            m_unsentPacket = packet;
            break;
        }
        else if (actual > 0 && (unsigned) actual < toSend)
        {
            // A Linux socket (non-blocking, such as in DCE) may return
            // a quantity less than the packet size.  Split the packet
            // into two, trace the sent packet, save the unsent packet
            NS_LOG_DEBUG ("Packet size: " << packet->GetSize () << "; sent: " << actual << "; fragment saved: " << toSend - (unsigned) actual);
            Ptr<Packet> sent = packet->CreateFragment (0, actual);
            Ptr<Packet> unsent = packet->CreateFragment (actual, (toSend - (unsigned) actual));
            m_totBytes += actual;
            m_txTrace (sent);
            m_unsentPacket = unsent;
            break;
        }
        else
        {
            NS_FATAL_ERROR ("Unexpected return value from m_socket->Send ()");
        }
    }
    // Check if time to close (all sent)
    if (m_totBytes == m_maxBytes && m_connected)
    {
        m_socket->Close ();
        m_connected = false;
    }
}


void TracerBulkSendApplication::ConnectionSucceeded (Ptr<Socket> socket)
{   
    /* 连接建立成功时调用 - socket 的回调函数 */
    /* TcpSocketBase 处理 SYN 建立连接成功时调用该回调函数 */
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_LOGIC ("BulkSendApplication Connection succeeded");
    m_connected = true;                 /* 设置 connected 成功 */
    Address from, to;                   /* 从 endpoint 中读取 from 和 to 地址 */
    socket->GetSockName (from);
    socket->GetPeerName (to);
    SendData (from, to);                /* 连接成功, 发送数据 */
}


void TracerBulkSendApplication::ConnectionFailed (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_LOGIC ("BulkSendApplication, Connection Failed");

    /* 不设置 m_connected = false; ? 可能之前就保持为 false */
}


void TracerBulkSendApplication::DataSend (Ptr<Socket> socket, uint32_t)
{
    NS_LOG_FUNCTION (this);     /* Buffer 空闲时, 调用该函数进行原语发送 */

    if (m_connected)
    { // Only send new data if the connection has completed
        Address from, to;
        socket->GetSockName (from);
        socket->GetPeerName (to);
        SendData (from, to);          /* 如果连接建立, 发送数据 */
    }
}


void TracerBulkSendApplication::SetCongestionAlgo (void)
{
    bool pacing;
    TypeId id = TracerUtils::GetCongestionTypeIdFromName (m_ccalgoname, pacing);
    
    ObjectFactory factory;
    factory.SetTypeId (id);
    Ptr<TcpCongestionOps> ops = factory.Create<TcpCongestionOps> ();
    
    Ptr<TcpSocketBase> base = DynamicCast<TcpSocketBase>(m_socket);
    base->SetPacingStatus (pacing);
    base->SetCongestionControlAlgorithm (ops);
}


void TracerBulkSendApplication::RegisterTraceFunctions (void)
{
    /* 注册 - 根据 m_traceflag 进行跟踪 */
    m_tracer = CreateObject<Tracer> (m_socket, m_traceflag, m_foldername);
    m_tracer->RegisterTraceFunctions ();
}


}