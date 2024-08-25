//---
//	Cross-platform endianness conversion. (seriously?)
//---

#ifndef FX_ENDIANNESS
#define FX_ENDIANNESS

#if defined(__APPLE__)

  #include <libkern/OSByteOrder.h>

  #define htobe16(x) OSSwapHostToBigInt16(x)
  #define htole16(x) OSSwapHostToLittleInt16(x)
  #define be16toh(x) OSSwapBigToHostInt16(x)
  #define le16toh(x) OSSwapLittleToHostInt16(x)

  #define htobe32(x) OSSwapHostToBigInt32(x)
  #define htole32(x) OSSwapHostToLittleInt32(x)
  #define be32toh(x) OSSwapBigToHostInt32(x)
  #define le32toh(x) OSSwapLittleToHostInt32(x)

  #define htobe64(x) OSSwapHostToBigInt64(x)
  #define htole64(x) OSSwapHostToLittleInt64(x)
  #define be64toh(x) OSSwapBigToHostInt64(x)
  #define le64toh(x) OSSwapLittleToHostInt64(x)

#elif defined(__WINDOWS__) || defined(_WIN16) || defined(_WIN32) || defined(_WIN64)

  #include <winsock2.h>

  #if BYTE_ORDER == LITTLE_ENDIAN
    #define htobe16(x) __builtin_bswap16(x)
    #define htole16(x) (x)
    #define be16toh(x) __builtin_bswap16(x)
    #define le16toh(x) (x)

    #define htobe32(x) __builtin_bswap32(x)
    #define htole32(x) (x)
    #define be32toh(x) __builtin_bswap32(x)
    #define le32toh(x) (x)

    #define htobe64(x) __builtin_bswap64(x)
    #define htole64(x) (x)
    #define be64toh(x) __builtin_bswap64(x)
    #define le64toh(x) (x)
  #elif BYTE_ORDER == BIG_ENDIAN
    #define htobe16(x) (x)
    #define htole16(x) __builtin_bswap16(x)
    #define be16toh(x) (x)
    #define le16toh(x) __builtin_bswap16(x)

    #define htobe32(x) (x)
    #define htole32(x) __builtin_bswap32(x)
    #define be32toh(x) (x)
    #define le32toh(x) __builtin_bswap32(x)

    #define htobe64(x) (x)
    #define htole64(x) __builtin_bswap64(x)
    #define be64toh(x) (x)
    #define le64toh(x) __builtin_bswap64(x)
  #endif

#elif defined(__linux__)

  #include <sys/types.h>

#endif

#endif /* FX_ENDIANNESS */
