/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

//==================
// Funcoes de copia
//==================

inline usz
StrLen(char* Buffer)
{
    usz Result = (usz)strlen(Buffer);
    return Result;
}

inline bool
CopyData(void* Dst, usz DstSize, void* Src, usz SrcSize)
{
    bool Result = false;
    if (DstSize >= SrcSize)
    {
        memcpy(Dst, Src, SrcSize);
        Result = true;
    }
    return Result;
}

inline bool
CopyString(string* Dst, string Src)
{
    bool Result = false;
    if (Dst->MaxSize >= Src.Size)
    {
        memcpy(Dst->Ptr, Src.Ptr, Src.Size);
        Dst->Size += Src.Size;
        Result = true;
    }
    return Result;
}

inline bool
AppendData(string* A, char* B)
{
    bool Result = false;
    usz BSize = StrLen(B);
    if ((A->MaxSize - A->Size) >= BSize)
    {
        memcpy(A->Ptr + A->Size, B, BSize);
        A->Size += BSize;
        Result = true;
    }
    return Result;
}

inline bool
AppendString(string* A, string B)
{
    bool Result = false;
    if ((A->MaxSize - A->Size) >= B.Size)
    {
        memcpy(A->Ptr + A->Size, B.Ptr, B.Size);
        A->Size += B.Size;
        Result = true;
    }
    return Result;
}

inline bool
AppendNumber(string* A, usz Number)
{
    bool Result = false;
    usz NumberOfDigits = GetNumberOfDigits(Number);
    if ((A->MaxSize - A->Size) >= NumberOfDigits)
    {
        for (usz Digit = 0; Digit < NumberOfDigits; Digit++)
        {
            A->Ptr[A->Size + NumberOfDigits - Digit - 1] = ((Number % 10) + 48);
            Number /= 10;
        }
        A->Size += NumberOfDigits;
        Result = true;
    }
    return Result;
}


//===================
// Classe de string
//===================

inline string
String(char* Data, usz Size)
{
    string Result;
    Result.Ptr = Data;
    Result.Size = Size;
    return Result;
}

#define StrLit(Str) String((Str), sizeof(Str)-1)

inline string
StringBuffer(char* Data, usz Size, usz MaxSize)
{
    string Result;
    Result.Ptr = Data;
    Result.Size = Size;
    Result.MaxSize = MaxSize;
    return Result;
}

//==========================
// Funcoes de conversao
//==========================

inline void
StringToArray(string A, char* B, usz BSize)
{
    Assert(B);
    
    if (BSize > A.Size)
    {
        CopyData(B, BSize, A.Ptr, A.Size);
        B[A.Size] = 0;
    }
}

inline bool
IsDigit(char Character)
{
    bool Result = (Character >= '0' && Character <= '9');
    return Result;
}

inline bool
IsLetter(char Character)
{
    bool Result = ((Character >= 'a' && Character <= 'z')
                   || (Character >= 'A' && Character <= 'Z'));
    return Result;
}

inline bool
IsNumber(string Number)
{
    bool Result = true;
    for (usz Idx = 0; Idx < Number.Size; Idx++)
    {
        if (!IsDigit(Number.Ptr[Idx]))
        {
            Result = false;
            break;
        }
    }
    return Result;
}

internal usz
StringToNumber(string A)
{
    usz Number = 0;
    if (A.Ptr)
    {
        for (usz Idx = 0; Idx < A.Size; Idx++)
        {
            if (!IsDigit(A.Ptr[Idx]))
            {
                Number = 0;
                break;
            }
            Number = Number * 10 + A.Ptr[Idx] - '0';
        }
    }
    return Number;
}

internal usz
ArrayToNumber(char* A)
{
    usz Number = 0;
    if (A)
    {
        for (usz Idx = 0; A[Idx]; Idx++)
        {
            if (!IsDigit(A[Idx]))
            {
                Number = 0;
                break;
            }
            Number = Number * 10 + A[Idx] - '0';
        }
    }
    return Number;
}

//=======================
// Funcoes de comparacao
//=======================

internal bool
Equals(string A, string B)
{
    bool Result = false;
    
    if (A.Size == B.Size)
    {
        usz Idx = 0;
        while (Idx < A.Size)
        {
            if (A.Ptr[Idx] != B.Ptr[Idx])
            {
                break;
            }
            Idx++;
        }
        if (Idx == A.Size)
        {
            Result = true;
        }
    }
    
    return Result;
}

