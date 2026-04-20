#define CCHESS_A3800

// ===== BEGIN base/base.h =====
#include <assert.h>
#ifdef __ANDROID__
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#ifndef BASE_H
#define BASE_H

#ifdef _MSC_VER
  typedef signed   __int64  int64_t; // ll
  typedef unsigned __int64 uint64_t; // qw
  typedef signed   __int32  int32_t; // l
  typedef unsigned __int32 uint32_t; // dw
  typedef signed   __int16  int16_t; // s
  typedef unsigned __int16 uint16_t; // w
  typedef signed   __int8   int8_t;  // c
  typedef unsigned __int8  uint8_t;  // uc
  #define FORMAT_I64 "I64"
#else
  #include <stdint.h>
  #define FORMAT_I64 "ll"
#endif

#define __ASSERT(a) assert(a)
#define __ASSERT_BOUND(a, b, c) assert((a) <= (b) && (b) <= (c))
#define __ASSERT_BOUND_2(a, b, c, d) assert((a) <= (b) && (b) <= (c) && (c) <= (d))

inline bool EQV(bool bArg1, bool bArg2) {
  return bArg1 ? bArg2 : !bArg2;
}

inline bool XOR(bool bArg1, bool bArg2) {
  return bArg1 ? !bArg2 : bArg2;
}

template <typename T> inline T MIN(T Arg1, T Arg2) {
  return Arg1 < Arg2 ? Arg1 : Arg2;
}

template <typename T> inline T MAX(T Arg1, T Arg2) {
  return Arg1 > Arg2 ? Arg1 : Arg2;
}

template <typename T> inline T ABS(T Arg) {
  return Arg < 0 ? -Arg : Arg;
}

template <typename T> inline T SQR(T Arg) {
  return Arg * Arg;
}

template <typename T> inline void SWAP(T &Arg1, T &Arg2) {
  T Temp;
  Temp = Arg1;
  Arg1 = Arg2;
  Arg2 = Temp;
}

inline int PopCnt8(uint8_t uc) {
  int n;
  n = ((uc >> 1) & 0x55) + (uc & 0x55);
  n = ((n >> 2) & 0x33) + (n & 0x33);
  return (n >> 4) + (n & 0x0f);
}

inline int PopCnt16(uint16_t w) {
  int n;
  n = ((w >> 1) & 0x5555) + (w & 0x5555);
  n = ((n >> 2) & 0x3333) + (n & 0x3333);
  n = ((n >> 4) & 0x0f0f) + (n & 0x0f0f);
  return (n >> 8) + (n & 0x00ff); 
}

inline int PopCnt32(uint32_t dw) {
  int n;
  n = ((dw >> 1) & 0x55555555) + (dw & 0x55555555);
  n = ((n >> 2) & 0x33333333) + (n & 0x33333333);
  n = ((n >> 4) & 0x0f0f0f0f) + (n & 0x0f0f0f0f);
  n = ((n >> 8) & 0x00ff00ff) + (n & 0x00ff00ff);
  return (n >> 16) + (n & 0x0000ffff);
}

#ifdef __ANDROID__

inline int64_t GetTime() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return (int64_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#else

inline int64_t GetTime() {
  timeb tb;
  ftime(&tb);
  return (int64_t) tb.time * 1000 + tb.millitm;
}

#endif

#endif
// ===== END base/base.h =====

// ===== BEGIN base/base2.h =====
#ifdef _WIN32
  #include <windows.h>
#else
  #include <pthread.h>
  #include <stdlib.h>
  #include <unistd.h>
#endif
#include <string.h>

#ifndef BASE2_H
#define BASE2_H

const int PATH_MAX_CHAR = 1024;

#ifdef _WIN32

inline void Idle(void) {
  Sleep(1);
}

const int PATH_SEPARATOR = '\\';

inline bool AbsolutePath(const char *sz) {
  return sz[0] == '\\' || (((sz[0] >= 'A' && sz[0] <= 'Z') || (sz[0] >= 'a' && sz[0] <= 'z')) && sz[1] == ':');
}

inline void GetSelfExe(char *szDst) {
  GetModuleFileName(NULL, szDst, PATH_MAX_CHAR);
}

inline void StartThread(void *ThreadEntry(void *), void *lpParameter) {
  DWORD dwThreadId;
  CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ThreadEntry, (LPVOID) lpParameter, 0, &dwThreadId);
}

#else

inline void Idle(void) {
  usleep(1000);
}

const int PATH_SEPARATOR = '/';

inline bool AbsolutePath(const char *sz) {
  return sz[0] == '/';
}

inline void GetSelfExe(char *szDst) {
  readlink("/proc/self/exe", szDst, PATH_MAX_CHAR);
}

inline void StartThread(void *ThreadEntry(void *), void *lpParameter) {
  pthread_t pthread;
  pthread_attr_t pthread_attr;
  pthread_attr_init(&pthread_attr);
  pthread_attr_setscope(&pthread_attr, PTHREAD_SCOPE_SYSTEM);
  pthread_create(&pthread, &pthread_attr, ThreadEntry, lpParameter);
}

#endif

inline void LocatePath(char *szDst, const char *szSrc) {
  char *lpSeparator;
  if (AbsolutePath(szSrc)) {
    strcpy(szDst, szSrc);
  } else {
    GetSelfExe(szDst);
    lpSeparator = strrchr(szDst, PATH_SEPARATOR);
    if (lpSeparator == NULL) {
      strcpy(szDst, szSrc);
    } else {
      strcpy(lpSeparator + 1, szSrc);
    }
  }
}

#endif
// ===== END base/base2.h =====

// ===== BEGIN base/x86asm.h =====

#ifndef X86ASM_H
#define X86ASM_H

inline uint32_t LOW_LONG(uint64_t Operand) {
  return (uint32_t) Operand;
}

inline uint32_t HIGH_LONG(uint64_t Operand) {
  return (uint32_t) (Operand >> 32);
}

inline uint64_t MAKE_LONG_LONG(uint32_t LowLong, uint32_t HighLong) {
  return (uint64_t) LowLong | ((uint64_t) HighLong << 32);
}

#if defined __arm__ || defined __aarch64__ || defined __mips__

static int cnBitScanTable[64] = {
  32,  0,  1, 12,  2,  6, -1, 13,  3, -1,  7, -1, -1, -1, -1, 14,
  10,  4, -1, -1,  8, -1, -1, 25, -1, -1, -1, -1, -1, 21, 27, 15,
  31, 11,  5, -1, -1, -1, -1, -1,  9, -1, -1, 24, -1, -1, 20, 26,
  30, -1, -1, -1, -1, 23, -1, 19, 29, -1, 22, 18, 28, 17, 16, -1
};

inline int BitScan(uint32_t Operand) {
  uint32_t dw = (Operand << 4) + Operand;
  dw += dw << 6;
  dw = (dw << 16) - dw;
  return cnBitScanTable[dw >> 26];  
}

inline int Bsf(uint32_t Operand) {
  return BitScan(Operand & -Operand);
}

inline int Bsr(uint32_t Operand) {
  uint32_t dw = Operand | (Operand >> 1);
  dw |= dw >> 2;
  dw |= dw >> 4;
  dw |= dw >> 8;
  dw |= dw >> 16;
  return BitScan(dw - (dw >> 1));
}

#else

#ifdef _MSC_VER

#pragma warning(disable: 4035)

__forceinline int Bsf(uint32_t Operand) {
  __asm {
    bsf eax, Operand;
  }
}

__forceinline int Bsr(uint32_t Operand) {
  __asm {
    bsr eax, Operand;
  }
}

__forceinline uint64_t TimeStampCounter(void) {
  __asm {
    rdtsc;
  }
}

__forceinline uint64_t LongMul(uint32_t Multiplier, uint32_t Multiplicand) {
  __asm {
    mov eax, Multiplier;
    mul Multiplicand;
  }
}

__forceinline uint64_t LongSqr(uint32_t Multiplier) {
  __asm {
    mov eax, Multiplier;
    mul Multiplier;
  }
}

__forceinline uint32_t LongDiv(uint64_t Dividend, uint32_t Divisor) {
  __asm {
    mov eax, dword ptr Dividend[0];
    mov edx, dword ptr Dividend[4];
    div Divisor;
  }
}

__forceinline uint32_t LongMod(uint64_t Dividend, uint32_t Divisor) {
  __asm {
    mov eax, dword ptr Dividend[0];
    mov edx, dword ptr Dividend[4];
    div Divisor;
    mov eax, edx;
  }
}

__forceinline uint32_t LongMulDiv(uint32_t Multiplier, uint32_t Multiplicand, uint32_t Divisor) {
  __asm {
    mov eax, Multiplier;
    mul Multiplicand;
    div Divisor;
  }
}

__forceinline uint32_t LongMulMod(uint32_t Multiplier, uint32_t Multiplicand, uint32_t Divisor) {
  __asm {
    mov eax, Multiplier;
    mul Multiplicand;
    div Divisor;
    mov eax, edx;
  }
}

__forceinline uint32_t Shld(uint32_t HighLong, uint32_t LowLong, uint32_t Count) {
  __asm {
    mov eax, HighLong;
    mov edx, LowLong;
    mov ecx, Count;
    shld eax, edx, cl;
  }
}

__forceinline uint32_t Shrd(uint32_t LowLong, uint32_t HighLong, uint32_t Count) {
  __asm {
    mov eax, LowLong;
    mov edx, HighLong;
    mov ecx, Count;
    shrd eax, edx, cl;
  }
}

#pragma warning(default: 4035)

#else

static __inline__ int Bsf(uint32_t Operand) {
  int eax;
  asm __volatile__ (
    "bsfl %0, %0" "\n\t"
    : "=a" (eax)
    : "0" (Operand)
  );
  return eax;
}

static __inline__ int Bsr(uint32_t Operand) {
  int eax;
  asm __volatile__ (
    "bsrl %0, %0" "\n\t"
    : "=a" (eax)
    : "0" (Operand)
  );
  return eax;
}

static __inline__ uint64_t TimeStampCounter(void) {
  uint32_t eax, edx;
  asm __volatile__ (
    "rdtsc" "\n\t"
    : "=a" (eax), "=d" (edx)
    :
  );
  return MAKE_LONG_LONG(eax, edx);
}

static __inline__ uint64_t LongMul(uint32_t Multiplier, uint32_t Multiplicand) {
  uint32_t eax, edx;
  asm __volatile__ (
    "mull %1" "\n\t"
    : "=a" (eax), "=d" (edx)
    : "0" (Multiplier), "1" (Multiplicand)
  );
  return MAKE_LONG_LONG(eax, edx);
}

static __inline__ uint64_t LongSqr(uint32_t Multiplier) {
  uint32_t eax, edx;
  asm __volatile__ (
    "mull %1" "\n\t"
    : "=a" (eax), "=d" (edx)
    : "0" (Multiplier), "1" (Multiplier)
  );
  return MAKE_LONG_LONG(eax, edx);
}

static __inline__ uint32_t LongDiv(uint64_t Dividend, uint32_t Divisor) {
  uint32_t eax, edx, dummy;
  asm __volatile__ (
    "divl %2" "\n\t"
    : "=a" (eax), "=d" (edx), "=g" (dummy)
    : "0" (LOW_LONG(Dividend)), "1" (HIGH_LONG(Dividend)), "2" (Divisor)
  );
  return eax;
}

static __inline__ uint32_t LongMod(uint64_t Dividend, uint32_t Divisor) {
  uint32_t eax, edx, dummy;
  asm __volatile__ (
    "divl %2"     "\n\t"
    : "=a" (eax), "=d" (edx), "=g" (dummy)
    : "0" (LOW_LONG(Dividend)), "1" (HIGH_LONG(Dividend)), "2" (Divisor)
  );
  return edx;
}

static __inline__ uint32_t LongMulDiv(uint32_t Multiplier, uint32_t Multiplicand, uint32_t Divisor) {
  uint32_t eax, edx, dummy;
  asm __volatile__ (
    "mull %1" "\n\t"
    "divl %2" "\n\t"
    : "=a" (eax), "=d" (edx), "=g" (dummy)
    : "0" (Multiplier), "1" (Multiplicand), "2" (Divisor)
  );
  return eax;
}

static __inline__ uint32_t LongMulMod(uint32_t Multiplier, uint32_t Multiplicand, uint32_t Divisor) {
  uint32_t eax, edx, dummy;
  asm __volatile__ (
    "mull %1"     "\n\t"
    "divl %2"     "\n\t"
    : "=a" (eax), "=d" (edx), "=g" (dummy)
    : "0" (Multiplier), "1" (Multiplicand), "2" (Divisor)
  );
  return edx;
}

static __inline uint32_t Shld(uint32_t High, uint32_t Low, uint32_t Count) {
  uint32_t eax, edx, ecx;
  asm __volatile__ (
    "shldl %%cl, %1, %0" "\n\t"
    : "=a" (eax), "=d" (edx), "=c" (ecx)
    : "0" (High), "1" (Low), "2" (Count)
  );
  return eax;
}

static __inline uint32_t Shrd(uint32_t Low, uint32_t High, uint32_t Count) {
  uint32_t eax, edx, ecx;
  asm __volatile__ (
    "shrdl %%cl, %1, %0" "\n\t"
    : "=a" (eax), "=d" (edx), "=c" (ecx)
    : "0" (Low), "1" (High), "2" (Count)
  );
  return eax;
}

#endif

inline uint64_t LongShl(uint64_t Operand, uint32_t Count) {
  if (Count < 32) {
    return MAKE_LONG_LONG(LOW_LONG(Operand) << Count, Shld(HIGH_LONG(Operand), LOW_LONG(Operand), Count));
  } else if (Count < 64) {
    return MAKE_LONG_LONG(0, LOW_LONG(Operand) << (Count - 32));
  } else {
    return MAKE_LONG_LONG(0, 0);
  }
}

inline uint64_t LongShr(uint64_t Operand, uint32_t Count) {
  if (Count < 32) {
    return MAKE_LONG_LONG(Shrd(LOW_LONG(Operand), HIGH_LONG(Operand), Count), HIGH_LONG(Operand) >> Count);
  } else if (Count < 64) {
    return MAKE_LONG_LONG(HIGH_LONG(Operand) >> (Count - 32), 0);
  } else {
    return MAKE_LONG_LONG(0, 0);
  }
}

#endif

#endif
// ===== END base/x86asm.h =====

// ===== BEGIN base/rc4prng.h =====

#ifndef RC4PRNG_H
#define RC4PRNG_H

struct RC4Struct {
  uint8_t s[256];
  int x, y;

  void Init(void *lpKey, int nKeyLen) {
    int i, j;
    x = y = j = 0;
    for (i = 0; i < 256; i ++) {
      s[i] = i;
    }
    for (i = 0; i < 256; i ++) {
      j = (j + s[i] + ((uint8_t *) lpKey)[i % nKeyLen]) & 255;
      SWAP(s[i], s[j]);
    }
  }

  void InitZero(void) {
    uint32_t dwKey;
    dwKey = 0;
    Init(&dwKey, 4);
  }

  void InitRand(void) {
    union {
      uint32_t dw[2];
      uint64_t qw;
    } Seed;
#if defined __arm__ || defined __aarch64__ || defined __mips__
    Seed.qw = 0;
#else
    Seed.qw = TimeStampCounter();
#endif
    Seed.dw[1] ^= (uint32_t) GetTime();
    Init(&Seed, 8);
  }

  uint8_t NextByte(void) {
    x = (x + 1) & 255;
    y = (y + s[x]) & 255;
    SWAP(s[x], s[y]);
    return s[(s[x] + s[y]) & 255];
  }

  uint32_t NextLong(void) {
    union {
      uint8_t uc[4];
      uint32_t dw;
    } Ret;
    Ret.uc[0] = NextByte();
    Ret.uc[1] = NextByte();
    Ret.uc[2] = NextByte();
    Ret.uc[3] = NextByte();
    return Ret.dw;
  }
};

#endif
// ===== END base/rc4prng.h =====

// ===== BEGIN bot/Common.h =====
#ifndef COMMON_H
#define COMMON_H

#include <string>

constexpr int BOARDWIDTH = 9;
constexpr int BOARDHEIGHT = 10;

inline int pgnchar2int(char c) {
    return static_cast<int>(c) - static_cast<int>('a');
}

inline char pgnint2char(int i) {
    return static_cast<char>(static_cast<int>('a') + i);
}

inline int char2int(char c) {
    return static_cast<int>(c) - static_cast<int>('0');
}

inline char int2char(int i) {
    return static_cast<char>(static_cast<int>('0') + i);
}

struct Move {
    int source_x;
    int source_y;
    int target_x;
    int target_y;

    Move() : source_x(-1), source_y(-1), target_x(-1), target_y(-1) {}

    Move(int sx, int sy, int tx, int ty)
        : source_x(sx), source_y(sy), target_x(tx), target_y(ty) {}

    Move(const std::string& source, const std::string& target) {
        source_x = pgnchar2int(source[0]);
        source_y = char2int(source[1]);
        target_x = pgnchar2int(target[0]);
        target_y = char2int(target[1]);
    }

    bool operator==(const Move& other) const {
        return source_x == other.source_x &&
               source_y == other.source_y &&
               target_x == other.target_x &&
               target_y == other.target_y;
    }

    bool operator!=(const Move& other) const {
        return !(*this == other);
    }

    bool isInvalid() const {
        return source_x < 0 || source_y < 0 || target_x < 0 || target_y < 0;
    }
};

#endif
// ===== END bot/Common.h =====

// ===== BEGIN bot/Protocol.h =====
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>

#ifdef _BOTZONE_ONLINE
#include "jsoncpp/json.h"
#else
#include <json/json.h>
#endif

struct ProtocolMoveEntry {
    bool valid;
    Move move;

    ProtocolMoveEntry() : valid(false), move() {}
};

struct ParsedInput {
    std::vector<ProtocolMoveEntry> requests;
    std::vector<ProtocolMoveEntry> responses;
    Json::Value data;

    ParsedInput() : data(Json::Value(Json::objectValue)) {}
};

class Protocol {
public:
    static bool parseInput(const std::string& text, ParsedInput& out, std::string* errorMessage = nullptr);
    static Json::Value makeOutput(const Move& move, const Json::Value& data = Json::Value(Json::objectValue));
    static std::string writeJson(const Json::Value& root);

private:
    static bool parseMoveObject(const Json::Value& value, ProtocolMoveEntry& entry);
    static std::string posToString(int x, int y);
};

#endif
// ===== END bot/Protocol.h =====

// ===== BEGIN bot/EyeAdapter.h =====
#ifndef EYE_ADAPTER_H
#define EYE_ADAPTER_H

#include <vector>

class EyeAdapter {
public:
    EyeAdapter() = default;
    Move getBestMove(const std::vector<Move>& history) const;

private:
    static void ensureInit();
    static int botzoneMoveToEye(const Move& mv);
    static Move eyeMoveToBotzone(int mv);
};

#endif
// ===== END bot/EyeAdapter.h =====

// ===== BEGIN eleeye/pregen.h =====
/*
pregen.h/pregen.cpp - Source Code for ElephantEye, Part II

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.0, Last Modified: Nov. 2007
Copyright (C) 2004-2007 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef PREGEN_H
#define PREGEN_H

#define __ASSERT_PIECE(pc) __ASSERT((pc) >= 16 && (pc) <= 47)
#define __ASSERT_SQUARE(sq) __ASSERT(IN_BOARD(sq))
#define __ASSERT_BITRANK(w) __ASSERT((w) < (1 << 9))
#define __ASSERT_BITFILE(w) __ASSERT((w) < (1 << 10))

const int RANK_TOP = 3;
const int RANK_BOTTOM = 12;
const int FILE_LEFT = 3;
const int FILE_CENTER = 7;
const int FILE_RIGHT = 11;

extern const bool cbcInBoard[256];    // 
extern const bool cbcInFort[256];     // ǳ
extern const bool cbcCanPromote[256]; // 
extern const int8_t ccLegalSpanTab[512];   // ŷȱ
extern const int8_t ccKnightPinTab[512];   // ȱ

inline bool IN_BOARD(int sq) {
  return cbcInBoard[sq];
}

inline bool IN_FORT(int sq) {
  return cbcInFort[sq];
}

inline bool CAN_PROMOTE(int sq) {
  return cbcCanPromote[sq];
}

inline int8_t LEGAL_SPAN_TAB(int nDisp) {
  return ccLegalSpanTab[nDisp];
}

inline int8_t KNIGHT_PIN_TAB(int nDisp) {
  return ccKnightPinTab[nDisp];
}

inline int RANK_Y(int sq) {
  return sq >> 4;
}

inline int FILE_X(int sq) {
  return sq & 15;
}

inline int COORD_XY(int x, int y) {
  return x + (y << 4);
}

inline int SQUARE_FLIP(int sq) {
  return 254 - sq;
}

inline int FILE_FLIP(int x) {
  return 14 - x;
}

inline int RANK_FLIP(int y) {
  return 15 - y;
}

inline int OPP_SIDE(int sd) {
  return 1 - sd;
}

inline int SQUARE_FORWARD(int sq, int sd) {
  return sq - 16 + (sd << 5);
}

inline int SQUARE_BACKWARD(int sq, int sd) {
  return sq + 16 - (sd << 5);
}

inline bool KING_SPAN(int sqSrc, int sqDst) {
  return LEGAL_SPAN_TAB(sqDst - sqSrc + 256) == 1;
}

inline bool ADVISOR_SPAN(int sqSrc, int sqDst) {
  return LEGAL_SPAN_TAB(sqDst - sqSrc + 256) == 2;
}

inline bool BISHOP_SPAN(int sqSrc, int sqDst) {
  return LEGAL_SPAN_TAB(sqDst - sqSrc + 256) == 3;
}

inline int BISHOP_PIN(int sqSrc, int sqDst) {
  return (sqSrc + sqDst) >> 1;
}

inline int KNIGHT_PIN(int sqSrc, int sqDst) {
  return sqSrc + KNIGHT_PIN_TAB(sqDst - sqSrc + 256);
}

inline bool WHITE_HALF(int sq) {
  return (sq & 0x80) != 0;
}

inline bool BLACK_HALF(int sq) {
  return (sq & 0x80) == 0;
}

inline bool HOME_HALF(int sq, int sd) {
  return (sq & 0x80) != (sd << 7);
}

inline bool AWAY_HALF(int sq, int sd) {
  return (sq & 0x80) == (sd << 7);
}

inline bool SAME_HALF(int sqSrc, int sqDst) {
  return ((sqSrc ^ sqDst) & 0x80) == 0;
}

inline bool DIFF_HALF(int sqSrc, int sqDst) {
  return ((sqSrc ^ sqDst) & 0x80) != 0;
}

inline int RANK_DISP(int y) {
  return y << 4;
}

inline int FILE_DISP(int x) {
  return x;
}

// λС͡λСɳŷԤýṹ
struct SlideMoveStruct {
  uint8_t ucNonCap[2];    // ߵһ/Сһ
  uint8_t ucRookCap[2];   // ߵһ/Сһ
  uint8_t ucCannonCap[2]; // ڳߵһ/Сһ
  uint8_t ucSuperCap[2];  // (ӳ)ߵһ/Сһ
}; // smv

// λС͡λСжϳŷԵԤýṹ
struct SlideMaskStruct {
  uint16_t wNonCap, wRookCap, wCannonCap, wSuperCap;
}; // sms

struct ZobristStruct {
  uint32_t dwKey, dwLock0, dwLock1;
  void InitZero(void) {
    dwKey = dwLock0 = dwLock1 = 0;
  }
  void InitRC4(RC4Struct &rc4) {
    dwKey = rc4.NextLong();
    dwLock0 = rc4.NextLong();
    dwLock1 = rc4.NextLong();
  }
  void Xor(const ZobristStruct &zobr) {
    dwKey ^= zobr.dwKey;
    dwLock0 ^= zobr.dwLock0;
    dwLock1 ^= zobr.dwLock1;
  }
  void Xor(const ZobristStruct &zobr1, const ZobristStruct &zobr2) {
    dwKey ^= zobr1.dwKey ^ zobr2.dwKey;
    dwLock0 ^= zobr1.dwLock0 ^ zobr2.dwLock0;
    dwLock1 ^= zobr1.dwLock1 ^ zobr2.dwLock1;
  }
}; // zobr

extern struct PreGenStruct {
  // ZobristֵZobristֵZobristУ
  ZobristStruct zobrPlayer;
  ZobristStruct zobrTable[14][256];

  uint16_t wBitRankMask[256]; // ÿӵλеλ
  uint16_t wBitFileMask[256]; // ÿӵλеλ

  /* λС͡λСɳŷжϳŷԵԤ
   *
   * λС͡λСElephantEyeĺļڵŷɡжϺ;
   * Գʼ췽ұߵڸежΪȱ֪еġλС"1010000101b"
   * ElephantEyeԤ飬"...MoveTab""...MaskTab"÷ֱǣ
   * һҪ֪ǰӵĿ(ʼ2Ŀ9)ôϣ֪ӣ
   * Ԥһ"FileMoveTab_CannonCap[10][1024]"ʹ"FileMoveTab_CannonCap[2][1010000101b] == 9"Ϳˡ
   * ҪжϸܷԵĿ(ͬʼ2Ŀ9Ϊ)ôҪ֪ĿλУ"0000000001b"
   * ֻҪ"...MoveTab"ĸԡλʽ¼"...MaskTab"Ϳˣá롱жܷԵĿ
   * ͨһ"...MaskTab"ԪλжܷԵͬлͬеĳʱֻҪһжϾͿˡ
   */
  SlideMoveStruct smvRankMoveTab[9][512];   // 36,864 ֽ
  SlideMoveStruct smvFileMoveTab[10][1024]; // 81,920 ֽ
  SlideMaskStruct smsRankMaskTab[9][512];   // 36,864 ֽ
  SlideMaskStruct smsFileMaskTab[10][1024]; // 81,920 ֽ
                                            // :  237,568 ֽ

  /* (ʺáλС͡λС)ŷԤ
   *
   * ⲿϵġŷԤɡ飬ԸĳӵʼֱӲ飬õеĿ
   * ʹʱԸʼȷһָ"g_...Moves[Square]"ָָһϵĿ0
   * Ϊ˶ַ[256][n]n4ıұn(Ϊ˽ʶ0)ۺ顣
   */
  uint8_t ucsqKingMoves[256][8];
  uint8_t ucsqAdvisorMoves[256][8];
  uint8_t ucsqBishopMoves[256][8];
  uint8_t ucsqBishopPins[256][4];
  uint8_t ucsqKnightMoves[256][12];
  uint8_t ucsqKnightPins[256][8];
  uint8_t ucsqPawnMoves[2][256][4];
} PreGen;

// Ԥ۽ṹ
extern struct PreEvalStruct {
  bool bPromotion;
  int vlAdvanced;
  uint8_t ucvlWhitePieces[7][256];
  uint8_t ucvlBlackPieces[7][256];
} PreEval;

void PreGenInit(void);

#endif
// ===== END eleeye/pregen.h =====

// ===== BEGIN eleeye/preeval.h =====
/*
preeval.h/preeval.cpp - Source Code for ElephantEye, Part X

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.3, Last Modified: Mar. 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef PREEVAL_H
#define PREEVAL_H

// չľԤ۽ṹ
extern struct PreEvalStructEx {
  int vlBlackAdvisorLeakage, vlWhiteAdvisorLeakage;
  int vlHollowThreat[16], vlCentralThreat[16];
  int vlWhiteBottomThreat[16], vlBlackBottomThreat[16];
  char cPopCnt16[65536]; // PopCnt16飬ֻҪʼһ
} PreEvalEx;

#endif
// ===== END eleeye/preeval.h =====

// ===== BEGIN eleeye/position.h =====
/*
position.h/position.cpp - Source Code for ElephantEye, Part III

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.3, Last Modified: Mar. 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string.h>

/* ElephantEyeԴʹõǺԼ
 *
 * sq: (0255"pregen.cpp")
 * pc: (047"position.cpp")
 * pt: (06"position.cpp")
 * mv: ŷ(065535"position.cpp")
 * sd: ӷ(0췽1ڷ)
 * vl: ֵ("-MATE_VALUE""MATE_VALUE""position.cpp")
 * (עĸǺſucdwȴļǺʹ)
 * pos: (PositionStructͣ"position.h")
 * sms: λкλеŷԤýṹ("pregen.h")
 * smv: λкλеŷжԤýṹ("pregen.h")
 */

#ifndef POSITION_H
#define POSITION_H

const int MAX_MOVE_NUM = 1024;  // ɵĻعŷ
const int MAX_GEN_MOVES = 128;  // ŷйκξ涼ᳬ120ŷ
const int DRAW_MOVES = 100;     // ĬϵĺŷElephantEye趨50غϼ100Ӧ
const int REP_HASH_MASK = 4095; // жظûȣ4096

