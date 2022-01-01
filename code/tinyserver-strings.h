#if !defined(TINYSERVER_STRINGS_H)
/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */
#define TINYSERVER_STRINGS_H

#define CMP_FLAGS _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ORDERED | _SIDD_LEAST_SIGNIFICANT

typedef struct _string
{
    char* Ptr;
    usz Size;
    usz MaxSize;
} string;

#endif