internal bool
EqualsCI(string A, string B)
{
    bool Result = false;
    
    if (A.Size == B.Size)
    {
        usz Idx = 0;
        while (Idx < A.Size)
        {
            char AChar = (A.Ptr[Idx] >= 'A' && A.Ptr[Idx] <= 'Z') ? A.Ptr[Idx] + 32 : A.Ptr[Idx];
            char BChar = (B.Ptr[Idx] >= 'A' && B.Ptr[Idx] <= 'Z') ? B.Ptr[Idx] + 32 : B.Ptr[Idx];
            if (AChar != BChar)
            {
                break;
            }
            Idx++;
        }
        if (Idx == A.Size)
        {
            Result = true;
        }
    }
    
    return Result;
}

internal bool
Contains(string Haystack, string Needle, usz* restrict FoundIdx)
{
    bool Result = false;
    
    if (Haystack.Size >= Needle.Size)
    {
#if defined(ARCH_X8664_AVX)
        __m128i NeedleChunk = {0};
        usz NeedleFirstChunkSize = 0;
        if (Needle.Size >= XMM128_SIZE)
        {
            NeedleChunk = _mm_loadu_si128((__m128i*)Needle.Ptr);
            NeedleFirstChunkSize = XMM128_SIZE;
        }
        else
        {
            usz CharIdx = 0;
            NeedleChunk = _mm_xor_si128(NeedleChunk, NeedleChunk);
            for (; CharIdx < Needle.Size; CharIdx++)
            {
                NeedleChunk = _mm_srli_si128(NeedleChunk, sizeof(char));
                NeedleChunk = _mm_insert_epi8(NeedleChunk, Needle.Ptr[CharIdx], XMM128_LAST_IDX);
            }
            for (usz ZeroIdx = 0, Remaining = XMM128_SIZE-CharIdx; ZeroIdx < Remaining; ZeroIdx++)
            {
                NeedleChunk = _mm_srli_si128(NeedleChunk, sizeof(char));
            }
            NeedleFirstChunkSize = CharIdx;
        }
        
        int Offset = XMM128_SIZE;
        char* HaystackPtr = Haystack.Ptr; 
        __m128i HaystackChunk;
        
        usz HaystackRemainingSize = Haystack.Size;
        while (true)
        {
            if (HaystackRemainingSize >= XMM128_SIZE)
            {
                HaystackChunk = _mm_loadu_si128((__m128i*)HaystackPtr);
                if (!_mm_cmpistrc(NeedleChunk, HaystackChunk, CMP_FLAGS))
                {
                    HaystackPtr += XMM128_SIZE;
                    HaystackRemainingSize -= XMM128_SIZE;
                    continue;
                }
                else
                {
                    Offset = _mm_cmpistri(NeedleChunk, HaystackChunk, CMP_FLAGS);
                    HaystackPtr += Offset;
                    HaystackRemainingSize -= Offset;
                }
            }
            
            // OBS(roberto): Se chegou aqui, eh pq achou um match.
            bool EqualChunks = true;
            if (HaystackRemainingSize >= Needle.Size)
            {
                int ConsumedAmount = (int)XMM128_SIZE - Offset;
                char* LocalHaystackPtr = HaystackPtr + ConsumedAmount;
                char* LocalNeedlePtr = Needle.Ptr + ConsumedAmount;
                __m128i LocalNeedleChunk = NeedleChunk;
                
                usz NeedleRemainingSize = Needle.Size - ConsumedAmount;
                while (NeedleRemainingSize >= XMM128_SIZE)
                {
                    HaystackChunk = _mm_loadu_si128((__m128i*)LocalHaystackPtr);
                    LocalNeedleChunk = _mm_loadu_si128((__m128i*)LocalNeedlePtr);
                    if (_mm_cmpistrc(LocalNeedleChunk, HaystackChunk, CMP_FLAGS))
                    {
                        LocalHaystackPtr += XMM128_SIZE;
                        LocalNeedlePtr += XMM128_SIZE;
                        NeedleRemainingSize -= XMM128_SIZE;
                    }
                    else
                    {
                        EqualChunks = false;
                        break;
                    }
                }
                
                if (NeedleRemainingSize > 0
                    && EqualChunks)
                {
                    for (usz Idx = 0; Idx < NeedleRemainingSize; Idx++)
                    {
                        if (LocalHaystackPtr[Idx] != LocalNeedlePtr[Idx])
                        {
                            EqualChunks = false;
                            break;
                        }
                    }
                }
                
                if (EqualChunks)
                {
                    *FoundIdx = HaystackPtr - Haystack.Ptr;
                    Result = true;
                    break;
                }
                else
                {
                    HaystackPtr++;
                    HaystackRemainingSize--;
                }
            }
            else
            {
                break;
            }
        }
#elif defined(ARCH_X8664_SSE2)
        __m128i AllOnes = _mm_set1_epi8(0xFF);
        
        i64 DoubleChar = (u8)Needle.Ptr[0] | ((u8)Needle.Ptr[0] << sizeof(u8));
        __m128i FirstChar = _mm_cvtsi64_si128(DoubleChar);
        FirstChar = _mm_shufflelo_epi16(FirstChar, 0);
        FirstChar = _mm_shuffle_epi32 (FirstChar, 0);
        
        char* HaystackPtr = Haystack.Ptr;
        usz HaystackRemainingSize = Haystack.Size;
        while (true)
        {
            if (HaystackRemainingSize >= XMM128_SIZE)
            {
                __m128i HaystackChunk = _mm_load_si128((__m128i*)HaystackPtr);
                __m128i CompareChunk = _mm_cmpeq_epi8(HaystackChunk, FirstChar);
                u32 Mask = _mm_movemask_epi8(CompareChunk);
                
                if (Mask == 0)
                {
                    HaystackPtr += XMM128_SIZE;
                    HaystackRemainingSize -= XMM128_SIZE;
                    continue;
                }
                
                u32 Offset = 0;
                _BitScanForward(&Offset, Mask);
                HaystackPtr += Offset;
                HaystackRemainingSize -= Offset;
            }
            
            // OBS(roberto): Se chegou aqui, eh pq achou um match.
            if (HaystackRemainingSize >= Needle.Size)
            {
                char* LocalHaystackPtr = HaystackPtr;
                char* LocalNeedlePtr = Needle.Ptr;
                u32 Mask = 0;
                
                usz NeedleRemainingSize = Needle.Size;
                while (NeedleRemainingSize >= XMM128_SIZE)
                {
                    __m128i HaystackChunk = _mm_load_si128((__m128i*)LocalHaystackPtr);
                    __m128i NeedleChunk = _mm_load_si128((__m128i*)LocalNeedlePtr);
                    
                    __m128i CompareChunk= _mm_cmpeq_epi8(HaystackChunk, NeedleChunk);
                    CompareChunk= _mm_xor_si128(CompareChunk, AllOnes);
                    Mask = _mm_movemask_epi8(CompareChunk);
                    
                    if (Mask == 0)
                    {
                        LocalHaystackPtr += XMM128_SIZE;
                        LocalNeedlePtr += XMM128_SIZE;
                        NeedleRemainingSize -= XMM128_SIZE;
                    }
                    else
                    {
                        break;
                    }
                }
                
                // OBS(roberto): strings sao iguais, analisa a parte final e sai se for tudo igual.
                if (NeedleRemainingSize > 0
                    && Mask == 0)
                {
                    for (usz Idx = 0; Idx < NeedleRemainingSize; Idx++)
                    {
                        if (LocalHaystackPtr[Idx] != LocalNeedlePtr[Idx])
                        {
                            Mask = 1;
                            break;
                        }
                    }
                }
                
                if (Mask == 0)
                {
                    *FoundIdx = HaystackPtr - Haystack.Ptr;
                    Result = true;
                    break;
                }
                else
                {
                    HaystackPtr++;
                    HaystackRemainingSize--;
                }
            }
            else
            {
                break;
            }
        }
#else
        Result = true;
        usz Idx = 0;
        for (; Idx < Needle.Size; Idx++)
        {
            if (Haystack.Ptr[Idx] != Needle.Ptr[Idx])
            {
                Result = false;
                break;
            }
            Idx++;
        }
        *FoundIdx = Idx;
#endif
    }
    
    return Result;
}

