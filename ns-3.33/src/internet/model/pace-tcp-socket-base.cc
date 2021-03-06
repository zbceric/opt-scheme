// /*
//  * @Author: Zhang Bochun
//  * @Date: 2022-04-22 10:27:58
//  * @LastEditTime: 2022-05-09 21:02:17
//  * @LastEditors: Zhang Bochun
//  * @Description: Receiver Pace ACK Tcp Socket Base
//  * @FilePath: /ns-3.33/src/internet/model/pace-tcp-socket-base.cc
//  */

// #include "ns3/abort.h"
// #include "ns3/node.h"
// #include "ns3/inet-socket-address.h"
// #include "ns3/inet6-socket-address.h"
// #include "ns3/log.h"
// #include "ns3/ipv4.h"
// #include "ns3/ipv6.h"
// #include "ns3/ipv4-interface-address.h"
// #include "ns3/ipv4-route.h"
// #include "ns3/ipv6-route.h"
// #include "ns3/ipv4-routing-protocol.h"
// #include "ns3/ipv6-routing-protocol.h"
// #include "ns3/simulation-singleton.h"
// #include "ns3/simulator.h"
// #include "ns3/packet.h"
// #include "ns3/uinteger.h"
// #include "ns3/double.h"
// #include "ns3/pointer.h"
// #include "ns3/trace-source-accessor.h"
// #include "ns3/data-rate.h"
// #include "ns3/object.h"
// #include "ns3/tcp-socket-base.h"
// #include "ns3/tcp-l4-protocol.h"
// #include "ns3/ipv4-end-point.h"
// #include "ns3/ipv6-end-point.h"
// #include "ns3/ipv6-l3-protocol.h"
// #include "ns3/tcp-tx-buffer.h"
// #include "ns3/tcp-rx-buffer.h"
// #include "ns3/rtt-estimator.h"
// #include "ns3/tcp-header.h"
// #include "ns3/tcp-option-winscale.h"
// #include "ns3/tcp-option-ts.h"
// #include "ns3/tcp-option-sack-permitted.h"
// #include "ns3/tcp-option-sack.h"
// #include "ns3/tcp-congestion-ops.h"
// #include "ns3/tcp-recovery-ops.h"
// #include "ns3/tcp-rate-ops.h"
// #include "pace-tcp-socket-base.h"
// #include "simulation-tcp-socket.h"

// #include <unistd.h>
// #include <memory.h>
// #include <math.h>
// #include <algorithm>

// namespace ns3 {

// NS_LOG_COMPONENT_DEFINE ("PaceTcpSocketBase");

// NS_OBJECT_ENSURE_REGISTERED (PaceTcpSocketBase);

// TypeId
// PaceTcpSocketBase::GetTypeId (void)
// {
//     static TypeId tid = TypeId ("ns3::PaceTcpSocketBase")
//         .SetParent<TcpSocketBase> ()
//         .SetGroupName ("Internet")
//         .AddConstructor<PaceTcpSocketBase> ()

//         // ????????????
//         .AddTraceSource ("SendRTT",
//                          "Last PeerRTT sample",
//                          MakeTraceSourceAccessor (&PaceTcpSocketBase::m_lastSendRttTrace),
//                          "ns3::TracedValueCallback::Time")
//         .AddTraceSource ("ReceRTT",
//                          "Last PeerRTT sample",
//                          MakeTraceSourceAccessor (&PaceTcpSocketBase::m_lastReceRttTrace),
//                          "ns3::TracedValueCallback::Time") 
//         .AddTraceSource ("SendCwnd",
//                          "Last PeerRTT sample",
//                          MakeTraceSourceAccessor (&PaceTcpSocketBase::m_SendCwndTrace),
//                          "ns3::TracedValueCallback::Time")
//         .AddTraceSource ("SendCongState",
//                          "TCP Congestion machine state",
//                          MakeTraceSourceAccessor (&PaceTcpSocketBase::m_sendCongStateTrace),
//                          "ns3::TcpSocketState::TcpCongStatesTracedValueCallback")
//         .AddTraceSource ("SimTx",
//                          "Send tcp packet to IP protocol",
//                          MakeTraceSourceAccessor (&PaceTcpSocketBase::m_simTxTrace),
//                          "ns3::TcpSocketBase::TcpTxRxTracedCallback")
//     ;
//     return tid;
// }

// TypeId
// PaceTcpSocketBase::GetInstanceTypeId () const
// {
//     return PaceTcpSocketBase::GetTypeId ();
// }

// PaceTcpSocketBase::PaceTcpSocketBase (void)
//   : TcpSocketBase (),
//     m_maxBwFilter (CYCLE, DataRate(0), 0)
// {
//     std::cout << GetTypeId () << std::endl;
// }

// PaceTcpSocketBase::PaceTcpSocketBase (const PaceTcpSocketBase& sock)
//   : TcpSocketBase (sock),
//     m_maxBwFilter (CYCLE, DataRate(0), 0)
// {
//     std::cout << GetTypeId () << std::endl;
// }
  

// PaceTcpSocketBase::~PaceTcpSocketBase (void)
// {
// }



// /* Associate a node with this TCP socket */
// void
// PaceTcpSocketBase::SetNode (Ptr<Node> node)
// {
//     m_node = node;
// }

// /* Associate the L4 protocol (e.g. mux/demux) with this socket */
// void
// PaceTcpSocketBase::SetTcp (Ptr<TcpL4Protocol> tcp)
// {
//     m_tcp = tcp;
// }

// /* Set an RTT estimator with this socket */
// void
// PaceTcpSocketBase::SetRtt (Ptr<RttEstimator> rtt)
// {
//     m_rtt = rtt;
// }

// /* Inherit from Socket class: Returns error code */
// enum Socket::SocketErrno
// PaceTcpSocketBase::GetErrno (void) const
// {
//     return m_errno;
// }

// /* Inherit from Socket class: Returns socket type, NS3_SOCK_STREAM */
// enum Socket::SocketType
// PaceTcpSocketBase::GetSocketType (void) const
// {
//     return NS3_SOCK_STREAM;
// }

// /* Inherit from Socket class: Returns associated node */
// Ptr<Node>
// PaceTcpSocketBase::GetNode (void) const
// {
//     return m_node;
// }

// /* Inherit from Socket class: Bind socket to an end-point in TcpL4Protocol */
// int
// PaceTcpSocketBase::Bind (void)
// {
//     NS_LOG_FUNCTION (this);
//     m_endPoint = m_tcp->Allocate ();
//     if (0 == m_endPoint)
//     {
//         m_errno = ERROR_ADDRNOTAVAIL;
//         return -1;
//     }

//     m_tcp->AddSocket (this);

//   return SetupCallback ();
// }

// int
// PaceTcpSocketBase::Bind6 (void)
// {
//     NS_LOG_FUNCTION (this);
//     m_endPoint6 = m_tcp->Allocate6 ();
//     if (0 == m_endPoint6)
//     {
//         m_errno = ERROR_ADDRNOTAVAIL;
//         return -1;
//     }

//     m_tcp->AddSocket (this);

//     return SetupCallback ();
// }

// /* Inherit from Socket class: Bind socket (with specific address) to an end-point in TcpL4Protocol */
// int
// PaceTcpSocketBase::Bind (const Address &address)
// {
//     NS_LOG_FUNCTION (this << address);
//     if (InetSocketAddress::IsMatchingType (address))
//     {
//         InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
//         Ipv4Address ipv4 = transport.GetIpv4 ();
//         uint16_t port = transport.GetPort ();
//         SetIpTos (transport.GetTos ());
//         if (ipv4 == Ipv4Address::GetAny () && port == 0)
//         {
//             m_endPoint = m_tcp->Allocate ();
//         }
//         else if (ipv4 == Ipv4Address::GetAny () && port != 0)
//         {
//             m_endPoint = m_tcp->Allocate (GetBoundNetDevice (), port);
//         }
//         else if (ipv4 != Ipv4Address::GetAny () && port == 0)
//         {
//             m_endPoint = m_tcp->Allocate (ipv4);
//         }
//         else if (ipv4 != Ipv4Address::GetAny () && port != 0)
//         {
//             m_endPoint = m_tcp->Allocate (GetBoundNetDevice (), ipv4, port);
//         }
//         if (0 == m_endPoint)
//         {
//             m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
//             return -1;
//         }
//     }
//     else if (Inet6SocketAddress::IsMatchingType (address))
//     {
//         Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
//         Ipv6Address ipv6 = transport.GetIpv6 ();
//         uint16_t port = transport.GetPort ();
//         if (ipv6 == Ipv6Address::GetAny () && port == 0)
//         {
//             m_endPoint6 = m_tcp->Allocate6 ();
//         }
//         else if (ipv6 == Ipv6Address::GetAny () && port != 0)
//         {
//             m_endPoint6 = m_tcp->Allocate6 (GetBoundNetDevice (), port);
//         }
//         else if (ipv6 != Ipv6Address::GetAny () && port == 0)
//         {
//             m_endPoint6 = m_tcp->Allocate6 (ipv6);
//         }
//         else if (ipv6 != Ipv6Address::GetAny () && port != 0)
//         {
//             m_endPoint6 = m_tcp->Allocate6 (GetBoundNetDevice (), ipv6, port);
//         }
//         if (0 == m_endPoint6)
//         {
//             m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
//             return -1;
//         }
//     }
//     else
//     {
//         m_errno = ERROR_INVAL;
//         return -1;
//     }

//     m_tcp->AddSocket (this);

//     NS_LOG_LOGIC ("PaceTcpSocketBase " << this << " got an endpoint: " << m_endPoint);

//     return SetupCallback ();
// }

// void
// PaceTcpSocketBase::SetInitialSSThresh (uint32_t threshold)
// {
//     NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || threshold == m_tcb->m_initialSsThresh,
//                           "PaceTcpSocketBase::SetSSThresh() cannot change initial ssThresh after connection started.");

//     m_tcb->m_initialSsThresh = threshold;
// }

// uint32_t
// PaceTcpSocketBase::GetInitialSSThresh (void) const
// {
//     return m_tcb->m_initialSsThresh;
// }

// void
// PaceTcpSocketBase::SetInitialCwnd (uint32_t cwnd)
// {
//     NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || cwnd == m_tcb->m_initialCWnd,
//                           "PaceTcpSocketBase::SetInitialCwnd() cannot change initial cwnd after connection started.");

//     m_tcb->m_initialCWnd = cwnd;
// }

// uint32_t
// PaceTcpSocketBase::GetInitialCwnd (void) const
// {
//     return m_tcb->m_initialCWnd;
// }

// /* Inherit from Socket class: Initiate connection to a remote address:port */
// int
// PaceTcpSocketBase::Connect (const Address & address)
// {
//     NS_LOG_FUNCTION (this << address);

//     // If haven't do so, Bind() this socket first
//     if (InetSocketAddress::IsMatchingType (address))
//     {
//         if (m_endPoint == nullptr)
//         {
//             if (Bind () == -1)
//             {
//                 NS_ASSERT (m_endPoint == nullptr);
//                 return -1; // Bind() failed
//             }
//             NS_ASSERT (m_endPoint != nullptr);
//         }
//         InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
//         m_endPoint->SetPeer (transport.GetIpv4 (), transport.GetPort ());
//         SetIpTos (transport.GetTos ());
//         m_endPoint6 = nullptr;

//         // Get the appropriate local address and port number from the routing protocol and set up endpoint
//         if (SetupEndpoint () != 0)
//         {
//             NS_LOG_ERROR ("Route to destination does not exist ?!");
//             return -1;
//         }
//     }
//     else if (Inet6SocketAddress::IsMatchingType (address))
//     {
//         // If we are operating on a v4-mapped address, translate the address to
//         // a v4 address and re-call this function
//         Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
//         Ipv6Address v6Addr = transport.GetIpv6 ();
//         if (v6Addr.IsIpv4MappedAddress () == true)
//         {
//             Ipv4Address v4Addr = v6Addr.GetIpv4MappedAddress ();
//             return Connect (InetSocketAddress (v4Addr, transport.GetPort ()));
//         }

//         if (m_endPoint6 == nullptr)
//         {
//             if (Bind6 () == -1)
//             {
//                 NS_ASSERT (m_endPoint6 == nullptr);
//                 return -1; // Bind() failed
//             }
//             NS_ASSERT (m_endPoint6 != nullptr);
//         }
//         m_endPoint6->SetPeer (v6Addr, transport.GetPort ());
//         m_endPoint = nullptr;

//         // Get the appropriate local address and port number from the routing protocol and set up endpoint
//         if (SetupEndpoint6 () != 0)
//         {
//             NS_LOG_ERROR ("Route to destination does not exist ?!");
//             return -1;
//         }
//     }
//     else
//     {
//         m_errno = ERROR_INVAL;
//         return -1;
//     }

//     // Re-initialize parameters in case this socket is being reused after CLOSE
//     m_rtt->Reset ();
//     m_synCount = m_synRetries;
//     m_dataRetrCount = m_dataRetries;

//     // DoConnect() will do state-checking and send a SYN packet
//     return DoConnect ();
// }

// /* Inherit from Socket class: Listen on the endpoint for an incoming connection */
// int
// PaceTcpSocketBase::Listen (void)
// {
//     NS_LOG_FUNCTION (this);

//     // Linux quits EINVAL if we're not in CLOSED state, so match what they do
//     if (m_state != CLOSED)
//     {
//         m_errno = ERROR_INVAL;
//         return -1;
//     }
//     // In other cases, set the state to LISTEN and done
//     NS_LOG_DEBUG ("CLOSED -> LISTEN");
//     m_state = LISTEN;
//     return 0;
// }

// /* Inherit from Socket class: Kill this socket and signal the peer (if any) */
// int
// PaceTcpSocketBase::Close (void)
// {
//     NS_LOG_FUNCTION (this);
//     /// \internal
//     /// First we check to see if there is any unread rx data.
//     /// \bugid{426} claims we should send reset in this case.
//     if (m_tcb->m_rxBuffer->Size () != 0)
//     {
//         NS_LOG_WARN ("Socket " << this << " << unread rx data during close.  Sending reset." <<
//                     "This is probably due to a bad sink application; check its code");
//         SendRST ();
//         return 0;
//     }

//     if (m_txBuffer->SizeFromSequence (m_tcb->m_nextTxSequence) > 0)
//     { // App close with pending data must wait until all data transmitted
//         if (m_closeOnEmpty == false)
//         {
//             m_closeOnEmpty = true;
//             NS_LOG_INFO ("Socket " << this << " deferring close, state " << TcpStateName[m_state]);
//         }
//         return 0;
//     }
//     return DoClose ();
// }

// /* Inherit from Socket class: Signal a termination of send */
// int
// PaceTcpSocketBase::ShutdownSend (void)
// {
//     NS_LOG_FUNCTION (this);

//     //this prevents data from being added to the buffer
//     m_shutdownSend = true;
//     m_closeOnEmpty = true;
//     //if buffer is already empty, send a fin now
//     //otherwise fin will go when buffer empties.
//     if (m_txBuffer->Size () == 0)
//     {
//         if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
//         {
//             NS_LOG_INFO ("Empty tx buffer, send fin");
//             SendEmptyPacket (TcpHeader::FIN);

//             if (m_state == ESTABLISHED)
//             { // On active close: I am the first one to send FIN
//                 NS_LOG_DEBUG ("ESTABLISHED -> FIN_WAIT_1");
//                 m_state = FIN_WAIT_1;
//             }
//             else
//             { // On passive close: Peer sent me FIN already
//                 NS_LOG_DEBUG ("CLOSE_WAIT -> LAST_ACK");
//                 m_state = LAST_ACK;
//             }
//         }
//     }

//     return 0;
// }

// /* Inherit from Socket class: Signal a termination of receive */
// int
// PaceTcpSocketBase::ShutdownRecv (void)
// {
//     NS_LOG_FUNCTION (this);
//     m_shutdownRecv = true;
//     return 0;
// }

// /* Inherit from Socket class: Send a packet. Parameter flags is not used.
//     Packet has no TCP header. Invoked by upper-layer application */
// int
// PaceTcpSocketBase::Send (Ptr<Packet> p, uint32_t flags)
// {
//     NS_LOG_FUNCTION (this << p);
//     NS_ABORT_MSG_IF (flags, "use of flags is not supported in PaceTcpSocketBase::Send()");
//     if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
//     {
//         // Store the packet into Tx buffer
//         if (!m_txBuffer->Add (p))
//         { // TxBuffer overflow, send failed
//             m_errno = ERROR_MSGSIZE;
//             return -1;
//         }
//         if (m_shutdownSend)
//         {
//             m_errno = ERROR_SHUTDOWN;
//             return -1;
//         }

//         m_rateOps->CalculateAppLimited(m_tcb->m_cWnd, m_tcb->m_bytesInFlight, m_tcb->m_segmentSize,
//                                        m_txBuffer->TailSequence (), m_tcb->m_nextTxSequence,
//                                        m_txBuffer->GetLost (), m_txBuffer->GetRetransmitsCount ());

//         // Submit the data to lower layers
//         NS_LOG_LOGIC ("txBufSize=" << m_txBuffer->Size () << " state " << TcpStateName[m_state]);
//         if ((m_state == ESTABLISHED || m_state == CLOSE_WAIT) && AvailableWindow () > 0)
//         {   // Try to send the data out: Add a little step to allow the application
//             // to fill the buffer
//             if (!m_sendPendingDataEvent.IsRunning ())
//             {
//                 m_sendPendingDataEvent = Simulator::Schedule (TimeStep (1),
//                                                               &PaceTcpSocketBase::SendPendingData,
//                                                               this, m_connected);
//             }
//         }
//         return p->GetSize ();
//     }
//     else
//     {   // Connection not established yet
//         m_errno = ERROR_NOTCONN;
//         return -1; // Send failure
//     }
// }

// /* Inherit from Socket class: In PaceTcpSocketBase, it is same as Send() call */
// int
// PaceTcpSocketBase::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
// {
//     NS_UNUSED (address);
//     return Send (p, flags); // SendTo() and Send() are the same
// }

