#include "ChessBoard.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <cstring>

static constexpr int EYE_DRAW_VALUE_CONST = 20;
static constexpr int EYE_MATE_SCORE_CONST = 900000;
static constexpr int EYE_BAN_SCORE_CONST  = EYE_MATE_SCORE_CONST - 100;

// ===== Legacy Zobrist 静态成员定义 =====
bool ChessBoard::zobristInited_ = false;
uint64_t ChessBoard::zobristTable_[BOARDWIDTH][BOARDHEIGHT][8][3];
uint64_t ChessBoard::zobristSide_;

// ===== Eye Zobrist 静态成员定义 =====
bool ChessBoard::eyeZobristInited_ = false;
EyeZobrist ChessBoard::eyeZobrTable_[14][256];
EyeZobrist ChessBoard::eyeZobrPlayer_;

static int eyeZobrIndex(int pc) {
    int pt = PIECE_TYPE_EYE(pc);
    if (pc >= 32) pt += 7;
    return pt;
}

static inline void SetPerpCheck(uint32_t &dwPerpCheck, int nChkChs) {
    if (nChkChs == 0) {
        dwPerpCheck = 0;
    } else if (nChkChs > 0) {
        dwPerpCheck &= 0x10000;
    } else {
        dwPerpCheck &= (1u << (-nChkChs));
    }
}

void ChessBoard::initEyeZobrist() {
    if (eyeZobristInited_) return;
    std::mt19937 rng(0xE0E0CAFE);
    auto rand32 = [&]() -> uint32_t { return rng(); };
    for (int pt = 0; pt < 14; ++pt) {
        for (int sq = 0; sq < 256; ++sq) {
            eyeZobrTable_[pt][sq].dwKey   = rand32();
            eyeZobrTable_[pt][sq].dwLock0 = rand32();
            eyeZobrTable_[pt][sq].dwLock1 = rand32();
        }
    }
    eyeZobrPlayer_.dwKey   = rand32();
    eyeZobrPlayer_.dwLock0 = rand32();
    eyeZobrPlayer_.dwLock1 = rand32();
    eyeZobristInited_ = true;
}

void ChessBoard::initZobrist() {
    if (zobristInited_) return;
    std::mt19937_64 rng(0x12345678ABCDEF01ULL);
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            for (int p = 0; p < 8; ++p) {
                for (int c = 0; c < 3; ++c) {
                    zobristTable_[x][y][p][c] = rng();
                }
            }
        }
    }
    zobristSide_ = rng();
    zobristInited_ = true;
}

bool ChessBoard::kingAttacked(colorType side) const {
    int kx = -1, ky = -1;
    if (!findKing(side, kx, ky)) {
        return true;
    }
    return attacked(oppColor(side), kx, ky);
}

uint64_t ChessBoard::fullComputeHash() const {
    uint64_t h = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            if (board_[x][y].color != EMPTY) {
                h ^= zobristTable_[x][y][board_[x][y].type][board_[x][y].color];
            }
        }
    }
    if (currColor_ == BLACK) h ^= zobristSide_;
    return h;
}

ChessBoard::ChessBoard() {
    initZobrist();
    initEyeZobrist();
    resetBoard();
}

// =====================================================================
//  Eye-style private helpers
// =====================================================================

void ChessBoard::eyeAddPiece(int sq, int pc, bool bDel) {
    if (bDel) {
        ucpcSquares_[sq] = 0;
        ucsqPieces_[pc] = 0;
    } else {
        ucpcSquares_[sq] = pc;
        ucsqPieces_[pc] = sq;
    }
    zobr_.xorWith(eyeZobrTable_[eyeZobrIndex(pc)][sq]);
}

int ChessBoard::eyeMovePiece(int eyeMv) {
    const int sqSrc = SRC(eyeMv);
    const int sqDst = DST(eyeMv);
    const int pcMoved = ucpcSquares_[sqSrc];
    const int pcCaptured = ucpcSquares_[sqDst];

    if (pcCaptured != 0) {
        ucsqPieces_[pcCaptured] = 0;
        zobr_.xorWith(eyeZobrTable_[eyeZobrIndex(pcCaptured)][sqDst]);
    }

    ucpcSquares_[sqSrc] = 0;
    ucpcSquares_[sqDst] = pcMoved;
    ucsqPieces_[pcMoved] = sqDst;

    zobr_.xorWith(eyeZobrTable_[eyeZobrIndex(pcMoved)][sqDst],
                  eyeZobrTable_[eyeZobrIndex(pcMoved)][sqSrc]);
    return pcCaptured;
}

void ChessBoard::eyeUndoMovePiece(int eyeMv, int pcCaptured) {
    const int sqSrc = SRC(eyeMv);
    const int sqDst = DST(eyeMv);
    const int pcMoved = ucpcSquares_[sqDst];

    ucpcSquares_[sqSrc] = pcMoved;
    ucsqPieces_[pcMoved] = sqSrc;

    if (pcCaptured != 0) {
        ucpcSquares_[sqDst] = pcCaptured;
        ucsqPieces_[pcCaptured] = sqDst;
    } else {
        ucpcSquares_[sqDst] = 0;
    }
}

void ChessBoard::eyeSaveStatus() {
    rbsList_[nMoveNum_].zobr = zobr_;
}

void ChessBoard::eyeRollback() {
    zobr_ = rbsList_[nMoveNum_].zobr;
}

void ChessBoard::eyeChangeSide() {
    sdPlayer_ = OPP_SIDE(sdPlayer_);
    zobr_.xorWith(eyeZobrPlayer_);
}

void ChessBoard::syncBoardFromEye() {
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            board_[x][y] = Grid();
        }
    }

    for (int pc = 16; pc < 48; ++pc) {
        const int sq = ucsqPieces_[pc];
        if (sq == 0) continue;
        const int x = sq2x(sq);
        const int y = sq2y(sq);
        board_[x][y] = Grid(eyePtToStone(PIECE_TYPE_EYE(pc)), pc < 32 ? RED : BLACK);
    }
}

// =====================================================================
//  Basic board / geometry
// =====================================================================

bool ChessBoard::inBoard(int mx, int my) {
    return mx >= 0 && mx < BOARDWIDTH && my >= 0 && my < BOARDHEIGHT;
}

bool ChessBoard::inKingArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return mx >= 3 && mx <= 5 && my >= 0 && my <= 2;
    } else {
        return mx >= 3 && mx <= 5 && my >= 7 && my <= 9;
    }
}

bool ChessBoard::inColorArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    return mcolor == RED ? (my <= 4) : (my >= 5);
}

colorType ChessBoard::currentColor() const {
    return currColor_;
}

colorType ChessBoard::oppColor() const {
    return currColor_ == RED ? BLACK : RED;
}

colorType ChessBoard::oppColor(colorType mcolor) {
    return mcolor == RED ? BLACK : RED;
}

bool ChessBoard::inSameLine(int sx, int sy, int ex, int ey) {
    const int dx = ex - sx;
    const int dy = ey - sy;
    return dx == 0 || dy == 0 || dx == dy || dx == -dy;
}

bool ChessBoard::inSameStraightLine(int sx, int sy, int ex, int ey) {
    return sx == ex || sy == ey;
}

bool ChessBoard::inSameObiqueLine(int sx, int sy, int ex, int ey) {
    const int dx = ex - sx;
    const int dy = ey - sy;
    return dx == dy || dx == -dy;
}

bool ChessBoard::betweenIsEmpty(int sx, int sy, int ex, int ey) const {
    assert(inSameStraightLine(sx, sy, ex, ey));
    const int dx = (ex > sx) ? 1 : ((ex < sx) ? -1 : 0);
    const int dy = (ey > sy) ? 1 : ((ey < sy) ? -1 : 0);
    int x = sx + dx, y = sy + dy;
    while (x != ex || y != ey) {
        if (board_[x][y].color != EMPTY) return false;
        x += dx;
        y += dy;
    }
    return true;
}

int ChessBoard::betweenNotEmptyNum(int sx, int sy, int ex, int ey) const {
    assert(inSameStraightLine(sx, sy, ex, ey));
    const int dx = (ex > sx) ? 1 : ((ex < sx) ? -1 : 0);
    const int dy = (ey > sy) ? 1 : ((ey < sy) ? -1 : 0);
    int x = sx + dx, y = sy + dy, cnt = 0;
    while (x != ex || y != ey) {
        if (board_[x][y].color != EMPTY) ++cnt;
        x += dx;
        y += dy;
    }
    return cnt;
}

