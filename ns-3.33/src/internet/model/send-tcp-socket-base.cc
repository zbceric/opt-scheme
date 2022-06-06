/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-22 10:24:33
 * @LastEditTime: 2022-04-22 12:24:35
 * @LastEditors: Zhang Bochun
 * @Description: Sender Tcp Socket Base
 * @FilePath: /ns-3.33/src/internet/model/send-tcp-socket-base.cc
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
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/ipv6-end-point.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/tcp-tx-buffer.h"
#include "ns3/tcp-rx-buffer.h"
#include "ns3/rtt-estimator.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-option-winscale.h"
#include "ns3/tcp-option-ts.h"
#include "ns3/tcp-option-sack-permitted.h"
#include "ns3/tcp-option-sack.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-recovery-ops.h"
#include "ns3/tcp-rate-ops.h"
#include "send-tcp-socket-base.h"

#include <math.h>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SendTcpSocketBase");

NS_OBJECT_ENSURE_REGISTERED (SendTcpSocketBase);

TypeId
SendTcpSocketBase::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SendTcpSocketBase")
        .SetParent<TcpSocketBase> ()
        .SetGroupName ("Internet")
        .AddConstructor<SendTcpSocketBase> ()

        // 自己加的
        // 这两个参量主要用于 sender 检测速率变化
        // .AddTraceSource ("TcpRateUpdated",
        //                 "Tcp rate information has been updated at receiver (local of pace-tcp-socket-base)",
        //                 MakeTraceSourceAccessor (&TcpSocketBase::m_rateTrace),
        //                 "ns3::TcpRateLinux::TcpRateUpdated")
        // .AddTraceSource ("TcpRateSampleUpdated",
        //                 "Tcp rate sample has been updated at receiver (local of pace-tcp-socket-base)",
        //                 MakeTraceSourceAccessor (&TcpSocketBase::m_rateSampleTrace),
        //                 "ns3::TcpRateLinux::TcpRateSampleUpdated")
    ;
    return tid;
}

TypeId
SendTcpSocketBase::GetInstanceTypeId () const
{
    return SendTcpSocketBase::GetTypeId ();
}

SendTcpSocketBase::SendTcpSocketBase (void)
  : TcpSocketBase ()
{
    std::cout << GetTypeId () << std::endl;
}

SendTcpSocketBase::SendTcpSocketBase (const SendTcpSocketBase& sock)
  : TcpSocketBase (sock)
{
    std::cout << GetTypeId () << std::endl;
}
  

SendTcpSocketBase::~SendTcpSocketBase (void)
{
}



/* Associate a node with this TCP socket */
void
SendTcpSocketBase::SetNode (Ptr<Node> node)
{
    m_node = node;
}

/* Associate the L4 protocol (e.g. mux/demux) with this socket */
void
SendTcpSocketBase::SetTcp (Ptr<TcpL4Protocol> tcp)
{
    m_tcp = tcp;
}

/* Set an RTT estimator with this socket */
void
SendTcpSocketBase::SetRtt (Ptr<RttEstimator> rtt)
{
    m_rtt = rtt;
}

/* Inherit from Socket class: Returns error code */
enum Socket::SocketErrno
SendTcpSocketBase::GetErrno (void) const
{
    return m_errno;
}

/* Inherit from Socket class: Returns socket type, NS3_SOCK_STREAM */
enum Socket::SocketType
SendTcpSocketBase::GetSocketType (void) const
{
    return NS3_SOCK_STREAM;
}

/* Inherit from Socket class: Returns associated node */
Ptr<Node>
SendTcpSocketBase::GetNode (void) const
{
    return m_node;
}

/* Inherit from Socket class: Bind socket to an end-point in TcpL4Protocol */
int
SendTcpSocketBase::Bind (void)
{
    NS_LOG_FUNCTION (this);
    m_endPoint = m_tcp->Allocate ();
    if (0 == m_endPoint)
    {
        m_errno = ERROR_ADDRNOTAVAIL;
        return -1;
    }

    m_tcp->AddSocket (this);

  return SetupCallback ();
}

int
SendTcpSocketBase::Bind6 (void)
{
    NS_LOG_FUNCTION (this);
    m_endPoint6 = m_tcp->Allocate6 ();
    if (0 == m_endPoint6)
    {
        m_errno = ERROR_ADDRNOTAVAIL;
        return -1;
    }

    m_tcp->AddSocket (this);

    return SetupCallback ();
}

/* Inherit from Socket class: Bind socket (with specific address) to an end-point in TcpL4Protocol */
int
SendTcpSocketBase::Bind (const Address &address)
{
    NS_LOG_FUNCTION (this << address);
    if (InetSocketAddress::IsMatchingType (address))
    {
        InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
        Ipv4Address ipv4 = transport.GetIpv4 ();
        uint16_t port = transport.GetPort ();
        SetIpTos (transport.GetTos ());
        if (ipv4 == Ipv4Address::GetAny () && port == 0)
        {
            m_endPoint = m_tcp->Allocate ();
        }
        else if (ipv4 == Ipv4Address::GetAny () && port != 0)
        {
            m_endPoint = m_tcp->Allocate (GetBoundNetDevice (), port);
        }
        else if (ipv4 != Ipv4Address::GetAny () && port == 0)
        {
            m_endPoint = m_tcp->Allocate (ipv4);
        }
        else if (ipv4 != Ipv4Address::GetAny () && port != 0)
        {
            m_endPoint = m_tcp->Allocate (GetBoundNetDevice (), ipv4, port);
        }
        if (0 == m_endPoint)
        {
            m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
            return -1;
        }
    }
    else if (Inet6SocketAddress::IsMatchingType (address))
    {
        Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
        Ipv6Address ipv6 = transport.GetIpv6 ();
        uint16_t port = transport.GetPort ();
        if (ipv6 == Ipv6Address::GetAny () && port == 0)
        {
            m_endPoint6 = m_tcp->Allocate6 ();
        }
        else if (ipv6 == Ipv6Address::GetAny () && port != 0)
        {
            m_endPoint6 = m_tcp->Allocate6 (GetBoundNetDevice (), port);
        }
        else if (ipv6 != Ipv6Address::GetAny () && port == 0)
        {
            m_endPoint6 = m_tcp->Allocate6 (ipv6);
        }
        else if (ipv6 != Ipv6Address::GetAny () && port != 0)
        {
            m_endPoint6 = m_tcp->Allocate6 (GetBoundNetDevice (), ipv6, port);
        }
        if (0 == m_endPoint6)
        {
            m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
            return -1;
        }
    }
    else
    {
        m_errno = ERROR_INVAL;
        return -1;
    }

    m_tcp->AddSocket (this);

    NS_LOG_LOGIC ("SendTcpSocketBase " << this << " got an endpoint: " << m_endPoint);

    return SetupCallback ();
}

void
SendTcpSocketBase::SetInitialSSThresh (uint32_t threshold)
{
    NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || threshold == m_tcb->m_initialSsThresh,
                          "SendTcpSocketBase::SetSSThresh() cannot change initial ssThresh after connection started.");

    m_tcb->m_initialSsThresh = threshold;
}

uint32_t
SendTcpSocketBase::GetInitialSSThresh (void) const
{
    return m_tcb->m_initialSsThresh;
}

void
SendTcpSocketBase::SetInitialCwnd (uint32_t cwnd)
{
    NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || cwnd == m_tcb->m_initialCWnd,
                          "SendTcpSocketBase::SetInitialCwnd() cannot change initial cwnd after connection started.");

    m_tcb->m_initialCWnd = cwnd;
}

uint32_t
SendTcpSocketBase::GetInitialCwnd (void) const
{
    return m_tcb->m_initialCWnd;
}

/* Inherit from Socket class: Initiate connection to a remote address:port */
int
SendTcpSocketBase::Connect (const Address & address)
{
    NS_LOG_FUNCTION (this << address);

    // If haven't do so, Bind() this socket first
    if (InetSocketAddress::IsMatchingType (address))
    {
        if (m_endPoint == nullptr)
        {
            if (Bind () == -1)
            {
                NS_ASSERT (m_endPoint == nullptr);
                return -1; // Bind() failed
            }
            NS_ASSERT (m_endPoint != nullptr);
        }
        InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
        m_endPoint->SetPeer (transport.GetIpv4 (), transport.GetPort ());
        SetIpTos (transport.GetTos ());
        m_endPoint6 = nullptr;

        // Get the appropriate local address and port number from the routing protocol and set up endpoint
        if (SetupEndpoint () != 0)
        {
            NS_LOG_ERROR ("Route to destination does not exist ?!");
            return -1;
        }
    }
    else if (Inet6SocketAddress::IsMatchingType (address))
    {
        // If we are operating on a v4-mapped address, translate the address to
        // a v4 address and re-call this function
        Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
        Ipv6Address v6Addr = transport.GetIpv6 ();
        if (v6Addr.IsIpv4MappedAddress () == true)
        {
            Ipv4Address v4Addr = v6Addr.GetIpv4MappedAddress ();
            return Connect (InetSocketAddress (v4Addr, transport.GetPort ()));
        }

        if (m_endPoint6 == nullptr)
        {
            if (Bind6 () == -1)
            {
                NS_ASSERT (m_endPoint6 == nullptr);
                return -1; // Bind() failed
            }
            NS_ASSERT (m_endPoint6 != nullptr);
        }
        m_endPoint6->SetPeer (v6Addr, transport.GetPort ());
        m_endPoint = nullptr;

        // Get the appropriate local address and port number from the routing protocol and set up endpoint
        if (SetupEndpoint6 () != 0)
        {
            NS_LOG_ERROR ("Route to destination does not exist ?!");
            return -1;
        }
    }
    else
    {
        m_errno = ERROR_INVAL;
        return -1;
    }

    // Re-initialize parameters in case this socket is being reused after CLOSE
    m_rtt->Reset ();
    m_synCount = m_synRetries;
    m_dataRetrCount = m_dataRetries;

    // DoConnect() will do state-checking and send a SYN packet
    return DoConnect ();
}

