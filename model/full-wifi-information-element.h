/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Dean Armstrong
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
 * Authors: Dean Armstrong <deanarm@gmail.com>
 */

#ifndef FULL_WIFI_INFORMATION_ELEMENT_H
#define FULL_WIFI_INFORMATION_ELEMENT_H

#include "ns3/header.h"
#include "ns3/simple-ref-count.h"

namespace ns3 {

/**
 * This type is used to represent an Information Element ID. An
 * enumeration would be tidier, but doesn't provide for the
 * inheritance that is currently preferable to cleanly support
 * pre-standard modules such as mesh. Maybe there is a nice way of
 * doing this with a class.
 *
 * Until such time as a better way of implementing this is dreamt up
 * and applied, developers will need to be careful to avoid
 * duplication of IE IDs in the defines below (and in files which
 * declare "subclasses" of WifiInformationElement). Sorry.
 */
typedef uint8_t FullWifiInformationElementId;


#ifndef IEID
#define IEID
/**
 * Here we have definition of all Information Element IDs in IEEE
 * 802.11-2007. See the comments for WifiInformationElementId - this could
 * probably be done in a considerably tidier manner.
 */
#define IE_SSID                                ((FullWifiInformationElementId)0)
#define IE_SUPPORTED_RATES                     ((FullWifiInformationElementId)1)
#define IE_FH_PARAMETER_SET                    ((FullWifiInformationElementId)2)
#define IE_DS_PARAMETER_SET                    ((FullWifiInformationElementId)3)
#define IE_CF_PARAMETER_SET                    ((FullWifiInformationElementId)4)
#define IE_TIM                                 ((FullWifiInformationElementId)5)
#define IE_IBSS_PARAMETER_SET                  ((FullWifiInformationElementId)6)
#define IE_COUNTRY                             ((FullWifiInformationElementId)7)
#define IE_HOPPING_PATTERN_PARAMETERS          ((FullWifiInformationElementId)8)
#define IE_HOPPING_PATTERN_TABLE               ((FullWifiInformationElementId)9)
#define IE_REQUEST                             ((FullWifiInformationElementId)10)
#define IE_BSS_LOAD                            ((FullWifiInformationElementId)11)
#define IE_EDCA_PARAMETER_SET                  ((FullWifiInformationElementId)12)
#define IE_TSPEC                               ((FullWifiInformationElementId)13)
#define IE_TCLAS                               ((FullWifiInformationElementId)14)
#define IE_SCHEDULE                            ((FullWifiInformationElementId)15)
#define IE_CHALLENGE_TEXT                      ((FullWifiInformationElementId)16)
// 17 to 31 are reserved in 802.11-2007
#define IE_POWER_CONSTRAINT                    ((FullWifiInformationElementId)32)
#define IE_POWER_CAPABILITY                    ((FullWifiInformationElementId)33)
#define IE_TPC_REQUEST                         ((FullWifiInformationElementId)34)
#define IE_TPC_REPORT                          ((FullWifiInformationElementId)35)
#define IE_SUPPORTED_CHANNELS                  ((FullWifiInformationElementId)36)
#define IE_CHANNEL_SWITCH_ANNOUNCEMENT         ((FullWifiInformationElementId)37)
#define IE_MEASUREMENT_REQUEST                 ((FullWifiInformationElementId)38)
#define IE_MEASUREMENT_REPORT                  ((FullWifiInformationElementId)39)
#define IE_QUIET                               ((FullWifiInformationElementId)40)
#define IE_IBSS_DFS                            ((FullWifiInformationElementId)41)
#define IE_ERP_INFORMATION                     ((FullWifiInformationElementId)42)
#define IE_TS_DELAY                            ((FullWifiInformationElementId)43)
#define IE_TCLAS_PROCESSING                    ((FullWifiInformationElementId)44)
// 45 is reserved in 802.11-2007
#define IE_QOS_CAPABILITY                      ((FullWifiInformationElementId)46)
// 47 is reserved in 802.11-2007
#define IE_RSN                                 ((FullWifiInformationElementId)48)
// 49 is reserved in 802.11-2007
#define IE_EXTENDED_SUPPORTED_RATES            ((FullWifiInformationElementId)50)
// 51 to 126 are reserved in 802.11-2007
#define IE_EXTENDED_CAPABILITIES               ((FullWifiInformationElementId)127)
// 128 to 220 are reserved in 802.11-2007
#define IE_VENDOR_SPECIFIC                     ((FullWifiInformationElementId)221)
// 222 to 255 are reserved in 802.11-2007
#endif /* IEID */

/**
 * \brief Information element, as defined in 802.11-2007 standard
 * \ingroup wifi
 *
 * The IEEE 802.11 standard includes the notion of Information
 * Elements, which are encodings of management information to be
 * communicated between STAs in the payload of various frames of type
 * Management. Information Elements (IEs) have a common format, each
 * starting with a single octet - the Element ID, which indicates the
 * specific type of IE (a type to represent the options here is
 * defined as WifiInformationElementId). The next octet is a length field and
 * encodes the number of octets in the third and final field, which is
 * the IE Information field.
 *
 * The class ns3::WifiInformationElement provides a base for classes
 * which represent specific Information Elements. This class defines
 * pure virtual methods for serialisation
 * (ns3::WifiInformationElement::SerializeInformationField) and
 * deserialisation
 * (ns3::WifiInformationElement::DeserializeInformationField) of IEs, from
 * or to data members or other objects that simulation objects use to
 * maintain the relevant state.
 *
 * This class also provides an implementation of the equality
 * operator, which operates by comparing the serialised versions of
 * the two WifiInformationElement objects concerned.
 *
 * Elements are defined to have a common general format consisting of
 * a 1 octet Element ID field, a 1 octet length field, and a
 * variable-length element-specific information field. Each element is
 * assigned a unique Element ID as defined in this standard. The
 * Length field specifies the number of octets in the Information
 * field.
 *
 * This class is pure virtual and acts as base for classes which know
 * how to serialize specific IEs.
 */
class FullWifiInformationElement : public SimpleRefCount<FullWifiInformationElement>
{
public:
  virtual ~FullWifiInformationElement ();
  /// Serialize entire IE including Element ID and length fields
  Buffer::Iterator Serialize (Buffer::Iterator i) const;
  /// Deserialize entire IE, which must be present. The iterator
  /// passed in must be pointing at the Element ID (i.e., the very
  /// first octet) of the correct type of information element,
  /// otherwise this method will generate a fatal error.
  Buffer::Iterator Deserialize (Buffer::Iterator i);
  /// Deserialize entire IE if it is present. The iterator passed in
  /// must be pointing at the Element ID of an information element. If
  /// the Element ID is not the one that the given class is interested
  /// in then it will return the same iterator.
  Buffer::Iterator DeserializeIfPresent (Buffer::Iterator i);
  /// Get the size of the serialized IE including Element ID and
  /// length fields.
  uint16_t GetSerializedSize () const;

