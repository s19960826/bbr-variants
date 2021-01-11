#ifndef IPV4_PRIORITY_TAG_H
#define IPV4_PRIORITY_TAG_H

#include "ns3/tag.h"

/*
priority level: 0 - highest, larger value for lower priority
default value is: 0
*/

namespace ns3 {

class Ipv4PriorityTag: public Tag
{
public:
    Ipv4PriorityTag ();

    static TypeId GetTypeId (void);

    void SetPriorityTag(uint32_t priorityTag);      

    uint32_t GetPriorityTag(void) const;

    virtual TypeId GetInstanceTypeId (void) const;

    virtual uint32_t GetSerializedSize (void) const;

    virtual void Serialize (TagBuffer i) const;

    virtual void Deserialize (TagBuffer i);

    virtual void Print (std::ostream &os) const;

private:
    uint32_t m_priorityTag;
};

}

#endif
