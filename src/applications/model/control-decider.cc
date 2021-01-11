#include "control-decider.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <numeric>
#include <fstream>
#include <sstream>
#include <math.h>
#include <fstream>


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("ControlDecider");

NS_OBJECT_ENSURE_REGISTERED (ControlDecider);

TypeId
ControlDecider::GetTypeId (void)
{
  	static TypeId tid = TypeId ("ns3::ControlDecider")
	    .SetParent<Object> ()
	    .SetGroupName("Applications")
	    .AddConstructor<ControlDecider> ()
	    ;
	return tid;
}


ControlDecider::ControlDecider()
{
	NS_LOG_FUNCTION(this);
}

ControlDecider::~ControlDecider()
{
	NS_LOG_FUNCTION(this);
}

void
ControlDecider::Breakpoint(uint16_t m_tuple, uint64_t m_time)
{
    //save the time, sizes of recvTime, idTime, reTime, rttvalue
    std::vector<double> tmp;
    tmp.push_back(double(m_time));
    tmp.push_back(double((recvTime[m_tuple]).size()));
    tmp.push_back(double((reTime[m_tuple]).size()));
    tmp.push_back(double(rttValue[m_tuple].size()));
    std::vector<std::vector<double>> preTmp = breakpoint[m_tuple];
    preTmp.push_back(tmp);
    breakpoint[m_tuple] = preTmp;
}

void
ControlDecider::RttValue(uint16_t m_tuple,uint32_t m_rttValue)
{
    std::vector<uint32_t> tmp = rttValue[m_tuple];
    tmp.push_back(m_rttValue);
    rttValue[m_tuple] = tmp;
}

void
ControlDecider::SendTime(uint16_t m_tuple, uint64_t sendValue)
{
  std::vector<uint64_t> tmp = sendTime[m_tuple];
  tmp.push_back(sendValue);
  sendTime[m_tuple] = tmp;
}


void
ControlDecider::RecvTime(uint16_t m_tuple,uint64_t recvValue)
{
  std::vector<uint64_t> tmp = recvTime[m_tuple];
  tmp.push_back(recvValue);
  recvTime[m_tuple] = tmp;
}

void
ControlDecider::PacketSize(uint16_t m_tuple,uint32_t size)
{
  std::vector<uint32_t> tmp = packetSize[m_tuple];
  tmp.push_back(size);
  packetSize[m_tuple] = tmp;
}

void
ControlDecider::ReTime(uint16_t m_tuple,uint64_t retime)
{
  std::vector<uint64_t> tmp = reTime[m_tuple];
  tmp.push_back(retime);
  reTime[m_tuple] = tmp;
}

std::vector<double>
ControlDecider::Avg(uint16_t m_tuple)
{
  //breakpoint for this flow
  std::vector<std::vector<double>> tmp = breakpoint[m_tuple];

  if(tmp.size()>1)
  {
    //info of current point
    std::vector<double> tmp1 = tmp[tmp.size()-1];
    //info of last point
    std::vector<double> tmp2 = tmp[tmp.size()-2];
    double diffTime = tmp1[0]-tmp2[0];

    double avgThroughput;
    //save the time, sizes of recvTime, idTime, reTime, rttSize
    if(tmp1[1]==tmp2[1])
    {
        avgThroughput = 0.0001;
    }
    else
    {
        double totalBytes=0;
	if(tmp2[1]>0)
	{
	  for(int i=tmp2[1]-1; i<tmp1[1]-1; i++)
            {
                totalBytes = totalBytes+packetSize[m_tuple][i];
            }
	}
	else
	{
	  for(int i=0; i<tmp1[1]-1; i++)
            {
                totalBytes = totalBytes+packetSize[m_tuple][i];
            }
	}
        avgThroughput=8*totalBytes/double(diffTime);  //Mbps

    }
    

    double avgLossRate;
    for(int i=tmp.size()-2; i--;i>=0)
    {
      tmp2 = tmp[i];
      if(tmp1[1]>tmp2[1])
      {
	avgLossRate = (tmp1[2]-tmp2[2])/(tmp1[1]-tmp2[1]);
	if(avgLossRate < 1)
	{
	  break;
	}
      }
      else
      {
        avgLossRate = 0.0001;
      }
    }
    double totalRtt = 0;
    double avgRtt = 0;
    if(tmp1[3]>tmp2[3])
    {
        if(tmp2[3]>0)
        {
            for(int i=tmp2[3]-1; i<tmp1[3]-1; i++)
            {
                totalRtt = totalRtt+rttValue[m_tuple][i];
            }
        avgRtt = totalRtt/(tmp1[3]-tmp2[3]);
        }
        else
        {
            for(int i=0; i<tmp1[3]-1; i++)
            {
                totalRtt = totalRtt+rttValue[m_tuple][i];
            }
        avgRtt = totalRtt/(tmp1[3]-tmp2[3]);
        }
    }
    else
    {
        if(tmp2[3]>0)
        {
            avgRtt = rttValue[m_tuple][tmp2[3]-1];
        }
        else
        {
            avgRtt = 0.0001;
        }
    }


    std::vector<double> tmp3;
    tmp3.push_back(avgThroughput/1000.0);
    tmp3.push_back(avgLossRate);
    tmp3.push_back(avgRtt/10000.0);
    m_states.push_back(tmp3);
    //std::cout << avgRtt << std::endl;
    //std::cout << (tmp1[2]-tmp2[2]) << std::endl;
    //std::cout << "loss: " << avgLossRate << std::endl;
    //std::cout << "rtt: " << avgRtt << std::endl;
    //std::cout << "..............................." << std::endl;
    //outFile3.open("learning.csv", std::ios::app|std::ios::out);
    //outFile3 << m_tuple << "," << Now().GetMicroSeconds() << "," << avgThroughput << "," << avgRtt << "," << avgLossRate << "\n";
    //outFile3.close();
    return(tmp3);
  }
  else
  {
    std::vector<double> tmp3;
    tmp3.push_back(0.0001);
    tmp3.push_back(0.0001);
    tmp3.push_back(0.0001);
    m_states.push_back(tmp3);
    return(tmp3);
  }
}

