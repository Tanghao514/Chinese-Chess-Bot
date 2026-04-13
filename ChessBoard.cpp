#include "ChessBoard.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <random>

// ===== Zobrist 静态成员定义 =====
bool ChessBoard::zobristInited_ = false;
uint64_t ChessBoard::zobristTable_[BOARDWIDTH][BOARDHEIGHT][8][3];
uint64_t ChessBoard::zobristSide_;

void ChessBoard::initZobrist() {
    if (zobristInited_) return;
    // 用固定种子保证确定性
    std::mt19937_64 rng(0x12345678ABCDEF01ULL);
    for (int x = 0; x < BOARDWIDTH; x++)
        for (int y = 0; y < BOARDHEIGHT; y++)
            for (int p = 0; p < 8; p++)
                for (int c = 0; c < 3; c++)
                    zobristTable_[x][y][p][c] = rng();
    zobristSide_ = rng();
    zobristInited_ = true;
}

ChessBoard::ChessBoard() {
    initZobrist();
    resetBoard();
    continuePeaceTurn_.clear();
    lastMoveEaten_.clear();
    hashHistory_.clear();

    continuePeaceTurn_.push_back(0);
    lastMoveEaten_.push_back(false);
    currColor_ = RED;
    currTurnId_ = 0;

    currentHash_ = computeHash();
    hashHistory_.push_back(currentHash_);
}

void ChessBoard::resetBoard() {
    std::vector<Grid> currState;
    Grid boardInfo[BOARDWIDTH][BOARDHEIGHT];

    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            boardInfo[x][y] = Grid(None, EMPTY);
        }
    }

    boardInfo[0][0] = Grid(Rook, RED);
    boardInfo[1][0] = Grid(Knight, RED);
    boardInfo[2][0] = Grid(Bishop, RED);
    boardInfo[3][0] = Grid(Assistant, RED);
    boardInfo[4][0] = Grid(King, RED);
    boardInfo[5][0] = Grid(Assistant, RED);
    boardInfo[6][0] = Grid(Bishop, RED);
    boardInfo[7][0] = Grid(Knight, RED);
    boardInfo[8][0] = Grid(Rook, RED);
    boardInfo[1][2] = Grid(Cannon, RED);
    boardInfo[7][2] = Grid(Cannon, RED);
    boardInfo[0][3] = Grid(Pawn, RED);
    boardInfo[2][3] = Grid(Pawn, RED);
    boardInfo[4][3] = Grid(Pawn, RED);
    boardInfo[6][3] = Grid(Pawn, RED);
    boardInfo[8][3] = Grid(Pawn, RED);

    boardInfo[0][9] = Grid(Rook, BLACK);
    boardInfo[1][9] = Grid(Knight, BLACK);
    boardInfo[2][9] = Grid(Bishop, BLACK);
    boardInfo[3][9] = Grid(Assistant, BLACK);
    boardInfo[4][9] = Grid(King, BLACK);
    boardInfo[5][9] = Grid(Assistant, BLACK);
    boardInfo[6][9] = Grid(Bishop, BLACK);
    boardInfo[7][9] = Grid(Knight, BLACK);
    boardInfo[8][9] = Grid(Rook, BLACK);
    boardInfo[1][7] = Grid(Cannon, BLACK);
    boardInfo[7][7] = Grid(Cannon, BLACK);
    boardInfo[0][6] = Grid(Pawn, BLACK);
    boardInfo[2][6] = Grid(Pawn, BLACK);
    boardInfo[4][6] = Grid(Pawn, BLACK);
    boardInfo[6][6] = Grid(Pawn, BLACK);
    boardInfo[8][6] = Grid(Pawn, BLACK);

    for (int mpos = 0; mpos < BOARDWIDTH * BOARDHEIGHT; mpos++) {
        currState.push_back(boardInfo[pos2x(mpos)][pos2y(mpos)]);
    }

    state_.clear();
    state_.push_back(currState);
    currentHash_ = 0;
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            const Grid& g = state_[0][xy2pos(x, y)];
            if (g.color != EMPTY) {
                currentHash_ ^= zobristTable_[x][y][g.type][g.color];
            }
        }
    }
    if (currColor_ == BLACK) {
        currentHash_ ^= zobristSide_;
    }
}

