#include "AIPlayer.h"
#include "MoveBook.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <utility>

namespace {

// Utility helpers
inline int homeRank(colorType side) { return side == RED ? 0 : 9; }
inline int forwardDir(colorType side) { return side == RED ? 1 : -1; }
inline bool crossedRiver(colorType side, int y) { return side == RED ? y >= 5 : y <= 4; }
inline int initialCannonRank(colorType side) { return side == RED ? 2 : 7; }
inline int initialPawnRank(colorType side) { return side == RED ? 3 : 6; }
inline int clampInt(int value, int low, int high) { return std::max(low, std::min(high, value)); }

int countPiecesBetween(const ChessBoard& board, int sx, int sy, int ex, int ey);

#ifdef CHESS_DEBUG_DECISION_TRACE
std::string traceMoveString(const Move& move) {
    if (move.isInvalid()) {
        return "-1";
    }
    std::string s;
    s += pgnint2char(move.source_x);
    s += int2char(move.source_y);
    s += pgnint2char(move.target_x);
    s += int2char(move.target_y);
    return s;
}
#endif

int countPiecesBetween(const ChessBoard& board, int sx, int sy, int ex, int ey) {
    if (sx == ex && sy == ey) {
        return 0;
    }
    if (!ChessBoard::inSameStraightLine(sx, sy, ex, ey)) {
        return 99;
    }

    int dx = (ex > sx) ? 1 : ((ex < sx) ? -1 : 0);
    int dy = (ey > sy) ? 1 : ((ey < sy) ? -1 : 0);
    int tx = sx + dx;
    int ty = sy + dy;
    int count = 0;

    while (tx != ex || ty != ey) {
        if (board.getGridAt(tx, ty).color != EMPTY) {
            ++count;
        }
        tx += dx;
        ty += dy;
    }
    return count;
}

int exchangeBaseValue(stoneType t) {
    switch (t) {
        case King:      return 5;
        case Assistant: return 1;
        case Bishop:    return 1;
        case Knight:    return 3;
        case Rook:      return 4;
        case Cannon:    return 3;
        case Pawn:      return 2;
        default:        return 0;
    }
}
static int valuableStringLevel(stoneType t) {
    switch (t) {
        case Knight: return 2;
        case Cannon: return 1;
        default:     return 0;
    }
}

static int stringHoldValueByDistance(int dist) {
    if (dist <= 0) {
        return 0;
    }
    const int v = 8 + dist * 4;
    return (v > 40 ? 40 : v);
}
static bool protectedSquareForExchange(const ChessBoard& board,
                                       colorType side,
                                       int tx, int ty,
                                       int exceptX = -1,
                                       int exceptY = -1) {
    auto isExcept = [&](int x, int y) -> bool {
        return x == exceptX && y == exceptY;
    };

    // 1. 如果目标格在己方半场
    if (homeRank(side) == 0 ? (ty <= 4) : (ty >= 5)) {
        if (ChessBoard::inKingArea(tx, ty, side)) {
            // 将/帅保护
            for (int d = 0; d < 4; ++d) {
                const int sx = tx + dx_strai[d];
                const int sy = ty + dy_strai[d];
                if (!ChessBoard::inBoard(sx, sy) || isExcept(sx, sy)) continue;
                const Grid g = board.getGridAt(sx, sy);
                if (g.color == side && g.type == King &&
                    ChessBoard::inKingArea(sx, sy, side)) {
                    return true;
                }
            }

            // 士保护
            for (int d = 0; d < 4; ++d) {
                const int sx = tx + dx_ob[d];
                const int sy = ty + dy_ob[d];
                if (!ChessBoard::inBoard(sx, sy) || isExcept(sx, sy)) continue;
                const Grid g = board.getGridAt(sx, sy);
                if (g.color == side && g.type == Assistant &&
                    ChessBoard::inKingArea(sx, sy, side)) {
                    return true;
                }
            }
        }

        // 象保护
        for (int d = 0; d < 4; ++d) {
            const int sx = tx + dx_bishop[d];
            const int sy = ty + dy_bishop[d];
            if (!ChessBoard::inBoard(sx, sy) || isExcept(sx, sy)) continue;
            if (!ChessBoard::inColorArea(sx, sy, side)) continue;

            const int ex = tx + dx_bishop_eye[d];
            const int ey = ty + dy_bishop_eye[d];
            const Grid g = board.getGridAt(sx, sy);
            if (g.color == side && g.type == Bishop &&
                board.getGridAt(ex, ey).color == EMPTY) {
                return true;
            }
        }
    } else {
        // 2. 过河兵横向保护
        for (int d = 0; d < 2; ++d) {
            const int sx = tx + dx_lr[d];
            const int sy = ty;
            if (!ChessBoard::inBoard(sx, sy) || isExcept(sx, sy)) continue;
            const Grid g = board.getGridAt(sx, sy);
            if (g.color == side && g.type == Pawn &&
                crossedRiver(side, sy)) {
                return true;
            }
        }
    }

    // 3. 兵纵向保护（从目标格往己方后方找）
    {
        const int sy = ty - (side == RED ? 1 : -1);
        const int sx = tx;
        if (ChessBoard::inBoard(sx, sy) && !isExcept(sx, sy)) {
            const Grid g = board.getGridAt(sx, sy);
            if (g.color == side && g.type == Pawn) {
                return true;
            }
        }
    }

    // 4. 马保护
    for (int d = 0; d < 8; ++d) {
        const int sx = tx - dx_knight[d];
        const int sy = ty - dy_knight[d];
        if (!ChessBoard::inBoard(sx, sy) || isExcept(sx, sy)) continue;

        const int fx = sx + dx_knight_foot[d];
        const int fy = sy + dy_knight_foot[d];
        if (!ChessBoard::inBoard(fx, fy)) continue;

        const Grid g = board.getGridAt(sx, sy);
        if (g.color == side && g.type == Knight &&
            board.getGridAt(fx, fy).color == EMPTY) {
            return true;
        }
    }

    // 5. 车保护
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            if ((x == tx && y == ty) || isExcept(x, y)) continue;
            const Grid g = board.getGridAt(x, y);
            if (g.color == side && g.type == Rook &&
                ChessBoard::inSameStraightLine(x, y, tx, ty) &&
                board.betweenNotEmptyNum(x, y, tx, ty) == 0) {
                return true;
            }
        }
    }

    // 6. 炮保护
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            if ((x == tx && y == ty) || isExcept(x, y)) continue;
            const Grid g = board.getGridAt(x, y);
            if (g.color == side && g.type == Cannon &&
                ChessBoard::inSameStraightLine(x, y, tx, ty) &&
                board.betweenNotEmptyNum(x, y, tx, ty) == 1) {
                return true;
            }
        }
    }

    return false;
}

static int mvvLvaForMove(const ChessBoard& board, const Move& mv, colorType side) {
    if (!board.isCapture(mv)) {
        return 0;
    }

    const Grid source = board.getGridAt(mv.source_x, mv.source_y);
    const Grid target = board.getGridAt(mv.target_x, mv.target_y);
    if (target.color == EMPTY) {
        return 0;
    }

    const int mvv = exchangeBaseValue(target.type);
    const int lva = exchangeBaseValue(source.type);

    const int lvaAdjust =
        protectedSquareForExchange(board, ChessBoard::oppColor(side),
                                   mv.target_x, mv.target_y) ? lva : 0;

    if (mvv >= lvaAdjust) {
        return mvv - lvaAdjust + 1;
    }

    // Eye: 吃到车马炮，或者吃到己方半场上的子，即便表面亏，也给 1
    const bool onHomeHalf = (side == RED ? (mv.target_y <= 4) : (mv.target_y >= 5));
    return (mvv >= 3 || onHomeHalf) ? 1 : 0;
}

} // namespace

// =====================================================================
// PST（红方视角，x=0..8, y=0..9）
// =====================================================================
// 1. 开中局、有进攻机会的帅(将)和兵(卒)，参照“梦入神蛋”
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

// 2. 开中局、没有进攻机会的帅(将)和兵(卒)
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

// 3. 残局、有进攻机会的帅(将)和兵(卒)
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

// 4. 残局、没有进攻机会的帅(将)和兵(卒)
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

// 5. 没受威胁的仕(士)和相(象)
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

// 5'. 可升变的，没受威胁的仕(士)和相(象)
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

// 6. 受到威胁的仕(士)和相(象)，参照“梦入神蛋”
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

// 7. 开中局的马，参照“梦入神蛋”
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

// 8. 残局的马
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