void
ControlDecider::FlowSize(uint16_t m_tuple, int m_size)
{
    if(m_size<1000*10)
    {
        m_type = 1;
    }
    else if(m_size>=1000*10 and m_size<1000*100)
    {
        m_type = 2;
    }
    else if(m_size>=1000*100 and m_size<1000*200)
    {
        m_type = 3;
    }
    else
    {
        m_type = 4;
    }
    flowSize.insert(std::pair<uint16_t, int>(m_tuple,m_type));
}

void
ControlDecider::StartFlowSize(uint16_t m_tuple, int m_size)
{
    if(m_size<1000*10)
    {
        m_type = 1;
    }
    else if(m_size>=1000*10 and m_size<1000*100)
    {
        m_type = 2;
    }
    else if(m_size>=1000*100 and m_size<1000*200)
    {
        m_type = 3;
    }
    else
    {
        m_type =4;
    }
    startFlowSize.insert(std::pair<uint16_t, int>(m_tuple,m_type));
    flowSize.insert(std::pair<uint16_t, int>(m_tuple,m_type));
}

uint64_t
ControlDecider::UpdateCounts(uint16_t m_tuple)
{
  uint64_t tmp = updateCounts[m_tuple];
  tmp = tmp+1;
  updateCounts[m_tuple] = tmp;
  return(tmp);
}

void
ControlDecider::SwitchAlg(uint16_t m_tuple, std::vector<double> m_avg)
{
  //dqn
  int tmp;
  if(flowSize[m_tuple]==1)
  {
    tmp = 1;
  }
  else if(flowSize[m_tuple]==2)
  {
    //tmp = medium_dqn->GetAction(m_avg)+1;
    tmp = 4;
    //Savedata(m_tuple, medium_dqn);
  }
  else
  {
    //tmp = large_dqn->GetAction(m_avg)+4;
    tmp = 0;
    //SaveData(m_tuple, large_dqn);
  }
  switchAlg[m_tuple] = tmp;
}

void
ControlDecider::FlowStartTime(uint16_t m_tuple, uint64_t m_time)
{
    flowStartTime[m_tuple] = m_time;
}

void
ControlDecider::FlowCompleteTime(uint16_t m_tuple, uint64_t m_time)
{
    flowCompleteTime[m_tuple] = m_time;
    double totalRtt=0;
    double avgRtt;
    if((rttValue[m_tuple]).size()>0)
    {
        for(int i=0; i<(rttValue[m_tuple]).size(); i++)
        {
            totalRtt = totalRtt+rttValue[m_tuple][i];
        }
        avgRtt = totalRtt/(rttValue[m_tuple]).size();
    }
    else
    {
    avgRtt = defaultRtt*1000;
    }
    for(int i =1; i<5; i++)
    {
      if(startFlowSize[m_tuple]==i)
      {
      outFile2.open(std::to_string(i)+".csv", std::ios::app|std::ios::out);
      outFile2 << m_tuple << "," << 1000*8*(recvTime[m_tuple]).size()/(2.0*(flowCompleteTime[m_tuple]-flowStartTime[m_tuple])) << ","; // throughput in Mbps
      outFile2 << avgRtt/1000.0  << ","; //rtt in Microseconds
      outFile2 << (flowCompleteTime[m_tuple]-flowStartTime[m_tuple])/1000 << ",";// FCT in Microseconds
      outFile2 << (recvTime[m_tuple].size())/2 << ",";
      outFile2 << (reTime[m_tuple]).size()/(1.0*(recvTime[m_tuple]).size()/2) << "\n"; //loss rate
      outFile2.close();
      break;
      }
    }
}