/* Inherit from Socket class: Listen on the endpoint for an incoming connection */
int
SendTcpSocketBase::Listen (void)
{
    NS_LOG_FUNCTION (this);

    // Linux quits EINVAL if we're not in CLOSED state, so match what they do
    if (m_state != CLOSED)
    {
        m_errno = ERROR_INVAL;
        return -1;
    }
    // In other cases, set the state to LISTEN and done
    NS_LOG_DEBUG ("CLOSED -> LISTEN");
    m_state = LISTEN;
    return 0;
}

/* Inherit from Socket class: Kill this socket and signal the peer (if any) */
int
SendTcpSocketBase::Close (void)
{
    NS_LOG_FUNCTION (this);
    /// \internal
    /// First we check to see if there is any unread rx data.
    /// \bugid{426} claims we should send reset in this case.
    if (m_tcb->m_rxBuffer->Size () != 0)
    {
        NS_LOG_WARN ("Socket " << this << " << unread rx data during close.  Sending reset." <<
                    "This is probably due to a bad sink application; check its code");
        SendRST ();
        return 0;
    }

    if (m_txBuffer->SizeFromSequence (m_tcb->m_nextTxSequence) > 0)
    { // App close with pending data must wait until all data transmitted
        if (m_closeOnEmpty == false)
        {
            m_closeOnEmpty = true;
            NS_LOG_INFO ("Socket " << this << " deferring close, state " << TcpStateName[m_state]);
        }
        return 0;
    }
    return DoClose ();
}

/* Inherit from Socket class: Signal a termination of send */
int
SendTcpSocketBase::ShutdownSend (void)
{
    NS_LOG_FUNCTION (this);

    //this prevents data from being added to the buffer
    m_shutdownSend = true;
    m_closeOnEmpty = true;
    //if buffer is already empty, send a fin now
    //otherwise fin will go when buffer empties.
    if (m_txBuffer->Size () == 0)
    {
        if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
        {
            NS_LOG_INFO ("Empty tx buffer, send fin");
            SendEmptyPacket (TcpHeader::FIN);

            if (m_state == ESTABLISHED)
            { // On active close: I am the first one to send FIN
                NS_LOG_DEBUG ("ESTABLISHED -> FIN_WAIT_1");
                m_state = FIN_WAIT_1;
            }
            else
            { // On passive close: Peer sent me FIN already
                NS_LOG_DEBUG ("CLOSE_WAIT -> LAST_ACK");
                m_state = LAST_ACK;
            }
        }
    }

    return 0;
}

/* Inherit from Socket class: Signal a termination of receive */
int
SendTcpSocketBase::ShutdownRecv (void)
{
    NS_LOG_FUNCTION (this);
    m_shutdownRecv = true;
    return 0;
}

/* Inherit from Socket class: Send a packet. Parameter flags is not used.
    Packet has no TCP header. Invoked by upper-layer application */
int
SendTcpSocketBase::Send (Ptr<Packet> p, uint32_t flags)
{
    NS_LOG_FUNCTION (this << p);
    NS_ABORT_MSG_IF (flags, "use of flags is not supported in SendTcpSocketBase::Send()");
    if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
    {
        // Store the packet into Tx buffer
        if (!m_txBuffer->Add (p))
        { // TxBuffer overflow, send failed
            m_errno = ERROR_MSGSIZE;
            return -1;
        }
        if (m_shutdownSend)
        {
            m_errno = ERROR_SHUTDOWN;
            return -1;
        }

        m_rateOps->CalculateAppLimited(m_tcb->m_cWnd, m_tcb->m_bytesInFlight, m_tcb->m_segmentSize,
                                       m_txBuffer->TailSequence (), m_tcb->m_nextTxSequence,
                                       m_txBuffer->GetLost (), m_txBuffer->GetRetransmitsCount ());

        // Submit the data to lower layers
        NS_LOG_LOGIC ("txBufSize=" << m_txBuffer->Size () << " state " << TcpStateName[m_state]);
        if ((m_state == ESTABLISHED || m_state == CLOSE_WAIT) && AvailableWindow () > 0)
        {   // Try to send the data out: Add a little step to allow the application
            // to fill the buffer
            if (!m_sendPendingDataEvent.IsRunning ())
            {
                m_sendPendingDataEvent = Simulator::Schedule (TimeStep (1),
                                                              &SendTcpSocketBase::SendPendingData,
                                                              this, m_connected);
            }
        }
        return p->GetSize ();
    }
    else
    {   // Connection not established yet
        m_errno = ERROR_NOTCONN;
        return -1; // Send failure
    }
}

/* Inherit from Socket class: In SendTcpSocketBase, it is same as Send() call */
int
SendTcpSocketBase::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
{
    NS_UNUSED (address);
    return Send (p, flags); // SendTo() and Send() are the same
}

/* Inherit from Socket class: Return data to upper-layer application. Parameter flags
   is not used. Data is returned as a packet of size no larger than maxSize */
Ptr<Packet>
SendTcpSocketBase::Recv (uint32_t maxSize, uint32_t flags)
{
    NS_LOG_FUNCTION (this);
    NS_ABORT_MSG_IF (flags, "use of flags is not supported in SendTcpSocketBase::Recv()");
    if (m_tcb->m_rxBuffer->Size () == 0 && m_state == CLOSE_WAIT)
    {
        return Create<Packet> (); // Send EOF on connection close
    }
    Ptr<Packet> outPacket = m_tcb->m_rxBuffer->Extract (maxSize);
    return outPacket;
}

/* Inherit from Socket class: Recv and return the remote's address */
Ptr<Packet>
SendTcpSocketBase::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
{
    NS_LOG_FUNCTION (this << maxSize << flags);
    Ptr<Packet> packet = Recv (maxSize, flags);
    // Null packet means no data to read, and an empty packet indicates EOF
    if (packet != nullptr && packet->GetSize () != 0)
    {
        if (m_endPoint != nullptr)
        {
            fromAddress = InetSocketAddress (m_endPoint->GetPeerAddress (), m_endPoint->GetPeerPort ());
        }
        else if (m_endPoint6 != nullptr)
        {
            fromAddress = Inet6SocketAddress (m_endPoint6->GetPeerAddress (), m_endPoint6->GetPeerPort ());
        }
        else
        {
            fromAddress = InetSocketAddress (Ipv4Address::GetZero (), 0);
        }
    }
    return packet;
}

/* Inherit from Socket class: Get the max number of bytes an app can send */
uint32_t
SendTcpSocketBase::GetTxAvailable (void) const
{
    NS_LOG_FUNCTION (this);
    return m_txBuffer->Available ();
}

/* Inherit from Socket class: Get the max number of bytes an app can read */
uint32_t
SendTcpSocketBase::GetRxAvailable (void) const
{
    NS_LOG_FUNCTION (this);
    return m_tcb->m_rxBuffer->Available ();
}

/* Inherit from Socket class: Return local address:port */
int
SendTcpSocketBase::GetSockName (Address &address) const
{
    NS_LOG_FUNCTION (this);
    if (m_endPoint != nullptr)
    {
        address = InetSocketAddress (m_endPoint->GetLocalAddress (), m_endPoint->GetLocalPort ());
    }
    else if (m_endPoint6 != nullptr)
    {
        address = Inet6SocketAddress (m_endPoint6->GetLocalAddress (), m_endPoint6->GetLocalPort ());
    }
    else
    {   // It is possible to call this method on a socket without a name
        // in which case, behavior is unspecified
        // Should this return an InetSocketAddress or an Inet6SocketAddress?
        address = InetSocketAddress (Ipv4Address::GetZero (), 0);
    }
    return 0;
}

int
SendTcpSocketBase::GetPeerName (Address &address) const
{
    NS_LOG_FUNCTION (this << address);

    if (!m_endPoint && !m_endPoint6)
    {
        m_errno = ERROR_NOTCONN;
        return -1;
    }

    if (m_endPoint)
    {
        address = InetSocketAddress (m_endPoint->GetPeerAddress (),
                                    m_endPoint->GetPeerPort ());
    }
    else if (m_endPoint6)
    {
        address = Inet6SocketAddress (m_endPoint6->GetPeerAddress (),
                                      m_endPoint6->GetPeerPort ());
    }
    else
    {
        NS_ASSERT (false);
    }

    return 0;
}

/* Inherit from Socket class: Bind this socket to the specified NetDevice */
void
SendTcpSocketBase::BindToNetDevice (Ptr<NetDevice> netdevice)
{
    NS_LOG_FUNCTION (netdevice);
    Socket::BindToNetDevice (netdevice); // Includes sanity check
    if (m_endPoint != nullptr)
    {
        m_endPoint->BindToNetDevice (netdevice);
    }

    if (m_endPoint6 != nullptr)
    {
        m_endPoint6->BindToNetDevice (netdevice);
    }

  return;
}





