/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
#include "full-wifi-phy-state-helper.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"

NS_LOG_COMPONENT_DEFINE ("FullWifiPhyStateHelper");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FullWifiPhyStateHelper);

TypeId
FullWifiPhyStateHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FullWifiPhyStateHelper")
    .SetParent<Object> ()
    .AddConstructor<FullWifiPhyStateHelper> ()
    .AddTraceSource ("State",
                     "The state of the PHY layer",
                     MakeTraceSourceAccessor (&FullWifiPhyStateHelper::m_stateLogger))
    .AddTraceSource ("RxOk",
                     "A packet has been received successfully.",
                     MakeTraceSourceAccessor (&FullWifiPhyStateHelper::m_rxOkTrace))
    .AddTraceSource ("RxError",
                     "A packet has been received unsuccessfully.",
                     MakeTraceSourceAccessor (&FullWifiPhyStateHelper::m_rxErrorTrace))
    .AddTraceSource ("Tx", "Packet transmission is starting.",
                     MakeTraceSourceAccessor (&FullWifiPhyStateHelper::m_txTrace))
  ;
  return tid;
}

FullWifiPhyStateHelper::FullWifiPhyStateHelper ()
  : m_rxing (false),
    m_endTx (Seconds (0)),
    m_endRx (Seconds (0)),
    m_endCcaBusy (Seconds (0)),
    m_endSwitching (Seconds (0)),
    m_startTx (Seconds (0)),
    m_startRx (Seconds (0)),
    m_startCcaBusy (Seconds (0)),
    m_startSwitching (Seconds (0)),
    m_previousStateChangeTime (Seconds (0))
{
  NS_LOG_FUNCTION (this);
}

void
FullWifiPhyStateHelper::SetReceiveOkCallback (FullWifiPhy::RxOkCallback callback)
{
  m_rxOkCallback = callback;
}
void
FullWifiPhyStateHelper::SetReceiveErrorCallback (FullWifiPhy::RxErrorCallback callback)
{
  m_rxErrorCallback = callback;
}
void
FullWifiPhyStateHelper::RegisterListener (FullWifiPhyListener *listener)
{
  m_listeners.push_back (listener);
}

bool
FullWifiPhyStateHelper::IsStateIdle (void)
{
  return (GetState () == FullWifiPhy::IDLE);
}
bool
FullWifiPhyStateHelper::IsStateBusy (void)
{
  return (GetState () != FullWifiPhy::IDLE);
}
bool
FullWifiPhyStateHelper::IsStateCcaBusy (void)
{
  return (GetState () == FullWifiPhy::CCA_BUSY);
}
bool
FullWifiPhyStateHelper::IsStateRx (void)
{
  return (GetState () == FullWifiPhy::RX);
}
bool
FullWifiPhyStateHelper::IsStateTx (void)
{
  return (GetState () == FullWifiPhy::TX);
}
bool
FullWifiPhyStateHelper::IsStateSwitching (void)
{
  return (GetState () == FullWifiPhy::SWITCHING);
}



Time
FullWifiPhyStateHelper::GetStateDuration (void)
{
  return Simulator::Now () - m_previousStateChangeTime;
}

Time
FullWifiPhyStateHelper::GetDelayUntilIdle (void)
{
  Time retval;

  switch (GetState ())
    {
    case FullWifiPhy::RX:
      retval = m_endRx - Simulator::Now ();
      break;
    case FullWifiPhy::TX:
      retval = m_endTx - Simulator::Now ();
      break;
    case FullWifiPhy::CCA_BUSY:
      retval = m_endCcaBusy - Simulator::Now ();
      break;
    case FullWifiPhy::SWITCHING:
      retval = m_endSwitching - Simulator::Now ();
      break;
    case FullWifiPhy::IDLE:
      retval = Seconds (0);
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      retval = Seconds (0);
      break;
    }
  retval = Max (retval, Seconds (0));
  return retval;
}

Time
FullWifiPhyStateHelper::GetLastRxStartTime (void) const
{
  return m_startRx;
}

enum FullWifiPhy::State
FullWifiPhyStateHelper::GetState (void)
{
  if (m_endTx > Simulator::Now ())
    {
      return FullWifiPhy::TX;
    }
  else if (m_rxing)
    {
      return FullWifiPhy::RX;
    }
  else if (m_endSwitching > Simulator::Now ())
    {
      return FullWifiPhy::SWITCHING;
    }
  else if (m_endCcaBusy > Simulator::Now ())
    {
      return FullWifiPhy::CCA_BUSY;
    }
  else
    {
      return FullWifiPhy::IDLE;
    }
}


void
FullWifiPhyStateHelper::NotifyTxStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyTxStart (duration);
    }
}
void
FullWifiPhyStateHelper::NotifyRxStart (Time duration, Ptr<const Packet> packet, FullWifiMode txMode, FullWifiPreamble preamble)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxStart (duration, packet, txMode, preamble);
    }
}
void
FullWifiPhyStateHelper::NotifyRxEndOk (void)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxEndOk ();
    }
}
void
FullWifiPhyStateHelper::NotifyRxEndError (void)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxEndError ();
    }
}
void
FullWifiPhyStateHelper::NotifyMaybeCcaBusyStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyMaybeCcaBusyStart (duration);
    }
}
void
FullWifiPhyStateHelper::NotifySwitchingStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifySwitchingStart (duration);
    }
}

