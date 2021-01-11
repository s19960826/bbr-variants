#include <ostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

#include "ns3/stats-module.h"

#include "ns3/address-tag.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AddressTag");

NS_OBJECT_ENSURE_REGISTERED (AddressTag);


//----------------------------------------------------------------------
//-- AddressTag
//------------------------------------------------------
TypeId 
AddressTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AddressTag")
    .SetParent<Tag> ()
    .SetGroupName ("Network")
    .AddConstructor<AddressTag> ()
    .AddAttribute ("Address",
                   "the type of flow size!",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&AddressTag::GetAddress),
                   MakeUintegerChecker<uint32_t>())
  ;
  return tid;
}
TypeId 
AddressTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
AddressTag::GetSerializedSize (void) const
{
  return 8;
}

void 
AddressTag::Serialize (TagBuffer i) const
{
  //int64_t t = m_type;
  //i.Write ((const uint8_t *)&t, 8);
  i.WriteU32(m_add);
}

void 
AddressTag::Deserialize (TagBuffer i)
{
  //int64_t t;
  //i.Read ((uint8_t *)&t, 8);
  //m_type = t;
  m_add = i.ReadU32();
}

void
AddressTag::SetAddress (uint32_t add)
{
  m_add = add;
}

uint32_t
AddressTag::GetAddress (void) const
{
  return m_add;
}

void 
AddressTag::Print (std::ostream &os) const
{
  os << "t=" << m_add;
}