void
ControlDecider::SaveFlowTime()
{

    //breakpoint for this flow
    std::vector<std::vector<double>> tmp_5000 = breakpoint[5000];
    std::vector<std::vector<double>> tmp_5001 = breakpoint[5001];
    std::vector<std::vector<double>> tmp_5002 = breakpoint[5002];

    double avgThroughput = 0;
    double avgThroughput_5000=0;
    double avgThroughput_5001=0;
    double avgThroughput_5002=0;
    double fairness = 0;
    double avgRtt = 0;
    double avgLossRate = 0;

    int tmpSize = tmp_5000.size();

    if(tmpSize==1)
    {
        //throughput Mbps
        double diffTime = tmp_5000[tmpSize-1][0]-flowStartTime[5000];
        avgThroughput = 1000*8*(tmp_5000[tmpSize-1][1]+tmp_5001[tmpSize-1][1]+tmp_5002[tmpSize-1][1])/(2*diffTime);
	avgThroughput_5000 = 1000*8*(tmp_5000[tmpSize-1][1])/(2*diffTime);
        avgThroughput_5001 = 1000*8*(tmp_5001[tmpSize-1][1])/(2*diffTime);
        avgThroughput_5002 = 1000*8*(tmp_5002[tmpSize-1][1])/(2*diffTime);
    
        //fairness
        fairness = pow((tmp_5000[tmpSize-1][1]+tmp_5001[tmpSize-1][1]+tmp_5002[tmpSize-1][1]),2)/(3*(pow(tmp_5000[tmpSize-1][1],2)+pow(tmp_5001[tmpSize-1][1],2)+pow(tmp_5002[tmpSize-1][1],2)));

        //rtt
        std::vector<double> tmpRtt;
        double sumRtt=0;
        int rttSize_5000 = tmp_5000[tmpSize-1][3];
        int rttSize_5001 = tmp_5001[tmpSize-1][3];
        int rttSize_5002 = tmp_5002[tmpSize-1][3];
        if(rttSize_5000>0)
        {
            for(int i=rttSize_5000-1; i>=0; i--)
            {
                tmpRtt.push_back(rttValue[5000][i]);
            }
        }
        if(rttSize_5001>0)
        {
            for(int i=rttSize_5001-1; i>=0; i--)
            {
                tmpRtt.push_back(rttValue[5001][i]);
            }
        }
        if(rttSize_5002>0)
        {
            for(int i=rttSize_5002-1; i>=0; i--)
            {
                tmpRtt.push_back(rttValue[5002][i]);
            }
        }
        for(int j=0; j<tmpRtt.size(); j++)
        {
            sumRtt += tmpRtt[j];
        }
	if(tmpRtt.size() ==0)
	{
	  avgRtt = defaultRtt;
	}
	else
	{
          avgRtt = sumRtt/(1000*tmpRtt.size());
	}

        //loss rate
        int lostNum = tmp_5000[tmpSize-1][2]+tmp_5001[tmpSize-1][2]+tmp_5002[tmpSize-1][2];
        int overNum = tmp_5000[tmpSize-1][1]+tmp_5001[tmpSize-1][1]+tmp_5002[tmpSize-1][1];
        avgLossRate = double(lostNum)/overNum;
    }
    if(tmpSize>1)
    {
        //throughput Mbps
        double diffTime = tmp_5000[tmpSize-1][0]-tmp_5000[tmpSize-2][0];
	int num1 = tmp_5000[tmpSize-1][1]-tmp_5000[tmpSize-2][1];
        int num2 = tmp_5001[tmpSize-1][1]-tmp_5001[tmpSize-2][1];
        avgThroughput = 1000*8*(num1+num2)/(2*diffTime);
	avgThroughput_5000 = 1000*8*(num1)/(2*diffTime);
        avgThroughput_5001 = 1000*8*(num2)/(2*diffTime);
    
        //fairness
	fairness = double(pow(num1+num2,2))/(2*(pow(num1,2)+pow(num2,2)));
        //std::cout << num1 << " " << num2 <<" " << num3 << std::endl;

        //rtt ms
        std::vector<double> tmpRtt;
        double sumRtt=0;
        int rttSize_5000 = tmp_5000[tmpSize-1][3];
        int rttSize_5001 = tmp_5001[tmpSize-1][3];
	//std::cout << rttSize_5000 << " " << rttSize_5001 << " "  << rttSize_5002 << std::endl;
        if(rttSize_5000>0)
        {
            for(int i=rttSize_5000-1; i>=tmp_5000[tmpSize-2][3]; i--)
            {
                tmpRtt.push_back(rttValue[5000][i]);
            }
        }
        if(rttSize_5001>0)
        {
            for(int i=rttSize_5001-1; i>=tmp_5001[tmpSize-1][3]; i--)
            {
                tmpRtt.push_back(rttValue[5001][i]);
            }
        }
        for(int j=0; j<tmpRtt.size(); j++)
        {
            sumRtt = sumRtt + tmpRtt[j];
        }
	if(tmpRtt.size()==0)
	{
	  avgRtt = defaultRtt;
	}
	else
	{
          avgRtt = sumRtt/(1000.0*tmpRtt.size());
	}

        //loss rate
        int lostNum = tmp_5000[tmpSize-1][2]+tmp_5001[tmpSize-1][2]-tmp_5000[tmpSize-2][2]-tmp_5001[tmpSize-2][2];
        int overNum = tmp_5000[tmpSize-1][1]+tmp_5001[tmpSize-1][1]-tmp_5000[tmpSize-2][1]-tmp_5001[tmpSize-2][1];
        avgLossRate = double(lostNum)/overNum;
    }

    outFile3.open("mqecn.csv", std::ios::app|std::ios::out);
    outFile3 << avgThroughput << "," << fairness << "," << avgRtt << "," << avgLossRate << ",";
    outFile3 << avgThroughput_5000 << "," << avgThroughput_5001 << "\n";
    outFile3.close();
    

    /*
    double totalRtt=0;
    double avgRtt;
    if((rttValue[m_tuple]).size()>0)
    {
        for(int i=0; i<(rttValue[m_tuple]).size(); i++)
        {
            totalRtt = totalRtt+rttValue[m_tuple][i];
        }
        avgRtt = totalRtt/(rttValue[m_tuple]).size();
    }
    else
    {
    avgRtt = 0.0001;
    }
    outFile2.open("myflowtime.csv", std::ios::app|std::ios::out);
    outFile2 << m_tuple << "," << 1000*8*(recvTime[m_tuple]).size()/(2.0*(flowCompleteTime[m_tuple]-flowStartTime[m_tuple])) << ","; // throughput in Mbps
    outFile2 << avgRtt/1000.0  << ","; //rtt in Microseconds
    outFile2 << (flowCompleteTime[m_tuple]-flowStartTime[m_tuple])/1000000.8 << ",";// FCT in Microseconds
    outFile2 << (reTime[m_tuple]).size()/(1.0*(recvTime[m_tuple]).size()/2) << "\n"; //loss rate 
    outFile2.close();
    */
}