internal bool
CharInArray(char Char, char* A)
{
    Assert(A);
    
    bool Result = false;
    while (*A)
    {
        if (Char == *A++)
        {
            Result = true;
            break;
        }
    }
    
    return Result;
}

internal bool
CharInString(char Needle, string Haystack, usz* restrict FoundIdx)
{
    bool Result = false;
    
#if defined(ARCH_X8664_AVX)
    if (Haystack.Size > XMM256_SIZE)
    {
        __m256i* Init = (__m256i*)((0-(intptr_t)XMM256_SIZE) & (intptr_t)Haystack.Ptr);
        __m256i Chunk = _mm256_loadu_si256(Init);
        
        i64 Target = (u8)Needle | ((u8)Needle << 8);
        __m128i LoMatch = _mm_cvtsi64_si128(Target);
        LoMatch = _mm_shufflelo_epi16(LoMatch, 0);
        LoMatch = _mm_shuffle_epi32(LoMatch, 0);
        __m256i Match = _mm256_setr_m128i(LoMatch, LoMatch);
        
        __m256i Temp = _mm256_cmpeq_epi8(Chunk, Match);
        unsigned long Offset = (unsigned long)((XMM256_LAST_IDX) & (intptr_t)Haystack.Ptr);
        u32 Mask = _mm256_movemask_epi8(Temp) & (~0u << Offset);
        
        usz RemainingSize = Haystack.Size - (XMM256_SIZE - Offset);
        while (RemainingSize >= XMM256_SIZE
               && Mask == 0)
        {
            Chunk = _mm256_loadu_si256(++Init);
            Temp = _mm256_cmpeq_epi8(Chunk, Match);
            Mask = _mm256_movemask_epi8(Temp);
            RemainingSize -= XMM256_SIZE;
        }
        
        if (Mask != 0)
        {
            _BitScanForward(&Offset, Mask);
            *FoundIdx = (intptr_t)Init - (intptr_t)Haystack.Ptr + Offset;
            Result = true;
        }
        else
        {
            *FoundIdx = (intptr_t)++Init - (intptr_t)Haystack.Ptr;
            Haystack.Ptr = (char*)Init;
            Haystack.Size = RemainingSize;
        }
    }
#elif defined(ARCH_X8664_SSE2)
    if (Haystack.Size > XMM128_SIZE)
    {
        __m128i* Init = (__m128i*)((0-(intptr_t)XMM128_SIZE) & (intptr_t)Haystack.Ptr);
        __m128i Chunk = _mm_load_si128(Init);
        
        i64 Target = (u8)Needle | ((u8)Needle << 8);
        __m128i Match = _mm_cvtsi64_si128(Target);
        Match = _mm_shufflelo_epi16(Match, 0);
        Match = _mm_shuffle_epi32(Match, 0);
        
        __m128i Temp = _mm_cmpeq_epi8(Chunk, Match);
        u32 Offset = (u32)((XMM128_SIZE-1) & (intptr_t)Haystack.Ptr);
        u32 Mask = _mm_movemask_epi8(Temp) & (~0u << Offset);
        
        usz RemainingSize = Haystack.Size - (XMM128_SIZE - Offset);
        while (RemainingSize >= XMM128_SIZE
               && Mask == 0)
        {
            Chunk = _mm_load_si128(++Init);
            Temp = _mm_cmpeq_epi8(Chunk, Match);
            Mask = _mm_movemask_epi8(Temp);
            RemainingSize -= XMM128_SIZE;
        }
        
        *FoundIdx = Haystack.Size - RemainingSize;
        if (Mask != 0)
        {
            _BitScanForward(&Offset, Mask);
            *FoundIdx += Offset;
            Result = true;
        }
        else
        {
            Haystack.Ptr = (char*)Init;
            Haystack.Size = RemainingSize;
        }
    }
#endif
    
    if (!Result)
    {
        for (usz Idx = 0; Idx < Haystack.Size; Idx++)
        {
            if (Haystack.Ptr[Idx] == Needle)
            {
                *FoundIdx += Idx;
                Result = true;
                break;
            }
        }
    }
    
    return Result;
}

