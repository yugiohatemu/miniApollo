//
//  AROUtil_C.h
//  apollo-ios-final
//
//  Created by Phil Kinsman on 2013-08-12.
//  Copyright (c) 2013 Arroware Industries Inc. All rights reserved.
//

#ifndef apollo_ios_final_AROUtil_C_h
#define apollo_ios_final_AROUtil_C_h
 
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#define NYB_TO_HEXCHAR(A)  ((((uint8_t)(A)) == (uint8_t)0) ? '0' :  \
(((uint8_t)(A)) == (uint8_t)1) ? '1' :  \
(((uint8_t)(A)) == (uint8_t)2) ? '2' :  \
(((uint8_t)(A)) == (uint8_t)3) ? '3' :  \
(((uint8_t)(A)) == (uint8_t)4) ? '4' :  \
(((uint8_t)(A)) == (uint8_t)5) ? '5' :  \
(((uint8_t)(A)) == (uint8_t)6) ? '6' :  \
(((uint8_t)(A)) == (uint8_t)7) ? '7' :  \
(((uint8_t)(A)) == (uint8_t)8) ? '8' :  \
(((uint8_t)(A)) == (uint8_t)9) ? '9' :  \
(((uint8_t)(A)) == (uint8_t)10) ? 'a' :  \
(((uint8_t)(A)) == (uint8_t)11) ? 'b' :  \
(((uint8_t)(A)) == (uint8_t)12) ? 'c' :  \
(((uint8_t)(A)) == (uint8_t)13) ? 'd' :  \
(((uint8_t)(A)) == (uint8_t)14) ? 'e' :  \
(((uint8_t)(A)) == (uint8_t)15) ? 'f' : 'X')

#define HEXCHAR_TO_NYB(A)   ((((char)(A)) == '0') ? (uint8_t)0 : \
(((char)(A)) == '1') ? (uint8_t)1 : \
(((char)(A)) == '2') ? (uint8_t)2 : \
(((char)(A)) == '3') ? (uint8_t)3 : \
(((char)(A)) == '4') ? (uint8_t)4 : \
(((char)(A)) == '5') ? (uint8_t)5 : \
(((char)(A)) == '6') ? (uint8_t)6 : \
(((char)(A)) == '7') ? (uint8_t)7 : \
(((char)(A)) == '8') ? (uint8_t)8 : \
(((char)(A)) == '9') ? (uint8_t)9 : \
(((char)(A)) == 'a') ? (uint8_t)10 : \
(((char)(A)) == 'A') ? (uint8_t)10 : \
(((char)(A)) == 'b') ? (uint8_t)11 : \
(((char)(A)) == 'B') ? (uint8_t)11 : \
(((char)(A)) == 'c') ? (uint8_t)12 : \
(((char)(A)) == 'C') ? (uint8_t)12 : \
(((char)(A)) == 'd') ? (uint8_t)13 : \
(((char)(A)) == 'D') ? (uint8_t)13 : \
(((char)(A)) == 'e') ? (uint8_t)14 : \
(((char)(A)) == 'E') ? (uint8_t)14 : \
(((char)(A)) == 'f') ? (uint8_t)15 : \
(((char)(A)) == 'F') ? (uint8_t)15 : (uint8_t)16)

#define NYB_SUCCESS(A)      (((A) & 0xf) == (A))

#define binaryReduceRangeWithKey(rangeLo,rangeHi,key,elementLo,elementHi,elementPivot) binaryReduceRangeWithRange(rangeLo,rangeHi,key,key,elementLo,elementHi,elementPivot)
    
#define binaryReduceRangeWithRange(rangeLo,rangeHi,keyLo,keyHi,elementLo,elementHi,elementPivot) \
   /* check range endpoints */                          \
   if (rangeHi >= rangeLo) { if (keyHi < (elementLo)) { \
       (rangeHi) = (rangeLo) - 1;                       \
   } else if (keyLo > (elementHi)) {                    \
       (rangeLo) = (rangeHi) + 1;                       \
   } else {                                             \
       /* find subrange lo */                           \
       if (keyLo > (elementLo)) {                       \
           int searchLo = (rangeLo);                    \
           int searchHi = (rangeHi);                    \
           int pivot;                                   \
           while (searchHi > (searchLo + 1)) {          \
               pivot = (searchHi + searchLo) / 2;       \
               if (keyLo > (elementPivot)) {            \
                   searchLo = pivot;                    \
               } else {                                 \
                   searchHi = pivot;                    \
               }                                        \
           }                                            \
           (rangeLo) = searchHi;                        \
       }                                                \
                                                        \
        /* find subrange hi */                          \
        if (keyHi < (elementLo)) {                      \
            (rangeHi) = (rangeLo) - 1;                  \
        } else {                                        \
            if (keyHi < (elementHi)) {                  \
                int searchLo = (rangeLo);               \
                int searchHi = (rangeHi);               \
                int pivot;                              \
                while (searchHi > (searchLo + 1)) {     \
                    pivot = (searchHi + searchLo) / 2;  \
                    if (keyHi < (elementPivot)) {       \
                        searchHi = pivot;               \
                    } else {                            \
                        searchLo = pivot;               \
                    }                                   \
                }                                       \
                (rangeHi) = searchLo;                   \
            }                                           \
        }                                               \
    } }

void usetmalloccounter(int val);
int ugetmalloccounter();
void usetmalloctraceflag();
void uunsetmalloctraceflag();
void *umalloc(int size);
void *urealloc(void *ptr, int size);
char *ustrdup(const char *op);
void ufree(void *ptr);
uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);

uint64_t AOc_localTimestamp(void);
uint16_t AOc_localTimestamp15_ms(void);
uint32_t AOc_localTimestamp31(void);
void AOc_uint64ToString(uint64_t value, char *str);
uint64_t AOc_uint64FromString(const char *str);
void AOc_uint32ToString(uint32_t value, char *str);
uint32_t AOc_uint32FromString(const char *str);
uint32_t AOc_hashRawData(uint32_t seed, uint8_t *bytes, int length);

#ifdef __cplusplus
}
#endif

#endif
