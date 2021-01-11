/** Network topology
 *
 *    10Mb/s, 2ms                            10Mb/s, 4ms
 * n0--------------|                    |---------------n4
 *                 |   1.5Mbps/s, 20ms  |
 *                 n2------------------n3
 *    10Mb/s, 3ms  |                    |    10Mb/s, 5ms
 * n1--------------|                    |---------------n5
 *
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/dqc-module.h"
#include "ns3/log.h"
#include<stdio.h>
#include<iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <memory>
#include <chrono>
#include "ns3/control-decider.h"

using namespace ns3;
using namespace dqc;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("bbr-rtt");

const uint32_t DEFAULT_PACKET_SIZE = 1000;

uint32_t checkTimes;
double avgQueueSize;

// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;

NodeContainer n0n2;
NodeContainer n2n3;
NodeContainer n3n4;
NodeContainer n1n2;
NodeContainer n3n5;

Ipv4InterfaceContainer i0i2;
Ipv4InterfaceContainer i1i2;
Ipv4InterfaceContainer i2i3;
Ipv4InterfaceContainer i3i4;
Ipv4InterfaceContainer i3i5;

typedef struct
{
uint64_t bps;
uint32_t msDelay;
uint32_t msQdelay;	
}link_config_t;
//unrelated topology
/*
   L3      L1      L4
configuration same as the above dumbbell topology
n0--L0--n2--L1--n3--L2--n4
n1--L3--n2--L1--n3--L4--n5
*/
link_config_t p4p[]={
[0]={100*1000000,10,150},
[1]={5*1000000,10,150},
[2]={100*1000000,10,150},
[3]={100*1000000,10,150},
[4]={100*1000000,10,150},
};
const uint32_t TOPO_DEFAULT_BW     = 5000000;    // in bps: 3Mbps
const uint32_t TOPO_DEFAULT_PDELAY =      10;    // in ms:   100ms
const uint32_t TOPO_DEFAULT_QDELAY =     150;    // in ms:  300ms
static void InstallDqc( Ptr<ControlDecider> controller, dqc::CongestionControlType cc_type,
                        Ptr<Node> sender,Ptr<Node> receiver,
                        uint16_t send_port,uint16_t recv_port,
                        float startTime,float stopTime,
                        DqcTrace *trace, DqcTraceState *stat,
                        uint32_t max_bps=0,uint32_t cid=0,bool ecn=false,uint32_t emucons=1)
{
    Ptr<DqcSender> sendApp = CreateObject<DqcSender> (cc_type,ecn);
    sendApp->SetAppController(controller, recv_port);
    Ptr<DqcReceiver> recvApp = CreateObject<DqcReceiver>();
    sender->AddApplication (sendApp);
    receiver->AddApplication (recvApp);
    sendApp->SetNumEmulatedConnections(emucons);
    Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
    Ipv4Address receiverIp = ipv4->GetAddress (1, 0).GetLocal ();
    recvApp->Bind(recv_port);
    sendApp->Bind(send_port);
    sendApp->ConfigurePeer(receiverIp,recv_port);
    sendApp->SetStartTime (Seconds (startTime));
    sendApp->SetStopTime (Seconds (stopTime));
    recvApp->SetStartTime (Seconds (startTime));
    recvApp->SetStopTime (Seconds (stopTime));
    if(max_bps>0){
        sendApp->SetMaxBandwidth(max_bps);
    }
    if(cid){
       sendApp->SetSenderId(cid);
        sendApp->SetCongestionId(cid);
    }
    if(trace){
        sendApp->SetBwTraceFuc(MakeCallback(&DqcTrace::OnBw,trace));
	sendApp->SetTraceLossPacketDelay(MakeCallback(&DqcTrace::OnRtt,trace));
        recvApp->SetOwdTraceFuc(MakeCallback(&DqcTrace::OnOwd,trace));
        recvApp->SetGoodputTraceFuc(MakeCallback(&DqcTrace::OnGoodput,trace));
        recvApp->SetStatsTraceFuc(MakeCallback(&DqcTrace::OnStats,trace));
        trace->SetStatsTraceFuc(MakeCallback(&DqcTraceState::OnStats,stat));
    }
}
void ns3_rtt(Ptr<ControlDecider> controller, int ins,std::string algo,DqcTraceState *stat,int sim_time=200,int loss_integer=0){
    std::string instance=std::to_string(ins);
    uint64_t linkBw   = TOPO_DEFAULT_BW;
    uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    uint16_t sendPort=1000;
    uint16_t recvPort=5000;
    uint32_t msQdelay;
    int m = 1; //for dynamic level of rtt
    if(instance==std::string("1")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*1;
        m = 1;
    }else if(instance==std::string("2")){
        linkBw=3000000;
        msDelay=10; 
        msQdelay = 30*2*3;  
        m = 1;  
    }else if(instance==std::string("3")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*5;
        m = 1;
    }else if(instance==std::string("4")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*3;
        m = 3;
    }else if(instance==std::string("5")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*7;
        m = 3;
    }else if(instance==std::string("6")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*11;	
        m = 3;
    }else if(instance==std::string("7")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*6;
        m = 6;
    }else if(instance==std::string("8")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*13;
        m = 6;
    }else if(instance==std::string("9")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*20;	
        m = 6;
    }else if(instance==std::string("10")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*10;
        m = 9;
    }else if(instance==std::string("11")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*19;
        m = 9;
    }else if(instance==std::string("12")){
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*28;	
        m = 9;
    }else{
        linkBw=3000000;
        msDelay=10;
        msQdelay = 30*2*3;  
        m = 1;      
    }
    double sim_dur=sim_time;
    int start_time=0;
    int end_time=sim_time;
    float appStart=start_time;
    float appStop=end_time;
    p4p[1].bps=linkBw;
    p4p[1].msDelay=msDelay;
    uint32_t owd1=p4p[0].msDelay+p4p[1].msDelay+p4p[2].msDelay;
    uint32_t owd2=p4p[3].msDelay+p4p[1].msDelay+p4p[4].msDelay;
    uint32_t owd=std::max(owd1,owd2);
    for(size_t i=0;i<sizeof(p4p)/sizeof(p4p[0]);i++){
        p4p[i].msQdelay=msQdelay;
    }

    NodeContainer c;
    c.Create (6);
    n0n2 = NodeContainer (c.Get (0), c.Get (2));
    n1n2 = NodeContainer (c.Get (1), c.Get (2));
    n2n3 = NodeContainer (c.Get (2), c.Get (3));
    n3n4 = NodeContainer (c.Get (3), c.Get (4));
    n3n5 = NodeContainer (c.Get (3), c.Get (5));
    uint32_t meanPktSize = 1500;
    link_config_t *config=p4p;
    uint32_t bufSize=0;	
    
    InternetStackHelper internet;
    internet.Install (c);
    
    NS_LOG_INFO ("Create channels");
    PointToPointHelper p2p;
    p2p.SetHelpController(controller);
    TrafficControlHelper tch;
    //L0
    bufSize =std::max<uint32_t> (1,config[0].bps * config[0].msQdelay/8000000);
    p2p.SetQueue ("ns3::DropTailQueue<Packet>",
                           "MaxSize", QueueSizeValue (QueueSize (std::to_string(bufSize)+"p")));
    p2p.SetDeviceAttribute("Mtu", UintegerValue(DEFAULT_PACKET_SIZE));
    p2p.SetDeviceAttribute ("DataRate", DataRateValue(DataRate (config[0].bps)));
    p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (config[0].msDelay)));
    NetDeviceContainer devn0n2 = p2p.Install (n0n2);

    //L3
    bufSize =std::max<uint32_t> (1,config[3].bps * config[3].msQdelay/8000000);
    p2p.SetQueue ("ns3::DropTailQueue<Packet>",
                           "MaxSize", QueueSizeValue (QueueSize (std::to_string(bufSize)+"p")));
    p2p.SetDeviceAttribute("Mtu", UintegerValue(DEFAULT_PACKET_SIZE));
    p2p.SetDeviceAttribute ("DataRate", DataRateValue(DataRate (config[3].bps)));
    p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (config[3].msDelay)));
    NetDeviceContainer devn1n2 = p2p.Install (n1n2);

    //L1
    bufSize =std::max<uint32_t> (1,config[1].bps * config[1].msQdelay/8000000);
    p2p.SetQueue ("ns3::DropTailQueue<Packet>",
                           "MaxSize", QueueSizeValue (QueueSize (std::to_string(bufSize)+"p")));
    p2p.SetDeviceAttribute("Mtu", UintegerValue(DEFAULT_PACKET_SIZE));
    p2p.SetDeviceAttribute ("DataRate", DataRateValue(DataRate (config[1].bps)));
    p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (config[1].msDelay)));
    NetDeviceContainer devn2n3 = p2p.Install (n2n3);

    //L2
    bufSize = std::max<uint32_t> (1,config[2].bps * config[2].msQdelay/8000000);
    p2p.SetQueue ("ns3::DropTailQueue<Packet>",
                           "MaxSize", QueueSizeValue (QueueSize (std::to_string(bufSize)+"p")));
    p2p.SetDeviceAttribute("Mtu", UintegerValue(DEFAULT_PACKET_SIZE));
    p2p.SetDeviceAttribute ("DataRate", DataRateValue(DataRate (config[2].bps)));
    p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (config[2].msDelay)));
    NetDeviceContainer devn3n4 = p2p.Install (n3n4);

    //L4
    bufSize =std::max<uint32_t> (1,config[4].bps * config[4].msQdelay/8000000);
    p2p.SetQueue ("ns3::DropTailQueue<Packet>",
                           "MaxSize", QueueSizeValue (QueueSize (std::to_string(bufSize)+"p")));
    p2p.SetDeviceAttribute("Mtu", UintegerValue(DEFAULT_PACKET_SIZE));
    p2p.SetDeviceAttribute ("DataRate", DataRateValue(DataRate (config[4].bps)));
    p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (config[4].msDelay)));
    NetDeviceContainer devn3n5 = p2p.Install (n3n5);

    //initialize buffer size in controller
    controller->bufferSize = bufSize;
    
    for(int i =1; i<20; i++)
    {
    	Simulator::Schedule (Seconds(i*1), Config::Set, "/ChannelList/*/$ns3::PointToPointChannel/Delay", TimeValue (MilliSeconds(m*i+config[1].msDelay)));
    }
    
   
    

    
    Ipv4AddressHelper ipv4;
    
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    i0i2 = ipv4.Assign (devn0n2);
    tch.Uninstall (devn0n2);
    
    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    i1i2 = ipv4.Assign (devn1n2);
    tch.Uninstall (devn1n2);
    
    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    i2i3 = ipv4.Assign (devn2n3);
    tch.Uninstall (devn2n3);
    
    ipv4.SetBase ("10.1.4.0", "255.255.255.0");
    i3i4 = ipv4.Assign (devn3n4);
    tch.Uninstall (devn3n4);
    
    ipv4.SetBase ("10.1.5.0", "255.255.255.0");
    i3i5 = ipv4.Assign (devn3n5);
    tch.Uninstall (devn3n5);

    // Set up the routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    dqc::CongestionControlType cc=kBBRPlus;
    if(algo.compare("bbr")==0){
        cc=kBBR;
    }else if(algo.compare("bbrplus")==0){
        cc=kBBRPlus;
    }else if(algo.compare("tsunami")==0){
        cc=kTsunami;
    }else if(algo.compare("bbrv2")==0){
        cc=kBBRv2;
    }else if(algo.compare("bbradd")==0){
        cc=kHighSpeedRail;
    }else if(algo.compare("bbrrand")==0){
        cc=kCopa;
    }else if(algo.compare("cubic")==0){
        cc=kCubicBytes;
    }else if(algo.compare("westwood")==0){
        cc=kWestwood;
    }else if(algo.compare("cubicplus")==0){
        cc=kCubicPlus;
    }else if(algo.compare("reno")==0){
        cc=kRenoBytes;	
    }else if(algo.compare("hsr")==0){
        cc=kHighSpeedRail;
    }
    //no use
    //CongestionControlManager cong_ops_manager;
    //RegisterCCManager(&cong_ops_manager);
    
    uint32_t max_bps=0;
    int test_pair=1;
    uint32_t sender_id=1;

    std::vector<std::unique_ptr<DqcTrace>> traces;
    std::string log;
    std::string delimiter="_";
    std::string prefix=instance+delimiter+algo+delimiter+"rtt"+delimiter;
    log=prefix+std::to_string(test_pair);
    std::unique_ptr<DqcTrace> trace;


    trace.reset(new DqcTrace(test_pair));
    stat->ReisterAvgDelayId(test_pair);
    stat->RegisterCongestionType(test_pair);
    trace->Log(log,DqcTraceEnable::E_DQC_RTT|DqcTraceEnable::E_DQC_GOODPUT|DqcTraceEnable::E_DQC_BW|DqcTraceEnable::E_DQC_OWD
|DqcTraceEnable::E_DQC_STAT);  

    InstallDqc(controller, cc,c.Get(0),c.Get(4),sendPort,recvPort,appStart+0.01,appStop,trace.get(),stat,max_bps,sender_id);
    sender_id++;
    test_pair++;
    sendPort++;
    recvPort++;
    traces.push_back(std::move(trace));

    trace.reset(new DqcTrace(test_pair));
    stat->ReisterAvgDelayId(test_pair);
    log=prefix+std::to_string(test_pair);
    trace->Log(log,DqcTraceEnable::E_DQC_RTT|DqcTraceEnable::E_DQC_GOODPUT|DqcTraceEnable::E_DQC_BW|DqcTraceEnable::E_DQC_OWD
|DqcTraceEnable::E_DQC_STAT);
	InstallDqc(controller, cc,c.Get(1),c.Get(5),sendPort,recvPort,appStart+0.01,appStop,trace.get(),stat,max_bps,sender_id);
    sender_id++;
    test_pair++;
    sendPort++;
    recvPort++;
    traces.push_back(std::move(trace));

    Simulator::Stop (Seconds(sim_dur));
    Simulator::Run ();
    Simulator::Destroy();  
    stat->Flush(linkBw,sim_dur);    
}
int main (int argc, char *argv[]){
    int sim_time=20;
    int ins = 1;
    std::string algos = "bbr";
    CommandLine cmd;
    cmd.AddValue ("instance", "instacne", ins);
    cmd.AddValue ("algorithm", "algorithm", algos);
    cmd.Parse (argc, argv);

    //set controller
    Ptr<ControlDecider> m_controller= CreateObject<ControlDecider>();

    std::string cong=std::string(algos);
    std::string name=cong;
    std::unique_ptr<DqcTraceState> stat;
    stat.reset(new DqcTraceState(name));
    auto inner_start = std::chrono::high_resolution_clock::now();
    ns3_rtt(m_controller, ins,cong,stat.get(),sim_time);

    auto inner_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> tm = inner_end - inner_start;
    std::chrono::duration<double, std::ratio<60>> minutes =inner_end- inner_start;
    stat->RecordRuningTime(tm.count(),minutes.count());
    
    return 0;
    /*
    int sim_time=200;
    int ins[]={1,2,3,4,5,6,7,8,9};
    char *algos[]={"bbr","bbrplus","tsunami","bbradd", "bbrrand"};

    //set controller
    Ptr<ControlDecider> m_controller= CreateObject<ControlDecider>();

    for(int c=0;c<(int)sizeof(algos)/sizeof(algos[0]);c++){
        std::string cong=std::string(algos[c]);
        std::string name=cong;
        std::unique_ptr<DqcTraceState> stat;
        stat.reset(new DqcTraceState(name));
        auto inner_start = std::chrono::high_resolution_clock::now();
        for(int i=0;i<sizeof(ins)/sizeof(ins[0]);i++){
            ns3_rtt(m_controller, ins[i],cong,stat.get(),sim_time);
        }
        auto inner_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> tm = inner_end - inner_start;
        std::chrono::duration<double, std::ratio<60>> minutes =inner_end- inner_start;
        stat->RecordRuningTime(tm.count(),minutes.count());     
    }
    return 0;
    */
}