int ChessBoard::xy2pos(int mx, int my) {
    return my * BOARDWIDTH + mx;
}

int ChessBoard::pos2x(int mpos) {
    return mpos % BOARDWIDTH;
}

int ChessBoard::pos2y(int mpos) {
    return mpos / BOARDWIDTH;
}

Grid ChessBoard::getGridAt(int mx, int my) const {
    return board_[mx][my];
}

void ChessBoard::printBoard() const {
    std::cout << "\nBoard\n";
    for (int y = 9; y >= 0; --y) {
        for (int x = 0; x < 9; ++x) {
            std::cout << stoneSym[board_[x][y].type] << " ";
        }
        std::cout << "\n";
    }
}

uint64_t ChessBoard::computeHash() const {
    return hash_;
}

// =====================================================================
//  Reset / init
// =====================================================================

void ChessBoard::resetBoard() {
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            board_[x][y] = Grid();
        }
    }

    board_[0][0] = Grid(Rook, RED);
    board_[1][0] = Grid(Knight, RED);
    board_[2][0] = Grid(Bishop, RED);
    board_[3][0] = Grid(Assistant, RED);
    board_[4][0] = Grid(King, RED);
    board_[5][0] = Grid(Assistant, RED);
    board_[6][0] = Grid(Bishop, RED);
    board_[7][0] = Grid(Knight, RED);
    board_[8][0] = Grid(Rook, RED);
    board_[1][2] = Grid(Cannon, RED);
    board_[7][2] = Grid(Cannon, RED);
    board_[0][3] = Grid(Pawn, RED);
    board_[2][3] = Grid(Pawn, RED);
    board_[4][3] = Grid(Pawn, RED);
    board_[6][3] = Grid(Pawn, RED);
    board_[8][3] = Grid(Pawn, RED);

    board_[0][9] = Grid(Rook, BLACK);
    board_[1][9] = Grid(Knight, BLACK);
    board_[2][9] = Grid(Bishop, BLACK);
    board_[3][9] = Grid(Assistant, BLACK);
    board_[4][9] = Grid(King, BLACK);
    board_[5][9] = Grid(Assistant, BLACK);
    board_[6][9] = Grid(Bishop, BLACK);
    board_[7][9] = Grid(Knight, BLACK);
    board_[8][9] = Grid(Rook, BLACK);
    board_[1][7] = Grid(Cannon, BLACK);
    board_[7][7] = Grid(Cannon, BLACK);
    board_[0][6] = Grid(Pawn, BLACK);
    board_[2][6] = Grid(Pawn, BLACK);
    board_[4][6] = Grid(Pawn, BLACK);
    board_[6][6] = Grid(Pawn, BLACK);
    board_[8][6] = Grid(Pawn, BLACK);

    currColor_ = RED;
    turnId_ = 0;
    peaceTurn_ = 0;

    undoStack_.clear();
    moveHistory_.clear();
    hashHistory_.clear();
    captureHistory_.clear();

    hash_ = fullComputeHash();
    hashHistory_.push_back(hash_);
    captureHistory_.push_back(false);

    std::memset(ucpcSquares_, 0, sizeof(ucpcSquares_));
    std::memset(ucsqPieces_, 0, sizeof(ucsqPieces_));
    std::memset(ucRepHash_, 0, sizeof(ucRepHash_));
    for (int i = 0; i < MAX_MOVE_NUM; ++i) {
        rbsList_[i] = EyeRollback{};
    }

    zobr_ = EyeZobrist();
    sdPlayer_ = 0;
    nDistance_ = 0;
    nMoveNum_ = 1;

    int pcRed[7] = {
        SIDE_TAG(0) + EYE_KING_FROM,
        SIDE_TAG(0) + EYE_ADVISOR_FROM,
        SIDE_TAG(0) + EYE_BISHOP_FROM,
        SIDE_TAG(0) + EYE_KNIGHT_FROM,
        SIDE_TAG(0) + EYE_ROOK_FROM,
        SIDE_TAG(0) + EYE_CANNON_FROM,
        SIDE_TAG(0) + EYE_PAWN_FROM
    };
    int pcBlack[7];
    for (int i = 0; i < 7; ++i) pcBlack[i] = pcRed[i] + 16;

    for (int rank = EYE_RANK_TOP; rank <= EYE_RANK_BOTTOM; ++rank) {
        for (int file = EYE_FILE_LEFT; file <= EYE_FILE_RIGHT; ++file) {
            const int sq = COORD_XY(file, rank);
            const int x = sq2x(sq);
            const int y = sq2y(sq);
            const Grid& g = board_[x][y];
            if (g.color == EMPTY) continue;

            const int pt = stoneToEyePt(g.type);
            if (pt < 0) continue;

            if (g.color == RED) {
                const int pc = pcRed[pt];
                if (pc < SIDE_TAG(0) + 16 && ucsqPieces_[pc] == 0) {
                    eyeAddPiece(sq, pc);
                    ++pcRed[pt];
                }
            } else {
                const int pc = pcBlack[pt];
                if (pc < SIDE_TAG(1) + 16 && ucsqPieces_[pc] == 0) {
                    eyeAddPiece(sq, pc);
                    ++pcBlack[pt];
                }
            }
        }
    }

    eyeSaveStatus();
    rbsList_[0].wmv = 0;
    rbsList_[0].chkChs = 0;
    rbsList_[0].cptDrw = -100;
}

// =====================================================================
//  Attack / check / chase
// =====================================================================

bool ChessBoard::attacked(colorType color, int mx, int my) const {
    const int sd = (color == RED ? 0 : 1);
    const int tag = SIDE_TAG(sd);

    // ---------------------------
    // King
    // ---------------------------
    {
        const int sq = ucsqPieces_[tag + EYE_KING_FROM];
        if (sq != 0) {
            const int x = sq2x(sq);
            const int y = sq2y(sq);

            if (inKingArea(mx, my, color) &&
                std::abs(x - mx) + std::abs(y - my) == 1) {
                return true;
            }

            if (x == mx && board_[mx][my].type == King) {
                const int step = (y < my ? 1 : -1);
                int yy = y + step;
                bool clear = true;
                while (yy != my) {
                    if (board_[x][yy].color != EMPTY) {
                        clear = false;
                        break;
                    }
                    yy += step;
                }
                if (clear) return true;
            }
        }
    }

    // ---------------------------
    // Advisors
    // ---------------------------
    for (int i = EYE_ADVISOR_FROM; i <= EYE_ADVISOR_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        if (inKingArea(mx, my, color) &&
            std::abs(x - mx) == 1 &&
            std::abs(y - my) == 1) {
            return true;
        }
    }

    // ---------------------------
    // Bishops
    // ---------------------------
    for (int i = EYE_BISHOP_FROM; i <= EYE_BISHOP_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        if (!inColorArea(mx, my, color)) continue;
        if (std::abs(x - mx) == 2 && std::abs(y - my) == 2) {
            const int ex = (x + mx) / 2;
            const int ey = (y + my) / 2;
            if (board_[ex][ey].color == EMPTY) {
                return true;
            }
        }
    }

    // ---------------------------
    // Knights
    // ---------------------------
    for (int i = EYE_KNIGHT_FROM; i <= EYE_KNIGHT_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        const int dx = mx - x;
        const int dy = my - y;

        if (std::abs(dx) == 2 && std::abs(dy) == 1) {
            const int fx = x + (dx > 0 ? 1 : -1);
            if (board_[fx][y].color == EMPTY) return true;
        } else if (std::abs(dx) == 1 && std::abs(dy) == 2) {
            const int fy = y + (dy > 0 ? 1 : -1);
            if (board_[x][fy].color == EMPTY) return true;
        }
    }

    // ---------------------------
    // Pawns
    // ---------------------------
    for (int i = EYE_PAWN_FROM; i <= EYE_PAWN_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        const int fwd = (color == RED ? 1 : -1);
        if (x == mx && y + fwd == my) {
            return true;
        }

        const bool crossed = (color == RED ? (y > 4) : (y < 5));
        if (crossed && y == my && std::abs(x - mx) == 1) {
            return true;
        }
    }

    // ---------------------------
    // Rook / Cannon / facing king
    // ---------------------------
    for (int d = 0; d < 4; ++d) {
        int x = mx + dx_strai[d];
        int y = my + dy_strai[d];
        bool firstOccupiedSeen = false;

        while (inBoard(x, y)) {
            const Grid& g = board_[x][y];
            if (g.color != EMPTY) {
                if (!firstOccupiedSeen) {
                    if (g.color == color) {
                        if (g.type == Rook) return true;
                        if (g.type == King &&
                            dx_strai[d] == 0 &&
                            board_[mx][my].type == King) {
                            return true;
                        }
                    }
                    firstOccupiedSeen = true;
                } else {
                    if (g.color == color && g.type == Cannon) return true;
                    break;
                }
            }
            x += dx_strai[d];
            y += dy_strai[d];
        }
    }

    return false;
}

