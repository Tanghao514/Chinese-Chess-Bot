#include "AIPlayer.h"
#include "OpeningBook.h"
#include "TacticalRules.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace {

int countPiecesBetween(const ChessBoard& board, int sx, int sy, int ex, int ey);
bool isCannonMoveUseful(const ChessBoard& board, const Move& move, colorType side, int phase);

bool isNearOpponentKing(colorType side, int x, int y) {
    if (side == RED) {
        return y >= 7 && x >= 2 && x <= 6;
    } else {
        return y <= 2 && x >= 2 && x <= 6;
    }
}

bool isPalacePressureMove(const ChessBoard& board, const Move& move, colorType side) {
    const Grid source = board.getGridAt(move.source_x, move.source_y);
    if (source.type != Rook && source.type != Cannon && source.type != Knight && source.type != Pawn) {
        return false;
    }
    return isNearOpponentKing(side, move.target_x, move.target_y);
}

bool isLikelyZugzwangLike(const ChessBoard& board, colorType side) {
    int rooks = 0, cannons = 0, knights = 0, pawns = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side) continue;
            switch (g.type) {
                case Rook: ++rooks; break;
                case Cannon: ++cannons; break;
                case Knight: ++knights; break;
                case Pawn: ++pawns; break;
                default: break;
            }
        }
    }
    return (rooks == 0 && cannons == 0 && knights <= 1) ||
           (rooks == 0 && cannons <= 1 && pawns <= 2);
}


int clampInt(int value, int low, int high) {
    return std::max(low, std::min(high, value));
}

int homeRank(colorType side) {
    return side == RED ? 0 : 9;
}

int palaceMinRank(colorType side) {
    return side == RED ? 0 : 7;
}

int palaceMaxRank(colorType side) {
    return side == RED ? 2 : 9;
}

int forwardDir(colorType side) {
    return side == RED ? 1 : -1;
}

int advanceOf(colorType side, int y) {
    return side == RED ? y : (9 - y);
}

bool crossedRiver(colorType side, int y) {
    return side == RED ? y >= 5 : y <= 4;
}

bool inPalace(colorType side, int x, int y) {
    return x >= 3 && x <= 5 && y >= palaceMinRank(side) && y <= palaceMaxRank(side);
}

int initialCannonRank(colorType side) {
    return side == RED ? 2 : 7;
}

int initialPawnRank(colorType side) {
    return side == RED ? 3 : 6;
}

bool isInitialRookSquare(colorType side, int x, int y) {
    return y == homeRank(side) && (x == 0 || x == 8);
}

bool isInitialKnightSquare(colorType side, int x, int y) {
    return y == homeRank(side) && (x == 1 || x == 7);
}

bool isNormalKnightSquare(colorType side, int x, int y) {
    return advanceOf(side, y) == 2 && (x == 2 || x == 6);
}

int flankIndexFromFile(int x) {
    if (x <= 3) {
        return 0;
    }
    if (x >= 5) {
        return 1;
    }
    return -1;
}

bool onFlank(int x, int flank) {
    return flank == 0 ? (x <= 3) : (x >= 5);
}

bool isDevelopedRook(colorType side, int x, int y) {
    return advanceOf(side, y) >= 2 || x == 3 || x == 4 || x == 5;
}

bool isDevelopedKnight(colorType side, int x, int y) {
    return !(y == homeRank(side) && (x == 1 || x == 7));
}

// 炮是否已经离开初始位置
bool isDevelopedCannon(colorType side, int x, int y) {
    const int cannonRow = initialCannonRank(side);
    return !(y == cannonRow && (x == 1 || x == 7));
}

int countKnightJumpOptions(const ChessBoard& board, colorType side, int x, int y) {
    int jumps = 0;
    for (int d = 0; d < 8; ++d) {
        const int tx = x + dx_knight[d];
        const int ty = y + dy_knight[d];
        if (!ChessBoard::inBoard(tx, ty)) {
            continue;
        }
        const int footX = x + dx_knight_foot[d];
        const int footY = y + dy_knight_foot[d];
        if (board.getGridAt(footX, footY).color != EMPTY) {
            continue;
        }
        if (board.getGridAt(tx, ty).color == side) {
            continue;
        }
        ++jumps;
    }
    return jumps;
}

int countKnightForwardJumps(const ChessBoard& board, colorType side, int x, int y) {
    int jumps = 0;
    const int sourceAdvance = advanceOf(side, y);
    for (int d = 0; d < 8; ++d) {
        const int tx = x + dx_knight[d];
        const int ty = y + dy_knight[d];
        if (!ChessBoard::inBoard(tx, ty)) {
            continue;
        }
        const int footX = x + dx_knight_foot[d];
        const int footY = y + dy_knight_foot[d];
        if (board.getGridAt(footX, footY).color != EMPTY) {
            continue;
        }
        if (board.getGridAt(tx, ty).color == side) {
            continue;
        }
        if (advanceOf(side, ty) > sourceAdvance) {
            ++jumps;
        }
    }
    return jumps;
}

int countKnightBlockedLegs(const ChessBoard& board, int x, int y) {
    int blocked = 0;
    if (ChessBoard::inBoard(x - 1, y) && board.getGridAt(x - 1, y).color != EMPTY) {
        ++blocked;
    }
    if (ChessBoard::inBoard(x + 1, y) && board.getGridAt(x + 1, y).color != EMPTY) {
        ++blocked;
    }
    if (ChessBoard::inBoard(x, y - 1) && board.getGridAt(x, y - 1).color != EMPTY) {
        ++blocked;
    }
    if (ChessBoard::inBoard(x, y + 1) && board.getGridAt(x, y + 1).color != EMPTY) {
        ++blocked;
    }
    return blocked;
}

int flankPawnAdvanceSteps(const ChessBoard& board, colorType side, int fileX) {
    const int startAdvance = advanceOf(side, initialPawnRank(side));
    int bestAdvance = startAdvance;
    for (int y = 0; y < BOARDHEIGHT; ++y) {
        const Grid g = board.getGridAt(fileX, y);
        if (g.color == side && g.type == Pawn) {
            bestAdvance = std::max(bestAdvance, advanceOf(side, y));
        }
    }
    return std::max(0, bestAdvance - startAdvance);
}

bool flankPawnAdvanced(const ChessBoard& board, colorType side, int fileX) {
    return flankPawnAdvanceSteps(board, side, fileX) >= 1;
}

bool flankPawnActivatesKnight(const ChessBoard& board, colorType side, int knightX, int knightY, int pawnFileX) {
    if (!isNormalKnightSquare(side, knightX, knightY)) {
        return false;
    }
    if (!flankPawnAdvanced(board, side, pawnFileX)) {
        return false;
    }
    const int jumpCount = countKnightJumpOptions(board, side, knightX, knightY);
    const int forwardJumps = countKnightForwardJumps(board, side, knightX, knightY);
    return jumpCount >= 4 || forwardJumps >= 2;
}

bool isNaturalRookDevelopmentMove(colorType side, const Move& move) {
    const int homeY = homeRank(side);
    if (move.source_y != homeY) {
        return false;
    }

    const bool fromCorner = (move.source_x == 0 || move.source_x == 8);
    if (fromCorner) {
        if (move.target_x == move.source_x && advanceOf(side, move.target_y) >= 1) {
            return true;
        }
        if (move.target_y == homeY &&
            (move.target_x == 1 || move.target_x == 7 || move.target_x == 3 || move.target_x == 5)) {
            return true;
        }
    }

    if (move.target_y == homeY && (move.target_x == 3 || move.target_x == 5)) {
        return true;
    }
    if (move.target_x == move.source_x && advanceOf(side, move.target_y) >= 2) {
        return true;
    }
    return false;
}

bool isMeaninglessEarlyRookShift(colorType side, const Move& move) {
    if (move.source_y != homeRank(side) || move.target_y != homeRank(side)) {
        return false;
    }
    if (isNaturalRookDevelopmentMove(side, move)) {
        return false;
    }
    return move.source_x != move.target_x;
}

int rookFileQuality(const ChessBoard& board, colorType side, int fileX) {
    bool ownPawn = false;
    bool oppPawn = false;
    const colorType opp = ChessBoard::oppColor(side);
    for (int y = 0; y < BOARDHEIGHT; ++y) {
        const Grid g = board.getGridAt(fileX, y);
        if (g.type != Pawn) {
            continue;
        }
        if (g.color == side) {
            ownPawn = true;
        } else if (g.color == opp) {
            oppPawn = true;
        }
    }
    if (!ownPawn && !oppPawn) {
        return 2;
    }
    if (!ownPawn && oppPawn) {
        return 1;
    }
    return 0;
}

int bishopDevelopmentReplyPotential(const ChessBoard& board, colorType side, int flank) {
    int best = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side || g.type != Bishop) {
                continue;
            }
            if (!onFlank(x, flank) && x != 4) {
                continue;
            }
            for (int d = 0; d < 4; ++d) {
                const int tx = x + dx_bishop[d];
                const int ty = y + dy_bishop[d];
                const int eyeX = x + dx_bishop_eye[d];
                const int eyeY = y + dy_bishop_eye[d];
                if (!ChessBoard::inBoard(tx, ty) || !ChessBoard::inColorArea(tx, ty, side)) {
                    continue;
                }
                if (board.getGridAt(eyeX, eyeY).color != EMPTY || board.getGridAt(tx, ty).color != EMPTY) {
                    continue;
                }

                int improve = 0;
                if (std::abs(tx - 4) < std::abs(x - 4)) {
                    improve += 6;
                }
                if (advanceOf(side, ty) > advanceOf(side, y)) {
                    improve += 4;
                }
                if (tx == 4) {
                    improve += 4;
                }
                best = std::max(best, improve);
            }
        }
    }
    return best;
}

int flankDefenseScore(const ChessBoard& board, colorType side, int flank) {
    int score = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side) {
                continue;
            }

            const bool mainFlank = onFlank(x, flank);
            switch (g.type) {
                case Rook:
                    score += mainFlank ? 6 : (x == 4 ? 2 : 0);
                    break;
                case Cannon:
                    score += mainFlank ? 5 : (x == 4 ? 2 : 0);
                    break;
                case Knight:
                    score += mainFlank ? 5 : (x == 4 ? 1 : 0);
                    break;
                case Bishop:
                case Assistant:
                    score += (mainFlank || x == 4) ? 2 : 0;
                    break;
                case Pawn:
                    if (mainFlank) {
                        score += crossedRiver(side, y) ? 2 : 1;
                    }
                    break;
                case King:
                    score += (x == 4 || mainFlank) ? 1 : 0;
                    break;
                default:
                    break;
            }
        }
    }
    return score;
}

int weakerFlank(const ChessBoard& board, colorType defender) {
    const int left = flankDefenseScore(board, defender, 0);
    const int right = flankDefenseScore(board, defender, 1);
    if (std::abs(left - right) < 3) {
        return -1;
    }
    return left < right ? 0 : 1;
}

int attackTowardsWeakerFlankBonus(const ChessBoard& board, colorType attacker,
                                  int x, int y, stoneType type) {
    if (type != Rook && type != Knight && type != Cannon) {
        return 0;
    }

    if (type == Rook && !isDevelopedRook(attacker, x, y)) {
        return 0;
    }
    if (type == Knight && !isDevelopedKnight(attacker, x, y)) {
        return 0;
    }
    if (type == Cannon && x != 4 && advanceOf(attacker, y) < 2) {
        return 0;
    }

    const colorType defender = ChessBoard::oppColor(attacker);
    const int weakFlank = weakerFlank(board, defender);
    if (weakFlank < 0) {
        return 0;
    }

    const int leftDefense = flankDefenseScore(board, defender, 0);
    const int rightDefense = flankDefenseScore(board, defender, 1);
    const int diff = std::abs(leftDefense - rightDefense);
    const int pieceFlank = flankIndexFromFile(x);
    const int base = (type == Rook) ? 14 : (type == Cannon ? 12 : 10);

    if (pieceFlank < 0) {
        if (x == 4 && (type == Rook || type == Cannon) && advanceOf(attacker, y) >= 2) {
            return 4 + diff;
        }
        return 0;
    }

    if (pieceFlank == weakFlank) {
        return base + diff * 2 + (advanceOf(attacker, y) >= 5 ? 4 : 0);
    }
    if (advanceOf(attacker, y) >= 4) {
        return -(base / 2) - diff;
    }
    return 0;
}

bool cannonAttacksTargetOnLine(const ChessBoard& board, int cannonX, int cannonY, int targetX, int targetY) {
    return ChessBoard::inSameStraightLine(cannonX, cannonY, targetX, targetY) &&
           countPiecesBetween(board, cannonX, cannonY, targetX, targetY) == 1;
}

bool cannonPressesFlankKnight(const ChessBoard& board, colorType attacker, int cannonX, int cannonY, int flank) {
    const colorType defender = ChessBoard::oppColor(attacker);
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != defender || g.type != Knight || !onFlank(x, flank)) {
                continue;
            }
            if (cannonAttacksTargetOnLine(board, cannonX, cannonY, x, y)) {
                return true;
            }
        }
    }
    return false;
}

bool cannonOnlyHarassesFlankPawn(const ChessBoard& board, colorType attacker, int cannonX, int cannonY, int flank) {
    const colorType defender = ChessBoard::oppColor(attacker);
    bool attacksFlankPawn = false;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != defender || g.type != Pawn || !onFlank(x, flank)) {
                continue;
            }
            if ((x == 2 || x == 6) && cannonAttacksTargetOnLine(board, cannonX, cannonY, x, y)) {
                attacksFlankPawn = true;
            }
        }
    }
    if (!attacksFlankPawn) {
        return false;
    }
    return !cannonPressesFlankKnight(board, attacker, cannonX, cannonY, flank);
}

