#include <iostream>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include "uint4x16_t.h"
#include "lookuptable.h"
#define MAINV1
#define MAINTESTSPEED

#define MANIPULATE_FIXED_VALUE_NO
#define TABLE8BIT
//#define NO_FUNCTIONS

uint8_t* table;

using namespace std;
using namespace std::chrono;

void pin_to_core(int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if(sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0)
    {
        perror("sched_setaffinity");
    }
}

void quicksort(uint64_t* arr, int n)
{
    if(n <= 1)
        return;
    int i = 0, j = n - 1;
    int pivot = arr[n/2];
    while(i <= j)
    {
        while(arr[i] < pivot)
            i++;
        while(arr[j] > pivot)
            j--;
        if(i >= j)
            break;
        int t = arr[j];
        arr[j] = arr[i];
        arr[i] = t;
        i++;
        j--;
    }
    quicksort(arr, i);
    if(i == j)
        i++;
    quicksort(arr + i, n - i);
}


#if defined MAINV0 || defined MAINV1
typedef uint4x16_t (*OpFunc)(uint4x16_t, uint4x16_t);
typedef uint8_t (*ComputeFunc)(uint8_t, uint8_t);
enum OperationType
{
    ADD, QADD, SUB, QSUB, MUL, QMUL
};
#elif defined MAINV2 || defined MAINV3 || defined MAINV5
typedef uint4x16_t (*OpFunc)(uint4x16_t, uint4x16_t, uint4x16_t, int);
typedef uint8_t (*ComputeFunc)(uint8_t, uint8_t, uint8_t);
enum OperationType
{
    MLALANE, QMLALANE
};
#elif defined MAINV4 || defined MAINV6
typedef uint16_t (*OpFunc)(uint4x16_t, uint4x16_t);
typedef uint16_t (*ComputeFunc)(uint4x16_t, uint4x16_t);
enum OperationType
{
    DOT
};
#elif defined MAINV7
typedef void (*OpFunc)(const uint4x16_t*, const uint4x16_t*, uint4x16_t*, int, int, int);
typedef uint8_t (*ComputeFunc)(uint8_t, uint8_t, uint8_t);
enum OperationType
{
    MM, QMM
};
#endif

uint64_t my_random()
{
    return ((((((((uint64_t)(rand() & 0x7FFF) << 15) | (uint64_t)(rand()) & 0x7FFF) << 15) | (uint64_t)(rand()) & 0x7FFF) << 15) | (uint64_t)(rand()) & 0x7FFF) << 15) | (uint64_t)(rand() & 0x7FFF);
}

uint8_t vadd_u4_compute(uint8_t a, uint8_t b)
{
    return (a + b) % 16;
}

uint8_t vqadd_u4_compute(uint8_t a, uint8_t b)
{
    return (a + b) < 16 ? a + b : 15;
}

uint8_t vsub_u4_compute(uint8_t a, uint8_t b)
{
    return (a >= b) ? a - b : 16 + a - b;
}

uint8_t vqsub_u4_compute(uint8_t a, uint8_t b)
{
    return (a >= b) ? a - b : 0;
}

uint8_t vmul_u4_compute(uint8_t a, uint8_t b)
{
    return (a * b) % 16;
}

uint8_t vqmul_u4_compute(uint8_t a, uint8_t b)
{
    return (a * b) < 16 ? a * b : 15;
}

#if defined MAINV0 || defined MAINV1
int auto_test_values(uint4x16_t a, uint4x16_t b, uint4x16_t r, ComputeFunc compute)
{
    for(int i = 0; i < 16; i++)
    {
        if(compute(a[i], b[i]) != uint8_t(r[i]))
            return i;
    }
    return -1;
}

int auto_test_correctness(OpFunc op, ComputeFunc compute)
{
    uint4x16_t a, b, r;
    int c = 0;
    for (int i = 0; i < 100000; i++)
    {
        a = my_random();
        b = my_random();
        r = op(a, b);
        int j = auto_test_values(a, b, r, compute);
        if (j >= 0)
        {
            for (int i = 15; i >= 0; i--)
                cout << a[i] << "\t";
            cout << endl;
            for (int i = 15; i >= 0; i--)
                cout << b[i] << "\t";
            cout << endl;
            for (int i = 15; i >= 0; i--)
                cout << r[i] << "\t";
            cout << endl
                 << "ERROR (" << j << ")" << endl;
            c++;
        }
    }
    return c;
}

int64_t auto_test_speed(OpFunc op, bool quick = false)
{
    int n_tests = 30000;
    int n_data = 10000;
    if(quick)
    {
        n_data /= 5;
        n_tests /= 2;
    }
    uint4x16_t a, b, r;
    auto start = high_resolution_clock::now();
    for (int j = 0; j < n_data; j++)
    {
        a = my_random();
        b = my_random();
        for (int i = 0; i < n_tests; i++)
        {
            r = op(a, b);
        }
    }
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count();
}

#ifdef TABLE8BIT
void build_table(ComputeFunc compute)
{
    table = new uint8_t[256 * 256];
    for(int i = 0; i < 256; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            table[i * 256 + j] = (compute(i & 0x0F, j & 0x0F)) | ((compute(i >> 4, j >> 4)) << 4);
        }
    }
}
#endif

#ifdef TABLE4BIT
void build_table(ComputeFunc compute)
{
    table = new uint8_t[256];
    for(int i = 0; i < 16; i++)
    {
        for(int j = 0; j < 16; j++)
        {
            table[i * 16 + j] = compute(i, j);
        }
    }
}
#endif

int64_t auto_test(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    switch(operation)
    {
        case ADD:
            op = vadd_u4;
            compute = vadd_u4_compute;
            name = "vadd_u4";
            break;
        case QADD:
            op = vqadd_u4;
            compute = vqadd_u4_compute;
            name = "vqadd_u4";
            break;
        case SUB:
            op = vsub_u4;
            compute = vsub_u4_compute;
            name = "vsub_u4";
            break;
        case QSUB:
            op = vqsub_u4;
            compute = vqsub_u4_compute;
            name = "vqsub_u4";
            break;
        case MUL:
            op = vmul_u4;
            compute = vmul_u4_compute;
            name = "vmul_u4";
            break;
        case QMUL:
            op = vqmul_u4;
            compute = vqmul_u4_compute;
            name = "vqmul_u4";
            break;
    }
    build_table(compute);
    int c = auto_test_correctness(table_u4, compute);
    if (c)
    {
        cout << name << " TABLE NOT WORKING (" << c << " errors)" << endl;
        return -1;
    }
    c = auto_test_correctness(op, compute);
    if (c)
    {
        cout << name << " ALGORITHM NOT WORKING (" << c << " errors)" << endl;
        return -1;
    }
    cout << name << " OK, ";
    int64_t t = auto_test_speed(op);
    cout << (t / 300000.) << " ns";
    t = auto_test_speed(table_u4, true);
    cout << " (table: " << (t / 30000.) << " ns)" << endl;
    return t;
}
#endif