void
FullWifiPhyStateHelper::LogPreviousIdleAndCcaBusyStates (void)
{
  Time now = Simulator::Now ();
  Time idleStart = Max (m_endCcaBusy, m_endRx);
  idleStart = Max (idleStart, m_endTx);
  idleStart = Max (idleStart, m_endSwitching);
  NS_ASSERT (idleStart <= now);
  if (m_endCcaBusy > m_endRx
      && m_endCcaBusy > m_endSwitching
      && m_endCcaBusy > m_endTx)
    {
      Time ccaBusyStart = Max (m_endTx, m_endRx);
      ccaBusyStart = Max (ccaBusyStart, m_startCcaBusy);
      ccaBusyStart = Max (ccaBusyStart, m_endSwitching);
      m_stateLogger (ccaBusyStart, idleStart - ccaBusyStart, FullWifiPhy::CCA_BUSY);
    }
  m_stateLogger (idleStart, now - idleStart, FullWifiPhy::IDLE);
}

void
FullWifiPhyStateHelper::SwitchToTx (Time txDuration, Ptr<const Packet> packet, FullWifiMode txMode,
                                FullWifiPreamble preamble, uint8_t txPower)
{
  m_txTrace (packet, txMode, preamble, txPower);
  NotifyTxStart (txDuration);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case FullWifiPhy::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_rxing = false;
      m_stateLogger (m_startRx, now - m_startRx, FullWifiPhy::RX);
      m_endRx = now;
      break;
    case FullWifiPhy::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, FullWifiPhy::CCA_BUSY);
      } break;
    case FullWifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case FullWifiPhy::SWITCHING:
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
  m_stateLogger (now, txDuration, FullWifiPhy::TX);
  m_previousStateChangeTime = now;
  m_endTx = now + txDuration;
  m_startTx = now;
}
void
FullWifiPhyStateHelper::SwitchToRx (Time rxDuration, Ptr<const Packet> packet, FullWifiMode txMode, FullWifiPreamble preamble)
{
  NS_ASSERT (IsStateIdle () || IsStateCcaBusy ());
  NS_ASSERT (!m_rxing);
  NotifyRxStart (rxDuration, packet, txMode, preamble);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case FullWifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case FullWifiPhy::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, FullWifiPhy::CCA_BUSY);
      } break;
    case FullWifiPhy::SWITCHING:
    case FullWifiPhy::RX:
    case FullWifiPhy::TX:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
  m_previousStateChangeTime = now;
  m_rxing = true;
  m_startRx = now;
  m_endRx = now + rxDuration;
  NS_ASSERT (IsStateRx ());
}

void
FullWifiPhyStateHelper::SwitchToChannelSwitching (Time switchingDuration)
{
  NotifySwitchingStart (switchingDuration);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case FullWifiPhy::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_rxing = false;
      m_stateLogger (m_startRx, now - m_startRx, FullWifiPhy::RX);
      m_endRx = now;
      break;
    case FullWifiPhy::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, FullWifiPhy::CCA_BUSY);
      } break;
    case FullWifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case FullWifiPhy::TX:
    case FullWifiPhy::SWITCHING:
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }

  if (now < m_endCcaBusy)
    {
      m_endCcaBusy = now;
    }

  m_stateLogger (now, switchingDuration, FullWifiPhy::SWITCHING);
  m_previousStateChangeTime = now;
  m_startSwitching = now;
  m_endSwitching = now + switchingDuration;
  NS_ASSERT (IsStateSwitching ());
}

void
FullWifiPhyStateHelper::SwitchFromRxEndOk (Ptr<Packet> packet, double snr, FullWifiMode mode, enum FullWifiPreamble preamble)
{
  m_rxOkTrace (packet, snr, mode, preamble);
  NotifyRxEndOk ();
  DoSwitchFromRx ();
  if (!m_rxOkCallback.IsNull ())
    {
      m_rxOkCallback (packet, snr, mode, preamble);
    }

}
void
FullWifiPhyStateHelper::SwitchFromRxEndError (Ptr<const Packet> packet, double snr)
{
  m_rxErrorTrace (packet, snr);
  NotifyRxEndError ();
  DoSwitchFromRx ();
  if (!m_rxErrorCallback.IsNull ())
    {
      m_rxErrorCallback (packet, snr);
    }
}

void
FullWifiPhyStateHelper::DoSwitchFromRx (void)
{
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (m_rxing);

  Time now = Simulator::Now ();
  m_stateLogger (m_startRx, now - m_startRx, FullWifiPhy::RX);
  m_previousStateChangeTime = now;
  m_rxing = false;

  NS_ASSERT (IsStateIdle () || IsStateCcaBusy ());
}
void
FullWifiPhyStateHelper::SwitchMaybeToCcaBusy (Time duration)
{
  NotifyMaybeCcaBusyStart (duration);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case FullWifiPhy::SWITCHING:
      break;
    case FullWifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case FullWifiPhy::CCA_BUSY:
      break;
    case FullWifiPhy::RX:
      break;
    case FullWifiPhy::TX:
      break;
    }
  m_startCcaBusy = now;
  m_endCcaBusy = std::max (m_endCcaBusy, now + duration);
}

} // namespace ns3