int cannonNaturalReliefPenalty(ChessBoard& board, const Move& move, colorType side) {
    const Grid source = board.getGridAt(move.source_x, move.source_y);
    if (source.type != Cannon) {
        return 0;
    }

    const int flank = flankIndexFromFile(move.target_x);
    if (flank < 0 || advanceOf(side, move.target_y) < 3) {
        return 0;
    }

    board.makeMoveAssumeLegal(move);
    const colorType defender = board.currentColor();
    const int bishopReply = bishopDevelopmentReplyPotential(board, defender, flank);
    const bool pressesKnight = cannonPressesFlankKnight(board, side, move.target_x, move.target_y, flank);
    const bool onlyHarassPawn = cannonOnlyHarassesFlankPawn(board, side, move.target_x, move.target_y, flank);

    int penalty = 0;
    if (pressesKnight) {
        penalty += 120;
        if (bishopReply >= 6) {
            penalty += 220;
        }
    }
    if (onlyHarassPawn) {
        penalty += 140;
        if (bishopReply >= 4) {
            penalty += 140;
        }
    }
    if (!isCannonMoveUseful(board, move, side, 256) && bishopReply > 0) {
        penalty += 80 + bishopReply * 5;
    }

    board.undoMove();
    return penalty;
}

bool isClassicWocaoSquare(colorType defender, int x, int y) {
    if (defender == RED) {
        return ((x == 3 || x == 5) && y == 2) || ((x == 2 || x == 6) && y == 1);
    }
    return ((x == 3 || x == 5) && y == 7) || ((x == 2 || x == 6) && y == 8);
}

bool isClassicShijiaoSquare(colorType defender, int x, int y) {
    if (defender == RED) {
        return y == 2 && (x == 2 || x == 6);
    }
    return y == 7 && (x == 2 || x == 6);
}

bool isBacktrack(const Move& newer, const Move& older) {
    return !older.isInvalid() &&
           newer.source_x == older.target_x &&
           newer.source_y == older.target_y &&
           newer.target_x == older.source_x &&
           newer.target_y == older.source_y;
}

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

bool knightAttacksSquare(const ChessBoard& board, int knightX, int knightY, int targetX, int targetY) {
    for (int d = 0; d < 8; ++d) {
        if (knightX + dx_knight[d] != targetX || knightY + dy_knight[d] != targetY) {
            continue;
        }
        const int footX = knightX + dx_knight_foot[d];
        const int footY = knightY + dy_knight_foot[d];
        if (board.getGridAt(footX, footY).color == EMPTY) {
            return true;
        }
    }
    return false;
}

// 判断炮的移动是否形成了有意义的战术构型（中路/空头炮/炮架/对敌将线）
bool isCannonMoveUseful(const ChessBoard& board, const Move& move, colorType side, int /*phase*/) {
    const int tx = move.target_x;
    const int ty = move.target_y;
    const colorType opp = ChessBoard::oppColor(side);

    // 中炮位置
    if (tx == 4) return true;

    // 对准对方帅的线路
    int oppKX = -1, oppKY = -1;
    board.findKing(opp, oppKX, oppKY);
    if (oppKX >= 0) {
        if (tx == oppKX || ty == oppKY) {
            // 在同一线上且有炮架 => 有威胁
            if (tx == oppKX) {
                int between = countPiecesBetween(board, tx, ty, oppKX, oppKY);
                if (between == 1) return true; // 有炮架，形成威胁
                if (between == 0) return true; // 空头压制
            }
            if (ty == oppKY) {
                int between = countPiecesBetween(board, tx, ty, oppKX, oppKY);
                if (between == 1) return true;
            }
        }
    }

    // 肋道炮（3路或5路）需要有实际配合才有用
    if (tx == 3 || tx == 5) {
        // 同列有己方车 → 车炮配合
        for (int fy = 0; fy < BOARDHEIGHT; ++fy) {
            if (fy == ty) continue;
            const Grid g = board.getGridAt(tx, fy);
            if (g.color == side && g.type == Rook) return true;
        }
        // 肋道且深入对方阵地接近帅 → 有压制价值
        int advLocal = advanceOf(side, ty);
        if (oppKX >= 0 && std::abs(tx - oppKX) <= 1 && advLocal >= 7) {
            return true;
        }
    }

    // 沉底炮位置（进入对方底线/二线附近）
    int adv = advanceOf(side, ty);
    if (adv >= 8) return true; // 沉底

    return false;
}

} // namespace

// =====================================================================
// PST（红方视角，x=0..8, y=0..9）
// =====================================================================
static const int PST_PAWN[BOARDWIDTH][BOARDHEIGHT] = {
    {  0,  0,  0,  0,  2,  1,  1,  1,  1, -5},
    {  0,  0,  0,  0,  1,  3,  3,  3,  4, -5},
    {  0,  0,  0,  0,  4,  6,  6,  8, 10, -5},
    {  0,  0,  0,  0,  6,  8, 10, 10, 14, -5},
    {  0,  0,  0,  0,  8, 10, 12, 12, 12, -6},
    {  0,  0,  0,  0,  6,  8, 10, 10, 14, -5},
    {  0,  0,  0,  0,  4,  6,  8,  8, 10, -5},
    {  0,  0,  0,  0,  1,  3,  3,  3,  4, -5},
    {  0,  0,  0,  0,  2,  1,  1,  1,  1, -5}
};

static const int PST_KNIGHT[BOARDWIDTH][BOARDHEIGHT] = {
    {-15,  0,  8, -4,  2,  2,  0,  8, -2,  0},
    { -8,-10,  6,  0,  5,  0, 10,  4,  4,  4},
    { -5, -4, 10, -2, 15,  4,  6,  8, 15,  2},
    { -4,  6,  4, -1,  8,  8, 10, 10,  4,  2},
    {-30, -5,  0,  8,  7,  2,  8,  8,  0,  0},
    { -4,  6,  4, -1,  8,  8, 10, 10,  4,  2},
    { -5, -4, 10, -2, 15,  4,  6,  8, 15,  2},
    { -8,-10,  6,  0,  5,  0, 10,  4,  4,  4},
    {-15,  0,  8, -4,  2,  2,  0,  8, -2,  0}
};

static const int PST_ROOK[BOARDWIDTH][BOARDHEIGHT] = {
    {  0,  6,  4,  2,  6,  6,  7,  0,  0,  4},
    {  8, 10,  6,  2, 12,  8, 12,  8, 10,  6},
    {  6,  4,  4,  2, 14,  0, 10,  6,  6,  5},
    {  9, 14,  8,  2, 12, 10, 12,  6, 10,  8},
    { -5, -4, -3,  2,  4,  3, 11,  2, -4, -6},
    {  9, 14,  8,  2, 12, 10, 12,  6, 10,  8},
    {  6,  4,  4,  2, 14,  0, 10,  6,  6,  5},
    {  8, 10,  6,  2, 12,  8, 12,  8, 10,  6},
    {  0,  6,  4,  2,  6,  6,  7,  0,  0,  4}
};

static const int PST_CANNON[BOARDWIDTH][BOARDHEIGHT] = {
    { -8,  4, 10,  2,  4,  6,  8,  7,  6, 14},
    {  2,  6,  6,  2,  8,  8,  8,  6,  6, 14},
    {  4,  4, 12,  2,  6,  2,  6,  7,  4,  6},
    {  6,  0, 12,  2,  4,  2,  6,  8,  6,  8},
    {-15,  6, 15,  6,  8,  6, 12,  8,  4,-10},
    {  6,  0, 12,  2,  4,  2,  6,  8,  6,  8},
    {  4,  4, 12,  2,  6,  2,  6,  7,  4,  6},
    {  2,  6,  6,  2,  8,  8,  8,  6,  6, 14},
    { -8,  4, 10,  2,  4,  6,  8,  7,  6, 14}
};

static const int PST_BISHOP[BOARDWIDTH][BOARDHEIGHT] = {
    { 0, 0, 4, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 6, 0, 0, 0, 2, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 10,0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 6, 0, 0, 0, 2, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 4, 0, 0, 0, 0, 0, 0, 0}
};

static const int PST_GUARD[BOARDWIDTH][BOARDHEIGHT] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 5, 0, 3, 0, 0, 0, 0, 0, 0, 0},
    { 0, 8, 0, 0, 0, 0, 0, 0, 0, 0},
    { 5, 0, 3, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static const int PST_KING[BOARDWIDTH][BOARDHEIGHT] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {10, 8, 2, 0, 0, 0, 0, 0, 0, 0},
    {20, 5, 3, 0, 0, 0, 0, 0, 0, 0},
    {10, 8, 2, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// =====================================================================
// LMR 表
// =====================================================================
static int lmrTable[64][64];
static struct LMRInit {
    LMRInit() {
        for (int d = 0; d < 64; ++d) {
            for (int m = 0; m < 64; ++m) {
                lmrTable[d][m] = static_cast<int>(0.5 + std::log(d + 1.0) * std::log(m + 1.0) / 2.5);
            }
        }
    }
} lmrInit_;

AIPlayer::AIPlayer()
    : tt_(),
      searchStart_(std::chrono::steady_clock::now()),
      timeUp_(false),
      nodesSearched_(0),
      searchAge_(0),
      allocatedTimeMs_(HARD_TIME_BASE_MS),
      softTimeMs_(SOFT_TIME_BASE_MS),
      hardTimeMs_(HARD_TIME_BASE_MS),
      lastIterationMs_(0),
      completedDepth_(0),
      bestMoveChanges_(0) {
    std::memset(history_, 0, sizeof(history_));
    for (int ply = 0; ply < MAX_DEPTH; ++ply) {
        prevMove_[ply] = Move();
        for (int k = 0; k < MAX_KILLERS; ++k) {
            killers_[ply][k] = Move();
        }
    }
    for (int color = 0; color < 2; ++color) {
        for (int from = 0; from < BOARDWIDTH * BOARDHEIGHT; ++from) {
            for (int to = 0; to < BOARDWIDTH * BOARDHEIGHT; ++to) {
                counterMoves_[color][from][to] = Move();
            }
        }
    }
}

// =====================================================================
// 子力价值：开局 / 残局 / 排序基础值
// =====================================================================
int AIPlayer::pieceValueMg(stoneType t) {
    switch (t) {
        case Rook:      return 850;
        case Cannon:    return 440;
        case Knight:    return 370;
        case Pawn:      return 100;
        case Bishop:    return 200;
        case Assistant: return 200;
        case King:      return 0;
        default:        return 0;
    }
}

int AIPlayer::pieceValueEg(stoneType t) {
    switch (t) {
        case Rook:      return 850;
        case Cannon:    return 330;
        case Knight:    return 460;
        case Pawn:      return 150;
        case Bishop:    return 180;
        case Assistant: return 180;
        case King:      return 0;
        default:        return 0;
    }
}

int AIPlayer::pieceBaseValue(stoneType t) {
    switch (t) {
        case Rook:      return 850;
        case Cannon:    return 400;
        case Knight:    return 400;
        case Pawn:      return 100;
        case Bishop:    return 200;
        case Assistant: return 200;
        case King:      return 10000;
        default:        return 0;
    }
}

// =====================================================================
// phase：0=残局，256=开局
// =====================================================================
int AIPlayer::endgamePhase(const ChessBoard& board) {
    int phase = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color == EMPTY || g.type == King) {
                continue;
            }
            switch (g.type) {
                case Rook:   phase += 44; break;
                case Cannon: phase += 20; break;
                case Knight: phase += 20; break;
                case Pawn:   phase += 6;  break;
                default:     phase += 4;  break;
            }
        }
    }
    return std::min(phase, 256);
}

// =====================================================================
// PST 位置分
// =====================================================================
int AIPlayer::positionValue(stoneType t, colorType c, int x, int y) {
    const int ry = (c == RED) ? y : (9 - y);
    switch (t) {
        case Pawn:      return PST_PAWN[x][ry];
        case Knight:    return PST_KNIGHT[x][ry];
        case Rook:      return PST_ROOK[x][ry];
        case Cannon:    return PST_CANNON[x][ry];
        case Bishop:    return PST_BISHOP[x][ry];
        case Assistant: return PST_GUARD[x][ry];
        case King:      return PST_KING[x][ry];
        default:        return 0;
    }
}

// =====================================================================
// 辅助：统计对方深入我方半场的大子数（车/马/炮）
// =====================================================================
int AIPlayer::countEnemyDeepMajors(const ChessBoard& board, colorType side) const {
    const colorType opp = ChessBoard::oppColor(side);
    int count = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != opp) continue;
            if (g.type != Rook && g.type != Knight && g.type != Cannon) continue;
            // 对方子已过河进入我方半场
            if (crossedRiver(opp, y)) {
                ++count;
            }
        }
    }
    return count;
}

// =====================================================================
// 评估 1：子力
// =====================================================================
int AIPlayer::evalMaterial(const ChessBoard& board, colorType side, int phase) const {
    int mgScore = 0;
    int egScore = 0;
    const colorType opp = ChessBoard::oppColor(side);
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color == EMPTY || g.type == King) {
                continue;
            }
            if (g.color == side) {
                mgScore += pieceValueMg(g.type);
                egScore += pieceValueEg(g.type);
            } else if (g.color == opp) {
                mgScore -= pieceValueMg(g.type);
                egScore -= pieceValueEg(g.type);
            }
        }
    }
    return (mgScore * phase + egScore * (256 - phase)) / 256;
}

// =====================================================================
// 评估 2：位置与贴将压迫
// =====================================================================
int AIPlayer::evalPosition(const ChessBoard& board, colorType side) const {
    int score = 0;
    const colorType opp = ChessBoard::oppColor(side);
    int oppKingX = -1;
    int oppKingY = -1;
    board.findKing(opp, oppKingX, oppKingY);

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color == EMPTY) {
                continue;
            }
            const int pst = positionValue(g.type, g.color, x, y);
            if (g.color == side) {
                score += pst;
                if (oppKingX >= 0) {
                    const int dist = std::abs(x - oppKingX) + std::abs(y - oppKingY);
                    if (g.type == Rook) {
                        if (x == oppKingX) {
                            score += 14;
                        } else if (y == oppKingY) {
                            score += 10;
                        }
                        if (dist <= 3) {
                            score += (4 - dist) * 3;
                        }
                    } else if (g.type == Knight && dist <= 3) {
                        score += (4 - dist) * 2;
                    } else if (g.type == Cannon && (x == oppKingX || y == oppKingY)) {
                        score += 8;
                    } else if (g.type == Pawn && dist <= 2) {
                        score += (3 - dist) * 3;
                    }
                }
            } else {
                score -= pst;
            }
        }
    }
    return score;
}

