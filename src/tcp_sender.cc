#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

// Done!
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return sequence_numbers_in_flight_;
}

// Done!
uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions_cnt_;
}

// I misunderstand this function. I think it would call ticks, but it not? we should make new 
// segment in this funtion, not push the segment in outstanding
void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.


  // when window is full, this is not same as window zero. maybe in this case we can return and tick
  // so that we can send something
  if (window_size && window_size <= sequence_numbers_in_flight_) {
    return ;
  }

  // make a TcpSenderMessage
  // note that we should send the SYN and FIN flags and that occupied seqno
  // when the first push, we assume the window_size = 1; 
  if(!has_SYN && window_size == 1) {
    TCPSenderMessage msg = make_empty_message();  
    msg.seqno = Wrap32::wrap(0, isn_);
    msg.SYN  = true;
    has_SYN = true;
    has_FIN = input_.reader().is_finished() && !input_.writer().bytes_pushed();
    msg.FIN = has_FIN;
    transmit(msg);

    outstanding.push(msg);
    // when push msg to outstanding queue, we should make sure to start timer.
    if(!timer.is_running()) {
      timer.start();
    }
    sequence_numbers_in_flight_ += 1;
    last_sent_ = 0 + has_FIN;

    return;
  } else if(window_size == 0) {
    // when window_size = 0, we should send a empty_message so that we can get the newest ackno;
    TCPSenderMessage msg = make_empty_message(); 
    transmit(msg);
    return;
  } else {
    uint64_t size = min(window_size - sequence_numbers_in_flight_, input_.reader().bytes_buffered());
    string sendmsg;
    read(input_.reader(), size, sendmsg);

    while (size) {
      uint64_t len = min(size, TCPConfig::MAX_PAYLOAD_SIZE);
      string payload = sendmsg.substr(0, len);
      // when the bytestream is finish and this is the last message. Set the has_FIN = true;
      if(input_.reader().is_finished() && payload == sendmsg) {
        has_FIN = true;
      }
      TCPSenderMessage msg {
        Wrap32::wrap(last_sent_ + 1, isn_),
        !has_SYN,
        move(payload),
        has_FIN,
        input_.writer().has_error()
      };
      // transmit message, modify the outstanding and record
      transmit(msg);
      outstanding.push(msg);
      // when push msg to outstanding queue, we should make sure to start timer.
      if(!timer.is_running()) {
        timer.start();
      }
      sequence_numbers_in_flight_ += len;
      last_sent_ += len + has_FIN; // there is a bug, you should record the FIN seqno

      // cut the sendmsg
      sendmsg = sendmsg.substr(len);
      size -= len;
    }

  }
  // transmit the message, if the timer is not running, start the timer
  // how to make which is the first Message with SYN?
}

// DONE!
TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  // maybe there should be more simple
  // this is the next seqno should be sent to receiver, and should be rejected because the window size 
  // is zero
  Wrap32 empty_seqno = Wrap32::wrap(last_sent_ + 1, isn_);
  bool SYN = false;
  string payload = {};
  bool FIN = false;
  bool RST = false;
  return TCPSenderMessage {
    .seqno = empty_seqno,
    .SYN = SYN,
    .payload = payload,
    .FIN = FIN,
    .RST = RST
  };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if(msg.RST) {
    //the byte_stream should get an error;
    input_.set_error();
    //_is_end = true;
    return;
  }
  window_size = msg.window_size;
  if(msg.ackno.has_value()) {
    // define the last_seqno
    uint64_t last_seqno = seqno;
    seqno = msg.ackno.value().unwrap(isn_, last_seqno);

    // when the ackno is greater than previous, we should:
    // (a) set RTO to initial
    // (b) if the outstanding is not empty, start the retransmission timer
    // (c) reset the consecutive transmission to 0
    if(seqno > last_seqno) {
      timer.reset_RTO();
      timer.start();
      retransmissions_cnt_ = 0;
    }

    // when all the segment in outstanding are acknowagement, the retransmission timer should be stopped
    if(seqno > last_sent_) {
      // stop the timer
      timer.stop();
      while(!outstanding.empty()) {
        outstanding.pop();
      }
      sequence_numbers_in_flight_ = 0;
      return;
    }


    // we should move the string in outstand according to the ackno
    while(!outstanding.empty()) {
      TCPSenderMessage next = outstanding.front();
      
      if(seqno > next.seqno.unwrap(isn_, last_seqno) + next.sequence_length() - 1) {  // there is a bug if(seqno > next.seqno.unwrap(isn_, last_seqno) + next.sequence_length()) 
        outstanding.pop();
        sequence_numbers_in_flight_ -= next.sequence_length();
      } else {
        break;
      }
    }
  } 
}


// DONE!
void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  timer.tick(ms_since_last_tick);

  // If the timer is expired
  // (a) sent the earliest message in outstanding
  // (b) if the window size is no_zero, record the consecutive retransmission number and double the RTO
  // (c) restart the timer according current RTO
  if(timer.is_expired()) {
    if(!outstanding.empty()) {
      transmit(outstanding.front());
      //outstanding.pop();  // don't pop outstanding unless you receive the ackno
      if(window_size) { // there is a bug not if(!windown_size)
        retransmissions_cnt_++;
        timer.double_RTO();
      }
      timer.start();
    }
  }

}