const int MATE_VALUE = 10000;           // ߷ֵķֵ
const int BAN_VALUE = MATE_VALUE - 100; // иķֵڸֵдû("hash.cpp")
const int WIN_VALUE = MATE_VALUE - 200; // ʤķֵޣֵ˵Ѿɱ
const int NULLOKAY_MARGIN = 200;        // ŲüԲֵ߽
const int NULLSAFE_MARGIN = 400;        // ʹÿŲüֵ߽
const int DRAW_VALUE = 20;              // ʱصķ(ȡֵ)

const bool CHECK_LAZY = true;   // ͵⽫
const int CHECK_MULTI = 48;     // ӽ

// ÿͱ
const int KING_TYPE = 0;
const int ADVISOR_TYPE = 1;
const int BISHOP_TYPE = 2;
const int KNIGHT_TYPE = 3;
const int ROOK_TYPE = 4;
const int CANNON_TYPE = 5;
const int PAWN_TYPE = 6;

// ÿĿʼźͽ
const int KING_FROM = 0;
const int ADVISOR_FROM = 1;
const int ADVISOR_TO = 2;
const int BISHOP_FROM = 3;
const int BISHOP_TO = 4;
const int KNIGHT_FROM = 5;
const int KNIGHT_TO = 6;
const int ROOK_FROM = 7;
const int ROOK_TO = 8;
const int CANNON_FROM = 9;
const int CANNON_TO = 10;
const int PAWN_FROM = 11;
const int PAWN_TO = 15;

// λ
const int KING_BITPIECE = 1 << KING_FROM;
const int ADVISOR_BITPIECE = (1 << ADVISOR_FROM) | (1 << ADVISOR_TO);
const int BISHOP_BITPIECE = (1 << BISHOP_FROM) | (1 << BISHOP_TO);
const int KNIGHT_BITPIECE = (1 << KNIGHT_FROM) | (1 << KNIGHT_TO);
const int ROOK_BITPIECE = (1 << ROOK_FROM) | (1 << ROOK_TO);
const int CANNON_BITPIECE = (1 << CANNON_FROM) | (1 << CANNON_TO);
const int PAWN_BITPIECE = (1 << PAWN_FROM) | (1 << (PAWN_FROM + 1)) |
    (1 << (PAWN_FROM + 2)) | (1 << (PAWN_FROM + 3)) | (1 << PAWN_TO);
const int ATTACK_BITPIECE = KNIGHT_BITPIECE | ROOK_BITPIECE | CANNON_BITPIECE | PAWN_BITPIECE;

inline uint32_t BIT_PIECE(int pc) {
  return 1 << (pc - 16);
}

inline uint32_t WHITE_BITPIECE(int nBitPiece) {
  return nBitPiece;
}

inline uint32_t BLACK_BITPIECE(int nBitPiece) {
  return nBitPiece << 16;
}

inline uint32_t BOTH_BITPIECE(int nBitPiece) {
  return nBitPiece + (nBitPiece << 16);
}

// "RepStatus()"صظλ
const int REP_NONE = 0;
const int REP_DRAW = 1;
const int REP_LOSS = 3;
const int REP_WIN = 5;

/* ElephantEyeĺܶжõ"SIDE_TAG()"췽Ϊ16ڷΪ32
 * "SIDE_TAG() + i"Էѡӵͣ"i"015ǣ
 * ˧ڱ(ʿʿ)
 * "i"ȡ"KNIGHT_FROM""KNIGHT_TO"ʾμλ
 */
inline int SIDE_TAG(int sd) {
  return 16 + (sd << 4);
}

inline int OPP_SIDE_TAG(int sd) {
  return 32 - (sd << 4);
}

inline int SIDE_VALUE(int sd, int vl) {
  return (sd == 0 ? vl : -vl);
}

inline int PIECE_INDEX(int pc) {
  return pc & 15;
}

extern const char *const cszStartFen;     // ʼFEN
extern const char *const cszPieceBytes;   // ͶӦӷ
extern const int cnPieceTypes[48];        // ŶӦ
extern const int cnSimpleValues[48];      // ӵļ򵥷ֵ
extern const uint8_t cucsqMirrorTab[256]; // ľ(ҶԳ)

inline char PIECE_BYTE(int pt) {
  return cszPieceBytes[pt];
}

inline int PIECE_TYPE(int pc) {
  return cnPieceTypes[pc];
}

inline int SIMPLE_VALUE(int pc) {
  return cnSimpleValues[pc];
}

inline uint8_t SQUARE_MIRROR(int sq) {
  return cucsqMirrorTab[sq];
}

// FENӱʶ
int FenPiece(int Arg);

// ŷṹ
union MoveStruct {
  uint32_t dwmv;           // ṹ
  struct {
    uint16_t wmv, wvl;     // ŷͷֵ
  };
  struct {
    uint8_t Src, Dst;      // ʼĿ
    int8_t CptDrw, ChkChs; // (+)/ŷ(-)(+)/׽(-)
  };
}; // mvs

// ŷṹ
inline int SRC(int mv) { // õŷ
  return mv & 255;
}

inline int DST(int mv) { // õŷյ
  return mv >> 8;
}

inline int MOVE(int sqSrc, int sqDst) {   // յõŷ
  return sqSrc + (sqDst << 8);
}

inline uint32_t MOVE_COORD(int mv) {      // ŷתַ
  union {
    char c[4];
    uint32_t dw;
  } Ret;
  Ret.c[0] = FILE_X(SRC(mv)) - FILE_LEFT + 'a';
  Ret.c[1] = '9' - RANK_Y(SRC(mv)) + RANK_TOP;
  Ret.c[2] = FILE_X(DST(mv)) - FILE_LEFT + 'a';
  Ret.c[3] = '9' - RANK_Y(DST(mv)) + RANK_TOP;
  // ŷĺ
  __ASSERT_BOUND('a', Ret.c[0], 'i');
  __ASSERT_BOUND('0', Ret.c[1], '9');
  __ASSERT_BOUND('a', Ret.c[2], 'i');
  __ASSERT_BOUND('0', Ret.c[3], '9');
  return Ret.dw;
}

inline int COORD_MOVE(uint32_t dwMoveStr) { // ַתŷ
  int sqSrc, sqDst;
  char *lpArgPtr;
  lpArgPtr = (char *) &dwMoveStr;
  sqSrc = COORD_XY(lpArgPtr[0] - 'a' + FILE_LEFT, '9' - lpArgPtr[1] + RANK_TOP);
  sqDst = COORD_XY(lpArgPtr[2] - 'a' + FILE_LEFT, '9' - lpArgPtr[3] + RANK_TOP);
  // ŷĺԲ
  // __ASSERT_SQUARE(sqSrc);
  // __ASSERT_SQUARE(sqDst);
  return (IN_BOARD(sqSrc) && IN_BOARD(sqDst) ? MOVE(sqSrc, sqDst) : 0);
}

inline int MOVE_MIRROR(int mv) {          // ŷ
  return MOVE(SQUARE_MIRROR(SRC(mv)), SQUARE_MIRROR(DST(mv)));
}

// عṹ
struct RollbackStruct {
  ZobristStruct zobr;   // Zobrist
  int vlWhite, vlBlack; // 췽ͺڷֵ
  MoveStruct mvs;       // ŷ
}; // rbs

const bool DEL_PIECE = true; // "PositionStruct::AddPiece()"ѡ

// ṹ
struct PositionStruct {
  // Ա
  int sdPlayer;             // ֵķߣ0ʾ췽1ʾڷ
  uint8_t ucpcSquares[256]; // ÿӷŵӣ0ʾû
  uint8_t ucsqPieces[48];   // ÿӷŵλã0ʾ
  ZobristStruct zobr;       // Zobrist

  // λṹԱǿ̵Ĵ
  union {
    uint32_t dwBitPiece;    // 32λλ031λαʾΪ1647Ƿ
    uint16_t wBitPiece[2];  // ֳ
  };
  uint16_t wBitRanks[16];   // λ飬ע÷"wBitRanks[RANK_Y(sq)]"
  uint16_t wBitFiles[16];   // λ飬ע÷"wBitFiles[FILE_X(sq)]"

  // 
  int vlWhite, vlBlack;   // 췽ͺڷֵ

  // عŷѭ
  int nMoveNum, nDistance;              // عŷ
  RollbackStruct rbsList[MAX_MOVE_NUM]; // عб
  uint8_t ucRepHash[REP_HASH_MASK + 1]; // жظû

  // ȡŷԤϢ
  SlideMoveStruct *RankMovePtr(int x, int y) const {
    return PreGen.smvRankMoveTab[x - FILE_LEFT] + wBitRanks[y];
  }
  SlideMoveStruct *FileMovePtr(int x, int y) const {
    return PreGen.smvFileMoveTab[y - RANK_TOP] + wBitFiles[x];
  }
  SlideMaskStruct *RankMaskPtr(int x, int y) const {
    return PreGen.smsRankMaskTab[x - FILE_LEFT] + wBitRanks[y];
  }
  SlideMaskStruct *FileMaskPtr(int x, int y) const {
    return PreGen.smsFileMaskTab[y - RANK_TOP] + wBitFiles[x];
  }

  // ̴
  void ClearBoard(void) { // ̳ʼ
    sdPlayer = 0;
    memset(ucpcSquares, 0, 256);
    memset(ucsqPieces, 0, 48);
    zobr.InitZero();
    dwBitPiece = 0;
    memset(wBitRanks, 0, 16 * sizeof(uint16_t));
    memset(wBitFiles, 0, 16 * sizeof(uint16_t));
    vlWhite = vlBlack = 0;
    // "ClearBoard()""SetIrrev()"ʼԱ
  }
  void ChangeSide(void) { // 巽
    sdPlayer = OPP_SIDE(sdPlayer);
    zobr.Xor(PreGen.zobrPlayer);
  }
  void SaveStatus(void) { // ״̬
    RollbackStruct *lprbs;
    lprbs = rbsList + nMoveNum;
    lprbs->zobr = zobr;
    lprbs->vlWhite = vlWhite;
    lprbs->vlBlack = vlBlack;
  }
  void Rollback(void) {   // ع
    RollbackStruct *lprbs;
    lprbs = rbsList + nMoveNum;
    zobr = lprbs->zobr;
    vlWhite = lprbs->vlWhite;
    vlBlack = lprbs->vlBlack;
  }
  void AddPiece(int sq, int pc, bool bDel = false); // 
  int MovePiece(int mv);                            // ƶ
  void UndoMovePiece(int mv, int pcCaptured);       // ƶ
  int Promote(int sq);                              // 
  void UndoPromote(int sq, int pcCaptured);         // 

  // ŷ
  bool MakeMove(int mv);   // ִһŷ
  void UndoMakeMove(void); // һŷ
  void NullMove(void);     // ִһ
  void UndoNullMove(void); // һ
  void SetIrrev(void) {    // Ѿɡ桱عŷ
    rbsList[0].mvs.dwmv = 0; // wmv, Chk, CptDrw, ChkChs = 0
    rbsList[0].mvs.ChkChs = CheckedBy();
    nMoveNum = 1;
    nDistance = 0;
    memset(ucRepHash, 0, REP_HASH_MASK + 1);
  }

  // 洦
  void FromFen(const char *szFen); // FENʶ
  void ToFen(char *szFen) const;   // FEN
  void Mirror(void);               // 澵

  // ŷ
  bool GoodCap(int mv) const {     // õĳŷ⣬ŷ¼ʷɱŷ
    int pcMoved, pcCaptured;
    pcCaptured = ucpcSquares[DST(mv)];
    if (pcCaptured == 0) {
      return false;
    }
    if (!Protected(OPP_SIDE(sdPlayer), DST(mv))) {
      return true;
    }
    pcMoved = ucpcSquares[SRC(mv)];
    return SIMPLE_VALUE(pcCaptured) > SIMPLE_VALUE(pcMoved);
  }
  bool LegalMove(int mv) const;            // ŷԼ⣬ڡɱŷļ
  int CheckedBy(bool bLazy = false) const; // ĸӽ
  bool IsMate(void);                       // жѱ
  MoveStruct LastMove(void) const {        // ǰһŷŷ˾Ľ״̬
    return rbsList[nMoveNum - 1].mvs;
  }
  bool CanPromote(void) const {            // жǷ
    return (wBitPiece[sdPlayer] & PAWN_BITPIECE) != PAWN_BITPIECE && LastMove().ChkChs <= 0;
  }
  bool NullOkay(void) const {              // ʹÿŲü
    return (sdPlayer == 0 ? vlWhite : vlBlack) > NULLOKAY_MARGIN;
  }
  bool NullSafe(void) const {              // ŲüԲ
    return (sdPlayer == 0 ? vlWhite : vlBlack) > NULLSAFE_MARGIN;
  }
  bool IsDraw(void) const {                // ж
    return (!PreEval.bPromotion && (dwBitPiece & BOTH_BITPIECE(ATTACK_BITPIECE)) == 0) ||
        -LastMove().CptDrw >= DRAW_MOVES || nMoveNum == MAX_MOVE_NUM;
  }
  int RepStatus(int nRecur = 1) const;     // ظ
  int DrawValue(void) const {              // ķֵ
    return (nDistance & 1) == 0 ? -DRAW_VALUE : DRAW_VALUE;
  }
  int RepValue(int vlRep) const {          // ظķֵ
    // return vlRep == REP_LOSS ? nDistance - MATE_VALUE : vlRep == REP_WIN ? MATE_VALUE - nDistance : DrawValue();
    // иķֵBAN_VALUEдû("hash.cpp")
    return vlRep == REP_LOSS ? nDistance - BAN_VALUE : vlRep == REP_WIN ? BAN_VALUE - nDistance : DrawValue();
  }
  int Material(void) const {               // ƽ⣬Ȩ
    return SIDE_VALUE(sdPlayer, vlWhite - vlBlack) + PreEval.vlAdvanced;
  }

  // ŷɹ̣Щ̴ر԰Ƕ"genmoves.cpp"
  bool Protected(int sd, int sqSrc, int sqExcept = 0) const; // ӱж
  int ChasedBy(int mv) const;                                // ׽ĸ
  int MvvLva(int sqDst, int pcCaptured, int nLva) const;     // MVV(LVA)ֵ
  int GenCapMoves(MoveStruct *lpmvs) const;                  // ŷ
  int GenNonCapMoves(MoveStruct *lpmvs) const;               // ŷ
  int GenAllMoves(MoveStruct *lpmvs) const {                 // ȫŷ
    int nCapNum;
    nCapNum = GenCapMoves(lpmvs);
    return nCapNum + GenNonCapMoves(lpmvs + nCapNum);
  }

  // ŷɹ̣Щ̴ر԰Ƕ"preeval.cpp""evaluate.cpp"
  void PreEvaluate(void);
  int AdvisorShape(void) const;
  int StringHold(void) const;
  int RookMobility(void) const;
  int KnightTrap(void) const;
  int Evaluate(int vlAlpha, int vlBeta) const;
}; // pos

#endif
// ===== END eleeye/position.h =====

// ===== BEGIN eleeye/hash.h =====
/*
hash.h/hash.cpp - Source Code for ElephantEye, Part V

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.31, Last Modified: May 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string.h>

#ifndef HASH_H
#define HASH_H

// û־ֻ"RecordHash()"
const int HASH_BETA = 1;
const int HASH_ALPHA = 2;
const int HASH_PV = HASH_ALPHA | HASH_BETA;

const int HASH_LAYERS = 2;   // ûĲ
const int NULL_DEPTH = 2;    // Ųü

// ûṹûϢZobristУм䣬Էֹȡͻ
struct HashStruct {
  uint32_t dwZobristLock0;           // ZobristУһ
  uint16_t wmv;                      // ŷ
  uint8_t ucAlphaDepth, ucBetaDepth; // (ϱ߽±߽)
  int16_t svlAlpha, svlBeta;         // ֵ(ϱ߽±߽)
  uint32_t dwZobristLock1;           // ZobristУڶ
}; // hsh

// ûϢ
extern int nHashMask;              // ûĴС
extern HashStruct *hshItems;       // ûָ룬ElephantEyeöû
#ifdef HASH_QUIESC
  extern HashStruct *hshItemsQ;
#endif

inline void ClearHash(void) {         // û
  memset(hshItems, 0, (nHashMask + 1) * sizeof(HashStruct));
#ifdef HASH_QUIESC
  memset(hshItemsQ, 0, (nHashMask + 1) * sizeof(HashStruct));
#endif
}

inline void NewHash(int nHashScale) { // ûС 2^nHashScale ֽ
  nHashMask = ((1 << nHashScale) / sizeof(HashStruct)) - 1;
  hshItems = new HashStruct[nHashMask + 1];
#ifdef HASH_QUIESC
  hshItemsQ = new HashStruct[nHashMask + 1];
#endif
  ClearHash();
}

inline void DelHash(void) {           // ͷû
  delete[] hshItems;
#ifdef HASH_QUIESC
  delete[] hshItemsQ;
#endif
}

// жûǷϾ(ZobristǷ)
inline bool HASH_POS_EQUAL(const HashStruct &hsh, const PositionStruct &pos) {
  return hsh.dwZobristLock0 == pos.zobr.dwLock0 && hsh.dwZobristLock1 == pos.zobr.dwLock1;
}

// Ͳȡû(һãԶ丳ֵ)
inline HashStruct &HASH_ITEM(const PositionStruct &pos, int nLayer) {
  return hshItems[(pos.zobr.dwKey + nLayer) & nHashMask];
}

// ûĹ
void RecordHash(const PositionStruct &pos, int nFlag, int vl, int nDepth, int mv);                    // 洢ûϢ
int ProbeHash(const PositionStruct &pos, int vlAlpha, int vlBeta, int nDepth, bool bNoNull, int &mv); // ȡûϢ
#ifdef HASH_QUIESC
  void RecordHashQ(const PositionStruct &pos, int vlBeta, int vlAlpha); // 洢ûϢ(̬)
  int ProbeHashQ(const PositionStruct &pos, int vlAlpha, int vlBeta);   // ȡûϢ(̬)
#endif

#ifndef CCHESS_A3800
  // UCCI֧ - HashеľϢ
  bool PopHash(const PositionStruct &pos);
#endif

#endif
// ===== END eleeye/hash.h =====

// ===== BEGIN eleeye/movesort.h =====
/*
movesort.h/movesort.cpp - Source Code for ElephantEye, Part VII

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.11, Last Modified: Dec. 2007
Copyright (C) 2004-2007 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MOVESORT_H
#define MOVESORT_H

#include <string.h>

const int LIMIT_DEPTH = 64;       // ļ
const int SORT_VALUE_MAX = 65535; // ŷֵ

extern const int FIBONACCI_LIST[32];

// "nHistory"ֻ"movesort.cpp"һģʹ
extern int nHistory[65536]; // ʷ

// ŷ˳ɽ׶("NextFull()")
const int PHASE_HASH = 0;
const int PHASE_GEN_CAP = 1;
const int PHASE_GOODCAP = 2;
const int PHASE_KILLER1 = 3;
const int PHASE_KILLER2 = 4;
const int PHASE_GEN_NONCAP = 5;
const int PHASE_REST = 6;

const bool NEXT_ALL = true;    // ŷ˳"MoveSortStruct::NextQuiesc()"ѡ
const bool ROOT_UNIQUE = true; // ŷ˳"MoveSortStruct::ResetRoot()"ѡ

// ŷнṹ
struct MoveSortStruct {
  int nPhase, nMoveIndex, nMoveNum;
  int mvHash, mvKiller1, mvKiller2;
  MoveStruct mvs[MAX_GEN_MOVES];

  void SetHistory(void); // ʷŷбֵ
  void ShellSort(void);  // ŷ
  // õĳŷ(ûŷʷɱŷ)
  bool GoodCap(const PositionStruct &pos, int mv) {
    return mv == 0 || nPhase == PHASE_GOODCAP || (nPhase < PHASE_GOODCAP && pos.GoodCap(mv));
  }

  // ̬ŷ˳
  void InitAll(const PositionStruct &pos) {
    nMoveIndex = 0;
    nMoveNum = pos.GenAllMoves(mvs);
    SetHistory();
    ShellSort();
  }
  void InitQuiesc(const PositionStruct &pos) {
    nMoveIndex = 0;
    nMoveNum = pos.GenCapMoves(mvs);
    ShellSort();
  }
  void InitQuiesc2(const PositionStruct &pos) {
    nMoveNum += pos.GenNonCapMoves(mvs);
    SetHistory();
    ShellSort();
  }
  int NextQuiesc(bool bNextAll = false) {
    if (nMoveIndex < nMoveNum && (bNextAll || mvs[nMoveIndex].wvl > 0)) {
      nMoveIndex ++;
      return mvs[nMoveIndex - 1].wmv;
    } else {
      return 0;
    }
  }

  // ȫŷ˳
  void InitFull(const PositionStruct &pos, int mv, const uint16_t *lpwmvKiller) {
    nPhase = PHASE_HASH;
    mvHash = mv;
    mvKiller1 = lpwmvKiller[0];
    mvKiller2 = lpwmvKiller[1];
  }
  int InitEvade(PositionStruct &pos, int mv, const uint16_t *lpwmvKiller);
  int NextFull(const PositionStruct &pos);

  // ŷ˳
  void InitRoot(const PositionStruct &pos, int nBanMoves, const uint16_t *lpwmvBanList);
  void ResetRoot(bool bUnique = false) {
    nMoveIndex = 0;
    ShellSort();
    nMoveIndex = (bUnique ? 1 : 0);
  }
  int NextRoot(void) {
    if (nMoveIndex < nMoveNum) {
      nMoveIndex ++;
      return mvs[nMoveIndex - 1].wmv;
    } else {
      return 0;
    }
  }
  void UpdateRoot(int mv);
};

// ʷ
inline void ClearHistory(void) {
  memset(nHistory, 0, sizeof(int[65536]));
}

// ɱŷ
inline void ClearKiller(uint16_t (*lpwmvKiller)[2]) {
  memset(lpwmvKiller, 0, LIMIT_DEPTH * sizeof(uint16_t[2]));
}

// ɱŷ
inline void CopyKiller(uint16_t (*lpwmvDst)[2], const uint16_t (*lpwmvSrc)[2]) {
  memcpy(lpwmvDst, lpwmvSrc, LIMIT_DEPTH * sizeof(uint16_t[2]));
}
     
/* ҵŷʱȡĴʩ
 *
 * ʷ¼ѡ
 * 1. ƽϵ(n^2)
 * 2. ָϵ(2^n)
 * 3. FibonacciУ
 * 4. ϼϣ磺n^2 + 2^nȵȡ
 * ElephantEyeʹͳƽϵ
 */
inline void SetBestMove(int mv, int nDepth, uint16_t *lpwmvKiller) {
  nHistory[mv] += SQR(nDepth);
  if (lpwmvKiller[0] != mv) {
    lpwmvKiller[1] = lpwmvKiller[0];
    lpwmvKiller[0] = mv;
  }
}

#endif
// ===== END eleeye/movesort.h =====

// ===== BEGIN eleeye/search.h =====
/*
search.h/search.cpp - Source Code for ElephantEye, Part VIII

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.31, Last Modified: May 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef CCHESS_A3800
  #include "ucci.h"
#endif

#ifndef SEARCH_H
#define SEARCH_H

// ģʽ
const int GO_MODE_INFINITY = 0;
const int GO_MODE_NODES = 1;
const int GO_MODE_TIMER = 2;

// ǰõȫֱָ
struct SearchStruct {
  PositionStruct pos;                // дľ
  bool bQuit, bPonder, bDraw;        // Ƿյ˳ָ̨˼ģʽģʽ
  bool bBatch, bDebug;               // Ƿģʽ͵ģʽ
  bool bUseHash, bUseBook;           // ǷʹûüͿֿ
  bool bNullMove, bKnowledge;        // ǷŲüʹþ֪ʶ
  bool bIdle;                        // Ƿ
  RC4Struct rc4Random;               // 
  int nGoMode, nNodes, nCountMask;   // ģʽ
  int nProperTimer, nMaxTimer;       // ƻʹʱ
  int nRandomMask, nBanMoves;        // λͽ
  uint16_t wmvBanList[MAX_MOVE_NUM]; // б
  char szBookFile[1024];             // ֿ
#ifdef CCHESS_A3800
  int mvResult;                      // ŷ
#endif
};

extern SearchStruct Search;

#ifndef CCHESS_A3800

// UCCI湹
void BuildPos(PositionStruct &pos, const UcciCommStruct &UcciComm);

// UCCI֧ - ҶӽľϢ
void PopLeaf(PositionStruct &pos);

#endif

// 
void SearchMain(int nDepth);

#endif
// ===== END eleeye/search.h =====

// ===== BEGIN bot/Protocol.cpp =====

bool Protocol::parseMoveObject(const Json::Value& value, ProtocolMoveEntry& entry) {
    entry.valid = false;
    entry.move = Move();

    if (!value.isObject()) return false;
    if (!value.isMember("source") || !value.isMember("target")) return false;
    if (!value["source"].isString() || !value["target"].isString()) return false;

    std::string source = value["source"].asString();
    std::string target = value["target"].asString();

    if (source == "-1" || target == "-1") {
        entry.valid = false;
        return true;
    }

    if (source.size() != 2 || target.size() != 2) return false;

    entry.move = Move(source, target);
    entry.valid = true;
    return true;
}

bool Protocol::parseInput(const std::string& text, ParsedInput& out, std::string* errorMessage) {
    Json::Reader reader;
    Json::Value root;

    if (!reader.parse(text, root)) {
        if (errorMessage) *errorMessage = reader.getFormatedErrorMessages();
        return false;
    }

    out.requests.clear();
    out.responses.clear();
    out.data = root.isMember("data") ? root["data"] : Json::Value(Json::objectValue);

    if (root.isMember("requests") && root["requests"].isArray()) {
        for (Json::Value::UInt i = 0; i < root["requests"].size(); ++i) {
            ProtocolMoveEntry entry;
            if (!parseMoveObject(root["requests"][i], entry)) {
                if (errorMessage) *errorMessage = "invalid requests format";
                return false;
            }
            out.requests.push_back(entry);
        }
    }

    if (root.isMember("responses") && root["responses"].isArray()) {
        for (Json::Value::UInt i = 0; i < root["responses"].size(); ++i) {
            ProtocolMoveEntry entry;
            if (!parseMoveObject(root["responses"][i], entry)) {
                if (errorMessage) *errorMessage = "invalid responses format";
                return false;
            }
            out.responses.push_back(entry);
        }
    }

    return true;
}

std::string Protocol::posToString(int x, int y) {
    return std::string(1, pgnint2char(x)) + std::string(1, int2char(y));
}

Json::Value Protocol::makeOutput(const Move& move, const Json::Value& data) {
    Json::Value ret;
    if (move.isInvalid()) {
        ret["response"]["source"] = std::string("-1");
        ret["response"]["target"] = std::string("-1");
    } else {
        ret["response"]["source"] = posToString(move.source_x, move.source_y);
        ret["response"]["target"] = posToString(move.target_x, move.target_y);
    }
    ret["data"] = data;
    return ret;
}

std::string Protocol::writeJson(const Json::Value& root) {
    Json::FastWriter writer;
    return writer.write(root);
}
// ===== END bot/Protocol.cpp =====

// ===== BEGIN bot/EyeAdapter.cpp =====

namespace {
    constexpr int EYE_INTERRUPT_COUNT = 4096;
    bool g_eyeInited = false;
}

void EyeAdapter::ensureInit() {
    if (g_eyeInited) {
        return;
    }
    g_eyeInited = true;

    PreGenInit();
    NewHash(24);  // 16MB，沿用 eleeye.cpp 的默认配置

    Search.pos.FromFen(cszStartFen);
    Search.pos.nDistance = 0;
    Search.pos.PreEvaluate();

    Search.nBanMoves = 0;

    Search.bQuit = false;
    Search.bBatch = false;
    Search.bDebug = false;

    Search.bUseHash = true;
    Search.bUseBook = false;     // Botzone 下先不开外部 BOOK.DAT
    Search.bNullMove = true;
    Search.bKnowledge = true;

    Search.bIdle = false;
    Search.bPonder = false;
    Search.bDraw = false;

    Search.nCountMask = EYE_INTERRUPT_COUNT - 1;
    Search.nRandomMask = 0;
    Search.rc4Random.InitRand();

    Search.mvResult = 0;
}

int EyeAdapter::botzoneMoveToEye(const Move& mv) {
    if (mv.isInvalid()) return 0;

    if (mv.source_x < 0 || mv.source_x > 8 ||
        mv.source_y < 0 || mv.source_y > 9 ||
        mv.target_x < 0 || mv.target_x > 8 ||
        mv.target_y < 0 || mv.target_y > 9) {
        return 0;
    }

    const int sqSrc = COORD_XY(mv.source_x + FILE_LEFT, 9 - mv.source_y + RANK_TOP);
    const int sqDst = COORD_XY(mv.target_x + FILE_LEFT, 9 - mv.target_y + RANK_TOP);

    if (!IN_BOARD(sqSrc) || !IN_BOARD(sqDst)) {
        return 0;
    }

    return MOVE(sqSrc, sqDst);
}

Move EyeAdapter::eyeMoveToBotzone(int mv) {
    if (mv == 0) return Move();

    const int sx = FILE_X(SRC(mv)) - FILE_LEFT;
    const int sy = 9 - RANK_Y(SRC(mv)) + RANK_TOP;
    const int tx = FILE_X(DST(mv)) - FILE_LEFT;
    const int ty = 9 - RANK_Y(DST(mv)) + RANK_TOP;

    if (sx < 0 || sx > 8 || sy < 0 || sy > 9 ||
        tx < 0 || tx > 8 || ty < 0 || ty > 9) {
        return Move();
    }

    return Move(sx, sy, tx, ty);
}

Move EyeAdapter::getBestMove(const std::vector<Move>& history) const {
    ensureInit();

    Search.pos.FromFen(cszStartFen);

    for (const Move& mvBot : history) {
        const int mvEye = botzoneMoveToEye(mvBot);
        if (mvEye == 0) {
            return Move();
        }

        if (!Search.pos.LegalMove(mvEye)) {
            return Move();
        }

        if (!Search.pos.MakeMove(mvEye)) {
            return Move();
        }

        if (Search.pos.LastMove().CptDrw > 0) {
            Search.pos.SetIrrev();
        }
    }

    Search.pos.nDistance = 0;
    Search.pos.PreEvaluate();

    Search.nBanMoves = 0;
    Search.bPonder = false;
    Search.bDraw = false;
    Search.mvResult = 0;

    Search.nGoMode = GO_MODE_TIMER;
    Search.nProperTimer = 700;
    Search.nMaxTimer = 950;

    SearchMain(64);

    if (Search.mvResult == 0) {
        return Move();
    }

    return eyeMoveToBotzone(Search.mvResult);
}
// ===== END bot/EyeAdapter.cpp =====

// ===== BEGIN eleeye/hash.cpp =====
/*
hash.h/hash.cpp - Source Code for ElephantEye, Part V

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.31, Last Modified: May 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef CCHESS_A3800
  #include <stdio.h>
#endif

int nHashMask;
HashStruct *hshItems;
#ifdef HASH_QUIESC
  HashStruct *hshItemsQ;
#endif

// 洢ûϢ
void RecordHash(const PositionStruct &pos, int nFlag, int vl, int nDepth, int mv) {
  HashStruct hsh;
  int i, nHashDepth, nMinDepth, nMinLayer;
  // 洢ûϢĹ̰¼裺

  // 1. Էֵɱ岽
  __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
  __ASSERT(mv == 0 || pos.LegalMove(mv));
  if (vl > WIN_VALUE) {
    if (mv == 0 && vl <= BAN_VALUE) {
      return; // ³ľ(ûü)ŷҲûУôûбҪдû
    }
    vl += pos.nDistance;
  } else if (vl < -WIN_VALUE) {
    if (mv == 0 && vl >= -BAN_VALUE) {
      return; // ͬ
    }
    vl -= pos.nDistance;
  } else if (vl == pos.DrawValue() && mv == 0) {
    return;   // ͬ
  }

  // 2. ̽û
  nMinDepth = 512;
  nMinLayer = 0;
  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i);

    // 3. ̽һľ棬ôûϢɣ
    if (HASH_POS_EQUAL(hsh, pos)) {
      // ȸ߽߱Сɸûֵ
      if ((nFlag & HASH_ALPHA) != 0 && (hsh.ucAlphaDepth <= nDepth || hsh.svlAlpha >= vl)) {
        hsh.ucAlphaDepth = nDepth;
        hsh.svlAlpha = vl;
      }
      // BetaҪע⣺ҪNull-MoveĽ㸲Ľ
      if ((nFlag & HASH_BETA) != 0 && (hsh.ucBetaDepth <= nDepth || hsh.svlBeta <= vl) && (mv != 0 || hsh.wmv == 0)) {
        hsh.ucBetaDepth = nDepth;
        hsh.svlBeta = vl;
      }
      // ŷʼոǵ
      if (mv != 0) {
        hsh.wmv = mv;
      }
      HASH_ITEM(pos, i) = hsh;
      return;
    }

    // 4. һľ棬ôСû
    nHashDepth = MAX((hsh.ucAlphaDepth == 0 ? 0 : hsh.ucAlphaDepth + 256),
        (hsh.wmv == 0 ? hsh.ucBetaDepth : hsh.ucBetaDepth + 256));
    __ASSERT(nHashDepth < 512);
    if (nHashDepth < nMinDepth) {
      nMinDepth = nHashDepth;
      nMinLayer = i;
    }
  }

  // 5. ¼û
  hsh.dwZobristLock0 = pos.zobr.dwLock0;
  hsh.dwZobristLock1 = pos.zobr.dwLock1;
  hsh.wmv = mv;
  hsh.ucAlphaDepth = hsh.ucBetaDepth = 0;
  hsh.svlAlpha = hsh.svlBeta = 0;
  if ((nFlag & HASH_ALPHA) != 0) {
    hsh.ucAlphaDepth = nDepth;
    hsh.svlAlpha = vl;
  }
  if ((nFlag & HASH_BETA) != 0) {
    hsh.ucBetaDepth = nDepth;
    hsh.svlBeta = vl;
  }
  HASH_ITEM(pos, nMinLayer) = hsh;
}

/* жϻȡûҪЩûķֵĸͬвͬĴ
 * һֵ"WIN_VALUE"("-WIN_VALUE""WIN_VALUE"֮䣬ͬ)ֻȡҪľ棻
 * ֵ"WIN_VALUE""BAN_VALUE"֮䣬ܻȡûеֵ(ֻܻȡŷο)ĿǷֹڳµġûĲȶԡ
 * ֵ"BAN_VALUE"⣬ȡʱؿҪΪЩѾ֤ɱˣ
 * ġֵ"DrawValue()"(ǵһ)ܻȡûеֵ(ԭڶͬ)
 * ע⣺ڵҪɱ岽е
 */