// =====================================================================
// 帅门格点危险分
// =====================================================================
int AIPlayer::scoreSquareDanger(const ChessBoard& board, colorType side, int x, int y) const {
    const colorType attacker = ChessBoard::oppColor(side);
    int rookThreats = 0;
    int cannonThreats = 0;

    for (int d = 0; d < 4; ++d) {
        int tx = x + dx_strai[d];
        int ty = y + dy_strai[d];
        bool seenScreen = false;
        while (ChessBoard::inBoard(tx, ty)) {
            const Grid g = board.getGridAt(tx, ty);
            if (g.color == EMPTY) {
                tx += dx_strai[d];
                ty += dy_strai[d];
                continue;
            }

            if (!seenScreen) {
                if (g.color == attacker) {
                    if (g.type == Rook) {
                        ++rookThreats;
                        break;
                    }
                    if (g.type == King && tx == x) {
                        ++rookThreats;
                        break;
                    }
                }
                seenScreen = true;
            } else {
                if (g.color == attacker && g.type == Cannon) {
                    ++cannonThreats;
                }
                break;
            }

            tx += dx_strai[d];
            ty += dy_strai[d];
        }
    }

    int knightThreats = 0;
    for (int d = 0; d < 8; ++d) {
        const int knightX = x - dx_knight[d];
        const int knightY = y - dy_knight[d];
        if (!ChessBoard::inBoard(knightX, knightY)) {
            continue;
        }
        const Grid knight = board.getGridAt(knightX, knightY);
        if (knight.color != attacker || knight.type != Knight) {
            continue;
        }
        const int footX = knightX + dx_knight_foot[d];
        const int footY = knightY + dy_knight_foot[d];
        if (board.getGridAt(footX, footY).color == EMPTY) {
            ++knightThreats;
        }
    }

    int pawnThreats = 0;
    const int pawnY = y - forwardDir(attacker);
    if (ChessBoard::inBoard(x, pawnY)) {
        const Grid g = board.getGridAt(x, pawnY);
        if (g.color == attacker && g.type == Pawn) {
            ++pawnThreats;
        }
    }
    if (crossedRiver(attacker, y)) {
        if (x > 0) {
            const Grid g = board.getGridAt(x - 1, y);
            if (g.color == attacker && g.type == Pawn) {
                ++pawnThreats;
            }
        }
        if (x < BOARDWIDTH - 1) {
            const Grid g = board.getGridAt(x + 1, y);
            if (g.color == attacker && g.type == Pawn) {
                ++pawnThreats;
            }
        }
    }

    int danger = rookThreats * 28 + cannonThreats * 24 + knightThreats * 18 + pawnThreats * 10;
    if (rookThreats + cannonThreats >= 2) {
        danger += 16;
    }
    if (knightThreats >= 2) {
        danger += 10;
    }
    return danger;
}

// =====================================================================
// 帅门安全模式识别
// =====================================================================
AIPlayer::KingDangerInfo AIPlayer::analyzeKingDanger(const ChessBoard& board, colorType side) const {
    KingDangerInfo info;
    if (!board.findKing(side, info.kingX, info.kingY)) {
        info.totalDanger = 240;
        return info;
    }

    const colorType opp = ChessBoard::oppColor(side);
    const int homeY = homeRank(side);
    const int dir = forwardDir(side);
    std::vector<std::pair<int, int>> enemyRooks;

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color == side) {
                if (g.type == Assistant) {
                    ++info.guardCount;
                } else if (g.type == Bishop) {
                    ++info.bishopCount;
                }
                if (inPalace(side, x, y) && g.type != King) {
                    ++info.palaceDefenders;
                }
            } else if (g.color == opp && g.type == Rook) {
                enemyRooks.push_back(std::make_pair(x, y));
            }
        }
    }

    for (int step = 1; step <= 3; ++step) {
        const int y = homeY + dir * step;
        if (!ChessBoard::inBoard(4, y)) {
            break;
        }
        const Grid g = board.getGridAt(4, y);
        if (g.color != side) {
            ++info.centerFileOpen;
        }
        if (g.color != EMPTY) {
            break;
        }
    }

    const int frontY = info.kingY + dir;
    if (ChessBoard::inBoard(info.kingX, frontY) && board.getGridAt(info.kingX, frontY).color == EMPTY) {
        ++info.frontOpen;
    }
    const int front2Y = info.kingY + dir * 2;
    if (ChessBoard::inBoard(info.kingX, front2Y) && board.getGridAt(info.kingX, front2Y).color == EMPTY) {
        ++info.frontOpen;
    }
    if (info.kingX > 3 && board.getGridAt(info.kingX - 1, info.kingY).color == EMPTY) {
        ++info.frontOpen;
    }
    if (info.kingX < 5 && board.getGridAt(info.kingX + 1, info.kingY).color == EMPTY) {
        ++info.frontOpen;
    }

    info.currentSquareDanger = scoreSquareDanger(board, side, info.kingX, info.kingY);
    int bestPalaceDanger = info.currentSquareDanger;
    for (int px = 3; px <= 5; ++px) {
        for (int py = palaceMinRank(side); py <= palaceMaxRank(side); ++py) {
            const Grid g = board.getGridAt(px, py);
            if (g.color == side && !(px == info.kingX && py == info.kingY)) {
                continue;
            }
            const int squareDanger = scoreSquareDanger(board, side, px, py);
            if (squareDanger <= 14) {
                ++info.safePalaceSquares;
            }
            if (squareDanger >= 24) {
                ++info.attackedPalaceSquares;
            }
            if (!(px == info.kingX && py == info.kingY)) {
                bestPalaceDanger = std::min(bestPalaceDanger, squareDanger);
            }
        }
    }
    info.escapeImprovement = std::max(0, info.currentSquareDanger - bestPalaceDanger);

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != opp) {
                continue;
            }

            switch (g.type) {
                case Cannon: {
                    if ((x == info.kingX || y == info.kingY) &&
                        countPiecesBetween(board, x, y, info.kingX, info.kingY) == 1) {
                        ++info.directCannonPressure;
                    }

                    if ((y == homeY || y == homeY + dir) && std::abs(x - 4) <= 1) {
                        const int keyXs[4] = {3, 4, 5, 4};
                        const int keyYs[4] = {homeY, homeY, homeY, homeY + dir};
                        int press = 0;
                        for (int i = 0; i < 4; ++i) {
                            if (!ChessBoard::inBoard(keyXs[i], keyYs[i])) {
                                continue;
                            }
                            if ((x == keyXs[i] || y == keyYs[i]) &&
                                countPiecesBetween(board, x, y, keyXs[i], keyYs[i]) == 1) {
                                ++press;
                            }
                        }
                        if (press > 0) {
                            const int add = std::min(2, press);
                            info.bottomCannonThreat += add;
                            if (x < 4) {
                                info.bottomCannonSide -= add;
                            } else if (x > 4) {
                                info.bottomCannonSide += add;
                            }
                        }
                    }
                    break;
                }
                case Rook: {
                    if ((x == info.kingX || y == info.kingY) &&
                        countPiecesBetween(board, x, y, info.kingX, info.kingY) == 0) {
                        ++info.directRookPressure;
                    }
                    if (x >= 3 && x <= 5) {
                        const int laneY = ChessBoard::inBoard(x, homeY + dir) ? (homeY + dir) : homeY;
                        if (countPiecesBetween(board, x, y, x, laneY) == 0) {
                            ++info.doubleCheckLanes;
                        }
                        if (countPiecesBetween(board, x, y, x, homeY) <= 1) {
                            ++info.sideFilePressure;
                        }
                    }
                    break;
                }
                case Knight: {
                    if (knightAttacksSquare(board, x, y, info.kingX, info.kingY)) {
                        info.horseThreat += 2;
                    } else {
                        int palaceHits = 0;
                        for (int px = 3; px <= 5; ++px) {
                            for (int py = palaceMinRank(side); py <= palaceMaxRank(side); ++py) {
                                if ((px == info.kingX && py == info.kingY) || !inPalace(side, px, py)) {
                                    continue;
                                }
                                if (knightAttacksSquare(board, x, y, px, py)) {
                                    ++palaceHits;
                                }
                            }
                        }
                        if (palaceHits > 0) {
                            info.horseThreat += 1 + (palaceHits > 1 ? 1 : 0);
                        }
                    }
                    if (isClassicWocaoSquare(side, x, y)) {
                        ++info.wocaoThreat;
                    }
                    if (isClassicShijiaoSquare(side, x, y)) {
                        ++info.shijiaoThreat;
                    }
                    break;
                }
                case Pawn: {
                    if (advanceOf(opp, y) >= 5 && std::abs(x - info.kingX) + std::abs(y - info.kingY) <= 2) {
                        ++info.pawnStorm;
                    }
                    if (x >= 3 && x <= 5 && crossedRiver(opp, y)) {
                        ++info.sideFilePressure;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (info.bottomCannonThreat > 0) {
        for (const auto& rookPos : enemyRooks) {
            const int rookX = rookPos.first;
            const int rookY = rookPos.second;
            if (rookX < 3 || rookX > 5) {
                continue;
            }
            const int frontLaneY = ChessBoard::inBoard(rookX, homeY + dir) ? (homeY + dir) : homeY;
            if (countPiecesBetween(board, rookX, rookY, rookX, frontLaneY) <= 1) {
                ++info.rookCannonThreat;
            }
            if (countPiecesBetween(board, rookX, rookY, rookX, homeY) <= 1) {
                ++info.rookCannonThreat;
            }
            if (info.bottomCannonSide < 0 && rookX == 3) {
                ++info.rookCannonThreat;
            }
            if (info.bottomCannonSide > 0 && rookX == 5) {
                ++info.rookCannonThreat;
            }
            if (info.bottomCannonSide == 0 && rookX == 4) {
                ++info.rookCannonThreat;
            }
        }
    }

    if (info.guardCount == 0 && info.wocaoThreat > 0) {
        ++info.wocaoThreat;
    }
    if (info.guardCount < 2 && info.shijiaoThreat > 0) {
        ++info.shijiaoThreat;
    }
    if (info.bishopCount == 0 && info.horseThreat > 0) {
        ++info.horseThreat;
    }

    int danger = 0;
    danger += (2 - info.guardCount) * 18;
    danger += (2 - info.bishopCount) * 12;
    danger += info.frontOpen * 8;
    danger += info.centerFileOpen * 12;
    danger += info.sideFilePressure * 6;
    danger += info.currentSquareDanger * 2;
    danger += info.directRookPressure * 24;
    danger += info.directCannonPressure * 20;
    danger += info.bottomCannonThreat * 24;
    danger += info.rookCannonThreat * 28;
    danger += info.horseThreat * 12;
    danger += info.wocaoThreat * 22;
    danger += info.shijiaoThreat * 18;
    danger += info.doubleCheckLanes * 12;
    danger += info.pawnStorm * 8;
    danger += std::max(0, 3 - info.safePalaceSquares) * 10;
    danger += info.attackedPalaceSquares * 4;
    danger += (info.palaceDefenders <= 1) ? 10 : 0;
    danger -= info.escapeImprovement;
    info.totalDanger = clampInt(danger, 0, 240);
    return info;
}

// =====================================================================
// 评估 3：帅门安全与条件化帅位
// =====================================================================
int AIPlayer::evalKingSafety(const ChessBoard& board, colorType side) const {
    const KingDangerInfo info = analyzeKingDanger(board, side);
    if (info.kingX < 0) {
        return -MATE_SCORE;
    }

    int score = 0;
    score += info.guardCount * 26;
    score += info.bishopCount * 18;
    score += info.palaceDefenders * 7;
    score += info.safePalaceSquares * 6;

    score -= info.frontOpen * 9;
    score -= info.centerFileOpen * 12;
    score -= info.sideFilePressure * 6;
    score -= info.currentSquareDanger * 3;
    score -= info.bottomCannonThreat * 34;
    score -= info.rookCannonThreat * 46;
    score -= info.directRookPressure * 18;
    score -= info.directCannonPressure * 16;
    score -= info.horseThreat * 16;
    score -= info.wocaoThreat * 40;
    score -= info.shijiaoThreat * 30;
    score -= info.doubleCheckLanes * 18;
    score -= info.pawnStorm * 10;
    score -= std::max(0, 2 - info.guardCount) * 12;
    score -= std::max(0, 2 - info.bishopCount) * 8;

    const int homeY = homeRank(side);
    const int displacement = std::abs(info.kingX - 4) * 18 + std::abs(info.kingY - homeY) * 14;
    const int placementWeight = std::max(26, 110 - info.totalDanger);

    score += 70;
    score -= displacement * placementWeight / 100;
    if (info.kingX == 4 && info.kingY == homeY && info.totalDanger < 70) {
        score += 24;
    }

    if (info.totalDanger >= 80) {
        score += std::min(info.escapeImprovement, 32);
        if (info.bottomCannonSide < 0 && info.kingX == 5) {
            score += 18;
        }
        if (info.bottomCannonSide > 0 && info.kingX == 3) {
            score += 18;
        }
        if (info.kingY != homeY && info.currentSquareDanger <= 20) {
            score += 8;
        }
    } else {
        if (info.kingX != 4) {
            score -= 6;
        }
        if (info.kingY != homeY) {
            score -= 10;
        }
    }

    return score;
}

// =====================================================================
// 评估 4：机动性
// =====================================================================
int AIPlayer::evalMobility(const ChessBoard& board, colorType side, int phase) const {
    int score = 0;
    const colorType opp = ChessBoard::oppColor(side);

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side) {
                continue;
            }

            if (g.type == Rook) {
                int mobility = 0;
                for (int d = 0; d < 4; ++d) {
                    int tx = x + dx_strai[d];
                    int ty = y + dy_strai[d];
                    while (ChessBoard::inBoard(tx, ty)) {
                        const Grid tg = board.getGridAt(tx, ty);
                        if (tg.color == EMPTY) {
                            ++mobility;
                        } else {
                            if (tg.color == opp) {
                                ++mobility;
                            }
                            break;
                        }
                        tx += dx_strai[d];
                        ty += dy_strai[d];
                    }
                }
                score += mobility * 2;
                bool ownPawnOnFile = false;
                for (int fy = 0; fy < BOARDHEIGHT; ++fy) {
                    if (fy == y) {
                        continue;
                    }
                    const Grid filePiece = board.getGridAt(x, fy);
                    if (filePiece.color == side && filePiece.type == Pawn) {
                        ownPawnOnFile = true;
                        break;
                    }
                }
                if (!ownPawnOnFile) {
                    score += 8;
                }
                const int adv = advanceOf(side, y);
                if (adv >= 5) {
                    score += (adv - 4) * 4;
                }
            }

            if (g.type == Knight) {
                int jumps = 0;
                int strongJumps = 0;
                for (int d = 0; d < 8; ++d) {
                    const int tx = x + dx_knight[d];
                    const int ty = y + dy_knight[d];
                    if (!ChessBoard::inBoard(tx, ty)) {
                        continue;
                    }
                    const int footX = x + dx_knight_foot[d];
                    const int footY = y + dy_knight_foot[d];
                    if (board.getGridAt(footX, footY).color != EMPTY) {
                        continue;
                    }
                    if (board.getGridAt(tx, ty).color == side) {
                        continue;
                    }
                    ++jumps;
                    if (advanceOf(side, ty) >= 5) {
                        ++strongJumps;
                    }
                }
                const int knightMul = (phase < 128) ? 4 : 3;
                score += jumps * knightMul + strongJumps * 2;
            }

            if (g.type == Cannon) {
                int mobility = 0;
                int threats = 0;
                for (int d = 0; d < 4; ++d) {
                    int tx = x + dx_strai[d];
                    int ty = y + dy_strai[d];
                    bool foundMount = false;
                    while (ChessBoard::inBoard(tx, ty)) {
                        const Grid tg = board.getGridAt(tx, ty);
                        if (!foundMount) {
                            if (tg.color == EMPTY) {
                                ++mobility;
                            } else {
                                foundMount = true;
                            }
                        } else if (tg.color != EMPTY) {
                            if (tg.color == opp) {
                                ++threats;
                            }
                            break;
                        }
                        tx += dx_strai[d];
                        ty += dy_strai[d];
                    }
                }
                score += mobility + threats * 6;
            }
        }
    }
    return score;
}

