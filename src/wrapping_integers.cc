#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32(zero_point + static_cast<uint32_t>(n));
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // uint64_t U32_VALUE = 1UL << 32;
  // //Wrap32 check = wrap(checkpoint, zero_point);
  // uint64_t offset = raw_value_ - zero_point.raw_value_;
  // if ( checkpoint > offset) {
  //   uint64_t value = (checkpoint - offset) + (U32_VALUE >> 1);
  //   uint64_t num = value / U32_VALUE;
  //   return U32_VALUE * num + offset;
  // } else {
  //   return offset;
  // }

    uint32_t offset = raw_value_ - wrap(checkpoint, zero_point).raw_value_;
    uint64_t res = checkpoint + offset;
    
    if (offset > (0x80000000) && res >= (0x100000000)) {
        res -= (0x100000000);
    }

    return res;
}