#ifdef MAINV0
int main()
{
    srand(time(NULL));
    auto_test(ADD);
    auto_test(QADD);
    auto_test(SUB);
    auto_test(QSUB);
    auto_test(MUL);
    auto_test(QMUL);
}
#endif

#if defined MAINV1
#ifdef MANIPULATE_FIXED_VALUE
uint64_t manipulate_data(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    uint64_t operand_vector = 0x2222222222222222;
    uint8_t operand_byte = 0x22;
    uint8_t operand = 1;
    switch(operation)
    {
        case ADD:
            op = vadd_u4;
            compute = vadd_u4_compute;
            name = "vadd_u4";
            break;
        case QADD:
            op = vqadd_u4;
            compute = vqadd_u4_compute;
            name = "vqadd_u4";
            break;
        case SUB:
            op = vsub_u4;
            compute = vsub_u4_compute;
            name = "vsub_u4";
            break;
        case QSUB:
            op = vqsub_u4;
            compute = vqsub_u4_compute;
            name = "vqsub_u4";
            break;
        case MUL:
            op = vmul_u4;
            compute = vmul_u4_compute;
            name = "vmul_u4";
            break;
        case QMUL:
            op = vqmul_u4;
            compute = vqmul_u4_compute;
            name = "vqmul_u4";
            break;
    }
    void* data0 = new uint64_t[1 << 22];
    void* data1 = new uint64_t[1 << 22];
    void* data2 = new uint64_t[1 << 22];
    void* control_data = new uint64_t[1 << 22];
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint64_t*)data0)[i] = my_random();
        ((uint64_t*)data1)[i] = ((uint64_t*)data0)[i];
        ((uint64_t*)data2)[i] = ((uint64_t*)data0)[i];
        ((uint64_t*)control_data)[i] = ((uint64_t*)data0)[i];
    }
    build_table(compute);
    cout << name << " \t";

    auto start0 = high_resolution_clock::now();
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint4x16_t*)data0)[i] = op(((uint4x16_t*)data0)[i], 0x2222222222222222);
    }
    auto end0 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end0 - start0).count() << " us (vectorized)\t ";

    auto start1 = high_resolution_clock::now();
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint4x16_t*)data1)[i] = table_u4(((uint4x16_t*)data1)[i], 0x2222222222222222);
    }
    auto end1 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end1 - start1).count() << " us (table)\t";

    auto start2 = high_resolution_clock::now();
    switch(operation)
    {
        case ADD:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)data2)[i] + 2) & 0x0F) | (((((uint8_t*)data2)[i]) + 0x20) & 0xF0);
            }
            break;
        case QADD:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)data2)[i] & 0x0F) < 0x0E ? (((uint8_t*)data2)[i] & 0x0F) + 2 : 0x0F) | ((((uint8_t*)data2)[i] & 0xF0) < 0xE0 ? (((uint8_t*)data2)[i] & 0xF0) + 0x20 : 0xF0);
            }
            break;
        case SUB:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)data2)[i] - 2) & 0x0F) | (((((uint8_t*)data2)[i]) - 0x20) & 0xF0);
            }
            break;
        case QSUB:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)data2)[i] & 0x0F) >= 2 ? (((uint8_t*)data2)[i] & 0x0F) - 2 : 0) | ((((uint8_t*)data2)[i] & 0xF0) >= 0x20 ? (((uint8_t*)data2)[i] & 0xF0) - 0x20 : 0);
            }
            break;
        case MUL:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)data2)[i] * 2) & 0x0F) | ((((((uint8_t*)data2)[i]) & 0xF0) * (2)) & 0xF0);
            }
            break;
        case QMUL:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)data2)[i] & 0x0F) <= 0x07 ? (((uint8_t*)data2)[i] & 0x0F) * 2 : 0x0F) | ((((uint8_t*)data2)[i] & 0xF0) <= 0x70 ? (((uint8_t*)data2)[i] & 0xF0) * 2 : 0xF0);
            }
            break;
    }
    auto end2 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end2 - start2).count() << " us (reference)\t" << endl;

    for(int i = 0; i < (1 << 22); i++)
    {
        uint4x16_t control = *((uint4x16_t*)(control_data) + i);
        uint4x16_t d0 = *((uint4x16_t*)(data0) + i);
        uint4x16_t d1 = *((uint4x16_t*)(data1) + i);
        uint4x16_t d2 = *((uint4x16_t*)(data2) + i);
        for(int i = 0; i < 16; i++)
        {
            if(d0[i] != compute(control[i], 2))
            {
                cout << "ERROR D0" << endl;
                return -1;
            }
                
            if(d1[i] != compute(control[i], 2))
            {
                cout << "ERROR D1" << endl;
                return -1;
            }
                
            if(d2[i] != compute(control[i], 2))
            {
                cout << "ERROR D2" << endl;
                return -1;
            }
        }
    }
    delete[] (uint64_t*)data0;
    delete[] (uint64_t*)data1;
    delete[] (uint64_t*)data2;
    delete[] (uint64_t*)control_data;
    return duration_cast<microseconds>(end0 - start0).count();
}