// =====================================================================
// 评估 5：兵形
// =====================================================================
int AIPlayer::evalPawnStructure(const ChessBoard& board, colorType side) const {
    int score = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side || g.type != Pawn) {
                continue;
            }
            if (advanceOf(side, y) >= 5) {
                if (x > 0 && board.getGridAt(x - 1, y) == Grid(Pawn, side)) {
                    score += 8;
                }
                if (x < BOARDWIDTH - 1 && board.getGridAt(x + 1, y) == Grid(Pawn, side)) {
                    score += 8;
                }
            }
        }
    }
    return score;
}

// =====================================================================
// 评估 6：兵的推进
// =====================================================================
int AIPlayer::evalPawnAdvancement(const ChessBoard& board, colorType side, int phase) const {
    int score = 0;
    const colorType opp = ChessBoard::oppColor(side);
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.type != Pawn) {
                continue;
            }
            if (g.color == side) {
                const int adv = advanceOf(side, y);
                if (adv >= 5) {
                    score += 100;
                    score += (adv - 5) * ((phase < 128) ? 15 : 10);
                }
            } else if (g.color == opp) {
                const int adv = advanceOf(opp, y);
                if (adv >= 5) {
                    score -= 100;
                    score -= (adv - 5) * ((phase < 128) ? 15 : 10);
                }
            }
        }
    }
    return score;
}

// =====================================================================
// 评估 7：攻击压力（含"帮对方活子"惩罚）
// =====================================================================
int AIPlayer::evalAttackPressure(const ChessBoard& board, colorType side) const {
    int score = 0;
    const colorType opp = ChessBoard::oppColor(side);
    ChessBoard& mutableBoard = const_cast<ChessBoard&>(board);
    const int phase = endgamePhase(board);
    const int weakFlank = TacticalRules::weakerFlank(board, opp);

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != opp) {
                continue;
            }
            if (!mutableBoard.attacked(side, x, y)) {
                continue;
            }

            // 基础攻击奖励
            if (g.type == Rook) {
                score += 12;
            } else if (g.type == Cannon || g.type == Knight) {
                score += 8;
            } else if (g.type == Assistant || g.type == Bishop) {
                score += 3;
            }

            TacticalRules::RootInfo rootInfo;
            if (g.type != King && g.type != None) {
                rootInfo = TacticalRules::analyzeRoot(board, x, y);
                if (rootInfo.severelyHanging) {
                    score += 20 + pieceBaseValue(g.type) / 35;
                } else if (rootInfo.inDanger) {
                    score += 10 + pieceBaseValue(g.type) / 55;
                } else if (rootInfo.virtualRoot) {
                    score += 5;
                } else if (rootInfo.hasRoot) {
                    score -= 4;
                }
                if (weakFlank >= 0 && TacticalRules::onFlank(x, weakFlank)) {
                    score += rootInfo.hasRoot ? 3 : 9;
                }
            }

            // "帮对方活子"惩罚：如果被攻击的子在差位置或初始受限位置，
            // 被赶走后可能去到更好的位置，此时攻击收益打折
            if (phase > 150) {
                // 通用：对方子当前位置PST很低 => 被赶走后大概率变好
                int piecePst = positionValue(g.type, g.color, x, y);
                if (piecePst <= 4 && (g.type == Rook || g.type == Knight || g.type == Cannon)) {
                    score -= 5;
                }
                // 对方马还在初始位置被攻击 => 赶走它反而帮它出子
                if (g.type == Knight && !isDevelopedKnight(opp, x, y)) {
                    score -= 8;
                }
                // 对方车在角上被骚扰 => 可能帮它出来
                if (g.type == Rook && !isDevelopedRook(opp, x, y)) {
                    score -= 10;
                }
                // 对方炮在初始位置被骚扰 => 可能帮它走到更好位置
                if (g.type == Cannon && !isDevelopedCannon(opp, x, y)) {
                    score -= 5;
                }
                if ((g.type == Rook || g.type == Cannon) && rootInfo.hasRoot && !rootInfo.virtualRoot) {
                    score -= 4; // 有实根的车炮不应被盲目高估为攻击成果
                }
            }
        }
    }
    return score;
}

// =====================================================================
// 评估 8：开局出子（含六大子均衡发展）
// =====================================================================
int AIPlayer::evalDevelopment(const ChessBoard& board, colorType side, int phase) const {
    if (phase <= 100) {
        return 0;
    }

    int score = 0;
    int developedRooks = 0;
    int developedKnights = 0;
    int developedCannons = 0;
    int centeredCannons = 0;
    int normalRooks = 0;
    int normalKnights = 0;
    int activatedKnights = 0;
    int usefulCannons = 0;
    int totalRooks = 0;
    int totalKnights = 0;
    int totalCannons = 0;
    bool leftKnightNormal = false;
    bool rightKnightNormal = false;
    bool leftKnightActivated = false;
    bool rightKnightActivated = false;

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side) {
                continue;
            }
            if (g.type == Rook) {
                ++totalRooks;
                if (isDevelopedRook(side, x, y)) {
                    const TacticalRules::RootInfo root = TacticalRules::analyzeRoot(board, x, y);
                    ++developedRooks;
                    score += 12;
                    if (y == homeRank(side) && (x == 1 || x == 7)) {
                        ++normalRooks;
                        score += 18; // 车一平二式的正常出车
                    }
                    if (y == homeRank(side) && (x == 3 || x == 5)) {
                        ++normalRooks;
                        score += 26; // 从士位出车更具人类布局感
                    }
                    if (advanceOf(side, y) >= 2) {
                        score += 18; // 进一步出车
                    }
                    const int fileQuality = rookFileQuality(board, side, x);
                    if (fileQuality == 2) {
                        score += 10;
                    } else if (fileQuality == 1) {
                        score += 6;
                    }
                    if (root.hasRoot) {
                        score += 6;
                    } else if (root.virtualRoot) {
                        score -= 8;
                    } else if (root.inDanger) {
                        score -= 14;
                    }
                } else if (x == 0 || x == 8) {
                    score -= 32; // 长时间闷在底线角上
                } else {
                    score -= 18;
                }
            } else if (g.type == Knight) {
                ++totalKnights;
                const int jumpCount = countKnightJumpOptions(board, side, x, y);
                const int forwardJumps = countKnightForwardJumps(board, side, x, y);
                const int blockedLegs = countKnightBlockedLegs(board, x, y);
                if (isDevelopedKnight(side, x, y)) {
                    const TacticalRules::RootInfo root = TacticalRules::analyzeRoot(board, x, y);
                    ++developedKnights;
                    score += 8;
                    score += jumpCount * 3 + forwardJumps * 4;
                    score -= blockedLegs * 5;

                    if (isNormalKnightSquare(side, x, y)) {
                        ++normalKnights;
                        score += 28; // 明显鼓励正马
                        if (x == 2) {
                            leftKnightNormal = true;
                        } else if (x == 6) {
                            rightKnightNormal = true;
                        }
                    } else if (phase > 170 && advanceOf(side, y) <= 2) {
                        score -= 16; // 早期边马、怪马降权
                    }

                    const int pawnFile = (x <= 4) ? 2 : 6;
                    if (flankPawnActivatesKnight(board, side, x, y, pawnFile)) {
                        ++activatedKnights;
                        score += 16; // 三七兵真正活马时再奖励
                        if (pawnFile == 2) {
                            leftKnightActivated = true;
                        } else {
                            rightKnightActivated = true;
                        }
                    }
                    if (root.hasRoot) {
                        score += 5;
                    } else if (root.virtualRoot) {
                        score -= 7;
                    } else if (root.inDanger) {
                        score -= 12;
                    }
                } else {
                    score -= 20;
                    if (isInitialKnightSquare(side, x, y) && phase > 170) {
                        score -= 6;
                    }
                }
            } else if (g.type == Cannon) {
                ++totalCannons;
                const int adv = advanceOf(side, y);
                const bool useful = isCannonMoveUseful(board, Move(x, y, x, y), side, phase);
                const TacticalRules::RootInfo root = TacticalRules::analyzeRoot(board, x, y);
                if (x == 4 && adv >= 2) {
                    ++centeredCannons;
                    ++developedCannons;
                    ++usefulCannons;
                    score += 22; // 中炮优先
                } else if (isDevelopedCannon(side, x, y)) {
                    ++developedCannons;
                    if (useful) {
                        ++usefulCannons;
                        score += 10;
                    } else {
                        if (phase > 170) {
                            score -= 14; // 炮先动但没形成中炮/炮架/沉底压制
                        }
                        if (adv >= 4) {
                            score -= 12;
                        }
                    }
                    if (phase > 170 && x != 4 && adv <= 2) {
                        score -= 10;
                    }
                    if (phase > 180 && adv >= 4 && x != 4 && !useful) {
                        score -= 18;
                    }
                } else {
                    score -= 6;
                }
                if (root.hasRoot && useful) {
                    score += 6;
                } else if (root.virtualRoot) {
                    score -= 8;
                } else if (root.inDanger && (!useful || adv >= 4)) {
                    score -= 14;
                }
            }
        }
    }

    if (flankPawnAdvanced(board, side, 2) && !leftKnightActivated && !leftKnightNormal && phase > 170) {
        score -= 6; // 三兵已动但没有真正活左马，不鼓励机械冲兵
    }
    if (flankPawnAdvanced(board, side, 6) && !rightKnightActivated && !rightKnightNormal && phase > 170) {
        score -= 6; // 七兵同理
    }

    if (developedRooks == 0) {
        score -= 34;
    } else if (developedRooks == 1 && totalRooks >= 2) {
        score -= 12;
    }
    if (developedKnights == 0) {
        score -= 20;
    }
    if (centeredCannons == 0 && phase > 180) {
        score -= 14;
    }
    if (normalKnights == 0 && phase > 170) {
        score -= 12;
    } else if (normalKnights == 1) {
        score += 8;
    } else if (normalKnights >= 2) {
        score += 18;
    }
    if (normalRooks >= 2) {
        score += 16;
    } else if (normalRooks == 1) {
        score += 6;
    }
    if (activatedKnights == 1) {
        score += 8;
    } else if (activatedKnights >= 2) {
        score += 16;
    }
    if (developedCannons > 0 && usefulCannons == 0 && phase > 180) {
        score -= 16; // 炮都动了，但没有一个是中炮/有用炮
    }

    // ---- 六大子均衡发展奖惩 ----
    const int totalDeveloped = developedRooks + developedKnights + developedCannons;
    const int totalMajors = totalRooks + totalKnights + totalCannons;

    if (phase > 160 && totalMajors >= 4) {
        // 奖励整体展开
        if (totalDeveloped >= 5) {
            score += 26;
        } else if (totalDeveloped >= 4) {
            score += 16;
        } else if (totalDeveloped >= 3) {
            score += 6;
        }

        // 惩罚不均衡：某类大子完全没出，但其他已经出了
        int typesStuck = 0;
        if (totalRooks > 0 && developedRooks == 0) ++typesStuck;
        if (totalKnights > 0 && developedKnights == 0) ++typesStuck;
        if (totalCannons > 0 && developedCannons == 0) ++typesStuck;

        if (typesStuck >= 2 && totalDeveloped >= 1) {
            score -= 26; // 严重不均衡惩罚加重
        } else if (typesStuck == 1 && totalDeveloped >= 2) {
            score -= 12;
        }

        // 特别惩罚：车没出但炮已经在跑 => 发展失调
        if (totalRooks > 0 && developedRooks == 0 && developedCannons > 0) {
            score -= 18;
        }
        // 特别惩罚：只有炮活跃，马车都没出
        if (totalRooks > 0 && developedRooks == 0
            && totalKnights > 0 && developedKnights == 0
            && developedCannons > 0) {
            score -= 22;
        }

        // 双车出动额外奖励
        if (developedRooks >= 2) {
            score += 14;
        }
        // 双马出动额外奖励
        if (developedKnights >= 2) {
            score += 8;
        }
        if (centeredCannons >= 1 && developedCannons >= 2) {
            score += 10; // 双炮形成合理分工，中炮更佳
        }
        // 三类子都至少出一个 → 协调奖励
        if (developedRooks > 0 && developedKnights > 0 && developedCannons > 0) {
            score += 12;
        }
        if (developedRooks >= 2 && normalKnights >= 2) {
            score += 14; // 车马都按常规节奏展开
        }
        if (developedRooks >= 2 && normalKnights >= 2 && centeredCannons >= 1) {
            score += 14; // 六大子整体协调时再给明显奖励
        }
    }

    const int devWeight = std::min(phase - 100, 156);
    return score * devWeight / 156;
}

