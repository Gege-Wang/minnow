#include "byte_stream.hh"
#include <deque>
#include <string_view>
#include <iostream>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_view_(), stream() {}

bool Writer::is_closed() const
{
  // Your code here.
  return close_;
}

void Writer::push( string data )
{
  // Your code here.
  if ( close_ ) {
    return;
  }
  if(data.size() == 0 || capacity_ - bytes_buffered_ == 0) {
    return;
  }
  uint64_t writecnt = min( data.size(), capacity_ - bytes_buffered_ );
  bytes_pushed_ += writecnt;
  bytes_buffered_ += writecnt;
  string s = data.substr(0, writecnt);
  //stream.insert( stream.end(), data.begin(), data.begin() + writecnt ); //for deque
  stream.push_back(move(s));
  buffer_view_.emplace_back(stream.back().c_str(), writecnt);
  return;
}

void Writer::close()
{
  // Your code here.
  close_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - bytes_buffered_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return close_ && buffer_view_.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  // vector<char> temp_string;
  // temp_string.assign(stream.begin(), stream.end());

  // 从 vector 创建 string_view
  // return std::string_view(temp_string.data(), temp_string.size());
  // 从 string 创建 string_view
  // peek_ = std::string_view(temp);
  // return string_view(stream.begin(), stream.end());
  if ( buffer_view_.empty()) {
    return {};
  }
  //std::cout<<"string_view"<<string_view_<<std::endl;
  return buffer_view_.front();
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t readcnt = min( len, bytes_buffered_ );
  bytes_buffered_ -= readcnt;
  bytes_popped_ += readcnt;
  // stream.erase(stream.begin(), stream.begin() + readcnt); //for deque
  while(readcnt > 0) {
    string_view s = buffer_view_.front();
    if(readcnt >= s.size()) {
      buffer_view_.pop_front();
      readcnt -= s.size();
    } else {
      buffer_view_.front().remove_prefix(readcnt);
      break;
    }
  }
  //auto start = stream.begin(); // for list
  //auto end = std::next( start, readcnt ); //for list
  //stream.erase( start, end ); //for list
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_buffered_;
}
