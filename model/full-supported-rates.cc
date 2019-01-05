/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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

#include "full-supported-rates.h"
#include "ns3/assert.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("FullSupportedRates");

namespace ns3 {

FullSupportedRates::FullSupportedRates ()
  : extended (this),
    m_nRates (0)
{
}

void
FullSupportedRates::AddSupportedRate (uint32_t bs)
{
  NS_ASSERT (m_nRates < MAX_SUPPORTED_RATES);
  if (IsSupportedRate (bs))
    {
      return;
    }
  m_rates[m_nRates] = bs / 500000;
  m_nRates++;
  NS_LOG_DEBUG ("add rate=" << bs << ", n rates=" << (uint32_t)m_nRates);
}
void
FullSupportedRates::SetBasicRate (uint32_t bs)
{
  uint8_t rate = bs / 500000;
  for (uint8_t i = 0; i < m_nRates; i++)
    {
      if ((rate | 0x80) == m_rates[i])
        {
          return;
        }
      if (rate == m_rates[i])
        {
          NS_LOG_DEBUG ("set basic rate=" << bs << ", n rates=" << (uint32_t)m_nRates);
          m_rates[i] |= 0x80;
          return;
        }
    }
  AddSupportedRate (bs);
  SetBasicRate (bs);
}
bool
FullSupportedRates::IsBasicRate (uint32_t bs) const
{
  uint8_t rate = (bs / 500000) | 0x80;
  for (uint8_t i = 0; i < m_nRates; i++)
    {
      if (rate == m_rates[i])
        {
          return true;
        }
    }
  return false;
}
bool
FullSupportedRates::IsSupportedRate (uint32_t bs) const
{
  uint8_t rate = bs / 500000;
  for (uint8_t i = 0; i < m_nRates; i++)
    {
      if (rate == m_rates[i]
          || (rate | 0x80) == m_rates[i])
        {
          return true;
        }
    }
  return false;
}
uint8_t
FullSupportedRates::GetNRates (void) const
{
  return m_nRates;
}
uint32_t
FullSupportedRates::GetRate (uint8_t i) const
{
  return (m_rates[i] & 0x7f) * 500000;
}

FullWifiInformationElementId
FullSupportedRates::ElementId () const
{
  return IE_SUPPORTED_RATES;
}
uint8_t
FullSupportedRates::GetInformationFieldSize () const
{
  // The Supported Rates Information Element contains only the first 8
  // supported rates - the remainder appear in the Extended Supported
  // Rates Information Element.
  return m_nRates > 8 ? 8 : m_nRates;
}
void
FullSupportedRates::SerializeInformationField (Buffer::Iterator start) const
{
  // The Supported Rates Information Element contains only the first 8
  // supported rates - the remainder appear in the Extended Supported
  // Rates Information Element.
  start.Write (m_rates, m_nRates > 8 ? 8 : m_nRates);
}
uint8_t
FullSupportedRates::DeserializeInformationField (Buffer::Iterator start,
                                             uint8_t length)
{
  NS_ASSERT (length <= 8);
  m_nRates = length;
  start.Read (m_rates, m_nRates);
  return m_nRates;
}

FullExtendedSupportedRatesIE::FullExtendedSupportedRatesIE ()
{
}

FullExtendedSupportedRatesIE::FullExtendedSupportedRatesIE (FullSupportedRates *sr)
{
  m_supportedRates = sr;
}

FullWifiInformationElementId
FullExtendedSupportedRatesIE::ElementId () const
{
  return IE_EXTENDED_SUPPORTED_RATES;
}

uint8_t
FullExtendedSupportedRatesIE::GetInformationFieldSize () const
{
  // If there are 8 or fewer rates then we don't need an Extended
  // Supported Rates IE and so could return zero here, but we're
  // overriding the GetSerializedSize() method, so if this function is
  // invoked in that case then it indicates a programming error. Hence
  // we have an assertion on that condition.
  NS_ASSERT (m_supportedRates->m_nRates > 8);

  // The number of rates we have beyond the initial 8 is the size of
  // the information field.
  return (m_supportedRates->m_nRates - 8);
}

void
FullExtendedSupportedRatesIE::SerializeInformationField (Buffer::Iterator start) const
{
  // If there are 8 or fewer rates then there should be no Extended
  // Supported Rates Information Element at all so being here would
  // seemingly indicate a programming error.
  //
  // Our overridden version of the Serialize() method should ensure
  // that this routine is never invoked in that case (by ensuring that
  // WifiInformationElement::Serialize() is not invoked).
  NS_ASSERT (m_supportedRates->m_nRates > 8);
  start.Write (m_supportedRates->m_rates + 8, m_supportedRates->m_nRates - 8);
}

Buffer::Iterator
FullExtendedSupportedRatesIE::Serialize (Buffer::Iterator start) const
{
  // If there are 8 or fewer rates then we don't need an Extended
  // Supported Rates IE, so we don't serialise anything.
  if (m_supportedRates->m_nRates <= 8)
    {
      return start;
    }

  // If there are more than 8 rates then we serialise as per normal.
  return FullWifiInformationElement::Serialize (start);
}

uint16_t
FullExtendedSupportedRatesIE::GetSerializedSize () const
{
  // If there are 8 or fewer rates then we don't need an Extended
  // Supported Rates IE, so it's serialised length will be zero.
  if (m_supportedRates->m_nRates <= 8)
    {
      return 0;
    }

  // Otherwise, the size of it will be the number of supported rates
  // beyond 8, plus 2 for the Element ID and Length.
  return FullWifiInformationElement::GetSerializedSize ();
}

uint8_t
FullExtendedSupportedRatesIE::DeserializeInformationField (Buffer::Iterator start,
                                                       uint8_t length)
{
  NS_ASSERT (length > 0);
  NS_ASSERT (m_supportedRates->m_nRates + length <= MAX_SUPPORTED_RATES);
  start.Read (m_supportedRates->m_rates + m_supportedRates->m_nRates, length);
  m_supportedRates->m_nRates += length;
  return length;
}

std::ostream &operator << (std::ostream &os, const FullSupportedRates &rates)
{
  os << "[";
  for (uint8_t i = 0; i < rates.GetNRates (); i++)
    {
      uint32_t rate = rates.GetRate (i);
      if (rates.IsBasicRate (rate))
        {
          os << "*";
        }
      os << rate / 1000000 << "mbs";
      if (i < rates.GetNRates () - 1)
        {
          os << " ";
        }
    }
  os << "]";
  return os;
}

} // namespace ns3
