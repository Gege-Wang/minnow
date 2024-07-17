#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"
// #include "ethernet_header.hh"


using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.

  // look up for the next_hop ethernet address. first look up in the cache, send it right away.

  if(arp_cache_.contains(next_hop.ipv4_numeric())) {
    // convert dgram to ethernet frame,
    EthernetHeader ethernet_header {
      .dst = arp_cache_[next_hop.ipv4_numeric()],
      .src = ethernet_address_,
      .type = EthernetHeader::TYPE_IPv4
    };

    EthernetFrame ethernet_frame {
      .header = ethernet_header,
      .payload = serialize(dgram)
    };
    transmit(ethernet_frame);
  } else {
    datagrams_to_send_.emplace(next_hop.ipv4_numeric(), dgram);
    if(arp_timer_.contains(next_hop.ipv4_numeric())  && arp_timer_[next_hop.ipv4_numeric()] < 5000) {
      return;
    }
    // if you can not find ethernet address in cache, send a arp request, add the datagram to send_queue, and wait for arp reply;
    ARPMessage arp_request_message {
      .opcode = ARPMessage::OPCODE_REQUEST,
      .sender_ethernet_address = ethernet_address_,
      .sender_ip_address = ip_address_.ipv4_numeric(),
      .target_ip_address = next_hop.ipv4_numeric()
    };
    
    EthernetHeader arp_request_ethernet_header {
      .dst = ETHERNET_BROADCAST,
      .src = ethernet_address_,
      .type = EthernetHeader::TYPE_ARP
    };

    EthernetFrame arp_request_ethernet_frame {
      .header = arp_request_ethernet_header,
      .payload = serialize(arp_request_message)
    };
    transmit(arp_request_ethernet_frame);
    arp_timer_[next_hop.ipv4_numeric()] = 0;

    // wait the arp reply
    // (a) timeout, how to maintain the time
    // (b) how to receive a arp reply

  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  // parse the ethernet frame header, see the type
  EthernetHeader ethernet_header = frame.header;
  
  if((ethernet_header.dst != ethernet_address_) && (ethernet_header.dst != ETHERNET_BROADCAST)) {
    return;
  }
  // if the type is ipv4, we should check if the destination is my, if yes we should parse the inbound frame and push it to receive queue
  if(ethernet_header.type == EthernetHeader::TYPE_IPv4 && ethernet_header.dst == ethernet_address_) {
    InternetDatagram ip_msg;
    bool parse_result = parse(ip_msg, frame.payload);
    if(!parse_result) {
      return;
    }
    datagrams_received_.push(ip_msg);
    // if not, we should discard
  } else if (ethernet_header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp_msg;
    bool parse_result = parse(arp_msg, frame.payload);
    if(!parse_result) {
      return;
    }
    // we should parse the inbound frame arp message, if the target ip address is my or broadcast, we should cache the src ip and src ethernet map for 30 s
    arp_cache_[arp_msg.sender_ip_address] = arp_msg.sender_ethernet_address;
    arp_cache_timer_[arp_msg.sender_ip_address] = 0;
    // we should give the arp_cache_ a timestamp
    if ( datagrams_to_send_.contains( arp_msg.sender_ip_address ) ) {
      auto range = datagrams_to_send_.equal_range( arp_msg.sender_ip_address );
      for ( auto i = range.first; i != range.second; i++ ) {
        EthernetHeader send_ethernet_header {
          .dst = arp_msg.sender_ethernet_address,
          .src = ethernet_address_,
          .type = EthernetHeader::TYPE_IPv4
        };

        EthernetFrame send_ethernet_frame {
          .header = send_ethernet_header,
          .payload = serialize(i -> second)
        };
        transmit(send_ethernet_frame);

      }
      datagrams_to_send_.erase( range.first, range.second );
    }

    // if the type is reply, we don't need to the following thing.
    // if the type is arp and is request, we should check the destination is my
    if(arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == ip_address_.ipv4_numeric()) {
      // then we should send arp reply to src
      ARPMessage arp_reply_message {
        .opcode = ARPMessage::OPCODE_REPLY,
        .sender_ethernet_address = ethernet_address_,
        .sender_ip_address = ip_address_.ipv4_numeric(),
        .target_ethernet_address = arp_msg.sender_ethernet_address,
        .target_ip_address = arp_msg.sender_ip_address
      };
      
      EthernetHeader arp_reply_ethernet_header {
        .dst = ethernet_header.src,
        .src = ethernet_address_,
        .type = EthernetHeader::TYPE_ARP
      };

      EthernetFrame arp_reply_ethernet_frame {
        .header = arp_reply_ethernet_header,
        .payload = serialize(arp_reply_message)
      };
      transmit(arp_reply_ethernet_frame);
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  for(auto it = arp_cache_timer_.begin(); it != arp_cache_timer_.end(); ) {
    it -> second += ms_since_last_tick;
    if(it -> second >= 30000) {
      auto cache_it = arp_cache_.find(it -> first);
      if (cache_it != arp_cache_.end())
        arp_cache_.erase(cache_it);
      it = arp_cache_timer_.erase(it);  // there is a bug, we should not use arp_cache_timer_.erase(it);
    } else {
      it++;
    }
  }

  for(auto it = arp_timer_.begin(); it != arp_timer_.end(); ) {
    it -> second += ms_since_last_tick;
    if(it -> second >= 5000) {
      it = arp_timer_.erase(it);
    } else {
      it++;
    }
  }
}
