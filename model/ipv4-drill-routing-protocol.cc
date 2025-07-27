#include "ipv4-drill-routing-protocol.h"

#include "ns3/assert.h"
#include "ns3/channel.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/queue.h"

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

Ipv4DrillRoutingProtocol::Ipv4DrillRoutingProtocol(uint32_t d)
    : m_drill_d(d)
{
    NS_LOG_FUNCTION(this << d);
    m_rng = std::mt19937(std::random_device{}());
}

Ipv4DrillRoutingProtocol::~Ipv4DrillRoutingProtocol()
{
    NS_LOG_FUNCTION(this);
}

bool
Ipv4DrillRoutingProtocol::RouteInput(Ptr<const Packet> p,
                                     const Ipv4Header& header,
                                     Ptr<const NetDevice> idev,
                                     const UnicastForwardCallback& ucb,
                                     const MulticastForwardCallback& mcb,
                                     const LocalDeliverCallback& lcb,
                                     const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(this << p << header << idev);

    for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); ++i)
    {
        for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); ++j)
        {
            if (header.GetDestination() == m_ipv4->GetAddress(i, j).GetLocal())
            {
                if (!lcb.IsNull())
                {
                    lcb(p, header, i);
                }
                return true;
            }
        }
    }
    // If no DRILL next-hops, fallback
    if (m_nextHops.empty())
    {
        return false;
    }
    
    // For this simple demo, we'll use all next-hops
    // In a real implementation, we'd filter by reachability to destination
    std::vector<uint32_t> validNextHops;
    for(uint32_t i = 0; i < m_nextHops.size(); i++) {
        validNextHops.push_back(i);
    }
    
    // DRILL sampling: d random + m memory
    std::vector<uint32_t> choices;
    uint32_t N = validNextHops.size();
    std::uniform_int_distribution<uint32_t> dist(0, N - 1);
    
    NS_LOG_DEBUG("DRILL routing for dest " << header.GetDestination() << 
                " with " << N << " next-hops, d=" << m_drill_d);
    
    for (uint32_t i = 0; i < m_drill_d; ++i)
    {
        uint32_t choice = validNextHops[dist(m_rng)];
        choices.push_back(choice);
        NS_LOG_DEBUG("  Random choice " << i << ": next-hop " << choice);
    }
    choices.insert(choices.end(), m_memory.begin(), m_memory.end());
    
    for (auto mem : m_memory)
    {
        NS_LOG_DEBUG("  Memory choice: next-hop " << mem);
    }

    // pick best by smallest queue length
    uint32_t best = choices[0];
    uint32_t minQ = std::numeric_limits<uint32_t>::max();
    for (auto idx : choices)
    {
        Ptr<PointToPointNetDevice> dev = m_nextHops[idx]->GetObject<PointToPointNetDevice>();
        Ptr<Queue<Packet>> q = dev->GetQueue();
        uint32_t len = q->GetNPackets();
        uint32_t maxSize = q->GetMaxSize().GetValue();
        NS_LOG_DEBUG("  Next-hop " << idx << " queue length: " << len << "/" << maxSize);
        if (len < minQ)
        {
            minQ = len;
            best = idx;
        }
    }
    
    NS_LOG_DEBUG("  Selected next-hop " << best << " with queue length " << minQ);
    
    m_memory.clear();
    m_memory.push_back(best);

    // Build route
    Ptr<Ipv4Route> route = Create<Ipv4Route>();
    route->SetDestination(header.GetDestination());
    route->SetSource(m_ipv4->GetAddress(1, 0).GetLocal());
    Ptr<NetDevice> outDev = m_nextHops[best];
    route->SetOutputDevice(outDev);

    // Find the gateway (peer's IP address on the selected interface)
    Ptr<PointToPointNetDevice> p2pnd = outDev->GetObject<PointToPointNetDevice>();
    Ptr<Channel> ch = p2pnd->GetChannel();
    Ptr<PointToPointNetDevice> peer = nullptr;
    
    // Get the peer device on the other end of the link
    if (ch->GetDevice(0) == p2pnd)
    {
        peer = ch->GetDevice(1)->GetObject<PointToPointNetDevice>();
    }
    else
    {
        peer = ch->GetDevice(0)->GetObject<PointToPointNetDevice>();
    }
    
    // Get the peer node and find its IP address on this interface
    Ptr<Node> peerNode = peer->GetNode();
    Ptr<Ipv4> peerIpv4 = peerNode->GetObject<Ipv4>();
    
    // Find the interface index for the peer device
    int32_t peerInterfaceIndex = -1;
    for (uint32_t i = 0; i < peerIpv4->GetNInterfaces(); ++i)
    {
        if (peerIpv4->GetNetDevice(i) == peer)
        {
            peerInterfaceIndex = i;
            break;
        }
    }
    
    if (peerInterfaceIndex >= 0 && peerIpv4->GetNAddresses(peerInterfaceIndex) > 0)
    {
        Ipv4Address gateway = peerIpv4->GetAddress(peerInterfaceIndex, 0).GetLocal();
        route->SetGateway(gateway);
        NS_LOG_INFO("  Using gateway: " << gateway);
    }
    else
    {
        NS_LOG_ERROR("Could not find gateway IP address for peer device");
        return false;
    }

    // Forward
    ucb(route, p, header);
    return true;
}

Ptr<Ipv4Route>
Ipv4DrillRoutingProtocol::RouteOutput(Ptr<Packet> p,
                                      const Ipv4Header& header,
                                      Ptr<NetDevice> oif,
                                      Socket::SocketErrno& sockerr)
{
    NS_LOG_FUNCTION(this << p << header << oif);

    sockerr = Socket::ERROR_NOTERROR;
    return nullptr;
}

void
Ipv4DrillRoutingProtocol::SetIpv4(Ptr<Ipv4> ipv4)
{
    NS_LOG_FUNCTION(this << ipv4);
    m_ipv4 = ipv4;
}

void
Ipv4DrillRoutingProtocol::SetNextHops(const std::vector<Ptr<NetDevice>>& hops)
{
    NS_LOG_FUNCTION(this << hops.size());
    m_nextHops = hops;
};

void
Ipv4DrillRoutingProtocol::NotifyInterfaceUp (uint32_t interface) {
    NS_LOG_FUNCTION(this << interface);
};

void
Ipv4DrillRoutingProtocol::NotifyInterfaceDown (uint32_t interface) {
    NS_LOG_FUNCTION(this << interface);
};

void
Ipv4DrillRoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {
    NS_LOG_FUNCTION(this << interface << address);
};

void 
Ipv4DrillRoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {
    NS_LOG_FUNCTION(this << interface << address);
};

void
Ipv4DrillRoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {
    NS_LOG_FUNCTION(this << stream << unit);
};

} // namespace ns3
