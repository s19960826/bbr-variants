#ifndef DQN_H
#define DQN_H

#include <random>
#include "ns3/bpnet.h"
#include "ns3/application.h"
#include <vector>
#include <numeric>
#include <map>


namespace ns3
{

class DQN : public Object
{
public:
    static TypeId GetTypeId (void);
    DQN();
    ~DQN();

    uint64_t GetAction(std::vector<double> avg);
    void Learn();
    void Init();

    Ptr<Bpnet> evalNet;
    Ptr<Bpnet> targetNet;
    double epsilon = 0.9;
    std::vector<std::vector<double>> allStates;
    std::vector<std::vector<double>> allActions;
    std::vector<int> allActionsIndex;
    std::vector<double> allReward;
    std::vector<sample> m_transitions;
    int learnStep = 0;  //learn
    int learnIter = 5;  //learn iterations
    int targetStep = 0;
    int targetIter = 5;
    double GAMMA = 0.9;

private:
    int m;
};

} // namespace ns3
#endif