inline int ValueAdjust(const PositionStruct &pos, bool &bBanNode, bool &bMateNode, int vl) {
  bBanNode = bMateNode = false;
  if (vl > WIN_VALUE) {
    if (vl <= BAN_VALUE) {
      bBanNode = true;
    } else {
      bMateNode = true;
      vl -= pos.nDistance;
    }
  } else if (vl < -WIN_VALUE) {
    if (vl >= -BAN_VALUE) {
      bBanNode = true;
    } else {
      bMateNode = true;
      vl += pos.nDistance;
    }
  } else if (vl == pos.DrawValue()) {
    bBanNode = true;
  }
  return vl;
}

// һŷǷȶڼûĲȶ
inline bool MoveStable(PositionStruct &pos, int mv) {
  // жһŷǷȶǣ
  // 1. ûкŷٶȶģ
  if (mv == 0) {
    return true;
  }
  // 2. ŷȶģ
  __ASSERT(pos.LegalMove(mv));
  if (pos.ucpcSquares[DST(mv)] != 0) {
    return true;
  }
  // 3. û·Ǩƣʹ·߳"MAX_MOVE_NUM"ʱӦֹ·ߣٶȶġ
  if (!pos.MakeMove(mv)) {
    return true;
  }
  return false;
}

// ·Ƿȶ(ѭ·)ڼûĲȶ
static bool PosStable(const PositionStruct &pos, int mv) {
  HashStruct hsh;
  int i, nMoveNum;
  bool bStable;
  // pos·߱仯ջỹԭԱΪ"const""posMutable"е"const"Ľɫ
  PositionStruct &posMutable = (PositionStruct &) pos;

  __ASSERT(mv != 0);
  nMoveNum = 0;
  bStable = true;
  while (!MoveStable(posMutable, mv)) {
    nMoveNum ++; // "!MoveStable()"ѾִһŷԺҪ
    // ִŷѭôֹ·ߣȷϸ·߲ȶ
    if (posMutable.RepStatus() > 0) {
      bStable = false;
      break;
    }
    // ȡûͬ"ProbeHash()"
    for (i = 0; i < HASH_LAYERS; i ++) {
      hsh = HASH_ITEM(posMutable, i);
      if (HASH_POS_EQUAL(hsh, posMutable)) {
        break;
      }
    }
    mv = (i == HASH_LAYERS ? 0 : hsh.wmv);
  }
  // ǰִйŷ
  for (i = 0; i < nMoveNum; i ++) {
    posMutable.UndoMakeMove();
  }
  return bStable;
}

// ȡûϢ(ûʱ"-MATE_VALUE")
int ProbeHash(const PositionStruct &pos, int vlAlpha, int vlBeta, int nDepth, bool bNoNull, int &mv) {
  HashStruct hsh;
  int i, vl;
  bool bBanNode, bMateNode;
  // ȡûϢĹ̰¼裺

  // 1. ȡû
  mv = 0;
  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i);
    if (HASH_POS_EQUAL(hsh, pos)) {
      mv = hsh.wmv;
      __ASSERT(mv == 0 || pos.LegalMove(mv));
      break;
    }
  }
  if (i == HASH_LAYERS) {
    return -MATE_VALUE;
  }

  // 2. жǷBeta߽
  if (hsh.ucBetaDepth > 0) {
    vl = ValueAdjust(pos, bBanNode, bMateNode, hsh.svlBeta);
    if (!bBanNode && !(hsh.wmv == 0 && bNoNull) && (hsh.ucBetaDepth >= nDepth || bMateNode) && vl >= vlBeta) {
      __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
      if (hsh.wmv == 0 || PosStable(pos, hsh.wmv)) {
        return vl;
      }
    }
  }

  // 3. жǷAlpha߽
  if (hsh.ucAlphaDepth > 0) {
    vl = ValueAdjust(pos, bBanNode, bMateNode, hsh.svlAlpha);
    if (!bBanNode && (hsh.ucAlphaDepth >= nDepth || bMateNode) && vl <= vlAlpha) {
      __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
      if (hsh.wmv == 0 || PosStable(pos, hsh.wmv)) {
        return vl;
      }
    }
  }
  return -MATE_VALUE;
}

#ifdef HASH_QUIESC

// 洢ûϢ(̬)
void RecordHashQ(const PositionStruct &pos, int vlBeta, int vlAlpha) {
  volatile HashStruct *lphsh;
  __ASSERT((vlBeta > -WIN_VALUE && vlBeta < WIN_VALUE) || (vlAlpha > -WIN_VALUE && vlAlpha < WIN_VALUE));
  lphsh = hshItemsQ + (pos.zobr.dwKey & nHashMask);
  lphsh->dwZobristLock0 = pos.zobr.dwLock0;
  lphsh->svlAlpha = vlAlpha;
  lphsh->svlBeta = vlBeta;
  lphsh->dwZobristLock1 = pos.zobr.dwLock1;
}

// ȡûϢ(̬)
int ProbeHashQ(const PositionStruct &pos, int vlAlpha, int vlBeta) {
  volatile HashStruct *lphsh;
  int vlHashAlpha, vlHashBeta;

  lphsh = hshItemsQ + (pos.zobr.dwKey & nHashMask);
  if (lphsh->dwZobristLock0 == pos.zobr.dwLock0) {
    vlHashAlpha = lphsh->svlAlpha;
    vlHashBeta = lphsh->svlBeta;
    if (lphsh->dwZobristLock1 == pos.zobr.dwLock1) {
      if (vlHashBeta >= vlBeta) {
        __ASSERT(vlHashBeta > -WIN_VALUE && vlHashBeta < WIN_VALUE);
        return vlHashBeta;
      }
      if (vlHashAlpha <= vlAlpha) {
        __ASSERT(vlHashAlpha > -WIN_VALUE && vlHashAlpha < WIN_VALUE);
        return vlHashAlpha;
      }
    }
  }
  return -MATE_VALUE;
}

#endif

#ifndef CCHESS_A3800

// UCCI֧ - HashеľϢ
bool PopHash(const PositionStruct &pos) {
  HashStruct hsh;
  uint32_t dwMoveStr;
  int i;

  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i);
    if (HASH_POS_EQUAL(hsh, pos)) {
      printf("pophash");
      if (hsh.wmv != 0) {
        __ASSERT(pos.LegalMove(hsh.wmv));
        dwMoveStr = MOVE_COORD(hsh.wmv);
        printf(" bestmove %.4s", (const char *) &dwMoveStr);
      }
      if (hsh.ucBetaDepth > 0) {
        printf(" lowerbound %d depth %d", hsh.svlBeta, hsh.ucBetaDepth);
      }
      if (hsh.ucAlphaDepth > 0) {
        printf(" upperbound %d depth %d", hsh.svlAlpha, hsh.ucAlphaDepth);
      }
      printf("\n");
      fflush(stdout);
      return true;
    }
  }
  return false;
}

#endif
// ===== END eleeye/hash.cpp =====

// ===== BEGIN eleeye/pregen.cpp =====
/* 
pregen.h/pregen.cpp - Source Code for ElephantEye, Part II

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.0, Last Modified: Nov. 2007
Copyright (C) 2004-2007 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string.h>

const bool cbcInBoard[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const bool cbcInFort[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const bool cbcCanPromote[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const int8_t ccLegalSpanTab[512] = {
                       0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

const int8_t ccKnightPinTab[512] = {
                               0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,-16,  0,-16,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0, -1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0, -1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0, 16,  0, 16,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0
};

PreGenStruct PreGen;
PreEvalStruct PreEval;

// ĸжӵӷΪǣsqDst = sqSrc + cnKnightMoveTab[i]
static const int cnKingMoveTab[4]    = {-0x10, -0x01, +0x01, +0x10};
static const int cnAdvisorMoveTab[4] = {-0x11, -0x0f, +0x0f, +0x11};
static const int cnBishopMoveTab[4]  = {-0x22, -0x1e, +0x1e, +0x22};
static const int cnKnightMoveTab[8]  = {-0x21, -0x1f, -0x12, -0x0e, +0x0e, +0x12, +0x1f, +0x21};

void PreGenInit(void) {
  int i, j, k, n, sqSrc, sqDst;
  RC4Struct rc4;
  SlideMoveStruct smv;
  SlideMaskStruct sms;

  // ȳʼZobristֵ
  rc4.InitZero();
  PreGen.zobrPlayer.InitRC4(rc4);
  for (i = 0; i < 14; i ++) {
    for (j = 0; j < 256; j ++) {
      PreGen.zobrTable[i][j].InitRC4(rc4);
    }
  }

  // Ȼʼλкλ
  // עλкλвλԾͻƵʹ"+/- RANK_TOP/FILE_LEFT"
  for (sqSrc = 0; sqSrc < 256; sqSrc ++) {
    if (IN_BOARD(sqSrc)) {
      PreGen.wBitRankMask[sqSrc] = 1 << (FILE_X(sqSrc) - FILE_LEFT);
      PreGen.wBitFileMask[sqSrc] = 1 << (RANK_Y(sqSrc) - RANK_TOP);
    } else {
      PreGen.wBitRankMask[sqSrc] = 0;
      PreGen.wBitFileMask[sqSrc] = 0;
    }
  }

  // ȻɳںԤ(Ӧò"pregen.h")
  for (i = 0; i < 9; i ++) {
    for (j = 0; j < 512; j ++) {
      // ʼڡλСĳڵŷԤ飬¼裺
      // 1. ʼʱ"SlideMoveTab"ûŷʼ
      smv.ucNonCap[0] = smv.ucNonCap[1] = smv.ucRookCap[0] = smv.ucRookCap[1] =
      smv.ucCannonCap[0] = smv.ucCannonCap[1] = smv.ucSuperCap[0] = smv.ucSuperCap[1] = i + FILE_LEFT;
      sms.wNonCap = sms.wRookCap = sms.wCannonCap = sms.wSuperCap = 0;
      // ʾ"pregen.h"...[0]ʾһƶƶ[0]֮Ȼ
      // 2. ƶĿ...[0]
      for (k = i + 1; k <= 8; k ++) {
        if ((j & (1 << k)) != 0) {
          smv.ucRookCap[0] = FILE_DISP(k + FILE_LEFT);
          sms.wRookCap |= 1 << k;
          break;
        }
        smv.ucNonCap[0] = FILE_DISP(k + FILE_LEFT);
        sms.wNonCap |= 1 << k;
      }
      for (k ++; k <= 8; k ++) {
        if ((j & (1 << k)) != 0) {
          smv.ucCannonCap[0] = FILE_DISP(k + FILE_LEFT);
          sms.wCannonCap |= 1 << k;
          break;
        }
      }
      for (k ++; k <= 8; k ++) {
        if ((j & (1 << k)) != 0) {
          smv.ucSuperCap[0] = FILE_DISP(k + FILE_LEFT);
          sms.wSuperCap |= 1 << k;
          break;
        }
      }
      // 3. ƶĿ...[1]
      for (k = i - 1; k >= 0; k --) {
        if ((j & (1 << k)) != 0) {
          smv.ucRookCap[1] = FILE_DISP(k + FILE_LEFT);
          sms.wRookCap |= 1 << k;
          break;
        }
        smv.ucNonCap[1] = FILE_DISP(k + FILE_LEFT);
        sms.wNonCap |= 1 << k;
      }
      for (k --; k >= 0; k --) {
        if ((j & (1 << k)) != 0) {
          smv.ucCannonCap[1] = FILE_DISP(k + FILE_LEFT);
          sms.wCannonCap |= 1 << k;
          break;
        }
      }
      for (k --; k >= 0; k --) {
        if ((j & (1 << k)) != 0) {
          smv.ucSuperCap[1] = FILE_DISP(k + FILE_LEFT);
          sms.wSuperCap |= 1 << k;
          break;
        }
      }
      // 4. Ϊ"smv""sms"ֵ
      __ASSERT_BOUND_2(3, smv.ucNonCap[1], smv.ucNonCap[0], 11);
      __ASSERT_BOUND_2(3, smv.ucRookCap[1], smv.ucRookCap[0], 11);
      __ASSERT_BOUND_2(3, smv.ucCannonCap[1], smv.ucCannonCap[0], 11);
      __ASSERT_BOUND_2(3, smv.ucSuperCap[1], smv.ucSuperCap[0], 11);
      __ASSERT_BITRANK(sms.wNonCap);
      __ASSERT_BITRANK(sms.wRookCap);
      __ASSERT_BITRANK(sms.wCannonCap);
      __ASSERT_BITRANK(sms.wSuperCap);
      // 5. ʱ"smv""sms"ŷԤ
      PreGen.smvRankMoveTab[i][j] = smv;
      PreGen.smsRankMaskTab[i][j] = sms;
    }
  }

  // ȻɳԤ(Ӧò"pregen.h")
  for (i = 0; i < 10; i ++) {
    for (j = 0; j < 1024; j ++) {
      // ʼڡλСĳڵŷԤ飬¼裺
      // 1. ʼʱ"smv"ûŷʼ
      smv.ucNonCap[0] = smv.ucNonCap[1] = smv.ucRookCap[0] = smv.ucRookCap[1] =
      smv.ucCannonCap[0] = smv.ucCannonCap[1] = smv.ucSuperCap[0] = smv.ucSuperCap[1] = (i + RANK_TOP) * 16;
      sms.wNonCap = sms.wRookCap = sms.wCannonCap = sms.wSuperCap = 0;
      // 2. ƶĿ...[0]
      for (k = i + 1; k <= 9; k ++) {
        if ((j & (1 << k)) != 0) {
          smv.ucRookCap[0] = RANK_DISP(k + RANK_TOP);
          sms.wRookCap |= 1 << k;
          break;
        }
        smv.ucNonCap[0] = RANK_DISP(k + RANK_TOP);
        sms.wNonCap |= 1 << k;
      }
      for (k ++; k <= 9; k ++) {
        if ((j & (1 << k)) != 0) {
          smv.ucCannonCap[0] = RANK_DISP(k + RANK_TOP);
          sms.wCannonCap |= 1 << k;
          break;
        }
      }
      for (k ++; k <= 9; k ++) {
        if ((j & (1 << k)) != 0) {
          smv.ucSuperCap[0] = RANK_DISP(k + RANK_TOP);
          sms.wSuperCap |= 1 << k;
          break;
        }
      }
      // 3. ƶĿ...[1]
      for (k = i - 1; k >= 0; k --) {
        if ((j & (1 << k)) != 0) {
          smv.ucRookCap[1] = RANK_DISP(k + RANK_TOP);
          sms.wRookCap |= 1 << k;
          break;
        }
        smv.ucNonCap[1] = RANK_DISP(k + RANK_TOP);
        sms.wNonCap |= 1 << k;
      }
      for (k --; k >= 0; k --) {
        if ((j & (1 << k)) != 0) {
          smv.ucCannonCap[1] = RANK_DISP(k + RANK_TOP);
          sms.wCannonCap |= 1 << k;
          break;
        }
      }
      for (k --; k >= 0; k --) {
        if ((j & (1 << k)) != 0) {
          smv.ucSuperCap[1] = RANK_DISP(k + RANK_TOP);
          sms.wSuperCap |= 1 << k;
          break;
        }
      }
      // 4. Ϊ"smv""sms"ֵ
      __ASSERT_BOUND_2(3, smv.ucNonCap[1] >> 4, smv.ucNonCap[0] >> 4, 12);
      __ASSERT_BOUND_2(3, smv.ucRookCap[1] >> 4, smv.ucRookCap[0] >> 4, 12);
      __ASSERT_BOUND_2(3, smv.ucCannonCap[1] >> 4, smv.ucCannonCap[0] >> 4, 12);
      __ASSERT_BOUND_2(3, smv.ucSuperCap[1] >> 4, smv.ucSuperCap[0] >> 4, 12);
      __ASSERT_BITFILE(sms.wNonCap);
      __ASSERT_BITFILE(sms.wRookCap);
      __ASSERT_BITFILE(sms.wCannonCap);
      __ASSERT_BITFILE(sms.wSuperCap);
      // 5. ʱ"smv""sms"ŷԤ
      PreGen.smvFileMoveTab[i][j] = smv;
      PreGen.smsFileMaskTab[i][j] = sms;
    }
  }

  // ŷԤ飬ͬԤ
  for (sqSrc = 0; sqSrc < 256; sqSrc ++) {
    if (IN_BOARD(sqSrc)) {
      // ˧()ŷԤ
      n = 0;
      for (i = 0; i < 4; i ++) {
        sqDst = sqSrc + cnKingMoveTab[i];
        if (IN_FORT(sqDst)) {
          PreGen.ucsqKingMoves[sqSrc][n] = sqDst;
          n ++;
        }
      }
      __ASSERT(n <= 4);
      PreGen.ucsqKingMoves[sqSrc][n] = 0;
      // (ʿ)ŷԤ
      n = 0;
      for (i = 0; i < 4; i ++) {
        sqDst = sqSrc + cnAdvisorMoveTab[i];
        if (IN_FORT(sqDst)) {
          PreGen.ucsqAdvisorMoves[sqSrc][n] = sqDst;
          n ++;
        }
      }
      __ASSERT(n <= 4);
      PreGen.ucsqAdvisorMoves[sqSrc][n] = 0;
      // ()ŷԤ飬
      n = 0;
      for (i = 0; i < 4; i ++) {
        sqDst = sqSrc + cnBishopMoveTab[i];
        if (IN_BOARD(sqDst) && SAME_HALF(sqSrc, sqDst)) {
          PreGen.ucsqBishopMoves[sqSrc][n] = sqDst;
          PreGen.ucsqBishopPins[sqSrc][n] = BISHOP_PIN(sqSrc, sqDst);
          n ++;
        }
      }
      __ASSERT(n <= 4);
      PreGen.ucsqBishopMoves[sqSrc][n] = 0;
      // ŷԤ飬
      n = 0;
      for (i = 0; i < 8; i ++) {
        sqDst = sqSrc + cnKnightMoveTab[i];
        if (IN_BOARD(sqDst)) {
          PreGen.ucsqKnightMoves[sqSrc][n] = sqDst;
          PreGen.ucsqKnightPins[sqSrc][n] = KNIGHT_PIN(sqSrc, sqDst);
          n ++;
        }
      }
      __ASSERT(n <= 8);
      PreGen.ucsqKnightMoves[sqSrc][n] = 0;
      // ɱ()ŷԤ
      for (i = 0; i < 2; i ++) {
        n = 0;
        sqDst = SQUARE_FORWARD(sqSrc, i);
        sqDst = sqSrc + (i == 0 ? -16 : 16);
        if (IN_BOARD(sqDst)) {
          PreGen.ucsqPawnMoves[i][sqSrc][n] = sqDst;
          n ++;
        }
        if (AWAY_HALF(sqSrc, i)) {
          for (j = -1; j <= 1; j += 2) {
            sqDst = sqSrc + j;
            if (IN_BOARD(sqDst)) {
              PreGen.ucsqPawnMoves[i][sqSrc][n] = sqDst;
              n ++;
            }
          }
        }
        __ASSERT(n <= 3);
        PreGen.ucsqPawnMoves[i][sqSrc][n] = 0;
      }
    }
  }

  // վԤ۽ṹ
  memset(&PreEval, 0, sizeof(PreEvalStruct));
  PreEval.bPromotion = false; // ȱʡǲ
}
// ===== END eleeye/pregen.cpp =====

// ===== BEGIN eleeye/preeval.cpp =====
/*
preeval.h/preeval.cpp - Source Code for ElephantEye, Part X

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.3, Last Modified: Mar. 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


/* ElephantEyeԴʹõǺԼ
 *
 * sq: (0255"pregen.cpp")
 * pc: (047"position.cpp")
 * pt: (06"position.cpp")
 * mv: ŷ(065535"position.cpp")
 * sd: ӷ(0췽1ڷ)
 * vl: ֵ("-MATE_VALUE""MATE_VALUE""position.cpp")
 * (עǺſucdwȴļǺʹ)
 * pos: (PositionStructͣ"position.h")
 * sms: λкλеŷԤýṹ("pregen.h")
 * smv: λкλеŷжԤýṹ("pregen.h")
 */

/* λüֵ
 * ElephantEyeλüֵԾ۵ĵ˺ܴãڲա񵰡Ļϣ¸Ķ
 * 1. ֺλطһڿ㣻
 * 2. һ·ı()Ѳλ÷ֵ5֣ԼäĿ߱()
 * 3. ӱ()(߳)10֣Լٹӱ()äĿ(ʿ)()
 * 4. һ·ںᳵλ÷ֵ5֣Լ(ʿ)ʱĺᳵ
 */

