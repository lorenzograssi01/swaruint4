#include <cstdint>
#include <iostream>

#ifndef UINT4X16_T
#define UINT4X16_T

class uint4_t
{
    uint8_t* addr;
    bool upper;
    uint4_t(uint8_t* _addr, bool _upper)
    {
        upper = _upper;
        addr = _addr;
    }
public:
    uint4_t& operator = (uint8_t b)
    {
        if(upper)
        {
            *addr = (*addr & 0x0F) | (b << 4);
            return *this;
        }
        *addr = (*addr & 0xF0) | (b & 0x0F);
        return *this;
    }
    operator uint8_t() const
    {
        if(upper)
            return (*addr) >> 4;
        return (*addr) & 0x0F;
    }
    friend std::ostream& operator << (std::ostream& os, const uint4_t& n)
    {
        os << +uint8_t(n);
        return os;
    }
    friend class uint4x16_t;
};

class uint4x16_t
{
public:
    uint64_t reg;
    uint4x16_t(uint64_t _reg)
    {
        reg = _reg;
    }
    uint4x16_t() {}
    uint4_t operator [] (int i)
    {
        return uint4_t(((uint8_t*)&reg) + (i / 2), i % 2);
    }
};
    
uint4x16_t vadd_u4(uint4x16_t v1, uint4x16_t v2);
uint4x16_t vqadd_u4(uint4x16_t v1, uint4x16_t v2);
uint4x16_t vsub_u4(uint4x16_t v1, uint4x16_t v2);
uint4x16_t vqsub_u4(uint4x16_t v1, uint4x16_t v2);
uint4x16_t vmul_u4(uint4x16_t v1, uint4x16_t v2);
uint4x16_t vqmul_u4(uint4x16_t v1, uint4x16_t v2);
uint4x16_t vmla_lane_u4(uint4x16_t v1, uint4x16_t v2, uint4x16_t v3, int lane);
uint4x16_t vqmla_lane_u4(uint4x16_t v1, uint4x16_t v2, uint4x16_t v3, int lane);
uint16_t vdot_u16(uint4x16_t v1, uint4x16_t v2);
void vmm_u4(const uint4x16_t* matrix0, const uint4x16_t* matrix1, uint4x16_t* result, int rows, int inner, int columns);
void vqmm_u4(const uint4x16_t* matrix0, const uint4x16_t* matrix1, uint4x16_t* result, int rows, int inner, int columns);

#endif