// /* Inherit from Socket class: Return data to upper-layer application. Parameter flags
//    is not used. Data is returned as a packet of size no larger than maxSize */
// Ptr<Packet>
// PaceTcpSocketBase::Recv (uint32_t maxSize, uint32_t flags)
// {
//     NS_LOG_FUNCTION (this);
//     NS_ABORT_MSG_IF (flags, "use of flags is not supported in PaceTcpSocketBase::Recv()");
//     if (m_tcb->m_rxBuffer->Size () == 0 && m_state == CLOSE_WAIT)
//     {
//         return Create<Packet> (); // Send EOF on connection close
//     }
//     Ptr<Packet> outPacket = m_tcb->m_rxBuffer->Extract (maxSize);
//     return outPacket;
// }

// /* Inherit from Socket class: Recv and return the remote's address */
// Ptr<Packet>
// PaceTcpSocketBase::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
// {
//     NS_LOG_FUNCTION (this << maxSize << flags);
//     Ptr<Packet> packet = Recv (maxSize, flags);
//     // Null packet means no data to read, and an empty packet indicates EOF
//     if (packet != nullptr && packet->GetSize () != 0)
//     {
//         if (m_endPoint != nullptr)
//         {
//             fromAddress = InetSocketAddress (m_endPoint->GetPeerAddress (), m_endPoint->GetPeerPort ());
//         }
//         else if (m_endPoint6 != nullptr)
//         {
//             fromAddress = Inet6SocketAddress (m_endPoint6->GetPeerAddress (), m_endPoint6->GetPeerPort ());
//         }
//         else
//         {
//             fromAddress = InetSocketAddress (Ipv4Address::GetZero (), 0);
//         }
//     }
//     return packet;
// }

// /* Inherit from Socket class: Get the max number of bytes an app can send */
// uint32_t
// PaceTcpSocketBase::GetTxAvailable (void) const
// {
//     NS_LOG_FUNCTION (this);
//     return m_txBuffer->Available ();
// }

// /* Inherit from Socket class: Get the max number of bytes an app can read */
// uint32_t
// PaceTcpSocketBase::GetRxAvailable (void) const
// {
//     NS_LOG_FUNCTION (this);
//     return m_tcb->m_rxBuffer->Available ();
// }

// /* Inherit from Socket class: Return local address:port */
// int
// PaceTcpSocketBase::GetSockName (Address &address) const
// {
//     NS_LOG_FUNCTION (this);
//     if (m_endPoint != nullptr)
//     {
//         address = InetSocketAddress (m_endPoint->GetLocalAddress (), m_endPoint->GetLocalPort ());
//     }
//     else if (m_endPoint6 != nullptr)
//     {
//         address = Inet6SocketAddress (m_endPoint6->GetLocalAddress (), m_endPoint6->GetLocalPort ());
//     }
//     else
//     {   // It is possible to call this method on a socket without a name
//         // in which case, behavior is unspecified
//         // Should this return an InetSocketAddress or an Inet6SocketAddress?
//         address = InetSocketAddress (Ipv4Address::GetZero (), 0);
//     }
//     return 0;
// }

// int
// PaceTcpSocketBase::GetPeerName (Address &address) const
// {
//     NS_LOG_FUNCTION (this << address);

//     if (!m_endPoint && !m_endPoint6)
//     {
//         m_errno = ERROR_NOTCONN;
//         return -1;
//     }

//     if (m_endPoint)
//     {
//         address = InetSocketAddress (m_endPoint->GetPeerAddress (),
//                                     m_endPoint->GetPeerPort ());
//     }
//     else if (m_endPoint6)
//     {
//         address = Inet6SocketAddress (m_endPoint6->GetPeerAddress (),
//                                       m_endPoint6->GetPeerPort ());
//     }
//     else
//     {
//         NS_ASSERT (false);
//     }

//     return 0;
// }

// /* Inherit from Socket class: Bind this socket to the specified NetDevice */
// void
// PaceTcpSocketBase::BindToNetDevice (Ptr<NetDevice> netdevice)
// {
//     NS_LOG_FUNCTION (netdevice);
//     Socket::BindToNetDevice (netdevice); // Includes sanity check
//     if (m_endPoint != nullptr)
//     {
//         m_endPoint->BindToNetDevice (netdevice);
//     }

//     if (m_endPoint6 != nullptr)
//     {
//         m_endPoint6->BindToNetDevice (netdevice);
//     }

//   return;
// }





// void
// PaceTcpSocketBase::DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
//                             const Address &toAddress)
// {
//     // in case the packet still has a priority tag attached, remove it
//     SocketPriorityTag priorityTag;
//     packet->RemovePacketTag (priorityTag);

//     // Peel off TCP header
//     TcpHeader tcpHeader;
//     packet->RemoveHeader (tcpHeader);
//     SequenceNumber32 seq = tcpHeader.GetSequenceNumber ();

//     if (m_state == ESTABLISHED && !(tcpHeader.GetFlags () & TcpHeader::RST))
//     {
//         // Check if the sender has responded to ECN echo by reducing the Congestion Window
//         if (tcpHeader.GetFlags () & TcpHeader::CWR )
//         {
//             // Check if a packet with CE bit set is received. If there is no CE bit set, then change the state to ECN_IDLE to
//             // stop sending ECN Echo messages. If there is CE bit set, the packet should continue sending ECN Echo messages
//             //
//             if (m_tcb->m_ecnState != TcpSocketState::ECN_CE_RCVD)
//             {
//                 NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_IDLE");
//                 m_tcb->m_ecnState = TcpSocketState::ECN_IDLE;
//             }
//         }
//     }

//     m_rxTrace (packet, tcpHeader, this);

//     if (tcpHeader.GetFlags () & TcpHeader::SYN)
//     {
//         /* The window field in a segment where the SYN bit is set (i.e., a <SYN>
//         * or <SYN,ACK>) MUST NOT be scaled (from RFC 7323 page 9). But should be
//         * saved anyway..
//         */
//         m_rWnd = tcpHeader.GetWindowSize ();

//         if (tcpHeader.HasOption (TcpOption::WINSCALE) && m_winScalingEnabled)
//         {
//             ProcessOptionWScale (tcpHeader.GetOption (TcpOption::WINSCALE));
//         }
//         else
//         {
//             m_winScalingEnabled = false;
//         }

//         if (tcpHeader.HasOption (TcpOption::SACKPERMITTED) && m_sackEnabled)
//         {
//             ProcessOptionSackPermitted (tcpHeader.GetOption (TcpOption::SACKPERMITTED));
//         }
//         else
//         {
//             m_sackEnabled = false;
//             m_txBuffer->SetSackEnabled (false);
//         }

//         // When receiving a <SYN> or <SYN-ACK> we should adapt TS to the other end
//         if (tcpHeader.HasOption (TcpOption::TS) && m_timestampEnabled)
//         {
//             ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS),
//                                     tcpHeader.GetSequenceNumber ());
//         }
//         else
//         {
//             m_timestampEnabled = false;
//         }

//         // Initialize cWnd and ssThresh
//         m_tcb->m_cWnd = GetInitialCwnd () * GetSegSize ();
//         m_tcb->m_cWndInfl = m_tcb->m_cWnd;
//         m_tcb->m_ssThresh = GetInitialSSThresh ();

//         if (tcpHeader.GetFlags () & TcpHeader::ACK)
//         {
//             EstimateRtt (tcpHeader);
//             m_highRxAckMark = tcpHeader.GetAckNumber ();
//         }
//     }
//     else if (tcpHeader.GetFlags () & TcpHeader::ACK)
//     {
//         NS_ASSERT (!(tcpHeader.GetFlags () & TcpHeader::SYN));
//         if (m_timestampEnabled)
//         {
//             if (!tcpHeader.HasOption (TcpOption::TS))
//             {
//                 // Ignoring segment without TS, RFC 7323
//                 NS_LOG_LOGIC ("At state " << TcpStateName[m_state] <<
//                               " received packet of seq [" << seq <<
//                               ":" << seq + packet->GetSize () <<
//                               ") without TS option. Silently discard it");
//               return;
//             }
//             else
//             {
//                 ProcessOptionTimestamp (tcpHeader.GetOption (TcpOption::TS),
//                                         tcpHeader.GetSequenceNumber ());
//             }
//         }

//         EstimateRtt (tcpHeader);
//         UpdateWindowSize (tcpHeader);
//     }

//     // ???????????? Put into SimulationTcpSocket's txBuffer
//     if (packet->GetSize () > 0)
//     {
//         m_peerSocket->FillDataPacket (packet, tcpHeader);
//         m_currentMaxReceived = std::max (tcpHeader.GetSequenceNumber(), m_currentMaxReceived);
//     }

//     // ????????????????????? packet, ????????????, ????????? Gradient
//     CalculateBandwidth (packet);        // ????????? Round
//     CalculateGradient ();

//     // ?????? PacingRate
//     // UpdateParameter ();
//     UpdateAckPacingRate ();


//     if (m_rWnd.Get () == 0 && m_persistEvent.IsExpired ())
//     {   // Zero window: Enter persist state to send 1 byte to probe
//         NS_LOG_LOGIC (this << " Enter zerowindow persist state");
//         NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
//                       (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
//         m_retxEvent.Cancel ();
//         NS_LOG_LOGIC ("Schedule persist timeout at time " <<
//                       Simulator::Now ().GetSeconds () << " to expire at time " <<
//                       (Simulator::Now () + m_persistTimeout).GetSeconds ());
//         m_persistEvent = Simulator::Schedule (m_persistTimeout, &PaceTcpSocketBase::PersistTimeout, this);
//         NS_ASSERT (m_persistTimeout == Simulator::GetDelayLeft (m_persistEvent));
//     }

//     // TCP state machine code in different process functions
//     // C.f.: tcp_rcv_state_process() in tcp_input.c in Linux kernel
//     switch (m_state)
//     {
//         case ESTABLISHED:
//             ProcessEstablished (packet, tcpHeader);
//             break;
//         case LISTEN:
//             ProcessListen (packet, tcpHeader, fromAddress, toAddress);
//             break;
//         case TIME_WAIT:
//             // Do nothing
//             break;
//         case CLOSED:
//             // Send RST if the incoming packet is not a RST
//             if ((tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG)) != TcpHeader::RST)
//             {   // Since m_endPoint is not configured yet, we cannot use SendRST here
//                 TcpHeader h;
//                 Ptr<Packet> p = Create<Packet> ();
//                 h.SetFlags (TcpHeader::RST);
//                 h.SetSequenceNumber (m_tcb->m_nextTxSequence);
//                 h.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
//                 h.SetSourcePort (tcpHeader.GetDestinationPort ());
//                 h.SetDestinationPort (tcpHeader.GetSourcePort ());
//                 h.SetWindowSize (AdvertisedWindowSize ());
//                 AddOptions (h);
//                 m_txTrace (p, h, this);
//                 m_tcp->SendPacket (p, h, toAddress, fromAddress, m_boundnetdevice);
//             }
//             break;
//         case SYN_SENT:
//             ProcessSynSent (packet, tcpHeader);
//             break;
//         case SYN_RCVD:
//             ProcessSynRcvd (packet, tcpHeader, fromAddress, toAddress);
//             break;
//         case FIN_WAIT_1:
//         case FIN_WAIT_2:
//         case CLOSE_WAIT:
//             ProcessWait (packet, tcpHeader);
//             break;
//         case CLOSING:
//             ProcessClosing (packet, tcpHeader);
//             break;
//         case LAST_ACK:
//             ProcessLastAck (packet, tcpHeader);
//             break;
//         default: // mute compiler
//             break;
//     }

//     if (m_rWnd.Get () != 0 && m_persistEvent.IsRunning ())
//     {   // persist probes end, the other end has increased the window
//         NS_ASSERT (m_connected);
//         NS_LOG_LOGIC (this << " Leaving zerowindow persist state");
//         m_persistEvent.Cancel ();

//         SendPendingData (m_connected);
//     }


//     // ????????????, ?????????????????????
//     Update ();
// }


// /* Process the newly received ACK */
// void
// PaceTcpSocketBase::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
// {
//     NS_LOG_FUNCTION (this << tcpHeader);

//     NS_ASSERT (0 != (tcpHeader.GetFlags () & TcpHeader::ACK));
//     NS_ASSERT (m_tcb->m_segmentSize > 0);

//     uint32_t previousLost = m_txBuffer->GetLost ();
//     uint32_t priorInFlight = m_tcb->m_bytesInFlight.Get ();

//     // RFC 6675, Section 5, 1st paragraph:
//     // Upon the receipt of any ACK containing SACK information, the
//     // scoreboard MUST be updated via the Update () routine (done in ReadOptions)
//     uint32_t bytesSacked = 0;
//     uint64_t previousDelivered = m_rateOps->GetConnectionRate ().m_delivered;
//     ReadOptions (tcpHeader, &bytesSacked);

//     SequenceNumber32 ackNumber = tcpHeader.GetAckNumber ();
//     SequenceNumber32 oldHeadSequence = m_txBuffer->HeadSequence ();


//     if (ackNumber < oldHeadSequence)
//     {
//         NS_LOG_DEBUG ("Possibly received a stale ACK (ack number < head sequence)");
//         // If there is any data piggybacked, store it into m_rxBuffer
//         if (packet->GetSize () > 0)
//         {
//             ReceivedData (packet, tcpHeader);
//         }
//       return;
//     }
//     if ((ackNumber > oldHeadSequence) && (ackNumber < m_recover)
//                                       && (m_tcb->m_congState == TcpSocketState::CA_RECOVERY))
//     {
//         uint32_t segAcked = (ackNumber - oldHeadSequence)/m_tcb->m_segmentSize;
//         for (uint32_t i = 0; i < segAcked; i++)
//         {
//             if (m_txBuffer->IsRetransmittedDataAcked (ackNumber - (i * m_tcb->m_segmentSize)))
//             {
//                 m_tcb->m_isRetransDataAcked = true;
//                 NS_LOG_DEBUG ("Ack Number " << ackNumber <<
//                               "is ACK of retransmitted packet.");
//             }
//         }
//     }

//     m_txBuffer->DiscardUpTo (ackNumber, MakeCallback (&TcpRateOps::SkbDelivered, m_rateOps));

//     uint32_t currentDelivered = static_cast<uint32_t> (m_rateOps->GetConnectionRate ().m_delivered - previousDelivered);

//     if (m_tcb->m_congState == TcpSocketState::CA_CWR && (ackNumber > m_recover))
//     {
//         // Recovery is over after the window exceeds m_recover
//         // (although it may be re-entered below if ECE is still set)
//         NS_LOG_DEBUG (TcpSocketState::TcpCongStateName[m_tcb->m_congState] << " -> CA_OPEN");
//         m_tcb->m_congState = TcpSocketState::CA_OPEN;
//         if (!m_congestionControl->HasCongControl ())
//         {
//             m_tcb->m_cWnd = m_tcb->m_ssThresh.Get ();
//             m_recoveryOps->ExitRecovery (m_tcb);
//             m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_COMPLETE_CWR);
//         }
//     }

//     if (ackNumber > oldHeadSequence && (m_tcb->m_ecnState != TcpSocketState::ECN_DISABLED) && (tcpHeader.GetFlags () & TcpHeader::ECE))
//     {
//         if (m_ecnEchoSeq < ackNumber)
//         {
//             NS_LOG_INFO ("Received ECN Echo is valid");
//             m_ecnEchoSeq = ackNumber;
//             NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_ECE_RCVD");
//             m_tcb->m_ecnState = TcpSocketState::ECN_ECE_RCVD;
//             if (m_tcb->m_congState != TcpSocketState::CA_CWR)
//             {
//                 EnterCwr (currentDelivered);
//             }
//         }
//     }
//     else if (m_tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD && !(tcpHeader.GetFlags () & TcpHeader::ECE))
//     {
//         m_tcb->m_ecnState = TcpSocketState::ECN_IDLE;
//     }

//     // Update bytes in flight before processing the ACK for proper calculation of congestion window
//     NS_LOG_INFO ("Update bytes in flight before processing the ACK.");
//     BytesInFlight ();

//     // RFC 6675 Section 5: 2nd, 3rd paragraph and point (A), (B) implementation
//     // are inside the function ProcessAck
//     ProcessAck (ackNumber, (bytesSacked > 0), currentDelivered, oldHeadSequence);
//     m_tcb->m_isRetransDataAcked = false;

//     if (m_congestionControl->HasCongControl ())
//     {
//         uint32_t currentLost = m_txBuffer->GetLost ();
//         uint32_t lost = (currentLost > previousLost) ?
//               currentLost - previousLost :
//               previousLost - currentLost;
//         auto rateSample = m_rateOps->GenerateSample (currentDelivered, lost,
//                                                 false, priorInFlight, m_tcb->m_minRtt);
//         auto rateConn = m_rateOps->GetConnectionRate ();
//         m_congestionControl->CongControl(m_tcb, rateConn, rateSample);
//     }

//     // If there is any data piggybacked, store it into m_rxBuffer
//     if (packet->GetSize () > 0)
//     {
//         ReceivedData (packet, tcpHeader);
//     }

//     // RFC 6675, Section 5, point (C), try to send more data. NB: (C) is implemented
//     // inside SendPendingData
//     SendPendingData (m_connected);
// }

// void
// PaceTcpSocketBase::ProcessAck(const SequenceNumber32 &ackNumber, bool scoreboardUpdated,
//                           uint32_t currentDelivered, const SequenceNumber32 &oldHeadSequence)
// {
//     NS_LOG_FUNCTION (this << ackNumber << scoreboardUpdated);
//     // RFC 6675, Section 5, 2nd paragraph:
//     // If the incoming ACK is a cumulative acknowledgment, the TCP MUST
//     // reset DupAcks to zero.
//     bool exitedFastRecovery = false;
//     uint32_t oldDupAckCount = m_dupAckCount; // remember the old value
//     m_tcb->m_lastAckedSeq = ackNumber; // Update lastAckedSeq
//     uint32_t bytesAcked = 0;

//     /* In RFC 5681 the definition of duplicate acknowledgment was strict:
//     *
//     * (a) the receiver of the ACK has outstanding data,
//     * (b) the incoming acknowledgment carries no data,
//     * (c) the SYN and FIN bits are both off,
//     * (d) the acknowledgment number is equal to the greatest acknowledgment
//     *     received on the given connection (TCP.UNA from [RFC793]),
//     * (e) the advertised window in the incoming acknowledgment equals the
//     *     advertised window in the last incoming acknowledgment.
//     *
//     * With RFC 6675, this definition has been reduced:
//     *
//     * (a) the ACK is carrying a SACK block that identifies previously
//     *     unacknowledged and un-SACKed octets between HighACK (TCP.UNA) and
//     *     HighData (m_highTxMark)
//     */