int ChessBoard::checkedBy(bool bLazy) const {
    const int kingPc = SIDE_TAG(sdPlayer_) + EYE_KING_FROM;
    const int kingSq = ucsqPieces_[kingPc];
    if (kingSq == 0 || !IN_BOARD(kingSq)) return 0;

    const int kx = sq2x(kingSq);
    const int ky = sq2y(kingSq);
    const colorType myColor = currColor_;
    const colorType opp = oppColor(myColor);

    int checker = 0;

    auto addChecker = [&](int pc) -> bool {
        if (pc == 0) return false;
        if (bLazy) {
            checker = CHECK_MULTI;
            return true;
        }
        if (checker != 0) {
            checker = CHECK_MULTI;
            return true;
        }
        checker = pc;
        return false;
    };

    auto crossedRiverLocal = [&](colorType c, int y) -> bool {
        return c == RED ? (y >= 5) : (y <= 4);
    };

    {
        const int sy = ky - (opp == RED ? 1 : -1);
        if (inBoard(kx, sy)) {
            const Grid& g = board_[kx][sy];
            if (g.color == opp && g.type == Pawn) {
                if (addChecker(ucpcSquares_[xy2sq(kx, sy)])) return checker;
            }
        }
        for (int d = 0; d < 2; ++d) {
            const int sx = kx + dx_lr[d];
            if (!inBoard(sx, ky)) continue;
            const Grid& g = board_[sx][ky];
            if (g.color == opp && g.type == Pawn && crossedRiverLocal(opp, ky)) {
                if (addChecker(ucpcSquares_[xy2sq(sx, ky)])) return checker;
            }
        }
    }

    for (int d = 0; d < 8; ++d) {
        const int sx = kx - dx_knight[d];
        const int sy = ky - dy_knight[d];
        if (!inBoard(sx, sy)) continue;
        const int fx = sx + dx_knight_foot[d];
        const int fy = sy + dy_knight_foot[d];
        if (!inBoard(fx, fy) || board_[fx][fy].color != EMPTY) continue;
        const Grid& g = board_[sx][sy];
        if (g.color == opp && g.type == Knight) {
            if (addChecker(ucpcSquares_[xy2sq(sx, sy)])) return checker;
        }
    }

    for (int d = 0; d < 4; ++d) {
        int x = kx + dx_strai[d];
        int y = ky + dy_strai[d];
        bool firstOccupiedSeen = false;
        while (inBoard(x, y)) {
            const Grid& g = board_[x][y];
            if (g.color != EMPTY) {
                const int pc = ucpcSquares_[xy2sq(x, y)];
                if (!firstOccupiedSeen) {
                    if (g.color == opp) {
                        if (g.type == Rook) {
                            if (addChecker(pc)) return checker;
                        } else if (g.type == King && dx_strai[d] == 0) {
                            if (addChecker(pc)) return checker;
                        }
                    }
                    firstOccupiedSeen = true;
                } else {
                    if (g.color == opp && g.type == Cannon) {
                        if (addChecker(pc)) return checker;
                    }
                    break;
                }
            }
            x += dx_strai[d];
            y += dy_strai[d];
        }
    }

    return checker;
}

int ChessBoard::chasedBy(int eyeMv) const {
    const int sqSrc = DST(eyeMv);
    const int pcMoved = ucpcSquares_[sqSrc];
    const int nSideTag = SIDE_TAG(sdPlayer_);
    if (!IN_BOARD(sqSrc) || pcMoved == 0) return 0;

    const int movedIndex = pcMoved - OPP_SIDE_TAG(sdPlayer_);
    if (movedIndex < 0 || movedIndex > 15) return 0;

    auto isProtectedSq = [&](int sq) -> bool {
        return attacked(currColor_, sq2x(sq), sq2y(sq));
    };

    auto pieceAtEyeSq = [&](int sq) -> int {
        return IN_BOARD(sq) ? ucpcSquares_[sq] : 0;
    };

    auto scanRookCap = [&](int startSq, int delta) -> int {
        int sq = startSq + delta;
        while (IN_BOARD(sq)) {
            if (pieceAtEyeSq(sq) != 0) return sq;
            sq += delta;
        }
        return startSq;
    };

    auto scanCannonCap = [&](int startSq, int delta) -> int {
        int sq = startSq + delta;
        bool foundScreen = false;
        while (IN_BOARD(sq)) {
            const int pc = pieceAtEyeSq(sq);
            if (!foundScreen) {
                if (pc != 0) foundScreen = true;
            } else if (pc != 0) {
                return sq;
            }
            sq += delta;
        }
        return startSq;
    };

    int sqDst = 0;
    int pcCaptured = 0;

    switch (movedIndex) {
        case EYE_KNIGHT_FROM:
        case EYE_KNIGHT_TO:
            for (int i = 0; i < 8; ++i) {
                sqDst = sqSrc + ccKnightDelta[i];
                if (!IN_BOARD(sqDst)) continue;
                const int sqPin = sqSrc + ccKnightPin[i];
                if (!IN_BOARD(sqPin) || ucpcSquares_[sqPin] != 0) continue;
                pcCaptured = ucpcSquares_[sqDst];
                if ((pcCaptured & nSideTag) == 0) continue;
                {
                    const int idx = pcCaptured - nSideTag;
                    if (idx >= EYE_ROOK_FROM && idx <= EYE_ROOK_TO) return pcCaptured;
                    if (idx >= EYE_CANNON_FROM && idx <= EYE_CANNON_TO && !isProtectedSq(sqDst)) return pcCaptured;
                    if (idx >= EYE_PAWN_FROM && idx <= EYE_PAWN_TO &&
                        AWAY_HALF(sqDst, sdPlayer_) && !isProtectedSq(sqDst)) return pcCaptured;
                }
            }
            break;

        case EYE_ROOK_FROM:
        case EYE_ROOK_TO: {
            const bool movedVertically = (((SRC(eyeMv) ^ sqSrc) & 0x0f) == 0);
            const int deltas[2] = { movedVertically ? -1 : -16, movedVertically ? 1 : 16 };
            for (int i = 0; i < 2; ++i) {
                sqDst = scanRookCap(sqSrc, deltas[i]);
                if (sqDst == sqSrc) continue;
                pcCaptured = ucpcSquares_[sqDst];
                if ((pcCaptured & nSideTag) == 0) continue;
                const int idx = pcCaptured - nSideTag;
                if (idx >= EYE_KNIGHT_FROM && idx <= EYE_KNIGHT_TO && !isProtectedSq(sqDst)) return pcCaptured;
                if (idx >= EYE_CANNON_FROM && idx <= EYE_CANNON_TO && !isProtectedSq(sqDst)) return pcCaptured;
                if (idx >= EYE_PAWN_FROM && idx <= EYE_PAWN_TO &&
                    AWAY_HALF(sqDst, sdPlayer_) && !isProtectedSq(sqDst)) return pcCaptured;
            }
            break;
        }

        case EYE_CANNON_FROM:
        case EYE_CANNON_TO: {
            const bool movedVertically = (((SRC(eyeMv) ^ sqSrc) & 0x0f) == 0);
            const int deltas[2] = { movedVertically ? -1 : -16, movedVertically ? 1 : 16 };
            for (int i = 0; i < 2; ++i) {
                sqDst = scanCannonCap(sqSrc, deltas[i]);
                if (sqDst == sqSrc) continue;
                pcCaptured = ucpcSquares_[sqDst];
                if ((pcCaptured & nSideTag) == 0) continue;
                const int idx = pcCaptured - nSideTag;
                if (idx >= EYE_KNIGHT_FROM && idx <= EYE_KNIGHT_TO && !isProtectedSq(sqDst)) return pcCaptured;
                if (idx >= EYE_ROOK_FROM && idx <= EYE_ROOK_TO) return pcCaptured;
                if (idx >= EYE_PAWN_FROM && idx <= EYE_PAWN_TO &&
                    AWAY_HALF(sqDst, sdPlayer_) && !isProtectedSq(sqDst)) return pcCaptured;
            }
            break;
        }

        default:
            break;
    }

    return 0;
}

