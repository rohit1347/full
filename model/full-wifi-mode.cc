/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "full-wifi-mode.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3 {

bool operator == (const FullWifiMode &a, const FullWifiMode &b)
{
  return a.GetUid () == b.GetUid ();
}
std::ostream & operator << (std::ostream & os, const FullWifiMode &mode)
{
  os << mode.GetUniqueName ();
  return os;
}
std::istream & operator >> (std::istream &is, FullWifiMode &mode)
{
  std::string str;
  is >> str;
  mode = FullWifiModeFactory::GetFactory ()->Search (str);
  return is;
}

uint32_t
FullWifiMode::GetBandwidth (void) const
{
  struct FullWifiModeFactory::FullWifiModeItem *item = FullWifiModeFactory::GetFactory ()->Get (m_uid);
  return item->bandwidth;
}
uint64_t
FullWifiMode::GetPhyRate (void) const
{
  struct FullWifiModeFactory::FullWifiModeItem *item = FullWifiModeFactory::GetFactory ()->Get (m_uid);
  return item->phyRate;
}
uint64_t
FullWifiMode::GetDataRate (void) const
{
  struct FullWifiModeFactory::FullWifiModeItem *item = FullWifiModeFactory::GetFactory ()->Get (m_uid);
  return item->dataRate;
}
enum FullWifiCodeRate
FullWifiMode::GetCodeRate (void) const
{
  struct FullWifiModeFactory::FullWifiModeItem *item = FullWifiModeFactory::GetFactory ()->Get (m_uid);
  return item->codingRate;
}
uint8_t
FullWifiMode::GetConstellationSize (void) const
{
  struct FullWifiModeFactory::FullWifiModeItem *item = FullWifiModeFactory::GetFactory ()->Get (m_uid);
  return item->constellationSize;
}
std::string
FullWifiMode::GetUniqueName (void) const
{
  // needed for ostream printing of the invalid mode
  struct FullWifiModeFactory::FullWifiModeItem *item = FullWifiModeFactory::GetFactory ()->Get (m_uid);
  return item->uniqueUid;
}
bool
FullWifiMode::IsMandatory (void) const
{
  struct FullWifiModeFactory::FullWifiModeItem *item = FullWifiModeFactory::GetFactory ()->Get (m_uid);
  return item->isMandatory;
}
uint32_t
FullWifiMode::GetUid (void) const
{
  return m_uid;
}
enum FullWifiModulationClass
FullWifiMode::GetModulationClass () const
{
  struct FullWifiModeFactory::FullWifiModeItem *item = FullWifiModeFactory::GetFactory ()->Get (m_uid);
  return item->modClass;
}
FullWifiMode::FullWifiMode ()
  : m_uid (0)
{
}
FullWifiMode::FullWifiMode (uint32_t uid)
  : m_uid (uid)
{
}
FullWifiMode::FullWifiMode (std::string name)
{
  *this = FullWifiModeFactory::GetFactory ()->Search (name);
}

ATTRIBUTE_HELPER_CPP (FullWifiMode);

FullWifiModeFactory::FullWifiModeFactory ()
{
}