//     bool isDupack = m_sackEnabled ?
//       scoreboardUpdated
//       : ackNumber == oldHeadSequence &&
//       ackNumber < m_tcb->m_highTxMark;

//     NS_LOG_DEBUG ("ACK of " << ackNumber <<
//                   " SND.UNA=" << oldHeadSequence <<
//                   " SND.NXT=" << m_tcb->m_nextTxSequence <<
//                   " in state: " << TcpSocketState::TcpCongStateName[m_tcb->m_congState] <<
//                   " with m_recover: " << m_recover);

//     // RFC 6675, Section 5, 3rd paragraph:
//     // If the incoming ACK is a duplicate acknowledgment per the definition
//     // in Section 2 (regardless of its status as a cumulative
//     // acknowledgment), and the TCP is not currently in loss recovery
//     if (isDupack)
//     {
//         // loss recovery check is done inside this function thanks to
//         // the congestion state machine
//         DupAck (currentDelivered);
//     }

//     if (ackNumber == oldHeadSequence
//         && ackNumber == m_tcb->m_highTxMark)
//     {
//         // Dupack, but the ACK is precisely equal to the nextTxSequence
//         return;
//     }
//     else if (ackNumber == oldHeadSequence
//             && ackNumber > m_tcb->m_highTxMark)
//     {
//         // ACK of the FIN bit ... nextTxSequence is not updated since we
//         // don't have anything to transmit
//         NS_LOG_DEBUG ("Update nextTxSequence manually to " << ackNumber);
//         m_tcb->m_nextTxSequence = ackNumber;
//     }
//     else if (ackNumber == oldHeadSequence)
//     {
//         // DupAck. Artificially call PktsAcked: after all, one segment has been ACKed.
//         m_congestionControl->PktsAcked (m_tcb, 1, m_tcb->m_lastRtt);
//     }
//     else if (ackNumber > oldHeadSequence)
//     {
//         // Please remember that, with SACK, we can enter here even if we
//         // received a dupack.
//         bytesAcked = ackNumber - oldHeadSequence;
//         uint32_t segsAcked  = bytesAcked / m_tcb->m_segmentSize;
//         m_bytesAckedNotProcessed += bytesAcked % m_tcb->m_segmentSize;
//         bytesAcked -= bytesAcked % m_tcb->m_segmentSize;

//         if (m_bytesAckedNotProcessed >= m_tcb->m_segmentSize)
//         {
//             segsAcked += 1;
//             bytesAcked += m_tcb->m_segmentSize;
//             m_bytesAckedNotProcessed -= m_tcb->m_segmentSize;
//         }

//         // Dupack count is reset to eventually fast-retransmit after 3 dupacks.
//         // Any SACK-ed segment will be cleaned up by DiscardUpTo.
//         // In the case that we advanced SND.UNA, but the ack contains SACK blocks,
//         // we do not reset. At the third one we will retransmit.
//         // If we are already in recovery, this check is useless since dupAcks
//         // are not considered in this phase. When from Recovery we go back
//         // to open, then dupAckCount is reset anyway.
//         if (!isDupack)
//         {
//             m_dupAckCount = 0;
//         }

//         // RFC 6675, Section 5, part (B)
//         // (B) Upon receipt of an ACK that does not cover RecoveryPoint, the
//         // following actions MUST be taken:
//         //
//         // (B.1) Use Update () to record the new SACK information conveyed
//         //       by the incoming ACK.
//         // (B.2) Use SetPipe () to re-calculate the number of octets still
//         //       in the network.
//         //
//         // (B.1) is done at the beginning, while (B.2) is delayed to part (C) while
//         // trying to transmit with SendPendingData. We are not allowed to exit
//         // the CA_RECOVERY phase. Just process this partial ack (RFC 5681)
//         if (ackNumber < m_recover && m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
//         {
//             if (!m_sackEnabled)
//             {
//                 // Manually set the head as lost, it will be retransmitted.
//                 NS_LOG_INFO ("Partial ACK. Manually setting head as lost");
//                 m_txBuffer->MarkHeadAsLost ();
//             }

//             // Before retransmitting the packet perform DoRecovery and check if
//             // there is available window
//             if (!m_congestionControl->HasCongControl () && segsAcked >= 1)
//             {
//                 m_recoveryOps->DoRecovery (m_tcb, currentDelivered);
//             }

//             // If the packet is already retransmitted do not retransmit it
//             if (!m_txBuffer->IsRetransmittedDataAcked (ackNumber + m_tcb->m_segmentSize))
//             {
//                 DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
//                 m_tcb->m_cWndInfl = SafeSubtraction (m_tcb->m_cWndInfl, bytesAcked);
//             }

//             // This partial ACK acknowledge the fact that one segment has been
//             // previously lost and now successfully received. All others have
//             // been processed when they come under the form of dupACKs
//             m_congestionControl->PktsAcked (m_tcb, 1, m_tcb->m_lastRtt);
//             NewAck (ackNumber, m_isFirstPartialAck);

//             if (m_isFirstPartialAck)
//             {
//                 NS_LOG_DEBUG ("Partial ACK of " << ackNumber <<
//                               " and this is the first (RTO will be reset);"
//                               " cwnd set to " << m_tcb->m_cWnd <<
//                               " recover seq: " << m_recover <<
//                               " dupAck count: " << m_dupAckCount);
//                 m_isFirstPartialAck = false;
//             }
//             else
//             {
//                 NS_LOG_DEBUG ("Partial ACK of " << ackNumber <<
//                               " and this is NOT the first (RTO will not be reset)"
//                               " cwnd set to " << m_tcb->m_cWnd <<
//                               " recover seq: " << m_recover <<
//                               " dupAck count: " << m_dupAckCount);
//             }
//         }
//         // From RFC 6675 section 5.1
//         // In addition, a new recovery phase (as described in Section 5) MUST NOT
//         // be initiated until HighACK is greater than or equal to the new value
//         // of RecoveryPoint.
//         else if (ackNumber < m_recover && m_tcb->m_congState == TcpSocketState::CA_LOSS)
//         {
//             m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
//             m_congestionControl->IncreaseWindow (m_tcb, segsAcked);

//             NS_LOG_DEBUG (" Cong Control Called, cWnd=" << m_tcb->m_cWnd <<
//                           " ssTh=" << m_tcb->m_ssThresh);
//             if (!m_sackEnabled)
//               {
//                 NS_ASSERT_MSG (m_txBuffer->GetSacked () == 0,
//                               "Some segment got dup-acked in CA_LOSS state: " <<
//                               m_txBuffer->GetSacked ());
//               }
//             NewAck (ackNumber, true);
//         }
//         else if (m_tcb->m_congState == TcpSocketState::CA_CWR)
//         {
//             m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
//             // TODO: need to check behavior if marking is compounded by loss
//             // and/or packet reordering
//             if (!m_congestionControl->HasCongControl () && segsAcked >= 1)
//             {
//                 m_recoveryOps->DoRecovery (m_tcb, currentDelivered);
//             }
//             NewAck (ackNumber, true);
//         }
//         else
//         {
//             if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
//             {
//                 m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
//             }
//             else if (m_tcb->m_congState == TcpSocketState::CA_DISORDER)
//             {
//                 if (segsAcked >= oldDupAckCount)
//                 {
//                     m_congestionControl->PktsAcked (m_tcb, segsAcked - oldDupAckCount, m_tcb->m_lastRtt);
//                 }

//                 if (!isDupack)
//                 {
//                     // The network reorder packets. Linux changes the counting lost
//                     // packet algorithm from FACK to NewReno. We simply go back in Open.
//                     m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
//                     m_tcb->m_congState = TcpSocketState::CA_OPEN;
//                     NS_LOG_DEBUG (segsAcked << " segments acked in CA_DISORDER, ack of " <<
//                                   ackNumber << " exiting CA_DISORDER -> CA_OPEN");
//                 }
//                 else
//                 {
//                     NS_LOG_DEBUG (segsAcked << " segments acked in CA_DISORDER, ack of " <<
//                                   ackNumber << " but still in CA_DISORDER");
//                 }
//             }
//             // RFC 6675, Section 5:
//             // Once a TCP is in the loss recovery phase, the following procedure
//             // MUST be used for each arriving ACK:
//             // (A) An incoming cumulative ACK for a sequence number greater than
//             // RecoveryPoint signals the end of loss recovery, and the loss
//             // recovery phase MUST be terminated.  Any information contained in
//             // the scoreboard for sequence numbers greater than the new value of
//             // HighACK SHOULD NOT be cleared when leaving the loss recovery
//             // phase.
//             else if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
//             {
//                 m_isFirstPartialAck = true;

//                 // Recalculate the segs acked, that are from m_recover to ackNumber
//                 // (which are the ones we have not passed to PktsAcked and that
//                 // can increase cWnd)
//                 // TODO:  check consistency for dynamic segment size
//                 segsAcked = static_cast<uint32_t>(ackNumber - oldHeadSequence) / m_tcb->m_segmentSize;
//                 m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);
//                 m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_COMPLETE_CWR);
//                 m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
//                 m_tcb->m_congState = TcpSocketState::CA_OPEN;
//                 exitedFastRecovery = true;
//                 m_dupAckCount = 0; // From recovery to open, reset dupack

//                 NS_LOG_DEBUG (segsAcked << " segments acked in CA_RECOVER, ack of " <<
//                               ackNumber << ", exiting CA_RECOVERY -> CA_OPEN");
//             }
//             else if (m_tcb->m_congState == TcpSocketState::CA_LOSS)
//             {
//                 m_isFirstPartialAck = true;

//                 // Recalculate the segs acked, that are from m_recover to ackNumber
//                 // (which are the ones we have not passed to PktsAcked and that
//                 // can increase cWnd)
//                 segsAcked = (ackNumber - m_recover) / m_tcb->m_segmentSize;

//                 m_congestionControl->PktsAcked (m_tcb, segsAcked, m_tcb->m_lastRtt);

//                 m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_OPEN);
//                 m_tcb->m_congState = TcpSocketState::CA_OPEN;
//                 NS_LOG_DEBUG (segsAcked << " segments acked in CA_LOSS, ack of" <<
//                               ackNumber << ", exiting CA_LOSS -> CA_OPEN");
//             }

//             if (ackNumber >= m_recover)
//             {
//                 // All lost segments in the congestion event have been
//                 // retransmitted successfully. The recovery point (m_recover)
//                 // should be deactivated.
//                 m_recoverActive = false;
//             }

//             if (exitedFastRecovery)
//             {
//                 NewAck (ackNumber, true);
//                 m_tcb->m_cWnd = m_tcb->m_ssThresh.Get ();
//                 m_recoveryOps->ExitRecovery (m_tcb);
//                 NS_LOG_DEBUG ("Leaving Fast Recovery; BytesInFlight() = " <<
//                               BytesInFlight () << "; cWnd = " << m_tcb->m_cWnd);
//             }
//             if (m_tcb->m_congState == TcpSocketState::CA_OPEN)
//             {
//                 m_congestionControl->IncreaseWindow (m_tcb, segsAcked);

//                 m_tcb->m_cWndInfl = m_tcb->m_cWnd;

//                 NS_LOG_LOGIC ("Congestion control called: " <<
//                               " cWnd: " << m_tcb->m_cWnd <<
//                               " ssTh: " << m_tcb->m_ssThresh <<
//                               " segsAcked: " << segsAcked);

//                 NewAck (ackNumber, true);
//             }
//         }
//     }
//     // Update the pacing rate, since m_congestionControl->IncreaseWindow() or
//     // m_congestionControl->PktsAcked () may change m_tcb->m_cWnd
//     // Make sure that control reaches the end of this function and there is no
//     // return in between
//     UpdatePacingRate ();
// }


// /* Send an empty packet with specified TCP flags */
// void
// PaceTcpSocketBase::SendEmptyPacket (uint8_t flags)
// {
//     NS_LOG_FUNCTION (this << static_cast<uint32_t> (flags));

//     if (m_endPoint == nullptr && m_endPoint6 == nullptr)
//     {
//         NS_LOG_WARN ("Failed to send empty packet due to null endpoint");
//         return;
//     }

//     Ptr<Packet> p = Create<Packet> ();
//     TcpHeader header;
//     SequenceNumber32 s = m_tcb->m_nextTxSequence;

//     if (flags & TcpHeader::FIN)
//     {
//         flags |= TcpHeader::ACK;
//     }
//     else if (m_state == FIN_WAIT_1 || m_state == LAST_ACK || m_state == CLOSING)
//     {
//         ++s;
//     }

//     AddSocketTags (p);

//     header.SetFlags (flags);
//     header.SetSequenceNumber (s);
//     header.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
//     if (m_endPoint != nullptr)
//     {
//         header.SetSourcePort (m_endPoint->GetLocalPort ());
//         header.SetDestinationPort (m_endPoint->GetPeerPort ());
//     }
//     else
//     {
//         header.SetSourcePort (m_endPoint6->GetLocalPort ());
//         header.SetDestinationPort (m_endPoint6->GetPeerPort ());
//     }

//     AddOptions (header);

//     // RFC 6298, clause 2.4
//     m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);

//     uint16_t windowSize = AdvertisedWindowSize ();
//     bool hasSyn = flags & TcpHeader::SYN;
//     bool hasFin = flags & TcpHeader::FIN;
//     bool isAck = flags == TcpHeader::ACK;
//     if (hasSyn)
//     {
//         if (m_winScalingEnabled)
//         { // The window scaling option is set only on SYN packets
//             AddOptionWScale (header);
//         }

//         if (m_sackEnabled)
//         {
//             AddOptionSackPermitted (header);
//         }

//         if (m_synCount == 0)
//         {   // No more connection retries, give up
//             NS_LOG_LOGIC ("Connection failed.");
//             m_rtt->Reset (); //According to recommendation -> RFC 6298
//             NotifyConnectionFailed ();
//             m_state = CLOSED;
//             DeallocateEndPoint ();
//             return;
//         }
//         else
//         {   // Exponential backoff of connection time out
//             int backoffCount = 0x1 << (m_synRetries - m_synCount);
//             m_rto = m_cnTimeout * backoffCount;
//             m_synCount--;
//         }

//         if (m_synRetries - 1 == m_synCount)
//         {
//             UpdateRttHistory (s, 0, false);
//         }
//         else
//         { // This is SYN retransmission
//             UpdateRttHistory (s, 0, true);
//         }

//         windowSize = AdvertisedWindowSize (false);
//     }
//     header.SetWindowSize (windowSize);

//     if (flags & TcpHeader::ACK)
//     { // If sending an ACK, cancel the delay ACK as well
//         m_delAckEvent.Cancel ();
//         m_delAckCount = 0;
//         if (m_highTxAck < header.GetAckNumber ())
//         {
//             m_highTxAck = header.GetAckNumber ();
//         }
//         if (m_sackEnabled && m_tcb->m_rxBuffer->GetSackListSize () > 0)
//         {
//             AddOptionSack (header);
//         }
//         NS_LOG_INFO ("Sending a pure ACK, acking seq " << m_tcb->m_rxBuffer->NextRxSequence ());
//     }


//     AppendAck (p, header);
//     if (m_ackSentEvent.IsExpired())
//     {
//         SendAck ();
//     }


//     if (m_retxEvent.IsExpired () && (hasSyn || hasFin) && !isAck )
//     { // Retransmit SYN / SYN+ACK / FIN / FIN+ACK to guard against lost
//         NS_LOG_LOGIC ("Schedule retransmission timeout at time "
//                       << Simulator::Now ().GetSeconds () << " to expire at time "
//                       << (Simulator::Now () + m_rto.Get ()).GetSeconds ());
//         m_retxEvent = Simulator::Schedule (m_rto, &PaceTcpSocketBase::SendEmptyPacket, this, flags);
//     }

// }


// /* This function is called only if a SYN received in LISTEN state. After
//    PaceTcpSocketBase cloned, allocate a new end point to handle the incoming
//    connection and send a SYN+ACK to complete the handshake. */
// void
// PaceTcpSocketBase::CompleteFork (Ptr<Packet> p, const TcpHeader& h,
//                              const Address& fromAddress, const Address& toAddress)
// {
//     NS_LOG_FUNCTION (this << p << h << fromAddress << toAddress);
//     NS_UNUSED (p);
//     // Get port and address from peer (connecting host)
//     if (InetSocketAddress::IsMatchingType (toAddress))
//     {
//         m_endPoint = m_tcp->Allocate (GetBoundNetDevice (),
//                                       InetSocketAddress::ConvertFrom (toAddress).GetIpv4 (),
//                                       InetSocketAddress::ConvertFrom (toAddress).GetPort (),
//                                       InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
//                                       InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
//         m_endPoint6 = nullptr;
//     }
//     else if (Inet6SocketAddress::IsMatchingType (toAddress))
//     {
//         m_endPoint6 = m_tcp->Allocate6 (GetBoundNetDevice (),
//                                         Inet6SocketAddress::ConvertFrom (toAddress).GetIpv6 (),
//                                         Inet6SocketAddress::ConvertFrom (toAddress).GetPort (),
//                                         Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
//                                         Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
//         m_endPoint = nullptr;
//     }
//     m_tcp->AddSocket (this);


//     // ????????????
//     // ???????????????
//     Initialization ();


//     // Change the cloned socket from LISTEN state to SYN_RCVD
//     NS_LOG_DEBUG ("LISTEN -> SYN_RCVD");
//     m_state = SYN_RCVD;
//     m_synCount = m_synRetries;
//     m_dataRetrCount = m_dataRetries;
//     SetupCallback ();
//     // Set the sequence number and send SYN+ACK
//     m_tcb->m_rxBuffer->SetNextRxSequence (h.GetSequenceNumber () + SequenceNumber32 (1));

//     /* Check if we received an ECN SYN packet. Change the ECN state of receiver to ECN_IDLE if sender has sent an ECN SYN
//     * packet and the traffic is ECN Capable
//     */
//     if (m_tcb->m_useEcn != TcpSocketState::Off &&
//         (h.GetFlags () & (TcpHeader::CWR | TcpHeader::ECE)) == (TcpHeader::CWR | TcpHeader::ECE))
//     {
//         SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK | TcpHeader::ECE);
//         NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_IDLE");
//         m_tcb->m_ecnState = TcpSocketState::ECN_IDLE;
//     }
//     else
//     {
//         SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
//         m_tcb->m_ecnState = TcpSocketState::ECN_DISABLED;
//     }