// =====================================================================
// 评估 9：棋子活跃度（含沉底抽将识别）
// =====================================================================
int AIPlayer::evalPieceActivity(const ChessBoard& board, colorType side, int phase) const {
    int score = 0;
    const colorType opp = ChessBoard::oppColor(side);
    int oppKingX = -1;
    int oppKingY = -1;
    board.findKing(opp, oppKingX, oppKingY);

    int rookCount = 0;
    int rookX[2] = {-1, -1};
    int rookY[2] = {-1, -1};
    const int riverY = (side == RED) ? 4 : 5;
    const int enemyBase = (side == RED) ? 9 : 0;

    // 收集己方沉底车/炮的位置，以及对方高价值子位置
    struct BottomPiece { int x; int y; stoneType type; };
    BottomPiece bottomPieces[4];
    int bottomCount = 0;

    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side) {
                continue;
            }

            if (g.type == Rook) {
                const TacticalRules::RootInfo root = TacticalRules::analyzeRoot(board, x, y);
                if (rookCount < 2) {
                    rookX[rookCount] = x;
                    rookY[rookCount] = y;
                    ++rookCount;
                }
                if (x == 4) {
                    score += 10;
                }
                if (x == 3 || x == 5) {
                    score += 16;
                }
                if (y == riverY || y == riverY + ((side == RED) ? 1 : -1)) {
                    score += 12;
                }
                const int fileQuality = rookFileQuality(board, side, x);
                if (fileQuality == 2) {
                    score += 14; // 开放线上的车更有压制力
                } else if (fileQuality == 1) {
                    score += 8; // 半开放线也应鼓励
                }
                if (phase > 170 && isInitialRookSquare(side, x, y)) {
                    score -= 14; // 车还闷在底线角上
                }
                score += attackTowardsWeakerFlankBonus(board, side, x, y, Rook);
                if (root.hasRoot) {
                    score += 8;
                } else if (root.virtualRoot) {
                    score -= 6;
                } else if (root.inDanger) {
                    score -= 14;
                }
                if (y == enemyBase) {
                    score += 18;
                    // 记录沉底车
                    if (bottomCount < 4) {
                        bottomPieces[bottomCount++] = {x, y, Rook};
                    }
                }
                if (oppKingX >= 0) {
                    const int dist = std::abs(x - oppKingX) + std::abs(y - oppKingY);
                    if (dist <= 2) {
                        score += 12;
                    }
                }
            }

            if (g.type == Knight) {
                const TacticalRules::RootInfo root = TacticalRules::analyzeRoot(board, x, y);
                const int adv = advanceOf(side, y);
                const int jumpCount = countKnightJumpOptions(board, side, x, y);
                const int forwardJumps = countKnightForwardJumps(board, side, x, y);
                const int blockedLegs = countKnightBlockedLegs(board, x, y);
                score += jumpCount * ((phase < 128) ? 4 : 3);
                score += forwardJumps * 3;
                score -= blockedLegs * 4;
                if (isNormalKnightSquare(side, x, y)) {
                    score += 14;
                } else if (phase > 170 && isDevelopedKnight(side, x, y) && adv <= 2) {
                    score -= 12; // 早期怪马、边马
                }
                const int pawnFile = (x <= 4) ? 2 : 6;
                if (flankPawnActivatesKnight(board, side, x, y, pawnFile)) {
                    score += 12;
                }
                score += attackTowardsWeakerFlankBonus(board, side, x, y, Knight);
                if (root.hasRoot) {
                    score += 5;
                } else if (root.virtualRoot) {
                    score -= 5;
                } else if (root.inDanger && adv >= 3) {
                    score -= 10;
                }
                if (adv >= 5) {
                    int strongJumps = 0;
                    for (int d = 0; d < 8; ++d) {
                        const int tx = x + dx_knight[d];
                        const int ty = y + dy_knight[d];
                        if (!ChessBoard::inBoard(tx, ty)) {
                            continue;
                        }
                        const int footX = x + dx_knight_foot[d];
                        const int footY = y + dy_knight_foot[d];
                        if (board.getGridAt(footX, footY).color != EMPTY) {
                            continue;
                        }
                        if (advanceOf(side, ty) >= 5) {
                            ++strongJumps;
                        }
                    }
                    score += strongJumps * ((phase < 128) ? 5 : 3);
                }
            }

            if (g.type == Cannon) {
                const TacticalRules::RootInfo root = TacticalRules::analyzeRoot(board, x, y);
                const int adv = advanceOf(side, y);
                const bool useful = isCannonMoveUseful(board, Move(x, y, x, y), side, phase);
                if (x == 4 && phase > 140) {
                    score += 18;
                    if (oppKingX == 4) {
                        const int minY = std::min(y, oppKingY);
                        const int maxY = std::max(y, oppKingY);
                        int between = 0;
                        for (int fy = minY + 1; fy < maxY; ++fy) {
                            if (board.getGridAt(4, fy).color != EMPTY) {
                                ++between;
                            }
                        }
                        if (between == 1) {
                            score += 38;
                        } else if (between == 0) {
                            score += 28; // 空头炮压中路要进一步奖励
                        }
                    }
                } else if (phase > 180 && adv <= 1) {
                    score -= 8;
                }

                if (useful && x != 4) {
                    score += 8;
                }
                if (!useful && phase > 170 && adv >= 3 && x != 4) {
                    score -= 12;
                }
                if ((x == 0 || x == 8) && phase > 160 && !useful) {
                    score -= 12; // 边线炮没有实质压制时应降权
                }

                // 炮在河岸区域（advance 4-6）但没有有效炮架 => 位置较差
                if (phase > 150 && adv >= 4 && adv <= 6 && x != 4) {
                    bool hasMount = false;
                    for (int md = 0; md < 4 && !hasMount; ++md) {
                        int mx = x + dx_strai[md];
                        int my = y + dy_strai[md];
                        bool foundScreen = false;
                        while (ChessBoard::inBoard(mx, my)) {
                            const Grid mg = board.getGridAt(mx, my);
                            if (mg.color != EMPTY) {
                                if (!foundScreen) {
                                    foundScreen = true;
                                } else {
                                    if (mg.color == opp &&
                                        (mg.type == Rook || mg.type == King || mg.type == Knight)) {
                                        hasMount = true;
                                    }
                                    break;
                                }
                            }
                            mx += dx_strai[md];
                            my += dy_strai[md];
                        }
                    }
                    if (!hasMount) {
                        score -= 16; // 河岸炮无有效威胁，位置差
                    }
                }

                const int flank = flankIndexFromFile(x);
                if (phase > 150 && flank >= 0) {
                    const int bishopReply = bishopDevelopmentReplyPotential(board, opp, flank);
                    if (cannonPressesFlankKnight(board, side, x, y, flank)) {
                        score += 10;
                        if (bishopReply >= 6) {
                            score -= 20; // 炮压马但可被飞象舒服化解
                        }
                    }
                    if (cannonOnlyHarassesFlankPawn(board, side, x, y, flank) && !useful) {
                        score -= 18;
                        if (bishopReply >= 4) {
                            score -= 14;
                        }
                    }
                }

                if (useful) {
                    score += attackTowardsWeakerFlankBonus(board, side, x, y, Cannon);
                }
                if (root.hasRoot && useful) {
                    score += 8;
                } else if (root.virtualRoot) {
                    score -= 8;
                } else if (root.inDanger && (!useful || adv >= 4)) {
                    score -= 16;
                }

                // 沉底炮
                if (adv >= 8) {
                    if (bottomCount < 4) {
                        bottomPieces[bottomCount++] = {x, y, Cannon};
                    }
                }

                for (int d = 0; d < 4; ++d) {
                    int tx = x + dx_strai[d];
                    int ty = y + dy_strai[d];
                    bool foundMount = false;
                    while (ChessBoard::inBoard(tx, ty)) {
                        const Grid tg = board.getGridAt(tx, ty);
                        if (!foundMount) {
                            if (tg.color != EMPTY) {
                                foundMount = true;
                            }
                        } else if (tg.color != EMPTY) {
                            if (tg.color == opp && (tg.type == Rook || tg.type == King)) {
                                score += 10;
                            }
                            break;
                        }
                        tx += dx_strai[d];
                        ty += dy_strai[d];
                    }
                }
            }
        }
    }

    // --- 沉底抽将识别 ---
    // 车/炮在对方底线附近，如果能将军且同一线路上有对方高价值子可抽 => 额外加分
    if (oppKingX >= 0) {
        for (int bi = 0; bi < bottomCount; ++bi) {
            const int bx = bottomPieces[bi].x;
            const int by = bottomPieces[bi].y;
            const stoneType btype = bottomPieces[bi].type;

            if (btype == Rook) {
                // 车沉底：检查是否在帅的行/列可以将军，并且同行/列有高价值子
                bool canCheckOnRank = (by == oppKingY && countPiecesBetween(board, bx, by, oppKingX, oppKingY) == 0);
                bool canCheckOnFile = (bx == oppKingX && countPiecesBetween(board, bx, by, oppKingX, oppKingY) == 0);

                if (canCheckOnRank) {
                    // 车在帅同行，将军后看同列是否有对方大子可抽
                    for (int fy = 0; fy < BOARDHEIGHT; ++fy) {
                        if (fy == by) continue;
                        const Grid tg = board.getGridAt(bx, fy);
                        if (tg.color == opp && (tg.type == Rook || tg.type == Cannon || tg.type == Knight)) {
                            score += 16; // 抽将潜力
                        }
                    }
                }
                if (canCheckOnFile) {
                    // 车在帅同列，将军后看同行是否有对方大子可抽
                    for (int fx = 0; fx < BOARDWIDTH; ++fx) {
                        if (fx == bx) continue;
                        const Grid tg = board.getGridAt(fx, by);
                        if (tg.color == opp && (tg.type == Rook || tg.type == Cannon || tg.type == Knight)) {
                            score += 16;
                        }
                    }
                }
            }

            if (btype == Cannon) {
                // 炮沉底：如果炮与帅在同一列且有炮架 => 将军牵制
                if (bx == oppKingX) {
                    int between = countPiecesBetween(board, bx, by, oppKingX, oppKingY);
                    if (between == 1) {
                        score += 12; // 炮架已形成，将军威胁
                        // 检查同行是否有对方大子，形成牵制/串打
                        for (int fx = 0; fx < BOARDWIDTH; ++fx) {
                            if (fx == bx) continue;
                            const Grid tg = board.getGridAt(fx, by);
                            if (tg.color == opp && (tg.type == Rook || tg.type == Knight)) {
                                score += 10;
                            }
                        }
                    }
                }
                // 炮与帅同行
                if (by == oppKingY) {
                    int between = countPiecesBetween(board, bx, by, oppKingX, oppKingY);
                    if (between == 1) {
                        score += 12;
                        for (int fy = 0; fy < BOARDHEIGHT; ++fy) {
                            if (fy == by) continue;
                            const Grid tg = board.getGridAt(bx, fy);
                            if (tg.color == opp && (tg.type == Rook || tg.type == Knight)) {
                                score += 10;
                            }
                        }
                    }
                }
            }
        }
    }

    // 双车联通
    if (rookCount == 2) {
        bool connected = false;
        if (rookX[0] == rookX[1]) {
            connected = true;
            const int lo = std::min(rookY[0], rookY[1]);
            const int hi = std::max(rookY[0], rookY[1]);
            for (int y = lo + 1; y < hi; ++y) {
                if (board.getGridAt(rookX[0], y).color != EMPTY) {
                    connected = false;
                    break;
                }
            }
        } else if (rookY[0] == rookY[1]) {
            connected = true;
            const int lo = std::min(rookX[0], rookX[1]);
            const int hi = std::max(rookX[0], rookX[1]);
            for (int x = lo + 1; x < hi; ++x) {
                if (board.getGridAt(x, rookY[0]).color != EMPTY) {
                    connected = false;
                    break;
                }
            }
        }
        if (connected) {
            score += 12;
        }
    }

    return score;
}

// =====================================================================
// 总评估
// =====================================================================
int AIPlayer::evaluate(const ChessBoard& board) const {
    const colorType side = board.currentColor();
    const colorType opp = ChessBoard::oppColor(side);
    const int phase = endgamePhase(board);

    return evalMaterial(board, side, phase)
         + evalPosition(board, side)
         + (evalKingSafety(board, side) - evalKingSafety(board, opp))
         + evalMobility(board, side, phase)
         + (evalPawnStructure(board, side) - evalPawnStructure(board, opp))
         + evalPawnAdvancement(board, side, phase)
         + (evalAttackPressure(board, side) - evalAttackPressure(board, opp))
         + (evalDevelopment(board, side, phase) - evalDevelopment(board, opp, phase))
         + (evalPieceActivity(board, side, phase) - evalPieceActivity(board, opp, phase));
}

// =====================================================================
// TT 操作
// =====================================================================
int AIPlayer::scoreToTT(int score, int ply) const {
    if (score > MATE_SCORE - MAX_DEPTH) {
        return score + ply;
    }
    if (score < -MATE_SCORE + MAX_DEPTH) {
        return score - ply;
    }
    return score;
}

int AIPlayer::scoreFromTT(int score, int ply) const {
    if (score > MATE_SCORE - MAX_DEPTH) {
        return score - ply;
    }
    if (score < -MATE_SCORE + MAX_DEPTH) {
        return score + ply;
    }
    return score;
}

void AIPlayer::ttStore(uint64_t key, int depth, int score, TTFlag flag,
                       const Move& bestMove, int ply) const {
    const int idx = static_cast<int>(key & TT_MASK);
    TTEntry& entry = tt_[idx];
    if (entry.key != key || depth >= entry.depth || entry.age != searchAge_) {
        entry.key = key;
        entry.depth = static_cast<int16_t>(depth);
        entry.score = scoreToTT(score, ply);
        entry.bestMove = bestMove;
        entry.flag = flag;
        entry.age = searchAge_;
    }
}

bool AIPlayer::ttProbe(uint64_t key, int depth, int alpha, int beta,
                       int& score, Move& ttMove, int ply) const {
    const int idx = static_cast<int>(key & TT_MASK);
    const TTEntry& entry = tt_[idx];
    if (entry.key != key) {
        return false;
    }

    ttMove = entry.bestMove;
    if (entry.depth < depth) {
        return false;
    }

    score = scoreFromTT(entry.score, ply);
    switch (entry.flag) {
        case TT_EXACT:      return true;
        case TT_LOWERBOUND: return score >= beta;
        case TT_UPPERBOUND: return score <= alpha;
    }
    return false;
}

