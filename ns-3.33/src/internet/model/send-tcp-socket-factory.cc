/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-22 10:40:56
 * @LastEditTime: 2022-06-07 23:48:33
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/internet/model/send-tcp-socket-factory.cc
 */

#include "send-tcp-socket-factory.h"

#include "ns3/tcp-l4-protocol.h"
#include "ns3/socket.h"
#include "ns3/assert.h"

namespace ns3 {

class SendTcpSocketBase;

TypeId SendTcpSocketFactory::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SendTcpSocketFactory")
        .SetParent<SocketFactory> ()
        .SetGroupName ("Tracer")
    ;
    return tid;
}

SendTcpSocketFactory::SendTcpSocketFactory ()
  : m_tcp (0)
{
}
SendTcpSocketFactory::~SendTcpSocketFactory ()
{
    NS_ASSERT (m_tcp == 0);
}

void SendTcpSocketFactory::SetTcp (Ptr<TcpL4Protocol> tcp)
{
  m_tcp = tcp;
}

Ptr<Socket> SendTcpSocketFactory::CreateSocket (void)
{
  Ptr<TcpSocketBase> socket = CreateObject<SendTcpSocketBase> ();
  return m_tcp->CreateSocket (socket);
}

void SendTcpSocketFactory::DoDispose (void)
{
  m_tcp = 0;
  SocketFactory::DoDispose ();
}

} // namespace ns3

