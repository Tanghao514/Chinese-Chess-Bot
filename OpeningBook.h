#ifndef OPENINGBOOK_H
#define OPENINGBOOK_H

#include "ChessBoard.h"
#include "TacticalRules.h"

#include <vector>

namespace OpeningBook {

struct Candidate {
    Move move;
    int priority = 0;
    const char* reason = nullptr;
};

inline bool hasPiece(const ChessBoard& board, int x, int y, colorType color, stoneType type) {
    if (!ChessBoard::inBoard(x, y)) {
        return false;
    }
    const Grid g = board.getGridAt(x, y);
    return g.color == color && g.type == type;
}

inline Move findMove(const std::vector<Move>& legalMoves, int sx, int sy, int tx, int ty) {
    for (const Move& move : legalMoves) {
        if (move.source_x == sx && move.source_y == sy &&
            move.target_x == tx && move.target_y == ty) {
            return move;
        }
    }
    return Move();
}

inline bool isOpeningStage(const ChessBoard& board) {
    return board.turnCount() <= 14 &&
           board.countMajorPieces(RED) + board.countMajorPieces(BLACK) >= 24;
}

inline bool hasCentralCannon(const ChessBoard& board, colorType side) {
    for (int y = 0; y < BOARDHEIGHT; ++y) {
        const Grid g = board.getGridAt(4, y);
        if (g.color == side && g.type == Cannon && TacticalRules::advanceOf(side, y) >= 2) {
            return true;
        }
    }
    return false;
}

inline int centralCannonRank(colorType side) {
    return side == RED ? 2 : 7;
}

inline int middlePawnRank(colorType side) {
    return side == RED ? 3 : 6;
}

inline int riverRetreatRank(colorType side) {
    return side == RED ? 4 : 5;
}

inline int frontDir(colorType side) {
    return side == RED ? 1 : -1;
}

inline Move centralCannonMove(const std::vector<Move>& legalMoves, colorType side, int preferredFlank) {
    const int y = centralCannonRank(side);
    const int preferredSource = preferredFlank == 0 ? 1 : 7;
    Move move = findMove(legalMoves, preferredSource, y, 4, y);
    if (!move.isInvalid()) {
        return move;
    }
    move = findMove(legalMoves, preferredSource == 1 ? 7 : 1, y, 4, y);
    if (!move.isInvalid()) {
        return move;
    }
    return Move();
}

inline Move normalHorseMove(const std::vector<Move>& legalMoves, colorType side, int flank) {
    const int homeY = TacticalRules::homeRank(side);
    const int targetY = side == RED ? 2 : 7;
    const int sourceX = flank == 0 ? 1 : 7;
    const int targetX = flank == 0 ? 2 : 6;
    Move move = findMove(legalMoves, sourceX, homeY, targetX, targetY);
    if (!move.isInvalid()) {
        return move;
    }
    return Move();
}

inline Move anyNormalHorseMove(const std::vector<Move>& legalMoves, colorType side, int preferredFlank) {
    Move move = normalHorseMove(legalMoves, side, preferredFlank);
    if (!move.isInvalid()) {
        return move;
    }
    move = normalHorseMove(legalMoves, side, preferredFlank == 0 ? 1 : 0);
    if (!move.isInvalid()) {
        return move;
    }
    return Move();
}

inline Move normalRookMove(const std::vector<Move>& legalMoves, colorType side, int flank) {
    const int homeY = TacticalRules::homeRank(side);
    const int forwardY = homeY + frontDir(side);
    if (flank == 0) {
        Move horizontal = findMove(legalMoves, 0, homeY, 1, homeY);
        if (!horizontal.isInvalid()) {
            return horizontal;
        }
        return findMove(legalMoves, 0, homeY, 0, forwardY);
    }

    Move horizontal = findMove(legalMoves, 8, homeY, 7, homeY);
    if (!horizontal.isInvalid()) {
        return horizontal;
    }
    return findMove(legalMoves, 8, homeY, 8, forwardY);
}

inline Move selectRookDevelopment(const std::vector<Move>& legalMoves, colorType side, int preferredFlank) {
    Move move = normalRookMove(legalMoves, side, preferredFlank);
    if (!move.isInvalid()) {
        return move;
    }
    return normalRookMove(legalMoves, side, preferredFlank == 0 ? 1 : 0);
}

inline Move sanQiCannonMove(const std::vector<Move>& legalMoves, colorType side, int flank) {
    const int y = centralCannonRank(side);
    if (flank == 0) {
        return findMove(legalMoves, 1, y, 2, y);
    }
    return findMove(legalMoves, 7, y, 6, y);
}

inline Move shiJiaoCannonMove(const std::vector<Move>& legalMoves, colorType side, int flank) {
    const int y = centralCannonRank(side);
    if (flank == 0) {
        return findMove(legalMoves, 1, y, 3, y);
    }
    return findMove(legalMoves, 7, y, 5, y);
}

inline Candidate makeCandidate(const Move& move, int priority, const char* reason) {
    Candidate candidate;
    candidate.move = move;
    candidate.priority = priority;
    candidate.reason = reason;
    return candidate;
}

inline int scoreCandidate(ChessBoard& board, const Candidate& candidate) {
    if (candidate.move.isInvalid()) {
        return -1000000000;
    }

    const TacticalRules::MoveTacticalInfo tactical = TacticalRules::analyzeMove(board, candidate.move);
    const Grid target = board.getGridAt(candidate.move.target_x, candidate.move.target_y);
    int score = candidate.priority;
    score += tactical.activityDelta * 10;
    score += tactical.destinationExchangeScore * 2;
    score += tactical.destinationWeightedNet - tactical.sourceWeightedNet;
    if (tactical.improvesByRunning) {
        score += 80;
    }
    if (tactical.canRootSafely) {
        score += 60;
    }
    if (tactical.prefersTrade) {
        score += 70;
    }
    if (target.color != EMPTY) {
        score += TacticalRules::pieceValue(target.type) / 3;
    }
    if (tactical.destinationExchangeScore < -120) {
        score -= 320;
    } else if (tactical.destinationExchangeScore < -40) {
        score -= 120;
    }
    return score;
}

inline Candidate bestCandidate(ChessBoard& board, const std::vector<Candidate>& candidates) {
    Candidate best;
    int bestScore = -1000000000;
    for (const Candidate& candidate : candidates) {
        const int score = scoreCandidate(board, candidate);
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }
    return best;
}

inline Candidate buildRedRule(ChessBoard& board, const std::vector<Move>& legalMoves) {
    std::vector<Candidate> candidates;
    const int weakFlank = TacticalRules::weakerFlank(board, BLACK) >= 0 ?
                          TacticalRules::weakerFlank(board, BLACK) : 0;

    if (!hasCentralCannon(board, RED)) {
        const Move move = centralCannonMove(legalMoves, RED, weakFlank);
        if (!move.isInvalid()) {
            candidates.push_back(makeCandidate(move, 960, "红方优先架中炮"));
        }
    }

    if (hasPiece(board, 4, 2, RED, Cannon) && hasPiece(board, 4, 6, BLACK, Pawn)) {
        const TacticalRules::RootInfo middlePawnRoot = TacticalRules::analyzeRoot(board, 4, 6);
        if (!middlePawnRoot.hasRoot || middlePawnRoot.exchangeScore < 0 || middlePawnRoot.weightedNet < -20) {
            const Move move = findMove(legalMoves, 4, 2, 4, 6);
            if (!move.isInvalid()) {
                candidates.push_back(makeCandidate(move, 930, "中兵无人稳根时直接打中兵"));
            }
        }
    }

    if (hasPiece(board, 4, 6, RED, Cannon)) {
        const Move retreat = findMove(legalMoves, 4, 6, 4, 4);
        if (!retreat.isInvalid()) {
            candidates.push_back(makeCandidate(retreat, 900, "打中兵后退炮保空头炮"));
        }
    }

    if (hasCentralCannon(board, RED)) {
        const TacticalRules::RootInfo middlePawnRoot = hasPiece(board, 4, 6, BLACK, Pawn) ?
            TacticalRules::analyzeRoot(board, 4, 6) : TacticalRules::RootInfo();
        if (!hasPiece(board, 4, 6, BLACK, Pawn) || middlePawnRoot.hasRoot || middlePawnRoot.exchangeScore >= 0) {
            const Move horse = anyNormalHorseMove(legalMoves, RED, weakFlank);
            if (!horse.isInvalid()) {
                candidates.push_back(makeCandidate(horse, 860, "中兵有人保时继续跳这一侧正马"));
            }
        }
    }

    const Move horse = anyNormalHorseMove(legalMoves, RED, weakFlank);
    if (!horse.isInvalid()) {
        candidates.push_back(makeCandidate(horse, 820, "开局优先跳正马"));
    }

    const Move rook = selectRookDevelopment(legalMoves, RED, weakFlank);
    if (!rook.isInvalid()) {
        candidates.push_back(makeCandidate(rook, 760, "马后动车，且只平一步或进一步"));
    }

    return bestCandidate(board, candidates);
}

inline Candidate buildBlackRule(ChessBoard& board, const std::vector<Move>& legalMoves) {
    std::vector<Candidate> candidates;
    const int weakFlank = TacticalRules::weakerFlank(board, RED) >= 0 ?
                          TacticalRules::weakerFlank(board, RED) : 1;

    const bool redCentralCannon = hasCentralCannon(board, RED);
    if (redCentralCannon && hasPiece(board, 4, 6, BLACK, Pawn)) {
        const TacticalRules::RootInfo middlePawnRoot = TacticalRules::analyzeRoot(board, 4, 6);
        if (!middlePawnRoot.hasRoot || middlePawnRoot.weightedNet < 20) {
            Move horse = anyNormalHorseMove(legalMoves, BLACK, weakFlank);
            if (!horse.isInvalid()) {
                candidates.push_back(makeCandidate(horse, 980, "黑方先跳马给中兵生根"));
            }
        }
    }

    if (!hasCentralCannon(board, BLACK)) {
        const Move move = centralCannonMove(legalMoves, BLACK, weakFlank);
        if (!move.isInvalid()) {
            candidates.push_back(makeCandidate(move, 900, "黑方常型优先中炮"));
        }
    }

    if (hasPiece(board, 4, 7, BLACK, Cannon) && hasPiece(board, 4, 3, RED, Pawn)) {
        const TacticalRules::RootInfo middlePawnRoot = TacticalRules::analyzeRoot(board, 4, 3);
        if (!middlePawnRoot.hasRoot || middlePawnRoot.exchangeScore < 0 || middlePawnRoot.weightedNet < -20) {
            const Move move = findMove(legalMoves, 4, 7, 4, 3);
            if (!move.isInvalid()) {
                candidates.push_back(makeCandidate(move, 880, "黑方中炮可直接打红中兵"));
            }
        }
    }

    if (hasPiece(board, 4, 3, BLACK, Cannon)) {
        const Move retreat = findMove(legalMoves, 4, 3, 4, 5);
        if (!retreat.isInvalid()) {
            candidates.push_back(makeCandidate(retreat, 850, "黑方打中兵后退炮稳空头炮"));
        }
    }

    {
        Move move = shiJiaoCannonMove(legalMoves, BLACK, weakFlank);
        if (!move.isInvalid()) {
            candidates.push_back(makeCandidate(move, 800, "黑方士角炮以稳阵并给马生根"));
        }
    }
    {
        Move move = sanQiCannonMove(legalMoves, BLACK, weakFlank);
        if (!move.isInvalid()) {
            candidates.push_back(makeCandidate(move, 780, "黑方三七炮压制对方马位"));
        }
    }

    {
        Move horse = anyNormalHorseMove(legalMoves, BLACK, weakFlank);
        if (!horse.isInvalid()) {
            candidates.push_back(makeCandidate(horse, 760, "黑方常规正马发展"));
        }
    }

    {
        Move rook = selectRookDevelopment(legalMoves, BLACK, weakFlank);
        if (!rook.isInvalid()) {
            candidates.push_back(makeCandidate(rook, 720, "黑方也应早出车"));
        }
    }

    return bestCandidate(board, candidates);
}

inline Move chooseOpeningMove(ChessBoard& board, const std::vector<Move>& legalMoves) {
    if (legalMoves.empty() || !isOpeningStage(board) || board.isInCheck()) {
        return Move();
    }

    Candidate best = board.currentColor() == RED ?
        buildRedRule(board, legalMoves) : buildBlackRule(board, legalMoves);
    return best.move;
}

} // namespace OpeningBook

#endif