void ChessBoard::generateMoves(std::vector<Move>& legalMoves, bool mustDefend) {
    legalMoves.clear();

    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state_[currTurnId_][xy2pos(x, y)];
            if (curGrid.color != currColor_) continue;

            switch (curGrid.type) {
                case King: {
                    for (int dir = 0; dir < 4; dir++) {
                        int tx = x + dx_strai[dir];
                        int ty = y + dy_strai[dir];
                        if (!inKingArea(tx, ty, curGrid.color)) continue;
                        auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
                        if (tGrid.color == currColor_) continue;

                        if (mustDefend) {
                            if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        } else {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    break;
                }

                case Assistant: {
                    for (int dir = 0; dir < 4; dir++) {
                        int tx = x + dx_ob[dir];
                        int ty = y + dy_ob[dir];
                        if (!inKingArea(tx, ty, curGrid.color)) continue;
                        auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
                        if (tGrid.color == currColor_) continue;

                        if (mustDefend) {
                            if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        } else {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    break;
                }

                case Pawn: {
                    int forwardOne = (curGrid.color == RED ? 1 : -1);
                    bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5));

                    int tx = x;
                    int ty = y + forwardOne;
                    if (inBoard(tx, ty)) {
                        auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
                        if (tGrid.color != currColor_) {
                            if (mustDefend) {
                                if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            } else {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                    }

                    if (crossMidLine) {
                        for (int dir = 0; dir < 2; dir++) {
                            int ox = x + dx_lr[dir];
                            int oy = y;
                            if (!inBoard(ox, oy)) continue;
                            auto& oGrid = state_[currTurnId_][xy2pos(ox, oy)];
                            if (oGrid.color == currColor_) continue;

                            if (mustDefend) {
                                if (!isMyKingAttackedAfterMove(Move(x, y, ox, oy))) {
                                    legalMoves.emplace_back(x, y, ox, oy);
                                }
                            } else {
                                legalMoves.emplace_back(x, y, ox, oy);
                            }
                        }
                    }
                    break;
                }

                case Knight: {
                    for (int dir = 0; dir < 8; dir++) {
                        int tx = x + dx_knight[dir];
                        int ty = y + dy_knight[dir];
                        if (!inBoard(tx, ty)) continue;
                        auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
                        int fx = x + dx_knight_foot[dir];
                        int fy = y + dy_knight_foot[dir];
                        auto& fGrid = state_[currTurnId_][xy2pos(fx, fy)];
                        if (tGrid.color == currColor_ || fGrid.color != EMPTY) continue;

                        if (mustDefend) {
                            if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        } else {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    break;
                }

                case Bishop: {
                    for (int dir = 0; dir < 4; dir++) {
                        int tx = x + dx_bishop[dir];
                        int ty = y + dy_bishop[dir];
                        if (!inBoard(tx, ty) || !inColorArea(tx, ty, currColor_)) continue;

                        int ex = x + dx_bishop_eye[dir];
                        int ey = y + dy_bishop_eye[dir];
                        auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
                        auto& eGrid = state_[currTurnId_][xy2pos(ex, ey)];
                        if (tGrid.color == currColor_ || eGrid.color != EMPTY) continue;

                        if (mustDefend) {
                            if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        } else {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    break;
                }

                case Rook: {
                    for (int dir = 0; dir < 4; dir++) {
                        int tx = x + dx_strai[dir];
                        int ty = y + dy_strai[dir];
                        while (inBoard(tx, ty)) {
                            auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
                            if (tGrid.color == currColor_) {
                                break;
                            } else if (tGrid.color == oppColor()) {
                                if (mustDefend) {
                                    if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                } else {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                                break;
                            } else {
                                if (mustDefend) {
                                    if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                } else {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            }
                            tx += dx_strai[dir];
                            ty += dy_strai[dir];
                        }
                    }
                    break;
                }

                case Cannon: {
                    for (int dir = 0; dir < 4; dir++) {
                        int tx = x + dx_strai[dir];
                        int ty = y + dy_strai[dir];
                        while (inBoard(tx, ty)) {
                            int betweenNum = betweenNotEmptyNum(x, y, tx, ty);
                            if (betweenNum > 1) {
                                break;
                            } else if (betweenNum == 1) {
                                auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
                                if (tGrid.color == currColor_ || tGrid.color == EMPTY) {
                                    tx += dx_strai[dir];
                                    ty += dy_strai[dir];
                                    continue;
                                }
                                if (mustDefend) {
                                    if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                } else {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            } else {
                                auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
                                if (tGrid.color != EMPTY) {
                                    tx += dx_strai[dir];
                                    ty += dy_strai[dir];
                                    continue;
                                }
                                if (mustDefend) {
                                    if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty))) {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                } else {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            }
                            tx += dx_strai[dir];
                            ty += dy_strai[dir];
                        }
                    }
                    break;
                }

                case None:
                    throw std::runtime_error("color is not empty but type is none");
            }
        }
    }
}

void ChessBoard::generateMovesWithForbidden(std::vector<Move>& legalMoves, bool mustDefend) {
    std::vector<Move> firstMoves;
    generateMoves(firstMoves, mustDefend);
    legalMoves.clear();

    for (const auto& move : firstMoves) {
        if (!repeatAfterMove(move)) {
            legalMoves.push_back(move);
        }
    }
    // 不做回退：若全部走法均为禁着，返回空列表
    // 由调用方（getBestMove 根节点）决定如何处理
}

bool ChessBoard::repeatAfterMove(const Move& move) {
    makeMoveAssumeLegal(move);
    int tmp_id = currTurnId_;

    if (lastMoveEaten_[tmp_id]) {
        undoMove();
        return false;
    }

    while (true) {
        if (tmp_id <= 1) break;
        if (lastMoveEaten_[tmp_id - 2]) break;
        tmp_id -= 2;
    }

    int repeatTime = 0;
    for (int id = currTurnId_; id >= tmp_id; id -= 2) {
        if (state_[id] == state_[currTurnId_]) {
            repeatTime++;
        }
    }

    undoMove();
    return repeatTime >= 3;
}

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
    if (mcolor == RED) {
        return my <= 4;
    } else {
        return my >= 5;
    }
}

colorType ChessBoard::currentColor() const {
    return currColor_;
}

colorType ChessBoard::oppColor() const {
    return currColor_ == BLACK ? RED : BLACK;
}

colorType ChessBoard::oppColor(colorType mcolor) {
    return mcolor == RED ? BLACK : RED;
}

bool ChessBoard::isMoveValid(const Move& move, bool mustDefend) {
    return isLegalMove(move, mustDefend);
}

bool ChessBoard::isLegalMove(const Move& move, bool mustDefend) {
    std::vector<Move> currLegalMoves;
    generateMoves(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

bool ChessBoard::isLegalMoveWithForbidden(const Move& move, bool mustDefend) {
    std::vector<Move> currLegalMoves;
    generateMovesWithForbidden(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

bool ChessBoard::isMoveValidWithForbidden(const Move& move, bool mustDefend) {
    return isLegalMoveWithForbidden(move, mustDefend);
}

bool ChessBoard::makeMoveAssumeLegal(const Move& move) {
    int nextPeaceTurn = continuePeaceTurn_[currTurnId_] + 1;
    std::vector<Grid> copyState = state_[currTurnId_];
    state_.push_back(copyState);
    currTurnId_++;

    auto& sourceGrid = state_[currTurnId_][xy2pos(move.source_x, move.source_y)];
    auto& targetGrid = state_[currTurnId_][xy2pos(move.target_x, move.target_y)];

    const Grid sourceBefore = sourceGrid;
    const Grid targetBefore = targetGrid;

    uint64_t newHash = currentHash_;

    if (sourceBefore.color != EMPTY) {
        newHash ^= zobristTable_[move.source_x][move.source_y][sourceBefore.type][sourceBefore.color];
    }
    if (targetBefore.color != EMPTY) {
        newHash ^= zobristTable_[move.target_x][move.target_y][targetBefore.type][targetBefore.color];
    }

    if (targetGrid.color == oppColor()) {
        nextPeaceTurn = 0;
        lastMoveEaten_.push_back(true);
    } else {
        lastMoveEaten_.push_back(false);
    }

    targetGrid.type = sourceGrid.type;
    targetGrid.color = sourceGrid.color;
    assert(sourceGrid.color == currColor_);

    newHash ^= zobristTable_[move.target_x][move.target_y][targetGrid.type][targetGrid.color];

    sourceGrid.color = EMPTY;
    sourceGrid.type = None;

    newHash ^= zobristSide_;

    continuePeaceTurn_.push_back(nextPeaceTurn);
    currColor_ = oppColor();

    currentHash_ = newHash;
    hashHistory_.push_back(currentHash_);
    return true;
}

bool ChessBoard::attacked(colorType color, int mx, int my) {
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state_[currTurnId_][xy2pos(x, y)];
            if (curGrid.color != color) continue;

            int dex = mx - x;
            int dey = my - y;
            switch (curGrid.type) {
                case King: {
                    if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my)) {
                        return true;
                    }
                    break;
                }

                case Assistant: {
                    if (inKingArea(mx, my, oppColor(color))) {
                        if (std::abs(dex) == 1 && std::abs(dey) == 1) return true;
                    }
                    break;
                }

                case Knight: {
                    if (std::abs(dex) == 2 && std::abs(dey) == 1) {
                        int fx = x + (dex > 0 ? 1 : -1);
                        int fy = y;
                        auto& fGrid = state_[currTurnId_][xy2pos(fx, fy)];
                        if (fGrid.color == EMPTY) return true;
                    } else if (std::abs(dex) == 1 && std::abs(dey) == 2) {
                        int fx = x;
                        int fy = y + (dey > 0 ? 1 : -1);
                        auto& fGrid = state_[currTurnId_][xy2pos(fx, fy)];
                        if (fGrid.color == EMPTY) return true;
                    }
                    break;
                }

                case Bishop: {
                    if (std::abs(dex) == 2 && std::abs(dey) == 2) {
                        int ex = x + dex / 2;
                        int ey = y + dey / 2;
                        auto& eGrid = state_[currTurnId_][xy2pos(ex, ey)];
                        if (eGrid.color == EMPTY) return true;
                    }
                    break;
                }

                case Rook: {
                    if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my)) {
                        return true;
                    }
                    break;
                }

                case Pawn: {
                    int forwardOne = (curGrid.color == RED ? 1 : -1);
                    bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5));
                    if (dex == 0 && dey == forwardOne) return true;
                    if (crossMidLine && std::abs(dex) == 1 && dey == 0) return true;
                    break;
                }

                case Cannon: {
                    if (inSameStraightLine(x, y, mx, my) && betweenNotEmptyNum(x, y, mx, my) == 1) {
                        return true;
                    }
                    break;
                }

                case None:
                    throw std::runtime_error("color is not empty but type is none");
            }
        }
    }
    return false;
}

bool ChessBoard::betweenIsEmpty(int sx, int sy, int ex, int ey) {
    assert(inSameStraightLine(sx, sy, ex, ey));
    int dex = ex > sx ? 1 : (ex == sx ? 0 : -1);
    int dey = ey > sy ? 1 : (ey == sy ? 0 : -1);
    int tx = sx + dex;
    int ty = sy + dey;

    while (inBoard(tx, ty) && !(tx == ex && ty == ey)) {
        auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
        if (tGrid.color != EMPTY) return false;
        tx += dex;
        ty += dey;
    }
    return true;
}

int ChessBoard::betweenNotEmptyNum(int sx, int sy, int ex, int ey) {
    assert(inSameStraightLine(sx, sy, ex, ey));
    int dex = ex > sx ? 1 : (ex == sx ? 0 : -1);
    int dey = ey > sy ? 1 : (ey == sy ? 0 : -1);
    int tx = sx + dex;
    int ty = sy + dey;
    int retNum = 0;

    while (inBoard(tx, ty) && !(tx == ex && ty == ey)) {
        auto& tGrid = state_[currTurnId_][xy2pos(tx, ty)];
        if (tGrid.color != EMPTY) retNum++;
        tx += dex;
        ty += dey;
    }
    return retNum;
}

bool ChessBoard::inSameLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx;
    int dey = ey - sy;
    return dex == dey || dex == -dey || dex == 0 || dey == 0;
}

bool ChessBoard::inSameStraightLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx;
    int dey = ey - sy;
    return dex == 0 || dey == 0;
}

bool ChessBoard::inSameObiqueLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx;
    int dey = ey - sy;
    return dex == dey || dex == -dey;
}

