#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "Common.h"
#include <vector>
#include <cstdint>

class ChessBoard {
private:
    std::vector<std::vector<Grid>> state_;
    int currTurnId_;
    colorType currColor_;
    std::vector<int> continuePeaceTurn_;
    std::vector<int> lastMoveEaten_;

    // Zobrist 哈希相关
    static bool zobristInited_;
    static uint64_t zobristTable_[BOARDWIDTH][BOARDHEIGHT][8][3]; // [x][y][pieceType][color]
    static uint64_t zobristSide_; // 走棋方哈希
    static void initZobrist();

public:
    ChessBoard();

    void resetBoard();
    void generateMoves(std::vector<Move>& legalMoves, bool mustDefend = true);
    void generateCaptures(std::vector<Move>& captures, bool mustDefend = true);
    void generateMovesWithForbidden(std::vector<Move>& legalMoves, bool mustDefend = true);
    bool repeatAfterMove(const Move& move);

    static bool inBoard(int mx, int my);
    static bool inKingArea(int mx, int my, colorType mcolor);
    static bool inColorArea(int mx, int my, colorType mcolor);

    colorType currentColor() const;
    colorType oppColor() const;
    static colorType oppColor(colorType mcolor);

    bool isMoveValid(const Move& move, bool mustDefend = true);
    bool isLegalMove(const Move& move, bool mustDefend = true);
    bool isLegalMoveWithForbidden(const Move& move, bool mustDefend = true);
    bool isMoveValidWithForbidden(const Move& move, bool mustDefend = true);

    bool makeMoveAssumeLegal(const Move& move);

    bool attacked(colorType color, int mx, int my);
    bool betweenIsEmpty(int sx, int sy, int ex, int ey);
    int betweenNotEmptyNum(int sx, int sy, int ex, int ey);

    static bool inSameLine(int sx, int sy, int ex, int ey);
    static bool inSameStraightLine(int sx, int sy, int ex, int ey);
    static bool inSameObiqueLine(int sx, int sy, int ex, int ey);

    bool isMyKingAttackedAfterMove(const Move& move);
    bool isOppKingAttackedAfterMove(const Move& move);
    bool winAfterMove(const Move& move);

    // 当前走棋方是否被将军
    bool isInCheck() const;
    // 找到指定颜色将帅的位置
    bool findKing(colorType c, int& kx, int& ky) const;
    // 判断走法是否为吃子步
    bool isCapture(const Move& mv) const;
    // 最后一步是否为吃子
    bool lastMoveWasCapture() const;
    // 棋盘上某方的非将帅子力总数
    int countMajorPieces(colorType c) const;
    // 当前总手数（半回合），用于开局规则判断
    int turnCount() const;

    void undoMove();
    bool exceedMaxPeaceState() const;

    static int xy2pos(int mx, int my);
    static int pos2x(int mpos);
    static int pos2y(int mpos);

    Grid getGridAt(int mx, int my) const;
    void printBoard() const;

    // Zobrist 哈希
    uint64_t computeHash() const;

    // 做空着（null move）：仅交换走棋方，不移动棋子
    void makeNullMove();
    void undoNullMove();
};

#endif