// =====================================================================
// Killer / History / Counter-move
// =====================================================================
void AIPlayer::updateKiller(int ply, const Move& mv) const {
    if (!(killers_[ply][0] == mv)) {
        killers_[ply][1] = killers_[ply][0];
        killers_[ply][0] = mv;
    }
}

void AIPlayer::updateHistory(colorType side, const Move& mv, int depth) const {
    const int colorIdx = static_cast<int>(side);
    const int from = mv.source_x * BOARDHEIGHT + mv.source_y;
    const int to = mv.target_x * BOARDHEIGHT + mv.target_y;
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
    if (prev.isInvalid()) {
        return;
    }
    const int colorIdx = static_cast<int>(side);
    const int from = prev.source_x * BOARDHEIGHT + prev.source_y;
    const int to = prev.target_x * BOARDHEIGHT + prev.target_y;
    counterMoves_[colorIdx][from][to] = cur;
}

bool AIPlayer::isKiller(int ply, const Move& mv) const {
    return killers_[ply][0] == mv || killers_[ply][1] == mv;
}

bool AIPlayer::isCounterMove(colorType side, const Move& prev, const Move& mv) const {
    if (prev.isInvalid()) {
        return false;
    }
    const int colorIdx = static_cast<int>(side);
    const int from = prev.source_x * BOARDHEIGHT + prev.source_y;
    const int to = prev.target_x * BOARDHEIGHT + prev.target_y;
    return counterMoves_[colorIdx][from][to] == mv;
}

// =====================================================================
// 走法排序（含炮乱动惩罚、六大子发展奖励、将军阶段化、抽将奖励）
// =====================================================================
int AIPlayer::scoreMoveForOrdering(const ChessBoard& board, const Move& move,
                                   int ply, const Move& ttMove,
                                   int oppKingX, int oppKingY,
                                   int phase, bool sideInCheck) const {
    if (move == ttMove) {
        return 2000000000;
    }

    const Grid source = board.getGridAt(move.source_x, move.source_y);
    const Grid target = board.getGridAt(move.target_x, move.target_y);
    int score = 0;

    // 吃子步：MVV-LVA
    if (target.color != EMPTY && target.color != source.color) {
        int captureScore = 1000000 + pieceBaseValue(target.type) * 10 - pieceBaseValue(source.type);

        // "帮对方活子"惩罚：吃对方还在初始位受限的低价值子，赶走后反而帮对方
        if (phase > 160) {
            // 吃对方初始位置的马 => 帮对方出马
            if (target.type == Knight && !isDevelopedKnight(target.color, move.target_x, move.target_y)) {
                captureScore -= 200;
            }
        }

        return captureScore;
    }

    // 将军方向加分（与局面危险程度挂钩）
    // 统计对方在我方半场的深入大子数，作为局面紧张度参考
    const colorType mySide = source.color;
    const int enemyDeep = countEnemyDeepMajors(board, mySide);
    // 当对方深入大子 >= 2 或者 phase < 140（中局偏后）时，将军权重更高
    const bool checkingPhase = (enemyDeep >= 2 || phase < 140);

    if (oppKingX >= 0) {
        const int tx = move.target_x;
        const int ty = move.target_y;

        // 将军方向奖励与局面紧张度关联：越紧张奖励越高
        int checkBonus;
        if (checkingPhase) {
            checkBonus = 300000;
        } else if (phase > 200 && enemyDeep == 0) {
            checkBonus = 50000; // 开局且无敌深入 => 将军优先级很低
        } else {
            checkBonus = 120000;
        }

        if (source.type == Rook && (tx == oppKingX || ty == oppKingY)) {
            score += checkBonus;

            // 抽将识别：车到帅的行/列，且同线上有对方大子
            if (tx == oppKingX && countPiecesBetween(board, tx, ty, oppKingX, oppKingY) == 0) {
                // 车同列将军 => 看同行是否有对方大子可抽
                for (int fx = 0; fx < BOARDWIDTH; ++fx) {
                    if (fx == tx) continue;
                    const Grid tg = board.getGridAt(fx, ty);
                    if (tg.color == ChessBoard::oppColor(mySide) &&
                        (tg.type == Rook || tg.type == Cannon || tg.type == Knight)) {
                        score += 80000; // 抽将潜力大加分
                        break;
                    }
                }
            }
            if (ty == oppKingY && countPiecesBetween(board, tx, ty, oppKingX, oppKingY) == 0) {
                for (int fy = 0; fy < BOARDHEIGHT; ++fy) {
                    if (fy == ty) continue;
                    const Grid tg = board.getGridAt(tx, fy);
                    if (tg.color == ChessBoard::oppColor(mySide) &&
                        (tg.type == Rook || tg.type == Cannon || tg.type == Knight)) {
                        score += 80000;
                        break;
                    }
                }
            }
        } else if (source.type == Cannon && (tx == oppKingX || ty == oppKingY)) {
            score += (checkBonus * 9 / 10); // 炮将军略低于车

            // 炮将军抽将
            if (tx == oppKingX) {
                int between = countPiecesBetween(board, tx, ty, oppKingX, oppKingY);
                if (between == 1) {
                    // 有炮架 => 真将军
                    for (int fx = 0; fx < BOARDWIDTH; ++fx) {
                        if (fx == tx) continue;
                        const Grid tg = board.getGridAt(fx, ty);
                        if (tg.color == ChessBoard::oppColor(mySide) &&
                            (tg.type == Rook || tg.type == Cannon || tg.type == Knight)) {
                            score += 60000;
                            break;
                        }
                    }
                }
            }
            if (ty == oppKingY) {
                int between = countPiecesBetween(board, tx, ty, oppKingX, oppKingY);
                if (between == 1) {
                    for (int fy = 0; fy < BOARDHEIGHT; ++fy) {
                        if (fy == ty) continue;
                        const Grid tg = board.getGridAt(tx, fy);
                        if (tg.color == ChessBoard::oppColor(mySide) &&
                            (tg.type == Rook || tg.type == Cannon || tg.type == Knight)) {
                            score += 60000;
                            break;
                        }
                    }
                }
            }
        } else if (source.type == Knight) {
            const int dx = std::abs(tx - oppKingX);
            const int dy = std::abs(ty - oppKingY);
            if ((dx == 2 && dy == 1) || (dx == 1 && dy == 2)) {
                score += (checkBonus * 95 / 100);
            }
        } else if (source.type == Pawn) {
            if (std::abs(tx - oppKingX) + std::abs(ty - oppKingY) == 1) {
                score += (checkingPhase ? 200000 : 80000);
            }
        }
    }

    // 被将军时的应对
    if (sideInCheck) {
        if (source.type == King) {
            score += 80000;
        } else if (source.type == Assistant || source.type == Bishop) {
            score += 35000;
        }
    }

    // Killer / Counter-move
    if (isKiller(ply, move)) {
        score += 150000;
    }
    if (ply > 0 && isCounterMove(source.color, prevMove_[ply - 1], move)) {
        score += 120000;
    }

    // History
    const int colorIdx = static_cast<int>(source.color);
    const int from = move.source_x * BOARDHEIGHT + move.source_y;
    const int to = move.target_x * BOARDHEIGHT + move.target_y;
    score += history_[colorIdx][from][to] / 100;

    // PST 变化
    const int pstDiff = positionValue(source.type, source.color, move.target_x, move.target_y)
                      - positionValue(source.type, source.color, move.source_x, move.source_y);
    score += (source.type == Rook || source.type == Cannon || source.type == Knight) ? pstDiff * 2 : pstDiff;

    if (source.type == Rook || source.type == Cannon || source.type == Knight || source.type == Pawn) {
        const bool sourceUnderAttack = const_cast<ChessBoard&>(board).attacked(
            ChessBoard::oppColor(source.color), move.source_x, move.source_y);
        if (sourceUnderAttack || target.color != EMPTY || phase > 180) {
            const TacticalRules::MoveTacticalInfo tactical = TacticalRules::analyzeMove(
                const_cast<ChessBoard&>(board), move);
            score += tactical.activityDelta * 8;
            score += tactical.destinationExchangeScore;
            score += (tactical.destinationWeightedNet - tactical.sourceWeightedNet) / 2;
            if (tactical.improvesByRunning) {
                score += 220; // 被攻击时，跑到更好更安全的位置应优先
            }
            if (tactical.canRootSafely) {
                score += 90; // 生根后交换不亏，排序应提前
            }
            if (tactical.prefersTrade) {
                score += 120; // 车炮对棋若有利，应敢于简化
            }
            if (sourceUnderAttack && !tactical.destinationSafer) {
                score -= 180; // 被打却只是机械挪一下，降权
            }
            if (target.color != EMPTY && (source.type == Rook || source.type == Cannon) && tactical.tradeScore < -30) {
                score -= 140; // 不利的对车对炮不应盲目交换
            }
        }
    }

    if (phase > 140 && (source.type == Rook || source.type == Knight || source.type == Cannon)) {
        score += attackTowardsWeakerFlankBonus(board, source.color, move.target_x, move.target_y, source.type) * 18;
    }

    // 回退步惩罚
    if (ply >= 2 && isBacktrack(move, prevMove_[ply - 2])) {
        if (phase > 160 && (source.type == Cannon || source.type == Knight || source.type == Rook)) {
            score -= 260;
        } else {
            score -= 120;
        }
    }

    // 开局避免同一大子反复承担全部任务
    if (phase > 170 && ply >= 2 &&
        prevMove_[ply - 2].target_x == move.source_x &&
        prevMove_[ply - 2].target_y == move.source_y) {
        if (source.type == Cannon) {
            score -= 240;
        } else if (source.type == Rook) {
            score -= 160;
        } else if (source.type == Knight) {
            score -= 120;
        }
    }

    // 开局阶段特化
    if (phase > 96) {
        if (source.type == Rook) {
            const int adv = advanceOf(source.color, move.target_y);
            if (isNaturalRookDevelopmentMove(source.color, move)) {
                score += 360; // 车一平二、进一步出车、士位出车优先
            }
            if (adv >= 2) {
                score += 280;
            }
            if (move.target_x == 3 || move.target_x == 5) {
                score += 220;
            }
            if (move.target_y == homeRank(source.color) && (move.target_x == 1 || move.target_x == 7)) {
                score += 120;
            }
            const int riverRank = (source.color == RED) ? 4 : 5;
            if (move.target_y == riverRank || move.target_y == riverRank + ((source.color == RED) ? 1 : -1)) {
                score += 160;
            }
            const int fileQuality = rookFileQuality(board, source.color, move.target_x);
            if (fileQuality == 2) {
                score += 80;
            } else if (fileQuality == 1) {
                score += 40;
            }
            if (phase > 170 && isMeaninglessEarlyRookShift(source.color, move)) {
                score -= 260;
            }
            // 开局车出动优先级应比炮高
            if (phase > 170 && !isDevelopedRook(source.color, move.source_x, move.source_y)) {
                score += 300; // 还没出动的车，出子步加分
            }
        }
        if (source.type == Knight) {
            const int adv = advanceOf(source.color, move.target_y);
            const bool toNormal = isNormalKnightSquare(source.color, move.target_x, move.target_y);
            if (adv >= 3) {
                score += 180;
            }
            // 马离开初始位 => 出子奖励
            if (phase > 170 && !isDevelopedKnight(source.color, move.source_x, move.source_y)
                && isDevelopedKnight(source.color, move.target_x, move.target_y)) {
                score += toNormal ? 420 : 120;
            }
            if (toNormal) {
                score += 200;
            } else if (phase > 170 && adv <= 2) {
                score -= 160; // 早期边马、怪马不优先
            }
            const int pawnFile = (move.target_x <= 4) ? 2 : 6;
            if (toNormal && flankPawnAdvanced(board, source.color, pawnFile)) {
                score += 80; // 已有三七兵配合时，正马更值得优先
            }
        }
        if (source.type == Pawn && phase > 160 && (move.source_x == 2 || move.source_x == 6) &&
            move.target_x == move.source_x && move.target_y == move.source_y + forwardDir(source.color)) {
            const int knightX = move.source_x;
            const int knightY = (source.color == RED) ? 2 : 7;
            const Grid wingKnight = board.getGridAt(knightX, knightY);
            if (wingKnight.color == source.color && wingKnight.type == Knight &&
                isNormalKnightSquare(source.color, knightX, knightY)) {
                score += 140; // 三七兵活正马
            } else {
                score -= 40; // 没有实际活马时，不鼓励机械冲兵
            }
        }
        if (source.type == Cannon) {
            const bool useful = isCannonMoveUseful(board, move, source.color, phase);
            if (move.target_x == 4 && phase > 160) {
                score += 260; // 中炮
                if (!isDevelopedCannon(source.color, move.source_x, move.source_y)) {
                    score += 180;
                }
                if (oppKingX == 4) {
                    score += 120;
                }
            }
            if (useful) {
                score += 180;
            }

            // ---- 炮乱动惩罚 ----
            // 开局炮不应该无意义跑到河岸去追打对方车
            if (phase > 160) {
                const int cannonAdv = advanceOf(source.color, move.target_y);
                const int reliefPenalty = cannonNaturalReliefPenalty(const_cast<ChessBoard&>(board), move, source.color);
                score -= reliefPenalty;

                // 炮到河岸（advance 4~6）但没有形成有效战术构型 => 强烈降权
                if (cannonAdv >= 4 && cannonAdv <= 6) {
                    if (!useful) {
                        score -= 600; // 强烈惩罚无意义的河岸炮
                    }
                }

                // 炮横移（停在同一前进度）但不在中路且不在肋道 => 可能无意义
                if (move.source_y == move.target_y && move.target_x != 4
                    && move.target_x != 3 && move.target_x != 5
                    && cannonAdv <= 3) {
                    score -= 220;
                }

                // 炮早期前压到对方半场非底线位置，没有明显战术 => 帮对方活子
                if (cannonAdv >= 5 && cannonAdv <= 7) {
                    if (!useful) {
                        score -= 450;
                    }
                }
            }

            // 炮还在初始位附近但不去中路
            if (advanceOf(source.color, move.target_y) <= 2 && move.target_x != 4) {
                score -= 120;
            }

            // 不在初始位的炮横移不去中路 => 可能无意义
            if (move.source_y != initialCannonRank(source.color) &&
                move.target_x != 4 && move.target_y == move.source_y && phase > 170) {
                score -= 140;
            }
        }
        if (source.type == King && !sideInCheck) {
            const int hY = homeRank(source.color);
            score -= (std::abs(move.target_x - 4) + std::abs(move.target_y - hY)) * 90;
        }
    }

    // 开局炮优于马，残局相反
    if (phase > 180) {
        if (source.type == Cannon) {
            score += 30;
        }
    } else if (phase < 100) {
        if (source.type == Knight) {
            score += 30;
        }
    }

    // 向对方帅靠近
    if (oppKingX >= 0) {
        const int oldDist = std::abs(move.source_x - oppKingX) + std::abs(move.source_y - oppKingY);
        const int newDist = std::abs(move.target_x - oppKingX) + std::abs(move.target_y - oppKingY);
        if (newDist < oldDist) {
            int bonus = (oldDist - newDist) * 15;
            // 开局阶段炮不应盲目前冲靠近对方帅
            if (source.type == Cannon && phase > 160) {
                bonus = bonus / 3;
            }
            score += bonus;
        }
    }

    return score;
}