// 9. 开中局的车，参照“梦入神蛋”
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

// 10. 残局的车
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

// 11. 开中局的炮，参照“梦入神蛋”
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

// 12. 残局的炮
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

static const int cvlHollowThreat[16] = {
   0,  0,  0,  0,  0,  0, 60, 65, 70, 75, 80, 80, 80,  0,  0,  0
};

static const int cvlCentralThreat[16] = {
   0,  0,  0,  0,  0,  0, 50, 45, 40, 35, 30, 30, 30,  0,  0,  0
};

static const int cvlBottomThreat[16] = {
   0,  0,  0, 40, 30,  0,  0,  0,  0,  0, 30, 40,  0,  0,  0,  0
};

// ================================================================
//  ElephantEye-style dynamic PST state
// ================================================================

static constexpr int EYE_ROOK_MIDGAME_VALUE = 6;
static constexpr int EYE_KNIGHT_CANNON_MIDGAME_VALUE = 3;
static constexpr int EYE_OTHER_MIDGAME_VALUE = 1;
static constexpr int EYE_TOTAL_MIDGAME_VALUE =
    EYE_ROOK_MIDGAME_VALUE * 4 +
    EYE_KNIGHT_CANNON_MIDGAME_VALUE * 8 +
    EYE_OTHER_MIDGAME_VALUE * 18;
static constexpr int EYE_TOTAL_ATTACK_VALUE = 8;

struct EyePstState {
    int midgameValue = 0;
    int redAttack = 0;
    int blackAttack = 0;
};

static inline int eyeSqFromRedXY(int x, int redY) {
    // Eye 16x16 board:
    // file = 3 + x
    // rank = 12 - redY   (redY=0 是红方底线，映到 Eye 的 rank 12)
    return (3 + x) + ((12 - redY) << 4);
}

static inline int eyeTbl9x10(const uint8_t tbl[256], int x, int redY) {
    return static_cast<int>(tbl[eyeSqFromRedXY(x, redY)]);
}

static inline int clampEyeAttack(int v) {
    if (v < 0) return 0;
    if (v > EYE_TOTAL_ATTACK_VALUE) return EYE_TOTAL_ATTACK_VALUE;
    return v;
}

static inline int lerpEye(int mg, int eg, int midgameValue) {
    return (mg * midgameValue + eg * (EYE_TOTAL_MIDGAME_VALUE - midgameValue)) / EYE_TOTAL_MIDGAME_VALUE;
}

static EyePstState buildEyePstState(const ChessBoard& board) {
    EyePstState st;

    int nMidgameValue = 0;
    int nRedAttacks = 0;
    int nBlackAttacks = 0;
    int nRedSimpleValue = 0;
    int nBlackSimpleValue = 0;

    // Iterate over all 32 pieces via Eye dual tables
    for (int pc = 16; pc < 48; ++pc) {
        int sq = board.squareOf(pc);
        if (sq == 0) continue;
        int pt = PIECE_TYPE_EYE(pc);
        if (pt == 0) continue; // King — skip for midgame/attack calc

        stoneType t = eyePtToStone(pt);
        colorType c = (pc < 32) ? RED : BLACK;
        int y = sq2y(sq);

        switch (t) {
            case Rook:   nMidgameValue += EYE_ROOK_MIDGAME_VALUE; break;
            case Knight: case Cannon: nMidgameValue += EYE_KNIGHT_CANNON_MIDGAME_VALUE; break;
            default:     nMidgameValue += EYE_OTHER_MIDGAME_VALUE; break;
        }

        const bool redAttackingHalf = (c == RED && y >= 5);
        const bool blackAttackingHalf = (c == BLACK && y <= 4);

        if (redAttackingHalf) {
            if (t == Rook || t == Knight) nRedAttacks += 2;
            else if (t == Cannon || t == Pawn) nRedAttacks += 1;
        }
        if (blackAttackingHalf) {
            if (t == Rook || t == Knight) nBlackAttacks += 2;
            else if (t == Cannon || t == Pawn) nBlackAttacks += 1;
        }

        if (c == RED) {
            if (t == Rook) nRedSimpleValue += 2;
            else if (t == Knight || t == Cannon) nRedSimpleValue += 1;
        } else {
            if (t == Rook) nBlackSimpleValue += 2;
            else if (t == Knight || t == Cannon) nBlackSimpleValue += 1;
        }
    }

    nMidgameValue =
        (2 * EYE_TOTAL_MIDGAME_VALUE - nMidgameValue) * nMidgameValue / EYE_TOTAL_MIDGAME_VALUE;
    if (nMidgameValue < 0) nMidgameValue = 0;
    if (nMidgameValue > EYE_TOTAL_MIDGAME_VALUE) nMidgameValue = EYE_TOTAL_MIDGAME_VALUE;

    if (nRedSimpleValue > nBlackSimpleValue) {
        nRedAttacks += (nRedSimpleValue - nBlackSimpleValue) * 2;
    } else {
        nBlackAttacks += (nBlackSimpleValue - nRedSimpleValue) * 2;
    }

    st.midgameValue = nMidgameValue;
    st.redAttack = clampEyeAttack(nRedAttacks);
    st.blackAttack = clampEyeAttack(nBlackAttacks);
    return st;
}

static int dynamicPstValue(const EyePstState& st,
                           stoneType t, colorType c, int x, int y) {
    const int redY = (c == RED) ? y : (9 - y);

    switch (t) {
        case King: {
            const int mg = eyeTbl9x10(cucvlKingPawnMidgameAttacking, x, redY);
            const int eg = eyeTbl9x10(cucvlKingPawnEndgameAttacking, x, redY);
            return lerpEye(mg, eg, st.midgameValue);
        }

        case Pawn: {
            const int attack = (c == RED) ? st.redAttack : st.blackAttack;

            const int mgAttack = eyeTbl9x10(cucvlKingPawnMidgameAttacking, x, redY);
            const int egAttack = eyeTbl9x10(cucvlKingPawnEndgameAttacking, x, redY);
            const int mgQuiet  = eyeTbl9x10(cucvlKingPawnMidgameAttackless, x, redY);
            const int egQuiet  = eyeTbl9x10(cucvlKingPawnEndgameAttackless, x, redY);

            const int attackVal = lerpEye(mgAttack, egAttack, st.midgameValue);
            const int quietVal  = lerpEye(mgQuiet,  egQuiet,  st.midgameValue);

            return (attackVal * attack +
                    quietVal * (EYE_TOTAL_ATTACK_VALUE - attack)) / EYE_TOTAL_ATTACK_VALUE;
        }

        case Assistant:
        case Bishop: {
            // 红方士象看黑方攻势，黑方士象看红方攻势
            const int threat = (c == RED) ? st.blackAttack : st.redAttack;
            const int threatened = eyeTbl9x10(cucvlAdvisorBishopThreatened, x, redY);
            const int threatless = eyeTbl9x10(cucvlAdvisorBishopThreatless, x, redY);
            return (threatened * threat +
                    threatless * (EYE_TOTAL_ATTACK_VALUE - threat)) / EYE_TOTAL_ATTACK_VALUE;
        }

        case Knight: {
            const int mg = eyeTbl9x10(cucvlKnightMidgame, x, redY);
            const int eg = eyeTbl9x10(cucvlKnightEndgame, x, redY);
            return lerpEye(mg, eg, st.midgameValue);
        }

        case Rook: {
            const int mg = eyeTbl9x10(cucvlRookMidgame, x, redY);
            const int eg = eyeTbl9x10(cucvlRookEndgame, x, redY);
            return lerpEye(mg, eg, st.midgameValue);
        }

        case Cannon: {
            const int mg = eyeTbl9x10(cucvlCannonMidgame, x, redY);
            const int eg = eyeTbl9x10(cucvlCannonEndgame, x, redY);
            return lerpEye(mg, eg, st.midgameValue);
        }

        default:
            return 0;
    }
}

