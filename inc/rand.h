// TODO: This file needs a serious audit

#include <cstdint>

constexpr uint64_t half_max = UINT64_MAX / 2;

// From https://en.wikipedia.org/wiki/Xorshift
inline void xorshift64(uint64_t &state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
}

// returns a random value between -1 and 1. modifies seed
inline float norm_rand(uint64_t &state) {
  xorshift64(state);
  return (state - half_max) / half_max;
}
