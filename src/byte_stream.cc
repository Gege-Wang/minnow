#include "byte_stream.hh"
#include<deque>
#include<vector>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), stream(){}

bool Writer::is_closed() const
{
  // Your code here.
  return close_;
}

void Writer::push( string data )
{
  // Your code here.
  if(close_) {
    return;
  }
  uint64_t writecnt = min(data.size(), capacity_ - bytes_buffered_);
  bytes_pushed_ += writecnt;
  bytes_buffered_ += writecnt;

  for(size_t i = 0; i < writecnt; i++) {
    stream.push_back(data[i]);
  }
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
  return close_ && stream.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  //vector<char> temp_string;
  //temp_string.assign(stream.begin(), stream.end());

  // 从 vector 创建 string_view
  //return std::string_view(temp_string.data(), temp_string.size());
  // 从 string 创建 string_view
  //peek_ = std::string_view(temp);
  //return string_view(stream.begin(), stream.end());
  if(!bytes_buffered_) {
    return {};
  }
  return string_view(&stream.front(), 1);

}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t readcnt = min(len, bytes_buffered_);
  bytes_buffered_ -= readcnt;
  bytes_popped_ += readcnt;
  for(size_t i = 0; i < readcnt; i++) {
    stream.pop_front();
  }
  
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_buffered_;
}