int pstValueRaw(stoneType t, colorType c, int x, int y) {
    const int redY = (c == RED) ? y : (9 - y);

    // 这里只是轻量 fallback，不承担真正总评估
    constexpr int neutralMidgame = EYE_TOTAL_MIDGAME_VALUE / 2;

    switch (t) {
        case King: {
            const int mg = eyeTbl9x10(cucvlKingPawnMidgameAttacking, x, redY);
            const int eg = eyeTbl9x10(cucvlKingPawnEndgameAttacking, x, redY);
            return lerpEye(mg, eg, neutralMidgame);
        }
        case Pawn: {
            const int mg = eyeTbl9x10(cucvlKingPawnMidgameAttackless, x, redY);
            const int eg = eyeTbl9x10(cucvlKingPawnEndgameAttackless, x, redY);
            return lerpEye(mg, eg, neutralMidgame);
        }
        case Assistant:
        case Bishop:
            return eyeTbl9x10(cucvlAdvisorBishopThreatless, x, redY);
        case Knight: {
            const int mg = eyeTbl9x10(cucvlKnightMidgame, x, redY);
            const int eg = eyeTbl9x10(cucvlKnightEndgame, x, redY);
            return lerpEye(mg, eg, neutralMidgame);
        }
        case Rook: {
            const int mg = eyeTbl9x10(cucvlRookMidgame, x, redY);
            const int eg = eyeTbl9x10(cucvlRookEndgame, x, redY);
            return lerpEye(mg, eg, neutralMidgame);
        }
        case Cannon: {
            const int mg = eyeTbl9x10(cucvlCannonMidgame, x, redY);
            const int eg = eyeTbl9x10(cucvlCannonEndgame, x, redY);
            return lerpEye(mg, eg, neutralMidgame);
        }
        default:
            return 0;
    }
}
int startPstValue(stoneType t, colorType c, int x) {
    switch (t) {
        case Rook:
            return pstValueRaw(t, c, x <= 4 ? 0 : 8, homeRank(c));
        case Knight:
            return pstValueRaw(t, c, x <= 4 ? 1 : 7, homeRank(c));
        case Cannon:
            return pstValueRaw(t, c, x <= 4 ? 1 : 7, initialCannonRank(c));
        case Pawn:
            return pstValueRaw(t, c, x, initialPawnRank(c));
        default:
            return pstValueRaw(t, c, x, homeRank(c));
    }
}

int developmentPstDelta(stoneType t, colorType c, int x, int y) {
    return pstValueRaw(t, c, x, y) - startPstValue(t, c, x);
}

AIPlayer::AIPlayer()
    : tt_(TT_SIZE * HASH_LAYERS),
      searchStart_(),
      timeUp_(false),
      nodesSearched_(0),
      allocatedTimeMs_(HARD_TIME_BASE_MS),
      softTimeMs_(SOFT_TIME_BASE_MS),
      hardTimeMs_(HARD_TIME_BASE_MS),
      lastIterationMs_(0),
      completedDepth_(0),
      bestMoveChanges_(0) {
    std::memset(history_, 0, sizeof(history_));

    for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < BOARDWIDTH * BOARDHEIGHT; ++i) {
            for (int j = 0; j < BOARDWIDTH * BOARDHEIGHT; ++j) {
                counterMoves_[side][i][j] = Move();
            }
        }
    }

    for (int ply = 0; ply < MAX_DEPTH; ++ply) {
        prevMove_[ply] = Move();
        for (int k = 0; k < MAX_KILLERS; ++k) {
            killers_[ply][k] = Move();
        }
    }
}
// =====================================================================
// 子力价值：开局 / 残局 / 排序基础值
// =====================================================================

// =====================================================================
// phase：0=残局，256=开局
// =====================================================================
int AIPlayer::endgamePhase(const ChessBoard& board) {
    int rookCount = 0;
    int knightCount = 0;
    int cannonCount = 0;

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color == EMPTY || g.type == King) {
                continue;
            }
            switch (g.type) {
                case Rook:   ++rookCount; break;
                case Knight: ++knightCount; break;
                case Cannon: ++cannonCount; break;
                default: break;
            }
        }
    }

    const int majorCount = rookCount + knightCount + cannonCount;
    const int knightCannonCount = knightCount + cannonCount;

    // 只按车马炮算阶段，车权重大，马炮次之
    // 四车 + 四马 + 四炮 的满配置：
    // 4*6 + 4*3 + 4*3 = 48
    const int phaseWeight = rookCount * 6 + knightCount * 3 + cannonCount * 3;
    int phase = (phaseWeight * 256 + 24) / 48;

    // 你定义的“明确残局”：
    // 1) 双方车马炮总数 <= 5
    // 2) 或双方都没有车，且马炮总数 <= 6
    //
    // 不直接硬 return 48，而是把 phase 上限压低，让它平滑进入残局
    if (majorCount <= 5 || (rookCount == 0 && knightCannonCount <= 6)) {
        phase = std::min(phase, 56);
    } else if (majorCount <= 7) {
        // 接近残局，但还没到你定义的硬残局
        phase = std::min(phase, 112);
    }

    return std::max(0, std::min(phase, 256));
}

// =====================================================================
// 评估 1：子力
// =====================================================================

int AIPlayer::advisorShapeScore(const ChessBoard& board, colorType side) const {
    const EyePstState st = buildEyePstState(board);
    const colorType opp = ChessBoard::oppColor(side);
    const int homeY = homeRank(side);
    const int dir = forwardDir(side);

    int kingX = -1, kingY = -1;
    if (!board.findKing(side, kingX, kingY)) {
        return -200;
    }

    int ownAdvisorCount = 0;
    int enemyRookCount = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color == side && g.type == Assistant) {
                ++ownAdvisorCount;
            } else if (g.color == opp && g.type == Rook) {
                ++enemyRookCount;
            }
        }
    }

    const int enemyAttack = (side == RED ? st.blackAttack : st.redAttack);

    auto threatRankIndex = [&](int y) -> int {
        // 对红方罚分时，Eye 用 RANK_FLIP(y)；对黑方罚分时，Eye 直接用 y
        return (side == RED ? (y + 3) : (12 - y));
    };

    auto threatFileIndex = [&](int x) -> int {
        return x + 3;
    };

    auto hollowThreatValue = [&](int y) -> int {
        const int idx = threatRankIndex(y);
        return cvlHollowThreat[idx] * (st.midgameValue + EYE_TOTAL_MIDGAME_VALUE)
             / (EYE_TOTAL_MIDGAME_VALUE * 2);
    };

    auto centralThreatValue = [&](int y) -> int {
        return cvlCentralThreat[threatRankIndex(y)];
    };

    auto bottomThreatValue = [&](int x) -> int {
        return cvlBottomThreat[threatFileIndex(x)] * enemyAttack / EYE_TOTAL_ATTACK_VALUE;
    };

    const int advisorLeakage = 80 * enemyAttack / EYE_TOTAL_ATTACK_VALUE;

    auto hasOwnAdvisor = [&](int x, int y) -> bool {
        if (!ChessBoard::inBoard(x, y)) return false;
        const Grid g = board.getGridAt(x, y);
        return g.color == side && g.type == Assistant;
    };

    auto hasOwnKnight = [&](int x, int y) -> bool {
        if (!ChessBoard::inBoard(x, y)) return false;
        const Grid g = board.getGridAt(x, y);
        return g.color == side && g.type == Knight;
    };

    auto enemyControlsSquare = [&](int x, int y) -> bool {
        return protectedSquareForExchange(board, opp, x, y);
    };

    auto ownBottomRookGuardsCenter = [&]() -> bool {
        for (int x = 0; x < BOARDWIDTH; ++x) {
            const Grid g = board.getGridAt(x, homeY);
            if (g.color == side && g.type == Rook) {
                if (countPiecesBetween(board, x, homeY, 4, homeY) == 0) {
                    return true;
                }
            }
        }
        return false;
    };

    const int SHAPE_NONE   = 0;
    const int SHAPE_CENTER = 1;
    const int SHAPE_LEFT   = 2;
    const int SHAPE_RIGHT  = 3;

    int penalty = 0;

    if (ownAdvisorCount == 2) {
        if (kingX == 4 && kingY == homeY) {
            int shape = SHAPE_NONE;

            const bool leftBottom  = hasOwnAdvisor(3, homeY);
            const bool rightBottom = hasOwnAdvisor(5, homeY);
            const bool centerGuard = hasOwnAdvisor(4, homeY + dir);

            if (leftBottom) {
                shape = rightBottom ? SHAPE_CENTER : (centerGuard ? SHAPE_LEFT : SHAPE_NONE);
            } else if (rightBottom) {
                shape = centerGuard ? SHAPE_RIGHT : SHAPE_NONE;
            } else if (centerGuard) {
                shape = SHAPE_NONE;
            }

            switch (shape) {
                case SHAPE_NONE:
                    break;

                case SHAPE_CENTER:
                    for (int x = 0; x < BOARDWIDTH; ++x) {
                        for (int y = 0; y < BOARDHEIGHT; ++y) {
                            const Grid g = board.getGridAt(x, y);
                            if (g.color != opp || g.type != Cannon) {
                                continue;
                            }

                            if (x == 4) {
                                const int cnt = countPiecesBetween(board, x, y, 4, homeY);
                                if (cnt == 0) {
                                    // 空头炮
                                    penalty += hollowThreatValue(y);
                                } else if (cnt == 1 && hasOwnKnight(4, homeY + dir)) {
                                    // 炮镇窝心马
                                    penalty += centralThreatValue(y);
                                }
                            }
                        }
                    }
                    break;

                case SHAPE_LEFT:
                case SHAPE_RIGHT: {
                    const int openDoorX = (shape == SHAPE_LEFT ? 5 : 3);

                    for (int x = 0; x < BOARDWIDTH; ++x) {
                        for (int y = 0; y < BOARDHEIGHT; ++y) {
                            const Grid g = board.getGridAt(x, y);
                            if (g.color != opp || g.type != Cannon) {
                                continue;
                            }

                            if (x == 4) {
                                const int cnt = countPiecesBetween(board, x, y, 4, homeY);
                                if (cnt == 1) {
                                    // 一般中炮威胁
                                    penalty += (centralThreatValue(y) >> 2);
                                    if (enemyControlsSquare(openDoorX, homeY)) {
                                        penalty += 20;
                                    }
                                    if (ownBottomRookGuardsCenter()) {
                                        penalty += 80;
                                    }
                                }
                            } else if (y == homeY) {
                                // 沉底炮威胁
                                if (countPiecesBetween(board, x, y, 4, homeY) == 0) {
                                    penalty += bottomThreatValue(x);
                                }
                            }
                        }
                    }
                    break;
                }

                default:
                    break;
            }
        } else if (kingX == 4 && kingY == homeY + dir) {
            // 花心帅
            penalty += 20;
        }
    } else {
        // 缺士怕双车
        if (enemyRookCount >= 2) {
            penalty += advisorLeakage;
        }
    }

    return -penalty;
}