#elif defined NO_FUNCTIONS
uint64_t manipulate_data(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    switch(operation)
    {
        case ADD:
            op = vadd_u4;
            compute = vadd_u4_compute;
            name = "vadd_u4";
            break;
        case QADD:
            op = vqadd_u4;
            compute = vqadd_u4_compute;
            name = "vqadd_u4";
            break;
        case SUB:
            op = vsub_u4;
            compute = vsub_u4_compute;
            name = "vsub_u4";
            break;
        case QSUB:
            op = vqsub_u4;
            compute = vqsub_u4_compute;
            name = "vqsub_u4";
            break;
        case MUL:
            op = vmul_u4;
            compute = vmul_u4_compute;
            name = "vmul_u4";
            break;
        case QMUL:
            op = vqmul_u4;
            compute = vqmul_u4_compute;
            name = "vqmul_u4";
            break;
    }
    void* i1 = new uint64_t[1 << 22];
    void* i2 = new uint64_t[1 << 22];
    void* data0 = new uint64_t[1 << 22];
    void* data1 = new uint64_t[1 << 22];
    void* data2 = new uint64_t[1 << 22];
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint64_t*)i1)[i] = my_random();
        ((uint64_t*)i2)[i] = my_random();
        ((uint64_t*)data0)[i] = 0;
        ((uint64_t*)data1)[i] = 0;
        ((uint64_t*)data2)[i] = 0;
    }
    build_table(compute);
    cout << name << " \t";

    auto start0 = high_resolution_clock::now();
    switch(operation)
    {
        case ADD:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i];
                uint64_t l = (a & 0x7777777777777777) + (b & 0x7777777777777777);
                uint64_t z = (a ^ b) & 0x8888888888888888;
                ((uint64_t*)data0)[i] = l ^ z;
            }
            break;
        case QADD:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i];
                uint64_t r_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F));
                uint64_t r_high = (((a >> 4) & 0x0F0F0F0F0F0F0F0F) + ((b >> 4) & 0x0F0F0F0F0F0F0F0F));
                uint64_t f = ((r_high & 0xF0F0F0F0F0F0F0F0) | ((r_low & 0xF0F0F0F0F0F0F0F0) >> 4)) * 15;
                ((uint64_t*)data0)[i] = ((r_high & 0x0F0F0F0F0F0F0F0F) << 4)| (r_low & 0x0F0F0F0F0F0F0F0F) | f;
            }
            break;
        case SUB:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i];
                uint64_t l = (a | 0x8888888888888888) - (b & 0x7777777777777777);
                uint64_t z = ~(a ^ b) & 0x8888888888888888;
                ((uint64_t*)data0)[i] = l ^ z;
            }
            break;
        case QSUB:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i];
                uint64_t r_low = ((a | 0xF0F0F0F0F0F0F0F0) - (b & 0x0F0F0F0F0F0F0F0F));
                uint64_t r_high = (((a >> 4) | 0xF0F0F0F0F0F0F0F0) - ((b >> 4) & 0x0F0F0F0F0F0F0F0F));
                uint64_t o = ~((r_high | 0x0F0F0F0F0F0F0F0F) & ((r_low >> 4) | 0xF0F0F0F0F0F0F0F0));
                uint64_t f = o * 15;
                ((uint64_t*)data0)[i] = (((r_high & 0x0F0F0F0F0F0F0F0F) << 4) | (r_low & 0x0F0F0F0F0F0F0F0F)) & ~f;
            }
            break;
        case MUL:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i], p, k, l, z;

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

                ((uint64_t*)data0)[i] = p;
            }
            break;
        case QMUL:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i], p_low, p_high, o, f, a_low, a_high, m;
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

                ((uint64_t*)data0)[i] = ((p_low & 0x0F0F0F0F0F0F0F0F) | ((p_high & 0x0F0F0F0F0F0F0F0F) << 4)) | f;
            }
            break;
    }
    auto end0 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end0 - start0).count() << " us (vectorized)\t ";

    auto start1 = high_resolution_clock::now();
    for(int i = 0; i < (1 << 22); i++)
    {   
        ((uint64_t*)data1)[i]  = (uint64_t)table[((((uint64_t*)i1)[i] & 0xFFull) << 8) | (((uint64_t*)i2)[i] & 0xFFull)];
        ((uint64_t*)data1)[i] |= (uint64_t)table[(((((uint64_t*)i1)[i] & 0xFF00ull) << 8) | (((uint64_t*)i2)[i] & 0xFF00ull)) >> 8] << 8;
        ((uint64_t*)data1)[i] |= (uint64_t)table[(((((uint64_t*)i1)[i] & 0xFF0000ull) << 8) | (((uint64_t*)i2)[i] & 0xFF0000ull)) >> 16] << 16;
        ((uint64_t*)data1)[i] |= (uint64_t)table[(((((uint64_t*)i1)[i] & 0xFF000000ull) << 8) | (((uint64_t*)i2)[i] & 0xFF000000ull)) >> 24] << 24;
        ((uint64_t*)data1)[i] |= (uint64_t)table[(((((uint64_t*)i1)[i] & 0xFF00000000ull) << 8) | (((uint64_t*)i2)[i] & 0xFF00000000ull)) >> 32] << 32;
        ((uint64_t*)data1)[i] |= (uint64_t)table[(((((uint64_t*)i1)[i] & 0xFF0000000000ull) << 8) | (((uint64_t*)i2)[i] & 0xFF0000000000ull)) >> 40] << 40;
        ((uint64_t*)data1)[i] |= (uint64_t)table[(((((uint64_t*)i1)[i] & 0xFF000000000000ull) << 8) | (((uint64_t*)i2)[i] & 0xFF000000000000ull)) >> 48] << 48;
        ((uint64_t*)data1)[i] |= (uint64_t)table[(((((uint64_t*)i1)[i] & 0xFF00000000000000ull)) | ((((uint64_t*)i2)[i] & 0xFF00000000000000ull) >> 8)) >> 48] << 56;
    }
    auto end1 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end1 - start1).count() << " us (table)\t";

    auto start2 = high_resolution_clock::now();
    switch(operation)
    {
        case ADD:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] + ((uint8_t*)i2)[i]) & 0x0F) | ((((uint8_t*)i1)[i] + (((uint8_t*)i2)[i] & 0xF0)) & 0xF0);
            }
            break;
        case QADD:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] & 0x0F) + (((uint8_t*)i2)[i] & 0x0F) > 0xF ? 0x0F : (((uint8_t*)i1)[i] & 0x0F) + (((uint8_t*)i2)[i] & 0x0F)) | ((((uint8_t*)i1)[i] & 0xF0) + (((uint8_t*)i2)[i] & 0xF0) > 0xF0 ? 0xF0 : (((uint8_t*)i1)[i] & 0xF0) + (((uint8_t*)i2)[i] & 0xF0));
            }
            break;
        case SUB:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] - ((uint8_t*)i2)[i]) & 0x0F) | ((((uint8_t*)i1)[i] - (((uint8_t*)i2)[i] & 0xF0)) & 0xF0);
            }
            break;
        case QSUB:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] & 0x0F) < (((uint8_t*)i2)[i] & 0x0F) ? 0x00 : (((uint8_t*)i1)[i] & 0x0F) - (((uint8_t*)i2)[i] & 0x0F)) | ((((uint8_t*)i1)[i] & 0xF0) < (((uint8_t*)i2)[i] & 0xF0) ? 0x00 : (((uint8_t*)i1)[i] & 0xF0) - (((uint8_t*)i2)[i] & 0xF0));
            }
            break;
        case MUL:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] * ((uint8_t*)i2)[i]) & 0x0F) | (((((uint8_t*)i1)[i] & 0xF0) * (((uint8_t*)i2)[i] >> 4)) & 0xF0);
            }
            break;
        case QMUL:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] & 0x0F) * (((uint8_t*)i2)[i] & 0x0F) > 0xF ? 0x0F : (((uint8_t*)i1)[i] & 0x0F) * (((uint8_t*)i2)[i] & 0x0F)) | ((((uint8_t*)i1)[i] & 0xF0) * ((((uint8_t*)i2)[i] & 0xF0) >> 4) > 0xF0 ? 0xF0 : (((uint8_t*)i1)[i] & 0xF0) * ((((uint8_t*)i2)[i] & 0xF0) >> 4));
            }
            break;
    }
    auto end2 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end2 - start2).count() << " us (reference)\t" << endl;

    for(int i = 0; i < (1 << 22); i++)
    {
        uint4x16_t control_a = *((uint4x16_t*)(i1) + i);
        uint4x16_t control_b = *((uint4x16_t*)(i2) + i);
        uint4x16_t d0 = *((uint4x16_t*)(data0) + i);
        uint4x16_t d1 = *((uint4x16_t*)(data1) + i);
        uint4x16_t d2 = *((uint4x16_t*)(data2) + i);
        for(int i = 0; i < 16; i++)
        {
            if(d0[i] != compute(control_a[i], control_b[i]))
            {
                cout << "ERROR D0" << endl;
                return -1;
            }
                
            if(d1[i] != compute(control_a[i], control_b[i]))
            {
                cout << "ERROR D1" << endl;
                return -1;
            }
                
            if(d2[i] != compute(control_a[i], control_b[i]))
            {
                cout << "ERROR D2" << endl;
                return -1;
            }
        }
    }
    delete[] (uint64_t*)data0;
    delete[] (uint64_t*)data1;
    delete[] (uint64_t*)data2;
    delete[] (uint64_t*)i1;
    delete[] (uint64_t*)i2;
    return (duration_cast<microseconds>(end0 - start0).count()) | (duration_cast<microseconds>(end1 - start1).count() << 20) | (duration_cast<microseconds>(end2 - start2).count() << 40);
}

