#ifndef CONTROL_DECIDER_H
#define CONTROL_DECIDER_H

#include "ns3/application.h"
#include "ns3/dqn.h"
#include <vector>
#include <numeric>
#include <map>
#include "ns3/bpnet.h"
#include <fstream>

namespace ns3

{
class ControlDecider : public Object
{
public:
	static TypeId GetTypeId (void);

	ControlDecider();
	~ControlDecider();

	//breakpoint
	void Breakpoint(uint16_t m_tuple, uint64_t m_time);
	std::map<uint16_t, std::vector<std::vector<double>>> breakpoint;
	double m_time=0;  //for breakpoint in recv-net-device
	double defaultRtt; //given base rtt
	double loss_rate; //for myproject
	double randomloss; //for channel-utilization
        std::string filename;

	//throughput and RTT
	void RttValue(uint16_t m_tuple,uint32_t m_rttValue);
	std::map<uint16_t, std::vector<uint32_t>> rttValue;

	void SendTime(uint16_t m_tuple,uint64_t sendValue);
	std::map<uint16_t, std::vector<uint64_t>> sendTime;

	void RecvTime(uint16_t m_tuple,uint64_t recvValue);
	void ReTime(uint16_t m_tuple,uint64_t retime);
	void PacketSize(uint16_t m_tuple, uint32_t size);
	std::map<uint16_t, std::vector<uint64_t>> recvTime;
	std::map<uint16_t, std::vector<uint64_t>> reTime;
	std::map<uint16_t, std::vector<uint32_t>> packetSize;


	//count the update step
	uint64_t UpdateCounts(uint16_t m_tuple);
	std::map<uint16_t, uint64_t>  updateCounts;

    //get the states
	std::vector<double> Avg(uint16_t m_tuple);
	std::vector<std::vector<double>> m_states;

    //get the algorithms for each type of flows
	void SwitchAlg(uint16_t m_tuple, std::vector<double> m_avg);
    std::map<uint16_t, int>  switchAlg;

    //initilize rl algorithms
	void SetBpnet(Ptr<Bpnet> net1, Ptr<Bpnet> net2);

    //save the tuple and flowsize
	void FlowSize(uint16_t m_tuple, int m_size);
	void StartFlowSize(uint16_t m_tuple, int m_size);
	std::map<uint16_t, int> flowSize;
	std::map<uint16_t, int> startFlowSize;
	int m_type = 1;

	//save flow time
	void FlowStartTime(uint16_t m_tuple, uint64_t m_time);
	std::map<uint16_t, int> flowStartTime;
    void FlowCompleteTime(uint16_t m_tuple, uint64_t m_time);
	std::map<uint16_t, int> flowCompleteTime;
	void SaveFlowTime();



    //save data
	void SaveData(uint16_t m_tuple, Ptr<DQN> net);
	std::ofstream outFile;
	std::ofstream outFile2;
	std::ofstream outFile3;

	Ptr<DQN> medium_dqn;
	Ptr<DQN> large_dqn;

	//dynamic queues
	double bufferSize = 0;
	double ratio_1 = 0.1;
	double ratio_2 = 0.1;
	double ratio_3 = 0.3;
	double ratio_4 = 0.5;
	int queue_1 = bufferSize*ratio_1;
	int queue_2 = bufferSize*ratio_2;
	int queue_3 = bufferSize*ratio_3;
	int queue_4 = bufferSize*ratio_4;

private:
	int m;

};

} // namespace ns3


#endif
