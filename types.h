#ifndef TYPESH
#define TYPESH

#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint8_t byte;

typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;


typedef struct xorshift32_state {
    u32 a;
}xorshift32_state ;

/* The state word must be initialized to non-zero */
u32 xorshift32(xorshift32_state *state)
{
    /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
    u32 x = state->a;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state->a = x;
}

#endif // TYPESH
