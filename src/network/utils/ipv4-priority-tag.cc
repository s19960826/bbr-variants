#include "ns3/ipv4-priority-tag.h"

namespace ns3
{

Ipv4PriorityTag::Ipv4PriorityTag () 
    : m_priorityTag(0)
{

}

TypeId
Ipv4PriorityTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4PriorityTag")
    .SetParent<Tag> ()
    .SetGroupName ("Network")
    .AddConstructor<Ipv4PriorityTag> ();
  return tid;
}

void
Ipv4PriorityTag::SetPriorityTag (uint32_t priorityTag)
{
  m_priorityTag = priorityTag;
}

uint32_t
Ipv4PriorityTag::GetPriorityTag (void) const
{
  return m_priorityTag;
}


TypeId
Ipv4PriorityTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
Ipv4PriorityTag::GetSerializedSize (void) const
{
  return sizeof (uint32_t);
}

void
Ipv4PriorityTag::Serialize (TagBuffer i) const
{
  i.WriteU32(m_priorityTag);
}

void
Ipv4PriorityTag::Deserialize (TagBuffer i)
{
  m_priorityTag = i.ReadU32 ();
}

void
Ipv4PriorityTag::Print (std::ostream &os) const
{
  os << "Priority Tag = " << m_priorityTag;
}

}