// =====================================================================
//  Repetition / draw
// =====================================================================

int ChessBoard::repStatus(int nRecur) const {
    return eyeRepStatus(nRecur);
}

int ChessBoard::eyeRepStatus(int nRecur) const {
    if (ucRepHash_[zobr_.dwKey & REP_HASH_MASK] == 0) {
        return REP_NONE;
    }

    int sd = OPP_SIDE(sdPlayer_);
    uint32_t dwPerpCheck = 0x1ffff;
    uint32_t dwOppPerpCheck = 0x1ffff;

    int idx = nMoveNum_ - 1;
    while (idx >= 0) {
        const EyeRollback& rb = rbsList_[idx];

        if (rb.wmv == 0 || rb.cptDrw > 0) {
            break;
        }

        if (sd == sdPlayer_) {
            SetPerpCheck(dwPerpCheck, rb.chkChs);

            if (rb.zobr.dwLock0 == zobr_.dwLock0 &&
                rb.zobr.dwLock1 == zobr_.dwLock1) {
                --nRecur;
                if (nRecur == 0) {
                    dwPerpCheck =
                        ((dwPerpCheck & 0xffff) == 0 ? dwPerpCheck : 0xffff);
                    dwOppPerpCheck =
                        ((dwOppPerpCheck & 0xffff) == 0 ? dwOppPerpCheck : 0xffff);

                    if (dwPerpCheck > dwOppPerpCheck) {
                        return REP_LOSS;
                    } else if (dwPerpCheck < dwOppPerpCheck) {
                        return REP_WIN;
                    } else {
                        return REP_DRAW;
                    }
                }
            }
        } else {
            SetPerpCheck(dwOppPerpCheck, rb.chkChs);
        }

        sd = OPP_SIDE(sd);
        --idx;
    }

    return REP_NONE;
}

int ChessBoard::drawValue() const {
    return (nDistance_ & 1) == 0 ? -EYE_DRAW_VALUE_CONST : EYE_DRAW_VALUE_CONST;
}

int ChessBoard::repValue(int nRepStatus) const {
    if (nRepStatus == REP_LOSS) {
        return nDistance_ - EYE_BAN_SCORE_CONST;
    } else if (nRepStatus == REP_WIN) {
        return EYE_BAN_SCORE_CONST - nDistance_;
    } else {
        return drawValue();
    }
}

bool ChessBoard::repeatAfterMove(const Move& move) {
    const colorType side = currColor_;
    if (!makeMoveFast(move)) return false;
    const bool repeated = (eyeRepStatus(1) != REP_NONE);
    undoMoveFast();
    assert(currColor_ == side);
    return repeated;
}

// =====================================================================
//  Move validity / legality
// =====================================================================

bool ChessBoard::isMoveValid(const Move& move, bool) {
    if (!inBoard(move.source_x, move.source_y) || !inBoard(move.target_x, move.target_y)) {
        return false;
    }

    const Grid source = board_[move.source_x][move.source_y];
    const Grid target = board_[move.target_x][move.target_y];

    if (source.color != currColor_ || source.type == None) {
        return false;
    }
    if (target.color == currColor_) {
        return false;
    }

    const int dx = move.target_x - move.source_x;
    const int dy = move.target_y - move.source_y;

    switch (source.type) {
        case King:
            if (target.color == oppColor() && target.type == King &&
                move.source_x == move.target_x &&
                betweenIsEmpty(move.source_x, move.source_y, move.target_x, move.target_y)) {
                return true;
            }
            return inKingArea(move.target_x, move.target_y, currColor_) &&
                   (std::abs(dx) + std::abs(dy) == 1);

        case Assistant:
            return inKingArea(move.target_x, move.target_y, currColor_) &&
                   std::abs(dx) == 1 && std::abs(dy) == 1;

        case Bishop:
            if (!inColorArea(move.target_x, move.target_y, currColor_) ||
                std::abs(dx) != 2 || std::abs(dy) != 2) {
                return false;
            }
            return board_[move.source_x + dx / 2][move.source_y + dy / 2].color == EMPTY;

        case Knight:
            if (std::abs(dx) == 2 && std::abs(dy) == 1) {
                return board_[move.source_x + (dx > 0 ? 1 : -1)][move.source_y].color == EMPTY;
            }
            if (std::abs(dx) == 1 && std::abs(dy) == 2) {
                return board_[move.source_x][move.source_y + (dy > 0 ? 1 : -1)].color == EMPTY;
            }
            return false;

        case Rook:
            return inSameStraightLine(move.source_x, move.source_y, move.target_x, move.target_y) &&
                   betweenIsEmpty(move.source_x, move.source_y, move.target_x, move.target_y);

        case Cannon:
            if (!inSameStraightLine(move.source_x, move.source_y, move.target_x, move.target_y)) {
                return false;
            }
            if (target.color == EMPTY) {
                return betweenNotEmptyNum(move.source_x, move.source_y, move.target_x, move.target_y) == 0;
            }
            return betweenNotEmptyNum(move.source_x, move.source_y, move.target_x, move.target_y) == 1;

        case Pawn: {
            const int forward = (currColor_ == RED ? 1 : -1);
            if (dx == 0 && dy == forward) return true;
            const bool crossed = (currColor_ == RED ? (move.source_y > 4) : (move.source_y < 5));
            return crossed && std::abs(dx) == 1 && dy == 0;
        }

        default:
            return false;
    }
}

bool ChessBoard::isLegalMove(const Move& move, bool mustDefend) {
    if (!isMoveValid(move, false)) return false;
    if (!mustDefend) return true;
    return !isMyKingAttackedAfterMove(move);
}

bool ChessBoard::isLegalMoveWithForbidden(const Move& move, bool mustDefend) {
    if (!isLegalMove(move, mustDefend)) return false;
    if (!repeatAfterMove(move)) return true;
    if (isOppKingAttackedAfterMove(move)) return false;
    return true;
}

bool ChessBoard::isMoveValidWithForbidden(const Move& move, bool mustDefend) {
    return isLegalMoveWithForbidden(move, mustDefend);
}

// =====================================================================
//  Make / undo / null move
// =====================================================================

bool ChessBoard::makeMoveAssumeLegal(const Move& move) {
    if (!inBoard(move.source_x, move.source_y) ||
        !inBoard(move.target_x, move.target_y)) {
        return false;
    }

    if (nMoveNum_ >= MAX_MOVE_NUM) {
        return false;
    }

    const Grid moving = board_[move.source_x][move.source_y];
    const Grid captured = board_[move.target_x][move.target_y];

    if (moving.color != currColor_ || moving.type == None) {
        return false;
    }
    if (captured.color == currColor_) {
        return false;
    }

    const int eyeMv = moveToEyeMv(move);
    const int sqSrc = SRC(eyeMv);
    const int sqDst = DST(eyeMv);

    if (!IN_BOARD(sqSrc) || !IN_BOARD(sqDst)) {
        return false;
    }

    UndoInfo ui;
    ui.move = move;
    ui.moving = moving;
    ui.captured = captured;
    ui.prevHash = hash_;
    ui.prevPeaceTurn = peaceTurn_;
    ui.wasCapture = (captured.color != EMPTY);
    ui.isNullMove = false;
    ui.pcMoved = ucpcSquares_[sqSrc];
    ui.pcCaptured = ucpcSquares_[sqDst];

    if (ui.pcMoved == 0) {
        return false;
    }

    const uint32_t oldKey = zobr_.dwKey;
    if (ucRepHash_[oldKey & REP_HASH_MASK] == 0) {
        ucRepHash_[oldKey & REP_HASH_MASK] = static_cast<uint8_t>(nMoveNum_);
    }

    eyeSaveStatus();

    board_[move.source_x][move.source_y] = Grid();
    board_[move.target_x][move.target_y] = moving;

    const int pcCaptured = eyeMovePiece(eyeMv);

    currColor_ = oppColor(currColor_);
    eyeChangeSide();

    hash_ = fullComputeHash();
    ++turnId_;
    moveHistory_.push_back(move);
    hashHistory_.push_back(hash_);
    captureHistory_.push_back(ui.wasCapture);

    peaceTurn_ = ui.wasCapture ? 0 : (peaceTurn_ + 1);

    EyeRollback& rb = rbsList_[nMoveNum_];
    rb.wmv = static_cast<uint16_t>(eyeMv);
    rb.chkChs = static_cast<int8_t>(checkedBy());

    if (pcCaptured == 0) {
        if (rb.chkChs == 0) {
            rb.chkChs = static_cast<int8_t>(-chasedBy(eyeMv));
        }

        const EyeRollback& last = rbsList_[nMoveNum_ - 1];
        if (last.cptDrw == -100) {
            rb.cptDrw = -100;
        } else {
            int cpt = std::min(static_cast<int>(last.cptDrw), 0)
                    - ((rb.chkChs > 0 || last.chkChs > 0) ? 0 : 1);
            if (cpt < -100) cpt = -100;
            rb.cptDrw = static_cast<int8_t>(cpt);
        }
    } else {
        rb.cptDrw = static_cast<int8_t>(pcCaptured);
    }

    undoStack_.push_back(ui);

    ++nMoveNum_;
    ++nDistance_;
    return true;
}