int AIPlayer::rookMobilityScore(const ChessBoard& board, colorType side) const {
    int mobility = 0;

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side || g.type != Rook) {
                continue;
            }

            // 左
            for (int tx = x - 1; tx >= 0; --tx) {
                if (board.getGridAt(tx, y).color != EMPTY) {
                    break;
                }
                ++mobility;
            }

            // 右
            for (int tx = x + 1; tx < BOARDWIDTH; ++tx) {
                if (board.getGridAt(tx, y).color != EMPTY) {
                    break;
                }
                ++mobility;
            }

            // 下
            for (int ty = y - 1; ty >= 0; --ty) {
                if (board.getGridAt(x, ty).color != EMPTY) {
                    break;
                }
                ++mobility;
            }

            // 上
            for (int ty = y + 1; ty < BOARDHEIGHT; ++ty) {
                if (board.getGridAt(x, ty).color != EMPTY) {
                    break;
                }
                ++mobility;
            }
        }
    }

    return mobility >> 1;
}

int AIPlayer::knightTrapScore(const ChessBoard& board, colorType side) const {
    const colorType opp = ChessBoard::oppColor(side);
    int penalty = 0;

    auto isEdgeSquare = [&](int x, int y) -> bool {
        return x == 0 || x == 8 || y == 0 || y == 9;
    };

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side || g.type != Knight) {
                continue;
            }

            int movable = 0;

            for (int d = 0; d < 8; ++d) {
                const int tx = x + dx_knight[d];
                const int ty = y + dy_knight[d];
                if (!ChessBoard::inBoard(tx, ty)) {
                    continue;
                }

                // Eye: 边线格不算“好落点”
                if (isEdgeSquare(tx, ty)) {
                    continue;
                }

                // 目标格必须为空
                if (board.getGridAt(tx, ty).color != EMPTY) {
                    continue;
                }

                // 马腿不能被堵
                const int fx = x + dx_knight_foot[d];
                const int fy = y + dy_knight_foot[d];
                if (board.getGridAt(fx, fy).color != EMPTY) {
                    continue;
                }

                // Eye: 对方控制格不算“好落点”
                if (protectedSquareForExchange(board, opp, tx, ty)) {
                    continue;
                }

                ++movable;
                if (movable > 1) {
                    break;
                }
            }

            if (movable == 0) {
                penalty += 10;
            } else if (movable == 1) {
                penalty += 5;
            }
        }
    }

    return -penalty;
}
int AIPlayer::stringHoldScore(const ChessBoard& board, colorType side) const {
    const colorType opp = ChessBoard::oppColor(side);
    int score = 0;

    auto collectBetweenPieces = [&](int sx, int sy, int tx, int ty,
                                    std::vector<std::pair<int, int>>& pieces) {
        pieces.clear();

        int dx = 0, dy = 0;
        if (sx == tx) {
            dy = (ty > sy ? 1 : -1);
        } else if (sy == ty) {
            dx = (tx > sx ? 1 : -1);
        } else {
            return;
        }

        int x = sx + dx;
        int y = sy + dy;
        while (x != tx || y != ty) {
            if (board.getGridAt(x, y).color != EMPTY) {
                pieces.push_back(std::make_pair(x, y));
            }
            x += dx;
            y += dy;
        }
    };

    auto addRookStringAgainstTarget = [&](int sx, int sy, int tx, int ty, bool targetIsRook) {
        std::vector<std::pair<int, int>> mids;
        collectBetweenPieces(sx, sy, tx, ty, mids);

        // Eye: 车牵制要求 source 到 target 之间正好只有一个被牵制子
        if (mids.size() != 1) {
            return;
        }

        const int strX = mids[0].first;
        const int strY = mids[0].second;
        const Grid strG = board.getGridAt(strX, strY);
        if (strG.color != opp) {
            return;
        }

        if (valuableStringLevel(strG.type) <= 0) {
            return;
        }

        // 被牵制子不能被目标子当根保护
        if (protectedSquareForExchange(board, opp, strX, strY, tx, ty)) {
            return;
        }

        // 如果目标是车，目标车自己也必须没根
        if (targetIsRook && protectedSquareForExchange(board, opp, tx, ty)) {
            return;
        }

        const int dist = (sx == tx) ? std::abs(ty - strY) : std::abs(tx - strX);
        score += stringHoldValueByDistance(dist);
    };

    auto addCannonStringAgainstTarget = [&](int sx, int sy, int tx, int ty, bool targetIsRook) {
        std::vector<std::pair<int, int>> mids;
        collectBetweenPieces(sx, sy, tx, ty, mids);

        // Eye: 炮牵制要求 source 到 target 之间有两层隔子
        if (mids.size() != 2) {
            return;
        }

        // 第一层是炮架，第二层是被牵制子
        const int strX = mids[1].first;
        const int strY = mids[1].second;
        const Grid strG = board.getGridAt(strX, strY);
        if (strG.color != opp) {
            return;
        }

        // Eye: 对炮来说，只有牵马有价值
        if (valuableStringLevel(strG.type) <= 1) {
            return;
        }

        if (protectedSquareForExchange(board, opp, strX, strY, tx, ty)) {
            return;
        }

        if (targetIsRook && protectedSquareForExchange(board, opp, tx, ty)) {
            return;
        }

        const int dist = (sx == tx) ? std::abs(ty - strY) : std::abs(tx - strX);
        score += stringHoldValueByDistance(dist);
    };

    int oppKingX = -1, oppKingY = -1;
    board.findKing(opp, oppKingX, oppKingY);

    std::vector<std::pair<int, int>> oppRooks;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color == opp && g.type == Rook) {
                oppRooks.push_back(std::make_pair(x, y));
            }
        }
    }

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side) {
                continue;
            }

            // 1) 车牵制将/车
            if (g.type == Rook) {
                if (oppKingX >= 0 && (x == oppKingX || y == oppKingY)) {
                    addRookStringAgainstTarget(x, y, oppKingX, oppKingY, false);
                }

                for (const auto& rk : oppRooks) {
                    const int tx = rk.first;
                    const int ty = rk.second;
                    if (x == tx || y == ty) {
                        addRookStringAgainstTarget(x, y, tx, ty, true);
                    }
                }
            }

            // 2) 炮牵制将/车
            if (g.type == Cannon) {
                if (oppKingX >= 0 && (x == oppKingX || y == oppKingY)) {
                    addCannonStringAgainstTarget(x, y, oppKingX, oppKingY, false);
                }

                for (const auto& rk : oppRooks) {
                    const int tx = rk.first;
                    const int ty = rk.second;
                    if (x == tx || y == ty) {
                        addCannonStringAgainstTarget(x, y, tx, ty, true);
                    }
                }
            }
        }
    }

    return score;
}
// =====================================================================
// 评估 2：PST 位置 + 简单贴将距离
// =====================================================================
int AIPlayer::evalPosition(const ChessBoard& board, colorType side) const {
    const EyePstState st = buildEyePstState(board);
    int score = 0;

    // Iterate over all 32 pieces via Eye dual tables
    for (int pc = 16; pc < 48; ++pc) {
        int sq = board.squareOf(pc);
        if (sq == 0) continue;
        int pt = PIECE_TYPE_EYE(pc);
        stoneType t = eyePtToStone(pt);
        colorType c = (pc < 32) ? RED : BLACK;
        int x = sq2x(sq), y = sq2y(sq);
        int v = dynamicPstValue(st, t, c, x, y);
        if (c == side) score += v;
        else           score -= v;
    }

    return score;
}
// =====================================================================
// 帅门格点危险分
// =====================================================================