bool ChessBoard::isMyKingAttackedAfterMove(const Move& move) {
    colorType originColor = currColor_;
    makeMoveAssumeLegal(move);

    int kingX = -1;
    int kingY = -1;
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state_[currTurnId_][xy2pos(x, y)];
            if (curGrid.color == originColor && curGrid.type == King) {
                kingX = x;
                kingY = y;
                break;
            }
        }
    }

    bool myKingStillAttacked = attacked(oppColor(originColor), kingX, kingY);
    undoMove();
    assert(currColor_ == originColor);
    return myKingStillAttacked;
}

bool ChessBoard::isOppKingAttackedAfterMove(const Move& move) {
    colorType originColor = currColor_;
    makeMoveAssumeLegal(move);

    int kingX = -1;
    int kingY = -1;
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state_[currTurnId_][xy2pos(x, y)];
            if (curGrid.color == oppColor(originColor) && curGrid.type == King) {
                kingX = x;
                kingY = y;
                break;
            }
        }
    }

    bool oppKingBeingAttacked = attacked(originColor, kingX, kingY);
    undoMove();
    assert(currColor_ == originColor);
    return oppKingBeingAttacked;
}

bool ChessBoard::winAfterMove(const Move& move) {
    makeMoveAssumeLegal(move);
    std::vector<Move> oppLegalMoves;
    generateMoves(oppLegalMoves);
    undoMove();
    return oppLegalMoves.empty();
}

