#include "ipv4-drill-routing-protocol.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/object.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4DrillRoutingProtocol");

NS_OBJECT_ENSURE_REGISTERED(Ipv4DrillRoutingProtocol);

TypeId
Ipv4DrillRoutingProtocol::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Ipv4DrillRoutingProtocol")
                            .SetParent<Ipv4RoutingProtocol>()
                            .SetGroupName("LoadBalancing");
    return tid;
}

Ipv4DrillRoutingProtocol::Ipv4DrillRoutingProtocol(int d)
    : m_drill_d(d)
{
    NS_LOG_FUNCTION(this << d);
}

Ipv4DrillRoutingProtocol::~Ipv4DrillRoutingProtocol()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
