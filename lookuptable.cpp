#include "lookuptable.h"

#define TABLEV1

extern uint8_t* table;

#ifdef TABLEV0
uint4x16_t table_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint8_t* a = (uint8_t*)&_a;
    uint8_t* b = (uint8_t*)&_b;
    uint64_t _r;
    uint8_t* r = (uint8_t*)&_r;
    r[0] = table[a[0] * 256 + b[0]];
    r[1] = table[a[1] * 256 + b[1]];
    r[2] = table[a[2] * 256 + b[2]];
    r[3] = table[a[3] * 256 + b[3]];
    r[4] = table[a[4] * 256 + b[4]];
    r[5] = table[a[5] * 256 + b[5]];
    r[6] = table[a[6] * 256 + b[6]];
    r[7] = table[a[7] * 256 + b[7]];
    return _r;
}
#endif

#ifdef TABLEV1
uint4x16_t table_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, r;
    r = (uint64_t)table[((a & 0xFFull) << 8) | (b & 0xFFull)];
    r |= (uint64_t)table[(((a & 0xFF00ull) << 8) | (b & 0xFF00ull)) >> 8] << 8;
    r |= (uint64_t)table[(((a & 0xFF0000ull) << 8) | (b & 0xFF0000ull)) >> 16] << 16;
    r |= (uint64_t)table[(((a & 0xFF000000ull) << 8) | (b & 0xFF000000ull)) >> 24] << 24;
    r |= (uint64_t)table[(((a & 0xFF00000000ull) << 8) | (b & 0xFF00000000ull)) >> 32] << 32;
    r |= (uint64_t)table[(((a & 0xFF0000000000ull) << 8) | (b & 0xFF0000000000ull)) >> 40] << 40;
    r |= (uint64_t)table[(((a & 0xFF000000000000ull) << 8) | (b & 0xFF000000000000ull)) >> 48] << 48;
    r |= (uint64_t)table[(((a & 0xFF00000000000000ull)) | ((b & 0xFF00000000000000ull) >> 8)) >> 48] << 56;
    return r;
}
#endif

#ifdef TABLEV2
uint4x16_t table_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, r;
    r = (uint64_t)table[((a & 0xFull) << 4) | (b & 0xFull)];
    r |= (uint64_t)table[(a & 0xF0ull) | ((b & 0xF0ull) >> 4)] << 4;
    r |= (uint64_t)table[((a & 0xF00ull) >> 4) | ((b & 0xF00ull) >> 8)] << 8;
    r |= (uint64_t)table[((a & 0xF000ull) >> 8) | ((b & 0xF000ull) >> 12)] << 12;
    r |= (uint64_t)table[((a & 0xF0000ull) >> 12) | ((b & 0xF0000ull) >> 16)] << 16;
    r |= (uint64_t)table[((a & 0xF00000ull) >> 16) | ((b & 0xF00000ull) >> 20)] << 20;
    r |= (uint64_t)table[((a & 0xF000000ull) >> 20) | ((b & 0xF000000ull) >> 24)] << 24;
    r |= (uint64_t)table[((a & 0xF0000000ull) >> 24) | ((b & 0xF0000000ull) >> 28)] << 28;
    r |= (uint64_t)table[((a & 0xF00000000ull) >> 28) | ((b & 0xF00000000ull) >> 32)] << 32;
    r |= (uint64_t)table[((a & 0xF000000000ull) >> 32) | ((b & 0xF000000000ull) >> 36)] << 36;
    r |= (uint64_t)table[((a & 0xF0000000000ull) >> 36) | ((b & 0xF0000000000ull) >> 40)] << 40;
    r |= (uint64_t)table[((a & 0xF00000000000ull) >> 40) | ((b & 0xF00000000000ull) >> 44)] << 44;
    r |= (uint64_t)table[((a & 0xF000000000000ull) >> 44) | ((b & 0xF000000000000ull) >> 48)] << 48;
    r |= (uint64_t)table[((a & 0xF0000000000000ull) >> 48) | ((b & 0xF0000000000000ull) >> 52)] << 52;
    r |= (uint64_t)table[((a & 0xF00000000000000ull) >> 52) | ((b & 0xF00000000000000ull) >> 56)] << 56;
    r |= (uint64_t)table[((a & 0xF000000000000000ull) >> 56) | ((b & 0xF000000000000000ull) >> 60)] << 60;
    return r;
}
#endif