// =====================================================================
// 帅门安全模式识别
// =====================================================================

// =====================================================================
// 评估 3：帅门安全与条件化帅位
// =====================================================================

// =====================================================================
// 评估 4：车炮马机动性（简单可控）
// =====================================================================

// =====================================================================
// 总评估 — Eye Evaluate(vlAlpha, vlBeta) 带4级懒惰裁剪
// =====================================================================
int AIPlayer::evaluate(const ChessBoard& board, int vlAlpha, int vlBeta) const {
    const colorType side = board.currentColor();
    const colorType opp = ChessBoard::oppColor(side);

    // Eye lazy eval margin constants
    static constexpr int EVAL_MARGIN1 = 160;
    static constexpr int EVAL_MARGIN2 = 80;
    static constexpr int EVAL_MARGIN3 = 40;
    static constexpr int EVAL_MARGIN4 = 20;

    // 1. Material + PST (最大开销项)
    int vl = evalPosition(board, side);
    if (vl + EVAL_MARGIN1 <= vlAlpha) return vl + EVAL_MARGIN1;
    if (vl - EVAL_MARGIN1 >= vlBeta)  return vl - EVAL_MARGIN1;

    // 2. AdvisorShape
    vl += advisorShapeScore(board, side) - advisorShapeScore(board, opp);
    if (vl + EVAL_MARGIN2 <= vlAlpha) return vl + EVAL_MARGIN2;
    if (vl - EVAL_MARGIN2 >= vlBeta)  return vl - EVAL_MARGIN2;

    // 3. StringHold
    vl += stringHoldScore(board, side) - stringHoldScore(board, opp);
    if (vl + EVAL_MARGIN3 <= vlAlpha) return vl + EVAL_MARGIN3;
    if (vl - EVAL_MARGIN3 >= vlBeta)  return vl - EVAL_MARGIN3;

    // 4. RookMobility
    vl += rookMobilityScore(board, side) - rookMobilityScore(board, opp);
    if (vl + EVAL_MARGIN4 <= vlAlpha) return vl + EVAL_MARGIN4;
    if (vl - EVAL_MARGIN4 >= vlBeta)  return vl - EVAL_MARGIN4;

    // 5. KnightTrap (最后一项，完整计算)
    vl += knightTrapScore(board, side) - knightTrapScore(board, opp);

    return vl;
}
// =====================================================================
// 评估 6：悬挂子惩罚（大子被攻击且无人防守）
// leaf eval 直接体现悬挂，不需要靠 QSearch "看见" 被吃
// =====================================================================
// =====================================================================
// TT 操作
// =====================================================================
// =====================================================================
// Eye-style pack/unpack Move <-> uint16_t
// =====================================================================
uint16_t AIPlayer::packMove(const Move& mv) {
    if (mv.isInvalid()) return 0;
    const int sqSrc = xy2sq(mv.source_x, mv.source_y);
    const int sqDst = xy2sq(mv.target_x, mv.target_y);
    return static_cast<uint16_t>((sqSrc << 8) | sqDst);
}

Move AIPlayer::unpackMove(uint16_t wmv) {
    if (wmv == 0) return Move();
    const int sqSrc = (wmv >> 8) & 0xff;
    const int sqDst = wmv & 0xff;
    Move mv;
    mv.source_x = sq2x(sqSrc);
    mv.source_y = sq2y(sqSrc);
    mv.target_x = sq2x(sqDst);
    mv.target_y = sq2y(sqDst);
    return mv;
}

int AIPlayer::scoreToTT(const ChessBoard& board, int vl) const {
    if (vl > WIN_SCORE) {
        if (vl <= BAN_SCORE) return vl;
        return vl + board.getDistance();
    } else if (vl < -WIN_SCORE) {
        if (vl >= -BAN_SCORE) return vl;
        return vl - board.getDistance();
    }
    return vl;
}

int AIPlayer::scoreFromTT(const ChessBoard& board, int vl) const {
    if (vl > WIN_SCORE) {
        if (vl <= BAN_SCORE) return vl;
        return vl - board.getDistance();
    } else if (vl < -WIN_SCORE) {
        if (vl >= -BAN_SCORE) return vl;
        return vl + board.getDistance();
    }
    return vl;
}

void AIPlayer::ttStore(const ChessBoard& board, int nFlag, int vl,
                       int nDepth, const Move& mv) const {
    const size_t needSize = static_cast<size_t>(TT_SIZE) * HASH_LAYERS;
    if (tt_.size() != needSize) {
        tt_.assign(needSize, TTEntry{});
    }

    const uint32_t dwLock0 = board.getZobrist().dwLock0;
    const uint32_t dwLock1 = board.getZobrist().dwLock1;
    const size_t baseIdx =
        static_cast<size_t>(board.getZobrist().dwKey & TT_MASK) * HASH_LAYERS;
    const uint16_t wmv = packMove(mv);

    int storedVl = vl;
    if (vl > WIN_SCORE || vl < -WIN_SCORE) {
        storedVl = scoreToTT(board, vl);
    }

    int minDepth = 512;
    int minLayer = 0;

    for (int i = 0; i < HASH_LAYERS; ++i) {
        TTEntry& hsh = tt_[baseIdx + static_cast<size_t>(i)];

        if (hsh.dwLock0 == dwLock0 && hsh.dwLock1 == dwLock1) {
            if ((nFlag & HASH_ALPHA) != 0 &&
                (hsh.ucAlphaDepth <= nDepth || hsh.svlAlpha >= storedVl)) {
                hsh.ucAlphaDepth = static_cast<uint8_t>(nDepth);
                hsh.svlAlpha = static_cast<int16_t>(storedVl);
            }
            if ((nFlag & HASH_BETA) != 0 &&
                (hsh.ucBetaDepth <= nDepth || hsh.svlBeta <= storedVl)) {
                hsh.ucBetaDepth = static_cast<uint8_t>(nDepth);
                hsh.svlBeta = static_cast<int16_t>(storedVl);
            }
            if (wmv != 0) {
                hsh.wmvPacked = wmv;
            }
            return;
        }

        const int slotDepth =
            std::min(static_cast<int>(hsh.ucAlphaDepth),
                     static_cast<int>(hsh.ucBetaDepth));

        if (hsh.dwLock0 == 0 && hsh.dwLock1 == 0) {
            minLayer = i;
            minDepth = -1;
            break;
        }
        if (slotDepth < minDepth) {
            minDepth = slotDepth;
            minLayer = i;
        }
    }

    TTEntry& hsh = tt_[baseIdx + static_cast<size_t>(minLayer)];
    hsh.dwLock0 = dwLock0;
    hsh.dwLock1 = dwLock1;
    hsh.wmvPacked = wmv;
    hsh.ucAlphaDepth = 0;
    hsh.ucBetaDepth = 0;
    hsh.svlAlpha = 0;
    hsh.svlBeta = 0;

    if ((nFlag & HASH_ALPHA) != 0) {
        hsh.ucAlphaDepth = static_cast<uint8_t>(nDepth);
        hsh.svlAlpha = static_cast<int16_t>(storedVl);
    }
    if ((nFlag & HASH_BETA) != 0) {
        hsh.ucBetaDepth = static_cast<uint8_t>(nDepth);
        hsh.svlBeta = static_cast<int16_t>(storedVl);
    }
}

