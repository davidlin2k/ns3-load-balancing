#ifndef IPV4_DRILL_ROUTING_PROTOCOL_H
#define IPV4_DRILL_ROUTING_PROTOCOL_H

#include "ns3/ipv4-routing-protocol.h"

#include <random>

namespace ns3
{

class Ipv4DrillRoutingProtocol : public Ipv4RoutingProtocol
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Ipv4DrillRoutingProtocol(uint32_t d);
    virtual ~Ipv4DrillRoutingProtocol();

    bool RouteInput(Ptr<const Packet> p,
                    const Ipv4Header& header,
                    Ptr<const NetDevice> idev,
                    const UnicastForwardCallback& ucb,
                    const MulticastForwardCallback& mcb,
                    const LocalDeliverCallback& lcb,
                    const ErrorCallback& ecb) override;

    Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p,
                               const Ipv4Header& header,
                               Ptr<NetDevice> oif,
                               Socket::SocketErrno& sockerr) override;

    void SetIpv4(Ptr<Ipv4> ipv4) override;
    void NotifyInterfaceUp (uint32_t interface) override;
    void NotifyInterfaceDown (uint32_t interface) override;
    void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) override;
    void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) override;
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const override;
    void SetNextHops(const std::vector<Ptr<NetDevice>>& hops);

  private:
    uint32_t m_drill_d = 2;
    std::vector<uint32_t> m_memory;
    std::vector<Ptr<NetDevice>> m_nextHops;
    Ptr<Ipv4> m_ipv4;
    std::mt19937 m_rng;
};

} // namespace ns3

#endif // IPV4_DRILL_ROUTING_PROTOCOL_H
