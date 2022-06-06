/*
 * @Author: Zhang Bochun
 * @Date: 2022-03-17 16:43:04
 * @LastEditTime: 2022-04-25 12:47:06
 * @LastEditors: Zhang Bochun
 * @Description: Same as PacketSink, add function like tracing attributions.
 *               and as a tcp sender, we trace attributions such as cwnd, sending rate, inflight and RTT.
 * @FilePath: /ns-3.33/src/tracer-tcp/model/tracer-packet-sink.cc
 */


#include "tracer-packet-sink.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TracerPacketSink");

NS_OBJECT_ENSURE_REGISTERED (TracerPacketSink);

TypeId TracerPacketSink::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TracerPacketSink")
        .SetParent<Application> ()
        .SetGroupName("TracerTcp")
        .AddConstructor<TracerPacketSink> ()
        .AddAttribute ("Local",
                       "The Address on which to Bind the rx socket.",
                       AddressValue (),
                       MakeAddressAccessor (&TracerPacketSink::m_local),
                       MakeAddressChecker ())
        .AddAttribute ("Protocol",
                       "The type id of the protocol to use for the rx socket.",
                       TypeIdValue (UdpSocketFactory::GetTypeId ()),
                       MakeTypeIdAccessor (&TracerPacketSink::m_tid),
                       MakeTypeIdChecker ())
        .AddAttribute ("EnableSeqTsSizeHeader",
                       "Enable optional header tracing of SeqTsSizeHeader",
                       BooleanValue (false),
                       MakeBooleanAccessor (&TracerPacketSink::m_enableSeqTsSizeHeader),
                       MakeBooleanChecker ())

        .AddAttribute ("Foldername",
                       "Folder to store Trace information",
                       StringValue (std::string("default")),
                       MakeStringAccessor (&TracerPacketSink::m_foldername),
                       MakeStringChecker ())
        .AddAttribute ("Traceflag",
                       "Trace flag",
                       UintegerValue (Tracer::E_TRACE_RECE_ALL),
                       MakeUintegerAccessor (&TracerPacketSink::m_traceflag),
                       MakeUintegerChecker<uint16_t> ())

        .AddTraceSource ("Rx",
                         "A packet has been received",
                         MakeTraceSourceAccessor (&TracerPacketSink::m_rxTrace),
                         "ns3::Packet::AddressTracedCallback")
        .AddTraceSource ("RxWithAddresses", "A packet has been received",
                         MakeTraceSourceAccessor (&TracerPacketSink::m_rxTraceWithAddresses),
                         "ns3::Packet::TwoAddressTracedCallback")
        .AddTraceSource ("RxWithSeqTsSize",
                         "A packet with SeqTsSize header has been received",
                         MakeTraceSourceAccessor (&TracerPacketSink::m_rxTraceWithSeqTsSize),
                         "ns3::TracerPacketSink::SeqTsSizeCallback")
    ;
    return tid;
}

TracerPacketSink::TracerPacketSink ()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_totalRx = 0;
}

TracerPacketSink::~TracerPacketSink()
{
    NS_LOG_FUNCTION (this);
}

uint64_t TracerPacketSink::GetTotalRx () const
{
    NS_LOG_FUNCTION (this);
    return m_totalRx;
}

Ptr<Socket> TracerPacketSink::GetListeningSocket (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socket;              /* 一个 Socket 监听, 真正接收的是 SocketList 中的 socket */
}

std::list<Ptr<Socket>> TracerPacketSink::GetAcceptedSockets (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socketList;
}

void TracerPacketSink::DoDispose (void)
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_socketList.clear ();
    m_tracerList.clear ();

    // chain up
    Application::DoDispose ();
}