void
SendTcpSocketBase::DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
                            const Address &toAddress)
{
    // in case the packet still has a priority tag attached, remove it
    SocketPriorityTag priorityTag;
    packet->RemovePacketTag (priorityTag);

    // Peel off TCP header
    TcpHeader tcpHeader;
    packet->RemoveHeader (tcpHeader);
    SequenceNumber32 seq = tcpHeader.GetSequenceNumber ();

    if (m_state == ESTABLISHED && !(tcpHeader.GetFlags () & TcpHeader::RST))
    {
        // Check if the sender has responded to ECN echo by reducing the Congestion Window
        if (tcpHeader.GetFlags () & TcpHeader::CWR )
        {
            // Check if a packet with CE bit set is received. If there is no CE bit set, then change the state to ECN_IDLE to
            // stop sending ECN Echo messages. If there is CE bit set, the packet should continue sending ECN Echo messages
            //
            if (m_tcb->m_ecnState != TcpSocketState::ECN_CE_RCVD)
            {
                NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_IDLE");
                m_tcb->m_ecnState = TcpSocketState::ECN_IDLE;
            }
        }
    }

    m_rxTrace (packet, tcpHeader, this);

    if (tcpHeader.GetFlags () & TcpHeader::SYN)
    {
        /* The window field in a segment where the SYN bit is set (i.e., a <SYN>
        * or <SYN,ACK>) MUST NOT be scaled (from RFC 7323 page 9). But should be
        * saved anyway..
        */
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
            ProcessOptionSackPermitted (tcpHeader.GetOption (TcpOption::SACKPERMITTED));
        }
        else
        {
            m_sackEnabled = false;
            m_txBuffer->SetSackEnabled (false);
        }

        // When receiving a <SYN> or <SYN-ACK> we should adapt TS to the other end
        if (tcpHeader.HasOption (TcpOption::TS) && m_timestampEnabled)
        {
            ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS),
                                    tcpHeader.GetSequenceNumber ());
        }
        else
        {
            m_timestampEnabled = false;
        }

        // Initialize cWnd and ssThresh
        m_tcb->m_cWnd = GetInitialCwnd () * GetSegSize ();
        m_tcb->m_cWndInfl = m_tcb->m_cWnd;
        m_tcb->m_ssThresh = GetInitialSSThresh ();

        if (tcpHeader.GetFlags () & TcpHeader::ACK)
        {
            EstimateRtt (tcpHeader);
            m_highRxAckMark = tcpHeader.GetAckNumber ();
        }
    }
    else if (tcpHeader.GetFlags () & TcpHeader::ACK)
    {
        NS_ASSERT (!(tcpHeader.GetFlags () & TcpHeader::SYN));
        if (m_timestampEnabled)
        {
            if (!tcpHeader.HasOption (TcpOption::TS))
            {
                // Ignoring segment without TS, RFC 7323
                NS_LOG_LOGIC ("At state " << TcpStateName[m_state] <<
                              " received packet of seq [" << seq <<
                              ":" << seq + packet->GetSize () <<
                              ") without TS option. Silently discard it");
              return;
            }
            else
            {
                ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS),
                                        tcpHeader.GetSequenceNumber ());
            }
        }

        EstimateRtt (tcpHeader);
        UpdateWindowSize (tcpHeader);
    }


    if (m_rWnd.Get () == 0 && m_persistEvent.IsExpired ())
    {   // Zero window: Enter persist state to send 1 byte to probe
        NS_LOG_LOGIC (this << " Enter zerowindow persist state");
        NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                      (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
        m_retxEvent.Cancel ();
        NS_LOG_LOGIC ("Schedule persist timeout at time " <<
                      Simulator::Now ().GetSeconds () << " to expire at time " <<
                      (Simulator::Now () + m_persistTimeout).GetSeconds ());
        m_persistEvent = Simulator::Schedule (m_persistTimeout, &SendTcpSocketBase::PersistTimeout, this);
        NS_ASSERT (m_persistTimeout == Simulator::GetDelayLeft (m_persistEvent));
    }

    // TCP state machine code in different process functions
    // C.f.: tcp_rcv_state_process() in tcp_input.c in Linux kernel
    switch (m_state)
    {
        case ESTABLISHED:
            ProcessEstablished (packet, tcpHeader);
            break;
        case LISTEN:
            ProcessListen (packet, tcpHeader, fromAddress, toAddress);
            break;
        case TIME_WAIT:
            // Do nothing
            break;
        case CLOSED:
            // Send RST if the incoming packet is not a RST
            if ((tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG)) != TcpHeader::RST)
            {   // Since m_endPoint is not configured yet, we cannot use SendRST here
                TcpHeader h;
                Ptr<Packet> p = Create<Packet> ();
                h.SetFlags (TcpHeader::RST);
                h.SetSequenceNumber (m_tcb->m_nextTxSequence);
                h.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
                h.SetSourcePort (tcpHeader.GetDestinationPort ());
                h.SetDestinationPort (tcpHeader.GetSourcePort ());
                h.SetWindowSize (AdvertisedWindowSize ());
                AddOptions (h);
                m_txTrace (p, h, this);
                m_tcp->SendPacket (p, h, toAddress, fromAddress, m_boundnetdevice);
            }
            break;
        case SYN_SENT:
            ProcessSynSent (packet, tcpHeader);
            break;
        case SYN_RCVD:
            ProcessSynRcvd (packet, tcpHeader, fromAddress, toAddress);
            break;
        case FIN_WAIT_1:
        case FIN_WAIT_2:
        case CLOSE_WAIT:
            ProcessWait (packet, tcpHeader);
            break;
        case CLOSING:
            ProcessClosing (packet, tcpHeader);
            break;
        case LAST_ACK:
            ProcessLastAck (packet, tcpHeader);
            break;
        default: // mute compiler
            break;
    }

    if (m_rWnd.Get () != 0 && m_persistEvent.IsRunning ())
    {   // persist probes end, the other end has increased the window
        NS_ASSERT (m_connected);
        NS_LOG_LOGIC (this << " Leaving zerowindow persist state");
        m_persistEvent.Cancel ();

        SendPendingData (m_connected);
    }
}


/* Process the newly received ACK */
void
SendTcpSocketBase::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
    NS_LOG_FUNCTION (this << tcpHeader);

    NS_ASSERT (0 != (tcpHeader.GetFlags () & TcpHeader::ACK));
    NS_ASSERT (m_tcb->m_segmentSize > 0);

    uint32_t previousLost = m_txBuffer->GetLost ();
    uint32_t priorInFlight = m_tcb->m_bytesInFlight.Get ();

    // RFC 6675, Section 5, 1st paragraph:
    // Upon the receipt of any ACK containing SACK information, the
    // scoreboard MUST be updated via the Update () routine (done in ReadOptions)
    uint32_t bytesSacked = 0;
    uint64_t previousDelivered = m_rateOps->GetConnectionRate ().m_delivered;
    ReadOptions (tcpHeader, &bytesSacked);

    SequenceNumber32 ackNumber = tcpHeader.GetAckNumber ();
    SequenceNumber32 oldHeadSequence = m_txBuffer->HeadSequence ();

    if (ackNumber < oldHeadSequence)
    {
        NS_LOG_DEBUG ("Possibly received a stale ACK (ack number < head sequence)");
        // If there is any data piggybacked, store it into m_rxBuffer
        if (packet->GetSize () > 0)
        {
            ReceivedData (packet, tcpHeader);
        }
      return;
    }
    if ((ackNumber > oldHeadSequence) && (ackNumber < m_recover)
                                      && (m_tcb->m_congState == TcpSocketState::CA_RECOVERY))
    {
        uint32_t segAcked = (ackNumber - oldHeadSequence)/m_tcb->m_segmentSize;
        for (uint32_t i = 0; i < segAcked; i++)
        {
            if (m_txBuffer->IsRetransmittedDataAcked (ackNumber - (i * m_tcb->m_segmentSize)))
            {
                m_tcb->m_isRetransDataAcked = true;
                NS_LOG_DEBUG ("Ack Number " << ackNumber <<
                              "is ACK of retransmitted packet.");
            }
        }
    }

    m_txBuffer->DiscardUpTo (ackNumber, MakeCallback (&TcpRateOps::SkbDelivered, m_rateOps));

    uint32_t currentDelivered = static_cast<uint32_t> (m_rateOps->GetConnectionRate ().m_delivered - previousDelivered);

    if (m_tcb->m_congState == TcpSocketState::CA_CWR && (ackNumber > m_recover))
    {
        // Recovery is over after the window exceeds m_recover
        // (although it may be re-entered below if ECE is still set)
        NS_LOG_DEBUG (TcpSocketState::TcpCongStateName[m_tcb->m_congState] << " -> CA_OPEN");
        m_tcb->m_congState = TcpSocketState::CA_OPEN;
        if (!m_congestionControl->HasCongControl ())
        {
            m_tcb->m_cWnd = m_tcb->m_ssThresh.Get ();
            m_recoveryOps->ExitRecovery (m_tcb);
            m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_COMPLETE_CWR);
        }
    }

    if (ackNumber > oldHeadSequence && (m_tcb->m_ecnState != TcpSocketState::ECN_DISABLED) && (tcpHeader.GetFlags () & TcpHeader::ECE))
    {
        if (m_ecnEchoSeq < ackNumber)
        {
            NS_LOG_INFO ("Received ECN Echo is valid");
            m_ecnEchoSeq = ackNumber;
            NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_ECE_RCVD");
            m_tcb->m_ecnState = TcpSocketState::ECN_ECE_RCVD;
            if (m_tcb->m_congState != TcpSocketState::CA_CWR)
            {
                EnterCwr (currentDelivered);
            }
        }
    }
    else if (m_tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD && !(tcpHeader.GetFlags () & TcpHeader::ECE))
    {
        m_tcb->m_ecnState = TcpSocketState::ECN_IDLE;
    }

    // Update bytes in flight before processing the ACK for proper calculation of congestion window
    NS_LOG_INFO ("Update bytes in flight before processing the ACK.");
    BytesInFlight ();

    // RFC 6675 Section 5: 2nd, 3rd paragraph and point (A), (B) implementation
    // are inside the function ProcessAck
    ProcessAck (ackNumber, (bytesSacked > 0), currentDelivered, oldHeadSequence);
    m_tcb->m_isRetransDataAcked = false;

    if (m_congestionControl->HasCongControl ())
    {
        uint32_t currentLost = m_txBuffer->GetLost ();
        uint32_t lost = (currentLost > previousLost) ?
              currentLost - previousLost :
              previousLost - currentLost;
        auto rateSample = m_rateOps->GenerateSample (currentDelivered, lost,
                                                false, priorInFlight, m_tcb->m_minRtt);
        auto rateConn = m_rateOps->GetConnectionRate ();
        m_congestionControl->CongControl(m_tcb, rateConn, rateSample);
    }

    // If there is any data piggybacked, store it into m_rxBuffer
    if (packet->GetSize () > 0)
    {
        ReceivedData (packet, tcpHeader);
    }

    // RFC 6675, Section 5, point (C), try to send more data. NB: (C) is implemented
    // inside SendPendingData
    SendPendingData (m_connected);
}

