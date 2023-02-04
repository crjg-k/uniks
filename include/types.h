#ifndef __TYPES_H__
#define __TYPES_H__

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#define INT8_MIN  (-0x7f - 1)
#define INT16_MIN (-0x7fff - 1)
#define INT32_MIN (-0x7fffffff - 1)
#define INT64_MIN (-0x7fffffffffffffff - 1)

#define INT8_MAX  (0x7f)
#define INT16_MAX (0x7fff)
#define INT32_MAX (0x7fffffff)
#define INT64_MAX (0x7fffffffffffffff)

#define UINT8_MAX  (0xff)
#define UINT16_MAX (0xffff)
#define UINT32_MAX (-1)
#define UINT64_MAX (-1)

/* size_t is used for memory object sizes */
typedef uint64_t size_t;

#endif /* !__TYPES_H__ */