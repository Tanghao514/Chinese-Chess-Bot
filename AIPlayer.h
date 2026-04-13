#ifndef AIPLAYER_H
#define AIPLAYER_H

#include "ChessBoard.h"
#include <chrono>
#include <cstdint>
#include <vector>

// 置换表条目
enum TTFlag : uint8_t {
    TT_EXACT      = 0,
    TT_LOWERBOUND = 1,
    TT_UPPERBOUND = 2
};

struct TTEntry {
    uint64_t key;
    int16_t  depth;
    int32_t  score;
    Move     bestMove;
    TTFlag   flag;
    uint8_t  age;

    TTEntry()
        : key(0), depth(-1), score(0), bestMove(), flag(TT_EXACT), age(0) {}
};

// 中国象棋传统搜索引擎
class AIPlayer {
public:
    AIPlayer();
    Move getBestMove(ChessBoard& board) const;

private:
    static constexpr int MAX_DEPTH = 64;
    static constexpr int DEFAULT_DEPTH = 14;
    static constexpr int INF_SCORE = 1000000;
    static constexpr int MATE_SCORE = 900000;
    static constexpr int DRAW_SCORE = 0;

    static constexpr int HARD_TIME_BASE_MS = 860;
    static constexpr int HARD_TIME_MIN_MS = 780;
    static constexpr int HARD_TIME_MAX_MS = 900;
    static constexpr int SOFT_TIME_BASE_MS = 540;
    static constexpr int SOFT_TIME_MIN_MS = 420;
    static constexpr int SOFT_TIME_MAX_MS = 680;
    static constexpr int TIME_CHECK_MASK = 255;

    static constexpr int TT_SIZE_BITS = 20;
    static constexpr int TT_SIZE = 1 << TT_SIZE_BITS;
    static constexpr int TT_MASK = TT_SIZE - 1;

    static constexpr int NULL_MOVE_MIN_DEPTH = 3;
    static constexpr int LMR_MIN_DEPTH = 3;
    static constexpr int LMR_MIN_MOVES = 3;

    static constexpr int FUTILITY_MARGIN_1 = 150;
    static constexpr int FUTILITY_MARGIN_2 = 280;
    static constexpr int FUTILITY_MARGIN_3 = 420;
    static constexpr int RAZOR_MARGIN = 260;

    static constexpr int ASP_WINDOW = 28;
    static constexpr int MAX_ASP_RESEARCH = 3;
    static constexpr int MAX_KILLERS = 2;
    static constexpr int MAX_QS_DEPTH = 8;
    static constexpr int MAX_QS_CHECKS = 6;

    struct KingDangerInfo {
        int kingX = -1;
        int kingY = -1;
        int guardCount = 0;
        int bishopCount = 0;
        int palaceDefenders = 0;
        int frontOpen = 0;
        int centerFileOpen = 0;
        int sideFilePressure = 0;
        int currentSquareDanger = 0;
        int safePalaceSquares = 0;
        int attackedPalaceSquares = 0;
        int escapeImprovement = 0;
        int bottomCannonThreat = 0;
        int bottomCannonSide = 0;
        int rookCannonThreat = 0;
        int directRookPressure = 0;
        int directCannonPressure = 0;
        int horseThreat = 0;
        int wocaoThreat = 0;
        int shijiaoThreat = 0;
        int doubleCheckLanes = 0;
        int pawnStorm = 0;
        int totalDanger = 0;
    };

    struct RootMoveInfo {
        Move move;
        int staticScore = 0;
        int lastScore = 0;
        int searchedDepth = 0;
    };

