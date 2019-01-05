/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 MIRKO BANCHI
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
#ifndef FULL_BLOCK_ACK_CACHE_H
#define FULL_BLOCK_ACK_CACHE_H

#include <stdint.h>

namespace ns3 {

class FullWifiMacHeader;
class FullCtrlBAckResponseHeader;

/**
 * \ingroup wifi
 *
 *
 */
class FullBlockAckCache
{
public:
  void Init (uint16_t winStart, uint16_t winSize);

  void UpdateWithMpdu (const FullWifiMacHeader *hdr);
  void UpdateWithBlockAckReq (uint16_t startingSeq);

  void FillBlockAckBitmap (FullCtrlBAckResponseHeader *blockAckHeader);
private:
  void ResetPortionOfBitmap (uint16_t start, uint16_t end);
  bool IsInWindow (uint16_t seq);

  uint16_t m_winStart;
  uint8_t m_winSize;
  uint16_t m_winEnd;

  uint16_t m_bitmap[4096];
};

} // namespace ns3

#endif /* FULL_BLOCK_ACK_CACHE_H */