// 1. о֡н˧()ͱ()ա񵰡
static const uint8_t cucvlKingPawnMidgameAttacking[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  9,  9,  9, 11, 13, 11,  9,  9,  9,  0,  0,  0,  0,
  0,  0,  0, 39, 49, 69, 84, 89, 84, 69, 49, 39,  0,  0,  0,  0,
  0,  0,  0, 39, 49, 64, 74, 74, 74, 64, 49, 39,  0,  0,  0,  0,
  0,  0,  0, 39, 46, 54, 59, 61, 59, 54, 46, 39,  0,  0,  0,  0,
  0,  0,  0, 29, 37, 41, 54, 59, 54, 41, 37, 29,  0,  0,  0,  0,
  0,  0,  0,  7,  0, 13,  0, 16,  0, 13,  0,  7,  0,  0,  0,  0,
  0,  0,  0,  7,  0,  7,  0, 15,  0,  7,  0,  7,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0, 11, 15, 11,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 2. о֡ûн˧()ͱ()
static const uint8_t cucvlKingPawnMidgameAttackless[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  9,  9,  9, 11, 13, 11,  9,  9,  9,  0,  0,  0,  0,
  0,  0,  0, 19, 24, 34, 42, 44, 42, 34, 24, 19,  0,  0,  0,  0,
  0,  0,  0, 19, 24, 32, 37, 37, 37, 32, 24, 19,  0,  0,  0,  0,
  0,  0,  0, 19, 23, 27, 29, 30, 29, 27, 23, 19,  0,  0,  0,  0,
  0,  0,  0, 14, 18, 20, 27, 29, 27, 20, 18, 14,  0,  0,  0,  0,
  0,  0,  0,  7,  0, 13,  0, 16,  0, 13,  0,  7,  0,  0,  0,  0,
  0,  0,  0,  7,  0,  7,  0, 15,  0,  7,  0,  7,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0, 11, 15, 11,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 3. о֡н˧()ͱ()
static const uint8_t cucvlKingPawnEndgameAttacking[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 10, 10, 10, 15, 15, 15, 10, 10, 10,  0,  0,  0,  0,
  0,  0,  0, 50, 55, 60, 85,100, 85, 60, 55, 50,  0,  0,  0,  0,
  0,  0,  0, 65, 70, 70, 75, 75, 75, 70, 70, 65,  0,  0,  0,  0,
  0,  0,  0, 75, 80, 80, 80, 80, 80, 80, 80, 75,  0,  0,  0,  0,
  0,  0,  0, 70, 70, 65, 70, 70, 70, 65, 70, 70,  0,  0,  0,  0,
  0,  0,  0, 45,  0, 40, 45, 45, 45, 40,  0, 45,  0,  0,  0,  0,
  0,  0,  0, 40,  0, 35, 40, 40, 40, 35,  0, 40,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  5,  5, 15,  5,  5,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  3,  3, 13,  3,  3,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  1,  1, 11,  1,  1,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 4. о֡ûн˧()ͱ()
static const uint8_t cucvlKingPawnEndgameAttackless[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 10, 10, 10, 15, 15, 15, 10, 10, 10,  0,  0,  0,  0,
  0,  0,  0, 10, 15, 20, 45, 60, 45, 20, 15, 10,  0,  0,  0,  0,
  0,  0,  0, 25, 30, 30, 35, 35, 35, 30, 30, 25,  0,  0,  0,  0,
  0,  0,  0, 35, 40, 40, 45, 45, 45, 40, 40, 35,  0,  0,  0,  0,
  0,  0,  0, 25, 30, 30, 35, 35, 35, 30, 30, 25,  0,  0,  0,  0,
  0,  0,  0, 25,  0, 25, 25, 25, 25, 25,  0, 25,  0,  0,  0,  0,
  0,  0,  0, 20,  0, 20, 20, 20, 20, 20,  0, 20,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  5,  5, 13,  5,  5,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  3,  3, 12,  3,  3,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  1,  1, 11,  1,  1,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 5. ûв(ʿ)()
static const uint8_t cucvlAdvisorBishopThreatless[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0, 20,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 18,  0,  0, 20, 23, 20,  0,  0, 18,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0, 23,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0, 20, 20,  0, 20, 20,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 5'. ģûв(ʿ)()
static const uint8_t cucvlAdvisorBishopPromotionThreatless[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0, 30,  0,  0,  0, 30,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 28,  0,  0, 30, 33, 30,  0,  0, 28,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0, 33,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0, 30, 30,  0, 30, 30,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 6. ܵв(ʿ)()ա񵰡
static const uint8_t cucvlAdvisorBishopThreatened[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0, 40,  0,  0,  0, 40,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 38,  0,  0, 40, 43, 40,  0,  0, 38,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0, 43,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0, 40, 40,  0, 40, 40,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 7. оֵա񵰡
static const uint8_t cucvlKnightMidgame[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 90, 90, 90, 96, 90, 96, 90, 90, 90,  0,  0,  0,  0,
  0,  0,  0, 90, 96,103, 97, 94, 97,103, 96, 90,  0,  0,  0,  0,
  0,  0,  0, 92, 98, 99,103, 99,103, 99, 98, 92,  0,  0,  0,  0,
  0,  0,  0, 93,108,100,107,100,107,100,108, 93,  0,  0,  0,  0,
  0,  0,  0, 90,100, 99,103,104,103, 99,100, 90,  0,  0,  0,  0,
  0,  0,  0, 90, 98,101,102,103,102,101, 98, 90,  0,  0,  0,  0,
  0,  0,  0, 92, 94, 98, 95, 98, 95, 98, 94, 92,  0,  0,  0,  0,
  0,  0,  0, 93, 92, 94, 95, 92, 95, 94, 92, 93,  0,  0,  0,  0,
  0,  0,  0, 85, 90, 92, 93, 78, 93, 92, 90, 85,  0,  0,  0,  0,
  0,  0,  0, 88, 85, 90, 88, 90, 88, 90, 85, 88,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 8. оֵ
static const uint8_t cucvlKnightEndgame[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 92, 94, 96, 96, 96, 96, 96, 94, 92,  0,  0,  0,  0,
  0,  0,  0, 94, 96, 98, 98, 98, 98, 98, 96, 94,  0,  0,  0,  0,
  0,  0,  0, 96, 98,100,100,100,100,100, 98, 96,  0,  0,  0,  0,
  0,  0,  0, 96, 98,100,100,100,100,100, 98, 96,  0,  0,  0,  0,
  0,  0,  0, 96, 98,100,100,100,100,100, 98, 96,  0,  0,  0,  0,
  0,  0,  0, 94, 96, 98, 98, 98, 98, 98, 96, 94,  0,  0,  0,  0,
  0,  0,  0, 94, 96, 98, 98, 98, 98, 98, 96, 94,  0,  0,  0,  0,
  0,  0,  0, 92, 94, 96, 96, 96, 96, 96, 94, 92,  0,  0,  0,  0,
  0,  0,  0, 90, 92, 94, 92, 92, 92, 94, 92, 90,  0,  0,  0,  0,
  0,  0,  0, 88, 90, 92, 90, 90, 90, 92, 90, 88,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 9. оֵĳա񵰡
static const uint8_t cucvlRookMidgame[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,206,208,207,213,214,213,207,208,206,  0,  0,  0,  0,
  0,  0,  0,206,212,209,216,233,216,209,212,206,  0,  0,  0,  0,
  0,  0,  0,206,208,207,214,216,214,207,208,206,  0,  0,  0,  0,
  0,  0,  0,206,213,213,216,216,216,213,213,206,  0,  0,  0,  0,
  0,  0,  0,208,211,211,214,215,214,211,211,208,  0,  0,  0,  0,
  0,  0,  0,208,212,212,214,215,214,212,212,208,  0,  0,  0,  0,
  0,  0,  0,204,209,204,212,214,212,204,209,204,  0,  0,  0,  0,
  0,  0,  0,198,208,204,212,212,212,204,208,198,  0,  0,  0,  0,
  0,  0,  0,200,208,206,212,200,212,206,208,200,  0,  0,  0,  0,
  0,  0,  0,194,206,204,212,200,212,204,206,194,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 10. оֵĳ
static const uint8_t cucvlRookEndgame[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,182,182,182,184,186,184,182,182,182,  0,  0,  0,  0,
  0,  0,  0,184,184,184,186,190,186,184,184,184,  0,  0,  0,  0,
  0,  0,  0,182,182,182,184,186,184,182,182,182,  0,  0,  0,  0,
  0,  0,  0,180,180,180,182,184,182,180,180,180,  0,  0,  0,  0,
  0,  0,  0,180,180,180,182,184,182,180,180,180,  0,  0,  0,  0,
  0,  0,  0,180,180,180,182,184,182,180,180,180,  0,  0,  0,  0,
  0,  0,  0,180,180,180,182,184,182,180,180,180,  0,  0,  0,  0,
  0,  0,  0,180,180,180,182,184,182,180,180,180,  0,  0,  0,  0,
  0,  0,  0,180,180,180,182,184,182,180,180,180,  0,  0,  0,  0,
  0,  0,  0,180,180,180,182,184,182,180,180,180,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 11. оֵڣա񵰡
static const uint8_t cucvlCannonMidgame[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,100,100, 96, 91, 90, 91, 96,100,100,  0,  0,  0,  0,
  0,  0,  0, 98, 98, 96, 92, 89, 92, 96, 98, 98,  0,  0,  0,  0,
  0,  0,  0, 97, 97, 96, 91, 92, 91, 96, 97, 97,  0,  0,  0,  0,
  0,  0,  0, 96, 99, 99, 98,100, 98, 99, 99, 96,  0,  0,  0,  0,
  0,  0,  0, 96, 96, 96, 96,100, 96, 96, 96, 96,  0,  0,  0,  0,
  0,  0,  0, 95, 96, 99, 96,100, 96, 99, 96, 95,  0,  0,  0,  0,
  0,  0,  0, 96, 96, 96, 96, 96, 96, 96, 96, 96,  0,  0,  0,  0,
  0,  0,  0, 97, 96,100, 99,101, 99,100, 96, 97,  0,  0,  0,  0,
  0,  0,  0, 96, 97, 98, 98, 98, 98, 98, 97, 96,  0,  0,  0,  0,
  0,  0,  0, 96, 96, 97, 99, 99, 99, 97, 96, 96,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// 12. оֵ
static const uint8_t cucvlCannonEndgame[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,100,100,100,100,100,100,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,100,100,100,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,100,100,100,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,102,104,102,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,102,104,102,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,102,104,102,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,102,104,102,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,102,104,102,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,104,106,104,100,100,100,  0,  0,  0,  0,
  0,  0,  0,100,100,100,104,106,104,100,100,100,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// ͷڵвֵָǶԺ췽˵к(ڷҪ15ȥ)ϿͷλԽвԽ󡣽оʱֵҪӦ١
static const int cvlHollowThreat[16] = {
   0,  0,  0,  0,  0,  0, 60, 65, 70, 75, 80, 80, 80,  0,  0,  0
};

// вֵָͬϣϸ߶ԽвԽûʱȡķ֮һоʱȡֵƺӦ仯
static const int cvlCentralThreat[16] = {
   0,  0,  0,  0,  0,  0, 50, 45, 40, 35, 30, 30, 30,  0,  0,  0
};

// ڵвֵָкţԽвԽвʱֵҪӦ١
static const int cvlBottomThreat[16] = {
   0,  0,  0, 40, 30,  0,  0,  0,  0,  0, 30, 40,  0,  0,  0,  0
};

// ģֻ漰"PositionStruct"е"ucsqPieces""dwBitPiece/wBitPiece""vlWhite""vlBlack"ĸԱʡǰ"this->"

/* Ԥ۾ǳʼԤ(PreEvalPreEvalEx)Ĺ̡
 * ElephantEyeľԤҪ棺
 * 1. жϾƴڿоֻǲоֽ׶Σ
 * 2. жÿһǷԶԷγв
 */

const int ROOK_MIDGAME_VALUE = 6;
const int KNIGHT_CANNON_MIDGAME_VALUE = 3;
const int OTHER_MIDGAME_VALUE = 1;
const int TOTAL_MIDGAME_VALUE = ROOK_MIDGAME_VALUE * 4 + KNIGHT_CANNON_MIDGAME_VALUE * 8 + OTHER_MIDGAME_VALUE * 18;
const int TOTAL_ADVANCED_VALUE = 4;
const int TOTAL_ATTACK_VALUE = 8;
const int ADVISOR_BISHOP_ATTACKLESS_VALUE = 80;
const int TOTAL_ADVISOR_LEAKAGE = 80;

static bool bInit = false;

PreEvalStructEx PreEvalEx;

void PositionStruct::PreEvaluate(void) {
  int i, sq, nMidgameValue, nWhiteAttacks, nBlackAttacks, nWhiteSimpleValue, nBlackSimpleValue;
  uint8_t ucvlPawnPiecesAttacking[256], ucvlPawnPiecesAttackless[256];

  if (!bInit) {
    bInit = true;
    // ʼ"PreEvalEx.cPopCnt16"飬ֻҪʼһ
    for (i = 0; i < 65536; i ++) {
      PreEvalEx.cPopCnt16[i] = PopCnt16(i);
    }
  }

  // жϾƴڿоֻǲоֽ׶ΣǼӵճ=6=3=1ӡ
  nMidgameValue = PopCnt32(this->dwBitPiece & BOTH_BITPIECE(ADVISOR_BITPIECE | BISHOP_BITPIECE | PAWN_BITPIECE)) * OTHER_MIDGAME_VALUE;
  nMidgameValue += PopCnt32(this->dwBitPiece & BOTH_BITPIECE(KNIGHT_BITPIECE | CANNON_BITPIECE)) * KNIGHT_CANNON_MIDGAME_VALUE;
  nMidgameValue += PopCnt32(this->dwBitPiece & BOTH_BITPIECE(ROOK_BITPIECE)) * ROOK_MIDGAME_VALUE;
  // ʹöκʱΪӽо
  nMidgameValue = (2 * TOTAL_MIDGAME_VALUE - nMidgameValue) * nMidgameValue / TOTAL_MIDGAME_VALUE;
  __ASSERT_BOUND(0, nMidgameValue, TOTAL_MIDGAME_VALUE);
  PreEval.vlAdvanced = (TOTAL_ADVANCED_VALUE * nMidgameValue + TOTAL_ADVANCED_VALUE / 2) / TOTAL_MIDGAME_VALUE;
  __ASSERT_BOUND(0, PreEval.vlAdvanced, TOTAL_ADVANCED_VALUE);
  for (sq = 0; sq < 256; sq ++) {
    if (IN_BOARD(sq)) {
      PreEval.ucvlWhitePieces[0][sq] = PreEval.ucvlBlackPieces[0][SQUARE_FLIP(sq)] = (uint8_t)
          ((cucvlKingPawnMidgameAttacking[sq] * nMidgameValue + cucvlKingPawnEndgameAttacking[sq] * (TOTAL_MIDGAME_VALUE - nMidgameValue)) / TOTAL_MIDGAME_VALUE);
      PreEval.ucvlWhitePieces[3][sq] = PreEval.ucvlBlackPieces[3][SQUARE_FLIP(sq)] = (uint8_t)
          ((cucvlKnightMidgame[sq] * nMidgameValue + cucvlKnightEndgame[sq] * (TOTAL_MIDGAME_VALUE - nMidgameValue)) / TOTAL_MIDGAME_VALUE);
      PreEval.ucvlWhitePieces[4][sq] = PreEval.ucvlBlackPieces[4][SQUARE_FLIP(sq)] = (uint8_t)
          ((cucvlRookMidgame[sq] * nMidgameValue + cucvlRookEndgame[sq] * (TOTAL_MIDGAME_VALUE - nMidgameValue)) / TOTAL_MIDGAME_VALUE);
      PreEval.ucvlWhitePieces[5][sq] = PreEval.ucvlBlackPieces[5][SQUARE_FLIP(sq)] = (uint8_t)
          ((cucvlCannonMidgame[sq] * nMidgameValue + cucvlCannonEndgame[sq] * (TOTAL_MIDGAME_VALUE - nMidgameValue)) / TOTAL_MIDGAME_VALUE);
      ucvlPawnPiecesAttacking[sq] = PreEval.ucvlWhitePieces[0][sq];
      ucvlPawnPiecesAttackless[sq] = (uint8_t)
          ((cucvlKingPawnMidgameAttackless[sq] * nMidgameValue + cucvlKingPawnEndgameAttackless[sq] * (TOTAL_MIDGAME_VALUE - nMidgameValue)) / TOTAL_MIDGAME_VALUE);
    }
  }
  for (i = 0; i < 16; i ++) {
    PreEvalEx.vlHollowThreat[i] = cvlHollowThreat[i] * (nMidgameValue + TOTAL_MIDGAME_VALUE) / (TOTAL_MIDGAME_VALUE * 2);
    __ASSERT_BOUND(0, PreEvalEx.vlHollowThreat[i], cvlHollowThreat[i]);
    PreEvalEx.vlCentralThreat[i] = cvlCentralThreat[i];
  }

  // ȻжϸǷڽ״̬Ǽֹӵճ2ڱ1ӡ
  nWhiteAttacks = nBlackAttacks = 0;
  for (i = SIDE_TAG(0) + KNIGHT_FROM; i <= SIDE_TAG(0) + ROOK_TO; i ++) {
    if (this->ucsqPieces[i] != 0 && BLACK_HALF(this->ucsqPieces[i])) {
      nWhiteAttacks += 2;
    }
  }
  for (i = SIDE_TAG(0) + CANNON_FROM; i <= SIDE_TAG(0) + PAWN_TO; i ++) {
    if (this->ucsqPieces[i] != 0 && BLACK_HALF(this->ucsqPieces[i])) {
      nWhiteAttacks ++;
    }
  }
  for (i = SIDE_TAG(1) + KNIGHT_FROM; i <= SIDE_TAG(1) + ROOK_TO; i ++) {
    if (this->ucsqPieces[i] != 0 && WHITE_HALF(this->ucsqPieces[i])) {
      nBlackAttacks += 2;
    }
  }
  for (i = SIDE_TAG(1) + CANNON_FROM; i <= SIDE_TAG(1) + PAWN_TO; i ++) {
    if (this->ucsqPieces[i] != 0 && WHITE_HALF(this->ucsqPieces[i])) {
      nBlackAttacks ++;
    }
  }
  // ȶԷ࣬ôÿһ(2)вֵ2вֵ಻8
  nWhiteSimpleValue = PopCnt16(this->wBitPiece[0] & ROOK_BITPIECE) * 2 + PopCnt16(this->wBitPiece[0] & (KNIGHT_BITPIECE | CANNON_BITPIECE));
  nBlackSimpleValue = PopCnt16(this->wBitPiece[1] & ROOK_BITPIECE) * 2 + PopCnt16(this->wBitPiece[1] & (KNIGHT_BITPIECE | CANNON_BITPIECE));
  if (nWhiteSimpleValue > nBlackSimpleValue) {
    nWhiteAttacks += (nWhiteSimpleValue - nBlackSimpleValue) * 2;
  } else {
    nBlackAttacks += (nBlackSimpleValue - nWhiteSimpleValue) * 2;
  }
  nWhiteAttacks = MIN(nWhiteAttacks, TOTAL_ATTACK_VALUE);
  nBlackAttacks = MIN(nBlackAttacks, TOTAL_ATTACK_VALUE);
  PreEvalEx.vlBlackAdvisorLeakage = TOTAL_ADVISOR_LEAKAGE * nWhiteAttacks / TOTAL_ATTACK_VALUE;
  PreEvalEx.vlWhiteAdvisorLeakage = TOTAL_ADVISOR_LEAKAGE * nBlackAttacks / TOTAL_ATTACK_VALUE;
  __ASSERT_BOUND(0, nWhiteAttacks, TOTAL_ATTACK_VALUE);
  __ASSERT_BOUND(0, nBlackAttacks, TOTAL_ATTACK_VALUE);
  __ASSERT_BOUND(0, PreEvalEx.vlBlackAdvisorLeakage, TOTAL_ADVISOR_LEAKAGE);
  __ASSERT_BOUND(0, PreEvalEx.vlBlackAdvisorLeakage, TOTAL_ADVISOR_LEAKAGE);
  for (sq = 0; sq < 256; sq ++) {
    if (IN_BOARD(sq)) {
      PreEval.ucvlWhitePieces[1][sq] = PreEval.ucvlWhitePieces[2][sq] = (uint8_t) ((cucvlAdvisorBishopThreatened[sq] * nBlackAttacks +
          (PreEval.bPromotion ? cucvlAdvisorBishopPromotionThreatless[sq] : cucvlAdvisorBishopThreatless[sq]) * (TOTAL_ATTACK_VALUE - nBlackAttacks)) / TOTAL_ATTACK_VALUE);
      PreEval.ucvlBlackPieces[1][sq] = PreEval.ucvlBlackPieces[2][sq] = (uint8_t) ((cucvlAdvisorBishopThreatened[SQUARE_FLIP(sq)] * nWhiteAttacks +
          (PreEval.bPromotion ? cucvlAdvisorBishopPromotionThreatless[SQUARE_FLIP(sq)] : cucvlAdvisorBishopThreatless[SQUARE_FLIP(sq)]) * (TOTAL_ATTACK_VALUE - nWhiteAttacks)) / TOTAL_ATTACK_VALUE);
      PreEval.ucvlWhitePieces[6][sq] = (uint8_t) ((ucvlPawnPiecesAttacking[sq] * nWhiteAttacks +
          ucvlPawnPiecesAttackless[sq] * (TOTAL_ATTACK_VALUE - nWhiteAttacks)) / TOTAL_ATTACK_VALUE);
      PreEval.ucvlBlackPieces[6][sq] = (uint8_t) ((ucvlPawnPiecesAttacking[SQUARE_FLIP(sq)] * nBlackAttacks +
          ucvlPawnPiecesAttackless[SQUARE_FLIP(sq)] * (TOTAL_ATTACK_VALUE - nBlackAttacks)) / TOTAL_ATTACK_VALUE);
    }
  }
  for (i = 0; i < 16; i ++) {
    PreEvalEx.vlWhiteBottomThreat[i] = cvlBottomThreat[i] * nBlackAttacks / TOTAL_ATTACK_VALUE;
    PreEvalEx.vlBlackBottomThreat[i] = cvlBottomThreat[i] * nWhiteAttacks / TOTAL_ATTACK_VALUE;
  }

  // ԤǷԳ
#ifndef NDEBUG
  for (sq = 0; sq < 256; sq ++) {
    if (IN_BOARD(sq)) {
      for (i = 0; i < 7; i ++) {
        __ASSERT(PreEval.ucvlWhitePieces[i][sq] == PreEval.ucvlWhitePieces[i][SQUARE_MIRROR(sq)]);
        __ASSERT(PreEval.ucvlBlackPieces[i][sq] == PreEval.ucvlBlackPieces[i][SQUARE_MIRROR(sq)]);
      }
    }
  }
  for (i = FILE_LEFT; i <= FILE_RIGHT; i ++) {
    __ASSERT(PreEvalEx.vlWhiteBottomThreat[i] == PreEvalEx.vlWhiteBottomThreat[FILE_FLIP(i)]);
    __ASSERT(PreEvalEx.vlBlackBottomThreat[i] == PreEvalEx.vlBlackBottomThreat[FILE_FLIP(i)]);
  }
#endif

  // вٵ(ʿ)()ֵ
  this->vlWhite = ADVISOR_BISHOP_ATTACKLESS_VALUE * (TOTAL_ATTACK_VALUE - nBlackAttacks) / TOTAL_ATTACK_VALUE;
  this->vlBlack = ADVISOR_BISHOP_ATTACKLESS_VALUE * (TOTAL_ATTACK_VALUE - nWhiteAttacks) / TOTAL_ATTACK_VALUE;
  // 䣬ôв(ʿ)()ֵһ
  if (PreEval.bPromotion) {
    this->vlWhite /= 2;
    this->vlBlack /= 2;
  }
  // ¼λ÷
  for (i = 16; i < 32; i ++) {
    sq = this->ucsqPieces[i];
    if (sq != 0) {
      __ASSERT_SQUARE(sq);
      this->vlWhite += PreEval.ucvlWhitePieces[PIECE_TYPE(i)][sq];
    }
  }
  for (i = 32; i < 48; i ++) {
    sq = this->ucsqPieces[i];
    if (sq != 0) {
      __ASSERT_SQUARE(sq);
      this->vlBlack += PreEval.ucvlBlackPieces[PIECE_TYPE(i)][sq];
    }
  }
}
// ===== END eleeye/preeval.cpp =====

// ===== BEGIN eleeye/genmoves.cpp =====
/*
genmoves.cpp - Source Code for ElephantEye, Part IV

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.0, Last Modified: Nov. 2007
Copyright (C) 2004-2007 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


/* ElephantEyeԴʹõǺԼ
 *
 * sq: (0255"pregen.cpp")
 * pc: (047"position.cpp")
 * pt: (06"position.cpp")
 * mv: ŷ(065535"position.cpp")
 * sd: ӷ(0췽1ڷ)
 * vl: ֵ("-MATE_VALUE""MATE_VALUE""position.cpp")
 * (עĸǺſucdwȴļǺʹ)
 * pos: (PositionStructͣ"position.h")
 * sms: λкλеŷԤýṹ("pregen.h")
 * smv: λкλеŷжԤýṹ("pregen.h")
 */

// ģֻ漰"PositionStruct"е"sdPlayer""ucpcSquares""ucsqPieces"Աʡǰ"this->"

// ӱж
bool PositionStruct::Protected(int sd, int sqSrc, int sqExcept) const {
  // "sqExcept"ʾų(ָӱ)ǱǣӵıʱҪųǣĿӵı
  int i, sqDst, sqPin, pc, x, y, nSideTag;
  SlideMaskStruct *lpsmsRank, *lpsmsFile;
  // ӱжϰ¼裺

  __ASSERT_SQUARE(sqSrc);
  nSideTag = SIDE_TAG(sd);
  if (HOME_HALF(sqSrc, sd)) {
    if (IN_FORT(sqSrc)) {

      // 1. жܵ˧()ı
      sqDst = ucsqPieces[nSideTag + KING_FROM];
      if (sqDst != 0 && sqDst != sqExcept) {
        __ASSERT_SQUARE(sqDst);
        if (KING_SPAN(sqSrc, sqDst)) {
          return true;
        }
      }

      // 2. жܵ(ʿ)ı
      for (i = ADVISOR_FROM; i <= ADVISOR_TO; i ++) {
        sqDst = ucsqPieces[nSideTag + i];
        if (sqDst != 0 && sqDst != sqExcept) {
          __ASSERT_SQUARE(sqDst);
          if (ADVISOR_SPAN(sqSrc, sqDst)) {
            return true;
          }
        }
      }
    }

    // 3. жܵ()ı
    for (i = BISHOP_FROM; i <= BISHOP_TO; i ++) {
      sqDst = ucsqPieces[nSideTag + i];
      if (sqDst != 0 && sqDst != sqExcept) {
        __ASSERT_SQUARE(sqDst);
        if (BISHOP_SPAN(sqSrc, sqDst) && ucpcSquares[BISHOP_PIN(sqSrc, sqDst)] == 0) {
          return true;
        }
      }
    }
  } else {

    // 4. жܵӱ()ı
    for (sqDst = sqSrc - 1; sqDst <= sqSrc + 1; sqDst += 2) {
      // ڱߣôԲ
      // __ASSERT_SQUARE(sqDst);
      if (sqDst != sqExcept) {
        pc = ucpcSquares[sqDst];
        if ((pc & nSideTag) != 0 && PIECE_INDEX(pc) >= PAWN_FROM) {
          return true;
        }
      }
    }
  }

  // 5. жܵ()ı
  sqDst = SQUARE_BACKWARD(sqSrc, sd);
  // ڵߣôԲ
  // __ASSERT_SQUARE(sqDst);
  if (sqDst != sqExcept) {
    pc = ucpcSquares[sqDst];
    if ((pc & nSideTag) != 0 && PIECE_INDEX(pc) >= PAWN_FROM) {
      return true;
    }
  }

  // 6. жܵı
  for (i = KNIGHT_FROM; i <= KNIGHT_TO; i ++) {
    sqDst = ucsqPieces[nSideTag + i];
    if (sqDst != 0 && sqDst != sqExcept) {
      __ASSERT_SQUARE(sqDst);
      sqPin = KNIGHT_PIN(sqDst, sqSrc); // ע⣬sqSrcsqDstǷģ
      if (sqPin != sqDst && ucpcSquares[sqPin] == 0) {
        return true;
      }
    }
  }

  x = FILE_X(sqSrc);
  y = RANK_Y(sqSrc);
  lpsmsRank = RankMaskPtr(x, y);
  lpsmsFile = FileMaskPtr(x, y);

  // 7. жܵı"position.cpp""CheckedBy()"
  for (i = ROOK_FROM; i <= ROOK_TO; i ++) {
    sqDst = ucsqPieces[nSideTag + i];
    if (sqDst != 0 && sqDst != sqSrc && sqDst != sqExcept) {
      if (x == FILE_X(sqDst)) {
        if ((lpsmsFile->wRookCap & PreGen.wBitFileMask[sqDst]) != 0) {
          return true;
        }
      } else if (y == RANK_Y(sqDst)) {
        if ((lpsmsRank->wRookCap & PreGen.wBitRankMask[sqDst]) != 0) {
          return true;
        }
      }
    }
  }

  // 8. жܵڵı"position.cpp""CheckedBy()"
  for (i = CANNON_FROM; i <= CANNON_TO; i ++) {
    sqDst = ucsqPieces[nSideTag + i];
    if (sqDst && sqDst != sqSrc && sqDst != sqExcept) {
      if (x == FILE_X(sqDst)) {
        if ((lpsmsFile->wCannonCap & PreGen.wBitFileMask[sqDst]) != 0) {
          return true;
        }
      } else if (y == RANK_Y(sqDst)) {
        if ((lpsmsRank->wCannonCap & PreGen.wBitRankMask[sqDst]) != 0) {
          return true;
        }
      }
    }
  }
  return false;
}

/* MVV(LVA)ֵĺ
 *
 * MVV(LVA)ָǣޱôȡֵMVVȡֵMVV-LVA
 * ElephantEyeMVV(LVA)ֵڼϺټ1ǣ¼ֺ壺
 * a. MVV(LVA)1˵Ӽֵڹ(׬)ֳӽ̬Ҳֳӣ
 * b. MVV(LVA)1˵һֵ(ԳڻԹӱ䣬ǿģҲֵһ)̬Ҳֳӣ
 * c. MVV(LVA)0˵ûмֳֵ̬ӡ
 *
 * MVVֵ"SIMPLE_VALUE"ǰ˧()=5=4=3()=2(ʿ)()=1趨ģ
 * LVAֱֵڳŷС
 */
int PositionStruct::MvvLva(int sqDst, int pcCaptured, int nLva) const {
  int nMvv, nLvaAdjust;
  nMvv = SIMPLE_VALUE(pcCaptured);
  nLvaAdjust = (Protected(OPP_SIDE(sdPlayer), sqDst) ? nLva : 0);
  if (nMvv >= nLvaAdjust) {
    return nMvv - nLvaAdjust + 1;
  } else {
    return (nMvv >= 3 || HOME_HALF(sqDst, sdPlayer)) ? 1 : 0;
  }
}

// ŷMVV(LVA)趨ֵ
int PositionStruct::GenCapMoves(MoveStruct *lpmvs) const {
  int i, sqSrc, sqDst, pcCaptured;
  int x, y, nSideTag, nOppSideTag;
  bool bCanPromote;
  SlideMoveStruct *lpsmv;
  uint8_t *lpucsqDst, *lpucsqPin;
  MoveStruct *lpmvsCurr;
  // ɳŷĹ̰¼裺

  lpmvsCurr = lpmvs;
  nSideTag = SIDE_TAG(sdPlayer);
  nOppSideTag = OPP_SIDE_TAG(sdPlayer);
  bCanPromote = PreEval.bPromotion && CanPromote();

  // 1. ˧()ŷ
  sqSrc = ucsqPieces[nSideTag + KING_FROM];
  if (sqSrc != 0) {
    __ASSERT_SQUARE(sqSrc);
    lpucsqDst = PreGen.ucsqKingMoves[sqSrc];
    sqDst = *lpucsqDst;
    while (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      // ҵһŷжϳԵǷǶԷӣ"nOppSideTag"ı־(1632ߵ)
      // ǶԷӣ򱣴MVV(LVA)ֵޱֻMVVMVV-LVA(MVV>LVAĻ)
      pcCaptured = ucpcSquares[sqDst];
      if ((pcCaptured & nOppSideTag) != 0) {
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
        lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 5); // ˧()ļֵ5
        lpmvsCurr ++;
      }
      lpucsqDst ++;
      sqDst = *lpucsqDst;
    }
  }

  // 2. (ʿ)ŷ
  for (i = ADVISOR_FROM; i <= ADVISOR_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqAdvisorMoves[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 1); // (ʿ)ļֵ1
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
      }
      if (bCanPromote && CAN_PROMOTE(sqSrc)) {
        lpmvsCurr->wmv = MOVE(sqSrc, sqSrc);
        lpmvsCurr->wvl = 0;
        lpmvsCurr ++;
      }
    }
  }

  // 3. ()ŷ
  for (i = BISHOP_FROM; i <= BISHOP_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqBishopMoves[sqSrc];
      lpucsqPin = PreGen.ucsqBishopPins[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[*lpucsqPin] == 0) {
          pcCaptured = ucpcSquares[sqDst];
          if ((pcCaptured & nOppSideTag) != 0) {
            __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
            lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
            lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 1); // ()ļֵ1
            lpmvsCurr ++;
          }
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
        lpucsqPin ++;
      }
      if (bCanPromote && CAN_PROMOTE(sqSrc)) {
        lpmvsCurr->wmv = MOVE(sqSrc, sqSrc);
        lpmvsCurr->wvl = 0;
        lpmvsCurr ++;
      }
    }
  }

  // 4. ŷ
  for (i = KNIGHT_FROM; i <= KNIGHT_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqKnightMoves[sqSrc];
      lpucsqPin = PreGen.ucsqKnightPins[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[*lpucsqPin] == 0) {
          pcCaptured = ucpcSquares[sqDst];
          if ((pcCaptured & nOppSideTag) != 0) {
            __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
            lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
            lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // ļֵ3
            lpmvsCurr ++;
          }
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
        lpucsqPin ++;
      }
    }
  }

  // 5. ɳŷ
  for (i = ROOK_FROM; i <= ROOK_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      x = FILE_X(sqSrc);
      y = RANK_Y(sqSrc);

      lpsmv = RankMovePtr(x, y);
      sqDst = lpsmv->ucRookCap[0] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 4); // ļֵ4
          lpmvsCurr ++;
        }
      }
      sqDst = lpsmv->ucRookCap[1] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 4); // ļֵ4
          lpmvsCurr ++;
        }
      }

      lpsmv = FileMovePtr(x, y);
      sqDst = lpsmv->ucRookCap[0] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 4); // ļֵ4
          lpmvsCurr ++;
        }
      }
      sqDst = lpsmv->ucRookCap[1] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 4); // ļֵ4
          lpmvsCurr ++;
        }
      }
    }
  }

  // 6. ڵŷ
  for (i = CANNON_FROM; i <= CANNON_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      x = FILE_X(sqSrc);
      y = RANK_Y(sqSrc);

      lpsmv = RankMovePtr(x, y);
      sqDst = lpsmv->ucCannonCap[0] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // ڵļֵ3
          lpmvsCurr ++;
        }
      }
      sqDst = lpsmv->ucCannonCap[1] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // ڵļֵ3
          lpmvsCurr ++;
        }
      }

      lpsmv = FileMovePtr(x, y);
      sqDst = lpsmv->ucCannonCap[0] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // ڵļֵ3
          lpmvsCurr ++;
        }
      }
      sqDst = lpsmv->ucCannonCap[1] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // ڵļֵ3
          lpmvsCurr ++;
        }
      }
    }
  }

  // 7. ɱ()ŷ
  for (i = PAWN_FROM; i <= PAWN_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqPawnMoves[sdPlayer][sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 2); // ()ļֵ2
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
      }
    }
  }
  return lpmvsCurr - lpmvs;
}

