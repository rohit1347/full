/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */
#include "full-block-ack-agreement.h"

namespace ns3 {

FullBlockAckAgreement::FullBlockAckAgreement ()
  : m_amsduSupported (0),
    m_blockAckPolicy (1),
    m_inactivityEvent ()
{
}

FullBlockAckAgreement::FullBlockAckAgreement (Mac48Address peer, uint8_t tid)
  : m_amsduSupported (0),
    m_blockAckPolicy (1),
    m_inactivityEvent ()
{
  m_tid = tid;
  m_peer = peer;
}

FullBlockAckAgreement::~FullBlockAckAgreement ()
{
  m_inactivityEvent.Cancel ();
}

void
FullBlockAckAgreement::SetBufferSize (uint16_t bufferSize)
{
  NS_ASSERT (bufferSize <= 1024);
  NS_ASSERT (bufferSize % 16 == 0);
  m_bufferSize = bufferSize;
}
void
FullBlockAckAgreement::SetTimeout (uint16_t timeout)
{
  m_timeout = timeout;
}
void
FullBlockAckAgreement::SetStartingSequence (uint16_t seq)
{
  NS_ASSERT (seq < 4096);
  m_startingSeq = seq;
}
void
FullBlockAckAgreement::SetImmediateBlockAck (void)
{
  m_blockAckPolicy = 1;
}
void
FullBlockAckAgreement::SetDelayedBlockAck (void)
{
  m_blockAckPolicy = 0;
}
void
FullBlockAckAgreement::SetAmsduSupport (bool supported)
{
  m_amsduSupported = supported;
}

uint8_t
FullBlockAckAgreement::GetTid (void) const
{
  return m_tid;
}
Mac48Address
FullBlockAckAgreement::GetPeer (void) const
{
  return m_peer;
}
uint16_t
FullBlockAckAgreement::GetBufferSize (void) const
{
  return m_bufferSize;
}
uint16_t
FullBlockAckAgreement::GetTimeout (void) const
{
  return m_timeout;
}
uint16_t
FullBlockAckAgreement::GetStartingSequence (void) const
{
  return m_startingSeq;
}
uint16_t
FullBlockAckAgreement::GetStartingSequenceControl (void) const
{
  uint16_t seqControl = (m_startingSeq << 4) | 0xfff0;
  return seqControl;
}
bool
FullBlockAckAgreement::IsImmediateBlockAck (void) const
{
  return (m_blockAckPolicy == 1);
}
bool
FullBlockAckAgreement::IsAmsduSupported (void) const
{
  return (m_amsduSupported == 1) ? true : false;
}

} // namespace ns3