void
SendTcpSocketBase::ProcessAck(const SequenceNumber32 &ackNumber, bool scoreboardUpdated,
                          uint32_t currentDelivered, const SequenceNumber32 &oldHeadSequence)
{
    NS_LOG_FUNCTION (this << ackNumber << scoreboardUpdated);
    // RFC 6675, Section 5, 2nd paragraph:
    // If the incoming ACK is a cumulative acknowledgment, the TCP MUST
    // reset DupAcks to zero.
    bool exitedFastRecovery = false;
    uint32_t oldDupAckCount = m_dupAckCount; // remember the old value
    m_tcb->m_lastAckedSeq = ackNumber; // Update lastAckedSeq
    uint32_t bytesAcked = 0;

    /* In RFC 5681 the definition of duplicate acknowledgment was strict:
    *
    * (a) the receiver of the ACK has outstanding data,
    * (b) the incoming acknowledgment carries no data,
    * (c) the SYN and FIN bits are both off,
    * (d) the acknowledgment number is equal to the greatest acknowledgment
    *     received on the given connection (TCP.UNA from [RFC793]),
    * (e) the advertised window in the incoming acknowledgment equals the
    *     advertised window in the last incoming acknowledgment.
    *
    * With RFC 6675, this definition has been reduced:
    *
    * (a) the ACK is carrying a SACK block that identifies previously
    *     unacknowledged and un-SACKed octets between HighACK (TCP.UNA) and
    *     HighData (m_highTxMark)
    */

    bool isDupack = m_sackEnabled ?
      scoreboardUpdated
      : ackNumber == oldHeadSequence &&
      ackNumber < m_tcb->m_highTxMark;

    NS_LOG_DEBUG ("ACK of " << ackNumber <<
                  " SND.UNA=" << oldHeadSequence <<
                  " SND.NXT=" << m_tcb->m_nextTxSequence <<
                  " in state: " << TcpSocketState::TcpCongStateName[m_tcb->m_congState] <<
                  " with m_recover: " << m_recover);

    // RFC 6675, Section 5, 3rd paragraph:
    // If the incoming ACK is a duplicate acknowledgment per the definition
    // in Section 2 (regardless of its status as a cumulative
    // acknowledgment), and the TCP is not currently in loss recovery
    if (isDupack)
    {
        // loss recovery check is done inside this function thanks to
        // the congestion state machine
        DupAck (currentDelivered);
    }

    if (ackNumber == oldHeadSequence
        && ackNumber == m_tcb->m_highTxMark)
    {
        // Dupack, but the ACK is precisely equal to the nextTxSequence
        return;
    }
    else if (ackNumber == oldHeadSequence
            && ackNumber > m_tcb->m_highTxMark)
    {
        // ACK of the FIN bit ... nextTxSequence is not updated since we
        // don't have anything to transmit
        NS_LOG_DEBUG ("Update nextTxSequence manually to " << ackNumber);
        m_tcb->m_nextTxSequence = ackNumber;
    }
    else if (ackNumber == oldHeadSequence)
    {
        // DupAck. Artificially call PktsAcked: after all, one segment has been ACKed.
        m_congestionControl->PktsAcked (m_tcb, 1, m_tcb->m_lastRtt);
    }
    else if (ackNumber > oldHeadSequence)
    {
        // Please remember that, with SACK, we can enter here even if we
        // received a dupack.
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

        // Dupack count is reset to eventually fast-retransmit after 3 dupacks.
        // Any SACK-ed segment will be cleaned up by DiscardUpTo.
        // In the case that we advanced SND.UNA, but the ack contains SACK blocks,
        // we do not reset. At the third one we will retransmit.
        // If we are already in recovery, this check is useless since dupAcks
        // are not considered in this phase. When from Recovery we go back
        // to open, then dupAckCount is reset anyway.
        if (!isDupack)
        {
            m_dupAckCount = 0;
        }

        // RFC 6675, Section 5, part (B)
        // (B) Upon receipt of an ACK that does not cover RecoveryPoint, the
        // following actions MUST be taken:
        //
        // (B.1) Use Update () to record the new SACK information conveyed
        //       by the incoming ACK.
        // (B.2) Use SetPipe () to re-calculate the number of octets still
        //       in the network.
        //
        // (B.1) is done at the beginning, while (B.2) is delayed to part (C) while
        // trying to transmit with SendPendingData. We are not allowed to exit
        // the CA_RECOVERY phase. Just process this partial ack (RFC 5681)
        if (ackNumber < m_recover && m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
        {
            if (!m_sackEnabled)
            {
                // Manually set the head as lost, it will be retransmitted.
                NS_LOG_INFO ("Partial ACK. Manually setting head as lost");
                m_txBuffer->MarkHeadAsLost ();
            }

            // Before retransmitting the packet perform DoRecovery and check if
            // there is available window
            if (!m_congestionControl->HasCongControl () && segsAcked >= 1)
            {
                m_recoveryOps->DoRecovery (m_tcb, currentDelivered);
            }

            // If the packet is already retransmitted do not retransmit it
            if (!m_txBuffer->IsRetransmittedDataAcked (ackNumber + m_tcb->m_segmentSize))
            {
                DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
                m_tcb->m_cWndInfl = SafeSubtraction (m_tcb->m_cWndInfl, bytesAcked);
            }

            // This partial ACK acknowledge the fact that one segment has been
            // previously lost and now successfully received. All others have
            // been processed when they come under the form of dupACKs
            m_congestionControl->PktsAcked (m_tcb, 1, m_tcb->m_lastRtt);
            NewAck (ackNumber, m_isFirstPartialAck);

            if (m_isFirstPartialAck)
            {
                NS_LOG_DEBUG ("Partial ACK of " << ackNumber <<
                              " and this is the first (RTO will be reset);"
                              " cwnd set to " << m_tcb->m_cWnd <<
                              " recover seq: " << m_recover <<
                              " dupAck count: " << m_dupAckCount);
                m_isFirstPartialAck = false;
            }
            else
            {
                NS_LOG_DEBUG ("Partial ACK of " << ackNumber <<
                              " and this is NOT the first (RTO will not be reset)"
                              " cwnd set to " << m_tcb->m_cWnd <<
                              " recover seq: " << m_recover <<
                              " dupAck count: " << m_dupAckCount);
            }
        }
        // From RFC 6675 section 5.1
        // In addition, a new recovery phase (as described in Section 5) MUST NOT
        // be initiated until HighACK is greater than or equal to the new value
        // of RecoveryPoint.
        else if (ackNumber < m_recover && m_tcb->m_congState == TcpSocketState::CA_LOSS)
        {
            m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
            m_congestionControl->IncreaseWindow (m_tcb, segsAcked);

            NS_LOG_DEBUG (" Cong Control Called, cWnd=" << m_tcb->m_cWnd <<
                          " ssTh=" << m_tcb->m_ssThresh);
            if (!m_sackEnabled)
              {
                NS_ASSERT_MSG (m_txBuffer->GetSacked () == 0,
                              "Some segment got dup-acked in CA_LOSS state: " <<
                              m_txBuffer->GetSacked ());
              }
            NewAck (ackNumber, true);
        }
        else if (m_tcb->m_congState == TcpSocketState::CA_CWR)
        {
            m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
            // TODO: need to check behavior if marking is compounded by loss
            // and/or packet reordering
            if (!m_congestionControl->HasCongControl () && segsAcked >= 1)
            {
                m_recoveryOps->DoRecovery (m_tcb, currentDelivered);
            }
            NewAck (ackNumber, true);
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
                    // The network reorder packets. Linux changes the counting lost
                    // packet algorithm from FACK to NewReno. We simply go back in Open.
                    m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
                    m_tcb->m_congState = TcpSocketState::CA_OPEN;
                    NS_LOG_DEBUG (segsAcked << " segments acked in CA_DISORDER, ack of " <<
                                  ackNumber << " exiting CA_DISORDER -> CA_OPEN");
                }
                else
                {
                    NS_LOG_DEBUG (segsAcked << " segments acked in CA_DISORDER, ack of " <<
                                  ackNumber << " but still in CA_DISORDER");
                }
            }
            // RFC 6675, Section 5:
            // Once a TCP is in the loss recovery phase, the following procedure
            // MUST be used for each arriving ACK:
            // (A) An incoming cumulative ACK for a sequence number greater than
            // RecoveryPoint signals the end of loss recovery, and the loss
            // recovery phase MUST be terminated.  Any information contained in
            // the scoreboard for sequence numbers greater than the new value of
            // HighACK SHOULD NOT be cleared when leaving the loss recovery
            // phase.
            else if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
            {
                m_isFirstPartialAck = true;

                // Recalculate the segs acked, that are from m_recover to ackNumber
                // (which are the ones we have not passed to PktsAcked and that
                // can increase cWnd)
                // TODO:  check consistency for dynamic segment size
                segsAcked = static_cast<uint32_t>(ackNumber - oldHeadSequence) / m_tcb->m_segmentSize;
                m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
                m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_COMPLETE_CWR);
                m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
                m_tcb->m_congState = TcpSocketState::CA_OPEN;
                exitedFastRecovery = true;
                m_dupAckCount = 0; // From recovery to open, reset dupack

                NS_LOG_DEBUG (segsAcked << " segments acked in CA_RECOVER, ack of " <<
                              ackNumber << ", exiting CA_RECOVERY -> CA_OPEN");
            }
            else if (m_tcb->m_congState == TcpSocketState::CA_LOSS)
            {
                m_isFirstPartialAck = true;

                // Recalculate the segs acked, that are from m_recover to ackNumber
                // (which are the ones we have not passed to PktsAcked and that
                // can increase cWnd)
                segsAcked = (ackNumber - m_recover) / m_tcb->m_segmentSize;

                m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);

                m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
                m_tcb->m_congState = TcpSocketState::CA_OPEN;
                NS_LOG_DEBUG (segsAcked << " segments acked in CA_LOSS, ack of" <<
                              ackNumber << ", exiting CA_LOSS -> CA_OPEN");
            }

            if (ackNumber >= m_recover)
            {
                // All lost segments in the congestion event have been
                // retransmitted successfully. The recovery point (m_recover)
                // should be deactivated.
                m_recoverActive = false;
            }

            if (exitedFastRecovery)
            {
                NewAck (ackNumber, true);
                m_tcb->m_cWnd = m_tcb->m_ssThresh.Get ();
                m_recoveryOps->ExitRecovery (m_tcb);
                NS_LOG_DEBUG ("Leaving Fast Recovery; BytesInFlight() = " <<
                              BytesInFlight () << "; cWnd = " << m_tcb->m_cWnd);
            }
            if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
            {
                m_congestionControl->IncreaseWindow (m_tcb, segsAcked);

                m_tcb->m_cWndInfl = m_tcb->m_cWnd;

                NS_LOG_LOGIC ("Congestion control called: " <<
                              " cWnd: " << m_tcb->m_cWnd <<
                              " ssTh: " << m_tcb->m_ssThresh <<
                              " segsAcked: " << segsAcked);

                NewAck (ackNumber, true);
            }
        }
    }
    // Update the pacing rate, since m_congestionControl->IncreaseWindow() or
    // m_congestionControl->PktsAcked () may change m_tcb->m_cWnd
    // Make sure that control reaches the end of this function and there is no
    // return in between
    UpdatePacingRate ();
}