void AIPlayer::orderMoves(const ChessBoard& board, std::vector<Move>& moves,
                          int ply, const Move& ttMove) const {
    const colorType opp = board.oppColor();
    int oppKingX = -1;
    int oppKingY = -1;
    board.findKing(opp, oppKingX, oppKingY);
    const int phase = endgamePhase(board);
    const bool sideInCheck = board.isInCheck();

    std::vector<std::pair<int, int>> scored(moves.size());
    for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
        scored[i] = std::make_pair(
            scoreMoveForOrdering(board, moves[i], ply, ttMove, oppKingX, oppKingY, phase, sideInCheck),
            i);
    }
    std::sort(scored.begin(), scored.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first > rhs.first;
    });

    std::vector<Move> sorted;
    sorted.reserve(moves.size());
    for (const auto& item : scored) {
        sorted.push_back(moves[item.second]);
    }
    moves.swap(sorted);
}

int AIPlayer::scoreRootMove(const ChessBoard& board, const RootMoveInfo& moveInfo,
                            const Move& pvMove, int currentDanger, int phase) const {
    if (moveInfo.move == pvMove) {
        return 2000000000;
    }

    int score = moveInfo.staticScore;
    if (moveInfo.searchedDepth > 0) {
        score += clampInt(moveInfo.lastScore, -4000, 4000) * 24;
        score += moveInfo.searchedDepth * 200;
    }

    const Grid source = board.getGridAt(moveInfo.move.source_x, moveInfo.move.source_y);
    const TacticalRules::MoveTacticalInfo tactical = TacticalRules::analyzeMove(
        const_cast<ChessBoard&>(board), moveInfo.move);
    score += tactical.activityDelta * 12;
    score += tactical.destinationExchangeScore * 2;
    score += tactical.destinationWeightedNet - tactical.sourceWeightedNet;
    if (tactical.improvesByRunning) {
        score += 240;
    }
    if (tactical.canRootSafely) {
        score += 120;
    }
    if (tactical.prefersTrade) {
        score += 150;
    }

    if (currentDanger >= 80) {
        if (source.type == King) {
            score += 1500;
        } else if (source.type == Assistant || source.type == Bishop) {
            score += 900;
        } else if (source.type == Rook) {
            score += 400;
        }
    } else if (phase > 170 && source.type == King) {
        score -= 800;
    }

    // 开局：惩罚炮的根节点步如果是无意义骚扰
    if (phase > 160 && source.type == Cannon) {
        const int cannonAdv = advanceOf(source.color, moveInfo.move.target_y);
        if (cannonAdv >= 4 && cannonAdv <= 7) {
            if (!isCannonMoveUseful(board, moveInfo.move, source.color, phase)) {
                score -= 5000;
            }
        }
        score -= cannonNaturalReliefPenalty(const_cast<ChessBoard&>(board), moveInfo.move, source.color) * 8;
    }

    if ((source.type == Rook || source.type == Cannon) && tactical.tradeScore < -40) {
        score -= 220;
    }

    return score;
}

void AIPlayer::orderRootMoves(const ChessBoard& board, std::vector<RootMoveInfo>& moves,
                              const Move& pvMove, int currentDanger) const {
    const int phase = endgamePhase(board);
    std::stable_sort(moves.begin(), moves.end(), [&](const RootMoveInfo& lhs, const RootMoveInfo& rhs) {
        return scoreRootMove(board, lhs, pvMove, currentDanger, phase) >
               scoreRootMove(board, rhs, pvMove, currentDanger, phase);
    });
}

// =====================================================================
// 时间控制
// =====================================================================
int AIPlayer::elapsedMs() const {
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - searchStart_).count());
}

void AIPlayer::allocateTimeBudget(const ChessBoard& board, int rootMoveCount) const {
    int hard = HARD_TIME_BASE_MS;
    int soft = SOFT_TIME_BASE_MS;
    const int phase = endgamePhase(board);

    if (board.isInCheck()) {
        hard += 30;
        soft += 50;
    }
    if (rootMoveCount <= 6) {
        hard += 25;
        soft += 40;
    } else if (rootMoveCount >= 28) {
        hard -= 40;
        soft -= 60;
    } else if (rootMoveCount >= 20) {
        hard -= 20;
        soft -= 35;
    }
    if (phase < 90) {
        hard += 20;
        soft += 20;
    }
    if (phase > 210 && rootMoveCount > 22) {
        soft -= 20;
    }

    hardTimeMs_ = clampInt(hard, HARD_TIME_MIN_MS, HARD_TIME_MAX_MS);
    softTimeMs_ = clampInt(soft, SOFT_TIME_MIN_MS, SOFT_TIME_MAX_MS);
    if (softTimeMs_ > hardTimeMs_ - 120) {
        softTimeMs_ = hardTimeMs_ - 120;
    }
    allocatedTimeMs_ = hardTimeMs_;
}

bool AIPlayer::checkTime() const {
    if (timeUp_) {
        return true;
    }
    if ((nodesSearched_ & TIME_CHECK_MASK) != 0) {
        return false;
    }
    if (elapsedMs() >= hardTimeMs_) {
        timeUp_ = true;
        return true;
    }
    return false;
}

bool AIPlayer::softTimeUp() const {
    return elapsedMs() >= softTimeMs_;
}

bool AIPlayer::shouldStopForNextDepth(int depth, int rootMoveCount) const {
    if (timeUp_) {
        return true;
    }

    const int elapsed = elapsedMs();
    if (elapsed >= softTimeMs_) {
        return true;
    }
    if (depth <= 2 || lastIterationMs_ <= 0) {
        return false;
    }

    int projected = lastIterationMs_;
    if (completedDepth_ >= 6) {
        projected = projected * 17 / 10;
    } else if (completedDepth_ >= 4) {
        projected = projected * 3 / 2;
    }
    if (rootMoveCount >= 20) {
        projected = projected * 11 / 10;
    }
    if (bestMoveChanges_ >= 2) {
        projected += 20;
    }

    if (elapsed + projected >= softTimeMs_) {
        return true;
    }
    return elapsed + projected / 2 >= hardTimeMs_ - 10;
}

// =====================================================================
// 初始化搜索状态
// =====================================================================
void AIPlayer::initSearchState() const {
    if (tt_.empty()) {
        tt_.resize(TT_SIZE);
    }
    std::memset(history_, 0, sizeof(history_));
    for (int ply = 0; ply < MAX_DEPTH; ++ply) {
        prevMove_[ply] = Move();
        for (int k = 0; k < MAX_KILLERS; ++k) {
            killers_[ply][k] = Move();
        }
    }
    for (int color = 0; color < 2; ++color) {
        for (int from = 0; from < BOARDWIDTH * BOARDHEIGHT; ++from) {
            for (int to = 0; to < BOARDWIDTH * BOARDHEIGHT; ++to) {
                counterMoves_[color][from][to] = Move();
            }
        }
    }
    timeUp_ = false;
    nodesSearched_ = 0;
    searchAge_ = static_cast<uint8_t>(searchAge_ + 1);
    allocatedTimeMs_ = HARD_TIME_BASE_MS;
    softTimeMs_ = SOFT_TIME_BASE_MS;
    hardTimeMs_ = HARD_TIME_BASE_MS;
    lastIterationMs_ = 0;
    completedDepth_ = 0;
    bestMoveChanges_ = 0;
}

// =====================================================================
// QSearch：将军扩展与局面危险阶段挂钩
// =====================================================================
int AIPlayer::scoreQSearchCheckingMove(const ChessBoard& board, const Move& move,
                                       int qsDepth, int standPat, int alpha,
                                       int oppDangerLevel) const {
    if (qsDepth > 0) {
        return -INF_SCORE;
    }
    if (oppDangerLevel < 30 && standPat + 120 < alpha) {
        return -INF_SCORE;
    }
    if (standPat + 80 < alpha) {
        return -INF_SCORE;
    }

    const Grid source = board.getGridAt(move.source_x, move.source_y);
    if (source.type == King || source.type == Assistant || source.type == Bishop) {
        return -INF_SCORE;
    }
    if (!const_cast<ChessBoard&>(board).isOppKingAttackedAfterMove(move)) {
        return -INF_SCORE;
    }

    int oppKingX = -1, oppKingY = -1;
    board.findKing(board.oppColor(), oppKingX, oppKingY);

    int sc = 2500 + pieceBaseValue(source.type);

    if (oppDangerLevel >= 80) sc += 700;
    else if (oppDangerLevel >= 60) sc += 450;
    else if (oppDangerLevel >= 30) sc += 180;

    if (oppKingX >= 0) {
        const int dist = std::abs(move.target_x - oppKingX) + std::abs(move.target_y - oppKingY);
        if (dist <= 3) {
            sc += (4 - dist) * 140;
        }

        if (source.type == Rook) {
            if (move.target_x == oppKingX &&
                countPiecesBetween(board, move.target_x, move.target_y, oppKingX, oppKingY) == 0) {
                sc += 300;
                for (int fx = 0; fx < BOARDWIDTH; ++fx) {
                    if (fx == move.target_x) continue;
                    const Grid tg = board.getGridAt(fx, move.target_y);
                    if (tg.color == board.oppColor() &&
                        (tg.type == Rook || tg.type == Cannon || tg.type == Knight)) {
                        sc += 700;
                        break;
                    }
                }
            }
            if (move.target_y == oppKingY &&
                countPiecesBetween(board, move.target_x, move.target_y, oppKingX, oppKingY) == 0) {
                sc += 300;
            }
        }

        if (source.type == Cannon) {
            if (move.target_x == oppKingX) {
                int between = countPiecesBetween(board, move.target_x, move.target_y, oppKingX, oppKingY);
                if (between == 1) sc += 650;
                else if (between == 0) sc += 420;
            }
            if (move.target_y == oppKingY) {
                int between = countPiecesBetween(board, move.target_x, move.target_y, oppKingX, oppKingY);
                if (between == 1) sc += 550;
            }
        }

        if (source.type == Knight) {
            const int dx = std::abs(move.target_x - oppKingX);
            const int dy = std::abs(move.target_y - oppKingY);
            if ((dx == 2 && dy == 1) || (dx == 1 && dy == 2)) {
                sc += 420;
            }
        }
    }

    const int pstDiff = positionValue(source.type, source.color, move.target_x, move.target_y)
                      - positionValue(source.type, source.color, move.source_x, move.source_y);
    sc += pstDiff * 3;
    return sc;
}

int AIPlayer::quiescence(ChessBoard& board, int alpha, int beta,
                         int ply, int qsDepth) const {
    ++nodesSearched_;
    if (checkTime()) {
        return 0;
    }

    const int standPat = evaluate(board);
    if (standPat >= beta) {
        return beta;
    }
    if (qsDepth >= MAX_QS_DEPTH) {
        return std::max(alpha, standPat);
    }
    if (standPat > alpha) {
        alpha = standPat;
    }

    const bool inCheck = board.isInCheck();
    std::vector<Move> moves;
    if (inCheck) {
        board.generateMoves(moves);
    } else {
        board.generateCaptures(moves);

        // 只在 qsDepth==0 时考虑非吃子将军步，且受局面危险控制
                if (qsDepth == 0 && standPat + 96 >= alpha) {
            const KingDangerInfo oppDanger = analyzeKingDanger(board, board.oppColor());
            const int oppDangerLevel = oppDanger.totalDanger;
            const int enemyDeep = countEnemyDeepMajors(board, board.currentColor());

            int maxChecks = MAX_QS_CHECKS;
            if (oppDangerLevel < 20 && enemyDeep == 0) {
                maxChecks = 1;
            } else if (oppDangerLevel < 35 && enemyDeep < 2) {
                maxChecks = 2;
            } else if (oppDangerLevel < 60) {
                maxChecks = 4;
            } else {
                maxChecks = 6;
            }

            std::vector<Move> allMoves;
            board.generateMoves(allMoves);
            std::vector<std::pair<int, Move>> checkingMoves;
            checkingMoves.reserve(allMoves.size());

            for (const auto& mv : allMoves) {
                if (board.isCapture(mv)) continue;
                const int sc = scoreQSearchCheckingMove(board, mv, qsDepth, standPat, alpha, oppDangerLevel);
                if (sc > 0) {
                    checkingMoves.push_back(std::make_pair(sc, mv));
                }
            }

            std::sort(checkingMoves.begin(), checkingMoves.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.first > rhs.first;
            });

            int added = 0;
            for (const auto& item : checkingMoves) {
                moves.push_back(item.second);
                if (++added >= maxChecks) break;
            }
        }
    }

    // QSearch 排序
    std::sort(moves.begin(), moves.end(), [&](const Move& lhs, const Move& rhs) {
        int leftScore = 0;
        int rightScore = 0;

        const Grid leftTarget = board.getGridAt(lhs.target_x, lhs.target_y);
        const Grid leftSource = board.getGridAt(lhs.source_x, lhs.source_y);
        if (leftTarget.color != EMPTY) {
            leftScore = 2000 + pieceBaseValue(leftTarget.type) * 10 - pieceBaseValue(leftSource.type);
        } else {
            leftScore = 1000; // 将军步已排序后加入
        }

        const Grid rightTarget = board.getGridAt(rhs.target_x, rhs.target_y);
        const Grid rightSource = board.getGridAt(rhs.source_x, rhs.source_y);
        if (rightTarget.color != EMPTY) {
            rightScore = 2000 + pieceBaseValue(rightTarget.type) * 10 - pieceBaseValue(rightSource.type);
        } else {
            rightScore = 1000;
        }

        return leftScore > rightScore;
    });

    for (const auto& mv : moves) {
        const Grid captured = board.getGridAt(mv.target_x, mv.target_y);
        if (!inCheck && captured.color != EMPTY && standPat + pieceBaseValue(captured.type) + 180 < alpha) {
            continue;
        }

        board.makeMoveAssumeLegal(mv);
        const int sc = -quiescence(board, -beta, -alpha, ply + 1, qsDepth + 1);
        board.undoMove();

        if (timeUp_) {
            return 0;
        }
        if (sc >= beta) {
            return beta;
        }
        if (sc > alpha) {
            alpha = sc;
        }
    }

    return alpha;
}