    // 可变搜索状态
    mutable std::vector<TTEntry> tt_;
    mutable Move killers_[MAX_DEPTH][MAX_KILLERS];
    mutable int history_[2][BOARDWIDTH * BOARDHEIGHT][BOARDWIDTH * BOARDHEIGHT];
    mutable Move counterMoves_[2][BOARDWIDTH * BOARDHEIGHT][BOARDWIDTH * BOARDHEIGHT];
    mutable std::chrono::steady_clock::time_point searchStart_;
    mutable bool timeUp_;
    mutable int nodesSearched_;
    mutable uint8_t searchAge_;
    mutable int allocatedTimeMs_;
    mutable int softTimeMs_;
    mutable int hardTimeMs_;
    mutable int lastIterationMs_;
    mutable int completedDepth_;
    mutable int bestMoveChanges_;
    mutable Move prevMove_[MAX_DEPTH];

    // 核心搜索
    int alphaBeta(ChessBoard& board, int depth, int alpha, int beta,
                  int ply, bool allowNull, bool cutNode) const;
    int quiescence(ChessBoard& board, int alpha, int beta,
                   int ply, int qsDepth) const;
    bool checkTime() const;
    bool softTimeUp() const;
    int elapsedMs() const;
    void allocateTimeBudget(const ChessBoard& board, int rootMoveCount) const;
    bool shouldStopForNextDepth(int depth, int rootMoveCount) const;

    // 评估函数（保持结构化）
    int evaluate(const ChessBoard& board) const;
    int evalMaterial(const ChessBoard& board, colorType side, int phase) const;
    int evalPosition(const ChessBoard& board, colorType side) const;
    int evalKingSafety(const ChessBoard& board, colorType side) const;
    int evalMobility(const ChessBoard& board, colorType side, int phase) const;
    int evalPawnStructure(const ChessBoard& board, colorType side) const;
    int evalPawnAdvancement(const ChessBoard& board, colorType side, int phase) const;
    int evalAttackPressure(const ChessBoard& board, colorType side) const;
    int evalDevelopment(const ChessBoard& board, colorType side, int phase) const;
    int evalPieceActivity(const ChessBoard& board, colorType side, int phase) const;

    KingDangerInfo analyzeKingDanger(const ChessBoard& board, colorType side) const;
    int scoreSquareDanger(const ChessBoard& board, colorType side, int x, int y) const;

    // 辅助：统计对方深入我方半场的大子数
    int countEnemyDeepMajors(const ChessBoard& board, colorType side) const;

    static int pieceValueMg(stoneType t);
    static int pieceValueEg(stoneType t);
    static int pieceBaseValue(stoneType t);
    static int positionValue(stoneType t, colorType c, int x, int y);
    static int endgamePhase(const ChessBoard& board);

    // 走法排序
    void orderMoves(const ChessBoard& board, std::vector<Move>& moves,
                    int ply, const Move& ttMove) const;
    int scoreMoveForOrdering(const ChessBoard& board, const Move& move,
                             int ply, const Move& ttMove,
                             int oppKingX, int oppKingY,
                             int phase, bool sideInCheck) const;
    void orderRootMoves(const ChessBoard& board, std::vector<RootMoveInfo>& moves,
                        const Move& pvMove, int currentDanger) const;
    int scoreRootMove(const ChessBoard& board, const RootMoveInfo& moveInfo,
                      const Move& pvMove, int currentDanger, int phase) const;
    int scoreQSearchCheckingMove(const ChessBoard& board, const Move& move,
                                 int qsDepth, int standPat, int alpha,
                                 int oppDangerLevel) const;

    // 置换表操作
    void ttStore(uint64_t key, int depth, int score, TTFlag flag,
                 const Move& bestMove, int ply) const;
    bool ttProbe(uint64_t key, int depth, int alpha, int beta,
                 int& score, Move& ttMove, int ply) const;
    int scoreToTT(int score, int ply) const;
    int scoreFromTT(int score, int ply) const;

    // Killer / History / Counter-move
    void updateKiller(int ply, const Move& mv) const;
    void updateHistory(colorType side, const Move& mv, int depth) const;
    void updateCounterMove(colorType side, const Move& prev, const Move& cur) const;
    bool isKiller(int ply, const Move& mv) const;
    bool isCounterMove(colorType side, const Move& prev, const Move& mv) const;

    void initSearchState() const;
};

#endif