#else
uint64_t manipulate_data(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    switch(operation)
    {
        case ADD:
            op = vadd_u4;
            compute = vadd_u4_compute;
            name = "vadd_u4";
            break;
        case QADD:
            op = vqadd_u4;
            compute = vqadd_u4_compute;
            name = "vqadd_u4";
            break;
        case SUB:
            op = vsub_u4;
            compute = vsub_u4_compute;
            name = "vsub_u4";
            break;
        case QSUB:
            op = vqsub_u4;
            compute = vqsub_u4_compute;
            name = "vqsub_u4";
            break;
        case MUL:
            op = vmul_u4;
            compute = vmul_u4_compute;
            name = "vmul_u4";
            break;
        case QMUL:
            op = vqmul_u4;
            compute = vqmul_u4_compute;
            name = "vqmul_u4";
            break;
    }
    void* i1 = new uint64_t[1 << 22];
    void* i2 = new uint64_t[1 << 22];
    void* data0 = new uint64_t[1 << 22];
    void* data1 = new uint64_t[1 << 22];
    void* data2 = new uint64_t[1 << 22];
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint64_t*)i1)[i] = my_random();
        ((uint64_t*)i2)[i] = my_random();
        ((uint64_t*)data0)[i] = 0;
        ((uint64_t*)data1)[i] = 0;
        ((uint64_t*)data2)[i] = 0;
    }
    build_table(compute);
    cout << name << " \t";

    auto start0 = high_resolution_clock::now();
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint4x16_t*)data0)[i] = op(((uint4x16_t*)i1)[i], ((uint4x16_t*)i2)[i]);
    }
    auto end0 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end0 - start0).count() << " us (vectorized)\t ";

    auto start1 = high_resolution_clock::now();
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint4x16_t*)data1)[i] = table_u4(((uint4x16_t*)i1)[i], ((uint4x16_t*)i2)[i]);
    }
    auto end1 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end1 - start1).count() << " us (table)\t";

    auto start2 = high_resolution_clock::now();
    switch(operation)
    {
        case ADD:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] + ((uint8_t*)i2)[i]) & 0x0F) | ((((uint8_t*)i1)[i] + (((uint8_t*)i2)[i] & 0xF0)) & 0xF0);
            }
            break;
        case QADD:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] & 0x0F) + (((uint8_t*)i2)[i] & 0x0F) > 0xF ? 0x0F : (((uint8_t*)i1)[i] & 0x0F) + (((uint8_t*)i2)[i] & 0x0F)) | ((((uint8_t*)i1)[i] & 0xF0) + (((uint8_t*)i2)[i] & 0xF0) > 0xF0 ? 0xF0 : (((uint8_t*)i1)[i] & 0xF0) + (((uint8_t*)i2)[i] & 0xF0));
            }
            break;
        case SUB:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] - ((uint8_t*)i2)[i]) & 0x0F) | ((((uint8_t*)i1)[i] - (((uint8_t*)i2)[i] & 0xF0)) & 0xF0);
            }
            break;
        case QSUB:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] & 0x0F) < (((uint8_t*)i2)[i] & 0x0F) ? 0x00 : (((uint8_t*)i1)[i] & 0x0F) - (((uint8_t*)i2)[i] & 0x0F)) | ((((uint8_t*)i1)[i] & 0xF0) < (((uint8_t*)i2)[i] & 0xF0) ? 0x00 : (((uint8_t*)i1)[i] & 0xF0) - (((uint8_t*)i2)[i] & 0xF0));
            }
            break;
        case MUL:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] * ((uint8_t*)i2)[i]) & 0x0F) | (((((uint8_t*)i1)[i] & 0xF0) * (((uint8_t*)i2)[i] >> 4)) & 0xF0);
            }
            break;
        case QMUL:
            for(int i = 0; i < (1 << 25); i++)
            {
                ((uint8_t*)data2)[i] = ((((uint8_t*)i1)[i] & 0x0F) * (((uint8_t*)i2)[i] & 0x0F) > 0xF ? 0x0F : (((uint8_t*)i1)[i] & 0x0F) * (((uint8_t*)i2)[i] & 0x0F)) | ((((uint8_t*)i1)[i] & 0xF0) * ((((uint8_t*)i2)[i] & 0xF0) >> 4) > 0xF0 ? 0xF0 : (((uint8_t*)i1)[i] & 0xF0) * ((((uint8_t*)i2)[i] & 0xF0) >> 4));
            }
            break;
    }
    auto end2 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end2 - start2).count() << " us (reference)\t" << endl;

    for(int i = 0; i < (1 << 22); i++)
    {
        uint4x16_t control_a = *((uint4x16_t*)(i1) + i);
        uint4x16_t control_b = *((uint4x16_t*)(i2) + i);
        uint4x16_t d0 = *((uint4x16_t*)(data0) + i);
        uint4x16_t d1 = *((uint4x16_t*)(data1) + i);
        uint4x16_t d2 = *((uint4x16_t*)(data2) + i);
        for(int i = 0; i < 16; i++)
        {
            if(d0[i] != compute(control_a[i], control_b[i]))
            {
                cout << "ERROR D0" << endl;
                return -1;
            }
            if(d1[i] != compute(control_a[i], control_b[i]))
            {
                cout << "ERROR D1" << endl;
                return -1;
            }
                
            if(d2[i] != compute(control_a[i], control_b[i]))
            {
                cout << "ERROR D2" << endl;
                return -1;
            }
        }
    }
    delete[] (uint64_t*)data0;
    delete[] (uint64_t*)data1;
    delete[] (uint64_t*)data2;
    delete[] (uint64_t*)i1;
    delete[] (uint64_t*)i2;
    return (duration_cast<microseconds>(end0 - start0).count()) | (duration_cast<microseconds>(end1 - start1).count() << 20) | (duration_cast<microseconds>(end2 - start2).count() << 40);
}
#endif
#endif

