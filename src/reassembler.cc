#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  if( data.size() == 0 ) {

    if((end_ && is_last_substring )|| (is_last_substring && !first_index)) { // handle the SYN + FIN + empty string;
      output_.writer().close(); // there is a bug because if we transmit a empty data with FIN, but we dont receive previous data, we should not close the bytestream
    }
    return;
  }
  if ( is_last_substring ) {
    last_bytes_index_ = first_index + data.size() - 1;
  }
  // if first_index > first_unassembled, we stored it in buffer_. we should modify the first_unassembled
  // when we write to output.
  uint64_t first_unacceptable_ = first_unassembled_ + output_.writer().available_capacity();
  if (first_index + data.size() - 1 < first_unassembled_ || first_index >= first_unacceptable_) {
    return;
  }
  for ( size_t i = 0; i < data.size(); i++ ) {
    if ( first_index + i >= first_unassembled_ && first_index + i < first_unacceptable_ ) {
      if(!buffer_[first_index + i])
        buffer_[first_index + i] = data[i];
    }
  }
  // else we discard it.

  // now we should check if we can write sth. to output
  uint64_t output_available_ = output_.writer().available_capacity();
  string data_to_write;
  auto it = buffer_.begin();
  while ( it != buffer_.end() && it->first == first_unassembled_ ) {
    if(output_available_ <= 0) {
      break;
    }
    if ( it->first == last_bytes_index_ ) {
      end_ = true;
    }
    data_to_write.push_back( it->second );
    output_available_--;
    it = buffer_.erase( it );
    first_unassembled_++;
  }
  output_.writer().push( std::move(data_to_write) );
  if ( end_  && buffer_.empty()) {
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return buffer_.size();
}