FullWifiMode
FullWifiModeFactory::CreateWifiMode (std::string uniqueName,
                                 enum FullWifiModulationClass modClass,
                                 bool isMandatory,
                                 uint32_t bandwidth,
                                 uint32_t dataRate,
                                 enum FullWifiCodeRate codingRate,
                                 uint8_t constellationSize)
{
  FullWifiModeFactory *factory = GetFactory ();
  uint32_t uid = factory->AllocateUid (uniqueName);
  FullWifiModeItem *item = factory->Get (uid);
  item->uniqueUid = uniqueName;
  item->modClass = modClass;
  // The modulation class for this WifiMode must be valid.
  NS_ASSERT (modClass != FULL_WIFI_MOD_CLASS_UNKNOWN);

  item->bandwidth = bandwidth;
  item->dataRate = dataRate;

  item->codingRate = codingRate;

  switch (codingRate)
    {
    case FULL_WIFI_CODE_RATE_3_4:
      item->phyRate = dataRate * 4 / 3;
      break;
    case FULL_WIFI_CODE_RATE_2_3:
      item->phyRate = dataRate * 3 / 2;
      break;
    case FULL_WIFI_CODE_RATE_1_2:
      item->phyRate = dataRate * 2 / 1;
      break;
    case FULL_WIFI_CODE_RATE_UNDEFINED:
    default:
      item->phyRate = dataRate;
      break;
    }

  // Check for compatibility between modulation class and coding
  // rate. If modulation class is DSSS then coding rate must be
  // undefined, and vice versa. I could have done this with an
  // assertion, but it seems better to always give the error (i.e.,
  // not only in non-optimised builds) and the cycles that extra test
  // here costs are only suffered at simulation setup.
  if ((codingRate == FULL_WIFI_CODE_RATE_UNDEFINED) != (modClass == FULL_WIFI_MOD_CLASS_DSSS))
    {
      NS_FATAL_ERROR ("Error in creation of WifiMode named " << uniqueName << std::endl
                                                             << "Code rate must be WIFI_CODE_RATE_UNDEFINED iff Modulation Class is WIFI_MOD_CLASS_DSSS");
    }

  item->constellationSize = constellationSize;
  item->isMandatory = isMandatory;

  return FullWifiMode (uid);
}

FullWifiMode
FullWifiModeFactory::Search (std::string name)
{
  FullWifiModeItemList::const_iterator i;
  uint32_t j = 0;
  for (i = m_itemList.begin (); i != m_itemList.end (); i++)
    {
      if (i->uniqueUid == name)
        {
          return FullWifiMode (j);
        }
      j++;
    }

  // If we get here then a matching WifiMode was not found above. This
  // is a fatal problem, but we try to be helpful by displaying the
  // list of WifiModes that are supported.
  NS_LOG_UNCOND ("Could not find match for WifiMode named \""
                 << name << "\". Valid options are:");
  for (i = m_itemList.begin (); i != m_itemList.end (); i++)
    {
      NS_LOG_UNCOND ("  " << i->uniqueUid);
    }
  // Empty fatal error to die. We've already unconditionally logged
  // the helpful information.
  NS_FATAL_ERROR ("");

  // This next line is unreachable because of the fatal error
  // immediately above, and that is fortunate, because we have no idea
  // what is in WifiMode (0), but we do know it is not what our caller
  // has requested by name. It's here only because it's the safest
  // thing that'll give valid code.
  return FullWifiMode (0);
}

uint32_t
FullWifiModeFactory::AllocateUid (std::string uniqueUid)
{
  uint32_t j = 0;
  for (FullWifiModeItemList::const_iterator i = m_itemList.begin ();
       i != m_itemList.end (); i++)
    {
      if (i->uniqueUid == uniqueUid)
        {
          return j;
        }
      j++;
    }
  uint32_t uid = m_itemList.size ();
  m_itemList.push_back (FullWifiModeItem ());
  return uid;
}

struct FullWifiModeFactory::FullWifiModeItem *
FullWifiModeFactory::Get (uint32_t uid)
{
  NS_ASSERT (uid < m_itemList.size ());
  return &m_itemList[uid];
}

FullWifiModeFactory *
FullWifiModeFactory::GetFactory (void)
{
  static bool isFirstTime = true;
  static FullWifiModeFactory factory;
  if (isFirstTime)
    {
      uint32_t uid = factory.AllocateUid ("Invalid-WifiMode");
      FullWifiModeItem *item = factory.Get (uid);
      item->uniqueUid = "Invalid-WifiMode";
      item->bandwidth = 0;
      item->dataRate = 0;
      item->phyRate = 0;
      item->modClass = FULL_WIFI_MOD_CLASS_UNKNOWN;
      item->constellationSize = 0;
      item->codingRate = FULL_WIFI_CODE_RATE_UNDEFINED;
      item->isMandatory = false;
      isFirstTime = false;
    }
  return &factory;
}

} // namespace ns3