//     ++Index;
//     index = Index;

//     char buf[FILENAME_MAX];
//     std::string path = std::string (getcwd(buf, FILENAME_MAX))+ "/traces/info/";
//     if (!dataRateFile.is_open())
//     {
//         std::string filename = path + std::to_string(index) + "_" + "datarate.txt";
//         dataRateFile.open (filename.c_str(), std::fstream::out);
//     }
//     if (!maxRateFile.is_open())
//     {
//         std::string filename = path + std::to_string(index) + "_" + "maxrate.txt";
//         maxRateFile.open (filename.c_str(), std::fstream::out);
//     }
//     if (!targetRateFile.is_open())
//     {
//         std::string filename = path + std::to_string(index) + "_" + "targetrate.txt";
//         targetRateFile.open (filename.c_str(), std::fstream::out);
//     }
// }


// /* Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
//     TCP header, and send to TcpL4Protocol */
// uint32_t
// PaceTcpSocketBase::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
// {
//     NS_LOG_FUNCTION (this << seq << maxSize << withAck);

//     bool isStartOfTransmission = BytesInFlight () == 0U;
//     TcpTxItem *outItem = m_txBuffer->CopyFromSequence (maxSize, seq);

//     m_rateOps->SkbSent(outItem, isStartOfTransmission&&(m_tcb->m_highTxMark==m_tcb->m_nextTxSequence));

//     bool isRetransmission = outItem->IsRetrans ();
//     Ptr<Packet> p = outItem->GetPacketCopy ();
//     uint32_t sz = p->GetSize (); // Size of packet
//     uint8_t flags = withAck ? TcpHeader::ACK : 0;
//     uint32_t remainingData = m_txBuffer->SizeFromSequence (seq + SequenceNumber32 (sz));

//     // TCP sender should not send data out of the window advertised by the
//     // peer when it is not retransmission.
//     NS_ASSERT (isRetransmission || ((m_highRxAckMark + SequenceNumber32 (m_rWnd)) >= (seq + SequenceNumber32 (maxSize))));

//     if (IsPacingEnabled ())
//     {
//         NS_LOG_INFO ("Pacing is enabled");
//         if (m_pacingTimer.IsExpired ())
//         {
//             NS_LOG_DEBUG ("Current Pacing Rate " << m_tcb->m_pacingRate);
//             NS_LOG_DEBUG ("Timer is in expired state, activate it " << m_tcb->m_pacingRate.Get ().CalculateBytesTxTime (sz));
//             m_pacingTimer.Schedule (m_tcb->m_pacingRate.Get ().CalculateBytesTxTime (sz));
//         }
//         else
//         {
//             NS_LOG_INFO ("Timer is already in running state");
//         }
//     }
//     else
//     {
//         NS_LOG_INFO ("Pacing is disabled");
//     }

//     if (withAck)
//     {
//         m_delAckEvent.Cancel ();
//         m_delAckCount = 0;
//     }

//     if (m_tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD && m_ecnEchoSeq.Get() > m_ecnCWRSeq.Get () && !isRetransmission)
//     {
//         NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_CWR_SENT");
//         m_tcb->m_ecnState = TcpSocketState::ECN_CWR_SENT;
//         m_ecnCWRSeq = seq;
//         flags |= TcpHeader::CWR;
//         NS_LOG_INFO ("CWR flags set");
//     }

//     AddSocketTags (p);

//     if (m_closeOnEmpty && (remainingData == 0))
//     {
//         flags |= TcpHeader::FIN;
//         if (m_state == ESTABLISHED)
//         {   // On active close: I am the first one to send FIN
//             NS_LOG_DEBUG ("ESTABLISHED -> FIN_WAIT_1");
//             m_state = FIN_WAIT_1;
//         }
//         else if (m_state == CLOSE_WAIT)
//         {   // On passive close: Peer sent me FIN already
//             NS_LOG_DEBUG ("CLOSE_WAIT -> LAST_ACK");
//             m_state = LAST_ACK;
//         }
//     }
//     TcpHeader header;
//     header.SetFlags (flags);
//     header.SetSequenceNumber (seq);
//     header.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
//     if (m_endPoint)
//     {
//         header.SetSourcePort (m_endPoint->GetLocalPort ());
//         header.SetDestinationPort (m_endPoint->GetPeerPort ());
//     }
//     else
//     {
//         header.SetSourcePort (m_endPoint6->GetLocalPort ());
//         header.SetDestinationPort (m_endPoint6->GetPeerPort ());
//     }
//     header.SetWindowSize (AdvertisedWindowSize ());
//     AddOptions (header);

//     if (m_retxEvent.IsExpired ())
//     {
//         // Schedules retransmit timeout. m_rto should be already doubled.

//         NS_LOG_LOGIC (this << " SendDataPacket Schedule ReTxTimeout at time " <<
//                       Simulator::Now ().GetSeconds () << " to expire at time " <<
//                       (Simulator::Now () + m_rto.Get ()).GetSeconds () );
//         m_retxEvent = Simulator::Schedule (m_rto, &PaceTcpSocketBase::ReTxTimeout, this);
//     }

//     m_txTrace (p, header, this);

//     if (m_endPoint)
//     {
//         m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
//                           m_endPoint->GetPeerAddress (), m_boundnetdevice);
//         NS_LOG_DEBUG ("Send segment of size " << sz << " with remaining data " <<
//                       remainingData << " via TcpL4Protocol to " <<  m_endPoint->GetPeerAddress () <<
//                       ". Header " << header);
//     }
//     else
//     {
//         m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
//                           m_endPoint6->GetPeerAddress (), m_boundnetdevice);
//         NS_LOG_DEBUG ("Send segment of size " << sz << " with remaining data " <<
//                       remainingData << " via TcpL4Protocol to " <<  m_endPoint6->GetPeerAddress () <<
//                       ". Header " << header);
//     }

//     UpdateRttHistory (seq, sz, isRetransmission);

//     // Update bytes sent during recovery phase
//     if (m_tcb->m_congState == TcpSocketState::CA_RECOVERY || m_tcb->m_congState == TcpSocketState::CA_CWR)
//     {
//         m_recoveryOps->UpdateBytesSent (sz);
//     }

//     // Notify the application of the data being sent unless this is a retransmit
//     if (!isRetransmission)
//     {
//         Simulator::ScheduleNow (&PaceTcpSocketBase::NotifyDataSent, this,
//                                 (seq + sz - m_tcb->m_highTxMark.Get ()));
//     }
//     // Update highTxMark
//     m_tcb->m_highTxMark = std::max (seq + sz, m_tcb->m_highTxMark.Get ());
//     return sz;
// }

// void
// PaceTcpSocketBase::UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
//                                  bool isRetransmission)
// {
//     NS_LOG_FUNCTION (this);

//     // update the history of sequence numbers used to calculate the RTT
//     if (isRetransmission == false)
//     { // This is the next expected one, just log at end
//         m_history.push_back (RttHistory (seq, sz, Simulator::Now ()));
//     }
//     else
//     { // This is a retransmit, find in list and mark as re-tx
//         for (std::deque<RttHistory>::iterator i = m_history.begin (); i != m_history.end (); ++i)
//         {
//             if ((seq >= i->seq) && (seq < (i->seq + SequenceNumber32 (i->count))))
//             { // Found it
//                 i->retx = true;
//                 i->count = ((seq + SequenceNumber32 (sz)) - i->seq); // And update count in hist
//                 break;
//             }
//         }
//     }
// }

// uint32_t
// PaceTcpSocketBase::UnAckDataCount () const
// {
//     return m_tcb->m_highTxMark - m_txBuffer->HeadSequence ();
// }

// uint32_t
// PaceTcpSocketBase::BytesInFlight () const
// {
//     uint32_t bytesInFlight = m_txBuffer->BytesInFlight ();
//     // Ugly, but we are not modifying the state; m_bytesInFlight is used
//     // only for tracing purpose.
//     m_tcb->m_bytesInFlight = bytesInFlight;

//     NS_LOG_DEBUG ("Returning calculated bytesInFlight: " << bytesInFlight);
//     return bytesInFlight;
// }

// uint32_t
// PaceTcpSocketBase::Window (void) const
// {
//     return std::min (m_rWnd.Get (), m_tcb->m_cWnd.Get ());
// }

// uint32_t
// PaceTcpSocketBase::AvailableWindow () const
// {
//     uint32_t win = Window ();             // Number of bytes allowed to be outstanding
//     uint32_t inflight = BytesInFlight (); // Number of outstanding bytes
//     return (inflight > win) ? 0 : win - inflight;
// }

// uint16_t
// PaceTcpSocketBase::AdvertisedWindowSize (bool scale) const
// {
//     NS_LOG_FUNCTION (this << scale);
//     uint32_t w;

//     // We don't want to advertise 0 after a FIN is received. So, we just use
//     // the previous value of the advWnd.
//     if (m_tcb->m_rxBuffer->GotFin ())
//     {
//         w = m_advWnd;
//     }
//     else
//     {
//         NS_ASSERT_MSG (m_tcb->m_rxBuffer->MaxRxSequence () - m_tcb->m_rxBuffer->NextRxSequence () >= 0,
//                       "Unexpected sequence number values");
//         w = static_cast<uint32_t> (m_tcb->m_rxBuffer->MaxRxSequence () - m_tcb->m_rxBuffer->NextRxSequence ());
//     }

//     // Ugly, but we are not modifying the state, that variable
//     // is used only for tracing purpose.
//     if (w != m_advWnd)
//     {
//         const_cast<PaceTcpSocketBase*> (this)->m_advWnd = w;
//     }
//     if (scale)
//     {
//         w >>= m_rcvWindShift;
//     }
//     if (w > m_maxWinSize)
//     {
//         w = m_maxWinSize;
//         NS_LOG_WARN ("Adv window size truncated to " << m_maxWinSize << "; possibly to avoid overflow of the 16-bit integer");
//     }
//     NS_LOG_LOGIC ("Returning AdvertisedWindowSize of " << static_cast<uint16_t> (w));
//     return static_cast<uint16_t> (w);
// }

// // Receipt of new packet, put into Rx buffer
// void
// PaceTcpSocketBase::ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader)
// {
//     NS_LOG_FUNCTION (this << tcpHeader);
//     NS_LOG_DEBUG ("Data segment, seq=" << tcpHeader.GetSequenceNumber () <<
//                   " pkt size=" << p->GetSize () );

//     // Put into Rx buffer
//     SequenceNumber32 expectedSeq = m_tcb->m_rxBuffer->NextRxSequence ();
//     if (!m_tcb->m_rxBuffer->Add (p, tcpHeader))
//     { // Insert failed: No data or RX buffer full
//         if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
//         {
//           SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
//           NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_SENDING_ECE");
//           m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
//         }
//         else
//         {
//             SendEmptyPacket (TcpHeader::ACK);
//         }
//       return;
//     }
//     // Notify app to receive if necessary
//     if (expectedSeq < m_tcb->m_rxBuffer->NextRxSequence ())
//     { // NextRxSeq advanced, we have something to send to the app
//         if (!m_shutdownRecv)
//         {
//             NotifyDataRecv ();
//         }
//         // Handle exceptions
//         if (m_closeNotified)
//         {
//             NS_LOG_WARN ("Why TCP " << this << " got data after close notification?");
//         }
//         // If we received FIN before and now completed all "holes" in rx buffer,
//         // invoke peer close procedure
//         if (m_tcb->m_rxBuffer->Finished () && (tcpHeader.GetFlags () & TcpHeader::FIN) == 0)
//         {
//             DoPeerClose ();
//             return;
//         }
//     }
//     // Now send a new ACK packet acknowledging all received and delivered data
//     if (m_tcb->m_rxBuffer->Size () > m_tcb->m_rxBuffer->Available () || m_tcb->m_rxBuffer->NextRxSequence () > expectedSeq + p->GetSize ())
//     {   // A gap exists in the buffer, or we filled a gap: Always ACK
//         m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_NON_DELAYED_ACK);
//         if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
//         {
//             SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
//             NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_SENDING_ECE");
//             m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
//         }
//         else
//         {
//             SendEmptyPacket (TcpHeader::ACK);
//         }
//     }
//     else
//     {   // In-sequence packet: ACK if delayed ack count allows
//         if (++m_delAckCount >= m_delAckMaxCount)
//         {
//             m_delAckEvent.Cancel ();
//             m_delAckCount = 0;
//             m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_NON_DELAYED_ACK);
//             if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
//             {
//                 NS_LOG_DEBUG("Congestion algo " << m_congestionControl->GetName ());
//                 SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
//                 NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_SENDING_ECE");
//                 m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
//             }
//             else
//             {
//                 SendEmptyPacket (TcpHeader::ACK);
//             }
//         }
//         else if (!m_delAckEvent.IsExpired ())
//         {
//             m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_DELAYED_ACK);
//         }
//         else if (m_delAckEvent.IsExpired ())
//         {
//             m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_DELAYED_ACK);
//             m_delAckEvent = Simulator::Schedule (m_delAckTimeout,
//                                                 &PaceTcpSocketBase::DelAckTimeout, this);
//             NS_LOG_LOGIC (this << " scheduled delayed ACK at " <<
//                           (Simulator::Now () + Simulator::GetDelayLeft (m_delAckEvent)).GetSeconds ());
//         }
//     }
// }

// /**
//  * \brief Estimate the RTT
//  *
//  * Called by ForwardUp() to estimate RTT.
//  *
//  * \param tcpHeader TCP header for the incoming packet
//  */
// void
// PaceTcpSocketBase::EstimateRtt (const TcpHeader& tcpHeader)
// {
//     SequenceNumber32 ackSeq = tcpHeader.GetAckNumber ();
//     Time m = Time (0.0);

//     // ????????????
//     if (m_timestampEnabled && tcpHeader.HasOption (TcpOption::TS))
//     {
//         Ptr<const TcpOptionTS> ts;
//         ts = DynamicCast<const TcpOptionTS> (tcpHeader.GetOption (TcpOption::TS));

//         if (ts->GetEcho () != m_lastEcho)
//         {
//             m_lastEcho = ts->GetEcho ();

//             Time rtt = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetEcho ());   // all

//             m_ReceRttEstimator->Measurement (rtt);
//             m_lastReceRtt = m_ReceRttEstimator->GetEstimate();
//             m_minRxRTT = std::min (m_lastReceRtt.Get (), m_minRxRTT);
//             m_maxRxRTT = std::max (rtt, m_maxRxRTT);

//             m_delayLow  = (m_maxRxRTT - m_minRxRTT) * m_alpha + m_minRxRTT;
//             m_delayHigh = (m_maxRxRTT - m_minRxRTT) * m_beta  + m_minRxRTT;
//         }

//     }

//     // An ack has been received, calculate rtt and log this measurement
//     // Note we use a linear search (O(n)) for this since for the common
//     // case the ack'ed packet will be at the head of the list
//     if (!m_history.empty ())
//     {
//         RttHistory& h = m_history.front ();
//         if (!h.retx && ackSeq >= (h.seq + SequenceNumber32 (h.count)))
//         {   // Ok to use this sample
//             if (m_timestampEnabled && tcpHeader.HasOption (TcpOption::TS))
//             {
//                 Ptr<const TcpOptionTS> ts;
//                 ts = DynamicCast<const TcpOptionTS> (tcpHeader.GetOption (TcpOption::TS));
//                 m = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetEcho ());
//             }
//             else
//             {
//                 m = Simulator::Now () - h.time; // Elapsed time
//             }
//         }
//     }

//     // Now delete all ack history with seq <= ack
//     while (!m_history.empty ())
//     {
//         RttHistory& h = m_history.front ();
//         if ((h.seq + SequenceNumber32 (h.count)) > ackSeq)
//         {
//             break;                                                              // Done removing
//         }
//         m_history.pop_front (); // Remove
//     }

//     if (!m.IsZero ())
//     {
//         m_rateOps->UpdateRtt(m);
//         m_rtt->Measurement (m);                // Log the measurement
//         // RFC 6298, clause 2.4
//         m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);
//         m_tcb->m_lastRtt = m_rtt->GetEstimate ();
//         m_tcb->m_minRtt = std::min (m_tcb->m_lastRtt.Get (), m_tcb->m_minRtt);
//         NS_LOG_INFO (this << m_tcb->m_lastRtt << m_tcb->m_minRtt);
//     }
// }

// // Called by the ReceivedAck() when new ACK received and by ProcessSynRcvd()
// // when the three-way handshake completed. This cancels retransmission timer
// // and advances Tx window
// void
// PaceTcpSocketBase::NewAck (SequenceNumber32 const& ack, bool resetRTO)
// {
//     NS_LOG_FUNCTION (this << ack);

//     // Reset the data retransmission count. We got a new ACK!
//     m_dataRetrCount = m_dataRetries;

//     if (m_state != SYN_RCVD && resetRTO)
//     {   // Set RTO unless the ACK is received in SYN_RCVD state
//         NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
//                       (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
//         m_retxEvent.Cancel ();
//         // On receiving a "New" ack we restart retransmission timer .. RFC 6298
//         // RFC 6298, clause 2.4
//         m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto);

//         NS_LOG_LOGIC (this << " Schedule ReTxTimeout at time " <<
//                       Simulator::Now ().GetSeconds () << " to expire at time " <<
//                       (Simulator::Now () + m_rto.Get ()).GetSeconds ());
//         m_retxEvent = Simulator::Schedule (m_rto, &PaceTcpSocketBase::ReTxTimeout, this);
//     }

//     // Note the highest ACK and tell app to send more
//     NS_LOG_LOGIC ("TCP " << this << " NewAck " << ack <<
//                   " numberAck " << (ack - m_txBuffer->HeadSequence ())); // Number bytes ack'ed

//     if (GetTxAvailable () > 0)
//     {
//         NotifySend (GetTxAvailable ());
//     }
//     if (ack > m_tcb->m_nextTxSequence)
//     {
//         m_tcb->m_nextTxSequence = ack; // If advanced
//     }
//     if (m_txBuffer->Size () == 0 && m_state != FIN_WAIT_1 && m_state != CLOSING)
//     {   // No retransmit timer if no data to retransmit
//         NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
//                       (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
//         m_retxEvent.Cancel ();
//     }
// }

// // Retransmit timeout
// void
// PaceTcpSocketBase::ReTxTimeout ()
// {
//     NS_LOG_FUNCTION (this);
//     NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
//     // If erroneous timeout in closed/timed-wait state, just return
//     if (m_state == CLOSED || m_state == TIME_WAIT)
//     {
//         return;
//     }