internal bool
StringInList(string Target, char** List)
{
    bool Result = false;
    
    while (*(*List))
    {
        string Test = String(*List, StrLen(*List++));
        if (Equals(Target, Test))
        {
            Result = true;
            break;
        }
    }
    
    return Result;
}

internal string
GetToken(string A, usz* restrict Idx, char EndingMark)
{
    string Token = {0};
    
    if (A.Size > *Idx)
    {
        string Buffer = String(A.Ptr + *Idx, A.Size - *Idx);
        usz FoundIdx = 0;
        if (CharInString(EndingMark, Buffer, &FoundIdx))
        {
            Token.Size = FoundIdx;
            Token.Ptr = Buffer.Ptr;
            *Idx += Token.Size + 1;
        }
    }
    
    return Token;
}

internal bool
AppendToPath(string* Path, string Subpath)
{
    bool Result = false;
    
    if (Subpath.Ptr[0] == '\\'
        || Subpath.Ptr[0] == '/')
    {
        Subpath.Ptr++;
        Subpath.Size--;
    }
    
    if (Path->Size == 0
        && CopyString(Path, Subpath))
    {
        Path->Size = Subpath.Size;
        Result = true;
    }
    else if (Path->Size < Path->MaxSize)
    {
        if (Path->Ptr[Path->Size-1] != '\\');
        {
            Path->Ptr[Path->Size] = '\\';
            Path->Size++;
        }
        
        if ((Path->MaxSize - Path->Size) >= Subpath.Size)
        {
            AppendString(Path, Subpath);
            Result = true;
        }
    }
    
    return Result;
}