// ŷ
int PositionStruct::GenNonCapMoves(MoveStruct *lpmvs) const {
  int i, sqSrc, sqDst, x, y, nSideTag;
  SlideMoveStruct *lpsmv;
  uint8_t *lpucsqDst, *lpucsqPin;
  MoveStruct *lpmvsCurr;
  // ɲŷĹ̰¼裺

  lpmvsCurr = lpmvs;
  nSideTag = SIDE_TAG(sdPlayer);

  // 1. ˧()ŷ
  sqSrc = ucsqPieces[nSideTag + KING_FROM];
  if (sqSrc != 0) {
    __ASSERT_SQUARE(sqSrc);
    lpucsqDst = PreGen.ucsqKingMoves[sqSrc];
    sqDst = *lpucsqDst;
    while (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      // ҵһŷжǷԵ
      if (ucpcSquares[sqDst] == 0) {
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
      }
      lpucsqDst ++;
      sqDst = *lpucsqDst;
    }
  }

  // 2. (ʿ)ŷ
  for (i = ADVISOR_FROM; i <= ADVISOR_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqAdvisorMoves[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[sqDst] == 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
      }
    }
  }

  // 3. ()ŷ
  for (i = BISHOP_FROM; i <= BISHOP_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqBishopMoves[sqSrc];
      lpucsqPin = PreGen.ucsqBishopPins[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[*lpucsqPin] == 0 && ucpcSquares[sqDst] == 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
        lpucsqPin ++;
      }
    }
  }

  // 4. ŷ
  for (i = KNIGHT_FROM; i <= KNIGHT_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqKnightMoves[sqSrc];
      lpucsqPin = PreGen.ucsqKnightPins[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[*lpucsqPin] == 0 && ucpcSquares[sqDst] == 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
        lpucsqPin ++;
      }
    }
  }

  // 5. ɳڵŷûбҪжǷԵ
  for (i = ROOK_FROM; i <= CANNON_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      x = FILE_X(sqSrc);
      y = RANK_Y(sqSrc);

      lpsmv = RankMovePtr(x, y);
      sqDst = lpsmv->ucNonCap[0] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      while (sqDst != sqSrc) {
        __ASSERT(ucpcSquares[sqDst] == 0);
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
        sqDst --;
      }
      sqDst = lpsmv->ucNonCap[1] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      while (sqDst != sqSrc) {
        __ASSERT(ucpcSquares[sqDst] == 0);
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
        sqDst ++;
      }

      lpsmv = FileMovePtr(x, y);
      sqDst = lpsmv->ucNonCap[0] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      while (sqDst != sqSrc) {
        __ASSERT(ucpcSquares[sqDst] == 0);
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
        sqDst -= 16;
      }
      sqDst = lpsmv->ucNonCap[1] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      while (sqDst != sqSrc) {
        __ASSERT(ucpcSquares[sqDst] == 0);
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
        sqDst += 16;
      }
    }
  }

  // 6. ɱ()ŷ
  for (i = PAWN_FROM; i <= PAWN_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqPawnMoves[sdPlayer][sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[sqDst] == 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
      }
    }
  }
  return lpmvsCurr - lpmvs;
}

// ׽ļ
int PositionStruct::ChasedBy(int mv) const {
  int i, nSideTag, pcMoved, pcCaptured;
  int sqSrc, sqDst, x, y;
  uint8_t *lpucsqDst, *lpucsqPin;
  SlideMoveStruct *lpsmv;

  sqSrc = DST(mv);
  pcMoved = this->ucpcSquares[sqSrc];
  nSideTag = SIDE_TAG(this->sdPlayer);
  __ASSERT_SQUARE(sqSrc);
  __ASSERT_PIECE(pcMoved);
  __ASSERT_BOUND(0, pcMoved - OPP_SIDE_TAG(this->sdPlayer), 15);

  // ׽жϰ¼ݣ
  switch (pcMoved - OPP_SIDE_TAG(this->sdPlayer)) {

  // 1. жǷ׽׽иڱ()
  case KNIGHT_FROM:
  case KNIGHT_TO:
    // һȵİ˸λ
    lpucsqDst = PreGen.ucsqKnightMoves[sqSrc];
    lpucsqPin = PreGen.ucsqKnightPins[sqSrc];
    sqDst = *lpucsqDst;
    while (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      if (ucpcSquares[*lpucsqPin] == 0) {
        pcCaptured = this->ucpcSquares[sqDst];
        if ((pcCaptured & nSideTag) != 0) {
          pcCaptured -= nSideTag;
          __ASSERT_BOUND(0, pcCaptured, 15);
          // ɣŻжϵķ֦
          if (pcCaptured <= ROOK_TO) {
            // ׽(ʿ)()迼
            if (pcCaptured >= ROOK_FROM) {
              // ׽˳
              return pcCaptured;
            }
          } else {
            if (pcCaptured <= CANNON_TO) {
              // ׽ڣҪжǷܱ
              if (!Protected(this->sdPlayer, sqDst)) {
                return pcCaptured;
              }
            } else {
              // ׽˱()Ҫжϱ()ǷӲܱ
              if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                return pcCaptured;
              }
            }
          }
        }
      }
      lpucsqDst ++;
      sqDst = *lpucsqDst;
      lpucsqPin ++;
    }
    break;

  // 2. ˳жǷ׽иڱ()
  case ROOK_FROM:
  case ROOK_TO:
    x = FILE_X(sqSrc);
    y = RANK_Y(sqSrc);
    if (((SRC(mv) ^ sqSrc) & 0xf) == 0) {
      // ƶˣжϳԵ
      lpsmv = RankMovePtr(x, y);
      for (i = 0; i < 2; i ++) {
        sqDst = lpsmv->ucRookCap[i] + RANK_DISP(y);
        __ASSERT_SQUARE(sqDst);
        if (sqDst != sqSrc) {
          pcCaptured = this->ucpcSquares[sqDst];
          if ((pcCaptured & nSideTag) != 0) {
            pcCaptured -= nSideTag;
            __ASSERT_BOUND(0, pcCaptured, 15);
            // ɣŻжϵķ֦
            if (pcCaptured <= ROOK_TO) {
              // ׽(ʿ)()迼
              if (pcCaptured >= KNIGHT_FROM) {
                if (pcCaptured <= KNIGHT_TO) {
                  // ׽ҪжǷܱ
                  if (!Protected(this->sdPlayer, sqDst)) {
                    return pcCaptured;
                  }
                }
                // ׽迼
              }
            } else {
              if (pcCaptured <= CANNON_TO) {
                // ׽ڣҪжǷܱ
                if (!Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              } else {
                // ׽˱()Ҫжϱ()ǷӲܱ
                if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              }
            }
          }
        }
      }
    } else {
      // ƶˣжϳԵ
      lpsmv = FileMovePtr(x, y);
      for (i = 0; i < 2; i ++) {
        sqDst = lpsmv->ucRookCap[i] + FILE_DISP(x);
        __ASSERT_SQUARE(sqDst);
        if (sqDst != sqSrc) {
          pcCaptured = this->ucpcSquares[sqDst];
          if ((pcCaptured & nSideTag) != 0) {
            pcCaptured -= nSideTag;
            __ASSERT_BOUND(0, pcCaptured, 15);
            // ɣŻжϵķ֦
            if (pcCaptured <= ROOK_TO) {
              // ׽(ʿ)()迼
              if (pcCaptured >= KNIGHT_FROM) {
                if (pcCaptured <= KNIGHT_TO) {
                  // ׽ҪжǷܱ
                  if (!Protected(this->sdPlayer, sqDst)) {
                    return pcCaptured;
                  }
                }
                // ׽迼
              }
            } else {
              if (pcCaptured <= CANNON_TO) {
                // ׽ڣҪжǷܱ
                if (!Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              } else {
                // ׽˱()Ҫжϱ()ǷӲܱ
                if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              }
            }
          }
        }
      }
    }
    break;

  // 3. ڣжǷ׽׽и()
  case CANNON_FROM:
  case CANNON_TO:
    x = FILE_X(sqSrc);
    y = RANK_Y(sqSrc);
    if (((SRC(mv) ^ sqSrc) & 0xf) == 0) {
      // ƶˣжںԵ
      lpsmv = RankMovePtr(x, y);
      for (i = 0; i < 2; i ++) {
        sqDst = lpsmv->ucCannonCap[i] + RANK_DISP(y);
        __ASSERT_SQUARE(sqDst);
        if (sqDst != sqSrc) {
          pcCaptured = this->ucpcSquares[sqDst];
          if ((pcCaptured & nSideTag) != 0) {
            pcCaptured -= nSideTag;
            __ASSERT_BOUND(0, pcCaptured, 15);
            // ɣŻжϵķ֦
            if (pcCaptured <= ROOK_TO) {
              // ׽(ʿ)()迼
              if (pcCaptured >= KNIGHT_FROM) {
                if (pcCaptured <= KNIGHT_TO) {
                  // ׽ҪжǷܱ
                  if (!Protected(this->sdPlayer, sqDst)) {
                    return pcCaptured;
                  }
                } else {
                  // ׽˳
                  return pcCaptured;
                }
              }
            } else {
              // ׽ڵ迼
              if (pcCaptured >= PAWN_FROM) {
                // ׽˱()Ҫжϱ()ǷӲܱ
                if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              }
            }
          }
        }
      }
    } else {
      // ںƶˣжԵ
      lpsmv = FileMovePtr(x, y);
      for (i = 0; i < 2; i ++) {
        sqDst = lpsmv->ucCannonCap[i] + FILE_DISP(x);
        __ASSERT_SQUARE(sqDst);
        if (sqDst != sqSrc) {
          pcCaptured = this->ucpcSquares[sqDst];
          if ((pcCaptured & nSideTag) != 0) {
            pcCaptured -= nSideTag;
            __ASSERT_BOUND(0, pcCaptured, 15);
            // ɣŻжϵķ֦
            if (pcCaptured <= ROOK_TO) {
              // ׽(ʿ)()迼
              if (pcCaptured >= KNIGHT_FROM) {
                if (pcCaptured <= KNIGHT_TO) {
                  // ׽ҪжǷܱ
                  if (!Protected(this->sdPlayer, sqDst)) {
                    return pcCaptured;
                  }
                } else {
                  // ׽˳
                  return pcCaptured;
                }
              }
            } else {
              // ׽ڵ迼
              if (pcCaptured >= PAWN_FROM) {
                // ׽˱()Ҫжϱ()ǷӲܱ
                if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              }
            }
          }
        }
      }
    }
    break;
  }

  return 0;
}
// ===== END eleeye/genmoves.cpp =====

// ===== BEGIN eleeye/movesort.cpp =====
/*
movesort.h/movesort.cpp - Source Code for ElephantEye, Part VII

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.11, Last Modified: Dec. 2007
Copyright (C) 2004-2007 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


int nHistory[65536]; // ʷ

// ʷŷбֵ
void MoveSortStruct::SetHistory(void) {
  int i, j, vl, nShift, nNewShift;
  nShift = 0;
  for (i = nMoveIndex; i < nMoveNum; i ++) {
    // ŷķֵ65536ͱŷķֵʹǶ65536
    vl = nHistory[mvs[i].wmv] >> nShift;
    if (vl > 65535) {
      nNewShift = Bsr(vl) - 15;
      for (j = nMoveIndex; j < i; j ++) {
        mvs[j].wvl >>= nNewShift;
      }
      vl >>= nNewShift;
      __ASSERT_BOUND(32768, vl, 65535);
      nShift += nNewShift;
    }
    mvs[i].wvl = vl;
  }
}

// Shell򷨣"1, 4, 13, 40 ..."УҪ"1, 2, 4, 8, ..."
static const int cnShellStep[8] = {0, 1, 4, 13, 40, 121, 364, 1093};

void MoveSortStruct::ShellSort(void) {
  int i, j, nStep, nStepLevel;
  MoveStruct mvsBest;
  nStepLevel = 1;
  while (cnShellStep[nStepLevel] < nMoveNum - nMoveIndex) {
    nStepLevel ++;
  }
  nStepLevel --;
  while (nStepLevel > 0) {
    nStep = cnShellStep[nStepLevel];
    for (i = nMoveIndex + nStep; i < nMoveNum; i ++) {
      mvsBest = mvs[i];
      j = i - nStep;
      while (j >= nMoveIndex && mvsBest.wvl > mvs[j].wvl) {
        mvs[j + nStep] = mvs[j];
        j -= nStep;
      }
      mvs[j + nStep] = mvsBest;
    }
    nStepLevel --;
  }
}

/* ɽ⽫ŷΨһӦŷ(жӦŷ򷵻)
 * 
 * ⽫ŷ˳£
 * 1. ûŷ(SORT_VALUE_MAX)
 * 2. ɱŷ(SORT_VALUE_MAX - 12)
 * 3. ŷʷ(1SORT_VALUE_MAX - 3)
 * 4. ܽ⽫ŷ(0)Щŷ˵
 */
int MoveSortStruct::InitEvade(PositionStruct &pos, int mv, const uint16_t *lpwmvKiller) {
  int i, nLegal;
  nPhase = PHASE_REST;
  nMoveIndex = 0;
  nMoveNum = pos.GenAllMoves(mvs);
  SetHistory();
  nLegal = 0;
  for (i = nMoveIndex; i < nMoveNum; i ++) {
    if (mvs[i].wmv == mv) {
      nLegal ++;
      mvs[i].wvl = SORT_VALUE_MAX;
    } else if (pos.MakeMove(mvs[i].wmv)) {
      pos.UndoMakeMove();
      nLegal ++;
      if (mvs[i].wmv == lpwmvKiller[0]) {
        mvs[i].wvl = SORT_VALUE_MAX - 1;
      } else if (mvs[i].wmv == lpwmvKiller[1]) {
        mvs[i].wvl = SORT_VALUE_MAX - 2;
      } else {
        mvs[i].wvl = MIN(mvs[i].wvl + 1, SORT_VALUE_MAX - 3);
      }
    } else {
      mvs[i].wvl = 0;
    }
  }
  ShellSort();
  nMoveNum = nMoveIndex + nLegal;
  return (nLegal == 1 ? mvs[0].wmv : 0);
}

// һŷ
int MoveSortStruct::NextFull(const PositionStruct &pos) {
  switch (nPhase) {
  // "nPhase"ʾŷɽ׶ΣΪ

  // 0. ûŷɺһ׶Σ
  case PHASE_HASH:
    nPhase = PHASE_GEN_CAP;
    if (mvHash != 0) {
      __ASSERT(pos.LegalMove(mvHash));
      return mvHash;
    }
    // ɣû"break"ʾ"switch"һ"case"ִһ"case"ͬ

  // 1. гŷɺһ׶Σ
  case PHASE_GEN_CAP:
    nPhase = PHASE_GOODCAP;
    nMoveIndex = 0;
    nMoveNum = pos.GenCapMoves(mvs);
    ShellSort();

  // 2. MVV(LVA)ҪѭɴΣ
  case PHASE_GOODCAP:
    if (nMoveIndex < nMoveNum && mvs[nMoveIndex].wvl > 1) {
      // ע⣺MVV(LVA)ֵ1˵ӲֱܻƵģЩŷԺ
      nMoveIndex ++;
      __ASSERT_PIECE(pos.ucpcSquares[DST(mvs[nMoveIndex - 1].wmv)]);
      return mvs[nMoveIndex - 1].wmv;
    }

  // 3. ɱŷ(һɱŷ)ɺһ׶Σ
  case PHASE_KILLER1:
    nPhase = PHASE_KILLER2;
    if (mvKiller1 != 0 && pos.LegalMove(mvKiller1)) {
      // ע⣺ɱŷŷԣͬ
      return mvKiller1;
    }

  // 4. ɱŷ(ڶɱŷ)ɺһ׶Σ
  case PHASE_KILLER2:
    nPhase = PHASE_GEN_NONCAP;
    if (mvKiller2 != 0 && pos.LegalMove(mvKiller2)) {
      return mvKiller2;
    }

  // 5. вŷɺһ׶Σ
  case PHASE_GEN_NONCAP:
    nPhase = PHASE_REST;
    nMoveNum += pos.GenNonCapMoves(mvs + nMoveNum);
    SetHistory();
    ShellSort();

  // 6. ʣŷʷ(ؽ⽫ŷ)
  case PHASE_REST:
    if (nMoveIndex < nMoveNum) {
      nMoveIndex ++;
      return mvs[nMoveIndex - 1].wmv;
    }

  // 7. ûŷˣ㡣
  default:
    return 0;
  }
}

// ɸŷ
void MoveSortStruct::InitRoot(const PositionStruct &pos, int nBanMoves, const uint16_t *lpwmvBanList) {
  int i, j, nBanned;
  nMoveIndex = 0;
  nMoveNum = pos.GenAllMoves(mvs);
  nBanned = 0;
  for (i = 0; i < nMoveNum; i ++) {
    mvs[i].wvl = 1;
    for (j = 0; j < nBanMoves; j ++) {
      if (mvs[i].wmv == lpwmvBanList[j]) {
        mvs[i].wvl = 0;
        nBanned ++;
        break;
      }
    }  
  }
  ShellSort();
  nMoveNum -= nBanned;
}

// ¸ŷб
void MoveSortStruct::UpdateRoot(int mv) {
  int i;
  for (i = 0; i < nMoveNum; i ++) {
    if (mvs[i].wmv == mv) {
      mvs[i].wvl = SORT_VALUE_MAX;
    } else if (mvs[i].wvl > 0) {
      mvs[i].wvl --;      
    }
  }
}
// ===== END eleeye/movesort.cpp =====

// ===== BEGIN eleeye/evaluate.cpp =====
/*
evaluate.cpp - Source Code for ElephantEye, Part XI

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.3, Last Modified: Mar. 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


/* ElephantEyeԴʹõǺԼ
 *
 * sq: (0255"pregen.cpp")
 * pc: (047"position.cpp")
 * pt: (06"position.cpp")
 * mv: ŷ(065535"position.cpp")
 * sd: ӷ(0췽1ڷ)
 * vl: ֵ("-MATE_VALUE""MATE_VALUE""position.cpp")
 * (עǺſucdwȴļǺʹ)
 * pos: (PositionStructͣ"position.h")
 * sms: λкλеŷԤýṹ("pregen.h")
 * smv: λкλеŷжԤýṹ("pregen.h")
 */

// ͵۵ı߽
const int EVAL_MARGIN1 = 160;
const int EVAL_MARGIN2 = 80;
const int EVAL_MARGIN3 = 40;
const int EVAL_MARGIN4 = 20;

// ģֻ漰"PositionStruct"е"sdPlayer""ucpcSquares""ucsqPieces""wBitPiece"ĸԱʡǰ"this->"

/* ElephantEyeľݹ44
 * 1. (ʿ)йص͵ۣ"AdvisorShape()"
 * 2. ǣ˧()򳵵͵ۣ"StringHold()"
 * 3. Եۣ"RookMobility()"
 * 4. ܵ谭ۣ"KnightTrap()"
 */

// ǵһ֣͵

/* (ʿ)״ھۣرжϿͷڡڵشãΪElephantEye״
 * 1. ˧()ԭλ˫(ʿ)ڵߣΪ1ţҪжϿͷں
 * 2. ˧()ԭλ˫(ʿ)߰Χ˧()Ϊ2ţҪжұߵĳںͳұߵ˧()ţ
 * 3. ˧()ԭλ˫(ʿ)ұ߰Χ˧()Ϊ3ţҪжߵĳںͳߵ˧()ţ
 * 4. ˧()ԭλȱ(ʿ)0
 * עԡºϡ̶̷λ涨ҡ
 */
const int WHITE_KING_BITFILE = 1 << (RANK_BOTTOM - RANK_TOP);
const int BLACK_KING_BITFILE = 1 << (RANK_TOP - RANK_TOP);
const int KING_BITRANK = 1 << (FILE_CENTER - FILE_LEFT);

const int SHAPE_NONE = 0;
const int SHAPE_CENTER = 1;
const int SHAPE_LEFT = 2;
const int SHAPE_RIGHT = 3;

int PositionStruct::AdvisorShape(void) const {
  int pcCannon, pcRook, sq, sqAdv1, sqAdv2, x, y, nShape;
  int vlWhitePenalty, vlBlackPenalty;
  SlideMaskStruct *lpsms;
  vlWhitePenalty = vlBlackPenalty = 0;
  if ((this->wBitPiece[0] & ADVISOR_BITPIECE) == ADVISOR_BITPIECE) {
    if (this->ucsqPieces[SIDE_TAG(0) + KING_FROM] == 0xc7) {
      sqAdv1 = this->ucsqPieces[SIDE_TAG(0) + ADVISOR_FROM];
      sqAdv2 = this->ucsqPieces[SIDE_TAG(0) + ADVISOR_TO];
      if (false) {
      } else if (sqAdv1 == 0xc6) { // 췽һ
        nShape = (sqAdv2 == 0xc8 ? SHAPE_CENTER : sqAdv2 == 0xb7 ? SHAPE_LEFT : SHAPE_NONE);
      } else if (sqAdv1 == 0xc8) { // 췽һҲ
        nShape = (sqAdv2 == 0xc6 ? SHAPE_CENTER : sqAdv2 == 0xb7 ? SHAPE_RIGHT : SHAPE_NONE);
      } else if (sqAdv1 == 0xb7) { // 췽һڻ
        nShape = (sqAdv2 == 0xc6 ? SHAPE_LEFT : sqAdv2 == 0xc8 ? SHAPE_RIGHT : SHAPE_NONE);
      } else {
        nShape = SHAPE_NONE;
      }
      switch (nShape) {
      case SHAPE_NONE:
        break;
      case SHAPE_CENTER:
        for (pcCannon = SIDE_TAG(1) + CANNON_FROM; pcCannon <= SIDE_TAG(1) + CANNON_TO; pcCannon ++) {
          sq = this->ucsqPieces[pcCannon];
          if (sq != 0) {
            x = FILE_X(sq);
            if (x == FILE_CENTER) {
              y = RANK_Y(sq);
              lpsms = this->FileMaskPtr(x, y);
              if ((lpsms->wRookCap & WHITE_KING_BITFILE) != 0) {
                // ͷڵв
                vlWhitePenalty += PreEvalEx.vlHollowThreat[RANK_FLIP(y)];
              } else if ((lpsms->wSuperCap & WHITE_KING_BITFILE) != 0 &&
                  (this->ucpcSquares[0xb7] == 21 || this->ucpcSquares[0xb7] == 22)) {
                // в
                vlWhitePenalty += PreEvalEx.vlCentralThreat[RANK_FLIP(y)];
              }
            }
          }
        }
        break;
      case SHAPE_LEFT:
      case SHAPE_RIGHT:
        for (pcCannon = SIDE_TAG(1) + CANNON_FROM; pcCannon <= SIDE_TAG(1) + CANNON_TO; pcCannon ++) {
          sq = this->ucsqPieces[pcCannon];
          if (sq != 0) {
            x = FILE_X(sq);
            y = RANK_Y(sq);
            if (x == FILE_CENTER) {
              if ((this->FileMaskPtr(x, y)->wSuperCap & WHITE_KING_BITFILE) != 0) {
                // һڵв˧()űԷƵĻжⷣ
                vlWhitePenalty += (PreEvalEx.vlCentralThreat[RANK_FLIP(y)] >> 2) +
                    (this->Protected(1, nShape == SHAPE_LEFT ? 0xc8 : 0xc6) ? 20 : 0);
                // ڵ߱˧()ķ֣
                for (pcRook = SIDE_TAG(0) + ROOK_FROM; pcRook <= SIDE_TAG(0) + ROOK_TO; pcRook ++) {
                  sq = this->ucsqPieces[pcRook];
                  if (sq != 0) {
                    y = RANK_Y(sq);
                    if (y == RANK_BOTTOM) {
                      x = FILE_X(sq);
                      if ((this->RankMaskPtr(x, y)->wRookCap & KING_BITRANK) != 0) {
                        vlWhitePenalty += 80;
                      }
                    }
                  }
                }
              }
            } else if (y == RANK_BOTTOM) {
              if ((this->RankMaskPtr(x, y)->wRookCap & KING_BITRANK) != 0) {
                // ڵв
                vlWhitePenalty += PreEvalEx.vlWhiteBottomThreat[x];
              }
            }
          }
        }
        break;
      default:
        break;
      }
    } else if (this->ucsqPieces[SIDE_TAG(0) + KING_FROM] == 0xb7) {
      // ˫(ʿ)ı˧()ռ죬Ҫ
      vlWhitePenalty += 20;
    }
  } else {
    if ((this->wBitPiece[1] & ROOK_BITPIECE) == ROOK_BITPIECE) {
      // ȱ(ʿ)˫з
      vlWhitePenalty += PreEvalEx.vlWhiteAdvisorLeakage;
    }
  }
  if ((this->wBitPiece[1] & ADVISOR_BITPIECE) == ADVISOR_BITPIECE) {
    if (this->ucsqPieces[SIDE_TAG(1) + KING_FROM] == 0x37) {
      sqAdv1 = this->ucsqPieces[SIDE_TAG(1) + ADVISOR_FROM];
      sqAdv2 = this->ucsqPieces[SIDE_TAG(1) + ADVISOR_TO];
      if (false) {
      } else if (sqAdv1 == 0x36) { // ڷһʿ
        nShape = (sqAdv2 == 0x38 ? SHAPE_CENTER : sqAdv2 == 0x47 ? SHAPE_LEFT : SHAPE_NONE);
      } else if (sqAdv1 == 0x38) { // ڷһʿҲ
        nShape = (sqAdv2 == 0x36 ? SHAPE_CENTER : sqAdv2 == 0x47 ? SHAPE_RIGHT : SHAPE_NONE);
      } else if (sqAdv1 == 0x47) { // ڷһʿڻ
        nShape = (sqAdv2 == 0x36 ? SHAPE_LEFT : sqAdv2 == 0x38 ? SHAPE_RIGHT : SHAPE_NONE);
      } else {
        nShape = SHAPE_NONE;
      }
      switch (nShape) {
      case SHAPE_NONE:
        break;
      case SHAPE_CENTER:
        for (pcCannon = SIDE_TAG(0) + CANNON_FROM; pcCannon <= SIDE_TAG(0) + CANNON_TO; pcCannon ++) {
          sq = this->ucsqPieces[pcCannon];
          if (sq != 0) {
            x = FILE_X(sq);
            if (x == FILE_CENTER) {
              y = RANK_Y(sq);
              lpsms = this->FileMaskPtr(x, y);
              if ((lpsms->wRookCap & BLACK_KING_BITFILE) != 0) {
                // ͷڵв
                vlBlackPenalty += PreEvalEx.vlHollowThreat[y];
              } else if ((lpsms->wSuperCap & BLACK_KING_BITFILE) != 0 &&
                  (this->ucpcSquares[0x47] == 37 || this->ucpcSquares[0x47] == 38)) {
                // в
                vlBlackPenalty += PreEvalEx.vlCentralThreat[y];
              }
            }
          }
        }
        break;
      case SHAPE_LEFT:
      case SHAPE_RIGHT:
        for (pcCannon = SIDE_TAG(0) + CANNON_FROM; pcCannon <= SIDE_TAG(0) + CANNON_TO; pcCannon ++) {
          sq = this->ucsqPieces[pcCannon];
          if (sq != 0) {
            x = FILE_X(sq);
            y = RANK_Y(sq);
            if (x == FILE_CENTER) {
              if ((this->FileMaskPtr(x, y)->wSuperCap & BLACK_KING_BITFILE) != 0) {
                // һڵв˧()űԷƵĻжⷣ
                vlBlackPenalty += (PreEvalEx.vlCentralThreat[y] >> 2) +
                    (this->Protected(0, nShape == SHAPE_LEFT ? 0x38 : 0x36) ? 20 : 0);
                // ڵ߱˧()ķ֣
                for (pcRook = SIDE_TAG(1) + ROOK_FROM; pcRook <= SIDE_TAG(1) + ROOK_TO; pcRook ++) {
                  sq = this->ucsqPieces[pcRook];
                  if (sq != 0) {
                    y = RANK_Y(sq);
                    if (y == RANK_TOP) {
                      x = FILE_X(sq);
                      if ((this->RankMaskPtr(x, y)->wRookCap & KING_BITRANK) != 0) {
                        vlBlackPenalty += 80;
                      }
                    }
                  }
                }
              }
            } else if (y == RANK_TOP) {
              if ((this->RankMaskPtr(x, y)->wRookCap & KING_BITRANK) != 0) {
                // ڵв
                vlBlackPenalty += PreEvalEx.vlBlackBottomThreat[x];
              }
            }
          }
        }
        break;
      default:
        break;
      }
    } else if (this->ucsqPieces[SIDE_TAG(1) + KING_FROM] == 0x47) {
      // ˫(ʿ)ı˧()ռ죬Ҫ
      vlBlackPenalty += 20;
    }
  } else {
    if ((this->wBitPiece[0] & ROOK_BITPIECE) == ROOK_BITPIECE) {
      // ȱ(ʿ)˫з
      vlBlackPenalty += PreEvalEx.vlBlackAdvisorLeakage;
    }
  }
  return SIDE_VALUE(this->sdPlayer, vlBlackPenalty - vlWhitePenalty);
}

// ǵһ֣͵

// ǵڶ֣ǣƵ

// "cnValuableStringPieces"жǣǷмֵ
// 0Ƕڳ˵ģǣ(ǣ)мֵ1Ƕ˵ֻǣмֵ
static const int cnValuableStringPieces[48] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 0, 0, 0
};