#ifdef MAINV1
#ifdef MAINTESTSPEED
void speedtest(OperationType op, const char* name)
{
    srand(time(NULL));
    uint64_t results_vector[1001];
    uint64_t results_novector[1001];
    uint64_t results_table[1001];
    std::ofstream file_vector;
    std::ofstream file_table;
    std::ofstream file_novector;
    char filename[50] = "Speed data: nofunction ";
    strcat(filename, name);
    strcat(filename, " O3.txt");
    file_vector.open(filename);
    
    file_table.open("Speed data: nofunction table O3.txt");

    strcpy(filename, "Speed data: nofunction ");
    strcat(filename, name);
    strcat(filename, " novector O3.txt");
    file_novector.open(filename);

    uint64_t sum_vector = 0;
    uint64_t sum_table = 0;
    uint64_t sum_novector = 0;
    for(int i = 0; i < 1001; i++)
    {
        cout << i << endl;
        uint64_t r = manipulate_data(op);
        results_vector[i] = r & 0xFFFFF;
        results_table[i] = (r >> 20) & 0xFFFFF;
        results_novector[i] = r >> 40;

        cout << results_vector[i] << " " << results_table[i] << " " << results_novector[i] << endl;

        file_vector << results_vector[i] << "\n";
        file_table << results_table[i] << "\n";
        file_novector << results_novector[i] << "\n";

        sum_vector += results_vector[i];
        sum_table += results_table[i];
        sum_novector += results_novector[i];
    }
    quicksort(results_vector, 1001);
    quicksort(results_table, 1001);
    quicksort(results_novector, 1001);

    file_vector << "\n" << "Median: " << results_vector[500] << "\n";
    file_vector << "Avg: " << sum_vector / 1001. << "\n";
    file_vector.close();
    
    file_table << "\n" << "Median: " << results_table[500] << "\n";
    file_table << "Avg: " << sum_table / 1001. << "\n";
    file_table.close();

    file_novector << "\n" << "Median: " << results_novector[500] << "\n";
    file_novector << "Avg: " << sum_novector / 1001. << "\n";
    file_novector.close();
}

int main()
{
    pin_to_core(1);
    char name[6][20];
    for(int i = 0; i < 6; i++)
    {
        cin >> name[i];
    }
    //speedtest(MUL, name[2]);
    speedtest(QADD, name[3]);
    speedtest(QSUB, name[4]);
    //speedtest(QMUL, name[5]);
    speedtest(ADD, name[0]);
    //speedtest(SUB, name[1]);
}
#else
int main()
{
    srand(time(NULL));
    manipulate_data(ADD);
    manipulate_data(QADD);
    manipulate_data(SUB);
    manipulate_data(QSUB);
    manipulate_data(MUL);
    manipulate_data(QMUL);
}
#endif
#endif

uint8_t vmla_u4_compute(uint8_t a, uint8_t b, uint8_t c)
{
    return (a + b * c) % 16;
}

uint8_t vqmla_u4_compute(uint8_t a, uint8_t b, uint8_t c)
{
    return (a + b * c) < 16 ? (a + b * c) : 15;
}

#if defined MAINV2
int auto_test_values(uint4x16_t a, uint4x16_t b, uint4x16_t c, int lane, uint4x16_t r, ComputeFunc compute)
{
    for(int i = 0; i < 16; i++)
    {
        if(compute(a[i], b[i], c[lane]) != uint8_t(r[i]))
            return i;
    }
    return -1;
}

int auto_test_correctness(OpFunc op, ComputeFunc compute)
{
    uint4x16_t a, b, v, r;
    int lane;
    int c = 0;
    for (int i = 0; i < 100000; i++)
    {
        a = my_random();
        b = my_random();
        v = my_random();
        lane = rand() % 16;
        r = op(a, b, v, lane);
        int j = auto_test_values(a, b, v, lane, r, compute);
        if (j >= 0)
        {
            for (int i = 15; i >= 0; i--)
                cout << a[i] << "\t";
            cout << endl;
            for (int i = 15; i >= 0; i--)
                cout << b[i] << "\t";
            cout << endl;
            cout << v[lane] << endl;
            for (int i = 15; i >= 0; i--)
                cout << r[i] << "\t";
            cout << endl
                 << "ERROR (" << j << ")" << endl;
            c++;
        }
    }
    return c;
}

int64_t auto_test_speed(OpFunc op, bool quick = false)
{
    int n_tests = 30000;
    int n_data = 10000;
    if(quick)
    {
        n_data /= 5;
        n_tests /= 2;
    }
    uint4x16_t a, b, v, r;
    int lane;
    auto start = high_resolution_clock::now();
    for (int j = 0; j < n_data; j++)
    {
        a = my_random();
        b = my_random();
        v = my_random();
        lane = rand() % 16;
        for (int i = 0; i < n_tests; i++)
        {
            r = op(a, b, v, lane);
        }
    }
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count();
}

int64_t auto_test(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    switch(operation)
    {
        case MLALANE:
            op = vmla_lane_u4;
            compute = vmla_u4_compute;
            name = "vmla_lane_u4";
            break;
        case QMLALANE:
            op = vqmla_lane_u4;
            compute = vqmla_u4_compute;
            name = "vqmla_lane_u4";
            break;
    }
    int c = auto_test_correctness(op, compute);
    if (c)
    {
        cout << name << " ALGORITHM NOT WORKING (" << c << " errors)" << endl;
        return -1;
    }
    cout << name << " OK, ";
    int64_t t = auto_test_speed(op);
    cout << (t / 300000.) << " ns" << endl;
    return t;
}

int main()
{
    auto_test(MLALANE);
    auto_test(QMLALANE);
}
#endif

#if defined MAINV3 || defined MAINV7
uint8_t i_j(uint8_t* p, int i, int j, int columns)
{
    uint8_t v = p[i * (columns/2) + j/2];
    if(j % 2)
        return v >> 4;
    return v & 0xF;
}

uint64_t sparse_random()
{
    uint64_t v = 0;
    while(rand() % 4 != 0)
        v += (1 << ((rand() % 16) * 4));
    return v;
}
#endif

#ifdef MAINV3
int main()
{
    int rows = 512, columns = 2048, inner = 1024;
    srand(time(NULL));
    uint64_t* matrix0 = new uint64_t[1 << 18];
    uint64_t* matrix1 = new uint64_t[1 << 18];
    uint64_t* result = new uint64_t[1 << 18];
    for(int i = 0; i < (1 << 18); i++)
    {
        matrix0[i] = sparse_random();
        matrix1[i] = sparse_random();
        result[i] = 0;
    }

    auto start = high_resolution_clock::now();
    vqmm_u4((uint4x16_t*)matrix0, (uint4x16_t*)matrix1, (uint4x16_t*)result, rows, inner, columns);
    auto end = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end - start).count() << " us" << endl;

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < columns; j++)
        {
            uint8_t r = 0;
            for(int k = 0; k < inner; k++)
            {
                r = vqmla_u4_compute(r, i_j((uint8_t*)matrix0, i, k, inner), i_j((uint8_t*)matrix1, k, j, columns));
            }
            if (r != i_j((uint8_t*)result, i, j, columns))
            {
                cout << (int)r << " " << (int)i_j((uint8_t*)result, i, j, columns) << "  (" << i << ", " << j << ")" << endl;
            }
        }
    }
}
#endif

uint16_t vdot_u16_compute(uint4x16_t a, uint4x16_t b)
{
    uint16_t r = 0;
    for(int i = 0; i < 16; i++)
    {
        r += a[i] * b[i];
    }
    return r;
}

#if defined MAINV4
int auto_test_values(uint4x16_t a, uint4x16_t b, uint16_t r, ComputeFunc compute)
{
    uint16_t r2 = compute(a, b);
    if(r2 != r)
        return r2;
    return -1;
}