/* Send an empty packet with specified TCP flags */
void
SendTcpSocketBase::SendEmptyPacket (uint8_t flags)
{
    NS_LOG_FUNCTION (this << static_cast<uint32_t> (flags));

    if (m_endPoint == nullptr && m_endPoint6 == nullptr)
    {
        NS_LOG_WARN ("Failed to send empty packet due to null endpoint");
        return;
    }

    Ptr<Packet> p = Create<Packet> ();
    TcpHeader header;
    SequenceNumber32 s = m_tcb->m_nextTxSequence;

    if (flags & TcpHeader::FIN)
    {
        flags |= TcpHeader::ACK;
    }
    else if (m_state == FIN_WAIT_1 || m_state == LAST_ACK || m_state == CLOSING)
    {
        ++s;
    }

    AddSocketTags (p);

    header.SetFlags (flags);
    header.SetSequenceNumber (s);
    header.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
    if (m_endPoint != nullptr)
    {
        header.SetSourcePort (m_endPoint->GetLocalPort ());
        header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
    else
    {
        header.SetSourcePort (m_endPoint6->GetLocalPort ());
        header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
    AddOptions (header);

    // RFC 6298, clause 2.4
    m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);

    uint16_t windowSize = AdvertisedWindowSize ();
    bool hasSyn = flags & TcpHeader::SYN;
    bool hasFin = flags & TcpHeader::FIN;
    bool isAck = flags == TcpHeader::ACK;
    if (hasSyn)
    {
        if (m_winScalingEnabled)
        { // The window scaling option is set only on SYN packets
            AddOptionWScale (header);
        }

        if (m_sackEnabled)
        {
            AddOptionSackPermitted (header);
        }

        if (m_synCount == 0)
        {   // No more connection retries, give up
            NS_LOG_LOGIC ("Connection failed.");
            m_rtt->Reset (); //According to recommendation -> RFC 6298
            NotifyConnectionFailed ();
            m_state = CLOSED;
            DeallocateEndPoint ();
            return;
        }
        else
        {   // Exponential backoff of connection time out
            int backoffCount = 0x1 << (m_synRetries - m_synCount);
            m_rto = m_cnTimeout * backoffCount;
            m_synCount--;
        }

        if (m_synRetries - 1 == m_synCount)
        {
            UpdateRttHistory (s, 0, false);
        }
        else
        { // This is SYN retransmission
            UpdateRttHistory (s, 0, true);
        }

        windowSize = AdvertisedWindowSize (false);
    }
    header.SetWindowSize (windowSize);

    if (flags & TcpHeader::ACK)
    { // If sending an ACK, cancel the delay ACK as well
        m_delAckEvent.Cancel ();
        m_delAckCount = 0;
        if (m_highTxAck < header.GetAckNumber ())
        {
            m_highTxAck = header.GetAckNumber ();
        }
        if (m_sackEnabled && m_tcb->m_rxBuffer->GetSackListSize () > 0)
        {
            AddOptionSack (header);
        }
        NS_LOG_INFO ("Sending a pure ACK, acking seq " << m_tcb->m_rxBuffer->NextRxSequence ());
    }

    m_txTrace (p, header, this);

    if (m_endPoint != nullptr)
    {
        m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                           m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
    else
    {
        m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                          m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }


    if (m_retxEvent.IsExpired () && (hasSyn || hasFin) && !isAck )
    { // Retransmit SYN / SYN+ACK / FIN / FIN+ACK to guard against lost
        NS_LOG_LOGIC ("Schedule retransmission timeout at time "
                      << Simulator::Now ().GetSeconds () << " to expire at time "
                      << (Simulator::Now () + m_rto.Get ()).GetSeconds ());
        m_retxEvent = Simulator::Schedule (m_rto, &SendTcpSocketBase::SendEmptyPacket, this, flags);
    }
}


/* This function is called only if a SYN received in LISTEN state. After
   SendTcpSocketBase cloned, allocate a new end point to handle the incoming
   connection and send a SYN+ACK to complete the handshake. */
void
SendTcpSocketBase::CompleteFork (Ptr<Packet> p, const TcpHeader& h,
                             const Address& fromAddress, const Address& toAddress)
{
    NS_LOG_FUNCTION (this << p << h << fromAddress << toAddress);
    NS_UNUSED (p);
    // Get port and address from peer (connecting host)
    if (InetSocketAddress::IsMatchingType (toAddress))
    {
        m_endPoint = m_tcp->Allocate (GetBoundNetDevice (),
                                      InetSocketAddress::ConvertFrom (toAddress).GetIpv4 (),
                                      InetSocketAddress::ConvertFrom (toAddress).GetPort (),
                                      InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                      InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
        m_endPoint6 = nullptr;
    }
    else if (Inet6SocketAddress::IsMatchingType (toAddress))
    {
        m_endPoint6 = m_tcp->Allocate6 (GetBoundNetDevice (),
                                        Inet6SocketAddress::ConvertFrom (toAddress).GetIpv6 (),
                                        Inet6SocketAddress::ConvertFrom (toAddress).GetPort (),
                                        Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                        Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
        m_endPoint = nullptr;
    }
    m_tcp->AddSocket (this);

    // Change the cloned socket from LISTEN state to SYN_RCVD
    NS_LOG_DEBUG ("LISTEN -> SYN_RCVD");
    m_state = SYN_RCVD;
    m_synCount = m_synRetries;
    m_dataRetrCount = m_dataRetries;
    SetupCallback ();
    // Set the sequence number and send SYN+ACK
    m_tcb->m_rxBuffer->SetNextRxSequence (h.GetSequenceNumber () + SequenceNumber32 (1));

    /* Check if we received an ECN SYN packet. Change the ECN state of receiver to ECN_IDLE if sender has sent an ECN SYN
    * packet and the traffic is ECN Capable
    */
    if (m_tcb->m_useEcn != TcpSocketState::Off &&
        (h.GetFlags () & (TcpHeader::CWR | TcpHeader::ECE)) == (TcpHeader::CWR | TcpHeader::ECE))
    {
        SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK | TcpHeader::ECE);
        NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_IDLE");
        m_tcb->m_ecnState = TcpSocketState::ECN_IDLE;
    }
    else
    {
        SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
        m_tcb->m_ecnState = TcpSocketState::ECN_DISABLED;
    }
}


/* Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
    TCP header, and send to TcpL4Protocol */
uint32_t
SendTcpSocketBase::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{
    NS_LOG_FUNCTION (this << seq << maxSize << withAck);

    bool isStartOfTransmission = BytesInFlight () == 0U;
    TcpTxItem *outItem = m_txBuffer->CopyFromSequence (maxSize, seq);

    m_rateOps->SkbSent(outItem, isStartOfTransmission&&(m_tcb->m_highTxMark==m_tcb->m_nextTxSequence));

    bool isRetransmission = outItem->IsRetrans ();
    Ptr<Packet> p = outItem->GetPacketCopy ();
    uint32_t sz = p->GetSize (); // Size of packet
    uint8_t flags = withAck ? TcpHeader::ACK : 0;
    uint32_t remainingData = m_txBuffer->SizeFromSequence (seq + SequenceNumber32 (sz));

    // TCP sender should not send data out of the window advertised by the
    // peer when it is not retransmission.
    NS_ASSERT (isRetransmission || ((m_highRxAckMark + SequenceNumber32 (m_rWnd)) >= (seq + SequenceNumber32 (maxSize))));

    if (IsPacingEnabled ())
    {
        NS_LOG_INFO ("Pacing is enabled");
        if (m_pacingTimer.IsExpired ())
        {
            NS_LOG_DEBUG ("Current Pacing Rate " << m_tcb->m_pacingRate);
            NS_LOG_DEBUG ("Timer is in expired state, activate it " << m_tcb->m_pacingRate.Get ().CalculateBytesTxTime (sz));
            m_pacingTimer.Schedule (m_tcb->m_pacingRate.Get ().CalculateBytesTxTime (sz));
        }
        else
        {
            NS_LOG_INFO ("Timer is already in running state");
        }
    }
    else
    {
        NS_LOG_INFO ("Pacing is disabled");
    }

    if (withAck)
    {
        m_delAckEvent.Cancel ();
        m_delAckCount = 0;
    }

    if (m_tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD && m_ecnEchoSeq.Get() > m_ecnCWRSeq.Get () && !isRetransmission)
    {
        NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_CWR_SENT");
        m_tcb->m_ecnState = TcpSocketState::ECN_CWR_SENT;
        m_ecnCWRSeq = seq;
        flags |= TcpHeader::CWR;
        NS_LOG_INFO ("CWR flags set");
    }

    AddSocketTags (p);

    if (m_closeOnEmpty && (remainingData == 0))
    {
        flags |= TcpHeader::FIN;
        if (m_state == ESTABLISHED)
        {   // On active close: I am the first one to send FIN
            NS_LOG_DEBUG ("ESTABLISHED -> FIN_WAIT_1");
            m_state = FIN_WAIT_1;
        }
        else if (m_state == CLOSE_WAIT)
        {   // On passive close: Peer sent me FIN already
            NS_LOG_DEBUG ("CLOSE_WAIT -> LAST_ACK");
            m_state = LAST_ACK;
        }
    }
    TcpHeader header;
    header.SetFlags (flags);
    header.SetSequenceNumber (seq);
    header.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
    if (m_endPoint)
    {
        header.SetSourcePort (m_endPoint->GetLocalPort ());
        header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
    else
    {
        header.SetSourcePort (m_endPoint6->GetLocalPort ());
        header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
    header.SetWindowSize (AdvertisedWindowSize ());
    AddOptions (header);

    if (m_retxEvent.IsExpired ())
    {
        // Schedules retransmit timeout. m_rto should be already doubled.

        NS_LOG_LOGIC (this << " SendDataPacket Schedule ReTxTimeout at time " <<
                      Simulator::Now ().GetSeconds () << " to expire at time " <<
                      (Simulator::Now () + m_rto.Get ()).GetSeconds () );
        m_retxEvent = Simulator::Schedule (m_rto, &SendTcpSocketBase::ReTxTimeout, this);
    }

    m_txTrace (p, header, this);

    if (m_endPoint)
    {
        m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                          m_endPoint->GetPeerAddress (), m_boundnetdevice);
        NS_LOG_DEBUG ("Send segment of size " << sz << " with remaining data " <<
                      remainingData << " via TcpL4Protocol to " <<  m_endPoint->GetPeerAddress () <<
                      ". Header " << header);
    }
    else
    {
        m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                          m_endPoint6->GetPeerAddress (), m_boundnetdevice);
        NS_LOG_DEBUG ("Send segment of size " << sz << " with remaining data " <<
                      remainingData << " via TcpL4Protocol to " <<  m_endPoint6->GetPeerAddress () <<
                      ". Header " << header);
    }

    UpdateRttHistory (seq, sz, isRetransmission);

    // Update bytes sent during recovery phase
    if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY || m_tcb->m_congState == TcpSocketState::CA_CWR)
    {
        m_recoveryOps->UpdateBytesSent (sz);
    }

    // Notify the application of the data being sent unless this is a retransmit
    if (!isRetransmission)
    {
        Simulator::ScheduleNow (&SendTcpSocketBase::NotifyDataSent, this,
                                (seq + sz - m_tcb->m_highTxMark.Get ()));
    }
    // Update highTxMark
    m_tcb->m_highTxMark = std::max (seq + sz, m_tcb->m_highTxMark.Get ());
    return sz;
}

void
SendTcpSocketBase::UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
                                 bool isRetransmission)
{
    NS_LOG_FUNCTION (this);

    // update the history of sequence numbers used to calculate the RTT
    if (isRetransmission == false)
    { // This is the next expected one, just log at end
        m_history.push_back (RttHistory (seq, sz, Simulator::Now ()));
    }
    else
    { // This is a retransmit, find in list and mark as re-tx
        for (std::deque<RttHistory>::iterator i = m_history.begin (); i != m_history.end (); ++i)
        {
            if ((seq >= i->seq) && (seq < (i->seq + SequenceNumber32 (i->count))))
            { // Found it
                i->retx = true;
                i->count = ((seq + SequenceNumber32 (sz)) - i->seq); // And update count in hist
                break;
            }
        }
    }
}

uint32_t
SendTcpSocketBase::UnAckDataCount () const
{
    return m_tcb->m_highTxMark - m_txBuffer->HeadSequence ();
}

uint32_t
SendTcpSocketBase::BytesInFlight () const
{
    uint32_t bytesInFlight = m_txBuffer->BytesInFlight ();
    // Ugly, but we are not modifying the state; m_bytesInFlight is used
    // only for tracing purpose.
    m_tcb->m_bytesInFlight = bytesInFlight;

    NS_LOG_DEBUG ("Returning calculated bytesInFlight: " << bytesInFlight);
    return bytesInFlight;
}

uint32_t
SendTcpSocketBase::Window (void) const
{
    return std::min (m_rWnd.Get (), m_tcb->m_cWnd.Get ());
}

uint32_t
SendTcpSocketBase::AvailableWindow () const
{
    uint32_t win = Window ();             // Number of bytes allowed to be outstanding
    uint32_t inflight = BytesInFlight (); // Number of outstanding bytes
    return (inflight > win) ? 0 : win - inflight;
}

uint16_t
SendTcpSocketBase::AdvertisedWindowSize (bool scale) const
{
    NS_LOG_FUNCTION (this << scale);
    uint32_t w;

    // We don't want to advertise 0 after a FIN is received. So, we just use
    // the previous value of the advWnd.
    if (m_tcb->m_rxBuffer->GotFin ())
    {
        w = m_advWnd;
    }
    else
    {
        NS_ASSERT_MSG (m_tcb->m_rxBuffer->MaxRxSequence () - m_tcb->m_rxBuffer->NextRxSequence () >= 0,
                      "Unexpected sequence number values");
        w = static_cast<uint32_t> (m_tcb->m_rxBuffer->MaxRxSequence () - m_tcb->m_rxBuffer->NextRxSequence ());
    }

    // Ugly, but we are not modifying the state, that variable
    // is used only for tracing purpose.
    if (w != m_advWnd)
    {
        const_cast<SendTcpSocketBase*> (this)->m_advWnd = w;
    }
    if (scale)
    {
        w >>= m_rcvWindShift;
    }
    if (w > m_maxWinSize)
    {
        w = m_maxWinSize;
        NS_LOG_WARN ("Adv window size truncated to " << m_maxWinSize << "; possibly to avoid overflow of the 16-bit integer");
    }
    NS_LOG_LOGIC ("Returning AdvertisedWindowSize of " << static_cast<uint16_t> (w));
    return static_cast<uint16_t> (w);
}

// Receipt of new packet, put into Rx buffer
void
SendTcpSocketBase::ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader)
{
    NS_LOG_FUNCTION (this << tcpHeader);
    NS_LOG_DEBUG ("Data segment, seq=" << tcpHeader.GetSequenceNumber () <<
                  " pkt size=" << p->GetSize () );

    // Put into Rx buffer
    SequenceNumber32 expectedSeq = m_tcb->m_rxBuffer->NextRxSequence ();
    if (!m_tcb->m_rxBuffer->Add (p, tcpHeader))
    { // Insert failed: No data or RX buffer full
        if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
        {
          SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
          NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_SENDING_ECE");
          m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
        }
        else
        {
            SendEmptyPacket (TcpHeader::ACK);
        }
      return;
    }
    // Notify app to receive if necessary
    if (expectedSeq < m_tcb->m_rxBuffer->NextRxSequence ())
    { // NextRxSeq advanced, we have something to send to the app
        if (!m_shutdownRecv)
        {
            NotifyDataRecv ();
        }
        // Handle exceptions
        if (m_closeNotified)
        {
            NS_LOG_WARN ("Why TCP " << this << " got data after close notification?");
        }
        // If we received FIN before and now completed all "holes" in rx buffer,
        // invoke peer close procedure
        if (m_tcb->m_rxBuffer->Finished () && (tcpHeader.GetFlags () & TcpHeader::FIN) == 0)
        {
            DoPeerClose ();
            return;
        }
    }
    // Now send a new ACK packet acknowledging all received and delivered data
    if (m_tcb->m_rxBuffer->Size () > m_tcb->m_rxBuffer->Available () || m_tcb->m_rxBuffer->NextRxSequence () > expectedSeq + p->GetSize ())
    {   // A gap exists in the buffer, or we filled a gap: Always ACK
        m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_NON_DELAYED_ACK);
        if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
        {
            SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
            NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_SENDING_ECE");
            m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
        }
        else
        {
            SendEmptyPacket (TcpHeader::ACK);
        }
    }
    else
    {   // In-sequence packet: ACK if delayed ack count allows
        if (++m_delAckCount >= m_delAckMaxCount)
        {
            m_delAckEvent.Cancel ();
            m_delAckCount = 0;
            m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_NON_DELAYED_ACK);
            if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
            {
                NS_LOG_DEBUG("Congestion algo " << m_congestionControl->GetName ());
                SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
                NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_SENDING_ECE");
                m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
            }
            else
            {
                SendEmptyPacket (TcpHeader::ACK);
            }
        }
        else if (!m_delAckEvent.IsExpired ())
        {
            m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_DELAYED_ACK);
        }
        else if (m_delAckEvent.IsExpired ())
        {
            m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_DELAYED_ACK);
            m_delAckEvent = Simulator::Schedule (m_delAckTimeout,
                                                &SendTcpSocketBase::DelAckTimeout, this);
            NS_LOG_LOGIC (this << " scheduled delayed ACK at " <<
                          (Simulator::Now () + Simulator::GetDelayLeft (m_delAckEvent)).GetSeconds ());
        }
    }
}

