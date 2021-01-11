/*test quic bbr on ns3 platform*/
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/dqc-module.h"
#include <string>
#include <iostream>

#include "ns3/control-decider.h"
#include "ns3/multi-queue.h"

using namespace ns3;
using namespace dqc;
const uint32_t TOPO_DEFAULT_BW     = 300*1000000;    // in bps: 3Mbps
const uint32_t TOPO_DEFAULT_PDELAY =      20/6.0;    // in ms:   100ms
const uint32_t TOPO_DEFAULT_QDELAY =     3;    // in ms:  300ms
const uint32_t DEFAULT_PACKET_SIZE = 1000;
static double simDuration=1;
uint16_t sendPort=5432;
uint16_t recvPort=5000;
float appStart=0.0;
float appStop=simDuration;
double loss_rate = 0/1000.0;
uint32_t MTU =1500;

//initialize MultipleFlows
#define LINK_CAPACITY  TOPO_DEFAULT_BW
double load = 99.0;


static void InstallDqc(Ptr<ControlDecider> controller, dqc::CongestionControlType cc_type,
                        Ptr<Node> sender,
                        Ptr<Node> receiver,
						uint16_t send_port,
                        uint16_t recv_port,
                        float startTime,
                        float stopTime,
						DqcTrace *trace,
                        uint32_t max_bps=0,uint32_t cid=0,uint32_t emucons=1)
{
    Ptr<DqcSender> sendApp = CreateObject<DqcSender> (cc_type);
    sendApp->SetAppController(controller, recv_port);
    //Ptr<DqcDelayAckReceiver> recvApp = CreateObject<DqcDelayAckReceiver>();
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
		  sendApp->SetCongestionId(cid);
	  }
	  if(trace){
	    sendApp->SetBwTraceFuc(MakeCallback(&DqcTrace::OnBw,trace));
	    sendApp->SetTraceLossPacketDelay(MakeCallback(&DqcTrace::OnRtt,trace));
            recvApp->SetOwdTraceFuc(MakeCallback(&DqcTrace::OnOwd,trace));
            recvApp->SetGoodputTraceFuc(MakeCallback(&DqcTrace::OnGoodput,trace));
            recvApp->SetStatsTraceFuc(MakeCallback(&DqcTrace::OnStats,trace));
	  }	
}

static void InstallTcp(Ptr<ControlDecider> m_controller,
                         Ptr<Node> sendNode,
                         Ptr<Node> sinkNode,
			 Ipv4Address sourceAddress, 
			 Ipv4Address remoteAddress,
                         uint16_t port,
                         float startTime,
                         float stopTime
)