  ///\name Each subclass must implement
  //\{
  /// Own unique Element ID
  virtual FullWifiInformationElementId ElementId () const = 0;
  /// Length of serialized information (i.e., the length of the body
  /// of the IE, not including the Element ID and length octets. This
  /// is the value that will appear in the second octet of the entire
  /// IE - the length field)
  virtual uint8_t GetInformationFieldSize () const = 0;
  /// Serialize information (i.e., the body of the IE, not including
  /// the Element ID and length octets)
  virtual void SerializeInformationField (Buffer::Iterator start) const = 0;
  /// Deserialize information (i.e., the body of the IE, not including
  /// the Element ID and length octets)
  virtual uint8_t DeserializeInformationField (Buffer::Iterator start,
                                               uint8_t length) = 0;
  //\}

  /// In addition, a subclass may optionally override the following...
  //\{
  /// Generate human-readable form of IE
  virtual void Print (std::ostream &os) const;
  /// Compare information elements using Element ID
  virtual bool operator< (FullWifiInformationElement const & a) const;
  /// Compare two IEs for equality by ID & Length, and then through
  /// memcmp of serialised version
  virtual bool operator== (FullWifiInformationElement const & a) const;
  //\}
};

}
#endif /* FULL_WIFI_INFORMATION_ELEMENT_H */