//     if (m_state == SYN_SENT)
//     {
//         NS_ASSERT (m_synCount > 0);
//         if (m_tcb->m_useEcn == TcpSocketState::On)
//         {
//             SendEmptyPacket (TcpHeader::SYN | TcpHeader::ECE | TcpHeader::CWR);
//         }
//         else
//         {
//             SendEmptyPacket (TcpHeader::SYN);
//         }
//         return;
//     }

//     // Retransmit non-data packet: Only if in FIN_WAIT_1 or CLOSING state
//     if (m_txBuffer->Size () == 0)
//     {
//         if (m_state == FIN_WAIT_1 || m_state == CLOSING)
//         {   // Must have lost FIN, re-send
//             SendEmptyPacket (TcpHeader::FIN);
//         }
//         return;
//     }

//     NS_LOG_DEBUG ("Checking if Connection is Established");
//     // If all data are received (non-closing socket and nothing to send), just return
//     if (m_state <= ESTABLISHED && m_txBuffer->HeadSequence () >= m_tcb->m_highTxMark && m_txBuffer->Size () == 0)
//     {
//         NS_LOG_DEBUG ("Already Sent full data" << m_txBuffer->HeadSequence () << " " << m_tcb->m_highTxMark);
//         return;
//     }

//     if (m_dataRetrCount == 0)
//     {
//         NS_LOG_INFO ("No more data retries available. Dropping connection");
//         NotifyErrorClose ();
//         DeallocateEndPoint ();
//         return;
//     }
//     else
//     {
//         --m_dataRetrCount;
//     }

//     uint32_t inFlightBeforeRto = BytesInFlight ();
//     bool resetSack = !m_sackEnabled; // Reset SACK information if SACK is not enabled.
//                                     // The information in the TcpTxBuffer is guessed, in this case.

//     // Reset dupAckCount
//     m_dupAckCount = 0;
//     if (!m_sackEnabled)
//     {
//         m_txBuffer->ResetRenoSack ();
//     }

//     // From RFC 6675, Section 5.1
//     // [RFC2018] suggests that a TCP sender SHOULD expunge the SACK
//     // information gathered from a receiver upon a retransmission timeout
//     // (RTO) "since the timeout might indicate that the data receiver has
//     // reneged."  Additionally, a TCP sender MUST "ignore prior SACK
//     // information in determining which data to retransmit."
//     // It has been suggested that, as long as robust tests for
//     // reneging are present, an implementation can retain and use SACK
//     // information across a timeout event [Errata1610].
//     // The head of the sent list will not be marked as sacked, therefore
//     // will be retransmitted, if the receiver renegotiate the SACK blocks
//     // that we received.
//     m_txBuffer->SetSentListLost (resetSack);

//     // From RFC 6675, Section 5.1
//     // If an RTO occurs during loss recovery as specified in this document,
//     // RecoveryPoint MUST be set to HighData.  Further, the new value of
//     // RecoveryPoint MUST be preserved and the loss recovery algorithm
//     // outlined in this document MUST be terminated.
//     m_recover = m_tcb->m_highTxMark;
//     m_recoverActive = true;

//     // RFC 6298, clause 2.5, double the timer
//     Time doubledRto = m_rto + m_rto;
//     m_rto = Min (doubledRto, Time::FromDouble (60,  Time::S));

//     // Empty RTT history
//     m_history.clear ();

//     // Please don't reset highTxMark, it is used for retransmission detection

//     // When a TCP sender detects segment loss using the retransmission timer
//     // and the given segment has not yet been resent by way of the
//     // retransmission timer, decrease ssThresh
//     if (m_tcb->m_congState != TcpSocketState::CA_LOSS || !m_txBuffer->IsHeadRetransmitted ())
//     {
//         m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb, inFlightBeforeRto);
//     }

//     // Cwnd set to 1 MSS
//     m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_LOSS);
//     m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_LOSS);
//     m_tcb->m_congState = TcpSocketState::CA_LOSS;
//     m_tcb->m_cWnd = m_tcb->m_segmentSize;
//     m_tcb->m_cWndInfl = m_tcb->m_cWnd;

//     std::cout << "Loss Event occur at time: " << Simulator::Now ().GetSeconds () << std::endl; 

//     m_pacingTimer.Cancel ();

//     NS_LOG_DEBUG ("RTO. Reset cwnd to " <<  m_tcb->m_cWnd << ", ssthresh to " <<
//                   m_tcb->m_ssThresh << ", restart from seqnum " <<
//                   m_txBuffer->HeadSequence () << " doubled rto to " <<
//                   m_rto.Get ().GetSeconds () << " s");

//     NS_ASSERT_MSG (BytesInFlight () == 0, "There are some bytes in flight after an RTO: " <<
//                   BytesInFlight ());

//     SendPendingData (m_connected);

//     NS_ASSERT_MSG (BytesInFlight () <= m_tcb->m_segmentSize,
//                   "In flight (" << BytesInFlight () <<
//                   ") there is more than one segment (" << m_tcb->m_segmentSize << ")");
// }

// void
// PaceTcpSocketBase::DelAckTimeout (void)
// {
//     m_delAckCount = 0;
//     m_congestionControl->CwndEvent (m_tcb, TcpSocketState::CA_EVENT_DELAYED_ACK);
//     if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
//     {
//         SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
//         m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
//     }
//     else
//     {
//         SendEmptyPacket (TcpHeader::ACK);
//     }
// }

// void
// PaceTcpSocketBase::LastAckTimeout (void)
// {
//     NS_LOG_FUNCTION (this);

//     m_lastAckEvent.Cancel ();
//     if (m_state == LAST_ACK)
//     {
//         if (m_dataRetrCount == 0)
//         {
//             NS_LOG_INFO ("LAST-ACK: No more data retries available. Dropping connection");
//             NotifyErrorClose ();
//             DeallocateEndPoint ();
//             return;
//         }
//         m_dataRetrCount--;
//         SendEmptyPacket (TcpHeader::FIN | TcpHeader::ACK);
//         NS_LOG_LOGIC ("PaceTcpSocketBase " << this << " rescheduling LATO1");
//         Time lastRto = m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4);
//         m_lastAckEvent = Simulator::Schedule (lastRto, &PaceTcpSocketBase::LastAckTimeout, this);
//     }
// }

// // Send 1-byte data to probe for the window size at the receiver when
// // the local knowledge tells that the receiver has zero window size
// // C.f.: RFC793 p.42, RFC1112 sec.4.2.2.17
// void
// PaceTcpSocketBase::PersistTimeout ()
// {
//     NS_LOG_LOGIC ("PersistTimeout expired at " << Simulator::Now ().GetSeconds ());
//     m_persistTimeout = std::min (Seconds (60), Time (2 * m_persistTimeout)); // max persist timeout = 60s
//     Ptr<Packet> p = m_txBuffer->CopyFromSequence (1, m_tcb->m_nextTxSequence)->GetPacketCopy ();
//     m_txBuffer->ResetLastSegmentSent ();
//     TcpHeader tcpHeader;
//     tcpHeader.SetSequenceNumber (m_tcb->m_nextTxSequence);
//     tcpHeader.SetAckNumber (m_tcb->m_rxBuffer->NextRxSequence ());
//     tcpHeader.SetWindowSize (AdvertisedWindowSize ());
//     if (m_endPoint != nullptr)
//     {
//         tcpHeader.SetSourcePort (m_endPoint->GetLocalPort ());
//         tcpHeader.SetDestinationPort (m_endPoint->GetPeerPort ());
//     }
//     else
//     {
//         tcpHeader.SetSourcePort (m_endPoint6->GetLocalPort ());
//         tcpHeader.SetDestinationPort (m_endPoint6->GetPeerPort ());
//     }
//     AddOptions (tcpHeader);
//     //Send a packet tag for setting ECT bits in IP header
//     if (m_tcb->m_ecnState != TcpSocketState::ECN_DISABLED)
//     {
//         SocketIpTosTag ipTosTag;
//         ipTosTag.SetTos (MarkEcnCodePoint (0, m_tcb->m_ectCodePoint));
//         p->AddPacketTag (ipTosTag);

//         SocketIpv6TclassTag ipTclassTag;
//         ipTclassTag.SetTclass (MarkEcnCodePoint (0, m_tcb->m_ectCodePoint));
//         p->AddPacketTag (ipTclassTag);
//     }
//     m_txTrace (p, tcpHeader, this);

//     if (m_endPoint != nullptr)
//     {
//         m_tcp->SendPacket (p, tcpHeader, m_endPoint->GetLocalAddress (),
//                           m_endPoint->GetPeerAddress (), m_boundnetdevice);
//     }
//     else
//     {
//         m_tcp->SendPacket (p, tcpHeader, m_endPoint6->GetLocalAddress (),
//                           m_endPoint6->GetPeerAddress (), m_boundnetdevice);
//     }

//     NS_LOG_LOGIC ("Schedule persist timeout at time "
//                   << Simulator::Now ().GetSeconds () << " to expire at time "
//                   << (Simulator::Now () + m_persistTimeout).GetSeconds ());
//     m_persistEvent = Simulator::Schedule (m_persistTimeout, &PaceTcpSocketBase::PersistTimeout, this);
// }


// /* Below are the attribute get/set functions */

// void
// PaceTcpSocketBase::SetSndBufSize (uint32_t size)
// {
//     NS_LOG_FUNCTION (this << size);
//     m_txBuffer->SetMaxBufferSize (size);
// }

// uint32_t
// PaceTcpSocketBase::GetSndBufSize (void) const
// {
//     return m_txBuffer->MaxBufferSize ();
// }

// void
// PaceTcpSocketBase::SetRcvBufSize (uint32_t size)
// {
//     NS_LOG_FUNCTION (this << size);
//     uint32_t oldSize = GetRcvBufSize ();

//     m_tcb->m_rxBuffer->SetMaxBufferSize (size);

//     /* The size has (manually) increased. Actively inform the other end to prevent
//     * stale zero-window states.
//     */
//     if (oldSize < size && m_connected)
//     {
//         if (m_tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || m_tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
//         {
//             SendEmptyPacket (TcpHeader::ACK | TcpHeader::ECE);
//             NS_LOG_DEBUG (TcpSocketState::EcnStateName[m_tcb->m_ecnState] << " -> ECN_SENDING_ECE");
//             m_tcb->m_ecnState = TcpSocketState::ECN_SENDING_ECE;
//         }
//         else
//         {
//             SendEmptyPacket (TcpHeader::ACK);
//         }
//     }
// }

// uint32_t
// PaceTcpSocketBase::GetRcvBufSize (void) const
// {
//     return m_tcb->m_rxBuffer->MaxBufferSize ();
// }

// void
// PaceTcpSocketBase::SetSegSize (uint32_t size)
// {
//     NS_LOG_FUNCTION (this << size);
//     m_tcb->m_segmentSize = size;
//     m_txBuffer->SetSegmentSize (size);

//     NS_ABORT_MSG_UNLESS (m_state == CLOSED, "Cannot change segment size dynamically.");
// }

// uint32_t
// PaceTcpSocketBase::GetSegSize (void) const
// {
//     return m_tcb->m_segmentSize;
// }

// void
// PaceTcpSocketBase::SetConnTimeout (Time timeout)
// {
//     NS_LOG_FUNCTION (this << timeout);
//     m_cnTimeout = timeout;
// }

// Time
// PaceTcpSocketBase::GetConnTimeout (void) const
// {
//     return m_cnTimeout;
// }

// void
// PaceTcpSocketBase::SetSynRetries (uint32_t count)
// {
//     NS_LOG_FUNCTION (this << count);
//     m_synRetries = count;
// }

// uint32_t
// PaceTcpSocketBase::GetSynRetries (void) const
// {
//     return m_synRetries;
// }

// void
// PaceTcpSocketBase::SetDataRetries (uint32_t retries)
// {
//     NS_LOG_FUNCTION (this << retries);
//     m_dataRetries = retries;
// }

// uint32_t
// PaceTcpSocketBase::GetDataRetries (void) const
// {
//     NS_LOG_FUNCTION (this);
//     return m_dataRetries;
// }

// void
// PaceTcpSocketBase::SetDelAckTimeout (Time timeout)
// {
//     NS_LOG_FUNCTION (this << timeout);
//     m_delAckTimeout = timeout;
// }

// Time
// PaceTcpSocketBase::GetDelAckTimeout (void) const
// {
//     return m_delAckTimeout;
// }

// void
// PaceTcpSocketBase::SetDelAckMaxCount (uint32_t count)
// {
//     NS_LOG_FUNCTION (this << count);
//     m_delAckMaxCount = count;
// }

// uint32_t
// PaceTcpSocketBase::GetDelAckMaxCount (void) const
// {
//     return m_delAckMaxCount;
// }

// void
// PaceTcpSocketBase::SetTcpNoDelay (bool noDelay)
// {
//     NS_LOG_FUNCTION (this << noDelay);
//     m_noDelay = noDelay;
// }

// bool
// PaceTcpSocketBase::GetTcpNoDelay (void) const
// {
//     return m_noDelay;
// }

// void
// PaceTcpSocketBase::SetPersistTimeout (Time timeout)
// {
//     NS_LOG_FUNCTION (this << timeout);
//     m_persistTimeout = timeout;
// }

// Time
// PaceTcpSocketBase::GetPersistTimeout (void) const
// {
//     return m_persistTimeout;
// }

// bool
// PaceTcpSocketBase::SetAllowBroadcast (bool allowBroadcast)
// {
//     // Broadcast is not implemented. Return true only if allowBroadcast==false
//     return (!allowBroadcast);
// }

// bool
// PaceTcpSocketBase::GetAllowBroadcast (void) const
// {
//     return false;
// }

// Ptr<TcpSocketBase>
// PaceTcpSocketBase::Fork (void)
// {
//     return CopyObject<PaceTcpSocketBase> (this);
// }


// void 
// PaceTcpSocketBase::Update (void)
// {
//     if (m_recordlastReceRtt == m_lastReceRtt)
//     {
//         UpdateReceRtt (m_recordlastReceRtt, m_recordlastReceRtt);
//     }
//     m_recordlastReceRtt = m_lastReceRtt;

//     // sendCwnd ????????????, ??????????????? SimulationTcpSocketBase ???????????????


//     // if (m_recordlastSendRtt == m_lastSendRtt)
//     // {
//     //     UpdateSendRtt (m_recordlastReceRtt, m_recordlastReceRtt);
//     // }
// }

// void 
// PaceTcpSocketBase::UpdateSendRtt  (Time oldValue, Time newValue)
// {
//     m_lastSendRttTrace (oldValue, newValue);
// }

// void 
// PaceTcpSocketBase::UpdateReceRtt  (Time oldValue, Time newValue)
// {
//     m_lastReceRttTrace (oldValue, newValue);
// }

// void 
// PaceTcpSocketBase::UpdateSendCwnd (uint32_t oldValue, uint32_t newValue)
// {
//     m_SendCwndTrace (oldValue, newValue);
// }

// void 
// PaceTcpSocketBase::UpdateSendCongState (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue)
// {
//     m_sendCongStateTrace (oldValue, newValue);
// }

// void 
// PaceTcpSocketBase::Initialization (void)
// {
//     if (m_ReceRttEstimator == nullptr && m_rtt != nullptr)
//     {
//         m_ReceRttEstimator = m_rtt->Copy ();
//         m_ReceRttEstimator->Reset ();
//         m_minRxRTT = Time::Max ();
//     }
//     m_lastReceRtt.ConnectWithoutContext (MakeCallback (&PaceTcpSocketBase::UpdateReceRtt, this));


//     if (m_peerSocket == nullptr && m_isSimulationEnable)
//     {
//         m_peerSocket = CreateObject<SimulationTcpSocket> (*this);
//         m_peerSocket->m_state = TcpSocket::TcpStates_t::SYN_SENT;
//         m_peerSocket->m_sackEnabled = m_sackEnabled;
//         m_peerSocket->m_rcvWindShift = m_sndWindShift;
//         m_peerSocket->m_timestampEnabled = m_timestampEnabled;

//         bool ok;
//         ok = m_peerSocket->TraceConnectWithoutContext ("CongestionWindow",
//                                                        MakeCallback (&PaceTcpSocketBase::UpdateSendCwnd, this));   
//         ok = m_peerSocket->TraceConnectWithoutContext ("CongState",
//                                                        MakeCallback (&PaceTcpSocketBase::UpdateSendCongState, this));
//         ok = m_peerSocket->TraceConnectWithoutContext ("Tx",
//                                                        MakeCallback (&PaceTcpSocketBase::TraceSimSendPacketTx, this));
//         if (ok == false)
//             std::cout << "fail" << std::endl;
//     }

//     m_ackSentEvent.Cancel ();

//     // if (m_SendRttEstimator == nullptr && m_rtt != nullptr)
//     // {
//     //     m_SendRttEstimator = m_rtt->Copy ();
//     //     m_SendRttEstimator->Reset ();
//     // }
//     // m_lastSendRtt.ConnectWithoutContext (MakeCallback (&PaceTcpSocketBase::UpdateSendRtt, this));
// }


// void
// PaceTcpSocketBase::ProcessOptionTimestamp (const Ptr<const TcpOption> option,
//                                        const SequenceNumber32 &seq)
// {
//     TcpSocketBase::ProcessOptionTimestamp (option, seq);
    
//     Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);
//     m_timestampOffset = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetTimestamp ()) - TcpOptionTS::ElapsedTimeFromTsValue (m_timestampToEcho);
// }


// void 
// PaceTcpSocketBase::TraceSimSendPacketTx (Ptr<const Packet> packet, const TcpHeader& header, Ptr<const TcpSocketBase> base)
// {
//     m_simTxTrace (packet, header, base);
// }



// void PaceTcpSocketBase::AppendAck (Ptr<Packet> packet, TcpHeader& header)
// {
//     TcpPacket *pack = new TcpPacket ();
//     TcpTxItem *item = new TcpTxItem ();

//     item->m_packet = packet;
//     item->m_startSeq = m_currentMaxReceived;
//     // item->
//     pack->m_item = item;
//     pack->m_head = header;

//     m_ackList.insert (m_ackList.end (), pack);
// }

// void 
// PaceTcpSocketBase::SendAck ()
// {
//     TcpPacketList::iterator it = m_ackList.begin ();
//     if (it == m_ackList.end ())
//     {
//         return;
//     }
    