int AIPlayer::ttProbe(const ChessBoard& board, int vlAlpha, int vlBeta,
                      int nDepth, bool bNoNull, Move& ttMove) const {
    ttMove = Move();

    const size_t needSize = static_cast<size_t>(TT_SIZE) * HASH_LAYERS;
    if (tt_.size() != needSize) {
        return -MATE_SCORE;
    }

    const uint32_t dwLock0 = board.getZobrist().dwLock0;
    const uint32_t dwLock1 = board.getZobrist().dwLock1;
    const size_t baseIdx =
        static_cast<size_t>(board.getZobrist().dwKey & TT_MASK) * HASH_LAYERS;

    for (int i = 0; i < HASH_LAYERS; ++i) {
        const TTEntry& hsh = tt_[baseIdx + static_cast<size_t>(i)];
        if (hsh.dwLock0 != dwLock0 || hsh.dwLock1 != dwLock1) {
            continue;
        }

        if (hsh.wmvPacked != 0) {
            Move cand = unpackMove(hsh.wmvPacked);
            if (ChessBoard::inBoard(cand.source_x, cand.source_y) &&
                ChessBoard::inBoard(cand.target_x, cand.target_y)) {
                ttMove = cand;
            }
        }

        if (!bNoNull && hsh.ucAlphaDepth >= nDepth) {
            const int vl = scoreFromTT(board, hsh.svlAlpha);
            if (vl <= vlAlpha) {
                return vl;
            }
        }

        if (hsh.ucBetaDepth >= nDepth) {
            const int vl = scoreFromTT(board, hsh.svlBeta);
            if (vl >= vlBeta) {
                return vl;
            }
        }
    }

    return -MATE_SCORE;
}
void AIPlayer::updateKiller(int ply, const Move& mv) const {
    if (ply < 0 || ply >= MAX_DEPTH) return;
    if (!(killers_[ply][0] == mv)) {
        killers_[ply][1] = killers_[ply][0];
        killers_[ply][0] = mv;
    }
}

void AIPlayer::updateHistory(colorType side, const Move& mv, int depth) const {
    const int colorIdx = static_cast<int>(side);
    const int from = mv.source_x * BOARDHEIGHT + mv.source_y;
    const int to   = mv.target_x * BOARDHEIGHT + mv.target_y;

    history_[colorIdx][from][to] += depth * depth;
    if (history_[colorIdx][from][to] > 800000) {
        for (int i = 0; i < BOARDWIDTH * BOARDHEIGHT; ++i) {
            for (int j = 0; j < BOARDWIDTH * BOARDHEIGHT; ++j) {
                history_[colorIdx][i][j] /= 2;
            }
        }
    }
}

void AIPlayer::updateCounterMove(colorType side, const Move& prev, const Move& cur) const {
    if (prev.isInvalid()) return;
    const int colorIdx = static_cast<int>(side);
    const int from = prev.source_x * BOARDHEIGHT + prev.source_y;
    const int to   = prev.target_x * BOARDHEIGHT + prev.target_y;
    counterMoves_[colorIdx][from][to] = cur;
}

bool AIPlayer::isKiller(int ply, const Move& mv) const {
    if (ply < 0 || ply >= MAX_DEPTH) return false;
    return killers_[ply][0] == mv || killers_[ply][1] == mv;
}

bool AIPlayer::isCounterMove(colorType side, const Move& prev, const Move& mv) const {
    if (prev.isInvalid()) return false;
    const int colorIdx = static_cast<int>(side);
    const int from = prev.source_x * BOARDHEIGHT + prev.source_y;
    const int to   = prev.target_x * BOARDHEIGHT + prev.target_y;
    return counterMoves_[colorIdx][from][to] == mv;
}

void AIPlayer::initSearchState() const {
    const size_t needSize = static_cast<size_t>(TT_SIZE) * HASH_LAYERS;
    if (tt_.size() != needSize) {
        tt_.assign(needSize, TTEntry{});
    }

    timeUp_ = false;
    nodesSearched_ = 0;
    lastIterationMs_ = 0;
    completedDepth_ = 0;
    bestMoveChanges_ = 0;
    allocatedTimeMs_ = HARD_TIME_BASE_MS;
    softTimeMs_ = SOFT_TIME_BASE_MS;
    hardTimeMs_ = HARD_TIME_BASE_MS;

    for (int ply = 0; ply < MAX_DEPTH; ++ply) {
        prevMove_[ply] = Move();
        for (int k = 0; k < MAX_KILLERS; ++k) {
            killers_[ply][k] = Move();
        }
    }
}

int AIPlayer::elapsedMs() const {
    const auto now = std::chrono::steady_clock::now();
    return static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStart_).count());
}

bool AIPlayer::checkTime() const {
    if ((nodesSearched_ & TIME_CHECK_MASK) != 0) {
        return timeUp_;
    }
    if (elapsedMs() >= hardTimeMs_) {
        timeUp_ = true;
    }
    return timeUp_;
}

bool AIPlayer::softTimeUp() const {
    return elapsedMs() >= softTimeMs_;
}

void AIPlayer::allocateTimeBudget(const ChessBoard&, int rootMoveCount) const {
    int soft = SOFT_TIME_BASE_MS;
    int hard = HARD_TIME_BASE_MS;

    if (rootMoveCount <= 4) {
        soft += 80;
        hard += 80;
    } else if (rootMoveCount >= 20) {
        soft -= 40;
        hard -= 40;
    }

    softTimeMs_ = std::max(SOFT_TIME_MIN_MS, std::min(SOFT_TIME_MAX_MS, soft));
    hardTimeMs_ = std::max(HARD_TIME_MIN_MS, std::min(HARD_TIME_MAX_MS, hard));
    allocatedTimeMs_ = hardTimeMs_;
}

bool AIPlayer::shouldStopForNextDepth(int depth, int) const {
    if (timeUp_) return true;
    if (elapsedMs() >= hardTimeMs_) return true;
    if (depth <= 1) return false;
    if (lastIterationMs_ <= 0) return false;
    return elapsedMs() + lastIterationMs_ * 2 > hardTimeMs_;
}

int AIPlayer::drawValue(int ply) const {
    return ((ply & 1) == 0 ? -DRAW_VALUE : DRAW_VALUE);
}

int AIPlayer::repValue(int repStatus, int ply) const {
    if (repStatus == REP_DRAW) {
        return drawValue(ply);
    }
    if (repStatus == REP_LOSS) {
        return ply - BAN_SCORE;
    }
    if (repStatus == REP_WIN) {
        return BAN_SCORE - ply;
    }
    return 0;
}

