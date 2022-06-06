/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-22 10:40:56
 * @LastEditTime: 2022-06-07 01:34:33
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/internet/model/pace-tcp-socket-factory.cc
 */

#include "pace-tcp-socket-factory.h"

#include "ns3/tcp-l4-protocol.h"
#include "ns3/socket.h"
#include "ns3/assert.h"

namespace ns3 {

TypeId PaceTcpSocketFactory::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::PaceTcpSocketFactory")
        .SetParent<SocketFactory> ()
        .SetGroupName ("Internet")
    ;
    return tid;
}

PaceTcpSocketFactory::PaceTcpSocketFactory ()
  : m_tcp (0)
{
}
PaceTcpSocketFactory::~PaceTcpSocketFactory ()
{
    NS_ASSERT (m_tcp == 0);
}

void PaceTcpSocketFactory::SetTcp (Ptr<TcpL4Protocol> tcp)
{
  m_tcp = tcp;
}

Ptr<Socket> PaceTcpSocketFactory::CreateSocket (void)
{
  return m_tcp->CreateSocket ();
}

void PaceTcpSocketFactory::DoDispose (void)
{
  m_tcp = 0;
  SocketFactory::DoDispose ();
}

} // namespace ns3