int auto_test_correctness(OpFunc op, ComputeFunc compute)
{
    uint4x16_t a, b;
    uint16_t r;
    int c = 0;
    for (int i = 0; i < 100000; i++)
    {
        a = my_random();
        b = my_random();
        r = op(a, b);
        int j = auto_test_values(a, b, r, compute);
        if (j >= 0)
        {
            for (int i = 15; i >= 0; i--)
                cout << a[i] << "\t";
            cout << endl;
            for (int i = 15; i >= 0; i--)
                cout << b[i] << "\t";
            cout << endl << r << endl << "ERROR (expected " << j << ")" << endl;
            c++;
        }
    }
    return c;
}

int64_t auto_test_speed(OpFunc op, bool quick = false)
{
    int n_tests = 30000;
    int n_data = 10000;
    if(quick)
    {
        n_data /= 5;
        n_tests /= 2;
    }
    uint4x16_t a, b, v, r;
    int lane;
    auto start = high_resolution_clock::now();
    for (int j = 0; j < n_data; j++)
    {
        a = my_random();
        b = my_random();
        v = my_random();
        lane = rand() % 16;
        for (int i = 0; i < n_tests; i++)
        {
            r = op(a, b);
        }
    }
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count();
}

int64_t auto_test(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    switch(operation)
    {
        case DOT:
            op = vdot_u16;
            compute = vdot_u16_compute;
            name = "vdot";
            break;
    }
    int c = auto_test_correctness(op, compute);
    if (c)
    {
        cout << name << " ALGORITHM NOT WORKING (" << c << " errors)" << endl;
        return -1;
    }
    cout << name << " OK, ";
    int64_t t = auto_test_speed(op);
    cout << (t / 300000.) << " ns" << endl;
    return t;
}

int main()
{
    auto_test(DOT);
}
#endif

#ifdef MAINV5
uint64_t manipulate_data(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    switch(operation)
    {
        case MLALANE:
            op = vmla_lane_u4;
            compute = vmla_u4_compute;
            name = "vmla_lane_u4";
            break;
        case QMLALANE:
            op = vqmla_lane_u4;
            compute = vqmla_u4_compute;
            name = "vqmla_lane_u4";
            break;
    }
    void* i1 = new uint64_t[1 << 22];
    void* i2 = new uint64_t[1 << 22];
    void* i3 = new uint64_t[1 << 22];
    void* data0 = new uint64_t[1 << 22];
    void* data2 = new uint64_t[1 << 22];
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint64_t*)i1)[i] = my_random();
        ((uint64_t*)i2)[i] = my_random();
        ((uint64_t*)i3)[i] = my_random();
        ((uint64_t*)data0)[i] = 0;
        ((uint64_t*)data2)[i] = 0;
    }
    cout << name << " \t";

    auto start0 = high_resolution_clock::now();
    switch(operation)
    {
        case MLALANE:
            for(int i = 0; i < (1 << 22); i++)
            {
                int lane = i & 0xF;
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i], c_lane = (((uint64_t*)i3)[i] >> (lane << 2)) & 0xFull;
                uint64_t m_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F) * c_lane) & 0x0F0F0F0F0F0F0F0F;
                uint64_t m_high = ((a & 0xF0F0F0F0F0F0F0F0) + (b & 0xF0F0F0F0F0F0F0F0) * c_lane) & 0xF0F0F0F0F0F0F0F0;
                ((uint64_t*)data0)[i] = m_low | m_high;
            }
            break;
        case QMLALANE:
            for(int i = 0; i < (1 << 22); i++)
            {
                int lane = i & 0xF;
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i], c_lane = (((uint64_t*)i3)[i] >> (lane << 2)) & 0xFull, f;
                uint64_t r_low = ((a & 0x0F0F0F0F0F0F0F0F) + (b & 0x0F0F0F0F0F0F0F0F) * c_lane);
                uint64_t r_high = (((a >> 4) & 0x0F0F0F0F0F0F0F0F) + ((b >> 4) & 0x0F0F0F0F0F0F0F0F) * c_lane);
                uint64_t o = ((r_high) & 0xF0F0F0F0F0F0F0F0) | (((r_low) & 0xF0F0F0F0F0F0F0F0) >> 4);
                o |= (o >> 1);
                o |= (o >> 2);
                o &= 0x1111111111111111;
                f = o * 15;
                ((uint64_t*)data0)[i] = ((r_high & 0x0F0F0F0F0F0F0F0F) << 4) | (r_low & 0x0F0F0F0F0F0F0F0F) | f;
            }
            break;
    }
    auto end0 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end0 - start0).count() << " us (vectorized)\t ";

    auto start2 = high_resolution_clock::now();
    switch(operation)
    {
        case MLALANE:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t c_lane = ((uint8_t*)i3)[(i << 3) + ((i & 0xF) >> 1)];
                c_lane = (c_lane >> ((i & 1) << 2)) & 0x0F;
                for(int j = 0; j < 8; j++)
                {
                    ((uint8_t*)data2)[(i<<3) + j] = ((((uint8_t*)i1)[(i<<3) + j] + ((uint8_t*)i2)[(i<<3) + j] * c_lane) & 0x0F) | ((((uint8_t*)i1)[(i<<3) + j] + ((((uint8_t*)i2)[(i<<3) + j] & 0xF0) * c_lane)) & 0xF0);
                }
            }
            break;
        case QMLALANE:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t c_lane = ((uint8_t*)i3)[(i << 3) + ((i & 0xF) >> 1)];
                c_lane = (c_lane >> ((i & 1) << 2)) & 0x0F;
                for(int j = 0; j < 8; j++)
                {
                    ((uint8_t*)data2)[(i<<3) + j] = ((((uint8_t*)i1)[(i<<3) + j] & 0x0F) + (((uint8_t*)i2)[(i<<3) + j] & 0x0F) * c_lane > 0x0F ? 0x0F : (((uint8_t*)i1)[(i<<3) + j] & 0x0F) + (((uint8_t*)i2)[(i<<3) + j] & 0x0F) * c_lane) | ((((uint8_t*)i1)[(i<<3) + j] & 0xF0) + ((((uint8_t*)i2)[(i<<3) + j] & 0xF0) * c_lane) > 0xF0 ? 0xF0 : (((uint8_t*)i1)[(i<<3) + j] & 0xF0) + ((((uint8_t*)i2)[(i<<3) + j] & 0xF0) * c_lane));
                }
            }
    }
    auto end2 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end2 - start2).count() << " us (reference)\t" << endl;

    for(int i = 0; i < (1 << 22); i++)
    {
        uint4x16_t control_a = *((uint4x16_t*)(i1) + i);
        uint4x16_t control_b = *((uint4x16_t*)(i2) + i);
        uint4x16_t control_c = *((uint4x16_t*)(i3) + i);
        uint4x16_t d0 = *((uint4x16_t*)(data0) + i);
        uint4x16_t d2 = *((uint4x16_t*)(data2) + i);
        for(int j = 0; j < 16; j++)
        {
            if(d0[j] != compute(control_a[j], control_b[j], control_c[i & 0xF]))
            {
                cout << "ERROR D0" << endl;
                return -1;
            }
                
            if(d2[j] != compute(control_a[j], control_b[j], control_c[i & 0xF]))
            {
                cout << "ERROR D2" << endl;
                return -1;
            }
        }
    }
    delete[] (uint64_t*)data0;
    delete[] (uint64_t*)data2;
    delete[] (uint64_t*)i1;
    delete[] (uint64_t*)i2;
    delete[] (uint64_t*)i3;
    return (duration_cast<microseconds>(end0 - start0).count()) | (duration_cast<microseconds>(end2 - start2).count() << 32);
}

