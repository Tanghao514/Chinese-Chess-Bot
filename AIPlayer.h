#ifndef AIPLAYER_H
#define AIPLAYER_H
#include "ChessBoard.h"
#include <chrono>
#include <cstdint>
#include <vector>

struct TTEntry {
    uint32_t dwLock0;
    uint16_t wmvPacked;
    uint8_t  ucAlphaDepth;
    uint8_t  ucBetaDepth;
    int16_t  svlAlpha;
    int16_t  svlBeta;
    uint32_t dwLock1;

    TTEntry()
        : dwLock0(0),
          wmvPacked(0),
          ucAlphaDepth(0),
          ucBetaDepth(0),
          svlAlpha(0),
          svlBeta(0),
          dwLock1(0) {}
};

class AIPlayer {
public:
    AIPlayer();
    Move getBestMove(ChessBoard& board) const;

private:
    // ============================================================
    // Core constants
    // ============================================================
    static constexpr int MAX_DEPTH = 64;
    static constexpr int DEFAULT_DEPTH = 64;

    static constexpr int INF_SCORE  = 1000000;
    static constexpr int MATE_SCORE = 900000;
    static constexpr int WIN_SCORE  = MATE_SCORE - 200;
    static constexpr int DRAW_VALUE = 20;
    static constexpr int BAN_SCORE  = MATE_SCORE - 100;

    static constexpr int TT_SIZE_BITS = 20;
    static constexpr int TT_SIZE      = 1 << TT_SIZE_BITS;
    static constexpr int TT_MASK      = TT_SIZE - 1;
    static constexpr int HASH_LAYERS  = 2;

    static constexpr int NULL_DEPTH       = 2;
    static constexpr int UNCHANGED_DEPTH  = 4;
    static constexpr int IID_REDUCE       = 2;
    static constexpr int DROPDOWN_VALUE   = 20;
    static constexpr int MAX_KILLERS      = 4;
    static constexpr int MAX_QS_DEPTH     = 8;
    static constexpr int TIME_CHECK_MASK  = 4095;

    static constexpr int HARD_TIME_BASE_MS = 950;
    static constexpr int HARD_TIME_MIN_MS  = 900;
    static constexpr int HARD_TIME_MAX_MS  = 980;
    static constexpr int SOFT_TIME_BASE_MS = 700;
    static constexpr int SOFT_TIME_MIN_MS  = 600;
    static constexpr int SOFT_TIME_MAX_MS  = 800;

    // Eye hash flags
    static constexpr int HASH_ALPHA = 1;
    static constexpr int HASH_BETA  = 2;
    static constexpr int HASH_PV    = HASH_ALPHA | HASH_BETA;

    struct RootMoveInfo {
        Move move;
        int  lastScore;
        int  searchedDepth;
        int  rootOrder;

        RootMoveInfo()
            : move(),
              lastScore(0),
              searchedDepth(0),
              rootOrder(1) {}
    };

    // ============================================================
    // Mutable search state
    // ============================================================
    mutable std::vector<TTEntry> tt_;
    mutable Move killers_[MAX_DEPTH][MAX_KILLERS];
    mutable int history_[2][BOARDWIDTH * BOARDHEIGHT][BOARDWIDTH * BOARDHEIGHT];
    mutable Move counterMoves_[2][BOARDWIDTH * BOARDHEIGHT][BOARDWIDTH * BOARDHEIGHT];

    mutable std::chrono::steady_clock::time_point searchStart_;
    mutable bool timeUp_;
    mutable int nodesSearched_;
    mutable int allocatedTimeMs_;
    mutable int softTimeMs_;
    mutable int hardTimeMs_;
    mutable int lastIterationMs_;
    mutable int completedDepth_;
    mutable int bestMoveChanges_;
    mutable Move prevMove_[MAX_DEPTH];

    // ============================================================
    // Search main line (Eye-style split)
    // ============================================================
    int quiescence(ChessBoard& board, int alpha, int beta,
                   int ply, int qsDepth) const;

    int searchCut(ChessBoard& board, int beta, int depth,
                  int ply, bool noNull) const;

    int searchPV(ChessBoard& board, int alpha, int beta, int depth,
                 int ply, std::vector<Move>& pvLine) const;

    int searchPVFast(ChessBoard& board, int alpha, int beta, int depth,
                 int ply, Move pvBuf[], int& pvLen) const;

    bool searchUnique(ChessBoard& board,
                      const std::vector<RootMoveInfo>& rootMoves,
                      const Move& bestMove,
                      int beta,
                      int depth) const;

    int harmlessPruning(const ChessBoard& board, int beta, int ply) const;

    // ============================================================
    // Evaluation
    // ============================================================
    int evaluate(const ChessBoard& board, int vlAlpha, int vlBeta) const;
    int evalPosition(const ChessBoard& board, colorType side) const;

    int advisorShapeScore(const ChessBoard& board, colorType side) const;
    int stringHoldScore(const ChessBoard& board, colorType side) const;
    int rookMobilityScore(const ChessBoard& board, colorType side) const;
    int knightTrapScore(const ChessBoard& board, colorType side) const;

    static int endgamePhase(const ChessBoard& board);

    // ============================================================
    // Move ordering
    // ============================================================
    void orderMoves(const ChessBoard& board,
                    std::vector<Move>& moves,
                    int ply,
                    const Move& ttMove) const;
    
    void orderMoveArray(const ChessBoard& board,
                    Move moves[],
                    int count,
                    int ply,
                    const Move& ttMove) const;

    void orderRootMoves(const ChessBoard& board,
                        std::vector<RootMoveInfo>& moves,
                        const Move& pvMove) const;

    void updateRootOrder(std::vector<RootMoveInfo>& moves,
                         const Move& bestMove) const;

    // ============================================================
    // TT / score conversion
    // ============================================================
    void ttStore(const ChessBoard& board, int nFlag, int vl,
                 int nDepth, const Move& mv) const;

    int ttProbe(const ChessBoard& board, int vlAlpha, int vlBeta,
                int nDepth, bool bNoNull, Move& ttMove) const;

    int scoreToTT(const ChessBoard& board, int vl) const;
    int scoreFromTT(const ChessBoard& board, int vl) const;

    static uint16_t packMove(const Move& mv);
    static Move unpackMove(uint16_t wmv);

    // ============================================================
    // History / killer / counter
    // ============================================================
    void updateKiller(int ply, const Move& mv) const;
    void updateHistory(colorType side, const Move& mv, int depth) const;
    void updateCounterMove(colorType side, const Move& prev, const Move& cur) const;

    bool isKiller(int ply, const Move& mv) const;
    bool isCounterMove(colorType side, const Move& prev, const Move& mv) const;

    // ============================================================
    // Time / root control
    // ============================================================
    bool checkTime() const;
    bool softTimeUp() const;
    int elapsedMs() const;

    void allocateTimeBudget(const ChessBoard& board, int rootMoveCount) const;
    bool shouldStopForNextDepth(int depth, int rootMoveCount) const;

    int drawValue(int ply) const;
    int repValue(int repStatus, int ply) const;

    void initSearchState() const;
};
#endif
