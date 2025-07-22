#ifndef IPV4_DRILL_ROUTING_PROTOCOL_H
#define IPV4_DRILL_ROUTING_PROTOCOL_H

#include "ns3/ipv4-routing-protocol.h"

namespace ns3
{

class Ipv4DrillRoutingProtocol : public Ipv4RoutingProtocol
{
  private:
    int m_drill_d;
    int m_drill_m = 1;

  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Ipv4DrillRoutingProtocol(int d);
    virtual ~Ipv4DrillRoutingProtocol();
};

} // namespace ns3

#endif // IPV4_DRILL_ROUTING_PROTOCOL_H