bool ChessBoard::makeMove(const Move& move) {
    if (!isLegalMove(move, true)) return false;
    return makeMoveAssumeLegal(move);
}

void ChessBoard::undoNullMove() {
    assert(!undoStack_.empty());
    if (undoStack_.empty() || !undoStack_.back().isNullMove) {
        return;
    }

    UndoInfo ui = undoStack_.back();
    undoStack_.pop_back();

    --nMoveNum_;
    --nDistance_;

    sdPlayer_ = OPP_SIDE(sdPlayer_);
    eyeRollback();

    if (ucRepHash_[zobr_.dwKey & REP_HASH_MASK] == nMoveNum_) {
        ucRepHash_[zobr_.dwKey & REP_HASH_MASK] = 0;
    }

    currColor_ = oppColor(currColor_);
    hash_ = ui.prevHash;
    peaceTurn_ = ui.prevPeaceTurn;
}
void ChessBoard::undoMove() {
    assert(!undoStack_.empty());
    if (undoStack_.empty()) {
        return;
    }

    if (undoStack_.back().isNullMove) {
        undoNullMove();
        return;
    }

    UndoInfo ui = undoStack_.back();
    undoStack_.pop_back();

    --nMoveNum_;
    --nDistance_;

    sdPlayer_ = OPP_SIDE(sdPlayer_);
    eyeRollback();
    eyeUndoMovePiece(moveToEyeMv(ui.move), ui.pcCaptured);

    if (ucRepHash_[zobr_.dwKey & REP_HASH_MASK] == nMoveNum_) {
        ucRepHash_[zobr_.dwKey & REP_HASH_MASK] = 0;
    }

    board_[ui.move.source_x][ui.move.source_y] = ui.moving;
    board_[ui.move.target_x][ui.move.target_y] = ui.captured;

    currColor_ = oppColor(currColor_);
    hash_ = ui.prevHash;
    peaceTurn_ = ui.prevPeaceTurn;

    if (!moveHistory_.empty()) moveHistory_.pop_back();
    if (!hashHistory_.empty()) hashHistory_.pop_back();
    if (!captureHistory_.empty()) captureHistory_.pop_back();

    --turnId_;
}

void ChessBoard::makeNullMove() {
    assert(nMoveNum_ < MAX_MOVE_NUM);
    if (nMoveNum_ >= MAX_MOVE_NUM) {
        return;
    }

    UndoInfo ui;
    ui.move = Move();
    ui.moving = Grid();
    ui.captured = Grid();
    ui.prevHash = hash_;
    ui.prevPeaceTurn = peaceTurn_;
    ui.wasCapture = false;
    ui.isNullMove = true;
    ui.pcCaptured = 0;
    ui.pcMoved = 0;

    const uint32_t oldKey = zobr_.dwKey;
    if (ucRepHash_[oldKey & REP_HASH_MASK] == 0) {
        ucRepHash_[oldKey & REP_HASH_MASK] = static_cast<uint8_t>(nMoveNum_);
    }

    eyeSaveStatus();

    currColor_ = oppColor(currColor_);
    hash_ ^= zobristSide_;
    eyeChangeSide();

    EyeRollback& rb = rbsList_[nMoveNum_];
    rb.wmv = 0;
    rb.chkChs = 0;
    rb.cptDrw = 0;

    undoStack_.push_back(ui);

    ++nMoveNum_;
    ++nDistance_;
}

bool ChessBoard::makeMoveFast(const Move& move) {
    if (!inBoard(move.source_x, move.source_y) ||
        !inBoard(move.target_x, move.target_y)) {
        return false;
    }

    if (nMoveNum_ >= MAX_MOVE_NUM) {
        return false;
    }

    const Grid moving = board_[move.source_x][move.source_y];
    const Grid captured = board_[move.target_x][move.target_y];

    if (moving.color != currColor_ || moving.type == None) {
        return false;
    }
    if (captured.color == currColor_) {
        return false;
    }

    const int eyeMv = moveToEyeMv(move);
    const int sqSrc = SRC(eyeMv);
    const int sqDst = DST(eyeMv);

    if (!IN_BOARD(sqSrc) || !IN_BOARD(sqDst)) {
        return false;
    }

    UndoInfo ui;
    ui.move = move;
    ui.moving = moving;
    ui.captured = captured;
    ui.prevHash = hash_;
    ui.prevPeaceTurn = peaceTurn_;
    ui.wasCapture = (captured.color != EMPTY);
    ui.isNullMove = false;
    ui.pcMoved = ucpcSquares_[sqSrc];
    ui.pcCaptured = ucpcSquares_[sqDst];

    if (ui.pcMoved == 0) {
        return false;
    }

    const uint32_t oldKey = zobr_.dwKey;
    if (ucRepHash_[oldKey & REP_HASH_MASK] == 0) {
        ucRepHash_[oldKey & REP_HASH_MASK] = static_cast<uint8_t>(nMoveNum_);
    }

    eyeSaveStatus();

    board_[move.source_x][move.source_y] = Grid();
    board_[move.target_x][move.target_y] = moving;

    const int pcCaptured = eyeMovePiece(eyeMv);

    currColor_ = oppColor(currColor_);
    eyeChangeSide();

    // 这里故意不做 fullComputeHash()
    // 这里故意不碰 moveHistory_/hashHistory_/captureHistory_/turnId_

    EyeRollback& rb = rbsList_[nMoveNum_];
    rb.wmv = static_cast<uint16_t>(eyeMv);
    rb.chkChs = static_cast<int8_t>(checkedBy());

    if (pcCaptured == 0) {
        if (rb.chkChs == 0) {
            rb.chkChs = static_cast<int8_t>(-chasedBy(eyeMv));
        }

        const EyeRollback& last = rbsList_[nMoveNum_ - 1];
        if (last.cptDrw == -100) {
            rb.cptDrw = -100;
        } else {
            int cpt = std::min(static_cast<int>(last.cptDrw), 0)
                    - ((rb.chkChs > 0 || last.chkChs > 0) ? 0 : 1);
            if (cpt < -100) cpt = -100;
            rb.cptDrw = static_cast<int8_t>(cpt);
        }
    } else {
        rb.cptDrw = static_cast<int8_t>(pcCaptured);
    }

    undoStack_.push_back(ui);
    ++nMoveNum_;
    ++nDistance_;
    return true;
}

