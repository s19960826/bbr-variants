#include "multiple-flows.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <numeric>
#include <fstream>
#include <sstream>
#include <math.h>
#include "ns3/node.h"
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <map>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include <ctime>
#include <vector>
#include "ns3/control-decider.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("MultipleFlows");

NS_OBJECT_ENSURE_REGISTERED (MultipleFlows);

TypeId
MultipleFlows::GetTypeId (void)
{
  	static TypeId tid = TypeId ("ns3::MultipleFlows")
	    .SetParent<Object> ()
	    .SetGroupName("Applications")
	    .AddConstructor<MultipleFlows> ()
	    ;
	return tid;
}


MultipleFlows::MultipleFlows()
{
	NS_LOG_FUNCTION(this);
}

MultipleFlows::~MultipleFlows()
{
	NS_LOG_FUNCTION(this);
}

void 
MultipleFlows::init_cdf(struct cdf_table *table)
{
    if(!table)
        return;

    table->entries = (struct cdf_entry*)malloc(TG_CDF_TABLE_ENTRY * sizeof(struct cdf_entry));
    table->num_entry = 0;
    table->max_entry = TG_CDF_TABLE_ENTRY;
    table->min_cdf = 0;
    table->max_cdf = 1;

    if (!(table->entries))
        perror("Error: malloc entries in init_cdf()");
}

/* free resources of a CDF distribution */
void 
MultipleFlows::free_cdf(struct cdf_table *table)
{
    if (table)
        free(table->entries);
}

/* get CDF distribution from a given file */
void 
MultipleFlows::load_cdf(struct cdf_table *table, const char *file_name)
{
    FILE *fd = NULL;
    char line[256] = {0};
    struct cdf_entry *e = NULL;
    int i = 0;

    // char buffer[100];
    // getcwd(buffer, 100);
    // std::cout<<buffer<<std::endl;

    if (!table)
        return;

    fd = fopen(file_name, "r");
    if (!fd)
        perror("Error: open the CDF file in load_cdf()");

    while (fgets(line, sizeof(line), fd))
    {
        /* resize entries */
        if (table->num_entry >= table->max_entry)
        {
            table->max_entry *= 2;
            e = (struct cdf_entry*)malloc(table->max_entry * sizeof(struct cdf_entry));
            if (!e)
                perror("Error: malloc entries in load_cdf()");
            for (i = 0; i < table->num_entry; i++)
                e[i] = table->entries[i];
            free(table->entries);
            table->entries = e;
        }

        sscanf(line, "%lf %lf", &(table->entries[table->num_entry].value), &(table->entries[table->num_entry].cdf));

        if (table->min_cdf > table->entries[table->num_entry].cdf)
            table->min_cdf = table->entries[table->num_entry].cdf;
        if (table->max_cdf < table->entries[table->num_entry].cdf)
            table->max_cdf = table->entries[table->num_entry].cdf;

        table->num_entry++;
    }
    fclose(fd);
}

/* print CDF distribution information */
void 
MultipleFlows::print_cdf(struct cdf_table *table)
{
    int i = 0;

    if (!table)
        return;

    for (i = 0; i < table->num_entry; i++)
        printf("%.2f %.2f\n", table->entries[i].value, table->entries[i].cdf);
}

/* get average value of CDF distribution */
double 
MultipleFlows::avg_cdf(struct cdf_table *table)
{
    int i = 0;
    double avg = 0;
    double value, prob;

    if (!table)
        return 0;

    for (i = 0; i < table->num_entry; i++)
    {
        if (i == 0)
        {
            value = table->entries[i].value / 2;
            prob = table->entries[i].cdf;
        }
        else
        {
            value = (table->entries[i].value + table->entries[i-1].value) / 2;
            prob = table->entries[i].cdf - table->entries[i-1].cdf;
        }
        avg += (value * prob);
    }

    return avg;
}

