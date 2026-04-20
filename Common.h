#ifndef COMMON_H
#define COMMON_H
#include <string>
#include <cstdint>

constexpr int BOARDWIDTH = 9;
constexpr int BOARDHEIGHT = 10;

enum stoneType {
    None = 0,
    King = 1,
    Bishop = 2,
    Knight = 3,
    Rook = 4,
    Pawn = 5,
    Cannon = 6,
    Assistant = 7
};

enum colorType {
    BLACK = 0,
    RED = 1,
    EMPTY = 2
};

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

    Move(int msx, int msy, int mtx, int mty)
        : source_x(msx), source_y(msy), target_x(mtx), target_y(mty) {}

    Move(const std::string& msource, const std::string& mtarget) {
        source_x = pgnchar2int(msource[0]);
        source_y = char2int(msource[1]);
        target_x = pgnchar2int(mtarget[0]);
        target_y = char2int(mtarget[1]);
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

struct Grid {
    stoneType type;
    colorType color;

    Grid() : type(None), color(EMPTY) {}
    Grid(stoneType mtype, colorType mcolor) : type(mtype), color(mcolor) {}

    Grid& operator=(const Grid& other) {
        if (this != &other) {
            type = other.type;
            color = other.color;
        }
        return *this;
    }

    bool operator==(const Grid& other) const {
        return type == other.type && color == other.color;
    }
};

inline constexpr int dx_ob[4] = {-1, 1, -1, 1};
inline constexpr int dy_ob[4] = {-1, -1, 1, 1};

inline constexpr int dx_strai[4] = {-1, 0, 0, 1};
inline constexpr int dy_strai[4] = {0, -1, 1, 0};

inline constexpr int dx_lr[2] = {-1, 1};

inline constexpr int dx_knight[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
inline constexpr int dy_knight[8] = {-1, 1, -2, 2, -2, 2, -1, 1};

inline constexpr int dx_knight_foot[8] = {-1, -1, 0, 0, 0, 0, 1, 1};
inline constexpr int dy_knight_foot[8] = {0, 0, -1, 1, -1, 1, 0, 0};

inline constexpr int dx_bishop[4] = {-2, -2, 2, 2};
inline constexpr int dy_bishop[4] = {-2, 2, -2, 2};

inline constexpr int dx_bishop_eye[4] = {-1, -1, 1, 1};
inline constexpr int dy_bishop_eye[4] = {-1, 1, -1, 1};

inline const char stoneSym[] = "*kbnrpca";

// =====================================================================
//  ElephantEye-style infrastructure
// =====================================================================

// --- 16×16 board coordinate system ---
// Valid squares: file ∈ [3,11], rank ∈ [3,12]
// Red home at ranks 10-12 (bottom), Black home at ranks 3-5 (top)
inline constexpr int EYE_RANK_TOP    = 3;
inline constexpr int EYE_RANK_BOTTOM = 12;
inline constexpr int EYE_FILE_LEFT   = 3;
inline constexpr int EYE_FILE_RIGHT  = 11;
inline constexpr int EYE_FILE_CENTER = 7;

inline int COORD_XY(int file, int rank) { return (rank << 4) | file; }
inline int FILE_X(int sq) { return sq & 0x0f; }
inline int RANK_Y(int sq) { return sq >> 4; }

// Convert between current project (x,y) and Eye square index.
// Current: x ∈ [0,8], y ∈ [0,9], RED bottom y=0..4
// Eye:     file ∈ [3,11], rank ∈ [3,12], RED bottom rank=10..12
inline int xy2sq(int x, int y) { return ((12 - y) << 4) | (3 + x); }
inline int sq2x(int sq) { return (sq & 0x0f) - 3; }
inline int sq2y(int sq) { return 12 - (sq >> 4); }

// Board-validity table for 256-square space
// IN_BOARD: valid board square; IN_FORT: nine-palace area
inline constexpr uint8_t ccInBoard_[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
inline bool IN_BOARD(int sq) { return ccInBoard_[sq] != 0; }

inline constexpr uint8_t ccInFort_[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
inline bool IN_FORT(int sq) { return ccInFort_[sq] != 0; }

// Half-board test: true if sq is in side sd's half
// sd=0 (Red) → ranks 10..12, sd=1 (Black) → ranks 3..5
inline bool SAME_HALF(int sqSrc, int sqDst) { return ((sqSrc ^ sqDst) & 0x80) == 0; }
inline bool HOME_HALF(int sq, int sd) { return (sq & 0x80) != (sd << 7); }
inline bool AWAY_HALF(int sq, int sd) { return (sq & 0x80) == (sd << 7); }

// Forward direction for a pawn: Red goes up (rank decreases → -16), Black goes down (+16)
inline int SQUARE_FORWARD(int sq, int sd) { return sq - 16 + (sd << 5); }

// --- Eye piece-ID system ---
// IDs 0-15 unused. 16-31 = Red pieces, 32-47 = Black pieces.
// Within a side (offset 0-15):
//   0:King  1-2:Advisor  3-4:Bishop  5-6:Knight  7-8:Rook  9-10:Cannon  11-15:Pawn
inline constexpr int EYE_KING_FROM     = 0;
inline constexpr int EYE_ADVISOR_FROM  = 1;
inline constexpr int EYE_ADVISOR_TO    = 2;
inline constexpr int EYE_BISHOP_FROM   = 3;
inline constexpr int EYE_BISHOP_TO     = 4;
inline constexpr int EYE_KNIGHT_FROM   = 5;
inline constexpr int EYE_KNIGHT_TO     = 6;
inline constexpr int EYE_ROOK_FROM     = 7;
inline constexpr int EYE_ROOK_TO       = 8;
inline constexpr int EYE_CANNON_FROM   = 9;
inline constexpr int EYE_CANNON_TO     = 10;
inline constexpr int EYE_PAWN_FROM     = 11;
inline constexpr int EYE_PAWN_TO       = 15;

inline int SIDE_TAG(int sd) { return 16 + (sd << 4); }
inline int OPP_SIDE_TAG(int sd) { return 32 - (sd << 4); }
inline int OPP_SIDE(int sd) { return 1 - sd; }
inline int PIECE_INDEX(int pc) { return pc & 15; }

// Piece type by piece-ID (0=King,1=Advisor,2=Bishop,3=Knight,4=Rook,5=Cannon,6=Pawn)
inline constexpr int cnPieceTypes[48] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,1,2,2,3,3,4,4,5,5,6,6,6,6,6,
  0,1,1,2,2,3,3,4,4,5,5,6,6,6,6,6,
};
inline int PIECE_TYPE_EYE(int pc) { return cnPieceTypes[pc]; }

// Simple piece values used in MvvLva comparison (Eye convention)
inline constexpr int cnSimpleValues[48] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  5,1,1,1,1,3,3,4,4,3,3,2,2,2,2,2,
  5,1,1,1,1,3,3,4,4,3,3,2,2,2,2,2,
};

// Map stoneType → Eye piece type offset
inline int stoneToEyePt(stoneType st) {
    switch (st) {
        case King:      return 0;
        case Assistant: return 1;
        case Bishop:    return 2;
        case Knight:    return 3;
        case Rook:      return 4;
        case Cannon:    return 5;
        case Pawn:      return 6;
        default:        return -1;
    }
}

// Map Eye piece type → stoneType
inline stoneType eyePtToStone(int pt) {
    static constexpr stoneType tbl[] = { King, Assistant, Bishop, Knight, Rook, Cannon, Pawn };
    return (pt >= 0 && pt <= 6) ? tbl[pt] : None;
}

// --- Eye move encoding ---
// A move is packed as (sqSrc << 8) | sqDst  (16-bit)
inline int SRC(int mv) { return (mv >> 8) & 0xff; }
inline int DST(int mv) { return mv & 0xff; }
inline int MAKE_MOVE(int sqSrc, int sqDst) { return (sqSrc << 8) | sqDst; }

// Convert between Move struct and Eye int mv
inline int moveToEyeMv(const Move& m) {
    return MAKE_MOVE(xy2sq(m.source_x, m.source_y),
                     xy2sq(m.target_x, m.target_y));
}
inline Move eyeMvToMove(int mv) {
    return Move(sq2x(SRC(mv)), sq2y(SRC(mv)),
                sq2x(DST(mv)), sq2y(DST(mv)));
}

// --- Repetition detection ---
inline constexpr int REP_NONE = 0;
inline constexpr int REP_DRAW = 1;
inline constexpr int REP_LOSS = 3;
inline constexpr int REP_WIN  = 5;

inline constexpr int REP_HASH_MASK = 0xfff;
inline constexpr int REP_HASH_SIZE = REP_HASH_MASK + 1;

// --- Zobrist triple (Eye style) ---
struct EyeZobrist {
    uint32_t dwKey;
    uint32_t dwLock0;
    uint32_t dwLock1;

    EyeZobrist() : dwKey(0), dwLock0(0), dwLock1(0) {}

    void xorWith(const EyeZobrist& z) {
        dwKey   ^= z.dwKey;
        dwLock0 ^= z.dwLock0;
        dwLock1 ^= z.dwLock1;
    }
    void xorWith(const EyeZobrist& z1, const EyeZobrist& z2) {
        dwKey   ^= z1.dwKey   ^ z2.dwKey;
        dwLock0 ^= z1.dwLock0 ^ z2.dwLock0;
        dwLock1 ^= z1.dwLock1 ^ z2.dwLock1;
    }
    bool operator==(const EyeZobrist& o) const {
        return dwKey == o.dwKey && dwLock0 == o.dwLock0 && dwLock1 == o.dwLock1;
    }
};

// Knight-pin: for knight at sqSrc attacking sqDst, find the blocking pin square.
// Eye uses a lookup table; we compute it directly.
inline int KNIGHT_PIN_SQ(int sqSrc, int sqDst) {
    // dx, dy in 16x16 coords
    int dx = FILE_X(sqDst) - FILE_X(sqSrc);
    int dy = RANK_Y(sqDst) - RANK_Y(sqSrc);
    // pin is on the "straight" leg
    if (dx == -2 || dx == 2) return sqSrc + (dx > 0 ? 1 : -1);
    if (dy == -2 || dy == 2) return sqSrc + (dy > 0 ? 16 : -16);
    return sqSrc; // invalid
}

// Bishop-pin (eye): midpoint between sqSrc and sqDst
inline int BISHOP_PIN_SQ(int sqSrc, int sqDst) {
    return (sqSrc + sqDst) >> 1;
}

// King-span and Advisor-span checks (Eye-style, via lookup or inline)
inline bool KING_SPAN(int sqSrc, int sqDst) {
    int dd = sqDst - sqSrc;
    return dd == 1 || dd == -1 || dd == 16 || dd == -16;
}
inline bool ADVISOR_SPAN(int sqSrc, int sqDst) {
    int dd = sqDst - sqSrc;
    return dd == 15 || dd == -15 || dd == 17 || dd == -17;
}
inline bool BISHOP_SPAN(int sqSrc, int sqDst) {
    int dd = sqDst - sqSrc;
    return dd == 30 || dd == -30 || dd == 34 || dd == -34;
}

// Knight offsets in Eye coord system (16x16)
// For knight at sq, possible destinations are sq + ccKnightDelta[i]
// and the pin squares are sq + ccKnightPin[i]
inline constexpr int ccKnightDelta[8] = { -33, -31, -18, -14, 14, 18, 31, 33 };
inline constexpr int ccKnightPin[8]   = { -16, -16,  -1,   1, -1,  1, 16, 16 };

// Maximum piece count, move count for Eye structures
inline constexpr int EYE_MAX_PIECES = 48;
inline constexpr int EYE_MAX_MOVES  = 256;

// CHECK_MULTI: multiple pieces giving check
inline constexpr int CHECK_LAZY = -1; // bLazy marker (any non-zero ≡ in check)
inline constexpr int CHECK_MULTI = 48; // value meaning ≥2 checkers
#endif