void ChessBoard::undoMoveFast() {
    assert(!undoStack_.empty());
    if (undoStack_.empty()) {
        return;
    }

    if (undoStack_.back().isNullMove) {
        undoNullMoveFast();
        return;
    }

    UndoInfo ui = undoStack_.back();
    undoStack_.pop_back();

    --nMoveNum_;
    --nDistance_;

    sdPlayer_ = OPP_SIDE(sdPlayer_);
    eyeRollback();
    eyeUndoMovePiece(moveToEyeMv(ui.move), ui.pcCaptured);

    if (ucRepHash_[zobr_.dwKey & REP_HASH_MASK] == nMoveNum_) {
        ucRepHash_[zobr_.dwKey & REP_HASH_MASK] = 0;
    }

    board_[ui.move.source_x][ui.move.source_y] = ui.moving;
    board_[ui.move.target_x][ui.move.target_y] = ui.captured;

    currColor_ = oppColor(currColor_);

    // 快速路径：不恢复 hashHistory_/captureHistory_/moveHistory_
    // hash_ 保持旧值也无所谓，因为搜索主干看的是 zobr_
}
void ChessBoard::makeNullMoveFast() {
    assert(nMoveNum_ < MAX_MOVE_NUM);
    if (nMoveNum_ >= MAX_MOVE_NUM) {
        return;
    }

    UndoInfo ui;
    ui.move = Move();
    ui.moving = Grid();
    ui.captured = Grid();
    ui.prevHash = hash_;
    ui.prevPeaceTurn = peaceTurn_;
    ui.wasCapture = false;
    ui.isNullMove = true;
    ui.pcCaptured = 0;
    ui.pcMoved = 0;

    const uint32_t oldKey = zobr_.dwKey;
    if (ucRepHash_[oldKey & REP_HASH_MASK] == 0) {
        ucRepHash_[oldKey & REP_HASH_MASK] = static_cast<uint8_t>(nMoveNum_);
    }

    eyeSaveStatus();

    currColor_ = oppColor(currColor_);
    eyeChangeSide();

    EyeRollback& rb = rbsList_[nMoveNum_];
    rb.wmv = 0;
    rb.chkChs = 0;
    rb.cptDrw = 0;

    undoStack_.push_back(ui);
    ++nMoveNum_;
    ++nDistance_;
}
void ChessBoard::undoNullMoveFast() {
    assert(!undoStack_.empty());
    if (undoStack_.empty() || !undoStack_.back().isNullMove) {
        return;
    }

    undoStack_.pop_back();

    --nMoveNum_;
    --nDistance_;

    sdPlayer_ = OPP_SIDE(sdPlayer_);
    eyeRollback();

    if (ucRepHash_[zobr_.dwKey & REP_HASH_MASK] == nMoveNum_) {
        ucRepHash_[zobr_.dwKey & REP_HASH_MASK] = 0;
    }

    currColor_ = oppColor(currColor_);
}
// =====================================================================
//  Check / king / legality probes
// =====================================================================

bool ChessBoard::findKing(colorType c, int& kx, int& ky) const {
    const int sd = (c == RED ? 0 : 1);
    const int pc = SIDE_TAG(sd) + EYE_KING_FROM;
    const int sq = ucsqPieces_[pc];

    if (sq == 0 || !IN_BOARD(sq)) {
        kx = -1;
        ky = -1;
        return false;
    }

    kx = sq2x(sq);
    ky = sq2y(sq);
    return true;
}

bool ChessBoard::isInCheck() const {
    return checkedBy(true) != 0;
}

bool ChessBoard::isMyKingAttackedAfterMove(const Move& move) {
    const colorType side = currColor_;
    if (!makeMoveFast(move)) {
        return true;
    }

    int kx = -1, ky = -1;
    const bool found = findKing(side, kx, ky);
    const bool attackedAfter = found && attacked(oppColor(side), kx, ky);

    undoMoveFast();
    return attackedAfter;
}

bool ChessBoard::isOppKingAttackedAfterMove(const Move& move) {
    const colorType side = currColor_;
    const colorType opp = oppColor(side);

    if (!makeMoveFast(move)) {
        return false;
    }

    int kx = -1, ky = -1;
    const bool found = findKing(opp, kx, ky);
    const bool attackedAfter = found && attacked(side, kx, ky);

    undoMoveFast();
    return attackedAfter;
}

bool ChessBoard::winAfterMove(const Move& move) {
    if (!makeMoveFast(move)) {
        return false;
    }

    const colorType movedSide = oppColor();

    // 非法着：自己被将
    if (kingAttacked(movedSide)) {
        undoMoveFast();
        return false;
    }

    // 如果对面将已经不在了，直接赢
    int okx = -1, oky = -1;
    const bool oppKingExists = findKing(currColor_, okx, oky);
    if (!oppKingExists) {
        undoMoveFast();
        return true;
    }

    // 看对方是否还有任何合法回应
    Move replies[EYE_MAX_MOVES];
    const int replyCnt = generateMovesFast(replies, false);

    bool hasLegalReply = false;
    for (int i = 0; i < replyCnt; ++i) {
        if (!makeMoveFast(replies[i])) {
            continue;
        }

        const colorType replySide = oppColor();
        const bool legal = !kingAttacked(replySide);

        undoMoveFast();

        if (legal) {
            hasLegalReply = true;
            break;
        }
    }

    undoMoveFast();
    return !hasLegalReply;
}

// =====================================================================
//  Move generation
// =====================================================================