void
ControlDecider::SetBpnet(Ptr<Bpnet> net1, Ptr<Bpnet> net2)
{
    //initilize neural networks and dqn algorithms
    medium_dqn->evalNet = net1;
    medium_dqn->targetNet = net2;
    large_dqn->evalNet = net1;
    large_dqn->targetNet = net2;

    //initilize the detailed parameters of neural networks
    medium_dqn->Init();
    large_dqn->Init();
}

void
ControlDecider::SaveData(uint16_t m_tuple, Ptr<DQN> net)
{
    outFile.open("mydata.csv", std::ios::app|std::ios::out);
    if((net->allStates).size()>0)
    {
        int sizeAll = (net->allStates).size();
        int sizeState = ((net->allStates)[sizeAll-1]).size();
        outFile << m_tuple << ",";
        for(int i=0; i<sizeState; i++)
        {
            outFile << (net->allStates)[sizeAll-1][i] << ",";
        }
        if(sizeAll == 1)
        {
            outFile << (net->allActionsIndex)[sizeAll-1] << "\n";
        }
        else
        {
            outFile << (net->allActionsIndex)[sizeAll-1] << "," << (net->allReward)[sizeAll-2] << "\n";
        }
    }

    //关闭文件

    outFile.close();
}

} // namespace ns3





