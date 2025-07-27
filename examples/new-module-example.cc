#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-drill-routing-protocol.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/command-line.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traced-value.h"

/**
 * @file
 *
 * Explain here what the example does.
 */

using namespace ns3;

// Trace callback functions
void
PacketTransmittedTrace(Ptr<const Packet> packet)
{
    std::cout << Simulator::Now().GetSeconds() << "s: Packet transmitted, size=" 
              << packet->GetSize() << " bytes" << std::endl;
}

void
PacketReceivedTrace(Ptr<const Packet> packet, const Address& from)
{
    std::cout << Simulator::Now().GetSeconds() << "s: Packet received, size=" 
              << packet->GetSize() << " bytes from " << from << std::endl;
}

void
ThroughputTrace(Ptr<PacketSink> sink, std::string fileName)
{
    static std::ofstream throughputFile(fileName);
    if (!throughputFile.is_open())
    {
        throughputFile.open(fileName);
        throughputFile << "Time(s)\tThroughput(Mbps)" << std::endl;
    }
    
    static uint32_t lastTotalRx = 0;
    uint32_t totalRx = sink->GetTotalRx();
    double throughput = (totalRx - lastTotalRx) * 8.0 / 1e6; // Mbps
    
    throughputFile << Simulator::Now().GetSeconds() << "\t" << throughput << std::endl;
    lastTotalRx = totalRx;
    
    // Schedule next measurement
    Simulator::Schedule(Seconds(0.1), &ThroughputTrace, sink, fileName);
}

void
QueueMonitor(Ptr<const Packet> packet)
{
    std::cout << Simulator::Now().GetSeconds() << "s: Packet enqueued, size=" 
              << packet->GetSize() << " bytes" << std::endl;
}

void
PeriodicQueueMonitor(std::vector<std::vector<Ptr<NetDevice>>>& spineIf)
{
    static int callCount = 0;
    callCount++;
    
    std::cout << "\n=== Periodic Queue Monitor (call " << callCount << ") at " 
              << Simulator::Now().GetSeconds() << "s ===" << std::endl;
    
    for(uint32_t j=0; j<spineIf.size(); j++){
        std::cout << "Spine " << j << ":" << std::endl;
        for(uint32_t i=0; i<spineIf[j].size(); i++){
            Ptr<PointToPointNetDevice> dev = spineIf[j][i]->GetObject<PointToPointNetDevice>();
            if(dev) {
                Ptr<Queue<Packet>> q = dev->GetQueue();
                uint32_t len = q->GetNPackets();
                uint32_t maxSize = q->GetMaxSize().GetValue();
                std::cout << "  Interface " << i << ": " << len << "/" << maxSize << " packets" << std::endl;
            }
        }
    }
    
    // Schedule next monitoring
    if(Simulator::Now().GetSeconds() < 2.5) {
        Simulator::Schedule(Seconds(0.1), &PeriodicQueueMonitor, std::ref(spineIf));
    }
}