//     TcpPacket *item = *it;
//     Ptr<Packet> p = item->m_item->m_packet;

//     Ptr<TcpOptionTS> ts;

//     auto header = item->m_head;

//     for (auto i = header.m_options.begin (); i != header.m_options.end (); ++i)
//     {
//         if ((*i)->GetKind () == TcpOption::TS)
//         {
//             Ptr<TcpOptionTS> option = CreateObject<TcpOptionTS> ();
//             uint32_t timestamp = TcpOptionTS::NowToTsValue ();

//             // if (!m_ackSentList.empty ())
//             // {
//             //     TcpPacket* tcpPacket = *(m_ackSentList.rbegin ());
//             //     Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (tcpPacket->m_head.GetOption (TcpOption::TS));
//             //     uint32_t lastTimestamp = ts->GetTimestamp ();
//             //     if (timestamp == lastTimestamp)
//             //     {
//             //         ++timestamp;
//             //     }
//             // }

//             option->SetTimestamp (timestamp);
//             option->SetEcho (m_timestampToEcho);
            
//             header.m_options.erase (i);
//             header.m_options.insert (header.m_options.begin (), option);
//             break;
//         }
//     }
//     item->m_head = header;


//     // ???????????????
//     // ts->SetTimestamp (TcpOptionTS::NowToTsValue ());
//     m_txTrace (p, header, this);

//     if (m_endPoint != nullptr)
//     {
//         m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
//                            m_endPoint->GetPeerAddress (), m_boundnetdevice);
//     }
//     else
//     {
//         m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
//                           m_endPoint6->GetPeerAddress (), m_boundnetdevice);
//     }

//     item->m_item->m_lastSent = Simulator::Now ();
//     item->m_item->m_rateInfo.m_delivered = m_totalRxBytes;  // ????????????????????????????????????
//     m_ackList.erase (it);
//     m_ackSentList.insert (m_ackSentList.end (), item);

//     PaceAndScheduleAck (p, header, item->m_item->m_startSeq);
// }


// void
// PaceTcpSocketBase::PaceAndScheduleAck (Ptr<Packet> packet, TcpHeader header, SequenceNumber32 seq)
// {
//     uint32_t npacket;

//     // ?????? m_peerSocket ????????????????????? ACK
//     if (m_peerSocket != nullptr)
//     {
//         npacket = m_peerSocket->DoForwardUp (packet, header, m_tcb->m_lastRtt, m_timestampOffset);
        
//         m_lastCongState = m_peerSocket->m_tcb->m_congState;


//         // ??????????????????ACK???, ?????????????????????
//         switch (m_pacingState)
//         {
//         case PC_BEGIN:
//             if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_RECOVERY || m_peerSocket->m_tcb->m_cWnd > m_peerSocket->m_tcb->m_ssThresh.Get ())
//             {
//                 m_pacingState = PC_BGREC;
//                 m_constAckRate  = m_maxBandwidth;
                
//                 ResetRound ();
//                 CalculateGradient ();
//                 UpdateAckPacingRate ();

//                 std::cout << (int)index << " - enter PC_BGREC at " << Simulator::Now().GetSeconds () << std::endl;
//             }
//             break;

//         case PC_BGREC:
//             if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_OPEN)
//             {
//                 m_pacingState = PacingState::PC_PROBE;
//                 m_constAckRate = m_maxBandwidth;
//                 m_roundMaxBW [0] = m_roundMaxBW [1] = m_roundMaxBW[2] = 0;
//                 std::cout << (int)index << " - enter PC_PROBE at " << Simulator::Now().GetSeconds () << std::endl;
//             }
//             break;

//         case PC_PROBE:
//             if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
//             {
//                 m_pacingState = PC_RECVR;
//                 m_maxBandwidth = m_bandwidth;
//                 m_exitRecovery = false;
//                 m_enterDrain   = false;

//                 ResetRound ();
//                 CalculateGradient ();
//                 UpdateAckPacingRate ();

//                 std::cout << (int)index << " - enter PC_RECVR at " << Simulator::Now().GetSeconds () << std::endl;
//                 std::cout << (int)index << " - pacing rate: " <<  m_bandwidth.GetBitRate () * 1.0 / 1000 / 1000 << std::endl;
//             }
//             else if (m_noIncreaseRoundCnt == 3)
//             {
//                 // m_pacingState = PacingState::PC_DRAIN;
//                 m_pacingState = PC_STABL;
//                 m_pacingAckRate = DataRate (m_maxBandwidth.GetBitRate () * 0.7);
//                 std::cout << (int)index << " - enter PC_STABL at " << Simulator::Now().GetSeconds () << std::endl;
//             }                                                                       
//             break;

//         case PC_RECVR:
//             // if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_OPEN && m_exitRecovery)
//             if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_OPEN)
//             {
//                 m_pacingState = PC_STABL;
//                 std::cout << (int)index << " - enter PC_STABL at " << Simulator::Now().GetSeconds () << std::endl;
//             }
//             // else if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_OPEN && m_enterDrain)
//             // {
//             //     m_pacingState = PC_DRAIN;
//             //     m_exitDrain = false;
//             //     std::cout << (int)index << " - enter PC_DRAIN at " << Simulator::Now().GetSeconds () << std::endl;
//             // }
//             break;


//         // case PC_DRAIN:
//         //     if (m_ex)
//         case PC_STABL:
//             if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
//             {
//                 m_pacingState = PC_RECVR;
//                 m_maxBandwidth = m_bandwidth;
//                 m_exitRecovery = false;
//                 m_enterDrain   = false;

//                 ResetRound ();
//                 m_maxBwFilter.Reset (m_pacingAckRate, m_round);
//                 CalculateGradient ();
//                 UpdateAckPacingRate ();

//                 std::cout << (int)index << " - enter PC_RECVR at " << Simulator::Now().GetSeconds () << std::endl;
//                 std::cout << (int)index << " - pacing rate: " <<  m_bandwidth.GetBitRate () * 1.0 / 1000 / 1000 << std::endl;
//             }      
//             break;

//         default:
//             break;
//         }
//     }

//     if (m_peerSocket != nullptr && m_isAckPacingEnable == true)
//     {
//         Time intervaltime;
//         if (m_pacingState == PC_BEGIN || npacket == 0)
//         {
//             SendAck ();
//             return;
//         }
//         else if (m_pacingState == PC_PROBE || m_pacingState == PC_BGREC)
//         {
//             intervaltime = m_constAckRate.CalculateBytesTxTime (GetSegSize () / m_delAckMaxCount);
//         }
//         else {
//             intervaltime = m_pacingAckRate.CalculateBytesTxTime (npacket * GetSegSize ());
//             // intervaltime = std::min (intervaltime, 0.5 * m_peerSocket->m_rtt->GetEstimate ());
//         }
//         m_ackSentEvent = Simulator::Schedule (intervaltime, &PaceTcpSocketBase::SendAck, this);
//     }
//     else
//     {
//         m_ackSentEvent.Cancel ();
//     }
// }


// void
// PaceTcpSocketBase::CalculateBandwidth (Ptr<Packet> packet)
// {
//     Time now = Simulator::Now ();
//     m_totalRxBytes += packet->GetSize();

//     // ??????????????? ESTABLISHED ??????, ????????????????????? ACK ??????, ???????????????
//     if (m_state != ESTABLISHED || m_ackSentList.empty ())
//     {
//         return;
//     }

//     // ?????? packet ????????????, ???????????????????????????????????? m_tcb ???
//     uint32_t echo = m_tcb->m_rcvTimestampEchoReply;

//     // ?????? ACK ??????, ?????????????????? ACK ??????????????????, ACK ??????????????? tcpPacket
//     TcpPacket* tcpPacket = nullptr;
//     TcpPacketList::iterator it = m_ackSentList.begin ();
//     while (it != m_ackSentList.end ())
//     {
//         Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> ((*it)->m_head.GetOption (TcpOption::TS));
//         uint32_t timestamp = ts->GetTimestamp ();
//         if (timestamp < echo)
//         {
//             // ????????????????????????????????????, 
//             // ???????????????????????????????????????, ??????????????????, 
//             // ???????????????????????????????????????????????????????????????
//             DestoryTcpPacket (tcpPacket);
//             tcpPacket = *it;
//             it = m_ackList.erase (it);
//         }
//         else if (timestamp == echo)
//         {
//             // ???????????????????????????, ????????????
//             DestoryTcpPacket (tcpPacket);
//             tcpPacket = *it;
//             break;
//         }
//         else
//         {
//             // ?????????????????????, ???????????????????????????????????????, ????????????????????? (????????????????????????????????????, ???????????????)
//             // ??????????????????, ?????????????????????
//             if (tcpPacket != nullptr)
//             {
//                 m_ackList.push_front (tcpPacket);
//             }
//             else 
//             {
//                 tcpPacket = *it;
//             }
//             break;
//         }
//     }


//     // ??????????????????????????????????????????
//     if (tcpPacket != nullptr)
//     {
//         // ????????????, ???????????? m_maxBwFilter ?????????
//         Time interval  = now - tcpPacket->m_item->GetLastSent ();
//         if (interval < m_minRxRTT)
//         {
//             return;
//         }
        
//         // ???????????????????????????
//         uint64_t delta = m_totalRxBytes -  tcpPacket->m_item->m_rateInfo.m_delivered;
//         m_bandwidth = DataRate (delta * 8.0 / interval.GetSeconds ());
//         m_maxBandwidth  = std::max (m_maxBandwidth, m_bandwidth);
//         m_roundMaxBW[2] = std::max (m_roundMaxBW[2], m_bandwidth);      // ?????? roundMaxBW

//         // ?????? round
//         CalculateRound (tcpPacket);
        

//         // ??????????????? round max bandwidth
//         if (m_roundStart == true)
//         {
//             m_ltltRoundMxBw = m_lastRoundMxBw;
//             m_lastRoundMxBw = m_bandwidth; 
//         }
//         else
//         {
//             m_lastRoundMxBw = std::max (m_lastRoundMxBw, m_bandwidth);
//         }


//         // record ????????????????????????
//         if (dataRateFile.is_open())
//         {
//             dataRateFile << now.GetSeconds () << "\t" << m_bandwidth.GetBitRate() * 1.0 / 1000/ 1000 << std::endl;
//             maxRateFile  << now.GetSeconds () << "\t" << m_maxBwFilter.GetBest ().GetBitRate() * 1.0 / 1000/ 1000 << std::endl;    
//         }
//     } 
//     else
//     {
//         return;
//     }

   
// }


// void 
// PaceTcpSocketBase::CalculateRound (TcpPacket* tcpPacket)
// {
//     uint64_t delivered = tcpPacket->m_item->m_rateInfo.m_delivered;
//     if (delivered >= m_nextRound)
//     {
//         ResetRound ();
//     }
//     else
//     {
//         m_roundStart = false;
//     }
// }

// void 
// PaceTcpSocketBase::ResetRound ()
// {
//     ++m_round;
//     m_roundStart = true;
//     m_nextRound  = m_totalRxBytes;
//     m_roundMaxBW[0] = m_roundMaxBW[1];
//     m_roundMaxBW[1] = m_roundMaxBW[2];
//     m_roundMaxBW[2] = 0;
// }


// void 
// PaceTcpSocketBase::DestoryTcpPacket (TcpPacket* tcpPacket)
// {
//     if (tcpPacket != nullptr)
//     {
//         delete tcpPacket->m_item;
//         delete tcpPacket;
//         tcpPacket = nullptr;
//     }
// }


// void 
// PaceTcpSocketBase::CalculateGradient ()
// {
//     // ?????? RTT
//     if (m_roundStart == true)
//     {
//         // std::cout << index << "- Time: " << Simulator::Now().GetSecouns () << ", new round = " << m_round << std::endl; 
//         m_lastRoundRtt = m_currRoundRtt;
//         m_currRoundRtt = m_ReceRttEstimator->GetEstimate ();
//         m_gradient = (m_currRoundRtt.GetSeconds () - m_lastRoundRtt.GetSeconds ())/(m_maxRxRTT.GetSeconds () - m_minRxRTT.GetSeconds ());
//     }

// }


// void 
// PaceTcpSocketBase::UpdateAckPacingRate ()
// {
//     auto tmp = m_pacingAckRate;
//     auto tmp2 = m_constAckRate;

//     // ?????????bandwidth??????????????? m_pacingAckRate ?????????
//     switch (m_pacingState)
//     {
//     case PC_BEGIN:
//         m_pacingAckRate = DataRate (0);
//         m_constAckRate  = DataRate (0);
//         break;

//     case PC_PROBE:
//         if (m_roundStart)
//         {
//             m_pacingAckRate = DataRate (0);
//             m_constAckRate  = m_maxBandwidth;

//             if (m_roundMaxBW[0] < m_roundMaxBW[1])
//             {
//                 m_noIncreaseRoundCnt = 0;
//             }
//             else if (m_roundMaxBW[0].GetBitRate () != 0 && m_roundMaxBW[1].GetBitRate () != 0)
//             {
//                 ++m_noIncreaseRoundCnt;
//             }
//         }

//         break;

//     case PC_BGREC:
//     case PC_RECVR:
//         if (m_roundStart)
//         {
//             m_pacingAckRate = m_maxBandwidth;

//             // if (m_currRoundRtt < m_delayHigh)
//             // {
//             //     m_exitRecovery = true;
//             // }
//             // else
//             // {
//             //     m_enterDrain = true;
//             // }
//         }

//     case PC_STABL:
//         if (m_roundStart)
//         {
//             m_ltltRoundPacingRate = m_lastRoundPacingRate;
//             m_lastRoundPacingRate = m_pacingAckRate; 

//             // double baserate = m_maxBwFilter.GetBest ().GetBitRate () * 1.0;   // ?????????????????????????????????????????????bandwidth, ??????????????????, ??????
            
//             if (m_lastRoundRtt > m_delayHigh)
//             {
//                 double rate = std::min ((m_lastRoundRtt.GetSeconds () - m_delayHigh.GetSeconds ()) / m_minRxRTT.GetSeconds (), 0.3);
//                 rate = std::min (rate, 0.15);
//                 auto delta = rate * m_lastRoundMxBw.GetBitRate ();
//                 m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () - delta);
//             }
//             else if (m_lastRoundRtt > m_delayLow)
//             {
//                 m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () + m_step.GetBitRate ());
//             }
//             else
//             {
//                 auto delta = (m_delayLow.GetSeconds () - m_lastRoundRtt.GetSeconds ()) / m_minRxRTT.GetSeconds () * m_lastRoundMxBw.GetBitRate ();
//                 m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () + delta);
//             }


//             // if (m_lastRoundRtt > m_delayHigh)
//             // {
//             //     double rate = std::min ((m_lastRoundRtt.GetSeconds () - m_delayLow.GetSeconds ()) / m_minRxRTT.GetSeconds (), 0.3);
//             //     // rate = std::min (rate, 0.15);
//             //     auto delta = rate * m_lastRoundMxBw.GetBitRate ();
//             //     m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () - delta);
//             // }
//             // else if (m_lastRoundRtt > m_delayLow)
//             // {
//             //     m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () + m_step.GetBitRate ());
//             // }
//             // else
//             // {
//             //     auto delta = (m_delayLow.GetSeconds () - m_lastRoundRtt.GetSeconds ()) / m_minRxRTT.GetSeconds () * m_lastRoundMxBw.GetBitRate ();
//             //     m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () + delta);
//             // }


//             // if (flags[m_round % CYCLE] == 1)
//             // {
//             //     m_pacingAckRate = DataRate (baserate * 1.05);
//             // }
//             // else
//             // {
//             //     m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () * 0.98);
//             // }
//         }

//         if (m_bandwidth <= m_lastRoundPacingRate)
//         {
//             m_maxBwFilter.Update (m_bandwidth, m_round);
//         }
//         else
//         {
//             m_maxBwFilter.Update (m_lastRoundPacingRate, m_round);
//         }

//         break;

//     default:
//         break;
//     }
    

//     // record
//     if (m_roundStart == true)
//     {
//         if (m_pacingState == PC_PROBE)
//         {
//             std::cout << (int)index << " - time:" << Simulator::Now().GetSeconds () << " old: " << tmp2.GetBitRate () * 1.0 / 1000/ 1000 << ", new: " << m_constAckRate.GetBitRate () * 1.0 / 1000/ 1000 << std::endl;
//         }
//         else if (m_pacingState == PC_STABL)
//         {
//             std::cout << (int)index << " - time:" << Simulator::Now().GetSeconds () << " old: " << tmp.GetBitRate () * 1.0 / 1000/ 1000 << ", new: " << m_pacingAckRate.GetBitRate () * 1.0 / 1000/ 1000 << std::endl;

//         }
        
//         targetRateFile << (Simulator::Now() - TimeStep (1)).GetSeconds () << "\t" << tmp.GetBitRate() * 1.0 / 1000/ 1000 << std::endl;
//         targetRateFile << Simulator::Now().GetSeconds () << "\t" << m_pacingAckRate.GetBitRate() * 1.0 / 1000/ 1000 << std::endl;
//     }
// }

// // void
// // PaceTcpSocketBase::UpdateParameter ()
// // {
// // }

// uint8_t PaceTcpSocketBase::Index = 0;

// } // namespace ns3




/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-22 10:27:58
 * @LastEditTime: 2022-05-09 15:12:09
 * @LastEditors: Zhang Bochun
 * @Description: Receiver Pace ACK Tcp Socket Base
 * @FilePath: /ns-3.33/src/internet/model/pace-tcp-socket-base.cc
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
#include "pace-tcp-socket-base.h"
#include "simulation-tcp-socket.h"

#include <unistd.h>
#include <memory.h>
#include <math.h>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PaceTcpSocketBase");

NS_OBJECT_ENSURE_REGISTERED (PaceTcpSocketBase);

