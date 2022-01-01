#if !defined(TINYSERVER_TYPES_H)
/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */
#define TINYSERVER_TYPES_H

#include <stdint.h>

// Configuracoes de sistema operacional
#if defined(_WIN32)
#define _WINDOWS_
#if defined(_M_X64)
#define ARCH_X64
#elif defined(_M_IX86)
#define ARCH_X86
#endif
#endif

// Configuracoes de arquitetura
#if defined(_WINDOWS_)
#include <intrin.h>
#if defined(ARCH_X86)
#if _M_IX86_FP == 2
#define ARCH_X8664_SSE
#define ARCH_X8664_SSE2
#if defined(__AVX2__)
#define ARCH_X8664_AVX2
#define ARCH_X8664_AVX
#elif defined(__AVX__)
#define ARCH_X8664_AVX
#endif
#elif _M_IX86_FP == 1
#define ARCH_X8664_SSE
#else
#define ARCH_X8664_IA32
#endif
#elif defined(ARCH_X64)
#if defined(__AVX2__)
#define ARCH_X8664_AVX2
#define ARCH_X8664_AVX
#elif defined(__AVX__)
#define ARCH_X8664_AVX
#endif
#endif
#endif

// Definicoes gerais

#if defined(_WINDOWS_)
#define _ReadBarrier_ _ReadBarrier(); _mm_sfence();
#define _WriteBarrier_ _WriteBarrier(); _mm_sfence();
#else
#define _ReadBarrier_
#define _WriteBarrier_
#endif

#define XMM128_SIZE 0x10
#define XMM128_LAST_IDX 0xF
#define XMM256_SIZE 0x20
#define XMM256_LAST_IDX 0x1F

// Definicao de tipos

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intptr_t isz;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t usz;

#if defined(ARCH_X64) // Registrador de 64-bits.
#define ISZ_MIN 0x8000000000000000
#define ISZ_MAX 0x7FFFFFFFFFFFFFFF
#define USZ_MIN 0x0
#define USZ_MAX 0xFFFFFFFFFFFFFFFF
#else               // Registrador de 32-bits.
#define ISZ_MIN 0x80000000
#define ISZ_MAX 0x7FFFFFFF
#define USZ_MIN 0x0
#define USZ_MAX 0xFFFFFFFF
#endif

#define __bool_true_false_are_defined 1
#if !defined(__cplusplus)
#define bool _Bool
#define false 0
#define true  1
#endif

#define internal static
#define global static
#define local static

#define Assert(Expression) if (!(Expression)) *(int*)0 = 0;
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))
#define _UnreachableCode_ Assert(0)

#define Kilobyte(Number) Number * 1024ULL
#define Megabyte(Number) Number * 1024ULL * 1024ULL
#define Gigabyte(Number) Number * 1024ULL * 1024ULL * 1024ULL
#define Terabyte(Number) Number * 1024ULL * 1024ULL * 1024ULL * 1024ULL

//======================
// Funcoes matematicas
//======================

inline u16
GetNumberOfDigits(usz Number)
{
    u16 Count = 0;
    do
    {
        Number /= 10;
        ++Count;
    } while (Number != 0);
    return(Count);
}

inline usz
Min(usz A, usz B)
{
    usz Result = A > B ? B : A;
    return Result;
}

inline usz
Max(usz A, usz B)
{
    usz Result = A > B ? A : B;
    return Result;
}

//================
// Funcoes da CRT
//================

#pragma function(memset)
void* memset(void* Dst, int Char, size_t Count)
{
#if defined(_FASTCODE_)
    __stosb((u8*)Dst, (u8)Char, Count);
#else
    char* Ptr = (char*)Dst;
    while (Count--)
    {
        *Ptr++ = (char)Char;
    }
#endif
    return Dst;
}

#pragma function(memcpy)
void* memcpy(void* Dst, void* Src, size_t SrcSize)
{
#if defined(_FASTCODE_)
    __movsb((u8*)Dst, (const u8*)Src, SrcSize);
#else
    u8* DstPtr = (u8*)Dst, *SrcPtr = (u8*)Src;
    while (SrcSize--)
    {
        *DstPtr++ = *SrcPtr++;
    }
#endif
    return Dst;
}

#pragma function(strlen)
size_t strlen(const char* Buffer)
{
    size_t Result = 0;
    while (*Buffer++)
    {
        Result++;
    }
    return Result;
}

#endif