void AIPlayer::orderMoves(const ChessBoard& board,
                          std::vector<Move>& moves,
                          int ply,
                          const Move& ttMove) const {
    auto pieceValue = [](stoneType t) -> int {
        switch (t) {
            case King:      return 1000;
            case Rook:      return 500;
            case Cannon:    return 350;
            case Knight:    return 300;
            case Bishop:    return 120;
            case Assistant: return 120;
            case Pawn:      return 100;
            default:        return 0;
        }
    };

    const int sideIdx = (board.currentColor() == RED ? 1 : 0);
    const int n = static_cast<int>(moves.size());

    int scores[EYE_MAX_MOVES];
    for (int i = 0; i < n; ++i) {
        const Move& mv = moves[i];
        int score = 0;

        if (!ttMove.isInvalid() && mv == ttMove) {
            score += 1000000000;
        }

        if (board.isCapture(mv)) {
            const Grid src = board.getGridAt(mv.source_x, mv.source_y);
            const Grid dst = board.getGridAt(mv.target_x, mv.target_y);
            score += 500000;
            score += pieceValue(dst.type) * 16 - pieceValue(src.type);
        } else {
            if (isKiller(ply, mv)) {
                score += 300000;
            }

            if (ply > 0 && isCounterMove(board.currentColor(), prevMove_[ply - 1], mv)) {
                score += 250000;
            }

            const int from = mv.source_x * BOARDHEIGHT + mv.source_y;
            const int to   = mv.target_x * BOARDHEIGHT + mv.target_y;
            score += history_[sideIdx][from][to];
        }

        scores[i] = score;
    }

    for (int i = 0; i < n; ++i) {
        int best = i;
        for (int j = i + 1; j < n; ++j) {
            if (scores[j] > scores[best]) {
                best = j;
            }
        }
        if (best != i) {
            std::swap(moves[i], moves[best]);
            std::swap(scores[i], scores[best]);
        }
    }
}
void AIPlayer::orderMoveArray(const ChessBoard& board,
                              Move moves[],
                              int count,
                              int ply,
                              const Move& ttMove) const {
    auto pieceValue = [](stoneType t) -> int {
        switch (t) {
            case King:      return 1000;
            case Rook:      return 500;
            case Cannon:    return 350;
            case Knight:    return 300;
            case Bishop:    return 120;
            case Assistant: return 120;
            case Pawn:      return 100;
            default:        return 0;
        }
    };

    const int sideIdx = (board.currentColor() == RED ? 1 : 0);
    int scores[EYE_MAX_MOVES];

    for (int i = 0; i < count; ++i) {
        const Move& mv = moves[i];
        int score = 0;

        if (!ttMove.isInvalid() && mv == ttMove) {
            score += 1000000000;
        }

        if (board.isCapture(mv)) {
            const Grid src = board.getGridAt(mv.source_x, mv.source_y);
            const Grid dst = board.getGridAt(mv.target_x, mv.target_y);
            score += 500000;
            score += pieceValue(dst.type) * 16 - pieceValue(src.type);
        } else {
            if (isKiller(ply, mv)) {
                score += 300000;
            }

            if (ply > 0 && isCounterMove(board.currentColor(), prevMove_[ply - 1], mv)) {
                score += 250000;
            }

            const int from = mv.source_x * BOARDHEIGHT + mv.source_y;
            const int to   = mv.target_x * BOARDHEIGHT + mv.target_y;
            score += history_[sideIdx][from][to];
        }

        scores[i] = score;
    }

    for (int i = 0; i < count; ++i) {
        int best = i;
        for (int j = i + 1; j < count; ++j) {
            if (scores[j] > scores[best]) {
                best = j;
            }
        }
        if (best != i) {
            std::swap(moves[i], moves[best]);
            std::swap(scores[i], scores[best]);
        }
    }
}
void AIPlayer::orderRootMoves(const ChessBoard&,
                              std::vector<RootMoveInfo>& moves,
                              const Move& pvMove) const {
    auto rootScore = [&](const RootMoveInfo& rm) -> int {
        int sc = 0;

        if (!pvMove.isInvalid() && rm.move == pvMove) {
            sc += 1000000000;
        }

        sc += rm.searchedDepth * 1000000;
        sc += rm.lastScore;
        sc -= rm.rootOrder;

        return sc;
    };

    for (size_t i = 0; i < moves.size(); ++i) {
        size_t best = i;
        int bestScore = rootScore(moves[i]);

        for (size_t j = i + 1; j < moves.size(); ++j) {
            const int sc = rootScore(moves[j]);
            if (sc > bestScore) {
                bestScore = sc;
                best = j;
            }
        }

        if (best != i) {
            std::swap(moves[i], moves[best]);
        }
    }
}

void AIPlayer::updateRootOrder(std::vector<RootMoveInfo>& moves,
                               const Move& bestMove) const {
    for (auto& rm : moves) {
        if (rm.move == bestMove) {
            rm.rootOrder = 0;
        } else if (rm.rootOrder < 1000000) {
            ++rm.rootOrder;
        }
    }
}

int AIPlayer::harmlessPruning(const ChessBoard& board, int beta, int ply) const {
    const int matePrune = ply - MATE_SCORE;
    if (matePrune >= beta) {
        return matePrune;
    }

    const int rep = board.repStatus();
    if (rep != REP_NONE) {
        return board.repValue(rep);
    }

    if (board.exceedMaxPeaceState()) {
        return board.drawValue();
    }

    return -MATE_SCORE;
}

