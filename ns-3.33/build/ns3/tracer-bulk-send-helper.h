
/*
 * @Author: Zhang Bochun
 * @Date: 2022-03-16 20:29:21
 * @LastEditTime: 2022-04-20 10:52:11
 * @LastEditors: Zhang Bochun
 * @Description: TracerBulkSendApplication Helper
 * @FilePath: /ns-3.33/src/tracer-tcp/helper/tracer-bulk-send-helper.h
 */

#ifndef TRACER_BULK_SEND_HELPER_H
#define TRACER_BULK_SEND_HELPER_H

#include <string>
#include <stdint.h>

#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"

#include "ns3/tracer-bulk-send-application.h"

namespace ns3 {


class TracerBulkSendHelper
{
public:
    TracerBulkSendHelper (std::string protocol, Address address);
    TracerBulkSendHelper (std::string protocol, Address address, uint16_t flag);

    void SetAttribute (std::string name, const AttributeValue &value);

    ApplicationContainer Install (NodeContainer c) const;

    ApplicationContainer Install (Ptr<Node> node) const;

    ApplicationContainer Install (std::string nodeName) const;

private:
    Ptr<Application> InstallPriv (Ptr<Node> node) const;

    ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* ON_OFF_HELPER_H */