#ifdef MAINTESTSPEED
void speedtest(OperationType op, const char* name)
{
    srand(time(NULL));
    uint64_t results_vector[1001];
    uint64_t results_novector[1001];
    std::ofstream file_vector;
    std::ofstream file_novector;
    char filename[50] = "Speed data: nofunction ";
    strcat(filename, name);
    strcat(filename, " O3.txt");
    file_vector.open(filename);

    strcpy(filename, "Speed data: nofunction ");
    strcat(filename, name);
    strcat(filename, " novector O3.txt");
    file_novector.open(filename);

    uint64_t sum_vector = 0;
    uint64_t sum_table = 0;
    uint64_t sum_novector = 0;
    for(int i = 0; i < 1001; i++)
    {
        cout << i << endl;
        uint64_t r = manipulate_data(op);
        results_vector[i] = r & 0xFFFFFFFF;
        results_novector[i] = r >> 32;

        cout << results_vector[i] << " " << results_novector[i] << endl;

        file_vector << results_vector[i] << "\n";
        file_novector << results_novector[i] << "\n";

        sum_vector += results_vector[i];
        sum_novector += results_novector[i];
    }
    quicksort(results_vector, 1001);
    quicksort(results_novector, 1001);

    file_vector << "\n" << "Median: " << results_vector[500] << "\n";
    file_vector << "Avg: " << sum_vector / 1001. << "\n";
    file_vector.close();

    file_novector << "\n" << "Median: " << results_novector[500] << "\n";
    file_novector << "Avg: " << sum_novector / 1001. << "\n";
    file_novector.close();
}

int main()
{
    pin_to_core(1);
    char name[2][20];
    for(int i = 0; i < 2; i++)
    {
        cin >> name[i];
    }
    speedtest(MLALANE, name[0]);
    speedtest(QMLALANE, name[1]);
}
#else
int main()
{
    srand(time(NULL));
    manipulate_data(MLALANE);
    manipulate_data(QMLALANE);
}
#endif
#endif

#ifdef MAINV7
uint64_t manipulate_data(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    int rows = 512, columns = 2048, inner = 1024;
    switch(operation)
    {
        case MM:
            op = vmm_u4;
            compute = vmla_u4_compute;
            name = "vmm_u4 ";
            break;
        case QMM:
            op = vqmm_u4;
            compute = vqmla_u4_compute;
            name = "vqmm_u4 ";
            break;
    }
    void* i1 = new uint64_t[1 << 15];
    void* i2 = new uint64_t[1 << 17];
    void* data0 = new uint64_t[1 << 16];
    void* data2 = new uint64_t[1 << 16];
    for(int i = 0; i < (1 << 15); i++)
    {
        ((uint64_t*)i1)[i] = sparse_random();
    }
    for(int i = 0; i < (1 << 17); i++)
    {
        ((uint64_t*)i2)[i] = sparse_random();
    }
    for(int i = 0; i < (1 << 16); i++)
    {
        ((uint64_t*)data0)[i] = 0;
        ((uint64_t*)data2)[i] = 0;
    }
    cout << name << " \t";

    auto start0 = high_resolution_clock::now();
    const uint4x16_t* m0 = (uint4x16_t*)i1;
    const uint4x16_t* m1 = (uint4x16_t*)i2;
    uint4x16_t* mr = (uint4x16_t*)data0;
    int parts = columns >> 4;
    int parts_input = inner >> 4;
    switch(operation)
    {
        case MM:
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
            break;
        case QMM:
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
            break;
    }
    auto end0 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end0 - start0).count() << " us (vectorized)\t ";

    auto start2 = high_resolution_clock::now();
    switch(operation)
    {
        case MM:
            for(int r = 0; r < rows; r++)
            {
                for(int p = 0; p < (columns >> 1); p++)
                {
                    ((uint8_t*)data2)[r * (columns >> 1) + p] = 0;
                    for(int i = 0; i < inner; i++)
                    {
                        uint8_t v = ((uint8_t*)i1)[r * (inner >> 1) + (i >> 1)];
                        v = (v >> ((i & 1) << 2));
                        uint8_t w = ((uint8_t*)i2)[i * (columns >> 1) + p];
                        w = ((w * v) & 0x0F) | (((w & 0xF0) * v) & 0xF0);
                        ((uint8_t*)data2)[r * (columns >> 1) + p] = ((((uint8_t*)data2)[r * (columns >> 1) + p] + w) & 0x0F) | ((((uint8_t*)data2)[r * (columns >> 1) + p] + (w & 0xF0)) & 0xF0);
                    }
                }
            }
            break;
        case QMM:
            for(int r = 0; r < rows; r++)
            {
                for(int p = 0; p < (columns >> 1); p++)
                {
                    ((uint8_t*)data2)[r * (columns >> 1) + p] = 0;
                    for(int i = 0; i < inner; i++)
                    {
                        uint8_t v = ((uint8_t*)i1)[r * (inner >> 1) + (i >> 1)];
                        v = (v >> ((i & 1) << 2)) & 0xF;
                        uint8_t w = ((uint8_t*)i2)[i * (columns >> 1) + p];
                        w = (((w & 0xF) * v) > 0xF ? 0xF : ((w & 0xF) * v)) | (((w & 0xF0) * v) > 0xF0 ? 0xF0 : ((w & 0xF0) * v));
                        ((uint8_t*)data2)[r * (columns >> 1) + p] = ((((uint8_t*)data2)[r * (columns >> 1) + p] & 0x0F) + (w & 0x0F) > 0xF ? 0x0F : (((uint8_t*)data2)[r * (columns >> 1) + p] & 0x0F) + (w & 0x0F)) | ((((uint8_t*)data2)[r * (columns >> 1) + p] & 0xF0) + (w & 0xF0) > 0xF0 ? 0xF0 : (((uint8_t*)data2)[r * (columns >> 1) + p] & 0xF0) + (w & 0xF0));
                    }
                }
            }
            break;
    }
    auto end2 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end2 - start2).count() << " us (reference)\t" << endl;

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < columns; j++)
        {
            uint8_t r = 0;
            for(int k = 0; k < inner; k++)
            {
                r = compute(r, i_j((uint8_t*)i1, i, k, inner), i_j((uint8_t*)i2, k, j, columns));
            }
            if (r != i_j((uint8_t*)data0, i, j, columns))
            {
                cout << "ERROR D0" << endl;
                return -1;
            }
            if (r != i_j((uint8_t*)data2, i, j, columns))
            {
                cout << "ERROR D2" << endl;
                return -1;
            }
        }
    }
    delete[] (uint64_t*)data0;
    delete[] (uint64_t*)data2;
    delete[] (uint64_t*)i1;
    delete[] (uint64_t*)i2;
    return (duration_cast<microseconds>(end0 - start0).count()) | (duration_cast<microseconds>(end2 - start2).count() << 32);
}

