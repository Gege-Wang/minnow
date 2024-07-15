#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <iostream>

class RetransmissionTimer
{
public:
  RetransmissionTimer( uint64_t initial_RTO_ms) : initial_RTO_ms_(initial_RTO_ms), RTO_ms_(initial_RTO_ms), ticks(0)
  {}

  void start() {
    ticks = 0;
    is_running_ = true;
  }

  void stop() {
    ticks = 0;
    is_running_ = false;
    
  }
  void tick(uint64_t ms_since_last_tick) {
    if(!is_running_) {
      std::cout << "timer is not runnint, please start timer first!\n";
    }
    ticks += ms_since_last_tick;
  }

  bool is_running() {
    return is_running_;
  }

  bool is_expired () {
    return ticks > RTO_ms_;
  }

  void double_RTO() {
    RTO_ms_ = RTO_ms_ * 2;
  }

  void reset_RTO() {
    RTO_ms_ = initial_RTO_ms_;
  }
private:
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms_; // the current RTO_ms
  uint64_t ticks; // the ticks passed since the start 
  bool is_running_ {false};

};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), timer(initial_RTO_ms)
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint16_t window_size {1};
  uint64_t seqno {}; // the newest abs ackno;
  uint64_t last_sent_ {};
  uint64_t sequence_numbers_in_flight_ {0}; // maintain the outstanding seq count, because we dont want to calculate it everytime;
  bool has_SYN {false};
  bool has_FIN {false};
  //bool is_end_ {false}; // this flag should be set by byte_stream input_, so I give it up;
  //outstanding queue
  std::queue<TCPSenderMessage> outstanding {};
  uint64_t retransmissions_cnt_ {0};
  RetransmissionTimer timer;
};