double 
MultipleFlows::interpolate(double x, double x1, double y1, double x2, double y2)
{
    if (x1 == x2)
        return (y1 + y2) / 2;
    else
        return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

/* generate a random floating point number from min to max */
double 
MultipleFlows::rand_range(double min, double max)
{
    return min + rand() * (max - min) / RAND_MAX;
}

int 
MultipleFlows::rand_range(int min, int max)
{
    return min + ((double)max - min) * rand () / RAND_MAX;
}

/* generate a random value based on CDF distribution */
double 
MultipleFlows::gen_random_cdf(struct cdf_table *table)
{
    int i = 0;
    double x = rand_range(table->min_cdf, table->max_cdf);
    /* printf("%f %f %f\n", x, table->min_cdf, table->max_cdf); */

    if (!table)
        return 0;

    for (i = 0; i < table->num_entry; i++)
    {
        if (x <= table->entries[i].cdf)
        {
            if (i == 0)
                return interpolate(x, 0, 0, table->entries[i].cdf, table->entries[i].value);
            else
                return interpolate(x, table->entries[i-1].cdf, table->entries[i-1].value, table->entries[i].cdf, table->entries[i].value);
        }
    }

    return table->entries[table->num_entry-1].value;
}

double 
MultipleFlows::poission_gen_interval(double avg_rate)
{
    if (avg_rate > 0)
       return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
    else
       return 0;
}


ApplicationContainer 
MultipleFlows::InstallApplication(Ptr<Node> sendNode, Ptr<Node> sinkNode,
    Ipv4Address sourceAddress, Ipv4Address remoteAddress, uint16_t port, uint32_t maxBytes, uint32_t sendSize, uint16_t m_tuple, Ptr<ControlDecider> m_controller)
{
    ApplicationContainer applications;

    ObjectFactory m_sendApplicationFactory("ns3::BulkSendApplication");
    m_sendApplicationFactory.Set("Protocol", StringValue("ns3::TcpSocketFactory"));
    m_sendApplicationFactory.Set("Remote", AddressValue(InetSocketAddress(remoteAddress, port)));
    m_sendApplicationFactory.Set("MaxBytes", UintegerValue(maxBytes));
    m_sendApplicationFactory.Set("SendSize", UintegerValue(sendSize));
    //m_sendApplicationFactory.Set("MyAddress", AddressValue(sourceAddress));
    Ptr<BulkSendApplication> sendApplication = m_sendApplicationFactory.Create<BulkSendApplication>();
    sendApplication->SetAppController(m_controller, m_tuple);

    sendNode->AddApplication(sendApplication);
    applications.Add(sendApplication);

    ObjectFactory m_sinkApplicationFactory("ns3::PacketSink");
    m_sinkApplicationFactory.Set("Protocol", StringValue("ns3::TcpSocketFactory"));
    m_sinkApplicationFactory.Set("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), port)));
    Ptr<PacketSink> sinkApplication = m_sinkApplicationFactory.Create<PacketSink>();
    sinkNode->AddApplication(sinkApplication);
    applications.Add(sinkApplication);

    return applications;
}

void 
MultipleFlows::InstallAllApplications(NodeContainer fromServers, NodeContainer destServers, double requestRate,
		struct cdf_table *cdfTable, std::vector<Ipv4Address> sourceAddresses, std::vector<Ipv4Address> destAddresses, 
		int flowCount, uint16_t start_port, int packet_size, Time timesim_start, Time timesim_end,Time time_flow_launch_end, Ptr<ControlDecider> m_controller)
{
    NS_LOG_INFO("Install applications...");
    int src_leaf_node_count = fromServers.GetN();
    int dst_leaf_node_count = destServers.GetN();
    std::map<std::string, std::vector <uint64_t>> mapTuple;

    for (int i = 0; i < src_leaf_node_count; i++)
    {
        Time startTime = timesim_start + Seconds(poission_gen_interval(int(requestRate)));

        while (startTime < time_flow_launch_end)
        {
            flowCount++;
            int destIndex;
	    destIndex = i;
            // 确保应用的源和目的不是同一服务器
            /*
	    do
            {
		        //srand((unsigned)time(NULL));
		        srand(startTime.GetMicroSeconds());
                destIndex = (rand()%(dst_leaf_node_count/2))+(dst_leaf_node_count/2);
            } while (destServers.Get(destIndex)->GetId() == fromServers.Get(i)->GetId());
            */
	    uint32_t flowSize = gen_random_cdf (cdfTable);
            //uint32_t flowSize = 1002*5;

            ApplicationContainer applications;

	        //std::ostringstream os;
            //os << i << " " <<  start_port << " "  << destIndex  << " ";
            std::cout << i << " " << flowSize/1024.0 << " " << destIndex << " " << sourceAddresses[i] << std::endl;
            //std::string m_tuple= os.str();
            //std::cout << "port is: " << start_port << std::endl;
            uint16_t m_tuple = start_port;
            //std::cout << "flow count is: " << flowCount << std::endl;
            //std::cout << ".........................................." << std::endl;


            applications = MultipleFlows::InstallApplication(fromServers.Get(i), destServers.Get(destIndex),
            sourceAddresses[i], destAddresses[destIndex], start_port, flowSize, packet_size, m_tuple, m_controller);
            applications.Get(0)->SetStartTime(startTime); //send
            applications.Get(0)->SetStopTime(timesim_end);
            applications.Get(1)->SetStartTime(timesim_start); //sink
            applications.Get(1)->SetStopTime(timesim_end);

            NS_LOG_INFO("\tFlow from server: " << i << " to server: "
                                               << destIndex << " on port: " << start_port << " with flow size: "
                                               << flowSize << " [start time: " << startTime << "]");

            startTime += Seconds(poission_gen_interval (requestRate));
            // 确保端口不会相互冲突
            start_port += 1;
        }
    }
    NS_LOG_INFO("Finish installing applications.");
}



} // namespace ns3