TypeId
PaceTcpSocketBase::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::PaceTcpSocketBase")
        .SetParent<TcpSocketBase> ()
        .SetGroupName ("Internet")
        .AddConstructor<PaceTcpSocketBase> ()

        // ????????????
        .AddTraceSource ("SendRTT",
                         "Last PeerRTT sample",
                         MakeTraceSourceAccessor (&PaceTcpSocketBase::m_lastSendRttTrace),
                         "ns3::TracedValueCallback::Time")
        .AddTraceSource ("ReceRTT",
                         "Last PeerRTT sample",
                         MakeTraceSourceAccessor (&PaceTcpSocketBase::m_lastReceRttTrace),
                         "ns3::TracedValueCallback::Time") 
        .AddTraceSource ("SendCwnd",
                         "Last PeerRTT sample",
                         MakeTraceSourceAccessor (&PaceTcpSocketBase::m_SendCwndTrace),
                         "ns3::TracedValueCallback::Time")
        .AddTraceSource ("SendCongState",
                         "TCP Congestion machine state",
                         MakeTraceSourceAccessor (&PaceTcpSocketBase::m_sendCongStateTrace),
                         "ns3::TcpSocketState::TcpCongStatesTracedValueCallback")
        .AddTraceSource ("SimTx",
                         "Send tcp packet to IP protocol",
                         MakeTraceSourceAccessor (&PaceTcpSocketBase::m_simTxTrace),
                         "ns3::TcpSocketBase::TcpTxRxTracedCallback")
    ;
    return tid;
}

TypeId
PaceTcpSocketBase::GetInstanceTypeId () const
{
    return PaceTcpSocketBase::GetTypeId ();
}

PaceTcpSocketBase::PaceTcpSocketBase (void)
  : TcpSocketBase (),
    m_maxBwFilter (CYCLE, DataRate(0), 0)
{
    std::cout << GetTypeId () << std::endl;
}

PaceTcpSocketBase::PaceTcpSocketBase (const PaceTcpSocketBase& sock)
  : TcpSocketBase (sock),
    m_maxBwFilter (CYCLE, DataRate(0), 0)
{
    std::cout << GetTypeId () << std::endl;
}
  

PaceTcpSocketBase::~PaceTcpSocketBase (void)
{
}



/* Associate a node with this TCP socket */
void
PaceTcpSocketBase::SetNode (Ptr<Node> node)
{
    m_node = node;
}

/* Associate the L4 protocol (e.g. mux/demux) with this socket */
void
PaceTcpSocketBase::SetTcp (Ptr<TcpL4Protocol> tcp)
{
    m_tcp = tcp;
}

/* Set an RTT estimator with this socket */
void
PaceTcpSocketBase::SetRtt (Ptr<RttEstimator> rtt)
{
    m_rtt = rtt;
}

/* Inherit from Socket class: Returns error code */
enum Socket::SocketErrno
PaceTcpSocketBase::GetErrno (void) const
{
    return m_errno;
}

/* Inherit from Socket class: Returns socket type, NS3_SOCK_STREAM */
enum Socket::SocketType
PaceTcpSocketBase::GetSocketType (void) const
{
    return NS3_SOCK_STREAM;
}

/* Inherit from Socket class: Returns associated node */
Ptr<Node>
PaceTcpSocketBase::GetNode (void) const
{
    return m_node;
}

/* Inherit from Socket class: Bind socket to an end-point in TcpL4Protocol */
int
PaceTcpSocketBase::Bind (void)
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
PaceTcpSocketBase::Bind6 (void)
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
PaceTcpSocketBase::Bind (const Address &address)
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

    NS_LOG_LOGIC ("PaceTcpSocketBase " << this << " got an endpoint: " << m_endPoint);

    return SetupCallback ();
}

void
PaceTcpSocketBase::SetInitialSSThresh (uint32_t threshold)
{
    NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || threshold == m_tcb->m_initialSsThresh,
                          "PaceTcpSocketBase::SetSSThresh() cannot change initial ssThresh after connection started.");

    m_tcb->m_initialSsThresh = threshold;
}

uint32_t
PaceTcpSocketBase::GetInitialSSThresh (void) const
{
    return m_tcb->m_initialSsThresh;
}

void
PaceTcpSocketBase::SetInitialCwnd (uint32_t cwnd)
{
    NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || cwnd == m_tcb->m_initialCWnd,
                          "PaceTcpSocketBase::SetInitialCwnd() cannot change initial cwnd after connection started.");

    m_tcb->m_initialCWnd = cwnd;
}

uint32_t
PaceTcpSocketBase::GetInitialCwnd (void) const
{
    return m_tcb->m_initialCWnd;
}

/* Inherit from Socket class: Initiate connection to a remote address:port */
int
PaceTcpSocketBase::Connect (const Address & address)
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
PaceTcpSocketBase::Listen (void)
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
PaceTcpSocketBase::Close (void)
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
PaceTcpSocketBase::ShutdownSend (void)
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
PaceTcpSocketBase::ShutdownRecv (void)
{
    NS_LOG_FUNCTION (this);
    m_shutdownRecv = true;
    return 0;
}

/* Inherit from Socket class: Send a packet. Parameter flags is not used.
    Packet has no TCP header. Invoked by upper-layer application */
int
PaceTcpSocketBase::Send (Ptr<Packet> p, uint32_t flags)
{
    NS_LOG_FUNCTION (this << p);
    NS_ABORT_MSG_IF (flags, "use of flags is not supported in PaceTcpSocketBase::Send()");
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
                                                              &PaceTcpSocketBase::SendPendingData,
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

/* Inherit from Socket class: In PaceTcpSocketBase, it is same as Send() call */
int
PaceTcpSocketBase::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
{
    NS_UNUSED (address);
    return Send (p, flags); // SendTo() and Send() are the same
}

/* Inherit from Socket class: Return data to upper-layer application. Parameter flags
   is not used. Data is returned as a packet of size no larger than maxSize */
Ptr<Packet>
PaceTcpSocketBase::Recv (uint32_t maxSize, uint32_t flags)
{
    NS_LOG_FUNCTION (this);
    NS_ABORT_MSG_IF (flags, "use of flags is not supported in PaceTcpSocketBase::Recv()");
    if (m_tcb->m_rxBuffer->Size () == 0 && m_state == CLOSE_WAIT)
    {
        return Create<Packet> (); // Send EOF on connection close
    }
    Ptr<Packet> outPacket = m_tcb->m_rxBuffer->Extract (maxSize);
    return outPacket;
}

/* Inherit from Socket class: Recv and return the remote's address */
Ptr<Packet>
PaceTcpSocketBase::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
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
PaceTcpSocketBase::GetTxAvailable (void) const
{
    NS_LOG_FUNCTION (this);
    return m_txBuffer->Available ();
}

/* Inherit from Socket class: Get the max number of bytes an app can read */
uint32_t
PaceTcpSocketBase::GetRxAvailable (void) const
{
    NS_LOG_FUNCTION (this);
    return m_tcb->m_rxBuffer->Available ();
}

/* Inherit from Socket class: Return local address:port */
int
PaceTcpSocketBase::GetSockName (Address &address) const
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
PaceTcpSocketBase::GetPeerName (Address &address) const
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
PaceTcpSocketBase::BindToNetDevice (Ptr<NetDevice> netdevice)
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
PaceTcpSocketBase::DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
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

    // ???????????? Put into SimulationTcpSocket's txBuffer
    if (packet->GetSize () > 0)
    {
        m_peerSocket->FillDataPacket (packet, tcpHeader);
        m_currentMaxReceived = std::max (tcpHeader.GetSequenceNumber(), m_currentMaxReceived);
    }

    // ????????????????????? packet, ????????????, ????????? Gradient
    CalculateBandwidth (packet);        // ????????? Round
    CalculateGradient ();

    // ?????? PacingRate
    // UpdateParameter ();
    UpdateAckPacingRate ();


    if (m_rWnd.Get () == 0 && m_persistEvent.IsExpired ())
    {   // Zero window: Enter persist state to send 1 byte to probe
        NS_LOG_LOGIC (this << " Enter zerowindow persist state");
        NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                      (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
        m_retxEvent.Cancel ();
        NS_LOG_LOGIC ("Schedule persist timeout at time " <<
                      Simulator::Now ().GetSeconds () << " to expire at time " <<
                      (Simulator::Now () + m_persistTimeout).GetSeconds ());
        m_persistEvent = Simulator::Schedule (m_persistTimeout, &PaceTcpSocketBase::PersistTimeout, this);
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


    // ????????????, ?????????????????????
    Update ();
}


/* Process the newly received ACK */
void
PaceTcpSocketBase::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
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
PaceTcpSocketBase::ProcessAck(const SequenceNumber32 &ackNumber, bool scoreboardUpdated,
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
PaceTcpSocketBase::SendEmptyPacket (uint8_t flags)
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


    AppendAck (p, header);
    if (m_ackSentEvent.IsExpired())
    {
        SendAck ();
    }


    if (m_retxEvent.IsExpired () && (hasSyn || hasFin) && !isAck )
    { // Retransmit SYN / SYN+ACK / FIN / FIN+ACK to guard against lost
        NS_LOG_LOGIC ("Schedule retransmission timeout at time "
                      << Simulator::Now ().GetSeconds () << " to expire at time "
                      << (Simulator::Now () + m_rto.Get ()).GetSeconds ());
        m_retxEvent = Simulator::Schedule (m_rto, &PaceTcpSocketBase::SendEmptyPacket, this, flags);
    }

}


/* This function is called only if a SYN received in LISTEN state. After
   PaceTcpSocketBase cloned, allocate a new end point to handle the incoming
   connection and send a SYN+ACK to complete the handshake. */
void
PaceTcpSocketBase::CompleteFork (Ptr<Packet> p, const TcpHeader& h,
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


    // ????????????
    // ???????????????
    Initialization ();


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

    ++Index;
    index = Index;

    char buf[FILENAME_MAX];
    std::string path = std::string (getcwd(buf, FILENAME_MAX))+ "/traces/info/";
    if (!dataRateFile.is_open())
    {
        std::string filename = path + std::to_string(index) + "_" + "datarate.txt";
        dataRateFile.open (filename.c_str(), std::fstream::out);
    }
    if (!maxRateFile.is_open())
    {
        std::string filename = path + std::to_string(index) + "_" + "maxrate.txt";
        maxRateFile.open (filename.c_str(), std::fstream::out);
    }
    if (!targetRateFile.is_open())
    {
        std::string filename = path + std::to_string(index) + "_" + "targetrate.txt";
        targetRateFile.open (filename.c_str(), std::fstream::out);
    }
}


/* Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
    TCP header, and send to TcpL4Protocol */
uint32_t
PaceTcpSocketBase::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
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
        m_retxEvent = Simulator::Schedule (m_rto, &PaceTcpSocketBase::ReTxTimeout, this);
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
        Simulator::ScheduleNow (&PaceTcpSocketBase::NotifyDataSent, this,
                                (seq + sz - m_tcb->m_highTxMark.Get ()));
    }
    // Update highTxMark
    m_tcb->m_highTxMark = std::max (seq + sz, m_tcb->m_highTxMark.Get ());
    return sz;
}

void
PaceTcpSocketBase::UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
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
PaceTcpSocketBase::UnAckDataCount () const
{
    return m_tcb->m_highTxMark - m_txBuffer->HeadSequence ();
}

uint32_t
PaceTcpSocketBase::BytesInFlight () const
{
    uint32_t bytesInFlight = m_txBuffer->BytesInFlight ();
    // Ugly, but we are not modifying the state; m_bytesInFlight is used
    // only for tracing purpose.
    m_tcb->m_bytesInFlight = bytesInFlight;

    NS_LOG_DEBUG ("Returning calculated bytesInFlight: " << bytesInFlight);
    return bytesInFlight;
}

uint32_t
PaceTcpSocketBase::Window (void) const
{
    return std::min (m_rWnd.Get (), m_tcb->m_cWnd.Get ());
}

uint32_t
PaceTcpSocketBase::AvailableWindow () const
{
    uint32_t win = Window ();             // Number of bytes allowed to be outstanding
    uint32_t inflight = BytesInFlight (); // Number of outstanding bytes
    return (inflight > win) ? 0 : win - inflight;
}

uint16_t
PaceTcpSocketBase::AdvertisedWindowSize (bool scale) const
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
        const_cast<PaceTcpSocketBase*> (this)->m_advWnd = w;
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
PaceTcpSocketBase::ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader)
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
                                                &PaceTcpSocketBase::DelAckTimeout, this);
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
PaceTcpSocketBase::EstimateRtt (const TcpHeader& tcpHeader)
{
    SequenceNumber32 ackSeq = tcpHeader.GetAckNumber ();
    Time m = Time (0.0);

    // ????????????
    if (m_timestampEnabled && tcpHeader.HasOption (TcpOption::TS))
    {
        Ptr<const TcpOptionTS> ts;
        ts = DynamicCast<const TcpOptionTS> (tcpHeader.GetOption (TcpOption::TS));

        if (ts->GetEcho () != m_lastEcho)
        {
            m_lastEcho = ts->GetEcho ();

            Time rtt = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetEcho ());   // all

            m_ReceRttEstimator->Measurement (rtt);
            m_lastReceRtt = m_ReceRttEstimator->GetEstimate();
            // m_minRxRTT = std::min (m_lastReceRtt.Get (), m_minRxRTT);
            // m_maxRxRTT = std::max (rtt, m_maxRxRTT);

            m_minRxRTT = Time ("100ms");
            m_maxRxRTT = Time ("150ms");

            m_delayLow  = (m_maxRxRTT - m_minRxRTT) * m_alpha + m_minRxRTT;
            m_delayHigh = (m_maxRxRTT - m_minRxRTT) * m_beta  + m_minRxRTT;
        }

    }

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
PaceTcpSocketBase::NewAck (SequenceNumber32 const& ack, bool resetRTO)
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
        m_retxEvent = Simulator::Schedule (m_rto, &PaceTcpSocketBase::ReTxTimeout, this);
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
PaceTcpSocketBase::ReTxTimeout ()
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

    std::cout << "Loss Event occur at time: " << Simulator::Now ().GetSeconds () << std::endl; 

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
PaceTcpSocketBase::DelAckTimeout (void)
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
PaceTcpSocketBase::LastAckTimeout (void)
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
        NS_LOG_LOGIC ("PaceTcpSocketBase " << this << " rescheduling LATO1");
        Time lastRto = m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation () * 4);
        m_lastAckEvent = Simulator::Schedule (lastRto, &PaceTcpSocketBase::LastAckTimeout, this);
    }
}

// Send 1-byte data to probe for the window size at the receiver when
// the local knowledge tells that the receiver has zero window size
// C.f.: RFC793 p.42, RFC1112 sec.4.2.2.17
void
PaceTcpSocketBase::PersistTimeout ()
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
    m_persistEvent = Simulator::Schedule (m_persistTimeout, &PaceTcpSocketBase::PersistTimeout, this);
}


/* Below are the attribute get/set functions */

void
PaceTcpSocketBase::SetSndBufSize (uint32_t size)
{
    NS_LOG_FUNCTION (this << size);
    m_txBuffer->SetMaxBufferSize (size);
}

uint32_t
PaceTcpSocketBase::GetSndBufSize (void) const
{
    return m_txBuffer->MaxBufferSize ();
}

void
PaceTcpSocketBase::SetRcvBufSize (uint32_t size)
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
PaceTcpSocketBase::GetRcvBufSize (void) const
{
    return m_tcb->m_rxBuffer->MaxBufferSize ();
}

void
PaceTcpSocketBase::SetSegSize (uint32_t size)
{
    NS_LOG_FUNCTION (this << size);
    m_tcb->m_segmentSize = size;
    m_txBuffer->SetSegmentSize (size);

    NS_ABORT_MSG_UNLESS (m_state == CLOSED, "Cannot change segment size dynamically.");
}

uint32_t
PaceTcpSocketBase::GetSegSize (void) const
{
    return m_tcb->m_segmentSize;
}

void
PaceTcpSocketBase::SetConnTimeout (Time timeout)
{
    NS_LOG_FUNCTION (this << timeout);
    m_cnTimeout = timeout;
}

Time
PaceTcpSocketBase::GetConnTimeout (void) const
{
    return m_cnTimeout;
}

void
PaceTcpSocketBase::SetSynRetries (uint32_t count)
{
    NS_LOG_FUNCTION (this << count);
    m_synRetries = count;
}

uint32_t
PaceTcpSocketBase::GetSynRetries (void) const
{
    return m_synRetries;
}

void
PaceTcpSocketBase::SetDataRetries (uint32_t retries)
{
    NS_LOG_FUNCTION (this << retries);
    m_dataRetries = retries;
}

uint32_t
PaceTcpSocketBase::GetDataRetries (void) const
{
    NS_LOG_FUNCTION (this);
    return m_dataRetries;
}

void
PaceTcpSocketBase::SetDelAckTimeout (Time timeout)
{
    NS_LOG_FUNCTION (this << timeout);
    m_delAckTimeout = timeout;
}

Time
PaceTcpSocketBase::GetDelAckTimeout (void) const
{
    return m_delAckTimeout;
}

void
PaceTcpSocketBase::SetDelAckMaxCount (uint32_t count)
{
    NS_LOG_FUNCTION (this << count);
    m_delAckMaxCount = count;
}

uint32_t
PaceTcpSocketBase::GetDelAckMaxCount (void) const
{
    return m_delAckMaxCount;
}

void
PaceTcpSocketBase::SetTcpNoDelay (bool noDelay)
{
    NS_LOG_FUNCTION (this << noDelay);
    m_noDelay = noDelay;
}

bool
PaceTcpSocketBase::GetTcpNoDelay (void) const
{
    return m_noDelay;
}

void
PaceTcpSocketBase::SetPersistTimeout (Time timeout)
{
    NS_LOG_FUNCTION (this << timeout);
    m_persistTimeout = timeout;
}

Time
PaceTcpSocketBase::GetPersistTimeout (void) const
{
    return m_persistTimeout;
}

bool
PaceTcpSocketBase::SetAllowBroadcast (bool allowBroadcast)
{
    // Broadcast is not implemented. Return true only if allowBroadcast==false
    return (!allowBroadcast);
}

bool
PaceTcpSocketBase::GetAllowBroadcast (void) const
{
    return false;
}

Ptr<TcpSocketBase>
PaceTcpSocketBase::Fork (void)
{
    return CopyObject<PaceTcpSocketBase> (this);
}


void 
PaceTcpSocketBase::Update (void)
{
    if (m_recordlastReceRtt == m_lastReceRtt)
    {
        UpdateReceRtt (m_recordlastReceRtt, m_recordlastReceRtt);
    }
    m_recordlastReceRtt = m_lastReceRtt;

    // sendCwnd ????????????, ??????????????? SimulationTcpSocketBase ???????????????


    // if (m_recordlastSendRtt == m_lastSendRtt)
    // {
    //     UpdateSendRtt (m_recordlastReceRtt, m_recordlastReceRtt);
    // }
}