/**
 * \brief Estimate the RTT
 *
 * Called by ForwardUp() to estimate RTT.
 *
 * \param tcpHeader TCP header for the incoming packet
 */
void
SendTcpSocketBase::EstimateRtt (const TcpHeader& tcpHeader)
{
    SequenceNumber32 ackSeq = tcpHeader.GetAckNumber ();
    Time m = Time (0.0);

    // An ack has been received, calculate rtt and log this measurement
    // Note we use a linear search (O(n)) for this since for the common
    // case the ack'ed packet will be at the head of the list
    if (!m_history.empty ())
    {
        RttHistory& h = m_history.front ();
        if (!h.retx && ackSeq >= (h.seq + SequenceNumber32 (h.count)))
        {   // Ok to use this sample
            if (m_timestampEnabled && tcpHeader.HasOption (TcpOption::TS))
            {
                Ptr<const TcpOptionTS> ts;
                ts = DynamicCast<const TcpOptionTS> (tcpHeader.GetOption (TcpOption::TS));
                m = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetEcho ());
            }
            else
            {
                m = Simulator::Now () - h.time; // Elapsed time
            }
        }
    }

    // Now delete all ack history with seq <= ack
    while (!m_history.empty ())
    {
        RttHistory& h = m_history.front ();
        if ((h.seq + SequenceNumber32 (h.count)) > ackSeq)
        {
            break;                                                              // Done removing
        }
        m_history.pop_front (); // Remove
    }

    if (!m.IsZero ())
    {
        m_rateOps->UpdateRtt(m);
        m_rtt->Measurement (m);                // Log the measurement
        // RFC 6298, clause 2.4
        m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);
        m_tcb->m_lastRtt = m_rtt->GetEstimate ();
        m_tcb->m_minRtt = std::min (m_tcb->m_lastRtt.Get (), m_tcb->m_minRtt);
        NS_LOG_INFO (this << m_tcb->m_lastRtt << m_tcb->m_minRtt);
    }
}

