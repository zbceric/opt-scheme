
/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-05 22:38:02
 * @LastEditTime: 2022-04-20 10:53:31
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/helper/tracer-packet-sink-helper.h
 */


#ifndef TRACER_PACKET_SINK_HELPER_H
#define TRACER_PACKET_SINK_HELPER_H


#include <string>
#include <stdint.h>

#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"


#include "ns3/tracer-packet-sink.h"


namespace ns3 {


class TracerPacketSinkHelper
{
public:
    TracerPacketSinkHelper (std::string protocol, Address address, uint16_t flag);

    void SetAttribute (std::string name, const AttributeValue &value);

    ApplicationContainer Install (NodeContainer c) const;
    ApplicationContainer Install (Ptr<Node> node) const;
    ApplicationContainer Install (std::string nodeName) const;

private:
    Ptr<Application> InstallPriv (Ptr<Node> node) const;
    ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif
