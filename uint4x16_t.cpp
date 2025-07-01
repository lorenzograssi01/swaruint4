#include "uint4x16_t.h"

#define ADDV4
#define QADDV4
#define SUBV4
#define QSUBV4
#define MULV3
#define QMULV1
#define MLALANEV0
#define QMLALANEV0
#define DOTV0
#define MMV1
#define QMMV1

#ifdef ADDV0
uint4x16_t vadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg;
    uint64_t s_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F)) & 0x0F0F0F0F0F0F0F0F;
    uint64_t s_high = ((a & 0xF0F0F0F0F0F0F0F0) + (b & 0xF0F0F0F0F0F0F0F0)) & 0xF0F0F0F0F0F0F0F0;
    return s_low | s_high;
}
#endif

#ifdef ADDV1
uint4x16_t vadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t r, a = _a.reg, b = _b.reg, x;
    r = a + b;
    x = (a ^ b ^ r) & 0x1111111111111111;
    while(x != 0)
    {
        a = r;
        r -= x;
        x = (x ^ a ^ r) & 0x1111111111111111;
        a = r;
        r += x;
        x = (x ^ a ^ r) & 0x1111111111111111;
    }
    return r;
}
#endif

#ifdef ADDV2
uint4x16_t vadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a, r = _a.reg, x = _b.reg;
    while(x != 0)
    {
        a = r;
        r += x;
        x = (a ^ r ^ x) & 0x1111111111111111;
        if(x == 0)
            break;
        a = r;
        r -= x;
        x = (a ^ r ^ x) & 0x1111111111111111;
    }
    return r;
}
#endif

#ifdef ADDV3
uint4x16_t vadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t r, a = _a.reg, b = _b.reg, x, y;
    r = a + b;
    x = (a ^ b ^ r) & 0x1111111111111111;
    y = (r<<1) | (r<<2) | (r<<3) | (r<<4) | ~(x<<4);
    x = x & y;
    return r - x;
}
#endif

#ifdef ADDV4
uint4x16_t vadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg;
    uint64_t l = (a & 0x7777777777777777) + (b & 0x7777777777777777);
    uint64_t z = (a ^ b) & 0x8888888888888888;
    return l ^ z;
}
#endif

#ifdef QADDV0
uint4x16_t vqadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t r, a = _a.reg, b = _b.reg, x;
    r = a + b;
    x = (a ^ b ^ r) & 0x1111111111111111;
    uint64_t f = (x >> 4) * 15;
    if(r < a)
        f |= (0xFull << 60);
    while(x != 0)
    {
        a = r;
        r -= x;
        x = (x ^ a ^ r) & 0x1111111111111111;
        if(x == 0)
            break;
        a = r;
        r += x;
        x = (x ^ a ^ r) & 0x1111111111111111;
    }
    r |= f;
    return r;
}
#endif

#ifdef QADDV1
uint4x16_t vqadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, s, r;
    uint64_t l = (a & 0x7777777777777777) + (b & 0x7777777777777777);
    uint64_t z = (a ^ b) & 0x8888888888888888;
    s = l ^ z;
    r = a + b;
    uint64_t x = (a ^ b ^ r) & 0x1111111111111111;
    uint64_t f = (x >> 4) * 15;
    if(r < a)
        f |= (0xFull << 60);
    return s | f;
}
#endif

#ifdef QADDV2
uint4x16_t vqadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg;
    uint64_t r_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F));
    uint64_t r_high = (((a >> 4) & 0x0F0F0F0F0F0F0F0F) + ((b >> 4) & 0x0F0F0F0F0F0F0F0F));
    uint64_t f = ((r_high & 0xF0F0F0F0F0F0F0F0) | ((r_low & 0xF0F0F0F0F0F0F0F0) >> 4)) * 15;
    return ((r_high & 0x0F0F0F0F0F0F0F0F) << 4)| (r_low & 0x0F0F0F0F0F0F0F0F) | f;
}
#endif

#ifdef QADDV3
uint4x16_t vqadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t result;
    __asm__ (
        "movabsq $8608480567731124087, %%rdx\n\t"
        "movq    %%rdi, %%rax\n\t"
        "movq    %%rdi, %%rcx\n\t"
        "xorq    %%r9, %%r9\n\t"
        "addq    %%rsi, %%rdi\n\t"
        "adcq    %%r9, %%r9\n\t"
        "salq    $60, %%r9\n\t"
        "andq    %%rdx, %%rax\n\t"
        "andq    %%rsi, %%rdx\n\t"
        "xorq    %%rsi, %%rcx\n\t"
        "addq    %%rdx, %%rax\n\t"
        "xorq    %%rcx, %%rdi\n\t"
        "movabsq $-8608480567731124088, %%rdx\n\t"
        "andq    %%rcx, %%rdx\n\t"
        "shrq    $4, %%rdi\n\t"
        "xorq    %%rdx, %%rax\n\t"
        "movabsq $76861433640456465, %%rdx\n\t"
        "andq    %%rdx, %%rdi\n\t"
        "orq     %%r9, %%rdi\n\t"
        "movq    %%rdi, %%rdx\n\t"
        "salq    $4, %%rdx\n\t"
        "subq    %%rdi, %%rdx\n\t"
        "orq     %%rdx, %%rax\n\t"
        : "=a"(result)
        : "D"(_a), "S"(_b)
    );
    return result;
}
#endif

#ifdef QADDV4
uint4x16_t vqadd_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg;
    uint64_t l = (a & 0x7777777777777777) + (b & 0x7777777777777777);
    uint64_t z = (a ^ b) & 0x8888888888888888;
    uint64_t y = ((a & b) | (a & l) | (l & b)) & 0x8888888888888888;
    uint64_t f = (y >> 3) * 15;
    return (l ^ z) | f;
}
#endif

#ifdef SUBV0
uint4x16_t vsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a, r = _a.reg, x = _b.reg;
    while(x != 0)
    {
        a = r;
        r -= x;
        x = (a ^ r ^ x) & 0x1111111111111111;
        if(x == 0)
            break;
        a = r;
        r += x;
        x = (a ^ r ^ x) & 0x1111111111111111;
    }
    return r;
}
#endif

#ifdef SUBV1
uint4x16_t vsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t r, a = _a.reg, b = _b.reg, x;
    r = a - b;
    x = (a ^ b ^ r) & 0x1111111111111111;
    uint64_t s_low = ((r & 0x0F0F0F0F0F0F0F0F) + (x & 0x0F0F0F0F0F0F0F0F)) & 0x0F0F0F0F0F0F0F0F;
    uint64_t s_high = ((r & 0xF0F0F0F0F0F0F0F0) + (x & 0xF0F0F0F0F0F0F0F0)) & 0xF0F0F0F0F0F0F0F0;
    return s_low | s_high;
}
#endif

#ifdef SUBV2
uint4x16_t vsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t r, a = _a.reg, b = _b.reg, x;
    r = a - b;
    x = (a ^ b ^ r) & 0x1111111111111111;
    while(x != 0)
    {
        a = r;
        r += x;
        x = (x ^ a ^ r) & 0x1111111111111111;
        a = r;
        r -= x;
        x = (x ^ a ^ r) & 0x1111111111111111;
    }
    return r;
}
#endif

#ifdef SUBV3
uint4x16_t vsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg;
    uint64_t d_low = ((a | 0xF0F0F0F0F0F0F0F0) - (b & 0x0F0F0F0F0F0F0F0F)) & 0x0F0F0F0F0F0F0F0F;
    uint64_t d_high = ((a | 0x0F0F0F0F0F0F0F0F) - (b & 0xF0F0F0F0F0F0F0F0)) & 0xF0F0F0F0F0F0F0F0;
    return d_low | d_high;
}
#endif

#ifdef SUBV4
uint4x16_t vsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg;
    uint64_t l = (a | 0x8888888888888888) - (b & 0x7777777777777777);
    uint64_t z = ~(a ^ b) & 0x8888888888888888;
    return l ^ z;
}
#endif

#ifdef QSUBV0
uint4x16_t vqsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t r, a = _a.reg, b = _b.reg, x;
    r = a - b;
    x = (a ^ b ^ r) & 0x1111111111111111;
    uint64_t f = (x >> 4) * 15;
    if(r > a)
        f |= (0xFull << 60);
    while(x != 0)
    {
        a = r;
        r += x;
        x = (x ^ a ^ r) & 0x1111111111111111;
        if(x == 0)
            break;
        a = r;
        r -= x;
        x = (x ^ a ^ r) & 0x1111111111111111;
    }
    r &= ~f;
    return r;
}
#endif

#ifdef QSUBV1
uint4x16_t vqsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, r, d;
    uint64_t l = (a | 0x8888888888888888) - (b & 0x7777777777777777);
    uint64_t z = ~(a ^ b) & 0x8888888888888888;
    d = l ^ z;
    r = a - b;
    uint64_t x = (a ^ b ^ r) & 0x1111111111111111;
    uint64_t f = (x >> 4) * 15;
    if(r > a)
        f |= (0xFull << 60);
    return d & ~f;
}
#endif

#ifdef QSUBV2
uint4x16_t vqsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg;
    uint64_t r_low = ((a | 0xF0F0F0F0F0F0F0F0) - (b & 0x0F0F0F0F0F0F0F0F));
    uint64_t r_high = (((a >> 4) | 0xF0F0F0F0F0F0F0F0) - ((b >> 4) & 0x0F0F0F0F0F0F0F0F));
    uint64_t o = ~((r_high | 0x0F0F0F0F0F0F0F0F) & ((r_low >> 4) | 0xF0F0F0F0F0F0F0F0));
    uint64_t f = o * 15;
    return (((r_high & 0x0F0F0F0F0F0F0F0F) << 4) | (r_low & 0x0F0F0F0F0F0F0F0F)) & ~f;
}
#endif

#ifdef QSUBV3
uint4x16_t vqsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t result;
    __asm__ (
        "movabsq $76861433640456465, %%rax\n\t"
        "movq    %%rdi, %%rcx\n\t"
        "movq    %%rdi, %%rdx\n\t"
        "movabsq $8608480567731124087, %%r8\n\t"
        "xorq    %%rsi, %%rcx\n\t"
        "xorq    %%r9, %%r9\n\t"
        "subq    %%rsi, %%rdx\n\t"
        "adcq    %%r9, %%r9\n\t"
        "salq    $60, %%r9\n\t"
        "andq    %%r8, %%rsi\n\t"
        "xorq    %%rcx, %%rdx\n\t"
        "notq    %%rcx\n\t"
        "shrq    $4, %%rdx\n\t"
        "andq    %%rax, %%rdx\n\t"
        "orq     %%r9, %%rdx\n\t"
        "movq    %%rdx, %%rax\n\t"
        "salq    $4, %%rax\n\t"
        "subq    %%rdx, %%rax\n\t"
        "movabsq $-8608480567731124088, %%rdx\n\t"
        "orq     %%rdx, %%rdi\n\t"
        "andq    %%rdx, %%rcx\n\t"
        "notq    %%rax\n\t"
        "subq    %%rsi, %%rdi\n\t"
        "xorq    %%rcx, %%rdi\n\t"
        "andq    %%rdi, %%rax\n\t"
        : "=a"(result)
        : "D"(_a), "S"(_b)
    );
    return result;
}
#endif

#ifdef QSUBV4
uint4x16_t vqsub_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg;
    uint64_t l = (a | 0x8888888888888888) - (b & 0x7777777777777777);
    uint64_t z = ~(a ^ b) & 0x8888888888888888;
    uint64_t y = ((~(a) & (b | ~l)) | (a & b & ~l)) & 0x8888888888888888;
    uint64_t f = (y >> 3) * 15;
    return (l ^ z) & (~f);
}
#endif

#ifdef MULV0
uint4x16_t vmul_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, p_low = 0, p_high = 0, k;
    for (int i = 0; i < 4; i++)
    {
        k = a & (((b >> i) & 0x1111111111111111) * 15);
        p_low += (k & 0x0F0F0F0F0F0F0F0F) << i;
        p_high += (k & 0xF0F0F0F0F0F0F0F0) << i;
    }
    return (p_low & 0x0F0F0F0F0F0F0F0F) | (p_high & 0xF0F0F0F0F0F0F0F0);
}
#endif

#ifdef MULV1
uint4x16_t vmul_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, p_low, p_high, k;

    k = a & ((b & 0x1111111111111111) * 15);
    p_low = k & 0x0F0F0F0F0F0F0F0F;
    p_high = k & 0xF0F0F0F0F0F0F0F0;

    k = a & (((b >> 1) & 0x1111111111111111) * 15);
    p_low += (k & 0x0F0F0F0F0F0F0F0F) << 1;
    p_high += (k & 0xF0F0F0F0F0F0F0F0) << 1;

    k = a & (((b >> 2) & 0x1111111111111111) * 15);
    p_low += (k & 0x0F0F0F0F0F0F0F0F) << 2;
    p_high += (k & 0xF0F0F0F0F0F0F0F0) << 2;

    k = a & (((b >> 3) & 0x1111111111111111) * 15);
    p_low += (k & 0x0F0F0F0F0F0F0F0F) << 3;
    p_high += (k & 0xF0F0F0F0F0F0F0F0) << 3;

    return (p_low & 0x0F0F0F0F0F0F0F0F) | (p_high & 0xF0F0F0F0F0F0F0F0);
}
#endif

#ifdef MULV2
uint4x16_t vmul_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, p_low, p_high, k;

    k = a & ((b & 0x1111111111111111) * 15);
    p_low = k & 0x0F0F0F0F0F0F0F0F;
    p_high = k & 0xF0F0F0F0F0F0F0F0;

    a <<= 1;
    k = a & ((b & 0x2222222222222222) * 7);
    p_low += k & 0x0F0F0F0F0F0F0F0F;
    p_high += k & 0xF0F0F0F0F0F0F0F0;

    a <<= 1;
    k = a & ((b & 0x4444444444444444) * 3);
    p_low += k & 0x0F0F0F0F0F0F0F0F;
    p_high += k & 0xF0F0F0F0F0F0F0F0;

    a <<= 1;
    k = a & (b & 0x8888888888888888);
    p_low += k & 0x0F0F0F0F0F0F0F0F;
    p_high += k & 0xF0F0F0F0F0F0F0F0;

    return (p_low & 0x0F0F0F0F0F0F0F0F) | (p_high & 0xF0F0F0F0F0F0F0F0);
}
#endif

#ifdef MULV3
uint4x16_t vmul_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, p, k, l, z;

    p = a & ((b & 0x1111111111111111) * 15);

    a <<= 1;
    k = a & ((b & 0x2222222222222222) * 7);
    l = (k & 0x7777777777777777) + (p & 0x7777777777777777);
    z = (k ^ p) & 0x8888888888888888;
    p = l ^ z;

    a <<= 1;
    k = a & ((b & 0x4444444444444444) * 3);
    l = (k & 0x7777777777777777) + (p & 0x7777777777777777);
    z = (k ^ p) & 0x8888888888888888;
    p = l ^ z;

    a <<= 1;
    k = a & (b & 0x8888888888888888);
    p = p ^ k;

    return p;
}
#endif

#ifdef QMULV0
uint4x16_t vqmul_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, p_low, p_high, k, o, f;

    k = a & ((b & 0x1111111111111111) * 15);
    p_low = k & 0x0F0F0F0F0F0F0F0F;
    p_high = (k >> 4) & 0x0F0F0F0F0F0F0F0F;

    k = a & (((b >> 1) & 0x1111111111111111) * 15);
    p_low += (k & 0x0F0F0F0F0F0F0F0F) << 1;
    p_high += ((k >> 4) & 0x0F0F0F0F0F0F0F0F) << 1;

    k = a & (((b >> 2) & 0x1111111111111111) * 15);
    p_low += (k & 0x0F0F0F0F0F0F0F0F) << 2;
    p_high += ((k >> 4) & 0x0F0F0F0F0F0F0F0F) << 2;

    k = a & (((b >> 3) & 0x1111111111111111) * 15);
    p_low += (k & 0x0F0F0F0F0F0F0F0F) << 3;
    p_high += ((k >> 4) & 0x0F0F0F0F0F0F0F0F) << 3;

    o = (p_high & 0xF0F0F0F0F0F0F0F0) | ((p_low & 0xF0F0F0F0F0F0F0F0) >> 4);
    o |= (o >> 1);
    o |= (o >> 2);
    o &= 0x1111111111111111;
    f = o * 15;

    return ((p_low & 0x0F0F0F0F0F0F0F0F) | ((p_high & 0x0F0F0F0F0F0F0F0F) << 4)) | f;
}
#endif

#ifdef QMULV1
uint4x16_t vqmul_u4(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, p_low, p_high, o, f, a_low, a_high, m;
    a_low = a & 0x0F0F0F0F0F0F0F0F;
    a_high = (a >> 4) & 0x0F0F0F0F0F0F0F0F;

    m = (b & 0x1111111111111111) * 15;
    p_low = a_low & m;
    p_high = a_high & (m >> 4);

    m = ((b >> 1) & 0x1111111111111111) * 15;
    p_low += (a_low & m) << 1;
    p_high += (a_high & (m >> 4)) << 1;

    m = ((b >> 2) & 0x1111111111111111) * 15;
    p_low += (a_low & m) << 2;
    p_high += (a_high & (m >> 4)) << 2;

    m = ((b >> 3) & 0x1111111111111111) * 15;
    p_low += (a_low & m) << 3;
    p_high += (a_high & (m >> 4)) << 3;

    o = (p_high & 0xF0F0F0F0F0F0F0F0) | ((p_low & 0xF0F0F0F0F0F0F0F0) >> 4);
    o |= (o >> 1);
    o |= (o >> 2);
    o &= 0x1111111111111111;
    f = o * 15;

    return ((p_low & 0x0F0F0F0F0F0F0F0F) | ((p_high & 0x0F0F0F0F0F0F0F0F) << 4)) | f;
}
#endif

#ifdef MLALANEV0
uint4x16_t vmla_lane_u4(uint4x16_t _a, uint4x16_t _b, uint4x16_t _c, int lane)
{
    uint64_t a = _a.reg, b = _b.reg, c_lane = (_c.reg >> (lane << 2)) & 0xFull;
    uint64_t m_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F) * c_lane) & 0x0F0F0F0F0F0F0F0F;
    uint64_t m_high = ((a & 0xF0F0F0F0F0F0F0F0) + (b & 0xF0F0F0F0F0F0F0F0) * c_lane) & 0xF0F0F0F0F0F0F0F0;
    return m_low | m_high;
}
#endif

#ifdef QMLALANEV0
uint4x16_t vqmla_lane_u4(uint4x16_t _a, uint4x16_t _b, uint4x16_t _c, int lane)
{
    uint64_t a = _a.reg, b = _b.reg, c_lane = (_c.reg >> (lane << 2)) & 0xFull, f;
    uint64_t r_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F) * c_lane);
    uint64_t r_high = (((a >> 4) & 0x0F0F0F0F0F0F0F0F) + ((b >> 4) & 0x0F0F0F0F0F0F0F0F) * c_lane);
    uint64_t o = ((r_high) & 0xF0F0F0F0F0F0F0F0) | (((r_low) & 0xF0F0F0F0F0F0F0F0) >> 4);
    o |= (o >> 1);
    o |= (o >> 2);
    o &= 0x1111111111111111;
    f = o * 15;
    return ((r_high & 0x0F0F0F0F0F0F0F0F) << 4) | (r_low & 0x0F0F0F0F0F0F0F0F) | f;
}
#endif

#ifdef DOTV0
uint16_t vdot_u16(uint4x16_t _a, uint4x16_t _b)
{
    uint64_t a = _a.reg, b = _b.reg, p_low, p_high, o, f, a_low, a_high, m, dot;
    a_low = a & 0x0F0F0F0F0F0F0F0F;
    a_high = (a >> 4) & 0x0F0F0F0F0F0F0F0F;

    m = (b & 0x1111111111111111) * 15;
    p_low = a_low & m;
    p_high = a_high & (m >> 4);

    m = ((b >> 1) & 0x1111111111111111) * 15;
    p_low += (a_low & m) << 1;
    p_high += (a_high & (m >> 4)) << 1;

    m = ((b >> 2) & 0x1111111111111111) * 15;
    p_low += (a_low & m) << 2;
    p_high += (a_high & (m >> 4)) << 2;

    m = ((b >> 3) & 0x1111111111111111) * 15;
    p_low += (a_low & m) << 3;
    p_high += (a_high & (m >> 4)) << 3;

    dot = (p_low & 0x00FF00FF00FF00FF) + (p_high & 0x00FF00FF00FF00FF) + ((p_low & 0xFF00FF00FF00FF00) >> 8) + ((p_high & 0xFF00FF00FF00FF00) >> 8);
    dot = dot + (dot >> 16);
    dot = dot + (dot >> 32);

    return (uint16_t)dot;
}
#endif