// Called by the ReceivedAck() when new ACK received and by ProcessSynRcvd()
// when the three-way handshake completed. This cancels retransmission timer
// and advances Tx window
void
SendTcpSocketBase::NewAck (SequenceNumber32 const& ack, bool resetRTO)
{
    NS_LOG_FUNCTION (this << ack);

    // Reset the data retransmission count. We got a new ACK!
    m_dataRetrCount = m_dataRetries;

    if (m_state != SYN_RCVD && resetRTO)
    {   // Set RTO unless the ACK is received in SYN_RCVD state
        NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                      (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
        m_retxEvent.Cancel ();
        // On receiving a "New" ack we restart retransmission timer .. RFC 6298
        // RFC 6298, clause 2.4
        m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);

        NS_LOG_LOGIC (this << " Schedule ReTxTimeout at time " <<
                      Simulator::Now ().GetSeconds () << " to expire at time " <<
                      (Simulator::Now () + m_rto.Get ()).GetSeconds ());
        m_retxEvent = Simulator::Schedule (m_rto, &SendTcpSocketBase::ReTxTimeout, this);
    }

    // Note the highest ACK and tell app to send more
    NS_LOG_LOGIC ("TCP " << this << " NewAck " << ack <<
                  " numberAck " << (ack - m_txBuffer->HeadSequence ())); // Number bytes ack'ed

    if (GetTxAvailable () > 0)
    {
        NotifySend (GetTxAvailable ());
    }
    if (ack > m_tcb->m_nextTxSequence)
    {
        m_tcb->m_nextTxSequence = ack; // If advanced
    }
    if (m_txBuffer->Size () == 0 && m_state != FIN_WAIT_1 && m_state != CLOSING)
    {   // No retransmit timer if no data to retransmit
        NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                      (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
        m_retxEvent.Cancel ();
    }
}

// Retransmit timeout
void
SendTcpSocketBase::ReTxTimeout ()
{
    NS_LOG_FUNCTION (this);
    NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
    // If erroneous timeout in closed/timed-wait state, just return
    if (m_state == CLOSED || m_state == TIME_WAIT)
    {
        return;
    }

    if (m_state == SYN_SENT)
    {
        NS_ASSERT (m_synCount > 0);
        if (m_tcb->m_useEcn == TcpSocketState::On)
        {
            SendEmptyPacket (TcpHeader::SYN | TcpHeader::ECE | TcpHeader::CWR);
        }
        else
        {
            SendEmptyPacket (TcpHeader::SYN);
        }
        return;
    }

    // Retransmit non-data packet: Only if in FIN_WAIT_1 or CLOSING state
    if (m_txBuffer->Size () == 0)
    {
        if (m_state == FIN_WAIT_1 || m_state == CLOSING)
        {   // Must have lost FIN, re-send
            SendEmptyPacket (TcpHeader::FIN);
        }
        return;
    }

    NS_LOG_DEBUG ("Checking if Connection is Established");
    // If all data are received (non-closing socket and nothing to send), just return
    if (m_state <= ESTABLISHED && m_txBuffer->HeadSequence () >= m_tcb->m_highTxMark && m_txBuffer->Size () == 0)
    {
        NS_LOG_DEBUG ("Already Sent full data" << m_txBuffer->HeadSequence () << " " << m_tcb->m_highTxMark);
        return;
    }

    if (m_dataRetrCount == 0)
    {
        NS_LOG_INFO ("No more data retries available. Dropping connection");
        NotifyErrorClose ();
        DeallocateEndPoint ();
        return;
    }
    else
    {
        --m_dataRetrCount;
    }

    uint32_t inFlightBeforeRto = BytesInFlight ();
    bool resetSack = !m_sackEnabled; // Reset SACK information if SACK is not enabled.
                                    // The information in the TcpTxBuffer is guessed, in this case.

    // Reset dupAckCount
    m_dupAckCount = 0;
    if (!m_sackEnabled)
    {
        m_txBuffer->ResetRenoSack ();
    }

    // From RFC 6675, Section 5.1
    // [RFC2018] suggests that a TCP sender SHOULD expunge the SACK
    // information gathered from a receiver upon a retransmission timeout
    // (RTO) "since the timeout might indicate that the data receiver has
    // reneged."  Additionally, a TCP sender MUST "ignore prior SACK
    // information in determining which data to retransmit."
    // It has been suggested that, as long as robust tests for
    // reneging are present, an implementation can retain and use SACK
    // information across a timeout event [Errata1610].
    // The head of the sent list will not be marked as sacked, therefore
    // will be retransmitted, if the receiver renegotiate the SACK blocks
    // that we received.
    m_txBuffer->SetSentListLost (resetSack);

    // From RFC 6675, Section 5.1
    // If an RTO occurs during loss recovery as specified in this document,
    // RecoveryPoint MUST be set to HighData.  Further, the new value of
    // RecoveryPoint MUST be preserved and the loss recovery algorithm
    // outlined in this document MUST be terminated.
    m_recover = m_tcb->m_highTxMark;
    m_recoverActive = true;

    // RFC 6298, clause 2.5, double the timer
    Time doubledRto = m_rto + m_rto;
    m_rto = Min (doubledRto, Time::FromDouble (60,  Time::S));

    // Empty RTT history
    m_history.clear ();

    // Please don't reset highTxMark, it is used for retransmission detection

    // When a TCP sender detects segment loss using the retransmission timer
    // and the given segment has not yet been resent by way of the
    // retransmission timer, decrease ssThresh
    if (m_tcb->m_congState != TcpSocketState::CA_LOSS || !m_txBuffer->IsHeadRetransmitted ())
    {
        m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb, inFlightBeforeRto);
    }

    // Cwnd set to 1 MSS
    m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_LOSS);
    m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_LOSS);
    m_tcb->m_congState = TcpSocketState::CA_LOSS;
    m_tcb->m_cWnd = m_tcb->m_segmentSize;
    m_tcb->m_cWndInfl = m_tcb->m_cWnd;

    m_pacingTimer.Cancel ();

    NS_LOG_DEBUG ("RTO. Reset cwnd to " <<  m_tcb->m_cWnd << ", ssthresh to " <<
                  m_tcb->m_ssThresh << ", restart from seqnum " <<
                  m_txBuffer->HeadSequence () << " doubled rto to " <<
                  m_rto.Get ().GetSeconds () << " s");

    NS_ASSERT_MSG (BytesInFlight () == 0, "There are some bytes in flight after an RTO: " <<
                  BytesInFlight ());

    SendPendingData (m_connected);

    NS_ASSERT_MSG (BytesInFlight () <= m_tcb->m_segmentSize,
                  "In flight (" << BytesInFlight () <<
                  ") there is more than one segment (" << m_tcb->m_segmentSize << ")");
}