void ChessBoard::generateMoves(std::vector<Move>& legalMoves, bool mustDefend) {
    legalMoves.clear();

    const int tag = SIDE_TAG(sdPlayer_);
    const colorType opp = oppColor();

    auto tryPush = [&](int sx, int sy, int tx, int ty) {
        if (!inBoard(tx, ty)) return;
        if (board_[tx][ty].color == currColor_) return;
        const Move mv(sx, sy, tx, ty);
        if (!mustDefend || !isMyKingAttackedAfterMove(mv)) {
            legalMoves.push_back(mv);
        }
    };

    {
        const int sq = ucsqPieces_[tag + EYE_KING_FROM];
        if (sq != 0) {
            const int x = sq2x(sq), y = sq2y(sq);
            for (int d = 0; d < 4; ++d) {
                const int tx = x + dx_strai[d], ty = y + dy_strai[d];
                if (inKingArea(tx, ty, currColor_)) tryPush(x, y, tx, ty);
            }
        }
    }

    for (int i = EYE_ADVISOR_FROM; i <= EYE_ADVISOR_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 4; ++d) {
            const int tx = x + dx_ob[d], ty = y + dy_ob[d];
            if (inKingArea(tx, ty, currColor_)) tryPush(x, y, tx, ty);
        }
    }

    for (int i = EYE_BISHOP_FROM; i <= EYE_BISHOP_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 4; ++d) {
            const int tx = x + dx_bishop[d], ty = y + dy_bishop[d];
            if (!inBoard(tx, ty) || !inColorArea(tx, ty, currColor_)) continue;
            const int ex = x + dx_bishop_eye[d], ey = y + dy_bishop_eye[d];
            if (board_[ex][ey].color != EMPTY) continue;
            tryPush(x, y, tx, ty);
        }
    }

    for (int i = EYE_KNIGHT_FROM; i <= EYE_KNIGHT_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 8; ++d) {
            const int tx = x + dx_knight[d], ty = y + dy_knight[d];
            if (!inBoard(tx, ty)) continue;
            const int fx = x + dx_knight_foot[d], fy = y + dy_knight_foot[d];
            if (board_[fx][fy].color != EMPTY) continue;
            tryPush(x, y, tx, ty);
        }
    }

    for (int i = EYE_ROOK_FROM; i <= EYE_ROOK_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 4; ++d) {
            int tx = x + dx_strai[d], ty = y + dy_strai[d];
            while (inBoard(tx, ty)) {
                if (board_[tx][ty].color == currColor_) break;
                tryPush(x, y, tx, ty);
                if (board_[tx][ty].color == opp) break;
                tx += dx_strai[d];
                ty += dy_strai[d];
            }
        }
    }

    for (int i = EYE_CANNON_FROM; i <= EYE_CANNON_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 4; ++d) {
            int tx = x + dx_strai[d], ty = y + dy_strai[d];
            bool foundScreen = false;
            while (inBoard(tx, ty)) {
                const Grid& target = board_[tx][ty];
                if (!foundScreen) {
                    if (target.color == EMPTY) {
                        tryPush(x, y, tx, ty);
                    } else {
                        foundScreen = true;
                    }
                } else {
                    if (target.color != EMPTY) {
                        if (target.color == opp) tryPush(x, y, tx, ty);
                        break;
                    }
                }
                tx += dx_strai[d];
                ty += dy_strai[d];
            }
        }
    }

    for (int i = EYE_PAWN_FROM; i <= EYE_PAWN_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        const int forwardOne = (currColor_ == RED ? 1 : -1);
        tryPush(x, y, x, y + forwardOne);
        const bool crossed = (currColor_ == RED ? (y > 4) : (y < 5));
        if (crossed) {
            tryPush(x, y, x - 1, y);
            tryPush(x, y, x + 1, y);
        }
    }
}
int ChessBoard::generateMovesFast(Move outMoves[], bool) {
    int cnt = 0;
    const colorType opp = oppColor();
    const int tag = SIDE_TAG(sdPlayer_);

    auto pushPseudo = [&](int sx, int sy, int tx, int ty) {
        if (!inBoard(tx, ty)) return;
        if (board_[tx][ty].color == currColor_) return;
        if (cnt < EYE_MAX_MOVES) {
            outMoves[cnt++] = Move(sx, sy, tx, ty);
        }
    };

    // King
    {
        const int sq = ucsqPieces_[tag + EYE_KING_FROM];
        if (sq != 0) {
            const int x = sq2x(sq), y = sq2y(sq);

            for (int d = 0; d < 4; ++d) {
                const int tx = x + dx_strai[d];
                const int ty = y + dy_strai[d];
                if (inBoard(tx, ty) && inKingArea(tx, ty, currColor_)) {
                    pushPseudo(x, y, tx, ty);
                }
            }

            int ty = y + (currColor_ == RED ? 1 : -1);
            while (inBoard(x, ty)) {
                if (board_[x][ty].color != EMPTY) {
                    if (board_[x][ty].color == opp &&
                        board_[x][ty].type == King) {
                        pushPseudo(x, y, x, ty);
                    }
                    break;
                }
                ty += (currColor_ == RED ? 1 : -1);
            }
        }
    }

    // Advisors
    for (int i = EYE_ADVISOR_FROM; i <= EYE_ADVISOR_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 4; ++d) {
            const int tx = x + dx_ob[d];
            const int ty = y + dy_ob[d];
            if (inBoard(tx, ty) && inKingArea(tx, ty, currColor_)) {
                pushPseudo(x, y, tx, ty);
            }
        }
    }

    // Bishops
    for (int i = EYE_BISHOP_FROM; i <= EYE_BISHOP_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 4; ++d) {
            const int tx = x + dx_bishop[d];
            const int ty = y + dy_bishop[d];
            const int ex = x + dx_bishop_eye[d];
            const int ey = y + dy_bishop_eye[d];
            if (!inBoard(tx, ty) || !inBoard(ex, ey)) continue;
            if (!inColorArea(tx, ty, currColor_)) continue;
            if (board_[ex][ey].color != EMPTY) continue;
            pushPseudo(x, y, tx, ty);
        }
    }

    // Knights
    for (int i = EYE_KNIGHT_FROM; i <= EYE_KNIGHT_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 8; ++d) {
            const int tx = x + dx_knight[d];
            const int ty = y + dy_knight[d];
            const int fx = x + dx_knight_foot[d];
            const int fy = y + dy_knight_foot[d];
            if (!inBoard(tx, ty) || !inBoard(fx, fy)) continue;
            if (board_[fx][fy].color != EMPTY) continue;
            pushPseudo(x, y, tx, ty);
        }
    }

    // Rooks
    for (int i = EYE_ROOK_FROM; i <= EYE_ROOK_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 4; ++d) {
            int tx = x + dx_strai[d];
            int ty = y + dy_strai[d];
            while (inBoard(tx, ty)) {
                if (board_[tx][ty].color == EMPTY) {
                    pushPseudo(x, y, tx, ty);
                } else {
                    if (board_[tx][ty].color == opp) {
                        pushPseudo(x, y, tx, ty);
                    }
                    break;
                }
                tx += dx_strai[d];
                ty += dy_strai[d];
            }
        }
    }

    // Cannons
    for (int i = EYE_CANNON_FROM; i <= EYE_CANNON_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 4; ++d) {
            int tx = x + dx_strai[d];
            int ty = y + dy_strai[d];

            while (inBoard(tx, ty) && board_[tx][ty].color == EMPTY) {
                pushPseudo(x, y, tx, ty);
                tx += dx_strai[d];
                ty += dy_strai[d];
            }

            if (!inBoard(tx, ty)) continue;

            tx += dx_strai[d];
            ty += dy_strai[d];
            while (inBoard(tx, ty)) {
                if (board_[tx][ty].color != EMPTY) {
                    if (board_[tx][ty].color == opp) {
                        pushPseudo(x, y, tx, ty);
                    }
                    break;
                }
                tx += dx_strai[d];
                ty += dy_strai[d];
            }
        }
    }

    // Pawns
    for (int i = EYE_PAWN_FROM; i <= EYE_PAWN_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        const int fwd = (currColor_ == RED ? 1 : -1);

        if (inBoard(x, y + fwd)) {
            pushPseudo(x, y, x, y + fwd);
        }

        const bool crossed = (currColor_ == RED ? (y > 4) : (y < 5));
        if (crossed) {
            if (inBoard(x - 1, y)) pushPseudo(x, y, x - 1, y);
            if (inBoard(x + 1, y)) pushPseudo(x, y, x + 1, y);
        }
    }

    return cnt;
}
void ChessBoard::generateCaptures(std::vector<Move>& captures, bool mustDefend) {
    captures.clear();

    const int tag = SIDE_TAG(sdPlayer_);
    const colorType opp = oppColor();

    auto tryCapPush = [&](int sx, int sy, int tx, int ty) {
        if (!inBoard(tx, ty)) return;
        if (board_[tx][ty].color != opp) return;
        const Move mv(sx, sy, tx, ty);
        if (!mustDefend || !isMyKingAttackedAfterMove(mv)) {
            captures.push_back(mv);
        }
    };

    {
        const int sq = ucsqPieces_[tag + EYE_KING_FROM];
        if (sq != 0) {
            const int x = sq2x(sq), y = sq2y(sq);
            for (int d = 0; d < 4; ++d) {
                const int tx = x + dx_strai[d], ty = y + dy_strai[d];
                if (inKingArea(tx, ty, currColor_)) tryCapPush(x, y, tx, ty);
            }
        }
    }

    for (int i = EYE_ADVISOR_FROM; i <= EYE_ADVISOR_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 4; ++d) {
            const int tx = x + dx_ob[d], ty = y + dy_ob[d];
            if (inKingArea(tx, ty, currColor_)) tryCapPush(x, y, tx, ty);
        }
    }

    for (int i = EYE_BISHOP_FROM; i <= EYE_BISHOP_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 4; ++d) {
            const int tx = x + dx_bishop[d], ty = y + dy_bishop[d];
            if (!inBoard(tx, ty) || !inColorArea(tx, ty, currColor_)) continue;
            const int ex = x + dx_bishop_eye[d], ey = y + dy_bishop_eye[d];
            if (board_[ex][ey].color != EMPTY) continue;
            tryCapPush(x, y, tx, ty);
        }
    }

    for (int i = EYE_KNIGHT_FROM; i <= EYE_KNIGHT_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 8; ++d) {
            const int tx = x + dx_knight[d], ty = y + dy_knight[d];
            if (!inBoard(tx, ty)) continue;
            const int fx = x + dx_knight_foot[d], fy = y + dy_knight_foot[d];
            if (board_[fx][fy].color != EMPTY) continue;
            tryCapPush(x, y, tx, ty);
        }
    }

    for (int i = EYE_ROOK_FROM; i <= EYE_ROOK_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 4; ++d) {
            int tx = x + dx_strai[d], ty = y + dy_strai[d];
            while (inBoard(tx, ty)) {
                const Grid& tg = board_[tx][ty];
                if (tg.color == EMPTY) {
                    tx += dx_strai[d];
                    ty += dy_strai[d];
                    continue;
                }
                if (tg.color == opp) tryCapPush(x, y, tx, ty);
                break;
            }
        }
    }

    for (int i = EYE_CANNON_FROM; i <= EYE_CANNON_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        for (int d = 0; d < 4; ++d) {
            int tx = x + dx_strai[d], ty = y + dy_strai[d];
            bool foundScreen = false;
            while (inBoard(tx, ty)) {
                const Grid& tg = board_[tx][ty];
                if (!foundScreen) {
                    if (tg.color != EMPTY) foundScreen = true;
                } else {
                    if (tg.color != EMPTY) {
                        if (tg.color == opp) tryCapPush(x, y, tx, ty);
                        break;
                    }
                }
                tx += dx_strai[d];
                ty += dy_strai[d];
            }
        }
    }

    for (int i = EYE_PAWN_FROM; i <= EYE_PAWN_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        const int fwd = (currColor_ == RED ? 1 : -1);
        tryCapPush(x, y, x, y + fwd);
        const bool crossed = (currColor_ == RED ? (y > 4) : (y < 5));
        if (crossed) {
            tryCapPush(x, y, x - 1, y);
            tryCapPush(x, y, x + 1, y);
        }
    }
}
int ChessBoard::generateCapturesFast(Move outMoves[], bool) {
    int cnt = 0;
    const colorType opp = oppColor();
    const int tag = SIDE_TAG(sdPlayer_);

    auto pushCap = [&](int sx, int sy, int tx, int ty) {
        if (!inBoard(tx, ty)) return;
        if (board_[tx][ty].color != opp) return;
        if (cnt < EYE_MAX_MOVES) {
            outMoves[cnt++] = Move(sx, sy, tx, ty);
        }
    };

    // King
    {
        const int sq = ucsqPieces_[tag + EYE_KING_FROM];
        if (sq != 0) {
            const int x = sq2x(sq), y = sq2y(sq);
            int ty = y + (currColor_ == RED ? 1 : -1);
            while (inBoard(x, ty)) {
                if (board_[x][ty].color != EMPTY) {
                    if (board_[x][ty].color == opp &&
                        board_[x][ty].type == King) {
                        pushCap(x, y, x, ty);
                    }
                    break;
                }
                ty += (currColor_ == RED ? 1 : -1);
            }
        }
    }

    // Advisors
    for (int i = EYE_ADVISOR_FROM; i <= EYE_ADVISOR_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 4; ++d) {
            const int tx = x + dx_ob[d];
            const int ty = y + dy_ob[d];
            if (inBoard(tx, ty) && inKingArea(tx, ty, currColor_)) {
                pushCap(x, y, tx, ty);
            }
        }
    }

    // Bishops
    for (int i = EYE_BISHOP_FROM; i <= EYE_BISHOP_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 4; ++d) {
            const int tx = x + dx_bishop[d];
            const int ty = y + dy_bishop[d];
            const int ex = x + dx_bishop_eye[d];
            const int ey = y + dy_bishop_eye[d];
            if (!inBoard(tx, ty) || !inBoard(ex, ey)) continue;
            if (!inColorArea(tx, ty, currColor_)) continue;
            if (board_[ex][ey].color != EMPTY) continue;
            pushCap(x, y, tx, ty);
        }
    }

    // Knights
    for (int i = EYE_KNIGHT_FROM; i <= EYE_KNIGHT_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 8; ++d) {
            const int tx = x + dx_knight[d];
            const int ty = y + dy_knight[d];
            const int fx = x + dx_knight_foot[d];
            const int fy = y + dy_knight_foot[d];
            if (!inBoard(tx, ty) || !inBoard(fx, fy)) continue;
            if (board_[fx][fy].color != EMPTY) continue;
            pushCap(x, y, tx, ty);
        }
    }

    // Rooks
    for (int i = EYE_ROOK_FROM; i <= EYE_ROOK_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 4; ++d) {
            int tx = x + dx_strai[d];
            int ty = y + dy_strai[d];
            while (inBoard(tx, ty)) {
                if (board_[tx][ty].color == EMPTY) {
                    tx += dx_strai[d];
                    ty += dy_strai[d];
                    continue;
                }
                if (board_[tx][ty].color == opp) {
                    pushCap(x, y, tx, ty);
                }
                break;
            }
        }
    }

    // Cannons
    for (int i = EYE_CANNON_FROM; i <= EYE_CANNON_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);

        for (int d = 0; d < 4; ++d) {
            int tx = x + dx_strai[d];
            int ty = y + dy_strai[d];
            bool foundScreen = false;
            while (inBoard(tx, ty)) {
                const Grid& tg = board_[tx][ty];
                if (!foundScreen) {
                    if (tg.color != EMPTY) foundScreen = true;
                } else {
                    if (tg.color != EMPTY) {
                        if (tg.color == opp) pushCap(x, y, tx, ty);
                        break;
                    }
                }
                tx += dx_strai[d];
                ty += dy_strai[d];
            }
        }
    }

    // Pawns
    for (int i = EYE_PAWN_FROM; i <= EYE_PAWN_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        const int x = sq2x(sq), y = sq2y(sq);
        const int fwd = (currColor_ == RED ? 1 : -1);

        pushCap(x, y, x, y + fwd);

        const bool crossed = (currColor_ == RED ? (y > 4) : (y < 5));
        if (crossed) {
            pushCap(x, y, x - 1, y);
            pushCap(x, y, x + 1, y);
        }
    }

    return cnt;
}
void ChessBoard::generateMovesWithForbidden(std::vector<Move>& legalMoves, bool) {
    Move allMoves[EYE_MAX_MOVES];
    const int allCnt = generateMovesFast(allMoves, false);

    legalMoves.clear();
    legalMoves.reserve(allCnt);

    for (int i = 0; i < allCnt; ++i) {
        const Move& mv = allMoves[i];

        if (!makeMoveFast(mv)) {
            continue;
        }

        const colorType movedSide = oppColor();

        // 先过滤“走完自己被将”的非法着
        if (kingAttacked(movedSide)) {
            undoMoveFast();
            continue;
        }

        // 再判断长打禁手：重复 + 对方被将
        const bool repeated = (eyeRepStatus(1) != REP_NONE);
        const bool givesCheck = isInCheck();

        undoMoveFast();

        if (!repeated || !givesCheck) {
            legalMoves.push_back(mv);
        }
    }
}

