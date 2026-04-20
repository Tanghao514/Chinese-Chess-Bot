#ifndef CHESSBOARD_H
#define CHESSBOARD_H
#include "Common.h"
#include <vector>
#include <cstdint>
#include <cstring>

// Eye-style rollback structure: saves Zobrist and ucRepHash entry per move
struct EyeRollback {
    EyeZobrist zobr;            // Zobrist state before the move
    uint16_t wmv;               // Eye-encoded move
    int8_t   chkChs;            // CheckedBy result after the move (0=none, >0=single, CHECK_MULTI=multi)
    int8_t   cptDrw;            // >0: captured piece-ID; <=0: draw countdown
};

// 撤销信息：保存一步走法的回滚所需数据 (kept for compatibility)
struct UndoInfo {
    Move move;              // 走法本身
    Grid moving;            // 起点移动的棋子
    Grid captured;          // 目标格原有棋子
    uint64_t prevHash;      // 走之前的 hash
    int prevPeaceTurn;      // 走之前的和棋步计数
    bool wasCapture;        // 本步是否吃子
    bool isNullMove;        // 是否为 null move

    // Eye-style extra info
    int pcCaptured;         // captured piece-ID (0 if none)
    int pcMoved;            // moved piece-ID
};

class ChessBoard {
private:
    Grid board_[BOARDWIDTH][BOARDHEIGHT]; // 单盘数组 (compatibility layer)
    colorType currColor_;
    uint64_t hash_;             // 增量维护的 Zobrist hash (legacy)
    int turnId_;                // 当前手数
    int peaceTurn_;             // 连续和棋步计数

    std::vector<UndoInfo> undoStack_;       // 撤销栈
    std::vector<Move> moveHistory_;         // 走法历史（供 MoveBook）
    std::vector<uint64_t> hashHistory_;     // 每手 hash（供 repeat 检测）
    std::vector<bool> captureHistory_;      // 每手是否吃子（供 repeat 边界）

    // Zobrist 哈希相关 (legacy 64-bit)
    static bool zobristInited_;
    static uint64_t zobristTable_[BOARDWIDTH][BOARDHEIGHT][8][3]; // [x][y][pieceType][color]
    static uint64_t zobristSide_; // 走棋方哈希
    static void initZobrist();

    // 从头计算 hash（仅用于初始化）
    uint64_t fullComputeHash() const;

    // =========================================================
    //  Eye-style data fields
    // =========================================================
    int ucpcSquares_[256];              // sq → piece-ID (0 if empty)
    int ucsqPieces_[EYE_MAX_PIECES];   // pc → sq (0 if off board)
    int sdPlayer_;                      // 0=Red, 1=Black
    int nDistance_;                      // search distance (managed by search)
    int nMoveNum_;                      // total half-moves played
    uint8_t ucRepHash_[REP_HASH_SIZE];  // repetition hash table
    EyeZobrist zobr_;                   // Eye-style triple Zobrist

    // Eye Zobrist random tables: [14][256]
    // Index: pt for red=0..6, pt+7 for black → total 14 entries per square
    static bool eyeZobristInited_;
    static EyeZobrist eyeZobrTable_[14][256];
    static EyeZobrist eyeZobrPlayer_;   // XOR when changing side
    static void initEyeZobrist();

    // Eye rollback list (parallel to undoStack_)
    static constexpr int MAX_MOVE_NUM = 512;
    EyeRollback rbsList_[MAX_MOVE_NUM]; // index by nMoveNum_

    // --- Eye-style piece manipulation (internal) ---
    void eyeAddPiece(int sq, int pc, bool bDel = false);
    int  eyeMovePiece(int eyeMv);
    void eyeUndoMovePiece(int eyeMv, int pcCaptured);

    // Save/restore Eye Zobrist for rollback
    void eyeSaveStatus();
    void eyeRollback();
    void eyeChangeSide();

    // Sync board_[][] from Eye dual tables (debug/init only)
    void syncBoardFromEye();

public:
    ChessBoard();

