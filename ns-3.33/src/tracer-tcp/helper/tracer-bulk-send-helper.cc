/*
 * @Author: Zhang Bochun
 * @Date: 2022-03-16 20:38:49
 * @LastEditTime: 2022-04-06 20:53:45
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/scratch/tracer-tcp-cubic/tracer-bulk-send-helper.cc
 */


#include "tracer-bulk-send-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/names.h"

namespace ns3 {


TracerBulkSendHelper::TracerBulkSendHelper (std::string protocol, Address address)
{
    m_factory.SetTypeId ("ns3::TracerBulkSendApplication");       /* 设置 ObjectFactory, 用于建立 BulkSendApplication 类 */
    m_factory.Set ("Protocol", StringValue (protocol));           /* 设置 Protocol 属性的初始值 */
    m_factory.Set ("Remote", AddressValue (address));             /* 设置 Remote 属性的初始值 */
}

TracerBulkSendHelper::TracerBulkSendHelper (std::string protocol, Address address, uint16_t flag)
{
    m_factory.SetTypeId ("ns3::TracerBulkSendApplication");       /* 设置 ObjectFactory, 用于建立 BulkSendApplication 类 */
    m_factory.Set ("Protocol", StringValue (protocol));           /* 设置 Protocol 属性的初始值 */
    m_factory.Set ("Remote", AddressValue (address));             /* 设置 Remote 属性的初始值 */
    m_factory.Set ("Traceflag", UintegerValue (flag));
}

void
TracerBulkSendHelper::SetAttribute (std::string name, const AttributeValue &value)
{
    m_factory.Set (name, value);              /* 设置属性 - 这个在 CreateObject 调用时统一生成 */
}


ApplicationContainer TracerBulkSendHelper::Install (Ptr<Node> node) const
{
    return ApplicationContainer (InstallPriv (node));
}


ApplicationContainer TracerBulkSendHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}


ApplicationContainer TracerBulkSendHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}


Ptr<Application> TracerBulkSendHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();    /* 全部 Install 函数的实现在这里, 调用 factory 可以反复生成变量 */
  node->AddApplication (app);

  return app;
}

} // namespace ns3
