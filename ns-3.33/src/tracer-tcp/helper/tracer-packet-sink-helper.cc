/*
 * @Author: Zhang Bochun
 * @Date: 2022-04-05 22:45:42
 * @LastEditTime: 2022-04-20 10:52:54
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/src/tracer-tcp/helper/tracer-packet-sink-helper.cc
 */


#include "tracer-packet-sink-helper.h"


namespace ns3 {

TracerPacketSinkHelper::TracerPacketSinkHelper (std::string protocol, Address address, uint16_t flag)
{
    m_factory.SetTypeId ("ns3::TracerPacketSink");
    m_factory.Set ("Protocol", StringValue (protocol));
    m_factory.Set ("Local", AddressValue (address));
    m_factory.Set ("Traceflag", UintegerValue (flag));
}

void TracerPacketSinkHelper::SetAttribute (std::string name, const AttributeValue &value)
{
    m_factory.Set (name, value);
}

ApplicationContainer TracerPacketSinkHelper::Install (Ptr<Node> node) const
{
    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer TracerPacketSinkHelper::Install (std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node> (nodeName);
    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer TracerPacketSinkHelper::Install (NodeContainer c) const
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
        apps.Add (InstallPriv (*i));
    }

    return apps;
}

Ptr<Application> TracerPacketSinkHelper::InstallPriv (Ptr<Node> node) const
{
    Ptr<Application> app = m_factory.Create<Application> ();
    node->AddApplication (app);

    return app;
}

} // namespace ns3