    int repStatus(int nRecur = 1) const;
    void resetBoard();
    void generateMoves(std::vector<Move>& legalMoves, bool mustDefend = true);
    void generateCaptures(std::vector<Move>& captures, bool mustDefend = true);
    int generateMovesFast(Move outMoves[], bool mustDefend = true);
    int generateCapturesFast(Move outMoves[], bool mustDefend = true);
    void generateMovesWithForbidden(std::vector<Move>& legalMoves, bool mustDefend = true);
    bool repeatAfterMove(const Move& move);

    static bool inBoard(int mx, int my);
    static bool inKingArea(int mx, int my, colorType mcolor);
    static bool inColorArea(int mx, int my, colorType mcolor);

    colorType currentColor() const;
    colorType oppColor() const;
    static colorType oppColor(colorType mcolor);
    bool kingAttacked(colorType side) const;

    bool isMoveValid(const Move& move, bool mustDefend = true);
    bool isLegalMove(const Move& move, bool mustDefend = true);
    bool isLegalMoveWithForbidden(const Move& move, bool mustDefend = true);
    bool isMoveValidWithForbidden(const Move& move, bool mustDefend = true);

    bool makeMoveAssumeLegal(const Move& move);
    bool makeMove(const Move& move);

    bool attacked(colorType color, int mx, int my) const;
    bool betweenIsEmpty(int sx, int sy, int ex, int ey) const;
    int betweenNotEmptyNum(int sx, int sy, int ex, int ey) const;

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

    // 走法历史（供开局库前缀匹配）
    const std::vector<Move>& moveHistory() const { return moveHistory_; }

    void undoMove();
    bool exceedMaxPeaceState() const;

    static int xy2pos(int mx, int my);
    static int pos2x(int mpos);
    static int pos2y(int mpos);

    Grid getGridAt(int mx, int my) const;
    void printBoard() const;

    // Zobrist 哈希（增量维护，O(1) 返回）
    uint64_t computeHash() const;

    // 做空着（null move）：仅交换走棋方，不移动棋子
    void makeNullMove();
    void undoNullMove();

    bool makeMoveFast(const Move& move);
    void undoMoveFast();
    void makeNullMoveFast();
    void undoNullMoveFast();

    // =========================================================
    //  Eye-style public interface
    // =========================================================

    // Eye dual tables access
    int  pieceAt(int sq) const { return ucpcSquares_[sq]; }
    int  squareOf(int pc) const { return ucsqPieces_[pc]; }
    int  sidePlayer() const { return sdPlayer_; }

    // Eye-style check detection: returns 0 (no check), piece-ID (single check),
    // or CHECK_MULTI (double+ check). If bLazy, returns CHECK_MULTI as soon as
    // any check is found (used for quick legality test in MakeMove).
    int chasedBy(int eyeMv) const;
    int  checkedBy(bool bLazy = false) const;

    // Eye-style repetition: uses ucRepHash_ for fast detection
    int  eyeRepStatus(int nRecur = 1) const;

    // Draw value from perspective of current side
    int  drawValue() const;
    // Repetition value: returns score given repStatus
    int  repValue(int nRepStatus) const;

    // Eye Zobrist key (for TT indexing)
    uint32_t eyeZobristKey() const { return zobr_.dwKey; }
    uint32_t eyeZobristLock0() const { return zobr_.dwLock0; }
    uint32_t eyeZobristLock1() const { return zobr_.dwLock1; }
    const EyeZobrist& getZobrist() const { return zobr_; }

    // Eye nDistance (search distance from root)
    int  getDistance() const { return nDistance_; }

    // Eye NullOkay/NullSafe: check if side has enough material for null move
    bool nullOkay() const;
    bool nullSafe() const;
    const EyeZobrist& eyeZobrist() const { return zobr_; }

    // Distance/MoveNum (managed externally by search)
    int  distance() const { return nDistance_; }
    void setDistance(int d) { nDistance_ = d; }
    int  moveNum() const { return nMoveNum_; }
};
#endif
