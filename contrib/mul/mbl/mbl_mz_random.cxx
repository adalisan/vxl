// This is mul/mbl/mbl_mz_random.cxx
#ifdef VCL_NEEDS_PRAGMA_INTERFACE
#pragma implementation
#endif
//:
//  \file

#include "mbl_mz_random.h"
#include <vcl_ctime.h>
#include <vcl_cmath.h>
#include <vcl_cassert.h>

unsigned long mbl_mz_random::linear_congruential_lrand32()
{
  return linear_congruential_previous = (linear_congruential_previous*linear_congruential_multiplier + 1)&0xffffffff;
}

//: Construct with seed
mbl_mz_random::mbl_mz_random(unsigned long seed)
  : mz_array_position(0L), mz_borrow(0), mz_previous_normal_flag(0)
{reseed(seed);}

//: Construct with seed
mbl_mz_random::mbl_mz_random(unsigned long seed[mbl_mz_array_size])
  : mz_array_position(0L), mz_borrow(0), mz_previous_normal_flag(0)
{reseed(seed);}

mbl_mz_random::mbl_mz_random(const mbl_mz_random& r)
  : linear_congruential_previous(r.linear_congruential_previous)
  , mz_array_position(r.mz_array_position)
  , mz_borrow(r.mz_borrow)
  , mz_previous_normal_flag(r.mz_previous_normal_flag)
{
  for (int i=0;i<mbl_mz_array_size;++i)
  {
    mz_seed_array[i] = r.mz_seed_array[i];
    mz_array[i] = r.mz_array[i];
  }
}

mbl_mz_random& mbl_mz_random::operator=(const mbl_mz_random& r)
{
  linear_congruential_previous=r.linear_congruential_previous;
  mz_array_position=r.mz_array_position;
  mz_borrow=r.mz_borrow;
  mz_previous_normal_flag=r.mz_previous_normal_flag;
  for (int i=0;i<mbl_mz_array_size;++i)
  {
    mz_seed_array[i] = r.mz_seed_array[i];
    mz_array[i] = r.mz_array[i];
  }
  return *this;
}

mbl_mz_random::mbl_mz_random() : mz_array_position(0), mz_borrow(0), mz_previous_normal_flag(0)
{
  reseed();
}

mbl_mz_random::~mbl_mz_random()
{
  for (int i=0;i<mbl_mz_array_size;++i)
  {
    mz_seed_array[i] = 0;
    mz_array[i] = 0;
  }
}

void mbl_mz_random::reseed()
{
  reseed((unsigned long)vcl_time(NULL));
}

void mbl_mz_random::reseed(unsigned long seed)
{
  mz_array_position = 0L;
  mz_borrow = 0L;

  linear_congruential_previous = seed;
  // Use the lc generator to fill the array
  for (int i=0;i<mbl_mz_array_size;++i)
  {
    mz_seed_array[i] = linear_congruential_lrand32();
    mz_array[i] = mz_seed_array[i];
  }

  // Warm up with 1000 randoms
  for (int j=0;j<1000;j++) lrand32();
}

void mbl_mz_random::reseed(unsigned long seed[mbl_mz_array_size])
{
  mz_array_position = 0L;
  mz_borrow = 0L;

  for (int i=0;i<mbl_mz_array_size;++i)
  {
    mz_array[i] = seed[i];
    mz_seed_array[i] = seed[i];
  }
}

void mbl_mz_random::restart()
{
  mz_array_position = 0L;

  for (int i=0;i<mbl_mz_array_size;++i)
  {
    mz_array[i] = mz_seed_array[i];
  }
}

double mbl_mz_random::normal()
{
  if (mz_previous_normal_flag)
  {
    mz_previous_normal_flag = 0;
    return mz_previous_normal;
  }
  else
  {
    double x,y,r2;
    do
    {
      x = drand32(-1.0,1.0);
      y = drand32(-1.0,1.0);
      r2 = x*x+y*y;
    }
    while (r2 >=1.0 || r2 == 0.0);
    double fac = vcl_sqrt(-2.0*vcl_log(r2)/r2);
    mz_previous_normal = x*fac;
    mz_previous_normal_flag = 1;
    return y*fac;
  }
}


//: Random value from a unit normal distribution about zero
// Uses a drand64() as its underlying generator.
// Because the function uses a probability transform, the randomness (and quantisation) is non-linearly dependant on the
// value. The further the sample is from zero, the lower the number of bits to which it is random.
double mbl_mz_random::normal64()
{
  if (mz_previous_normal_flag)
  {
    mz_previous_normal_flag = 0;
    return mz_previous_normal;
  }
  else
  {
    double x,y,r2;
    do
    {
      x = drand64(-1.0,1.0);
      y = drand64(-1.0,1.0);
      r2 = x*x+y*y;
    }
    while (r2 >=1.0 || r2 == 0.0);
    double fac = vcl_sqrt(-2.0*vcl_log(r2)/r2);
    mz_previous_normal = x*fac;
    mz_previous_normal_flag = 1;
    return y*fac;
  }
}

unsigned long mbl_mz_random::lrand32()
{
  unsigned long p1 = mz_array[(mbl_mz_array_size + mz_array_position - mz_previous1)%mbl_mz_array_size];
  unsigned long p2 = (p1 - mz_array[mz_array_position] - mz_borrow)&0xffffffff;
  if (p2 < p1) mz_borrow = 0;
  if (p2 > p1) mz_borrow = 1;
  mz_array[mz_array_position] = p2;
  mz_array_position = (++mz_array_position)%mbl_mz_array_size;
  return p2;
}

int mbl_mz_random::lrand32(int lower, int upper)
{
  assert(lower <= upper);

  // Note: we have to reject some numbers otherwise we get a very slight bias
  // towards the lower part of the range lower - upper. See below

  unsigned long range = upper-lower+1;
  unsigned long denom = 0xffffffff/range;
  unsigned long ran;
  while ((ran=lrand32()) >= denom*range) ;
  return lower + int(ran/denom);
}


int mbl_mz_random::lrand32(int lower, int upper, int &count)
{
  assert(lower <= upper);

  // Note: we have to reject some numbers otherwise we get a very slight bias
  // towards the lower part of the range lower - upper. Hence this is a "count"
  // version of the above function that returns the number of lrand32()
  // calls made.

  unsigned long range = upper-lower+1;
  unsigned long denom = 0xffffffff/range;
  unsigned long ran;
  count = 1;
  while ((ran=lrand32())>=denom*range) ++count;
  return lower + int(ran/denom);
}

double mbl_mz_random::drand32(double lower, double upper)
{
  assert(lower <= upper);
  return  (double(lrand32())/0xffffffff)*(upper-lower) + lower;
}

double mbl_mz_random::drand64(double lower, double upper)
{
  assert(lower <= upper);
  return  (double(lrand32())/0xffffffff + double(lrand32())/(double(0xffffffff)*double(0xffffffff)))*(upper-lower) + lower;
}