// =====================================================================
// Alpha-Beta / PVS / TT / Null Move / LMR
// =====================================================================
int AIPlayer::alphaBeta(ChessBoard& board, int depth, int alpha, int beta,
                        int ply, bool allowNull, bool cutNode) const {
    ++nodesSearched_;
    if (checkTime()) {
        return 0;
    }
    if (ply >= MAX_DEPTH - 1) {
        return evaluate(board);
    }
    if (board.exceedMaxPeaceState()) {
        return DRAW_SCORE;
    }

    const bool pvNode = (beta - alpha > 1);
    const bool inCheck = board.isInCheck();
    if (inCheck) {
        ++depth;
    }
    if (depth <= 0) {
        return quiescence(board, alpha, beta, ply, 0);
    }

    const int originalAlpha = alpha;
    const uint64_t key = board.computeHash();
    int ttScore = 0;
    Move ttMove;
    if (ttProbe(key, depth, alpha, beta, ttScore, ttMove, ply)) {
        if (!pvNode) {
            return ttScore;
        }
    }

    if (pvNode && ttMove.isInvalid() && depth >= 5) {
        alphaBeta(board, depth - 2, alpha, beta, ply, true, false);
        ttProbe(key, 0, -INF_SCORE, INF_SCORE, ttScore, ttMove, ply);
    }

    if (!pvNode && !inCheck && depth <= 2 && ttMove.isInvalid()) {
        const int eval = evaluate(board);
        const int margin = RAZOR_MARGIN + (depth - 1) * 120;
        if (eval + margin <= alpha) {
            const int qScore = quiescence(board, alpha, beta, ply, 0);
            if (qScore <= alpha) {
                return qScore;
            }
        }
    }

    const colorType side = board.currentColor();
const KingDangerInfo selfDanger = analyzeKingDanger(board, side);
const bool nullUnsafe =
    isLikelyZugzwangLike(board, side) ||
    selfDanger.totalDanger >= 70 ||
    selfDanger.directRookPressure > 0 ||
    selfDanger.directCannonPressure > 0 ||
    selfDanger.bottomCannonThreat > 0 ||
    selfDanger.rookCannonThreat > 0 ||
    selfDanger.wocaoThreat > 0 ||
    countEnemyDeepMajors(board, side) >= 2;

if (allowNull && !pvNode && !inCheck &&
    depth >= NULL_MOVE_MIN_DEPTH &&
    board.countMajorPieces(board.currentColor()) >= 4 &&
    !nullUnsafe) {
    int reduction = 2 + (depth >= 6 ? 1 : 0) + (depth >= 10 ? 1 : 0);

    // 局面危险时宁可少减一点，降低误剪风险
    if (selfDanger.totalDanger >= 45) {
        reduction = std::max(1, reduction - 1);
    }

    if (reduction > depth - 1) {
        reduction = depth - 1;
    }

    board.makeNullMove();
    const int nullScore = -alphaBeta(board, depth - 1 - reduction, -beta, -beta + 1,
                                     ply + 1, false, !cutNode);
    board.undoNullMove();

    if (timeUp_) {
        return 0;
    }

    if (nullScore >= beta) {
        // 深层时保留验证搜索，但在危险局面下更谨慎
        if (depth >= 8 || selfDanger.totalDanger >= 45) {
            const int verify = alphaBeta(board, depth - 1 - reduction, alpha, beta, ply, false, false);
            if (verify >= beta) {
                return beta;
            }
        } else {
            return beta;
        }
    }
}

    std::vector<Move> moves;
    board.generateMoves(moves);
    if (moves.empty()) {
        return -MATE_SCORE + ply;
    }
    orderMoves(board, moves, ply, ttMove);

    bool canFutility = !pvNode && !inCheck && depth <= 2;
    int futilityBase = 0;
    if (canFutility) {
        futilityBase = evaluate(board);
        const int margins[3] = {0, FUTILITY_MARGIN_1, FUTILITY_MARGIN_2};
        if (futilityBase + margins[depth] > alpha) {
            canFutility = false;
        }
    }

    Move bestMove;
    int bestScore = -INF_SCORE;
    int movesSearched = 0;

    for (const auto& mv : moves) {
        if (checkTime()) {
            break;
        }

        const bool isCapture = board.isCapture(mv);
        const Grid source = board.getGridAt(mv.source_x, mv.source_y);
        if (canFutility && movesSearched > 2 && !isCapture &&
            source.type != King && source.type != Assistant && source.type != Bishop &&
            !isKiller(ply, mv)) {
            continue;
        }

        board.makeMoveAssumeLegal(mv);
        prevMove_[ply] = mv;
        const bool givesCheck = board.isInCheck();
        int sc = 0;

        if (movesSearched == 0) {
            sc = -alphaBeta(board, depth - 1, -beta, -alpha, ply + 1, true, false);
        } else {
            int newDepth = depth - 1;
                        const bool palacePressure = isPalacePressureMove(board, mv, source.color);
            const bool tacticalEscape =
                (source.type == Rook || source.type == Cannon || source.type == Knight) &&
                const_cast<ChessBoard&>(board).attacked(ChessBoard::oppColor(source.color),
                                                        mv.source_x, mv.source_y);
            const int phase = endgamePhase(board);
            if (!inCheck && !isCapture && !givesCheck && !palacePressure && !tacticalEscape &&
                depth >= LMR_MIN_DEPTH && movesSearched >= LMR_MIN_MOVES) {
                int reduction = lmrTable[std::min(depth, 63)][std::min(movesSearched, 63)];
                if (cutNode) {
                    ++reduction;
                }
                if (pvNode) {
                    reduction = std::max(0, reduction - 1);
                }
                if (source.type == King || source.type == Assistant || source.type == Bishop) {
                    reduction = std::max(0, reduction - 1);
                }
                if (phase < 110) {
                    reduction = std::max(0, reduction - 1);
                }
                newDepth = std::max(1, depth - 1 - reduction);
            }
            sc = -alphaBeta(board, newDepth, -(alpha + 1), -alpha, ply + 1, true, !cutNode);
            if (sc > alpha && (newDepth < depth - 1 || !pvNode)) {
                sc = -alphaBeta(board, depth - 1, -beta, -alpha, ply + 1, true, false);
            }
        }

        board.undoMove();
        ++movesSearched;

        if (timeUp_) {
            break;
        }
        if (sc > bestScore) {
            bestScore = sc;
            bestMove = mv;
        }
        if (sc > alpha) {
            alpha = sc;
        }
        if (alpha >= beta) {
            if (!isCapture) {
                updateKiller(ply, mv);
                updateHistory(board.currentColor(), mv, depth);
                if (ply > 0) {
                    updateCounterMove(board.currentColor(), prevMove_[ply - 1], mv);
                }
            }
            break;
        }
    }

    if (movesSearched == 0) {
        return originalAlpha;
    }

    if (!timeUp_ && !bestMove.isInvalid()) {
        TTFlag flag = TT_UPPERBOUND;
        if (bestScore >= beta) {
            flag = TT_LOWERBOUND;
        } else if (bestScore > originalAlpha) {
            flag = TT_EXACT;
        }
        ttStore(key, depth, bestScore, flag, bestMove, ply);
    }
    return bestScore;
}

// =====================================================================
// 迭代加深 + 根节点 PVS + 受限 aspiration
// =====================================================================
Move AIPlayer::getBestMove(ChessBoard& board) const {
    initSearchState();
    searchStart_ = std::chrono::steady_clock::now();

    std::vector<Move> rootLegalMoves;
    board.generateMovesWithForbidden(rootLegalMoves);
    const bool allForbidden = rootLegalMoves.empty();
    if (allForbidden) {
        board.generateMoves(rootLegalMoves);
        if (rootLegalMoves.empty()) {
            return Move();
        }
    }
    if (rootLegalMoves.size() == 1) {
        return rootLegalMoves[0];
    }

    const Move openingMove = OpeningBook::chooseOpeningMove(board, rootLegalMoves);
    if (!openingMove.isInvalid()) {
        return openingMove;
    }

    allocateTimeBudget(board, static_cast<int>(rootLegalMoves.size()));

    int oppKingX = -1;
    int oppKingY = -1;
    board.findKing(board.oppColor(), oppKingX, oppKingY);
    const int phase = endgamePhase(board);
    const bool rootInCheck = board.isInCheck();
    const int currentDanger = analyzeKingDanger(board, board.currentColor()).totalDanger;

    std::vector<RootMoveInfo> rootMoves;
    rootMoves.reserve(rootLegalMoves.size());
    for (const auto& mv : rootLegalMoves) {
        RootMoveInfo info;
        info.move = mv;
        info.staticScore = scoreMoveForOrdering(board, mv, 0, Move(), oppKingX, oppKingY, phase, rootInCheck);
        rootMoves.push_back(info);
    }
    orderRootMoves(board, rootMoves, Move(), currentDanger);

    Move bestMove = rootMoves[0].move;
    Move pvMove = bestMove;
    int previousScore = 0;
    int previousElapsed = 0;

    for (int depth = 1; depth <= DEFAULT_DEPTH; ++depth) {
        if (depth > 1 && shouldStopForNextDepth(depth, static_cast<int>(rootMoves.size()))) {
            break;
        }

        orderRootMoves(board, rootMoves, pvMove, currentDanger);

        int aspiration = ASP_WINDOW;
        int windowAlpha = (depth >= 4) ? (previousScore - aspiration) : -INF_SCORE;
        int windowBeta = (depth >= 4) ? (previousScore + aspiration) : INF_SCORE;

        Move depthBest;
        int depthScore = -INF_SCORE;
        bool depthComplete = false;
        bool usedFullWindow = depth < 4;
        int researchCount = 0;

        while (true) {
            if (checkTime()) {
                break;
            }

            int localAlpha = windowAlpha;
            depthBest = Move();
            depthScore = -INF_SCORE;
            int searchedMoves = 0;

            for (auto& root : rootMoves) {
                if (checkTime()) {
                    break;
                }

                board.makeMoveAssumeLegal(root.move);
                int sc = 0;
                if (searchedMoves == 0) {
                    sc = -alphaBeta(board, depth - 1, -windowBeta, -localAlpha, 1, true, false);
                } else {
                    sc = -alphaBeta(board, depth - 1, -(localAlpha + 1), -localAlpha, 1, true, true);
                    if (sc > localAlpha && sc < windowBeta) {
                        sc = -alphaBeta(board, depth - 1, -windowBeta, -localAlpha, 1, true, false);
                    }
                }
                board.undoMove();
                ++searchedMoves;

                if (timeUp_) {
                    break;
                }

                root.lastScore = sc;
                root.searchedDepth = depth;
                if (sc > depthScore) {
                    depthScore = sc;
                    depthBest = root.move;
                }
                if (sc > localAlpha) {
                    localAlpha = sc;
                }
                if (localAlpha >= windowBeta) {
                    break;
                }
            }

            if (timeUp_ || depthBest.isInvalid()) {
                break;
            }
            if (depth < 4) {
                depthComplete = true;
                break;
            }

            if (depthScore <= windowAlpha) {
                ++researchCount;
                if (researchCount >= MAX_ASP_RESEARCH || softTimeUp()) {
                    if (!usedFullWindow) {
                        windowAlpha = -INF_SCORE;
                        windowBeta = INF_SCORE;
                        usedFullWindow = true;
                        researchCount = 0;
                        continue;
                    }
                    break;
                }
                aspiration = std::min(aspiration * 2 + 16, 320);
                windowAlpha = previousScore - aspiration;
                if (aspiration >= 320) {
                    windowAlpha = -INF_SCORE;
                    windowBeta = INF_SCORE;
                    usedFullWindow = true;
                }
                continue;
            }

            if (depthScore >= windowBeta) {
                ++researchCount;
                if (researchCount >= MAX_ASP_RESEARCH || softTimeUp()) {
                    if (!usedFullWindow) {
                        windowAlpha = -INF_SCORE;
                        windowBeta = INF_SCORE;
                        usedFullWindow = true;
                        researchCount = 0;
                        continue;
                    }
                    break;
                }
                aspiration = std::min(aspiration * 2 + 16, 320);
                windowBeta = previousScore + aspiration;
                if (aspiration >= 320) {
                    windowAlpha = -INF_SCORE;
                    windowBeta = INF_SCORE;
                    usedFullWindow = true;
                }
                continue;
            }

            depthComplete = true;
            break;
        }

        if (!depthComplete) {
            break;
        }

        if (!(depthBest == bestMove)) {
            ++bestMoveChanges_;
        }
        bestMove = depthBest;
        pvMove = depthBest;
        previousScore = depthScore;
        completedDepth_ = depth;
        orderRootMoves(board, rootMoves, pvMove, currentDanger);

        const int now = elapsedMs();
        lastIterationMs_ = std::max(1, now - previousElapsed);
        previousElapsed = now;

        if (softTimeUp()) {
            break;
        }
    }

    bool found = false;
    for (const auto& root : rootMoves) {
        if (root.move == bestMove) {
            found = true;
            break;
        }
    }
    if (!found) {
        bestMove = rootMoves[0].move;
    }

    if (!allForbidden && board.repeatAfterMove(bestMove)) {
        for (const auto& root : rootMoves) {
            if (!board.repeatAfterMove(root.move)) {
                bestMove = root.move;
                break;
            }
        }
    }

    return bestMove;
}
