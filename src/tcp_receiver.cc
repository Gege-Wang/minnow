#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if(message.RST) {
    reassembler_.reader().set_error();
    return;
  }
  //set the ISN if necessary
  if(message.SYN) {
    has_syn_flag = true;
    zero_point = message.seqno; // there is a bug here being
    uint64_t index = 0;
    reassembler_.insert(index, message.payload, message.FIN);
    return;
  }
  // discard all the messages before the SYN message arrived.
  if(!has_syn_flag) {
    return;
  }
  // record the first_unassembled
  uint64_t abs_seqno = reassembler_.writer().bytes_pushed() + 1;
  // convert the seqno to abs seqno
  uint64_t current_seqno = message.seqno.unwrap(zero_point, abs_seqno);
  // record the stream index
  uint64_t index = current_seqno - 1;

  reassembler_.insert(index, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  std::optional<Wrap32> ackno;
  if(has_syn_flag) {
    ackno = Wrap32::wrap(reassembler_.writer().bytes_pushed() + 1 + reassembler_.writer().is_closed(), zero_point);
  } else {
    ackno = std::nullopt;
  }
  uint64_t pre_window_size = reassembler_.writer().available_capacity();
  uint16_t window_size  = pre_window_size > UINT16_MAX ? UINT64_MAX : pre_window_size;
  return TCPReceiverMessage{
    .ackno = ackno,
    .window_size = window_size,
    .RST = reassembler_.reader().has_error() // there is a bug, because we should check whether the output has error
  };

}