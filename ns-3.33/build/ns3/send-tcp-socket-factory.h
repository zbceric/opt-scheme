/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-22 10:40:45
 * @LastEditTime: 2022-06-07 23:52:46
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/internet/model/send-tcp-socket-factory.h
 */

#ifndef SEND_TCP_SOCKET_FACTORY_H
#define SEND_TCP_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"
#include "ns3/ptr.h"

#include "send-tcp-socket-base.h"

namespace ns3 {

class TcpL4Protocol;
class SendTcpSocketBase;

class SendTcpSocketFactory : public SocketFactory
{
public:
    static TypeId GetTypeId (void);
    SendTcpSocketFactory ();
    virtual ~SendTcpSocketFactory ();


    void SetTcp (Ptr<TcpL4Protocol> tcp);

    virtual Ptr<Socket> CreateSocket (void);

protected:
    virtual void DoDispose (void);
private:
    Ptr<TcpL4Protocol> m_tcp; //!< the associated TCP L4 protocol
};

} // namespace ns3

#endif