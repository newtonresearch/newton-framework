//
// Copyright Voipster 2006. Use, modification and distribution
// is subject to the Voipster Software License, Version
// 1.0. (See accompanying file VOIPSTER_LICENSE_1_0)
//



// gsm0610iq.cpp
//
//

#include <confp.hpp>
#include "audio_codec.hpp"
#include "audio_gsm0610.hpp"
#include "gsm.h"
#include <iostream>
#include <map>
#include <string>
#include <cstdlib>

namespace {

static const char* Impl1Message =
  "gsm0610Impl1 " VOIP_BUILD_REVISION " compiled " __DATE__ " at " __TIME__ " with " _COMPILER;

class Impl1 : public audio::codec::Interface {
public :
  Impl1();
  ~Impl1();

  enum { ENCODE_SRC_SIZE = GSM_SAMPLES_PER_FRAME };
  enum { ENCODE_DST_SIZE = GSM_ENCODED_FRAME_SIZE };

  virtual size_t samples() const { return ENCODE_SRC_SIZE; }
  virtual size_t size() const { return ENCODE_DST_SIZE; }

//  virtual size_t decode_src_size() const { return ENCODE_DST_SIZE; }
//  virtual size_t decode_dst_size() const { return ENCODE_SRC_SIZE; }
  virtual const unsigned char* encode(short const** first, short const* last);
  virtual void decode(unsigned char const* first, 
                      unsigned char const* last, float* out);
  virtual const char* describe()
  {
    return Impl1Message;
  }

private:
  gsm0610_encode encoder;
  std::size_t encode_src_idx_;
  short encode_src_[ENCODE_SRC_SIZE];
  unsigned char encode_dst_[ENCODE_DST_SIZE];

  gsm0610_decode decoder;
};

Impl1::Impl1()
  : encode_src_idx_(0)
{
}

Impl1::~Impl1()
{
}

const unsigned char* Impl1::encode(short const** first, short const* last)
{
  // fill buffer
  std::size_t size = std::min(ENCODE_SRC_SIZE - encode_src_idx_,
                              static_cast<size_t>(std::distance(*first, last)));
  std::copy(*first, *first + size, encode_src_ + encode_src_idx_);
  *first += size;
  encode_src_idx_ += size;

  unsigned char* dst = 0;
  if(encode_src_idx_ == ENCODE_SRC_SIZE)
  {
    dst = encode_dst_;
    encoder.encode(encode_src_, encode_src_ + ENCODE_SRC_SIZE, dst);
    encode_src_idx_ = 0;
  }

  return dst;
}

void Impl1::decode(unsigned char const* first, 
                   unsigned char const* last, float* out)
{
  decoder.decode(first, last, out);
}

} // of the anonymous namespace

////////////////////////////////////////////////////////////////////////

extern audio::codec::Interface* CreateImpl1()
{
  return new Impl1;
}

// End Of File