{
    ApplicationContainer applications;
    std::cout << "source: " << sourceAddress << "remote: " << remoteAddress << std::endl;

    ObjectFactory m_sendApplicationFactory("ns3::BulkSendApplication");
    m_sendApplicationFactory.Set("Protocol", StringValue("ns3::TcpSocketFactory"));
    m_sendApplicationFactory.Set("Remote", AddressValue(InetSocketAddress(remoteAddress, port)));
    //m_sendApplicationFactory.Set("MaxBytes", UintegerValue(flowsize));
    m_sendApplicationFactory.Set("SendSize", UintegerValue(DEFAULT_PACKET_SIZE));
    //m_sendApplicationFactory.Set("MyAddress", AddressValue(sourceAddress));
    Ptr<BulkSendApplication> sendApplication = m_sendApplicationFactory.Create<BulkSendApplication>();
    sendApplication->SetAppController(m_controller, port);

    sendNode->AddApplication(sendApplication);
    applications.Add(sendApplication);

    ObjectFactory m_sinkApplicationFactory("ns3::PacketSink");
    m_sinkApplicationFactory.Set("Protocol", StringValue("ns3::TcpSocketFactory"));
    m_sinkApplicationFactory.Set("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), port)));
    Ptr<PacketSink> sinkApplication = m_sinkApplicationFactory.Create<PacketSink>();
    sinkNode->AddApplication(sinkApplication);
    applications.Add(sinkApplication);

    applications.Get(0)->SetStartTime(Seconds(startTime)); //send
    applications.Get(0)->SetStopTime(Seconds(stopTime));
    applications.Get(1)->SetStartTime(Seconds(startTime)); //sink
    applications.Get(1)->SetStopTime(Seconds(stopTime));

}

int main(int argc, char *argv[]){
    uint64_t linkBw   = TOPO_DEFAULT_BW;
    uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    uint32_t msQDelay = TOPO_DEFAULT_QDELAY;
    uint32_t max_bps=0;
    double myloss=0; //default loss
    std::string traditionaltmp = "NewReno";
    std::string learningtmp = "BBR";
    std::string cc_name;  //for learning-based CC
    std::string filename = "myfile";
    CommandLine cmd;
    cmd.AddValue("loss", "loss", myloss);
    cmd.AddValue("bandwidth", "bandwidth", linkBw);
    cmd.AddValue("delay", "delay", msDelay);
    cmd.AddValue("traCC", "traCC", traditionaltmp); //for traditional CC
    cmd.AddValue ("learningCC", "learningCC", learningtmp);  //for learning-based C
    cmd.AddValue("filename", "filename", filename);
    cmd.AddValue("duration", "duration", simDuration);
    cmd.Parse (argc, argv);
    appStop = simDuration;
    int BUFFER_SIZE = 2*linkBw*msDelay*6*pow(10,-6)/8.0;
    std::cout << "buffer size is: " << BUFFER_SIZE << std::endl;
    uint64_t accessBw = linkBw;
    //cc_name="_"+learningtmp+"_"+std::to_string(linkBw)+"_"+std::to_string(msDelay)+"_"+std::to_string(myloss)+"_";
    cc_name = "_"+learningtmp+"_";

     
     // Select TCP variant
    if (traditionaltmp.compare ("NewReno") == 0)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
    }
    if (traditionaltmp.compare ("Bic") == 0)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpBic::GetTypeId ()));
    }
    if (traditionaltmp.compare ("HighSpeed") == 0)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHighSpeed::GetTypeId ()));
    }
    if (traditionaltmp.compare ("Hybla") == 0)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHybla::GetTypeId ()));
    }
    if (traditionaltmp.compare ("Htcp") == 0)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHtcp::GetTypeId ()));
    }
    if (traditionaltmp.compare ("Westwood") == 0)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
    }
    if (traditionaltmp.compare ("Veno") == 0)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVeno::GetTypeId ()));
    }
    if (traditionaltmp.compare ("Yeah") == 0)
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpYeah::GetTypeId ()));
    }
    
    
    /*
    //learning-based CC
    dqc::CongestionControlType learningCC = kRenoBytes;
    if(learningtmp==std::string("bbr")){
      learningCC=kBBR;
    }else if(learningtmp==std::string("bbrd")){
      learningCC=kBBRD;
    }else if(learningtmp==std::string("bbrplus")){
      learningCC=kBBRPlus;
    }else if(learningtmp==std::string("cubic")){
      learningCC=kCubicBytes;
    }else if(learningtmp==std::string("reno")){
      learningCC=kRenoBytes;
    }else if(learningtmp==std::string("vegas")){
      learningCC=kVegas;
    }else if(learningtmp==std::string("ledbat")){
      learningCC=kLedbat;
    }else if(learningtmp==std::string("lptcp")){
      learningCC=kLpTcp;
    }else if(learningtmp==std::string("copa")){
      learningCC=kCopa;
      std::cout<<learningCC<<std::endl;
    }else if(learningtmp==std::string("elastic")){
      learningCC=kElastic;
    }else if(learningtmp==std::string("veno")){
      learningCC=kVeno;
    }else if(learningtmp==std::string("westwood")){
      learningCC=kWestwood;
    }else if(learningtmp==std::string("pcc")){
      learningCC=kPCC;
    }else if(learningtmp==std::string("viva")){
      learningCC=kVivace;
    }else if(learningtmp==std::string("webviva")){
      learningCC=kWebRTCVivace;
    }else if(learningtmp==std::string("lpbbr")){
      learningCC=kLpBBR;
    }else if(learningtmp==std::string("liaen")){
      learningCC=kLiaEnhance;
    }else if(learningtmp==std::string("liaen2")){
      learningCC=kLiaEnhance2;
    }else if(learningtmp==std::string("learning")){
      learningCC=kLearningBytes;
    }else if(learningtmp==std::string("hunnan")){
      learningCC=kHunnanBytes;
    }else if(learningtmp==std::string("cubicplus")){
      learningCC=kCubicPlus;
    }else if(learningtmp==std::string("hunnanreno")){
      learningCC=kHunnanBytes;
    }else{
      learningCC=kRenoBytes;
    }
    */
    

    
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (DEFAULT_PACKET_SIZE));
    //Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(NewReno::GetTypeId()));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (BUFFER_SIZE * DEFAULT_PACKET_SIZE));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (BUFFER_SIZE * DEFAULT_PACKET_SIZE));
    

	if(loss_rate>0){
	Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (loss_rate));
	Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));
	Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (loss_rate));
	Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));
	}

    //生成网络节点
    NodeContainer ends, switches;
    ends.Create(4);
    switches.Create(2);

    //安装协议栈
    InternetStackHelper stack;
    stack.Install(ends);
    stack.Install(switches);

    NodeContainer h0s0 = NodeContainer(ends.Get(0), switches.Get(0));
    NodeContainer h1s0 = NodeContainer(ends.Get(1), switches.Get(0));
    NodeContainer h2s1 = NodeContainer(ends.Get(2), switches.Get(1));
    NodeContainer h3s1 = NodeContainer(ends.Get(3), switches.Get(1));
    NodeContainer s0s1 = NodeContainer(switches.Get(0), switches.Get(1));


    Ptr<ControlDecider> m_controller= CreateObject<ControlDecider>();
    m_controller->defaultRtt = msDelay*6; //for base rtt calculated in controller
    m_controller->loss_rate = myloss; //for random loss rate in send device
    m_controller->filename = filename; //for get the file name  
    m_controller->bufferSize = BUFFER_SIZE;

    SendHelper sendhelper;
    sendhelper.SetHelpController(m_controller);
    //sendhelper.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (linkBw)));
    sendhelper.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (accessBw)));
    sendhelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    sendhelper.SetDeviceAttribute("Mtu", UintegerValue(MTU));
    //sendhelper.SetQueue ("ns3::DropTailQueue<Packet>",
      //                     "MaxSize",  QueueSizeValue (QueueSize (std::to_string(BUFFER_SIZE)+"p")));
    sendhelper.SetQueue ("ns3::MultiQueue<Packet>",
                           "MaxSize",  QueueSizeValue (QueueSize (std::to_string(BUFFER_SIZE)+"p")));


    NetDeviceContainer d_h0s0 = sendhelper.Install(h0s0);
    NetDeviceContainer d_h1s0 = sendhelper.Install(h1s0);


    RecvHelper recvhelper;
    recvhelper.SetHelpController(m_controller);
    //recvhelper.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (linkBw)));
    recvhelper.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (accessBw)));
    recvhelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    recvhelper.SetDeviceAttribute("Mtu", UintegerValue(MTU));
    //recvhelper.SetQueue ("ns3::DropTailQueue<Packet>",
      //                     "MaxSize",  QueueSizeValue (QueueSize (std::to_string(BUFFER_SIZE)+"p")));
    recvhelper.SetQueue ("ns3::MultiQueue<Packet>",
                           "MaxSize",  QueueSizeValue (QueueSize (std::to_string(BUFFER_SIZE)+"p")));


    NetDeviceContainer d_h2s1 = recvhelper.Install(h2s1);
    NetDeviceContainer d_h3s1 = recvhelper.Install(h3s1);


    PointToPointHelper p2phelper;
    //p2phelper.SetHelpController(m_controller);
    p2phelper.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (linkBw)));
    p2phelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    p2phelper.SetDeviceAttribute("Mtu", UintegerValue(MTU));
    //p2phelper.SetQueue ("ns3::DropTailQueue<Packet>",
      //                     "MaxSize",  QueueSizeValue (std::to_string(BUFFER_SIZE)+"p"));
    p2phelper.SetQueue ("ns3::MultiQueue<Packet>",
                           "MaxSize",  QueueSizeValue (QueueSize (std::to_string(BUFFER_SIZE)+"p")));

    NetDeviceContainer d_s0s1 = recvhelper.Install(s0s1);

      
      //Simulator::Schedule (Seconds(5), Config::Set, "/QueueList/*/$ns3::DropTailQueue<Packet>/MaxSize",QueueSizeValue (std::to_string(100*BUFFER_SIZE)+"p"));
      /* 
      for(int i =9; i<20; i++)
      {
        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h0s0.Get(0),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));
        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h0s0.Get(1),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));

        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h1s0.Get(0),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));
        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h1s0.Get(1),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));

        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h2s0.Get(0),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));
        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h2s0.Get(1),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));

        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_s0s1.Get(0),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));
        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_s0s1.Get(1),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));

        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h3s1.Get(0),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));
        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h3s1.Get(1),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));

        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h4s1.Get(0),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));
        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h4s1.Get(1),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));

        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h5s1.Get(0),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));
        Simulator::Schedule(Seconds(0.5*(i-1)),&NetDevice::SetAttribute,d_h5s1.Get(1),"DataRate",DataRateValue(DataRate ((i-5)*linkBw)));
      }
      */
    

    //分配ip
     
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer i0i0 = ipv4.Assign(d_h0s0);

    ipv4.SetBase("10.1.2.0","255.255.255.0");
    Ipv4InterfaceContainer i1i0 = ipv4.Assign(d_h1s0);

    ipv4.SetBase("10.1.3.0","255.255.255.0");
    Ipv4InterfaceContainer i2i1 = ipv4.Assign(d_h2s1);

    ipv4.SetBase("10.1.4.0","255.255.255.0");
    Ipv4InterfaceContainer i3i1 = ipv4.Assign(d_h3s1);

    ipv4.SetBase("10.1.5.0","255.255.255.0");
    Ipv4InterfaceContainer m_s0s1 = ipv4.Assign(d_s0s1);


    //std::cout << "device: " << sendadd1.GetAddress(1) << std::endl;
    TrafficControlHelper tch;
    tch.Uninstall (d_h0s0);
    tch.Uninstall (d_h1s0);
    tch.Uninstall (d_h2s1);
    tch.Uninstall (d_h3s1);
    tch.Uninstall (d_s0s1);


    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    
    std::string errorModelType = "ns3::RateErrorModel";
    ObjectFactory factory;
    factory.SetTypeId (errorModelType);
    Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
      
    d_h3s1.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    d_h2s1.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    d_s0s1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    d_h0s0.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    d_h1s0.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    
    //traditional CC
    /*
    InstallTcp(m_controller, h0s0.Get(0),h2s1.Get(0),i0i0.GetAddress(0), i2i1.GetAddress(0),recvPort,appStart,appStop);	   
    InstallTcp(m_controller, h1s0.Get(0),h3s1.Get(0),i1i0.GetAddress(0), i3i1.GetAddress(0),recvPort+1,appStart,appStop);
    */

    //for multiple flows
    std::string cdfFileName = "./scratch/DCTCP_CDF.txt";
    //NS_LOG_INFO("Initialize CDF table");
    struct cdf_table *cdfTable = new cdf_table();
    MultipleFlows::init_cdf(cdfTable);
    MultipleFlows::load_cdf(cdfTable, cdfFileName.c_str());

	double requestRate = load *  LINK_CAPACITY * 8 / 2 / (8 * MultipleFlows::avg_cdf (cdfTable)) / 8;
	std::cout << "the rate is: " <<  requestRate << std::endl;

	NodeContainer sendhost;
	NodeContainer recvhost;
	sendhost.Add(h0s0.Get(0));
	sendhost.Add(h1s0.Get(0));
    recvhost.Add(h2s1.Get(0));
    recvhost.Add(h3s1.Get(0));

    std::vector<ns3::Ipv4Address> sendAddress;
    std::vector<ns3::Ipv4Address> recvAddress;
    sendAddress.push_back(i0i0.GetAddress(0));
    sendAddress.push_back(i1i0.GetAddress(0));
    recvAddress.push_back(i2i1.GetAddress(0));
    recvAddress.push_back(i3i1.GetAddress(0));

    MultipleFlows myApp;
	myApp.InstallAllApplications(sendhost,recvhost, requestRate, cdfTable, sendAddress, recvAddress,
	0, recvPort, DEFAULT_PACKET_SIZE, Seconds(appStart), Seconds(appStop),Seconds(appStop/2), m_controller);
    /*
    //learning-based CC
    uint32_t cc_id=1;
    int test_pair=1;
    DqcTrace trace1;
    std::string log_common = cc_name;
    std::string log_1=log_common+std::to_string(test_pair);
    trace1.Log(log_1,DqcTraceEnable::E_DQC_RTT|DqcTraceEnable::E_DQC_BW);
    test_pair++;
    
    InstallDqc(m_controller, learningCC, h0s0.Get(0),h2s1.Get(0),sendPort,recvPort,appStart,appStop,&trace1,max_bps,cc_id);
    cc_id++;	
    
    
    DqcTrace trace2;
    std::string log_2=log_common+std::to_string(test_pair);
    trace2.Log(log_2,DqcTraceEnable::E_DQC_RTT|DqcTraceEnable::E_DQC_BW);
    test_pair++;
    InstallDqc(m_controller, learningCC,h1s0.Get(0),h3s1.Get(0),sendPort+1,recvPort+1,appStart,appStop,&trace2,max_bps,cc_id);
    cc_id++;
   */
        

    //Simulator::Schedule (Seconds(0.1), Config::Set, "/ChannelList/*/$ns3::SendChannel/Delay", TimeValue (MicroSeconds(130.0)));
    Simulator::Stop (Seconds(simDuration));
    Simulator::Run ();
    Simulator::Destroy();
    return 0;
}

