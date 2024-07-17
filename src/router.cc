#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const std::optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  route_table_.emplace_back(route_prefix, prefix_length, next_hop, interface_num);

}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  //first you should have a datagram, so you can get the dst ip address
  for (auto& it: _interfaces) {
    //std::queue<InternetDatagram> datagram_buffer = it -> datagrams_received();  // there is a bug, because the app will call a funtion many time, so the received datagram queue will not change,
    auto& datagram_buffer = it -> datagrams_received();
    while(!datagram_buffer.empty()) {
      InternetDatagram datagram = datagram_buffer.front();

      // go through the route_table and find the longest prefix and get the interface
      std::optional<Router::route_table_entry> optional_matched_route= longest_prefix_match(datagram.header.dst);
      // if not match, you should drop the datagram
      // decrement the TTL, if the TTL is zero or zero after decrement, you should drop the datagram
      if(!datagram.header.ttl || !(datagram.header.ttl - 1) || !optional_matched_route.has_value()) {
        datagram_buffer.pop();
        continue;
      } 
      Router::route_table_entry matched_route = optional_matched_route.value();
      datagram.header.ttl --;
      datagram.header.compute_checksum();
      Address next_hop = matched_route.next_hop.has_value() ? matched_route.next_hop.value() : Address::from_ipv4_numeric(datagram.header.dst);
    // then you can transmit to link-layer
      interface(matched_route.interface_num) -> send_datagram(datagram, next_hop);
      datagram_buffer.pop();
      
    }

  }
}


std::optional<Router::route_table_entry> Router::longest_prefix_match (uint32_t ip_address) {

  // go through the route_table and compare the prefix and find the max and maintain the interface
  int32_t final_prefix_length = INT32_MIN;
  Router::route_table_entry final_route_table_entry;
  for(auto it: route_table_) {
    uint8_t it_prefix_length = it.prefix_length;
    if (it_prefix_length == 0) {
      if(it_prefix_length > final_prefix_length) {
        final_prefix_length = it_prefix_length;
        final_route_table_entry = it;
      }
    } else {
      uint8_t len = 32U - it_prefix_length;
      uint32_t ip_num = ip_address >> len;
      uint32_t it_num = it.route_prefix >> len;
      if(it_num == ip_num) {
        if(it_prefix_length >= final_prefix_length) {
          final_prefix_length = it_prefix_length;
          final_route_table_entry = it;
        }
      }
    }

  }
  return final_prefix_length == INT32_MIN ? std::nullopt : std::optional<Router::route_table_entry>(final_route_table_entry);

}