#ifdef MAINTESTSPEED
void speedtest(OperationType op, const char* name)
{
    srand(time(NULL));
    uint64_t results_vector[1001];
    uint64_t results_novector[1001];
    std::ofstream file_vector;
    std::ofstream file_novector;
    char filename[50] = "Speed data: nofunction ";
    strcat(filename, name);
    strcat(filename, " O3.txt");
    file_vector.open(filename);

    strcpy(filename, "Speed data: nofunction ");
    strcat(filename, name);
    strcat(filename, " novector O3.txt");
    file_novector.open(filename);

    uint64_t sum_vector = 0;
    uint64_t sum_table = 0;
    uint64_t sum_novector = 0;
    for(int i = 0; i < 1001; i++)
    {
        cout << i << endl;
        uint64_t r = manipulate_data(op);
        results_vector[i] = r & 0xFFFFFFFF;
        results_novector[i] = r >> 32;

        cout << results_vector[i] << " " << results_novector[i] << endl;

        file_vector << results_vector[i] << "\n";
        file_novector << results_novector[i] << "\n";

        sum_vector += results_vector[i];
        sum_novector += results_novector[i];
    }
    quicksort(results_vector, 1001);
    quicksort(results_novector, 1001);

    file_vector << "\n" << "Median: " << results_vector[500] << "\n";
    file_vector << "Avg: " << sum_vector / 1001. << "\n";
    file_vector.close();

    file_novector << "\n" << "Median: " << results_novector[500] << "\n";
    file_novector << "Avg: " << sum_novector / 1001. << "\n";
    file_novector.close();
}

int main()
{
    pin_to_core(1);
    char name[2][20];
    for(int i = 0; i < 2; i++)
    {
        cin >> name[i];
    }
    speedtest(QMM, name[1]);
    speedtest(MM, name[0]);
}
#else
int main()
{
    srand(time(NULL));
    manipulate_data(MM);
    manipulate_data(QMM);
}
#endif
#endif

#ifdef MAINV6
uint64_t manipulate_data(OperationType operation)
{
    OpFunc op;
    ComputeFunc compute;
    const char* name;
    switch(operation)
    {
        case DOT:
            op = vdot_u16;
            compute = vdot_u16_compute;
            name = "vdot_u16";
            break;
    }
    void* i1 = new uint64_t[1 << 22];
    void* i2 = new uint64_t[1 << 22];
    void* data0 = new uint16_t[1 << 22];
    void* data2 = new uint16_t[1 << 22];
    for(int i = 0; i < (1 << 22); i++)
    {
        ((uint64_t*)i1)[i] = my_random();
        ((uint64_t*)i2)[i] = my_random();
        ((uint16_t*)data0)[i] = 0;
        ((uint16_t*)data2)[i] = 0;
    }
    cout << name << " \t";

    auto start0 = high_resolution_clock::now();
    switch(operation)
    {
        case DOT:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint64_t a = ((uint64_t*)i1)[i], b = ((uint64_t*)i2)[i], p_low, p_high, o, f, a_low, a_high, m, dot;
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

                ((uint16_t*)data0)[i] = (uint16_t)dot;
            }
            break;
    }
    auto end0 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end0 - start0).count() << " us (vectorized)\t ";

    auto start2 = high_resolution_clock::now();
    switch(operation)
    {
        case DOT:
            for(int i = 0; i < (1 << 22); i++)
            {
                uint16_t dot = 0;
                for(int j = 0; j < 8; j++)
                {
                    dot += (((uint8_t*)i1)[(i<<3) + j] & 0x0F) * (((uint8_t*)i2)[(i<<3) + j] & 0x0F);
                    dot += (((uint8_t*)i1)[(i<<3) + j] >> 4) * (((uint8_t*)i2)[(i<<3) + j] >> 4);
                }
                ((uint16_t*)data2)[i] = (uint16_t)dot;
            }
            break;
    }
    auto end2 = high_resolution_clock::now();
    cout << duration_cast<microseconds>(end2 - start2).count() << " us (reference)\t" << endl;

    for(int i = 0; i < (1 << 22); i++)
    {
        uint4x16_t control_a = *((uint4x16_t*)(i1) + i);
        uint4x16_t control_b = *((uint4x16_t*)(i2) + i);
        uint16_t d0 = *((uint16_t*)(data0) + i);
        uint16_t d2 = *((uint16_t*)(data2) + i);

        if(d0 != compute(control_a, control_b))
        {
            cout << "ERROR D0" << endl;
            return -1;
        }
            
        if(d2 != compute(control_a, control_b))
        {
            cout << "ERROR D2" << endl;
            return -1;
        }
    }
    delete[] (uint16_t*)data0;
    delete[] (uint16_t*)data2;
    delete[] (uint64_t*)i1;
    delete[] (uint64_t*)i2;
    return (duration_cast<microseconds>(end0 - start0).count()) | (duration_cast<microseconds>(end2 - start2).count() << 32);
}

#ifdef MAINTESTSPEED
void speedtest(OperationType op, const char* name)
{
    srand(time(NULL));
    uint64_t results_vector[1001];
    uint64_t results_novector[1001];
    std::ofstream file_vector;
    std::ofstream file_novector;
    char filename[50] = "Speed data: nofunction ";
    strcat(filename, name);
    strcat(filename, " O3.txt");
    file_vector.open(filename);

    strcpy(filename, "Speed data: nofunction ");
    strcat(filename, name);
    strcat(filename, " novector O3.txt");
    file_novector.open(filename);

    uint64_t sum_vector = 0;
    uint64_t sum_table = 0;
    uint64_t sum_novector = 0;
    for(int i = 0; i < 1001; i++)
    {
        cout << i << endl;
        uint64_t r = manipulate_data(op);
        results_vector[i] = r & 0xFFFFFFFF;
        results_novector[i] = r >> 32;

        cout << results_vector[i] << " " << results_novector[i] << endl;

        file_vector << results_vector[i] << "\n";
        file_novector << results_novector[i] << "\n";

        sum_vector += results_vector[i];
        sum_novector += results_novector[i];
    }
    quicksort(results_vector, 1001);
    quicksort(results_novector, 1001);

    file_vector << "\n" << "Median: " << results_vector[500] << "\n";
    file_vector << "Avg: " << sum_vector / 1001. << "\n";
    file_vector.close();

    file_novector << "\n" << "Median: " << results_novector[500] << "\n";
    file_novector << "Avg: " << sum_novector / 1001. << "\n";
    file_novector.close();
}

int main()
{
    pin_to_core(1);
    char name[1][20];
    for(int i = 0; i < 1; i++)
    {
        cin >> name[i];
    }
    speedtest(DOT, name[0]);
}
#else
int main()
{
    srand(time(NULL));
    manipulate_data(DOT);
}
#endif
#endif