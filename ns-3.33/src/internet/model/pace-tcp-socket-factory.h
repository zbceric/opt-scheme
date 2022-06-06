/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-22 10:40:45
 * @LastEditTime: 2022-04-22 12:42:14
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/internet/model/pace-tcp-socket-factory.h
 */

#ifndef PACE_TCP_SOCKET_FACTORY_H
#define PACE_TCP_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"
#include "ns3/ptr.h"

namespace ns3 {

class TcpL4Protocol;

class PaceTcpSocketFactory : public SocketFactory
{
public:
    static TypeId GetTypeId (void);
    PaceTcpSocketFactory ();
    virtual ~PaceTcpSocketFactory ();


    void SetTcp (Ptr<TcpL4Protocol> tcp);

    virtual Ptr<Socket> CreateSocket (void);

protected:
    virtual void DoDispose (void);
private:
    Ptr<TcpL4Protocol> m_tcp; //!< the associated TCP L4 protocol
};

} // namespace ns3

#endif