int
main(int argc, char* argv[])
{
    // Enable logging for better visibility
    LogComponentEnable("Ipv4DrillRoutingProtocol", LOG_LEVEL_DEBUG);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    uint32_t nLeaf=4, nSpine=2, d=2, m=1;
    double simTime=10.0;
    bool enablePcap = true;
    bool enableAscii = true;
    bool enableFlowMonitor = true;
    bool useDrill = true;  // Option to switch between DRILL and global routing
    
    CommandLine cmd;
    cmd.AddValue("d","DRILL d (#choices)", d);
    cmd.AddValue("m","DRILL m (memory)", m);
    cmd.AddValue("simTime","sim time (s)", simTime);
    cmd.AddValue("enablePcap","Enable pcap tracing", enablePcap);
    cmd.AddValue("enableAscii","Enable ASCII tracing", enableAscii);
    cmd.AddValue("enableFlowMonitor","Enable FlowMonitor", enableFlowMonitor);
    cmd.AddValue("useDrill","Use DRILL routing (false = global routing)", useDrill);
    cmd.Parse(argc,argv);

    NodeContainer leaves, spines;
    leaves.Create(nLeaf); spines.Create(nSpine);
    InternetStackHelper internet;
    internet.Install(leaves); internet.Install(spines);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100kbps"));  // Very slow link to force queuing
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));      // Add some delay
    
    // Set a larger queue to ensure packets can accumulate
    p2p.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("50p"));

    Ipv4AddressHelper ipv4;
    std::vector<std::vector<Ptr<NetDevice>>> leafIf(nLeaf);
    std::vector<std::vector<Ptr<NetDevice>>> spineIf(nSpine);
    
    // Create the leaf-spine topology
    for (uint32_t i=0;i<nLeaf;i++){
        for(uint32_t j=0;j<nSpine;j++){
            NodeContainer p(leaves.Get(i), spines.Get(j));
            auto devs = p2p.Install(p);
            leafIf[i].push_back(devs.Get(0));   // Leaf side interface
            spineIf[j].push_back(devs.Get(1));  // Spine side interface
            std::ostringstream b; b<<"10."<<i<<"."<<j<<".0";
            ipv4.SetBase(b.str().c_str(),"255.255.255.0");
            ipv4.Assign(devs);
        }
    }
    
    if (useDrill)
    {
        // Install DRILL on spine switches (where load balancing decisions are made)
        std::cout << "Installing DRILL routing on spine switches..." << std::endl;
        for(uint32_t j=0;j<nSpine;j++){
            auto node = spines.Get(j);
            Ptr<Ipv4> ip = node->GetObject<Ipv4>();
            auto drill = CreateObject<Ipv4DrillRoutingProtocol>(d);
            drill->SetNextHops(spineIf[j]); // All interfaces to leaves
            drill->SetIpv4(ip);
            ip->SetRoutingProtocol(drill);
            std::cout << "  Spine " << j << " has " << spineIf[j].size() << " next-hop interfaces" << std::endl;
        }
        
        // Install static routing on leaf nodes to send all non-local traffic to spines
        std::cout << "Installing static routing on leaves to route via spines..." << std::endl;
        Ipv4StaticRoutingHelper staticRoutingHelper;
        for(uint32_t i=0;i<nLeaf;i++){
            auto node = leaves.Get(i);
            Ptr<Ipv4> ip = node->GetObject<Ipv4>();
            Ptr<Ipv4StaticRouting> staticRouting = staticRoutingHelper.GetStaticRouting(ip);
            
            // For each other leaf subnet, route via the first spine
            for(uint32_t k=0;k<nLeaf;k++){
                if(k != i) { // Don't route to self
                    std::ostringstream network; 
                    network << "10." << k << ".0.0";
                    // Route to leaf k's subnet via spine 0 (interface 1 connects to spine 0)
                    std::ostringstream gatewayStr; 
                    gatewayStr << "10." << i << ".0.2"; // Spine 0 IP on link to leaf i (10.i.0.2)
                    Ipv4Address gateway(gatewayStr.str().c_str());
                    staticRouting->AddNetworkRouteTo(Ipv4Address(network.str().c_str()),
                                                   Ipv4Mask("255.255.0.0"),
                                                   gateway,
                                                   1); // Interface 1 (first spine connection)
                }
            }
        }
    }
    else
    {
        std::cout << "Using global routing..." << std::endl;
    }
    
    // Note: DRILL handles routing decisions on leaf nodes, 
    // spine nodes use static routing to route back to leaves
    
    // Print IP addresses for debugging
    std::cout << "\n=== IP Address Assignment ===" << std::endl;
    for (uint32_t i = 0; i < nLeaf; i++)
    {
        Ptr<Node> node = leaves.Get(i);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        std::cout << "Leaf " << i << " addresses:" << std::endl;
        for (uint32_t j = 0; j < ipv4->GetNInterfaces(); j++)
        {
            for (uint32_t k = 0; k < ipv4->GetNAddresses(j); k++)
            {
                std::cout << "  Interface " << j << ": " << ipv4->GetAddress(j, k).GetLocal() << std::endl;
            }
        }
    }
    
    // Get destination address (first interface of last leaf after loopback)
    Ptr<Node> destNode = leaves.Get(nLeaf-1);
    Ptr<Ipv4> destIpv4 = destNode->GetObject<Ipv4>();
    Ipv4Address destAddr = destIpv4->GetAddress(1, 0).GetLocal(); // First non-loopback interface
    std::cout << "Using destination address: " << destAddr << std::endl;
    
    // Traffic - Create a simple single flow to test queue behavior
    uint16_t port=50000;
    
    // Single flow from leaf 0 to leaf 3
    OnOffHelper onoff("ns3::TcpSocketFactory",
                        InetSocketAddress(destAddr, port));
    onoff.SetConstantRate(DataRate("1Mbps"));  // 10x the link capacity to force queuing
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1000]")); // Always on
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]")); // Never off
    auto apps=onoff.Install(leaves.Get(0));
    apps.Start(Seconds(1.0)); apps.Stop(Seconds(simTime-1));
    
    // Install sink on destination
    PacketSinkHelper sink("ns3::TcpSocketFactory",
                            InetSocketAddress(Ipv4Address::GetAny(), port));
    auto sinkApps=sink.Install(leaves.Get(nLeaf-1));
    sinkApps.Start(Seconds(0.0)); sinkApps.Stop(Seconds(simTime));
    
    // Get pointer to PacketSink for throughput tracing  
    // (Note: using a simple placeholder since we have multiple sinks now)
    Ptr<PacketSink> sinkPtr = nullptr;
    
    // Enable tracing
    if (enablePcap)
    {
        std::cout << "Enabling pcap tracing..." << std::endl;
        p2p.EnablePcapAll("drill-load-balancing");
    }
    
    if (enableAscii)
    {
        std::cout << "Enabling ASCII tracing..." << std::endl;
        AsciiTraceHelper ascii;
        p2p.EnableAsciiAll(ascii.CreateFileStream("drill-load-balancing.tr"));
    }
    
    // // Start throughput monitoring
    // Simulator::Schedule(Seconds(1.1), &ThroughputTrace, sinkPtr, "throughput.dat");
    
    // // Connect to application traces
    // Config::ConnectWithoutContext("/NodeList/0/ApplicationList/0/$ns3::OnOffApplication/Tx",
    //                              MakeCallback(&PacketTransmittedTrace));
    // Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
    //                              MakeCallback(&PacketReceivedTrace));

    // Install FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor;
    if (enableFlowMonitor)
    {
        std::cout << "Installing FlowMonitor..." << std::endl;
        monitor = flowmon.InstallAll();
    }

    // Start periodic queue monitoring
    Simulator::Schedule(Seconds(1.5), &PeriodicQueueMonitor, std::ref(spineIf));

    std::cout << "Starting simulation..." << std::endl;

    // Only use global routing if not using DRILL
    if (!useDrill)
    {
        std::cout << "Populating global routing tables..." << std::endl;
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    }
    
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    
    // Print FlowMonitor statistics
    if (enableFlowMonitor && monitor)
    {
        std::cout << "\n=== FlowMonitor Statistics ===" << std::endl;
        monitor->CheckForLostPackets();
        auto classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
        auto stats = monitor->GetFlowStats();
        
        for (auto& flow : stats)
        {
            auto t = classifier->FindFlow(flow.first);
            std::cout << "Flow " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")" << std::endl;
            std::cout << "  Tx Bytes: " << flow.second.txBytes << std::endl;
            std::cout << "  Rx Bytes: " << flow.second.rxBytes << std::endl;
            std::cout << "  Tx Packets: " << flow.second.txPackets << std::endl;
            std::cout << "  Rx Packets: " << flow.second.rxPackets << std::endl;
            std::cout << "  Lost Packets: " << flow.second.lostPackets << std::endl;
            if (flow.second.rxPackets > 0)
            {
                std::cout << "  Throughput: " << flow.second.rxBytes * 8.0 / (flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds()) / 1000000.0 << " Mbps" << std::endl;
                std::cout << "  Mean Delay: " << flow.second.delaySum.GetSeconds() / flow.second.rxPackets << " s" << std::endl;
                if (flow.second.rxPackets > 1)
                {
                    std::cout << "  Mean Jitter: " << flow.second.jitterSum.GetSeconds() / (flow.second.rxPackets - 1) << " s" << std::endl;
                }
            }
        }
        
        // Save FlowMonitor results to XML
        monitor->SerializeToXmlFile("drill-load-balancing-flowmon.xml", true, true);
        std::cout << "FlowMonitor results saved to drill-load-balancing-flowmon.xml" << std::endl;
    }
    
    // Print final statistics
    std::cout << "\n=== Final Statistics ===" << std::endl;
    if(sinkPtr) {
        std::cout << "Total bytes received by sink: " << sinkPtr->GetTotalRx() << " bytes" << std::endl;
        std::cout << "Average throughput: " << sinkPtr->GetTotalRx() * 8.0 / simTime / 1000000.0 << " Mbps" << std::endl;
    }
    
    Simulator::Destroy();

    return 0;
}