// "ccvlStringValueTab""KNIGHT_PIN_TAB"ĳ("pregen.h")ǣƼֵ
// мӺͱǣӵľԽǣƵļֵԽ
static const char ccvlStringValueTab[512] = {
                               0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 16,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 24,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 28,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 32,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 36,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 40,  0,  0,  0,  0,  0,  0,  0,  0,
  12, 16, 20, 24, 28, 32, 36,  0, 36, 32, 28, 24, 20, 16, 12,  0,
   0,  0,  0,  0,  0,  0,  0, 40,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 36,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 32,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 28,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 24,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 16,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0
};

// ǣ˧()򳵵͵
int PositionStruct::StringHold(void) const {
  int sd, i, j, nDir, sqSrc, sqDst, sqStr;
  int x, y, nSideTag, nOppSideTag;
  int vlString[2];
  SlideMoveStruct *lpsmv;

  for (sd = 0; sd < 2; sd ++) {
    vlString[sd] = 0;
    nSideTag = SIDE_TAG(sd);
    nOppSideTag = OPP_SIDE_TAG(sd);
    // óǣƵ
    for (i = ROOK_FROM; i <= ROOK_TO; i ++) {
      sqSrc = this->ucsqPieces[nSideTag + i];
      if (sqSrc != 0) {
        __ASSERT_SQUARE(sqSrc);
        // ǣĿ˧()
        sqDst = this->ucsqPieces[nOppSideTag + KING_FROM];
        if (sqDst != 0) {
          __ASSERT_SQUARE(sqDst);
          x = FILE_X(sqSrc);
          y = RANK_Y(sqSrc);
          if (x == FILE_X(sqDst)) {
            lpsmv = this->FileMovePtr(x, y);
            nDir = (sqSrc < sqDst ? 0 : 1);
            // ڵĳԷ(óڵŷ)ܳԵĿ"sqDst"ǣƾͳˣͬ
            if (sqDst == lpsmv->ucCannonCap[nDir] + FILE_DISP(x)) {
              // ǣ"sqStr"ǳ()ܳԵӣͬ
              sqStr = lpsmv->ucRookCap[nDir] + FILE_DISP(x);
              __ASSERT_SQUARE(sqStr);
              // ǣӱǶԷӣͬ
              if ((this->ucpcSquares[sqStr] & nOppSideTag) != 0) {
                // ǣмֵģұǣûб(Ŀӱ)ôǣмֵģͬ
                if (cnValuableStringPieces[this->ucpcSquares[sqStr]] > 0 &&
                    !this->Protected(OPP_SIDE(sd), sqStr, sqDst)) {
                  vlString[sd] += ccvlStringValueTab[sqDst - sqStr + 256];
                }
              }
            }
          } else if (y == RANK_Y(sqDst)) {
            lpsmv = this->RankMovePtr(x, y);
            nDir = (sqSrc < sqDst ? 0 : 1);
            if (sqDst == lpsmv->ucCannonCap[nDir] + RANK_DISP(y)) {
              sqStr = lpsmv->ucRookCap[nDir] + RANK_DISP(y);
              __ASSERT_SQUARE(sqStr);
              if ((this->ucpcSquares[sqStr] & nOppSideTag) != 0) {
                if (cnValuableStringPieces[this->ucpcSquares[sqStr]] > 0 &&
                    !this->Protected(OPP_SIDE(sd), sqStr, sqDst)) {
                  vlString[sd] += ccvlStringValueTab[sqDst - sqStr + 256];
                }
              }
            }
          }
        }
        // ǣĿǳ
        for (j = ROOK_FROM; j <= ROOK_TO; j ++) {
          sqDst = this->ucsqPieces[nOppSideTag + j];
          if (sqDst != 0) {
            __ASSERT_SQUARE(sqDst);
            x = FILE_X(sqSrc);
            y = RANK_Y(sqSrc);
            if (x == FILE_X(sqDst)) {
              lpsmv = this->FileMovePtr(x, y);
              nDir = (sqSrc < sqDst ? 0 : 1);
              if (sqDst == lpsmv->ucCannonCap[nDir] + FILE_DISP(x)) {
                sqStr = lpsmv->ucRookCap[nDir] + FILE_DISP(x);
                __ASSERT_SQUARE(sqStr);
                if ((this->ucpcSquares[sqStr] & nOppSideTag) != 0) {
                  // Ŀǳͬ˧()ҪҲûбʱǣƼֵͬ
                  if (cnValuableStringPieces[this->ucpcSquares[sqStr]] > 0 &&
                      !this->Protected(OPP_SIDE(sd), sqDst) && !this->Protected(OPP_SIDE(sd), sqStr, sqDst)) {
                    vlString[sd] += ccvlStringValueTab[sqDst - sqStr + 256];
                  }
                }
              }
            } else if (y == RANK_Y(sqDst)) {
              lpsmv = this->RankMovePtr(x, y);
              nDir = (sqSrc < sqDst ? 0 : 1);
              if (sqDst == lpsmv->ucCannonCap[nDir] + RANK_DISP(y)) {
                sqStr = lpsmv->ucRookCap[nDir] + RANK_DISP(y);
                __ASSERT_SQUARE(sqStr);
                if ((this->ucpcSquares[sqStr] & nOppSideTag) != 0) {
                  if (cnValuableStringPieces[this->ucpcSquares[sqStr]] > 0 &&
                      !this->Protected(OPP_SIDE(sd), sqDst) && !this->Protected(OPP_SIDE(sd), sqStr, sqDst)) {
                    vlString[sd] += ccvlStringValueTab[sqDst - sqStr + 256];
                  }
                }
              }
            }
          }
        }
      }
    }

    // ǣƵ
    for (i = CANNON_FROM; i <= CANNON_TO; i ++) {
      sqSrc = this->ucsqPieces[nSideTag + i];
      if (sqSrc != 0) {
        __ASSERT_SQUARE(sqSrc);
        // ǣĿ˧()
        sqDst = this->ucsqPieces[nOppSideTag + KING_FROM];
        if (sqDst != 0) {
          __ASSERT_SQUARE(sqDst);
          x = FILE_X(sqSrc);
          y = RANK_Y(sqSrc);
          if (x == FILE_X(sqDst)) {
            lpsmv = this->FileMovePtr(x, y);
            nDir = (sqSrc < sqDst ? 0 : 1);
            if (sqDst == lpsmv->ucSuperCap[nDir] + FILE_DISP(x)) {
              sqStr = lpsmv->ucCannonCap[nDir] + FILE_DISP(x);
              __ASSERT_SQUARE(sqStr);
              if ((this->ucpcSquares[sqStr] & nOppSideTag) != 0) {
                if (cnValuableStringPieces[this->ucpcSquares[sqStr]] > 1 &&
                    !this->Protected(OPP_SIDE(sd), sqStr, sqDst)) {
                  vlString[sd] += ccvlStringValueTab[sqDst - sqStr + 256];
                }
              }
            }
          } else if (y == RANK_Y(sqDst)) {
            lpsmv = this->RankMovePtr(x, y);
            nDir = (sqSrc < sqDst ? 0 : 1);
            if (sqDst == lpsmv->ucSuperCap[nDir] + RANK_DISP(y)) {
              sqStr = lpsmv->ucCannonCap[nDir] + RANK_DISP(y);
              __ASSERT_SQUARE(sqStr);
              if ((this->ucpcSquares[sqStr] & nOppSideTag) != 0) {
                if (cnValuableStringPieces[this->ucpcSquares[sqStr]] > 1 &&
                    !this->Protected(OPP_SIDE(sd), sqStr, sqDst)) {
                  vlString[sd] += ccvlStringValueTab[sqDst - sqStr + 256];
                }
              }
            }
          }
        }
        // ǣĿǳ
        for (j = ROOK_FROM; j <= ROOK_TO; j ++) {
          sqDst = this->ucsqPieces[nOppSideTag + j];
          if (sqDst != 0) {
            __ASSERT_SQUARE(sqDst);
            x = FILE_X(sqSrc);
            y = RANK_Y(sqSrc);
            if (x == FILE_X(sqDst)) {
              lpsmv = this->FileMovePtr(x, y);
              nDir = (sqSrc < sqDst ? 0 : 1);
              if (sqDst == lpsmv->ucSuperCap[nDir] + FILE_DISP(x)) {
                sqStr = lpsmv->ucCannonCap[nDir] + FILE_DISP(x);
                __ASSERT_SQUARE(sqStr);
                if ((this->ucpcSquares[sqStr] & nOppSideTag) != 0) {
                  if (cnValuableStringPieces[this->ucpcSquares[sqStr]] > 1 &&
                      !this->Protected(OPP_SIDE(sd), sqStr, sqDst)) {
                    vlString[sd] += ccvlStringValueTab[sqDst - sqStr + 256];
                  }
                }
              }
            } else if (y == RANK_Y(sqDst)) {
              lpsmv = this->RankMovePtr(x, y);
              nDir = (sqSrc < sqDst ? 0 : 1);
              if (sqDst == lpsmv->ucSuperCap[nDir] + RANK_DISP(y)) {
                sqStr = lpsmv->ucCannonCap[nDir] + RANK_DISP(y);
                __ASSERT_SQUARE(sqStr);
                if ((this->ucpcSquares[sqStr] & nOppSideTag) != 0) {
                  if (cnValuableStringPieces[this->ucpcSquares[sqStr]] > 1 &&
                      !this->Protected(OPP_SIDE(sd), sqStr, sqDst)) {
                    vlString[sd] += ccvlStringValueTab[sqDst - sqStr + 256];
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return SIDE_VALUE(this->sdPlayer, vlString[0] - vlString[1]);
}

// ǵڶ֣ǣƵ

// ǵ֣Ե

int PositionStruct::RookMobility(void) const {
  int sd, i, sqSrc, nSideTag, x, y;
  int vlRookMobility[2];
  for (sd = 0; sd < 2; sd ++) {
    vlRookMobility[sd] = 0;
    nSideTag = SIDE_TAG(sd);
    for (i = ROOK_FROM; i <= ROOK_TO; i ++) {
      sqSrc = this->ucsqPieces[nSideTag + i];
      if (sqSrc != 0) {
        __ASSERT_SQUARE(sqSrc);
        x = FILE_X(sqSrc);
        y = RANK_Y(sqSrc);
        vlRookMobility[sd] += PreEvalEx.cPopCnt16[this->RankMaskPtr(x, y)->wNonCap] +
            PreEvalEx.cPopCnt16[this->FileMaskPtr(x, y)->wNonCap];
      }
    }
    __ASSERT(vlRookMobility[sd] <= 34);
  }
  return SIDE_VALUE(this->sdPlayer, vlRookMobility[0] - vlRookMobility[1]) >> 1;
}

// ǵ֣Ե

// ǵĲ֣ܵ谭

// "cbcEdgeSquares"˲λã̱Եλõǻ
static const bool cbcEdgeSquares[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int PositionStruct::KnightTrap(void) const {
  int sd, i, sqSrc, sqDst, nSideTag, nMovable;
  uint8_t *lpucsqDst, *lpucsqPin;
  int vlKnightTraps[2];

  for (sd = 0; sd < 2; sd ++) {
    vlKnightTraps[sd] = 0;
    nSideTag = SIDE_TAG(sd);
    // ߵλãߵ̱ԵϣߵԷĿƸ񣬶ų
    for (i = KNIGHT_FROM; i <= KNIGHT_TO; i ++) {
      sqSrc = this->ucsqPieces[nSideTag + i];
      if (sqSrc != 0) {
        __ASSERT_SQUARE(sqSrc);
        nMovable = 0;
        lpucsqDst = PreGen.ucsqKnightMoves[sqSrc];
        lpucsqPin = PreGen.ucsqKnightPins[sqSrc];
        sqDst = *lpucsqDst;
        while (sqDst != 0) {
          __ASSERT_SQUARE(sqDst);
          // µж"genmoves.cpp"еŷųߵ̱ԵߵԷƸŷ
          if (!cbcEdgeSquares[sqDst] && this->ucpcSquares[sqDst] == 0 &&
              this->ucpcSquares[*lpucsqPin] == 0 && !this->Protected(OPP_SIDE(sd), sqDst)) {
            nMovable ++;
            if (nMovable > 1) {
              break;
            }
          }
          lpucsqDst ++;
          sqDst = *lpucsqDst;
          lpucsqPin ++;
        }
        // ûкõŷ10ַֻ֣һõŷ5ַ
        if (nMovable == 0) {
          vlKnightTraps[sd] += 10;
        } else if (nMovable == 1) {
          vlKnightTraps[sd] += 5;
        }
      }
      __ASSERT(vlKnightTraps[sd] <= 20);
    }
  }
  return SIDE_VALUE(this->sdPlayer, vlKnightTraps[1] - vlKnightTraps[0]);
}

// ǵĲ֣ܵ谭

// ۹
int PositionStruct::Evaluate(int vlAlpha, int vlBeta) const {
  int vl;
  // ͵ľۺ¼Σ

  // 1. ļ͵(͵)ֻƽ⣻
  vl = this->Material();
  if (vl + EVAL_MARGIN1 <= vlAlpha) {
    return vl + EVAL_MARGIN1;
  } else if (vl - EVAL_MARGIN1 >= vlBeta) {
    return vl - EVAL_MARGIN1;
  }

  // 2. ͵ۣͣ
  vl += this->AdvisorShape();
  if (vl + EVAL_MARGIN2 <= vlAlpha) {
    return vl + EVAL_MARGIN2;
  } else if (vl - EVAL_MARGIN2 >= vlBeta) {
    return vl - EVAL_MARGIN2;
  }

  // 3. ͵ۣǣƣ
  vl += this->StringHold();
  if (vl + EVAL_MARGIN3 <= vlAlpha) {
    return vl + EVAL_MARGIN3;
  } else if (vl - EVAL_MARGIN3 >= vlBeta) {
    return vl - EVAL_MARGIN3;
  }

  // 4. һ͵ۣԣ
  vl += this->RookMobility();
  if (vl + EVAL_MARGIN4 <= vlAlpha) {
    return vl + EVAL_MARGIN4;
  } else if (vl - EVAL_MARGIN4 >= vlBeta) {
    return vl - EVAL_MARGIN4;
  }

  // 5. 㼶͵(ȫ)谭
  return vl + this->KnightTrap();
}
// ===== END eleeye/evaluate.cpp =====

// ===== BEGIN eleeye/position.cpp =====
/*
position.h/position.cpp - Source Code for ElephantEye, Part III

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.0, Last Modified: Nov. 2007
Copyright (C) 2004-2007 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


/* ElephantEyeԴʹõǺԼ
 *
 * sq: (0255"pregen.cpp")
 * pc: (047"position.cpp")
 * pt: (06"position.cpp")
 * mv: ŷ(065535"position.cpp")
 * sd: ӷ(0췽1ڷ)
 * vl: ֵ("-MATE_VALUE""MATE_VALUE""position.cpp")
 * (עǺſucdwȴļǺʹ)
 * pos: (PositionStructͣ"position.h")
 * sms: λкλеŷԤýṹ("pregen.h")
 * smv: λкλеŷжԤýṹ("pregen.h")
 */

// ģ漰"PositionStruct"ĳԱ"this->"ǳԷ

// ʼFEN
const char *const cszStartFen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w";

// ͶӦӷ
const char *const cszPieceBytes = "KABNRCP";

/* ŶӦ
 *
 * ElephantEyeŴ047015ã1631ʾӣ3247ʾӡ
 * ÿ˳ǣ˧ڱ(ʿʿ)
 * ʾжǺ"pc < 32""pc >= 32"
 */
const int cnPieceTypes[48] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 6,
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 6
};

// ӵļ򵥷ֵֻڼ򵥱Ƚʱο
const int cnSimpleValues[48] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  5, 1, 1, 1, 1, 3, 3, 4, 4, 3, 3, 2, 2, 2, 2, 2,
  5, 1, 1, 1, 1, 3, 3, 4, 4, 3, 3, 2, 2, 2, 2, 2,
};

// ܷʵľ(ҶԳ)
const uint8_t cucsqMirrorTab[256] = {
  0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0,
  0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0,
  0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0,
  0, 0, 0, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0, 0, 0, 0,
  0, 0, 0, 0x4b, 0x4a, 0x49, 0x48, 0x47, 0x46, 0x45, 0x44, 0x43, 0, 0, 0, 0,
  0, 0, 0, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53, 0, 0, 0, 0,
  0, 0, 0, 0x6b, 0x6a, 0x69, 0x68, 0x67, 0x66, 0x65, 0x64, 0x63, 0, 0, 0, 0,
  0, 0, 0, 0x7b, 0x7a, 0x79, 0x78, 0x77, 0x76, 0x75, 0x74, 0x73, 0, 0, 0, 0,
  0, 0, 0, 0x8b, 0x8a, 0x89, 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0, 0, 0, 0,
  0, 0, 0, 0x9b, 0x9a, 0x99, 0x98, 0x97, 0x96, 0x95, 0x94, 0x93, 0, 0, 0, 0,
  0, 0, 0, 0xab, 0xaa, 0xa9, 0xa8, 0xa7, 0xa6, 0xa5, 0xa4, 0xa3, 0, 0, 0, 0,
  0, 0, 0, 0xbb, 0xba, 0xb9, 0xb8, 0xb7, 0xb6, 0xb5, 0xb4, 0xb3, 0, 0, 0, 0,
  0, 0, 0, 0xcb, 0xca, 0xc9, 0xc8, 0xc7, 0xc6, 0xc5, 0xc4, 0xc3, 0, 0, 0, 0,
  0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0,
  0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0,
  0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0,
};

// FENӱʶעֻʶдĸСдĸʱȱתΪд
int FenPiece(int nArg) {
  switch (nArg) {
  case 'K':
    return 0;
  case 'A':
    return 1;
  case 'B':
  case 'E':
    return 2;
  case 'N':
  case 'H':
    return 3;
  case 'R':
    return 4;
  case 'C':
    return 5;
  case 'P':
    return 6;
  default:
    return 7;
  }
}

// һЩ̴

// 
void PositionStruct::AddPiece(int sq, int pc, bool bDel) {
  int pt;

  __ASSERT_SQUARE(sq);
  __ASSERT_PIECE(pc);
  if (bDel) {
    this->ucpcSquares[sq] = 0;
    this->ucsqPieces[pc] = 0;
  } else {
    this->ucpcSquares[sq] = pc;
    this->ucsqPieces[pc] = sq;
  }
  this->wBitRanks[RANK_Y(sq)] ^= PreGen.wBitRankMask[sq];
  this->wBitFiles[FILE_X(sq)] ^= PreGen.wBitFileMask[sq];
  __ASSERT_BITRANK(this->wBitRanks[RANK_Y(sq)]);
  __ASSERT_BITFILE(this->wBitRanks[FILE_X(sq)]);
  this->dwBitPiece ^= BIT_PIECE(pc);
  pt = PIECE_TYPE(pc);
  if (pc < 32) {
    if (bDel) {
      this->vlWhite -= PreEval.ucvlWhitePieces[pt][sq];
    } else {
      this->vlWhite += PreEval.ucvlWhitePieces[pt][sq];
    }
  } else {
    if (bDel) {
      this->vlBlack -= PreEval.ucvlBlackPieces[pt][sq];
    } else {
      this->vlBlack += PreEval.ucvlBlackPieces[pt][sq];
    }
    pt += 7;
  }
  __ASSERT_BOUND(0, pt, 13);
  this->zobr.Xor(PreGen.zobrTable[pt][sq]);
}

// ƶ
int PositionStruct::MovePiece(int mv) {
  int sqSrc, sqDst, pcMoved, pcCaptured, pt;
  uint8_t *lpucvl;
  // ƶӰ¼裺

  // 1. õƶźͱԵţ
  sqSrc = SRC(mv);
  sqDst = DST(mv);
  pcMoved = this->ucpcSquares[sqSrc];
  __ASSERT_SQUARE(sqSrc);
  __ASSERT_SQUARE(sqDst);
  __ASSERT_PIECE(pcMoved);
  pcCaptured = this->ucpcSquares[sqDst];
  if (pcCaptured == 0) {

    // 2. ûбԵӣôĿλкλС
    //    仰˵бԵӣĿλкλоͲظˡ
    this->wBitRanks[RANK_Y(sqDst)] ^= PreGen.wBitRankMask[sqDst];
    this->wBitFiles[FILE_X(sqDst)] ^= PreGen.wBitFileMask[sqDst];
    __ASSERT_BITRANK(this->wBitRanks[RANK_Y(sqDst)]);
    __ASSERT_BITFILE(this->wBitRanks[FILE_X(sqDst)]);
  } else {

    __ASSERT_PIECE(pcCaptured);
    // 3. бԵӣôλ"ucsqPieces"
    //    ͬʱֵλλСZobristֵУ
    this->ucsqPieces[pcCaptured] = 0;
    this->dwBitPiece ^= BIT_PIECE(pcCaptured);
    pt = PIECE_TYPE(pcCaptured);
    if (pcCaptured < 32) {
      this->vlWhite -= PreEval.ucvlWhitePieces[pt][sqDst];
    } else {
      this->vlBlack -= PreEval.ucvlBlackPieces[pt][sqDst];
      pt += 7;
    }
    __ASSERT_BOUND(0, pt, 13);
    this->zobr.Xor(PreGen.zobrTable[pt][sqDst]);
  }

  // 4. "ucpcSquares""ucsqPieces"ƶӣע⡰-ϵ顱ƶӵķ
  //    ͬʱλСλСֵλλСZobristֵУ
  this->ucpcSquares[sqSrc] = 0;
  this->ucpcSquares[sqDst] = pcMoved;
  this->ucsqPieces[pcMoved] = sqDst;
  this->wBitRanks[RANK_Y(sqSrc)] ^= PreGen.wBitRankMask[sqSrc];
  this->wBitFiles[FILE_X(sqSrc)] ^= PreGen.wBitFileMask[sqSrc];
  __ASSERT_BITRANK(this->wBitRanks[RANK_Y(sqSrc)]);
  __ASSERT_BITFILE(this->wBitRanks[FILE_X(sqSrc)]);
  pt = PIECE_TYPE(pcMoved);
  if (pcMoved < 32) {
    lpucvl = PreEval.ucvlWhitePieces[pt];
    this->vlWhite += lpucvl[sqDst] - lpucvl[sqSrc];
  } else {
    lpucvl = PreEval.ucvlBlackPieces[pt];
    this->vlBlack += lpucvl[sqDst] - lpucvl[sqSrc];
    pt += 7;
  }
  __ASSERT_BOUND(0, pt, 13);
  this->zobr.Xor(PreGen.zobrTable[pt][sqDst], PreGen.zobrTable[pt][sqSrc]);
  return pcCaptured;
}

// ƶ
void PositionStruct::UndoMovePiece(int mv, int pcCaptured) {
  int sqSrc, sqDst, pcMoved;
  sqSrc = SRC(mv);
  sqDst = DST(mv);
  pcMoved = this->ucpcSquares[sqDst];
  __ASSERT_SQUARE(sqSrc);
  __ASSERT_SQUARE(sqDst);
  __ASSERT_PIECE(pcMoved);
  this->ucpcSquares[sqSrc] = pcMoved;
  this->ucsqPieces[pcMoved] = sqSrc;
  this->wBitRanks[RANK_Y(sqSrc)] ^= PreGen.wBitRankMask[sqSrc];
  this->wBitFiles[FILE_X(sqSrc)] ^= PreGen.wBitFileMask[sqSrc];
  __ASSERT_BITRANK(this->wBitRanks[RANK_Y(sqSrc)]);
  __ASSERT_BITFILE(this->wBitRanks[FILE_X(sqSrc)]);
  if (pcCaptured > 0) {
    __ASSERT_PIECE(pcCaptured);
    this->ucpcSquares[sqDst] = pcCaptured;
    this->ucsqPieces[pcCaptured] = sqDst;
    this->dwBitPiece ^= BIT_PIECE(pcCaptured);
  } else {
    this->ucpcSquares[sqDst] = 0;
    this->wBitRanks[RANK_Y(sqDst)] ^= PreGen.wBitRankMask[sqDst];
    this->wBitFiles[FILE_X(sqDst)] ^= PreGen.wBitFileMask[sqDst];
    __ASSERT_BITRANK(this->wBitRanks[RANK_Y(sqDst)]);
    __ASSERT_BITFILE(this->wBitRanks[FILE_X(sqDst)]);
  }
}

// 
int PositionStruct::Promote(int sq) {
  int pcCaptured, pcPromoted, pt;
  // ¼裺

  // 1. õǰӵţ
  __ASSERT_SQUARE(sq);
  __ASSERT(CanPromote());
  __ASSERT(CAN_PROMOTE(sq));
  pcCaptured = this->ucpcSquares[sq];
  __ASSERT_PIECE(pcCaptured);
  pcPromoted = SIDE_TAG(this->sdPlayer) + Bsf(~this->wBitPiece[this->sdPlayer] & PAWN_BITPIECE);
  __ASSERT_PIECE(pcPromoted);
  __ASSERT(this->ucsqPieces[pcPromoted] == 0);

  // 2. ȥǰӣͬʱֵZobristֵУ
  this->dwBitPiece ^= BIT_PIECE(pcPromoted) ^ BIT_PIECE(pcCaptured);
  this->ucsqPieces[pcCaptured] = 0;
  pt = PIECE_TYPE(pcCaptured);
  if (pcCaptured < 32) {
    this->vlWhite -= PreEval.ucvlWhitePieces[pt][sq];
  } else {
    this->vlBlack -= PreEval.ucvlBlackPieces[pt][sq];
    pt += 7;
  }
  __ASSERT_BOUND(0, pt, 13);
  this->zobr.Xor(PreGen.zobrTable[pt][sq]);

  // 3. ӣͬʱֵZobristֵУ
  this->ucpcSquares[sq] = pcPromoted;
  this->ucsqPieces[pcPromoted] = sq;
  pt = PIECE_TYPE(pcPromoted);
  if (pcPromoted < 32) {
    this->vlWhite += PreEval.ucvlWhitePieces[pt][sq];
  } else {
    this->vlBlack += PreEval.ucvlBlackPieces[pt][sq];
    pt += 7;
  }
  __ASSERT_BOUND(0, pt, 13);
  this->zobr.Xor(PreGen.zobrTable[pt][sq]);
  return pcCaptured;
}

// 
void PositionStruct::UndoPromote(int sq, int pcCaptured) {
  int pcPromoted;
  __ASSERT_SQUARE(sq);
  __ASSERT_PIECE(pcCaptured);
  pcPromoted = this->ucpcSquares[sq];
  __ASSERT(PIECE_TYPE(pcPromoted) == 6);
  this->ucsqPieces[pcPromoted] = 0;
  this->ucpcSquares[sq] = pcCaptured;
  this->ucsqPieces[pcCaptured] = sq;
  this->dwBitPiece ^= BIT_PIECE(pcPromoted) ^ BIT_PIECE(pcCaptured);
}

// һЩ̴

// һЩŷ

// ִһŷ
bool PositionStruct::MakeMove(int mv) {
  int sq, pcCaptured;
  uint32_t dwOldZobristKey;
  RollbackStruct *lprbs;

  // ﵽŷôжΪǷŷ
  if (this->nMoveNum == MAX_MOVE_NUM) {
    return false;
  }
  __ASSERT(this->nMoveNum < MAX_MOVE_NUM);
  // ִһŷҪ¼裺

  // 1. ԭZobristֵ
  dwOldZobristKey = this->zobr.dwKey;
  SaveStatus();

  // 2. ƶӣסԵ(еĻ)
  sq = SRC(mv);
  if (sq == DST(mv)) {
    pcCaptured = Promote(sq);
  } else {
    pcCaptured = MovePiece(mv);

    // 3. ƶ󱻽ˣôŷǷǷģŷ
    if (CheckedBy(CHECK_LAZY) > 0) {
      UndoMovePiece(mv, pcCaptured);
      Rollback();
      return false;
    }
  }

  // 4. ӷ
  ChangeSide();

  // 5. ԭZobristֵ¼ظû
  if (this->ucRepHash[dwOldZobristKey & REP_HASH_MASK] == 0) {
    this->ucRepHash[dwOldZobristKey & REP_HASH_MASK] = this->nMoveNum;
  }

  // 6. ŷ浽ʷŷбУסԵӺͽ״̬
  lprbs = this->rbsList + this->nMoveNum;
  lprbs->mvs.wmv = mv;
  lprbs->mvs.ChkChs = CheckedBy();

  // 7. úŷ(Ӧ)
  if (pcCaptured == 0) {
    if (lprbs->mvs.ChkChs == 0) {
      lprbs->mvs.ChkChs = -ChasedBy(mv);
    }
    if (LastMove().CptDrw == -100) {
      lprbs->mvs.CptDrw = -100;
    } else {
      lprbs->mvs.CptDrw = MIN((int) LastMove().CptDrw, 0) - (lprbs->mvs.ChkChs > 0 || LastMove().ChkChs > 0 ? 0 : 1);
    }
    __ASSERT_BOUND(-100, lprbs->mvs.CptDrw, 0);
  } else {
    lprbs->mvs.CptDrw = pcCaptured;
    __ASSERT_PIECE(pcCaptured);
  }
  this->nMoveNum ++;
  this->nDistance ++;

  return true;
}

// һŷ
void PositionStruct::UndoMakeMove(void) {
  int sq;
  RollbackStruct *lprbs;
  this->nMoveNum --;
  this->nDistance --;
  lprbs = this->rbsList + this->nMoveNum;
  sq = SRC(lprbs->mvs.wmv);
  if (sq == DST(lprbs->mvs.wmv)) {
    __ASSERT_BOUND(ADVISOR_TYPE, PIECE_TYPE(lprbs->mvs.CptDrw), BISHOP_TYPE);
    UndoPromote(sq, lprbs->mvs.CptDrw);
  } else {
    UndoMovePiece(lprbs->mvs.wmv, lprbs->mvs.CptDrw);
  }
  this->sdPlayer = OPP_SIDE(this->sdPlayer);
  Rollback();
  if (this->ucRepHash[this->zobr.dwKey & REP_HASH_MASK] == this->nMoveNum) {
    this->ucRepHash[this->zobr.dwKey & REP_HASH_MASK] = 0;
  }
  __ASSERT(this->nMoveNum > 0);
}

// ִһ
void PositionStruct::NullMove(void) {
  __ASSERT(this->nMoveNum < MAX_MOVE_NUM);
  if (this->ucRepHash[this->zobr.dwKey & REP_HASH_MASK] == 0) {
    this->ucRepHash[this->zobr.dwKey & REP_HASH_MASK] = this->nMoveNum;
  }
  SaveStatus();
  ChangeSide();
  this->rbsList[nMoveNum].mvs.dwmv = 0; // wmv, Chk, CptDrw, ChkChs = 0
  this->nMoveNum ++;
  this->nDistance ++;
}

// һ
void PositionStruct::UndoNullMove(void) {
  this->nMoveNum --;
  this->nDistance --;
  this->sdPlayer = OPP_SIDE(this->sdPlayer);
  Rollback();
  if (this->ucRepHash[this->zobr.dwKey & REP_HASH_MASK] == this->nMoveNum) {
    this->ucRepHash[this->zobr.dwKey & REP_HASH_MASK] = 0;
  }
  __ASSERT(this->nMoveNum > 0);
}

// һЩŷ

// һЩ洦

// FENʶ
void PositionStruct::FromFen(const char *szFen) {
  int i, j, k;
  int pcWhite[7];
  int pcBlack[7];
  const char *lpFen;
  // FENʶ¼裺
  // 1. ʼ
  pcWhite[0] = SIDE_TAG(0) + KING_FROM;
  pcWhite[1] = SIDE_TAG(0) + ADVISOR_FROM;
  pcWhite[2] = SIDE_TAG(0) + BISHOP_FROM;
  pcWhite[3] = SIDE_TAG(0) + KNIGHT_FROM;
  pcWhite[4] = SIDE_TAG(0) + ROOK_FROM;
  pcWhite[5] = SIDE_TAG(0) + CANNON_FROM;
  pcWhite[6] = SIDE_TAG(0) + PAWN_FROM;
  for (i = 0; i < 7; i ++) {
    pcBlack[i] = pcWhite[i] + 16;
  }
  /* "pcWhite[7]""pcBlack[7]"ֱ췽ͺڷÿּռеţ
   * "pcWhite[7]"Ϊ1631δ˧ڱ
   * Ӧ"pcWhite[7] = {16, 17, 19, 21, 23, 25, 27}"ÿһӣ1
   * Ӷ(ӵڶ˧ͱ)ǰҪ߽
   */
  ClearBoard();
  lpFen = szFen;
  if (*lpFen == '\0') {
    SetIrrev();
    return;
  }
  // 2. ȡϵ
  i = RANK_TOP;
  j = FILE_LEFT;
  while (*lpFen != ' ') {
    if (*lpFen == '/') {
      j = FILE_LEFT;
      i ++;
      if (i > RANK_BOTTOM) {
        break;
      }
    } else if (*lpFen >= '1' && *lpFen <= '9') {
      j += (*lpFen - '0');
    } else if (*lpFen >= 'A' && *lpFen <= 'Z') {
      if (j <= FILE_RIGHT) {
        k = FenPiece(*lpFen);
        if (k < 7) {
          if (pcWhite[k] < 32) {
            if (this->ucsqPieces[pcWhite[k]] == 0) {
              AddPiece(COORD_XY(j, i), pcWhite[k]);
              pcWhite[k] ++;
            }
          }
        }
        j ++;
      }
    } else if (*lpFen >= 'a' && *lpFen <= 'z') {
      if (j <= FILE_RIGHT) {
        k = FenPiece(*lpFen + 'A' - 'a');
        if (k < 7) {
          if (pcBlack[k] < 48) {
            if (this->ucsqPieces[pcBlack[k]] == 0) {
              AddPiece(COORD_XY(j, i), pcBlack[k]);
              pcBlack[k] ++;
            }
          }
        }
        j ++;
      }
    }
    lpFen ++;
    if (*lpFen == '\0') {
      SetIrrev();
      return;
    }
  }
  lpFen ++;
  // 3. ȷֵķ
  if (*lpFen == 'b') {
    ChangeSide();
  }
  // 4. Ѿɡ桱
  SetIrrev();
}

// FEN
void PositionStruct::ToFen(char *szFen) const {
  int i, j, k, pc;
  char *lpFen;

  lpFen = szFen;
  for (i = RANK_TOP; i <= RANK_BOTTOM; i ++) {
    k = 0;
    for (j = FILE_LEFT; j <= FILE_RIGHT; j ++) {
      pc = this->ucpcSquares[COORD_XY(j, i)];
      if (pc != 0) {
        if (k > 0) {
          *lpFen = k + '0';
          lpFen ++;
          k = 0;
        }
        *lpFen = PIECE_BYTE(PIECE_TYPE(pc)) + (pc < 32 ? 0 : 'a' - 'A');
        lpFen ++;
      } else {
        k ++;
      }
    }
    if (k > 0) {
      *lpFen = k + '0';
      lpFen ++;
    }
    *lpFen = '/';
    lpFen ++;
  }
  *(lpFen - 1) = ' '; // һ'/'滻' '
  *lpFen = (this->sdPlayer == 0 ? 'w' : 'b');
  lpFen ++;
  *lpFen = '\0';
}

// 澵
void PositionStruct::Mirror(void) {
  int i, sq, nMoveNumSave;
  uint16_t wmvList[MAX_MOVE_NUM];
  uint8_t ucsqList[32];
  // 澵Ҫ²νУ

  // 1. ¼ʷŷ
  nMoveNumSave = this->nMoveNum;
  for (i = 1; i < nMoveNumSave; i ++) {
    wmvList[i] = this->rbsList[i].mvs.wmv;
  }

  // 2. ŷ
  for (i = 1; i < nMoveNumSave; i ++) {
    UndoMakeMove();
  }

  // 3. Ӵߣλü¼"ucsqList"飻
  for (i = 16; i < 48; i ++) {
    sq = this->ucsqPieces[i];
    ucsqList[i - 16] = sq;
    if (sq != 0) {
      AddPiece(sq, i, DEL_PIECE);
    }
  }

  // 4. ߵӰվλ·ŵϣ
  for (i = 16; i < 48; i ++) {
    sq = ucsqList[i - 16];
    if (sq != 0) {
      AddPiece(SQUARE_MIRROR(sq), i);
    }
  }

  // 6. ԭŷ
  SetIrrev();
  for (i = 1; i < nMoveNumSave; i ++) {
    MakeMove(MOVE_MIRROR(wmvList[i]));
  }
}

// һЩ洦

// һЩŷ

// ŷԼ⣬ڡɱŷļ
bool PositionStruct::LegalMove(int mv) const {
  int sqSrc, sqDst, sqPin, pcMoved, pcCaptured, x, y, nSideTag;
  // ŷԼ¼裺

  // 1. ҪߵǷ
  nSideTag = SIDE_TAG(this->sdPlayer);
  sqSrc = SRC(mv);
  sqDst = DST(mv);
  pcMoved = this->ucpcSquares[sqSrc];
  if ((pcMoved & nSideTag) == 0) {
    return false;
  }
  __ASSERT_SQUARE(sqSrc);
  __ASSERT_SQUARE(sqDst);
  __ASSERT_PIECE(pcMoved);

  // 2. ԵǷΪԷ(гӲûĻ)
  pcCaptured = this->ucpcSquares[sqDst];
  if (sqSrc != sqDst && (pcCaptured & nSideTag) != 0) {
    return false;
  }
  __ASSERT_BOUND(0, PIECE_INDEX(pcMoved), 15);
  switch (PIECE_INDEX(pcMoved)) {

  // 3. ˧()(ʿ)ȿǷھŹڣٿǷǺλ
  case KING_FROM:
    return IN_FORT(sqDst) && KING_SPAN(sqSrc, sqDst);
  case ADVISOR_FROM:
  case ADVISOR_TO:
    if (sqSrc == sqDst) {
      // 䣬ڵ߲ұ()ȫʱſ
      return CAN_PROMOTE(sqSrc) && CanPromote();
    } else {
      return IN_FORT(sqDst) && ADVISOR_SPAN(sqSrc, sqDst);
    }

  // 4. ()ȿǷӣٿǷǺλƣûб
  case BISHOP_FROM:
  case BISHOP_TO:
    if (sqSrc == sqDst) {
      // 䣬ڵ߲ұ()ȫʱſ
      return CAN_PROMOTE(sqSrc) && CanPromote();
    } else {
      return SAME_HALF(sqSrc, sqDst) && BISHOP_SPAN(sqSrc, sqDst) && this->ucpcSquares[BISHOP_PIN(sqSrc, sqDst)] == 0;
    }

  // 5. ȿǷǺλƣٿûб
  case KNIGHT_FROM:
  case KNIGHT_TO:
    sqPin = KNIGHT_PIN(sqSrc, sqDst);
    return sqPin != sqSrc && this->ucpcSquares[sqPin] == 0;

  // 6. ǳȿǺƶƶٶȡλлλеŷԤ
  case ROOK_FROM:
  case ROOK_TO:
    x = FILE_X(sqSrc);
    y = RANK_Y(sqSrc);
    if (x == FILE_X(sqDst)) {
      if (pcCaptured == 0) {
        return (FileMaskPtr(x, y)->wNonCap & PreGen.wBitFileMask[sqDst]) != 0;
      } else {
        return (FileMaskPtr(x, y)->wRookCap & PreGen.wBitFileMask[sqDst]) != 0;
      }
    } else if (y == RANK_Y(sqDst)) {
      if (pcCaptured == 0) {
        return (RankMaskPtr(x, y)->wNonCap & PreGen.wBitRankMask[sqDst]) != 0;
      } else {
        return (RankMaskPtr(x, y)->wRookCap & PreGen.wBitRankMask[sqDst]) != 0;
      }
    } else {
      return false;
    }

  // 7. ڣжͳһ
  case CANNON_FROM:
  case CANNON_TO:
    x = FILE_X(sqSrc);
    y = RANK_Y(sqSrc);
    if (x == FILE_X(sqDst)) {
      if (pcCaptured == 0) {
        return (FileMaskPtr(x, y)->wNonCap & PreGen.wBitFileMask[sqDst]) != 0;
      } else {
        return (FileMaskPtr(x, y)->wCannonCap & PreGen.wBitFileMask[sqDst]) != 0;
      }
    } else if (y == RANK_Y(sqDst)) {
      if (pcCaptured == 0) {
        return (RankMaskPtr(x, y)->wNonCap & PreGen.wBitRankMask[sqDst]) != 0;
      } else {
        return (RankMaskPtr(x, y)->wCannonCap & PreGen.wBitRankMask[sqDst]) != 0;
      }
    } else {
      return false;
    }

  // 8. Ǳ()򰴺췽ͺڷ
  default:
    if (AWAY_HALF(sqDst, this->sdPlayer) && (sqDst == sqSrc - 1 || sqDst == sqSrc + 1)) {
      return true;
    } else {
      return sqDst == SQUARE_FORWARD(sqSrc, this->sdPlayer);
    }
  }
}

// 
int PositionStruct::CheckedBy(bool bLazy) const {
  int pcCheckedBy, i, sqSrc, sqDst, sqPin, pc, x, y, nOppSideTag;
  SlideMaskStruct *lpsmsRank, *lpsmsFile;

  pcCheckedBy = 0;
  nOppSideTag = OPP_SIDE_TAG(this->sdPlayer);
  // жϰ¼ݣ

  // 1. ж˧()Ƿ
  sqSrc = this->ucsqPieces[SIDE_TAG(this->sdPlayer)];
  if (sqSrc == 0) {
    return 0;
  }
  __ASSERT_SQUARE(sqSrc);

  // 2. ˧()ڸӵλкλ
  x = FILE_X(sqSrc);
  y = RANK_Y(sqSrc);
  lpsmsRank = RankMaskPtr(x, y);
  lpsmsFile = FileMaskPtr(x, y);

  // 3. жǷ˧
  sqDst = this->ucsqPieces[nOppSideTag + KING_FROM];
  if (sqDst != 0) {
    __ASSERT_SQUARE(sqDst);
    if (x == FILE_X(sqDst) && (lpsmsFile->wRookCap & PreGen.wBitFileMask[sqDst]) != 0) {
      return CHECK_MULTI;
    }
  }

  // 4. жǷ
  for (i = KNIGHT_FROM; i <= KNIGHT_TO; i ++) {
    sqDst = this->ucsqPieces[nOppSideTag + i];
    if (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      sqPin = KNIGHT_PIN(sqDst, sqSrc); // ע⣬sqSrcsqDstǷģ
      if (sqPin != sqDst && this->ucpcSquares[sqPin] == 0) {
        if (bLazy || pcCheckedBy > 0) {
          return CHECK_MULTI;
        }
        pcCheckedBy = nOppSideTag + i;
        __ASSERT_PIECE(pcCheckedBy);
      }
    }
  }

  // 5. жǷ񱻳˧
  for (i = ROOK_FROM; i <= ROOK_TO; i ++) {
    sqDst = this->ucsqPieces[nOppSideTag + i];
    if (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      if (x == FILE_X(sqDst)) {
        if ((lpsmsFile->wRookCap & PreGen.wBitFileMask[sqDst]) != 0) {
          if (bLazy || pcCheckedBy > 0) {
            return CHECK_MULTI;
          }
          pcCheckedBy = nOppSideTag + i;
          __ASSERT_PIECE(pcCheckedBy);
        }
      } else if (y == RANK_Y(sqDst)) {
        if ((lpsmsRank->wRookCap & PreGen.wBitRankMask[sqDst]) != 0) {
          if (bLazy || pcCheckedBy > 0) {
            return CHECK_MULTI;
          }
          pcCheckedBy = nOppSideTag + i;
          __ASSERT_PIECE(pcCheckedBy);
        }
      }
    }
  }

  // 6. жǷڽ
  for (i = CANNON_FROM; i <= CANNON_TO; i ++) {
    sqDst = this->ucsqPieces[nOppSideTag + i];
    if (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      if (x == FILE_X(sqDst)) {
        if ((lpsmsFile->wCannonCap & PreGen.wBitFileMask[sqDst]) != 0) {
          if (bLazy || pcCheckedBy > 0) {
            return CHECK_MULTI;
          }
          pcCheckedBy = nOppSideTag + i;
          __ASSERT_PIECE(pcCheckedBy);
        }
      } else if (y == RANK_Y(sqDst)) {
        if ((lpsmsRank->wCannonCap & PreGen.wBitRankMask[sqDst]) != 0) {
          if (bLazy || pcCheckedBy > 0) {
            return CHECK_MULTI;
          }
          pcCheckedBy = nOppSideTag + i;
          __ASSERT_PIECE(pcCheckedBy);
        }
      }
    }
  }

  // 7. жǷ񱻱()
  for (sqDst = sqSrc - 1; sqDst <= sqSrc + 1; sqDst += 2) {
    // ˧()ڱ(ElephantEye)ôԲ
    // __ASSERT_SQUARE(sqDst);
    pc = this->ucpcSquares[sqDst];
    if ((pc & nOppSideTag) != 0 && PIECE_INDEX(pc) >= PAWN_FROM) {
      if (bLazy || pcCheckedBy > 0) {
        return CHECK_MULTI;
      }
      pcCheckedBy = nOppSideTag + i;
      __ASSERT_PIECE(pcCheckedBy);
    }
  }
  pc = this->ucpcSquares[SQUARE_FORWARD(sqSrc, this->sdPlayer)];
  if ((pc & nOppSideTag) != 0 && PIECE_INDEX(pc) >= PAWN_FROM) {
    if (bLazy || pcCheckedBy > 0) {
      return CHECK_MULTI;
    }
    pcCheckedBy = nOppSideTag + i;
    __ASSERT_PIECE(pcCheckedBy);
  }
  return pcCheckedBy;
}

// жǷ񱻽
bool PositionStruct::IsMate(void) {
  int i, nGenNum;
  MoveStruct mvsGen[MAX_GEN_MOVES];
  nGenNum = GenCapMoves(mvsGen);
  for (i = 0; i < nGenNum; i ++) {
    if (MakeMove(mvsGen[i].wmv)) {
      UndoMakeMove();
      return false;
    }
  }
  // ŷɷԽԼʱ
  nGenNum = GenNonCapMoves(mvsGen);
  for (i = 0; i < nGenNum; i ++) {
    if (MakeMove(mvsGen[i].wmv)) {
      UndoMakeMove();
      return false;
    }
  }
  return true;
}

// ý״̬λ
inline void SetPerpCheck(uint32_t &dwPerpCheck, int nChkChs) {
  if (nChkChs == 0) {
    dwPerpCheck = 0;
  } else if (nChkChs > 0) {
    dwPerpCheck &= 0x10000;
  } else {
    dwPerpCheck &= (1 << -nChkChs);
  }
}

// ظ
int PositionStruct::RepStatus(int nRecur) const {
  // "nRecur"ָظȡ1Ч(Ĭֵ)㴦ȡ3Ӧ
  int sd;
  uint32_t dwPerpCheck, dwOppPerpCheck;
  const RollbackStruct *lprbs;
  /* ظ¼裺
   *
   * 1. жϼظûǷеǰ棬ûпܣòж
   *    û"ucRepHash"ElephantEyeһɫÿִһŷʱͻûм¼µǰ"nMoveNum"
   *    ûѾ棬Ͳظˣ"MakeMove()"
   *    ˳ŷʱֻҪûֵǷڵǰ"nMoveNum"ո
   *    ڵǰ"nMoveNum"˵֮ǰоռûո"position.h"е"UndoMakeMove()"
   */
  if (this->ucRepHash[this->zobr.dwKey & REP_HASH_MASK] == 0) {
    return REP_NONE;
  }

  // 2. ʼ
  sd = OPP_SIDE(this->sdPlayer);
  dwPerpCheck = dwOppPerpCheck = 0x1ffff;
  lprbs = this->rbsList + this->nMoveNum - 1;

  // 3. һŷǿŻŷͲظ
  while (lprbs->mvs.wmv != 0 && lprbs->mvs.CptDrw <= 0) {
    __ASSERT(lprbs >= this->rbsList);

    // 4. ж˫ĳ򼶱0ʾ޳0xffffʾ׽0x10000ʾ
    if (sd == this->sdPlayer) {
      SetPerpCheck(dwPerpCheck, lprbs->mvs.ChkChs);

      // 5. Ѱظ棬ظﵽԤ򷵻ظǺ
      if (lprbs->zobr.dwLock0 == this->zobr.dwLock0 && lprbs->zobr.dwLock1 == this->zobr.dwLock1) {
        nRecur --;
        if (nRecur == 0) {
          dwPerpCheck = ((dwPerpCheck & 0xffff) == 0 ? dwPerpCheck : 0xffff);
          dwOppPerpCheck = ((dwOppPerpCheck & 0xffff) == 0 ? dwOppPerpCheck : 0xffff);
          return dwPerpCheck > dwOppPerpCheck ? REP_LOSS : dwPerpCheck < dwOppPerpCheck ? REP_WIN : REP_DRAW;
        }
      }

    } else {
      SetPerpCheck(dwOppPerpCheck, lprbs->mvs.ChkChs);
    }

    sd = OPP_SIDE(sd);
    lprbs --;
  }
  return REP_NONE;
}

// һЩŷ
// ===== END eleeye/position.cpp =====

// ===== BEGIN eleeye/search.cpp =====
/*
search.h/search.cpp - Source Code for ElephantEye, Part VIII

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.32, Last Modified: May 2012
Copyright (C) 2004-2012 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef CCHESS_A3800
  #include <stdio.h>
#endif
#ifndef CCHESS_A3800
  #include "ucci.h"
  #include "book.h"
#endif

const int IID_DEPTH = 2;         // ڲ
const int SMP_DEPTH = 6;         // 
const int UNCHANGED_DEPTH = 4;   // δıŷ

const int DROPDOWN_VALUE = 20;   // ķֵ
const int RESIGN_VALUE = 300;    // ķֵ
const int DRAW_OFFER_VALUE = 40; // ͵ķֵ

SearchStruct Search;

// ϢǷװģڲ
static struct {
  int64_t llTime;                     // ʱ
  bool bStop, bPonderStop;            // ֹźźͺ̨˼Ϊֹź
  bool bPopPv, bPopCurrMove;          // Ƿpvcurrmove
  int nPopDepth, vlPopValue;          // Ⱥͷֵ
  int nAllNodes, nMainNodes;          // ܽĽ
  int nUnchanged;                     // δıŷ
  uint16_t wmvPvLine[MAX_MOVE_NUM];   // Ҫ·ϵŷб
  uint16_t wmvKiller[LIMIT_DEPTH][2]; // ɱŷ
  MoveSortStruct MoveSort;            // ŷ
} Search2;

#ifndef CCHESS_A3800

void BuildPos(PositionStruct &pos, const UcciCommStruct &UcciComm) {
  int i, mv;
  pos.FromFen(UcciComm.szFenStr);
  for (i = 0; i < UcciComm.nMoveNum; i ++) {
    mv = COORD_MOVE(UcciComm.lpdwMovesCoord[i]);
    if (mv == 0) {
      break;
    }
    if (pos.LegalMove(mv) && pos.MakeMove(mv) && pos.LastMove().CptDrw > 0) {
      // ʼpos.nMoveNumӳûӵĲ
      pos.SetIrrev();
    }
  }
}

#endif

// ж
static bool Interrupt(void) {
  if (Search.bIdle) {
    Idle();
  }
  if (Search.nGoMode == GO_MODE_NODES) {
    if (!Search.bPonder && Search2.nAllNodes > Search.nNodes * 4) {
      Search2.bStop = true;
      return true;
    }
  } else if (Search.nGoMode == GO_MODE_TIMER) {
    if (!Search.bPonder && (int) (GetTime() - Search2.llTime) > Search.nMaxTimer) {
      Search2.bStop = true;
      return true;
    }
  }
  if (Search.bBatch) {
    return false;
  }

#ifdef CCHESS_A3800
  return false;
#else
  UcciCommStruct UcciComm;
  PositionStruct posProbe;
  // ģʽôȵUCCIͳжǷֹ
  switch (BusyLine(UcciComm, Search.bDebug)) {
  case UCCI_COMM_ISREADY:
    // "isready"ָʵû
    printf("readyok\n");
    fflush(stdout);
    return false;
  case UCCI_COMM_PONDERHIT:
    // "ponderhit"ָʱܣ"SearchMain()"ΪѾ㹻ʱ䣬 ôֹź
    if (Search2.bPonderStop) {
      Search2.bStop = true;
      return true;
    } else {
      Search.bPonder = false;
      return false;
    }
  case UCCI_COMM_PONDERHIT_DRAW:
    // "ponderhit draw"ָʱܣͱ־
    Search.bDraw = true;
    if (Search2.bPonderStop) {
      Search2.bStop = true;
      return true;
    } else {
      Search.bPonder = false;
      return false;
    }
  case UCCI_COMM_STOP:
    // "stop"ָֹź
    Search2.bStop = true;
    return true;
  case UCCI_COMM_PROBE:
    // "probe"ָHashϢ
    BuildPos(posProbe, UcciComm);
    PopHash(posProbe);
    return false;
  case UCCI_COMM_QUIT:
    // "quit"ָ˳ź
    Search.bQuit = Search2.bStop = true;
    return true;
  default:
    return false;
  }
#endif
}

#ifndef CCHESS_A3800

// Ҫ
static void PopPvLine(int nDepth = 0, int vl = 0) {
  uint16_t *lpwmv;
  uint32_t dwMoveStr;
  // δﵽҪȣô¼ȺͷֵԺ
  if (nDepth > 0 && !Search2.bPopPv && !Search.bDebug) {
    Search2.nPopDepth = nDepth;
    Search2.vlPopValue = vl;
    return;
  }
  // ʱ
  printf("info time %d nodes %d\n", (int) (GetTime() - Search2.llTime), Search2.nAllNodes);
  fflush(stdout);
  if (nDepth == 0) {
    // Ѿô
    if (Search2.nPopDepth == 0) {
      return;
    }
    // ȡǰûȺͷֵ
    nDepth = Search2.nPopDepth;
    vl = Search2.vlPopValue;
  } else {
    // ﵽҪȣôԺ󲻱
    Search2.nPopDepth = Search2.vlPopValue = 0;
  }
  printf("info depth %d score %d pv", nDepth, vl);
  lpwmv = Search2.wmvPvLine;
  while (*lpwmv != 0) {
    dwMoveStr = MOVE_COORD(*lpwmv);
    printf(" %.4s", (const char *) &dwMoveStr);
    lpwmv ++;
  }
  printf("\n");
  fflush(stdout);
}

#endif

// ޺ü
static int HarmlessPruning(const PositionStruct &pos, int vlBeta) {
  int vl, vlRep;

  // 1. ɱ岽ü
  vl = pos.nDistance - MATE_VALUE;
  if (vl >= vlBeta) {
    return vl;
  }

  // 2. ü
  if (pos.IsDraw()) {
    return 0; // ȫﲻ"pos.DrawValue()";
  }

  // 3. ظü
  vlRep = pos.RepStatus();
  if (vlRep > 0) {
    return pos.RepValue(vlRep);
  }

  return -MATE_VALUE;
}

// ;ۺ
inline int Evaluate(const PositionStruct &pos, int vlAlpha, int vlBeta) {
  int vl;
  vl = Search.bKnowledge ? pos.Evaluate(vlAlpha, vlBeta) : pos.Material();
  return vl == pos.DrawValue() ? vl - 1 : vl;
}

// ̬
static int SearchQuiesc(PositionStruct &pos, int vlAlpha, int vlBeta) {
  int vlBest, vl, mv;
  bool bInCheck;
  MoveSortStruct MoveSort;  
  // ̬̰¼裺
  Search2.nAllNodes ++;

  // 1. ޺ü
  vl = HarmlessPruning(pos, vlBeta);
  if (vl > -MATE_VALUE) {
    return vl;
  }

#ifdef HASH_QUIESC
  // 3. ûü
  vl = ProbeHashQ(pos, vlAlpha, vlBeta);
  if (Search.bUseHash && vl > -MATE_VALUE) {
    return vl;
  }
#endif

  // 4. ﵽȣֱӷֵ
  if (pos.nDistance == LIMIT_DEPTH) {
    return Evaluate(pos, vlAlpha, vlBeta);
  }
  __ASSERT(Search.pos.nDistance < LIMIT_DEPTH);

  // 5. ʼ
  vlBest = -MATE_VALUE;
  bInCheck = (pos.LastMove().ChkChs > 0);

  // 6. ڱľ棬ȫŷ
  if (bInCheck) {
    MoveSort.InitAll(pos);
  } else {

    // 7. δľ棬ŷǰȳԿ()Ծۣ
    vl = Evaluate(pos, vlAlpha, vlBeta);
    __ASSERT_BOUND(1 - WIN_VALUE, vl, WIN_VALUE - 1);
    __ASSERT(vl > vlBest);
    if (vl >= vlBeta) {
#ifdef HASH_QUIESC
      RecordHashQ(pos, vl, MATE_VALUE);
#endif
      return vl;
    }
    vlBest = vl;
    vlAlpha = MAX(vl, vlAlpha);

    // 8. δľ棬ɲгŷ(MVV(LVA))
    MoveSort.InitQuiesc(pos);
  }

  // 9. Alpha-Beta㷨Щŷ
  while ((mv = MoveSort.NextQuiesc(bInCheck)) != 0) {
    __ASSERT(bInCheck || pos.ucpcSquares[DST(mv)] > 0);
    if (pos.MakeMove(mv)) {
      vl = -SearchQuiesc(pos, -vlBeta, -vlAlpha);
      pos.UndoMakeMove();
      if (vl > vlBest) {
        if (vl >= vlBeta) {
#ifdef HASH_QUIESC
          if (vl > -WIN_VALUE && vl < WIN_VALUE) {
            RecordHashQ(pos, vl, MATE_VALUE);
          }
#endif
          return vl;
        }
        vlBest = vl;
        vlAlpha = MAX(vl, vlAlpha);
      }
    }
  }

  // 10. طֵ
  if (vlBest == -MATE_VALUE) {
    __ASSERT(pos.IsMate());
    return pos.nDistance - MATE_VALUE;
  } else {
#ifdef HASH_QUIESC
    if (vlBest > -WIN_VALUE && vlBest < WIN_VALUE) {
      RecordHashQ(pos, vlBest > vlAlpha ? vlBest : -MATE_VALUE, vlBest);
    }
#endif
    return vlBest;
  }
}

#ifndef CCHESS_A3800

// UCCI֧ - ҶӽľϢ
void PopLeaf(PositionStruct &pos) {
  int vl;
  Search2.nAllNodes = 0;
  vl = SearchQuiesc(pos, -MATE_VALUE, MATE_VALUE);
  printf("pophash lowerbound %d depth 0 upperbound %d depth 0\n", vl, vl);
  fflush(stdout);
}

#endif

const bool NO_NULL = true; // "SearchCut()"ĲǷֹŲü

// 㴰ȫ
static int SearchCut(int vlBeta, int nDepth, bool bNoNull = false) {
  int nNewDepth, vlBest, vl;
  int mvHash, mv, mvEvade;
  MoveSortStruct MoveSort;
  // ȫ̰¼裺

  // 1. Ҷӽ㴦þ̬
  if (nDepth <= 0) {
    __ASSERT(nDepth >= -NULL_DEPTH);
    return SearchQuiesc(Search.pos, vlBeta - 1, vlBeta);
  }
  Search2.nAllNodes ++;

  // 2. ޺ü
  vl = HarmlessPruning(Search.pos, vlBeta);
  if (vl > -MATE_VALUE) {
    return vl;
  }

  // 3. ûü
  vl = ProbeHash(Search.pos, vlBeta - 1, vlBeta, nDepth, bNoNull, mvHash);
  if (Search.bUseHash && vl > -MATE_VALUE) {
    return vl;
  }

  // 4. ﵽȣֱӷֵ
  if (Search.pos.nDistance == LIMIT_DEPTH) {
    return Evaluate(Search.pos, vlBeta - 1, vlBeta);
  }
  __ASSERT(Search.pos.nDistance < LIMIT_DEPTH);

  // 5. жϵã
  Search2.nMainNodes ++;
  vlBest = -MATE_VALUE;
  if ((Search2.nMainNodes & Search.nCountMask) == 0 && Interrupt()) {
    return vlBest;
  }

  // 6. ԿŲü
  if (Search.bNullMove && !bNoNull && Search.pos.LastMove().ChkChs <= 0 && Search.pos.NullOkay()) {
    Search.pos.NullMove();
    vl = -SearchCut(1 - vlBeta, nDepth - NULL_DEPTH - 1, NO_NULL);
    Search.pos.UndoNullMove();
    if (Search2.bStop) {
      return vlBest;
    }

    if (vl >= vlBeta) {
      if (Search.pos.NullSafe()) {
        // a. Ųü飬ô¼Ϊ(NULL_DEPTH + 1)
        RecordHash(Search.pos, HASH_BETA, vl, MAX(nDepth, NULL_DEPTH + 1), 0);
        return vl;
      } else if (SearchCut(vlBeta, nDepth - NULL_DEPTH, NO_NULL) >= vlBeta) {
        // b. Ųü飬ô¼Ϊ(NULL_DEPTH)
        RecordHash(Search.pos, HASH_BETA, vl, MAX(nDepth, NULL_DEPTH), 0);
        return vl;
      }
    }
  }

  // 7. ʼ
  if (Search.pos.LastMove().ChkChs > 0) {
    // ǽ棬ôӦŷ
    mvEvade = MoveSort.InitEvade(Search.pos, mvHash, Search2.wmvKiller[Search.pos.nDistance]);
  } else {
    // ǽ棬ôʹŷб
    MoveSort.InitFull(Search.pos, mvHash, Search2.wmvKiller[Search.pos.nDistance]);
    mvEvade = 0;
  }

  // 8. "MoveSortStruct::NextFull()"̵ŷ˳һ
  while ((mv = MoveSort.NextFull(Search.pos)) != 0) {
    if (Search.pos.MakeMove(mv)) {

      // 9. ѡ죻
      nNewDepth = (Search.pos.LastMove().ChkChs > 0 || mvEvade != 0 ? nDepth : nDepth - 1);

      // 10. 㴰
      vl = -SearchCut(1 - vlBeta, nNewDepth);
      Search.pos.UndoMakeMove();
      if (Search2.bStop) {
        return vlBest;
      }

      // 11. ضж
      if (vl > vlBest) {
        vlBest = vl;
        if (vl >= vlBeta) {
          RecordHash(Search.pos, HASH_BETA, vlBest, nDepth, mv);
          if (!MoveSort.GoodCap(Search.pos, mv)) {
            SetBestMove(mv, nDepth, Search2.wmvKiller[Search.pos.nDistance]);
          }
          return vlBest;
        }
      }
    }
  }

  // 12. ضϴʩ
  if (vlBest == -MATE_VALUE) {
    __ASSERT(Search.pos.IsMate());
    return Search.pos.nDistance - MATE_VALUE;
  } else {
    RecordHash(Search.pos, HASH_ALPHA, vlBest, nDepth, mvEvade);
    return vlBest;
  }
}

// Ҫ
static void AppendPvLine(uint16_t *lpwmvDst, uint16_t mv, const uint16_t *lpwmvSrc) {
  *lpwmvDst = mv;
  lpwmvDst ++;
  while (*lpwmvSrc != 0) {
    *lpwmvDst = *lpwmvSrc;
    lpwmvSrc ++;
    lpwmvDst ++;
  }
  *lpwmvDst = 0;
}

/* Ҫọ̈̄㴰ȫ¼㣺
 *
 * 1. ڲ
 * 2. ʹиӰĲü
 * 3. Alpha-Beta߽жӣ
 * 4. PVҪȡҪ
 * 5. PV㴦ŷ
 */
static int SearchPV(int vlAlpha, int vlBeta, int nDepth, uint16_t *lpwmvPvLine) {
  int nNewDepth, nHashFlag, vlBest, vl;
  int mvBest, mvHash, mv, mvEvade;
  MoveSortStruct MoveSort;
  uint16_t wmvPvLine[LIMIT_DEPTH];
  // ȫ̰¼裺

  // 1. Ҷӽ㴦þ̬
  *lpwmvPvLine = 0;
  if (nDepth <= 0) {
    __ASSERT(nDepth >= -NULL_DEPTH);
    return SearchQuiesc(Search.pos, vlAlpha, vlBeta);
  }
  Search2.nAllNodes ++;

  // 2. ޺ü
  vl = HarmlessPruning(Search.pos, vlBeta);
  if (vl > -MATE_VALUE) {
    return vl;
  }

  // 3. ûü
  vl = ProbeHash(Search.pos, vlAlpha, vlBeta, nDepth, NO_NULL, mvHash);
  if (Search.bUseHash && vl > -MATE_VALUE) {
    // PV㲻ûüԲᷢPV·жϵ
    return vl;
  }

  // 4. ﵽȣֱӷֵ
  __ASSERT(Search.pos.nDistance > 0);
  if (Search.pos.nDistance == LIMIT_DEPTH) {
    return Evaluate(Search.pos, vlAlpha, vlBeta);
  }
  __ASSERT(Search.pos.nDistance < LIMIT_DEPTH);

  // 5. жϵã
  Search2.nMainNodes ++;
  vlBest = -MATE_VALUE;
  if ((Search2.nMainNodes & Search.nCountMask) == 0 && Interrupt()) {
    return vlBest;
  }

  // 6. ڲ
  if (nDepth > IID_DEPTH && mvHash == 0) {
    __ASSERT(nDepth / 2 <= nDepth - IID_DEPTH);
    vl = SearchPV(vlAlpha, vlBeta, nDepth / 2, wmvPvLine);
    if (vl <= vlAlpha) {
      vl = SearchPV(-MATE_VALUE, vlBeta, nDepth / 2, wmvPvLine);
    }
    if (Search2.bStop) {
      return vlBest;
    }
    mvHash = wmvPvLine[0];
  }

  // 7. ʼ
  mvBest = 0;
  nHashFlag = HASH_ALPHA;
  if (Search.pos.LastMove().ChkChs > 0) {
    // ǽ棬ôӦŷ
    mvEvade = MoveSort.InitEvade(Search.pos, mvHash, Search2.wmvKiller[Search.pos.nDistance]);
  } else {
    // ǽ棬ôʹŷб
    MoveSort.InitFull(Search.pos, mvHash, Search2.wmvKiller[Search.pos.nDistance]);
    mvEvade = 0;
  }

  // 8. "MoveSortStruct::NextFull()"̵ŷ˳һ
  while ((mv = MoveSort.NextFull(Search.pos)) != 0) {
    if (Search.pos.MakeMove(mv)) {

      // 9. ѡ죻
      nNewDepth = (Search.pos.LastMove().ChkChs > 0 || mvEvade != 0 ? nDepth : nDepth - 1);

      // 10. Ҫ
      if (vlBest == -MATE_VALUE) {
        vl = -SearchPV(-vlBeta, -vlAlpha, nNewDepth, wmvPvLine);
      } else {
        vl = -SearchCut(-vlAlpha, nNewDepth);
        if (vl > vlAlpha && vl < vlBeta) {
          vl = -SearchPV(-vlBeta, -vlAlpha, nNewDepth, wmvPvLine);
        }
      }
      Search.pos.UndoMakeMove();
      if (Search2.bStop) {
        return vlBest;
      }

      // 11. Alpha-Beta߽ж
      if (vl > vlBest) {
        vlBest = vl;
        if (vl >= vlBeta) {
          mvBest = mv;
          nHashFlag = HASH_BETA;
          break;
        }
        if (vl > vlAlpha) {
          vlAlpha = vl;
          mvBest = mv;
          nHashFlag = HASH_PV;
          AppendPvLine(lpwmvPvLine, mv, wmvPvLine);
        }
      }
    }
  }

  // 12. ûʷɱŷ
  if (vlBest == -MATE_VALUE) {
    __ASSERT(Search.pos.IsMate());
    return Search.pos.nDistance - MATE_VALUE;
  } else {
    RecordHash(Search.pos, nHashFlag, vlBest, nDepth, mvEvade == 0 ? mvBest : mvEvade);
    if (mvBest != 0 && !MoveSort.GoodCap(Search.pos, mvBest)) {
      SetBestMove(mvBest, nDepth, Search2.wmvKiller[Search.pos.nDistance]);
    }
    return vlBest;
  }
}

/* ̣ȫ¼㣺
 *
 * 1. ʡ޺ü(Ҳȡûŷ)
 * 2. ʹÿŲü
 * 3. ѡֻʹý죻
 * 4. ˵ֹŷ
 * 5. ŷʱҪܶദ(¼Ҫ)
 * 6. ʷɱŷ
 */
static int SearchRoot(int nDepth) {
  int nNewDepth, vlBest, vl, mv, nCurrMove;
#ifndef CCHESS_A3800
  uint32_t dwMoveStr;
#endif
  uint16_t wmvPvLine[LIMIT_DEPTH];
  // ̰¼裺

  // 1. ʼ
  vlBest = -MATE_VALUE;
  Search2.MoveSort.ResetRoot();

  // 2. һÿŷ(Ҫ˽ֹŷ)
  nCurrMove = 0;
  while ((mv = Search2.MoveSort.NextRoot()) != 0) {
    if (Search.pos.MakeMove(mv)) {
#ifndef CCHESS_A3800
      if (Search2.bPopCurrMove || Search.bDebug) {
        dwMoveStr = MOVE_COORD(mv);
        nCurrMove ++;
        printf("info currmove %.4s currmovenumber %d\n", (const char *) &dwMoveStr, nCurrMove);
        fflush(stdout);
      }
#endif

      // 3. ѡ(ֻǽ)
      nNewDepth = (Search.pos.LastMove().ChkChs > 0 ? nDepth : nDepth - 1);

      // 4. Ҫ
      if (vlBest == -MATE_VALUE) {
        vl = -SearchPV(-MATE_VALUE, MATE_VALUE, nNewDepth, wmvPvLine);
        __ASSERT(vl > vlBest);
      } else {
        vl = -SearchCut(-vlBest, nNewDepth);
        if (vl > vlBest) { // ﲻҪ" && vl < MATE_VALUE"
          vl = -SearchPV(-MATE_VALUE, -vlBest, nNewDepth, wmvPvLine);
        }
      }
      Search.pos.UndoMakeMove();
      if (Search2.bStop) {
        return vlBest;
      }

      // 5. Alpha-Beta߽ж("vlBest""SearchPV()"е"vlAlpha")
      if (vl > vlBest) {

        // 6. һŷô"δıŷ"ļ1
        Search2.nUnchanged = (vlBest == -MATE_VALUE ? Search2.nUnchanged + 1 : 0);
        vlBest = vl;

        // 7. ŷʱ¼Ҫ
        AppendPvLine(Search2.wmvPvLine, mv, wmvPvLine);
#ifndef CCHESS_A3800
        PopPvLine(nDepth, vl);
#endif

        // 8. ҪԣAlphaֵҪɱʱ
        if (vlBest > -WIN_VALUE && vlBest < WIN_VALUE) {
          vlBest += (Search.rc4Random.NextLong() & Search.nRandomMask) -
              (Search.rc4Random.NextLong() & Search.nRandomMask);
          vlBest = (vlBest == Search.pos.DrawValue() ? vlBest - 1 : vlBest);
        }

        // 9. ¸ŷб
        Search2.MoveSort.UpdateRoot(mv);
      }
    }
  }
  return vlBest;
}

// ΨһŷElephantEyeϵһɫжĳȽеǷҵΨһŷ
// ԭǰҵŷɽֹŷȻ(-WIN_VALUE, 1 - WIN_VALUE)Ĵ
// ͳ߽˵ŷɱ
static bool SearchUnique(int vlBeta, int nDepth) {
  int vl, mv;
  Search2.MoveSort.ResetRoot(ROOT_UNIQUE);
  // һŷ
  while ((mv = Search2.MoveSort.NextRoot()) != 0) {
    if (Search.pos.MakeMove(mv)) {
      vl = -SearchCut(1 - vlBeta, Search.pos.LastMove().ChkChs > 0 ? nDepth : nDepth - 1);
      Search.pos.UndoMakeMove();
      if (Search2.bStop || vl >= vlBeta) {
        return false;
      }
    }
  }
  return true;
}

// 
void SearchMain(int nDepth) {
  int i, vl, vlLast, nDraw;
  int nCurrTimer, nLimitTimer, nLimitNodes;
  bool bUnique;
#ifndef CCHESS_A3800
  int nBookMoves;
  uint32_t dwMoveStr;
  BookStruct bks[MAX_GEN_MOVES];
#endif
  // ̰¼裺

  // 1. ֱӷ
  if (Search.pos.IsDraw() || Search.pos.RepStatus(3) > 0) {
#ifndef CCHESS_A3800
    printf("nobestmove\n");
    fflush(stdout);
#endif
    return;    
  }

#ifndef CCHESS_A3800
  // 2. ӿֿŷ
  if (Search.bUseBook) {
    // a. ȡֿе߷
    nBookMoves = GetBookMoves(Search.pos, Search.szBookFile, bks);
    if (nBookMoves > 0) {
      vl = 0;
      for (i = 0; i < nBookMoves; i ++) {
        vl += bks[i].wvl;
        dwMoveStr = MOVE_COORD(bks[i].wmv);
        printf("info depth 0 score %d pv %.4s\n", bks[i].wvl, (const char *) &dwMoveStr);
        fflush(stdout);
      }
      // b. Ȩѡһ߷
      vl = Search.rc4Random.NextLong() % (uint32_t) vl;
      for (i = 0; i < nBookMoves; i ++) {
        vl -= bks[i].wvl;
        if (vl < 0) {
          break;
        }
      }
      __ASSERT(vl < 0);
      __ASSERT(i < nBookMoves);
      // c. ֿеŷѭ棬ôŷ
      Search.pos.MakeMove(bks[i].wmv);
      if (Search.pos.RepStatus(3) == 0) {
        dwMoveStr = MOVE_COORD(bks[i].wmv);
        printf("bestmove %.4s", (const char *) &dwMoveStr);
        // d. ̨˼ŷ(ֿеһȨĺŷ)
        nBookMoves = GetBookMoves(Search.pos, Search.szBookFile, bks);
        Search.pos.UndoMakeMove();
        if (nBookMoves > 0) {
          dwMoveStr = MOVE_COORD(bks[0].wmv);
          printf(" ponder %.4s", (const char *) &dwMoveStr);
        }
        printf("\n");
        fflush(stdout);
        return;
      }
      Search.pos.UndoMakeMove();
    }
  }
#endif

  // 3. Ϊ򷵻ؾֵ̬
  if (nDepth == 0) {
#ifndef CCHESS_A3800
    printf("info depth 0 score %d\n", SearchQuiesc(Search.pos, -MATE_VALUE, MATE_VALUE));
    fflush(stdout);
    printf("nobestmove\n");
    fflush(stdout);
#endif
    return;
  }

  // 4. ɸÿŷ
  Search2.MoveSort.InitRoot(Search.pos, Search.nBanMoves, Search.wmvBanList);

  // 5. ʼʱͼ
  Search2.bStop = Search2.bPonderStop = Search2.bPopPv = Search2.bPopCurrMove = false;
  Search2.nPopDepth = Search2.vlPopValue = 0;
  Search2.nAllNodes = Search2.nMainNodes = Search2.nUnchanged = 0;
  Search2.wmvPvLine[0] = 0;
  ClearKiller(Search2.wmvKiller);
  ClearHistory();
  ClearHash();
  //  ClearHash() Ҫһʱ䣬ԼʱԺʼȽϺ
  Search2.llTime = GetTime();
  vlLast = 0;
  // 10غŷôͣԺÿ8غһ
  nDraw = -Search.pos.LastMove().CptDrw;
  if (nDraw > 5 && ((nDraw - 4) / 2) % 8 == 0) {
    Search.bDraw = true;
  }
  bUnique = false;
  nCurrTimer = 0;

  // 6. 
  for (i = 1; i <= nDepth; i ++) {
    // ҪҪʱһ"info depth n"ǲ
#ifndef CCHESS_A3800
    if (Search2.bPopPv || Search.bDebug) {
      printf("info depth %d\n", i);
      fflush(stdout);
    }

    // 7. ʱǷҪҪ͵ǰ˼ŷ
    Search2.bPopPv = (nCurrTimer > 300);
    Search2.bPopCurrMove = (nCurrTimer > 3000);
#endif

    // 8. 
    vl = SearchRoot(i);
    if (Search2.bStop) {
      if (vl > -MATE_VALUE) {
        vlLast = vl; // vlLastжͶҪһֵ
      }
      break; // û"vl"ǿɿֵ
    }

    nCurrTimer = (int) (GetTime() - Search2.llTime);
    // 9. ʱ䳬ʵʱޣֹ
    if (Search.nGoMode == GO_MODE_TIMER) {
      // a. ûʹÿŲüôʵʱ޼(Ϊ֦Ӽӱ)
      nLimitTimer = (Search.bNullMove ? Search.nProperTimer : Search.nProperTimer / 2);
      // b. ǰֵûǰһܶ࣬ôʵʱ޼
      nLimitTimer = (vl + DROPDOWN_VALUE >= vlLast ? nLimitTimer / 2 : nLimitTimer);
      // c. ŷûб仯ôʵʱ޼
      nLimitTimer = (Search2.nUnchanged >= UNCHANGED_DEPTH ? nLimitTimer / 2 : nLimitTimer);
      if (nCurrTimer > nLimitTimer) {
        if (Search.bPonder) {
          Search2.bPonderStop = true; // ں̨˼ģʽôֻں̨˼кֹ
        } else {
          vlLast = vl;
          break; // Ƿ"vlLast"Ѹ
        }
      }
    } else if (Search.nGoMode == GO_MODE_NODES) {
      // nLimitNodesļ㷽nLimitTimerһ
      nLimitNodes = (Search.bNullMove ? Search.nNodes : Search.nNodes / 2);
      nLimitNodes = (vl + DROPDOWN_VALUE >= vlLast ? nLimitNodes / 2 : nLimitNodes);
      nLimitNodes = (Search2.nUnchanged >= UNCHANGED_DEPTH ? nLimitNodes / 2 : nLimitNodes);
      // GO_MODE_NODESǲӳ̨˼ʱ
      if (Search2.nAllNodes > nLimitNodes) {
        vlLast = vl;
        break;
      }
    }
    vlLast = vl;

    // 10. ɱֹ
    if (vlLast > WIN_VALUE || vlLast < -WIN_VALUE) {
      break;
    }

    // 11. Ψһŷֹ
    if (SearchUnique(1 - WIN_VALUE, i)) {
      bUnique = true;
      break;
    }
  }

#ifdef CCHESS_A3800
  Search.mvResult = Search2.wmvPvLine[0];
#else
  // 12. ŷӦ(Ϊ̨˼Ĳ²ŷ)
  if (Search2.wmvPvLine[0] != 0) {
    PopPvLine();
    dwMoveStr = MOVE_COORD(Search2.wmvPvLine[0]);
    printf("bestmove %.4s", (const char *) &dwMoveStr);
    if (Search2.wmvPvLine[1] != 0) {
      dwMoveStr = MOVE_COORD(Search2.wmvPvLine[1]);
      printf(" ponder %.4s", (const char *) &dwMoveStr);
    }

    // 13. жǷͣǾΨһŷĲʺ(ΪȲ)
    if (!bUnique) {
      if (vlLast > -WIN_VALUE && vlLast < -RESIGN_VALUE) {
        printf(" resign");
      } else if (Search.bDraw && !Search.pos.NullSafe() && vlLast < DRAW_OFFER_VALUE * 2) {
        printf(" draw");
      }
    }
  } else {
    printf("nobestmove");
  }
  printf("\n");
  fflush(stdout);
#endif
}
// ===== END eleeye/search.cpp =====

// ===== BEGIN bot/main.cpp =====

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::vector<Move> collectHistory(const ParsedInput& parsed) {
    std::vector<Move> history;
    const size_t turnID = parsed.responses.size();

    for (size_t i = 0; i < turnID; ++i) {
        if (i < parsed.requests.size() && parsed.requests[i].valid) {
            history.push_back(parsed.requests[i].move);
        }
        if (i < parsed.responses.size() && parsed.responses[i].valid) {
            history.push_back(parsed.responses[i].move);
        }
    }

    if (turnID < parsed.requests.size() && parsed.requests[turnID].valid) {
        history.push_back(parsed.requests[turnID].move);
    }

    return history;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    std::string input = buffer.str();

    ParsedInput parsed;
    std::string errorMessage;
    if (!Protocol::parseInput(input, parsed, &errorMessage)) {
        Json::Value out = Protocol::makeOutput(Move(), Json::Value(Json::objectValue));
        std::cout << Protocol::writeJson(out) << std::endl;
        return 0;
    }

    std::vector<Move> history = collectHistory(parsed);

    EyeAdapter ai;
    Move bestMove = ai.getBestMove(history);

    Json::Value outData = parsed.data;
    if (!outData.isObject()) {
        outData = Json::Value(Json::objectValue);
    }

    Json::Value out = Protocol::makeOutput(bestMove, outData);
    std::cout << Protocol::writeJson(out) << std::endl;
    return 0;
}
// ===== END bot/main.cpp =====
