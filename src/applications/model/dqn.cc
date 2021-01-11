#include "ns3/dqn.h"
#include "ns3/bpnet.h"
#include <time.h>
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <numeric>
#include <fstream>
#include <sstream>
#include <math.h>
#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("DQN");

NS_OBJECT_ENSURE_REGISTERED (DQN);

TypeId
DQN::GetTypeId (void)
{
  	static TypeId tid = TypeId ("ns3::DQN")
	    .SetParent<Object> ()
	    .SetGroupName("Applications")
	    .AddConstructor<DQN> ()
	    ;
	return tid;
}


DQN::DQN()
{
  NS_LOG_FUNCTION (this);
}

DQN::~DQN()
{
	NS_LOG_FUNCTION(this);
}

void
DQN::Init()
{
    evalNet->Init();
    targetNet->Init();
}

uint64_t
DQN::GetAction(std::vector<double> avg)
{
  //current state
  std::vector<double> m_states = avg;

  if(m_states[1]<0.5)
  {
    m_states[1]=m_states[1]+0.0001;
  }
  else
  {
    m_states[1]=m_states[1]-0.0001;
  }



  //predict action
  sample Inout;
  Inout.in = m_states;
  Inout.out = m_states;
  std::vector<sample> m_input;
  m_input.push_back(Inout);
  std::vector<double> m_output = evalNet->predict(m_input);

  //save the action
  allActions.push_back(m_output);

  //store the transitions
  if(allStates.size()!=0)
  {
    sample tmp;
    int a_index = allActionsIndex[allActionsIndex.size()-1];
    std::vector<double> q_next_vector = targetNet->predict(m_input);
    double q_next = 0.0;
    for(int i=0; i<q_next_vector.size();i++)
    {
        if(q_next_vector[i]>q_next)
        {
            q_next = q_next_vector[i];
        }
    }

    double r = log(m_states[0])-(m_states[1])-log(m_states[2]);
    allReward.push_back(r);
    double q_target = (r)/10.0+ GAMMA*q_next;
    std::vector<double> pre = allActions[allActions.size()-1];
    pre[a_index] = q_target; 
    tmp.in = allStates[allStates.size()-1];
    tmp.out = pre;
    m_transitions.push_back(tmp);
  }

  learnStep = learnStep+1;
  if(learnStep % learnIter == 0 and learnStep>=learnIter)
  {
    Learn();
  }

  //get the action
  int m_size = m_output.size();
  srand((unsigned)time(NULL));   
  if((rand()%100)/100.0<epsilon)
  {
    int m_index = max_element(m_output.begin(),m_output.end())-m_output.begin();
    allStates.push_back(m_states);
    allActionsIndex.push_back(m_index);
    return(m_index);
  }
  else
  {
    srand((unsigned)time(NULL));  
    allStates.push_back(m_states);
    int m_index = (rand()%(m_size-1));
    allActionsIndex.push_back(m_index);
    return(m_index);
  }

}


void
DQN::Learn()
{
  if(targetStep % targetIter==0 and targetStep>=targetIter)
  {
    //load the parameters of evalNet to targetNet
    for(int i=0; i<innode; i++)
    {
      memcpy(&(targetNet->inputLayer[i]), &(evalNet->inputLayer[i]), sizeof(inputNode));
    }
    for(int i=0; i<outnode; i++)
    {
      memcpy(&(targetNet->outputLayer[i]), &(evalNet->outputLayer[i]), sizeof(outputNode));
    }
    for(int i=0; i<hidelayer; i++)
    {
      for(int j=0; j<hidenode; j++)
      {
        memcpy(&(targetNet->hiddenLayer[i][j]), &(evalNet->hiddenLayer[i][j]), sizeof(hiddenNode));
      }
    }
  }
  targetStep = targetStep+1;
  evalNet->training(m_transitions, 0.01);
}

} // namespace ns3





