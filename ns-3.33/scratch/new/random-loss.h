/*
 * @Author: Zhang Bochun
 * @Date: 2022-06-08 22:59:42
 * @LastEditTime: 2022-06-08 23:28:41
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/scratch/loss/random-loss.h
 */

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

namespace ns3
{


class TriggerRandomLoss
{
public:
    TriggerRandomLoss () = default;

    ~TriggerRandomLoss ()
    {
        if(m_timer.IsRunning ())
        {
            m_timer.Cancel ();
        }
    }

    void RegisterDevice (Ptr<NetDevice> device)
    {
        m_netDevice = device;
    }

    void Start (Time time)
    {
        m_timer = Simulator::Schedule(time, &TriggerRandomLoss::ConfigureRandomLoss, this);
    }

    void ConfigureRandomLoss ()
    {
        if(m_timer.IsExpired ())
        {
            std::string errorModelType = "ns3::RateErrorModel";
            ObjectFactory factory;
            factory.SetTypeId (errorModelType);
            Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
            m_netDevice->SetAttribute ("ReceiveErrorModel", PointerValue (em));            
            m_timer.Cancel();
        }
        m_counter++;
    }

private:
    Ptr<NetDevice> m_netDevice;
    EventId m_timer;
    uint128_t m_counter{1};
};
}