int AIPlayer::quiescence(ChessBoard& board, int alpha, int beta,
                         int ply, int qsDepth) const {
    ++nodesSearched_;

    if ((nodesSearched_ & TIME_CHECK_MASK) == 0 && checkTime()) {
        return 0;
    }

    const int harmless = harmlessPruning(board, beta, ply);
    if (harmless > -MATE_SCORE) {
        return harmless;
    }

    const bool inCheck = board.isInCheck();

    if (qsDepth >= MAX_QS_DEPTH || ply >= MAX_DEPTH - 1) {
        if (inCheck) {
            Move legalMoves[EYE_MAX_MOVES];
            if (board.generateMovesFast(legalMoves, false) == 0) {
                return -MATE_SCORE + ply;
            }
        }
        return evaluate(board, alpha, beta);
    }

    int best = -MATE_SCORE;

    if (!inCheck) {
        const int standPat = evaluate(board, alpha, beta);
        if (standPat >= beta) {
            return standPat;
        }
        best = standPat;
        if (standPat > alpha) {
            alpha = standPat;
        }
    }

    Move moves[EYE_MAX_MOVES];
    const int moveCount = inCheck
        ? board.generateMovesFast(moves, false)
        : board.generateCapturesFast(moves, false);

    if (inCheck && moveCount == 0) {
        return -MATE_SCORE + ply;
    }

    orderMoveArray(board, moves, moveCount, ply, Move());

    for (int i = 0; i < moveCount; ++i) {
        const Move& mv = moves[i];

        if (!board.makeMoveFast(mv)) {
            continue;
        }

        const colorType movedSide = board.oppColor();
        if (board.kingAttacked(movedSide)) {
            board.undoMoveFast();
            continue;
        }

        prevMove_[ply] = mv;
        const int sc = -quiescence(board, -beta, -alpha, ply + 1, qsDepth + 1);
        board.undoMoveFast();

        if (timeUp_) {
            return 0;
        }

        if (sc > best) {
            best = sc;
            if (sc >= beta) {
                return sc;
            }
            if (sc > alpha) {
                alpha = sc;
            }
        }
    }

    if (best == -MATE_SCORE) {
        return -MATE_SCORE + ply;
    }
    return best;
}
int AIPlayer::searchCut(ChessBoard& board, int beta, int depth,
                        int ply, bool noNull) const {
    if (depth <= 0) {
        return quiescence(board, beta - 1, beta, ply, 0);
    }

    ++nodesSearched_;
    if ((nodesSearched_ & TIME_CHECK_MASK) == 0 && checkTime()) {
        return 0;
    }

    const int harmless = harmlessPruning(board, beta, ply);
    if (harmless > -MATE_SCORE) {
        return harmless;
    }

    if (ply >= MAX_DEPTH - 1) {
        return evaluate(board, beta - 1, beta);
    }

    const bool inCheck = board.isInCheck();
    const bool nodeInCheck = inCheck;
    const colorType sideToMove = board.currentColor();

    Move ttMove;
    {
        const int probe = ttProbe(board, beta - 1, beta, depth, true, ttMove);
        if (probe > -MATE_SCORE) {
            return probe;
        }
    }

    if (!noNull && !inCheck && board.nullOkay() && depth > NULL_DEPTH + 1) {
        board.makeNullMoveFast();
        const int nullScore =
            -searchCut(board, 1 - beta, depth - NULL_DEPTH - 1, ply + 1, true);
        board.undoNullMoveFast();

        if (timeUp_) {
            return 0;
        }

        if (nullScore >= beta) {
            if (board.nullSafe()) {
                ttStore(board, HASH_BETA, nullScore,
                        std::max(depth, NULL_DEPTH + 1), Move());
                return nullScore;
            }

            const int verify =
                searchCut(board, beta, depth - NULL_DEPTH, ply, true);

            if (verify >= beta) {
                ttStore(board, HASH_BETA, verify,
                        std::max(depth, NULL_DEPTH), Move());
                return verify;
            }
        }
    }

    auto goodCapture = [&](const Move& mv) -> bool {
        return board.isCapture(mv);
    };

    Move moves[EYE_MAX_MOVES];
    const int moveCount = board.generateMovesFast(moves, false);
    if (moveCount == 0) {
        return -MATE_SCORE + ply;
    }

    orderMoveArray(board, moves, moveCount, ply, ttMove);

    int best = -MATE_SCORE;
    Move bestMove;

    for (int i = 0; i < moveCount; ++i) {
        const Move& mv = moves[i];

        if (!board.makeMoveFast(mv)) {
            continue;
        }

        const colorType movedSide = board.oppColor();
        if (board.kingAttacked(movedSide)) {
            board.undoMoveFast();
            continue;
        }

        prevMove_[ply] = mv;
        const bool givesCheck = board.isInCheck();
        const int newDepth = (givesCheck || nodeInCheck ? depth : depth - 1);
        const int sc = -searchCut(board, 1 - beta, newDepth, ply + 1, false);
        board.undoMoveFast();

        if (timeUp_) {
            return 0;
        }

        if (sc > best) {
            best = sc;
            bestMove = mv;

            if (sc >= beta) {
                ttStore(board, HASH_BETA, sc, depth, mv);

                if (!goodCapture(mv)) {
                    updateKiller(ply, mv);
                    updateHistory(sideToMove, mv, depth);
                    if (ply > 0) {
                        updateCounterMove(sideToMove, prevMove_[ply - 1], mv);
                    }
                }
                return sc;
            }
        }
    }

    if (best == -MATE_SCORE) {
        return -MATE_SCORE + ply;
    }

    ttStore(board, HASH_ALPHA, best, depth, bestMove);
    return best;
}
int AIPlayer::searchPV(ChessBoard& board, int alpha, int beta, int depth,
                       int ply, std::vector<Move>& pvLine) const {
    Move pvBuf[MAX_DEPTH];
    int pvLen = 0;

    const int sc = searchPVFast(board, alpha, beta, depth, ply, pvBuf, pvLen);

    pvLine.clear();
    for (int i = 0; i < pvLen; ++i) {
        pvLine.push_back(pvBuf[i]);
    }
    return sc;
}
int AIPlayer::searchPVFast(ChessBoard& board, int alpha, int beta, int depth,
                           int ply, Move pvBuf[], int& pvLen) const {
    pvLen = 0;

    if (depth <= 0) {
        return quiescence(board, alpha, beta, ply, 0);
    }

    ++nodesSearched_;
    if ((nodesSearched_ & TIME_CHECK_MASK) == 0 && checkTime()) {
        return 0;
    }

    const int harmless = harmlessPruning(board, beta, ply);
    if (harmless > -MATE_SCORE) {
        return harmless;
    }

    if (ply >= MAX_DEPTH - 1) {
        return evaluate(board, alpha, beta);
    }

    const int alphaOrig = alpha;
    const bool inCheck = board.isInCheck();
    const bool nodeInCheck = inCheck;
    const colorType sideToMove = board.currentColor();

    Move ttMove;
    {
        const int probe = ttProbe(board, alpha, beta, depth, true, ttMove);
        if (probe > -MATE_SCORE) {
            return probe;
        }
    }

    if (depth > 2 && ttMove.isInvalid()) {
        Move iidBuf[MAX_DEPTH];
        int iidLen = 0;
        (void)searchPVFast(board, alpha, beta, depth / 2, ply, iidBuf, iidLen);
        if (iidLen > 0) {
            ttMove = iidBuf[0];
        }
        if (timeUp_) {
            return 0;
        }
    }

    auto goodCapture = [&](const Move& mv) -> bool {
        return board.isCapture(mv);
    };

    Move moves[EYE_MAX_MOVES];
    const int moveCount = board.generateMovesFast(moves, false);
    if (moveCount == 0) {
        return -MATE_SCORE + ply;
    }

    orderMoveArray(board, moves, moveCount, ply, ttMove);

    int best = -MATE_SCORE;
    Move bestMove;
    bool firstLegalMove = true;

    for (int i = 0; i < moveCount; ++i) {
        const Move& mv = moves[i];

        if (!board.makeMoveFast(mv)) {
            continue;
        }

        const colorType movedSide = board.oppColor();
        if (board.kingAttacked(movedSide)) {
            board.undoMoveFast();
            continue;
        }

        prevMove_[ply] = mv;
        const bool givesCheck = board.isInCheck();
        const int newDepth = (givesCheck || nodeInCheck ? depth : depth - 1);

        Move childBuf[MAX_DEPTH];
        int childLen = 0;
        int sc;

        if (firstLegalMove) {
            sc = -searchPVFast(board, -beta, -alpha, newDepth, ply + 1, childBuf, childLen);
            firstLegalMove = false;
        } else {
            sc = -searchCut(board, -alpha, newDepth, ply + 1, false);
            if (!timeUp_ && sc > alpha && sc < beta) {
                childLen = 0;
                sc = -searchPVFast(board, -beta, -alpha, newDepth, ply + 1, childBuf, childLen);
            }
        }

        board.undoMoveFast();

        if (timeUp_) {
            return 0;
        }

        if (sc > best) {
            best = sc;
            bestMove = mv;

            if (sc > alpha) {
                alpha = sc;

                pvLen = 0;
                if (pvLen < MAX_DEPTH) {
                    pvBuf[pvLen++] = mv;
                }
                for (int k = 0; k < childLen && pvLen < MAX_DEPTH; ++k) {
                    pvBuf[pvLen++] = childBuf[k];
                }

                if (sc >= beta) {
                    ttStore(board, HASH_BETA, sc, depth, mv);

                    if (!goodCapture(mv)) {
                        updateKiller(ply, mv);
                        updateHistory(sideToMove, mv, depth);
                        if (ply > 0) {
                            updateCounterMove(sideToMove, prevMove_[ply - 1], mv);
                        }
                    }
                    return sc;
                }
            }
        }
    }

    if (best == -MATE_SCORE) {
        return -MATE_SCORE + ply;
    }

    if (best > alphaOrig) {
        ttStore(board, HASH_PV, best, depth, bestMove);
    } else {
        ttStore(board, HASH_ALPHA, best, depth, bestMove);
    }
    return best;
}
bool AIPlayer::searchUnique(ChessBoard& board,
                            const std::vector<RootMoveInfo>& rootMoves,
                            const Move& bestMove,
                            int beta,
                            int depth) const {
    for (const auto& root : rootMoves) {
        if (root.move == bestMove) {
            continue;
        }

        if (!board.makeMoveFast(root.move)) {
            continue;
        }

        const colorType movedSide = board.oppColor();
        if (board.kingAttacked(movedSide)) {
            board.undoMoveFast();
            continue;
        }

        prevMove_[0] = root.move;

        const bool givesCheck = board.isInCheck();
        const int newDepth = (givesCheck ? depth : depth - 1);

        const int sc = -searchCut(board, 1 - beta, newDepth, 1, false);

        board.undoMoveFast();

        if (timeUp_) {
            return false;
        }

        if (sc >= beta) {
            return false;
        }
    }

    return true;
}

Move AIPlayer::getBestMove(ChessBoard& board) const {
    initSearchState();
    searchStart_ = std::chrono::steady_clock::now();

    std::vector<Move> legalMoves;
    board.generateMovesWithForbidden(legalMoves);

    if (legalMoves.empty()) {
        board.generateMoves(legalMoves);
    }
    if (legalMoves.empty()) {
        return Move();
    }

    allocateTimeBudget(board, static_cast<int>(legalMoves.size()));

    std::vector<RootMoveInfo> rootMoves;
    rootMoves.reserve(legalMoves.size());
    for (size_t i = 0; i < legalMoves.size(); ++i) {
        RootMoveInfo info;
        info.move = legalMoves[i];
        info.lastScore = -INF_SCORE;
        info.searchedDepth = 0;
        info.rootOrder = static_cast<int>(i) + 1;
        rootMoves.push_back(info);
    }

    Move bestMove = legalMoves.front();
    int bestScore = -INF_SCORE;

    for (int depth = 1; depth <= DEFAULT_DEPTH; ++depth) {
        if (shouldStopForNextDepth(depth, static_cast<int>(rootMoves.size()))) {
            break;
        }

        orderRootMoves(board, rootMoves, bestMove);

        int alpha = -INF_SCORE;
        int beta  = INF_SCORE;

        Move iterBestMove = bestMove;
        int iterBestScore = -INF_SCORE;

        for (auto& rm : rootMoves) {
            if (checkTime()) {
                timeUp_ = true;
                break;
            }

            if (!board.makeMoveFast(rm.move)) {
                continue;
            }

            const colorType movedSide = board.oppColor();
            if (board.kingAttacked(movedSide)) {
                board.undoMoveFast();
                continue;
            }

            Move childBuf[MAX_DEPTH];
            int childLen = 0;
            const int sc = -searchPVFast(board, -beta, -alpha, depth - 1, 1, childBuf, childLen);
            board.undoMoveFast();

            if (timeUp_) {
                break;
            }

            rm.lastScore = sc;
            rm.searchedDepth = depth;

            if (sc > iterBestScore) {
                iterBestScore = sc;
                iterBestMove = rm.move;
            }

            if (sc > alpha) {
                alpha = sc;
            }
        }

        if (timeUp_) {
            break;
        }

        bestMove = iterBestMove;
        bestScore = iterBestScore;
        updateRootOrder(rootMoves, bestMove);

        completedDepth_ = depth;
        lastIterationMs_ = elapsedMs();

        if (std::abs(bestScore) >= WIN_SCORE) {
            break;
        }
        if (softTimeUp()) {
            break;
        }
    }

    return bestMove;
}