// Application Methods
void TracerPacketSink::StartApplication ()    // Called at time specified by Start
{
    NS_LOG_FUNCTION (this);
    // Create the socket if not already
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket (GetNode (), m_tid);      /* 生成 Listening Socket */

        std::cout << "Rece: factory -> " << m_tid << std::endl;

        if (m_socket->Bind (m_local) == -1)                       /* 绑定本地 Endpoint */
        {
            NS_FATAL_ERROR ("Failed to bind socket");
        }
        m_socket->Listen ();                                      /* 监听 Socket */
        m_socket->ShutdownSend ();                                /* 关闭发送方向的连接 */
        if (addressUtils::IsMulticast (m_local))                  /* multicast - 多播 - 监听多个 IP 的分组 */
        {
            Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);   /* 检查转换是否成功判段是否为 UDP */
            if (udpSocket)
            {
                // equivalent to setsockopt (MCAST_JOIN_GROUP)
                udpSocket->MulticastJoinGroup (0, m_local);
            }
            else
            {
                NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }

    m_socket->SetRecvCallback (MakeCallback (&TracerPacketSink::HandleRead, this));       /* Notify application when new data is available to be read. */
    m_socket->SetAcceptCallback (
        MakeNullCallback<bool, Ptr<Socket>, const Address &> (),    /* connection requests from peer - true to accept, false to reject */
        MakeCallback (&TracerPacketSink::HandleAccept, this));            /* if accept connection requests, callback this function and pass pointer to socket to user */
    m_socket->SetCloseCallbacks (
        MakeCallback (&TracerPacketSink::HandlePeerClose, this),
        MakeCallback (&TracerPacketSink::HandlePeerError, this));
}

void TracerPacketSink::StopApplication ()     // Called at time specified by Stop
{
    NS_LOG_FUNCTION (this);
    while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
        Ptr<Socket> acceptedSocket = m_socketList.front ();
        m_socketList.pop_front ();
        acceptedSocket->Close ();
    }
    
    while(!m_tracerList.empty ()) //these are accepted sockets, close them
    {
        Ptr<Tracer> tracer = m_tracerList.front ();
        m_tracerList.pop_front ();
    }
    
    if (m_socket) 
    {
        m_socket->Close ();
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());    /* 关闭连接 */
    }
}

void TracerPacketSink::HandleRead (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom (from)))            /* 读取 socket 的源 Address 和 一个 Packet */
    {
        if (packet->GetSize () == 0)                      /* 检查是否读完 */
        { //EOF
            break;
        }
        m_totalRx += packet->GetSize ();                  /* 累积到 totalRx */
        if (InetSocketAddress::IsMatchingType (from))     /* Ipv4 */
        {
            NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
        else if (Inet6SocketAddress::IsMatchingType (from))   /* ipv6 */
        {
            NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
        socket->GetSockName (localAddress);
        m_rxTrace (packet, from);                                   /* 调用接收的 callback - 告知源地址 */
        m_rxTraceWithAddresses (packet, from, localAddress);        /* 同上 - 告知 源地址和目的地址 */

        if (m_enableSeqTsSizeHeader)
        {
          PacketReceived (packet, from, localAddress);            /* 提取 Header - 默认 false */
        }
    }
}

void TracerPacketSink::PacketReceived (const Ptr<Packet> &p, const Address &from,
                            const Address &localAddress)
{
    SeqTsSizeHeader header;
    Ptr<Packet> buffer;

    auto itBuffer = m_buffer.find (from);
    if (itBuffer == m_buffer.end ())
    {
        itBuffer = m_buffer.insert (std::make_pair (from, Create<Packet> (0))).first;
    }

    buffer = itBuffer->second;
    buffer->AddAtEnd (p);
    buffer->PeekHeader (header);    /* 提取 Header */

    NS_ABORT_IF (header.GetSize () == 0);

    while (buffer->GetSize () >= header.GetSize ())
    {
        NS_LOG_DEBUG ("Removing packet of size " << header.GetSize () << " from buffer of size " << buffer->GetSize ());
        Ptr<Packet> complete = buffer->CreateFragment (0, static_cast<uint32_t> (header.GetSize ()));
        buffer->RemoveAtStart (static_cast<uint32_t> (header.GetSize ()));

        complete->RemoveHeader (header);

        m_rxTraceWithSeqTsSize (complete, from, localAddress, header);

        if (buffer->GetSize () > header.GetSerializedSize ())
        {
            buffer->PeekHeader (header);
        }
        else
        {
            break;
        }
    }
}

void TracerPacketSink::HandlePeerClose (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}
 
void TracerPacketSink::HandlePeerError (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}

void TracerPacketSink::HandleAccept (Ptr<Socket> s, const Address& from)
{
    NS_LOG_FUNCTION (this << s << from);
    s->SetRecvCallback (MakeCallback (&TracerPacketSink::HandleRead, this));        /* 将 Receiver Callback 连接到 HandleRead */
    m_socketList.push_back (s);     /* 将 Socket 存入 List */

    Ptr<Tracer> tracer = CreateObject<Tracer> (s, m_traceflag, m_foldername);
    tracer->RegisterTraceFunctions ();
    m_tracerList.push_back (tracer);

    /* 对 socket 进行设置 */
}

} // Namespace ns3