void 
PaceTcpSocketBase::UpdateSendRtt  (Time oldValue, Time newValue)
{
    m_lastSendRttTrace (oldValue, newValue);
}

void 
PaceTcpSocketBase::UpdateReceRtt  (Time oldValue, Time newValue)
{
    m_lastReceRttTrace (oldValue, newValue);
}

void 
PaceTcpSocketBase::UpdateSendCwnd (uint32_t oldValue, uint32_t newValue)
{
    m_SendCwndTrace (oldValue, newValue);
}

void 
PaceTcpSocketBase::UpdateSendCongState (TcpSocketState::TcpCongState_t oldValue, TcpSocketState::TcpCongState_t newValue)
{
    m_sendCongStateTrace (oldValue, newValue);
}

void 
PaceTcpSocketBase::Initialization (void)
{
    if (m_ReceRttEstimator == nullptr && m_rtt != nullptr)
    {
        m_ReceRttEstimator = m_rtt->Copy ();
        m_ReceRttEstimator->Reset ();
        m_minRxRTT = Time::Max ();
    }
    m_lastReceRtt.ConnectWithoutContext (MakeCallback (&PaceTcpSocketBase::UpdateReceRtt, this));


    if (m_peerSocket == nullptr && m_isSimulationEnable)
    {
        m_peerSocket = CreateObject<SimulationTcpSocket> (*this);
        m_peerSocket->m_state = TcpSocket::TcpStates_t::SYN_SENT;
        m_peerSocket->m_sackEnabled = m_sackEnabled;
        m_peerSocket->m_rcvWindShift = m_sndWindShift;
        m_peerSocket->m_timestampEnabled = m_timestampEnabled;

        bool ok;
        ok = m_peerSocket->TraceConnectWithoutContext ("CongestionWindow",
                                                       MakeCallback (&PaceTcpSocketBase::UpdateSendCwnd, this));   
        ok = m_peerSocket->TraceConnectWithoutContext ("CongState",
                                                       MakeCallback (&PaceTcpSocketBase::UpdateSendCongState, this));
        ok = m_peerSocket->TraceConnectWithoutContext ("Tx",
                                                       MakeCallback (&PaceTcpSocketBase::TraceSimSendPacketTx, this));
        if (ok == false)
            std::cout << "fail" << std::endl;
    }

    m_ackSentEvent.Cancel ();

    // if (m_SendRttEstimator == nullptr && m_rtt != nullptr)
    // {
    //     m_SendRttEstimator = m_rtt->Copy ();
    //     m_SendRttEstimator->Reset ();
    // }
    // m_lastSendRtt.ConnectWithoutContext (MakeCallback (&PaceTcpSocketBase::UpdateSendRtt, this));
}


void
PaceTcpSocketBase::ProcessOptionTimestamp (const Ptr<const TcpOption> option,
                                       const SequenceNumber32 &seq)
{
    TcpSocketBase::ProcessOptionTimestamp (option, seq);
    
    Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);
    m_timestampOffset = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetTimestamp ()) - TcpOptionTS::ElapsedTimeFromTsValue (m_timestampToEcho);
}


void 
PaceTcpSocketBase::TraceSimSendPacketTx (Ptr<const Packet> packet, const TcpHeader& header, Ptr<const TcpSocketBase> base)
{
    m_simTxTrace (packet, header, base);
}



void PaceTcpSocketBase::AppendAck (Ptr<Packet> packet, TcpHeader& header)
{
    TcpPacket *pack = new TcpPacket ();
    TcpTxItem *item = new TcpTxItem ();

    item->m_packet = packet;
    item->m_startSeq = m_currentMaxReceived;
    // item->
    pack->m_item = item;
    pack->m_head = header;

    m_ackList.insert (m_ackList.end (), pack);
}

void 
PaceTcpSocketBase::SendAck ()
{
    TcpPacketList::iterator it = m_ackList.begin ();
    if (it == m_ackList.end ())
    {
        return;
    }
    
    TcpPacket *item = *it;
    Ptr<Packet> p = item->m_item->m_packet;

    Ptr<TcpOptionTS> ts;

    auto header = item->m_head;

    for (auto i = header.m_options.begin (); i != header.m_options.end (); ++i)
    {
        if ((*i)->GetKind () == TcpOption::TS)
        {
            Ptr<TcpOptionTS> option = CreateObject<TcpOptionTS> ();
            uint32_t timestamp = TcpOptionTS::NowToTsValue ();

            // if (!m_ackSentList.empty ())
            // {
            //     TcpPacket* tcpPacket = *(m_ackSentList.rbegin ());
            //     Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (tcpPacket->m_head.GetOption (TcpOption::TS));
            //     uint32_t lastTimestamp = ts->GetTimestamp ();
            //     if (timestamp == lastTimestamp)
            //     {
            //         ++timestamp;
            //     }
            // }

            option->SetTimestamp (timestamp);
            option->SetEcho (m_timestampToEcho);
            
            header.m_options.erase (i);
            header.m_options.insert (header.m_options.begin (), option);
            break;
        }
    }
    item->m_head = header;


    // ???????????????
    // ts->SetTimestamp (TcpOptionTS::NowToTsValue ());
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

    item->m_item->m_lastSent = Simulator::Now ();
    item->m_item->m_rateInfo.m_delivered = m_totalRxBytes;  // ????????????????????????????????????
    m_ackList.erase (it);
    m_ackSentList.insert (m_ackSentList.end (), item);

    PaceAndScheduleAck (p, header, item->m_item->m_startSeq);
}


void
PaceTcpSocketBase::PaceAndScheduleAck (Ptr<Packet> packet, TcpHeader header, SequenceNumber32 seq)
{
    uint32_t npacket;

    // ?????? m_peerSocket ????????????????????? ACK
    if (m_peerSocket != nullptr)
    {
        npacket = m_peerSocket->DoForwardUp (packet, header, m_tcb->m_lastRtt, m_timestampOffset);
        
        m_lastCongState = m_peerSocket->m_tcb->m_congState;


        // ??????????????????ACK???, ?????????????????????
        switch (m_pacingState)
        {
        case PC_BEGIN:
            if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_RECOVERY || m_peerSocket->m_tcb->m_cWnd > m_peerSocket->m_tcb->m_ssThresh.Get ())
            {
                m_pacingState = PC_BGREC;
                m_constAckRate  = m_maxBandwidth;
                
                ResetRound ();
                CalculateGradient ();
                UpdateAckPacingRate ();

                std::cout << (int)index << " - enter PC_BGREC at " << Simulator::Now().GetSeconds () << std::endl;
            }
            break;

        case PC_BGREC:
            if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_OPEN)
            {
                m_pacingState = PacingState::PC_PROBE;
                m_constAckRate = m_maxBandwidth;
                m_roundMaxBW [0] = m_roundMaxBW [1] = m_roundMaxBW[2] = 0;
                std::cout << (int)index << " - enter PC_PROBE at " << Simulator::Now().GetSeconds () << std::endl;
            }
            break;

        case PC_PROBE:
            if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
            {
                m_pacingState = PC_RECVR;
                m_maxBandwidth = m_bandwidth;
                m_exitRecovery = false;
                m_enterDrain   = false;

                ResetRound ();
                CalculateGradient ();
                UpdateAckPacingRate ();

                std::cout << (int)index << " - enter PC_RECVR at " << Simulator::Now().GetSeconds () << std::endl;
                std::cout << (int)index << " - pacing rate: " <<  m_bandwidth.GetBitRate () * 1.0 / 1000 / 1000 << std::endl;
            }
            else if (m_noIncreaseRoundCnt == 3)
            {
                // m_pacingState = PacingState::PC_DRAIN;
                m_pacingState = PC_STABL;
                m_pacingAckRate = DataRate (m_maxBandwidth.GetBitRate () * 0.7);
                std::cout << (int)index << " - enter PC_STABL at " << Simulator::Now().GetSeconds () << std::endl;
            }                                                                       
            break;

        case PC_RECVR:
            // if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_OPEN && m_exitRecovery)
            if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_OPEN)
            {
                m_pacingState = PC_STABL;
                std::cout << (int)index << " - enter PC_STABL at " << Simulator::Now().GetSeconds () << std::endl;
            }
            // else if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_OPEN && m_enterDrain)
            // {
            //     m_pacingState = PC_DRAIN;
            //     m_exitDrain = false;
            //     std::cout << (int)index << " - enter PC_DRAIN at " << Simulator::Now().GetSeconds () << std::endl;
            // }
            break;


        // case PC_DRAIN:
        //     if (m_ex)
        case PC_STABL:
            if (m_peerSocket->m_tcb->m_congState == TcpSocketState::CA_RECOVERY)
            {
                m_pacingState = PC_RECVR;
                m_maxBandwidth = m_bandwidth;
                m_exitRecovery = false;
                m_enterDrain   = false;

                ResetRound ();
                m_maxBwFilter.Reset (m_pacingAckRate, m_round);
                CalculateGradient ();
                UpdateAckPacingRate ();

                std::cout << (int)index << " - enter PC_RECVR at " << Simulator::Now().GetSeconds () << std::endl;
                std::cout << (int)index << " - pacing rate: " <<  m_bandwidth.GetBitRate () * 1.0 / 1000 / 1000 << std::endl;
            }      
            break;

        default:
            break;
        }
    }

    if (m_peerSocket != nullptr && m_isAckPacingEnable == true)
    {
        Time intervaltime;
        if (m_pacingState == PC_BEGIN || npacket == 0)
        {
            SendAck ();
            return;
        }
        else if (m_pacingState == PC_PROBE || m_pacingState == PC_BGREC)
        {
            intervaltime = m_constAckRate.CalculateBytesTxTime (GetSegSize () / m_delAckMaxCount);
        }
        else {
            intervaltime = m_pacingAckRate.CalculateBytesTxTime (npacket * GetSegSize ());
        }
        intervaltime = std::min (intervaltime, m_minRxRTT);
        m_ackSentEvent = Simulator::Schedule (intervaltime, &PaceTcpSocketBase::SendAck, this);
    }
    else
    {
        m_ackSentEvent.Cancel ();
    }
}


void
PaceTcpSocketBase::CalculateBandwidth (Ptr<Packet> packet)
{
    Time now = Simulator::Now ();
    m_totalRxBytes += packet->GetSize();

    // ??????????????? ESTABLISHED ??????, ????????????????????? ACK ??????, ???????????????
    if (m_state != ESTABLISHED || m_ackSentList.empty ())
    {
        return;
    }

    // ?????? packet ????????????, ???????????????????????????????????? m_tcb ???
    uint32_t echo = m_tcb->m_rcvTimestampEchoReply;

    // ?????? ACK ??????, ?????????????????? ACK ??????????????????, ACK ??????????????? tcpPacket
    TcpPacket* tcpPacket = nullptr;
    TcpPacketList::iterator it = m_ackSentList.begin ();
    while (it != m_ackSentList.end ())
    {
        Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> ((*it)->m_head.GetOption (TcpOption::TS));
        uint32_t timestamp = ts->GetTimestamp ();
        if (timestamp < echo)
        {
            // ????????????????????????????????????, 
            // ???????????????????????????????????????, ??????????????????, 
            // ???????????????????????????????????????????????????????????????
            DestoryTcpPacket (tcpPacket);
            tcpPacket = *it;
            it = m_ackList.erase (it);
        }
        else if (timestamp == echo)
        {
            // ???????????????????????????, ????????????
            DestoryTcpPacket (tcpPacket);
            tcpPacket = *it;
            break;
        }
        else
        {
            // ?????????????????????, ???????????????????????????????????????, ????????????????????? (????????????????????????????????????, ???????????????)
            // ??????????????????, ?????????????????????
            if (tcpPacket != nullptr)
            {
                m_ackList.push_front (tcpPacket);
            }
            else 
            {
                tcpPacket = *it;
            }
            break;
        }
    }


    // ??????????????????????????????????????????
    if (tcpPacket != nullptr)
    {
        // ????????????, ???????????? m_maxBwFilter ?????????
        Time interval  = now - tcpPacket->m_item->GetLastSent ();
        if (interval < m_minRxRTT)
        {
            return;
        }
        
        // ???????????????????????????
        uint64_t delta = m_totalRxBytes -  tcpPacket->m_item->m_rateInfo.m_delivered;
        m_bandwidth = DataRate (delta * 8.0 / interval.GetSeconds ());
        m_maxBandwidth  = std::max (m_maxBandwidth, m_bandwidth);
        m_roundMaxBW[2] = std::max (m_roundMaxBW[2], m_bandwidth);      // ?????? roundMaxBW

        // ?????? round
        CalculateRound (tcpPacket);
        m_maxBwFilter.Update (m_bandwidth, m_round);
        

        // ??????????????? round max bandwidth
        if (m_roundStart == true)
        {
            m_ltltRoundMxBw = m_lastRoundMxBw;
            m_lastRoundMxBw = m_bandwidth; 
        }
        else
        {
            m_lastRoundMxBw = std::max (m_lastRoundMxBw, m_bandwidth);
        }


        // record ????????????????????????
        if (dataRateFile.is_open())
        {
            dataRateFile << now.GetSeconds () << "\t" << m_bandwidth.GetBitRate() * 1.0 / 1000/ 1000 << std::endl;
            // maxRateFile  << now.GetSeconds () << "\t" << m_maxBwFilter.GetBest ().GetBitRate() * 1.0 / 1000/ 1000 << std::endl;    
        }
    } 
    else
    {
        return;
    }

   
}


void 
PaceTcpSocketBase::CalculateRound (TcpPacket* tcpPacket)
{
    uint64_t delivered = tcpPacket->m_item->m_rateInfo.m_delivered;
    if (delivered >= m_nextRound)
    {
        ResetRound ();
    }
    else
    {
        m_roundStart = false;
    }
}

void 
PaceTcpSocketBase::ResetRound ()
{
    ++m_round;
    m_roundStart = true;
    m_nextRound  = m_totalRxBytes;
    m_roundMaxBW[0] = m_roundMaxBW[1];
    m_roundMaxBW[1] = m_roundMaxBW[2];
    m_roundMaxBW[2] = 0;
}


void 
PaceTcpSocketBase::DestoryTcpPacket (TcpPacket* tcpPacket)
{
    if (tcpPacket != nullptr)
    {
        delete tcpPacket->m_item;
        delete tcpPacket;
        tcpPacket = nullptr;
    }
}


void 
PaceTcpSocketBase::CalculateGradient ()
{
    // ?????? RTT
    if (m_roundStart == true)
    {
        // std::cout << index << "- Time: " << Simulator::Now().GetSecouns () << ", new round = " << m_round << std::endl; 
        m_lastRoundRtt = m_currRoundRtt;
        m_currRoundRtt = m_ReceRttEstimator->GetEstimate ();
        m_gradient = (m_currRoundRtt.GetSeconds () - m_lastRoundRtt.GetSeconds ())/m_minRxRTT.GetSeconds ();
    }

}


void 
PaceTcpSocketBase::UpdateAckPacingRate ()
{
    auto tmp = m_pacingAckRate;
    auto tmp2 = m_constAckRate;

    // ?????????bandwidth??????????????? m_pacingAckRate ?????????
    switch (m_pacingState)
    {
    case PC_BEGIN:
        m_pacingAckRate = DataRate (0);
        m_constAckRate  = DataRate (0);
        break;

    case PC_PROBE:
        if (m_roundStart)
        {
            m_pacingAckRate = DataRate (0);
            m_constAckRate  = m_maxBandwidth;

            if (m_roundMaxBW[0] < m_roundMaxBW[1])
            {
                m_noIncreaseRoundCnt = 0;
            }
            else if (m_roundMaxBW[0].GetBitRate () != 0 && m_roundMaxBW[1].GetBitRate () != 0)
            {
                ++m_noIncreaseRoundCnt;
            }
        }

        break;

    case PC_RECVR:
        if (m_roundStart)
        {
            m_pacingAckRate = m_maxBandwidth;

            // if (m_currRoundRtt < m_delayHigh)
            // {
            //     m_exitRecovery = true;
            // }
            // else
            // {
            //     m_enterDrain = true;
            // }
        }

    case PC_STABL:
        if (m_roundStart)
        {
            if (m_lastRoundRtt > m_delayHigh)
            {
                double rate = (m_lastRoundRtt.GetSeconds () - m_delayHigh.GetSeconds ()) / m_minRxRTT.GetSeconds ();
                rate = std::min (rate, 0.70);
                auto delta = rate * m_lastRoundMxBw.GetBitRate ();
                m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () - delta);
                // m_maxBwFilter.Reset (m_pacingAckRate, m_round);
            }
            else if (m_lastRoundRtt > m_delayLow)
            {
                m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () + m_step.GetBitRate ());
            }
            else
            {
                auto delta = (m_delayLow.GetSeconds () - m_lastRoundRtt.GetSeconds ()) / m_minRxRTT.GetSeconds () * m_lastRoundMxBw.GetBitRate ();
                m_pacingAckRate = DataRate (m_maxBwFilter.GetBest ().GetBitRate () + delta);
            }
        }
        break;

    default:
        break;
    }
    

    // record
    if (m_roundStart == true)
    {
        if (m_pacingState == PC_PROBE)
        {
            std::cout << (int)index << " - time:" << Simulator::Now().GetSeconds () << " old: " << tmp2.GetBitRate () * 1.0 / 1000/ 1000 << ", new: " << m_constAckRate.GetBitRate () * 1.0 / 1000/ 1000 << std::endl;
        }
        else if (m_pacingState == PC_STABL)
        {
            std::cout << (int)index << " - time:" << Simulator::Now().GetSeconds () << " old: " << tmp.GetBitRate () * 1.0 / 1000/ 1000 << ", new: " << m_pacingAckRate.GetBitRate () * 1.0 / 1000/ 1000 << std::endl;

        }
        
        targetRateFile << (Simulator::Now() - TimeStep (1)).GetSeconds () << "\t" << tmp.GetBitRate() * 1.0 / 1000/ 1000 << std::endl;
        targetRateFile << Simulator::Now().GetSeconds () << "\t" << m_pacingAckRate.GetBitRate() * 1.0 / 1000/ 1000 << std::endl;
    }
}

// void
// PaceTcpSocketBase::UpdateParameter ()
// {
// }

uint8_t PaceTcpSocketBase::Index = 0;

} // namespace ns3