void ChessBoard::undoMove() {
    if (currTurnId_ == 0) return;
    state_.pop_back();
    currTurnId_--;
    currColor_ = oppColor();
    continuePeaceTurn_.pop_back();
    lastMoveEaten_.pop_back();

    hashHistory_.pop_back();
    currentHash_ = hashHistory_.back();
}

bool ChessBoard::exceedMaxPeaceState() const {
    return continuePeaceTurn_[currTurnId_] >= 59;
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
    return state_[currTurnId_][xy2pos(mx, my)];
}

void ChessBoard::printBoard() const {
    std::cout << "\nBoard\n";
    for (int y = 9; y >= 0; y--) {
        for (int x = 0; x <= 8; x++) {
            std::cout << stoneSym[state_[currTurnId_][xy2pos(x, y)].type] << " ";
        }
        std::cout << "\n";
    }
}

// =====================================================================
//  新增接口实现
// =====================================================================

uint64_t ChessBoard::computeHash() const {
    return currentHash_;
}

bool ChessBoard::findKing(colorType c, int& kx, int& ky) const {
    for (int x = 3; x <= 5; x++) {
        int yS = (c == RED) ? 0 : 7;
        int yE = (c == RED) ? 2 : 9;
        for (int y = yS; y <= yE; y++) {
            const Grid& g = state_[currTurnId_][xy2pos(x, y)];
            if (g.type == King && g.color == c) {
                kx = x; ky = y;
                return true;
            }
        }
    }
    kx = ky = -1;
    return false;
}