// =====================================================================
//  Misc public helpers
// =====================================================================

bool ChessBoard::isCapture(const Move& mv) const {
    const Grid& tg = board_[mv.target_x][mv.target_y];
    return tg.color != EMPTY && tg.color != currColor_;
}

bool ChessBoard::lastMoveWasCapture() const {
    return !undoStack_.empty() &&
           !undoStack_.back().isNullMove &&
           undoStack_.back().wasCapture;
}

int ChessBoard::countMajorPieces(colorType c) const {
    const int sd = (c == RED ? 0 : 1);
    const int tag = SIDE_TAG(sd);
    int cnt = 0;
    for (int i = 1; i <= 15; ++i) {
        if (ucsqPieces_[tag + i] != 0) ++cnt;
    }
    return cnt;
}

int ChessBoard::turnCount() const {
    return turnId_;
}

bool ChessBoard::exceedMaxPeaceState() const {
    return peaceTurn_ >= 59;
}

bool ChessBoard::nullOkay() const {
    const int tag = SIDE_TAG(sdPlayer_);
    int total = 0;

    if (ucsqPieces_[tag + EYE_ROOK_FROM] != 0) total += 120;
    if (ucsqPieces_[tag + EYE_ROOK_TO]   != 0) total += 120;

    if (ucsqPieces_[tag + EYE_KNIGHT_FROM] != 0) total += 80;
    if (ucsqPieces_[tag + EYE_KNIGHT_TO]   != 0) total += 80;

    if (ucsqPieces_[tag + EYE_CANNON_FROM] != 0) total += 80;
    if (ucsqPieces_[tag + EYE_CANNON_TO]   != 0) total += 80;

    for (int i = EYE_PAWN_FROM; i <= EYE_PAWN_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        total += AWAY_HALF(sq, sdPlayer_) ? 40 : 20;
    }

    return total > 200;
}

bool ChessBoard::nullSafe() const {
    const int tag = SIDE_TAG(sdPlayer_);
    int total = 0;

    if (ucsqPieces_[tag + EYE_ROOK_FROM] != 0) total += 120;
    if (ucsqPieces_[tag + EYE_ROOK_TO]   != 0) total += 120;

    if (ucsqPieces_[tag + EYE_KNIGHT_FROM] != 0) total += 80;
    if (ucsqPieces_[tag + EYE_KNIGHT_TO]   != 0) total += 80;

    if (ucsqPieces_[tag + EYE_CANNON_FROM] != 0) total += 80;
    if (ucsqPieces_[tag + EYE_CANNON_TO]   != 0) total += 80;

    for (int i = EYE_PAWN_FROM; i <= EYE_PAWN_TO; ++i) {
        const int sq = ucsqPieces_[tag + i];
        if (sq == 0) continue;
        total += AWAY_HALF(sq, sdPlayer_) ? 40 : 20;
    }

    return total > 400;
}
