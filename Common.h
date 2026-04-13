#ifndef COMMON_H
#define COMMON_H

#include <string>

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

#endif