bool ChessBoard::isInCheck() const {
    int kx, ky;
    // 这里需要 const_cast 因为 attacked 不是 const（它用了 state_[currTurnId_]）
    // 但 attacked 实际上不修改状态，所以安全
    ChessBoard* self = const_cast<ChessBoard*>(this);
    if (!findKing(currColor_, kx, ky)) return false;
    return self->attacked(oppColor(currColor_), kx, ky);
}

bool ChessBoard::isCapture(const Move& mv) const {
    const Grid& tg = state_[currTurnId_][xy2pos(mv.target_x, mv.target_y)];
    return tg.color != EMPTY && tg.color != currColor_;
}

bool ChessBoard::lastMoveWasCapture() const {
    if (currTurnId_ == 0) return false;
    return lastMoveEaten_[currTurnId_];
}

int ChessBoard::countMajorPieces(colorType c) const {
    int cnt = 0;
    for (int pos = 0; pos < BOARDWIDTH * BOARDHEIGHT; pos++) {
        const Grid& g = state_[currTurnId_][pos];
        if (g.color == c && g.type != King && g.type != None) cnt++;
    }
    return cnt;
}

int ChessBoard::turnCount() const {
    return currTurnId_;
}

void ChessBoard::generateCaptures(std::vector<Move>& captures, bool mustDefend) {
    std::vector<Move> allMoves;
    generateMoves(allMoves, mustDefend);
    captures.clear();
    for (const auto& mv : allMoves) {
        if (isCapture(mv)) {
            captures.push_back(mv);
        }
    }
}

void ChessBoard::makeNullMove() {
    std::vector<Grid> copyState = state_[currTurnId_];
    state_.push_back(copyState);
    currTurnId_++;
    continuePeaceTurn_.push_back(continuePeaceTurn_[currTurnId_ - 1] + 1);
    lastMoveEaten_.push_back(false);
    currColor_ = oppColor();

    currentHash_ ^= zobristSide_;
    hashHistory_.push_back(currentHash_);
}

void ChessBoard::undoNullMove() {
    // 和 undoMove 完全一样
    undoMove();
}