#ifdef MMV0
void vmm_u4(const uint4x16_t* m0, const uint4x16_t* m1, uint4x16_t* mr, int rows, int inner, int columns)
{
    int parts = columns >> 4;
    int parts_input = inner >> 4;
    for(int r = 0; r < rows; r++)
    {
        for(int p = 0; p < parts; p++)
        {
            mr[r * parts + p] = 0;
            for(int i = 0; i < inner; i++)
            {
                mr[r * parts + p] = vmla_lane_u4(mr[r * parts + p], m1[i * parts + p], m0[r * parts_input + (i >> 4)], i & 15);
            }
        }
    }
}
#endif

#ifdef MMV1
void vmm_u4(const uint4x16_t* m0, const uint4x16_t* m1, uint4x16_t* mr, int rows, int inner, int columns)
{
    int parts = columns >> 4;
    int parts_input = inner >> 4;
    for(int r = 0; r < rows; r++)
    {
        for(int p = 0; p < parts; p++)
        {
            mr[r * parts + p] = 0;
            for(int i = 0; i < inner; i++)
            {
                uint64_t a = mr[r * parts + p].reg, b = m1[i * parts + p].reg, c_lane = (m0[r * parts_input + (i >> 4)].reg >> ((i & 15) << 2)) & 0xFull;
                uint64_t m_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F) * c_lane) & 0x0F0F0F0F0F0F0F0F;
                uint64_t m_high = ((a & 0xF0F0F0F0F0F0F0F0) + (b & 0xF0F0F0F0F0F0F0F0) * c_lane) & 0xF0F0F0F0F0F0F0F0;
                mr[r * parts + p] = m_low | m_high;
            }
        }
    }
}
#endif

#ifdef QMMV0
void vqmm_u4(const uint4x16_t* m0, const uint4x16_t* m1, uint4x16_t* mr, int rows, int inner, int columns)
{
    int parts = columns >> 4;
    int parts_input = inner >> 4;
    for(int r = 0; r < rows; r++)
    {
        for(int p = 0; p < parts; p++)
        {
            mr[r * parts + p] = 0;
            for(int i = 0; i < inner; i++)
            {
                mr[r * parts + p] = vqmla_lane_u4(mr[r * parts + p], m1[i * parts + p], m0[r * parts_input + (i >> 4)], i & 15);
            }
        }
    }
}
#endif

#ifdef QMMV1
void vqmm_u4(const uint4x16_t* m0, const uint4x16_t* m1, uint4x16_t* mr, int rows, int inner, int columns)
{
    int parts = columns >> 4;
    int parts_input = inner >> 4;
    for(int r = 0; r < rows; r++)
    {
        for(int p = 0; p < parts; p++)
        {
            mr[r * parts + p] = 0;
            for(int i = 0; i < inner; i++)
            {
                uint64_t a = mr[r * parts + p].reg, b = m1[i * parts + p].reg, c_lane = (m0[r * parts_input + (i >> 4)].reg >> ((i & 15) << 2)) & 0xFull, f;
                uint64_t r_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F) * c_lane);
                uint64_t r_high = (((a >> 4) & 0x0F0F0F0F0F0F0F0F) + ((b >> 4) & 0x0F0F0F0F0F0F0F0F) * c_lane);
                uint64_t o = ((r_high) & 0xF0F0F0F0F0F0F0F0) | (((r_low) & 0xF0F0F0F0F0F0F0F0) >> 4);
                o |= (o >> 1);
                o |= (o >> 2);
                o &= 0x1111111111111111;
                f = o * 15;
                mr[r * parts + p] = ((r_high & 0x0F0F0F0F0F0F0F0F) << 4) | (r_low & 0x0F0F0F0F0F0F0F0F) | f;
            }
        }
    }
}
#endif