void
SendTcpSocketBase::DelAckTimeout (void)
{
    m_delAckCount = 0;
    m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_DELAYED_ACK);
    if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
    {
        SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
        m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
    }
    else
    {
        SendEmptyPacket (TcpHeader::ACK);
    }
}

void
SendTcpSocketBase::LastAckTimeout (void)
{
    NS_LOG_FUNCTION (this);

    m_lastAckEvent.Cancel ();
    if (m_state == LAST_ACK)
    {
        if (m_dataRetrCount == 0)
        {
            NS_LOG_INFO ("LAST-ACK: No more data retries available. Dropping connection");
            NotifyErrorClose ();
            DeallocateEndPoint ();
            return;
        }
        m_dataRetrCount--;
        SendEmptyPacket (TcpHeader::FIN | TcpHeader::ACK);
        NS_LOG_LOGIC ("SendTcpSocketBase " << this << " rescheduling LATO1");
        Time lastRto = m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4);
        m_lastAckEvent = Simulator::Schedule (lastRto, &SendTcpSocketBase::LastAckTimeout, this);
    }
}

// Send 1-byte data to probe for the window size at the receiver when
// the local knowledge tells that the receiver has zero window size
// C.f.: RFC793 p.42, RFC1112 sec.4.2.2.17
void
SendTcpSocketBase::PersistTimeout ()
{
    NS_LOG_LOGIC ("PersistTimeout expired at " << Simulator::Now ().GetSeconds ());
    m_persistTimeout = std::min (Seconds (60), Time (2 * m_persistTimeout)); // max persist timeout = 60s
    Ptr<Packet> p = m_txBuffer->CopyFromSequence (1, m_tcb->m_nextTxSequence)->GetPacketCopy ();
    m_txBuffer->ResetLastSegmentSent ();
    TcpHeader tcpHeader;
    tcpHeader.SetSequenceNumber (m_tcb->m_nextTxSequence);
    tcpHeader.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
    tcpHeader.SetWindowSize (AdvertisedWindowSize ());
    if (m_endPoint != nullptr)
    {
        tcpHeader.SetSourcePort (m_endPoint->GetLocalPort ());
        tcpHeader.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
    else
    {
        tcpHeader.SetSourcePort (m_endPoint6->GetLocalPort ());
        tcpHeader.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
    AddOptions (tcpHeader);
    //Send a packet tag for setting ECT bits in IP header
    if (m_tcb->m_ecnState != TcpSocketState::ECN_DISABLED)
    {
        SocketIpTosTag ipTosTag;
        ipTosTag.SetTos (MarkEcnCodePoint (0, m_tcb->m_ectCodePoint));
        p->AddPacketTag (ipTosTag);

        SocketIpv6TclassTag ipTclassTag;
        ipTclassTag.SetTclass (MarkEcnCodePoint (0, m_tcb->m_ectCodePoint));
        p->AddPacketTag (ipTclassTag);
    }
    m_txTrace (p, tcpHeader, this);

    if (m_endPoint != nullptr)
    {
        m_tcp->SendPacket (p, tcpHeader, m_endPoint->GetLocalAddress (),
                          m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
    else
    {
        m_tcp->SendPacket (p, tcpHeader, m_endPoint6->GetLocalAddress (),
                          m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }

    NS_LOG_LOGIC ("Schedule persist timeout at time "
                  << Simulator::Now ().GetSeconds () << " to expire at time "
                  << (Simulator::Now () + m_persistTimeout).GetSeconds ());
    m_persistEvent = Simulator::Schedule (m_persistTimeout, &SendTcpSocketBase::PersistTimeout, this);
}


/* Below are the attribute get/set functions */

void
SendTcpSocketBase::SetSndBufSize (uint32_t size)
{
    NS_LOG_FUNCTION (this << size);
    m_txBuffer->SetMaxBufferSize (size);
}

uint32_t
SendTcpSocketBase::GetSndBufSize (void) const
{
    return m_txBuffer->MaxBufferSize ();
}

void
SendTcpSocketBase::SetRcvBufSize (uint32_t size)
{
    NS_LOG_FUNCTION (this << size);
    uint32_t oldSize = GetRcvBufSize ();

    m_tcb->m_rxBuffer->SetMaxBufferSize (size);

    /* The size has (manually) increased. Actively inform the other end to prevent
    * stale zero-window states.
    */
    if (oldSize < size && m_connected)
    {
        if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
        {
            SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
            NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_SENDING_ECE");
            m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
        }
        else
        {
            SendEmptyPacket (TcpHeader::ACK);
        }
    }
}

uint32_t
SendTcpSocketBase::GetRcvBufSize (void) const
{
    return m_tcb->m_rxBuffer->MaxBufferSize ();
}

void
SendTcpSocketBase::SetSegSize (uint32_t size)
{
    NS_LOG_FUNCTION (this << size);
    m_tcb->m_segmentSize = size;
    m_txBuffer->SetSegmentSize (size);

    NS_ABORT_MSG_UNLESS (m_state == CLOSED, "Cannot change segment size dynamically.");
}

uint32_t
SendTcpSocketBase::GetSegSize (void) const
{
    return m_tcb->m_segmentSize;
}

void
SendTcpSocketBase::SetConnTimeout (Time timeout)
{
    NS_LOG_FUNCTION (this << timeout);
    m_cnTimeout = timeout;
}

Time
SendTcpSocketBase::GetConnTimeout (void) const
{
    return m_cnTimeout;
}

void
SendTcpSocketBase::SetSynRetries (uint32_t count)
{
    NS_LOG_FUNCTION (this << count);
    m_synRetries = count;
}

uint32_t
SendTcpSocketBase::GetSynRetries (void) const
{
    return m_synRetries;
}

void
SendTcpSocketBase::SetDataRetries (uint32_t retries)
{
    NS_LOG_FUNCTION (this << retries);
    m_dataRetries = retries;
}

uint32_t
SendTcpSocketBase::GetDataRetries (void) const
{
    NS_LOG_FUNCTION (this);
    return m_dataRetries;
}

void
SendTcpSocketBase::SetDelAckTimeout (Time timeout)
{
    NS_LOG_FUNCTION (this << timeout);
    m_delAckTimeout = timeout;
}

Time
SendTcpSocketBase::GetDelAckTimeout (void) const
{
    return m_delAckTimeout;
}

void
SendTcpSocketBase::SetDelAckMaxCount (uint32_t count)
{
    NS_LOG_FUNCTION (this << count);
    m_delAckMaxCount = count;
}

uint32_t
SendTcpSocketBase::GetDelAckMaxCount (void) const
{
    return m_delAckMaxCount;
}

void
SendTcpSocketBase::SetTcpNoDelay (bool noDelay)
{
    NS_LOG_FUNCTION (this << noDelay);
    m_noDelay = noDelay;
}

bool
SendTcpSocketBase::GetTcpNoDelay (void) const
{
    return m_noDelay;
}

void
SendTcpSocketBase::SetPersistTimeout (Time timeout)
{
    NS_LOG_FUNCTION (this << timeout);
    m_persistTimeout = timeout;
}

Time
SendTcpSocketBase::GetPersistTimeout (void) const
{
    return m_persistTimeout;
}

bool
SendTcpSocketBase::SetAllowBroadcast (bool allowBroadcast)
{
    // Broadcast is not implemented. Return true only if allowBroadcast==false
    return (!allowBroadcast);
}

bool
SendTcpSocketBase::GetAllowBroadcast (void) const
{
    return false;
}

Ptr<TcpSocketBase>
SendTcpSocketBase::Fork (void)
{
    return CopyObject<SendTcpSocketBase> (this);
}


} // namespace ns3
