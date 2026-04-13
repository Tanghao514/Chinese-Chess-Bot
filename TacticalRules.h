#ifndef TACTICALRULES_H
#define TACTICALRULES_H

#include "ChessBoard.h"

#include <algorithm>
#include <array>
#include <vector>

namespace TacticalRules {

struct AttackUnit {
    int x = -1;
    int y = -1;
    stoneType type = None;
    int value = 0;
};

struct RootInfo {
    int supportCount = 0;
    int attackCount = 0;
    int netCount = 0;
    int weightedSupport = 0;
    int weightedAttack = 0;
    int weightedNet = 0;
    int exchangeScore = 0;
    bool hasRoot = false;
    bool virtualRoot = false;
    bool inDanger = false;
    bool severelyHanging = false;
};

struct MoveTacticalInfo {
    int sourceExchangeScore = 0;
    int destinationExchangeScore = 0;
    int sourceWeightedNet = 0;
    int destinationWeightedNet = 0;
    int activityDelta = 0;
    int tradeScore = 0;
    bool improvesByRunning = false;
    bool canRootSafely = false;
    bool prefersTrade = false;
    bool destinationSafer = false;
};

inline int pieceValue(stoneType type) {
    switch (type) {
        case Rook:      return 900;
        case Cannon:    return 460;
        case Knight:    return 430;
        case Pawn:      return 120;
        case Bishop:    return 220;
        case Assistant: return 220;
        case King:      return 10000;
        default:        return 0;
    }
}

inline int homeRank(colorType side) {
    return side == RED ? 0 : 9;
}

inline int advanceOf(colorType side, int y) {
    return side == RED ? y : (9 - y);
}

inline bool crossedRiver(colorType side, int y) {
    return side == RED ? y >= 5 : y <= 4;
}

inline bool isNormalKnightSquare(colorType side, int x, int y) {
    return advanceOf(side, y) == 2 && (x == 2 || x == 6);
}

inline int flankIndexFromFile(int x) {
    if (x <= 3) {
        return 0;
    }
    if (x >= 5) {
        return 1;
    }
    return -1;
}

inline bool onFlank(int x, int flank) {
    return flank == 0 ? (x <= 3) : (x >= 5);
}

namespace detail {

using Snapshot = std::array<Grid, BOARDWIDTH * BOARDHEIGHT>;

inline int pos(int x, int y) {
    return ChessBoard::xy2pos(x, y);
}

inline Grid get(const Snapshot& snapshot, int x, int y) {
    return snapshot[pos(x, y)];
}

inline void set(Snapshot& snapshot, int x, int y, const Grid& grid) {
    snapshot[pos(x, y)] = grid;
}

inline Snapshot makeSnapshot(const ChessBoard& board) {
    Snapshot snapshot{};
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            snapshot[pos(x, y)] = board.getGridAt(x, y);
        }
    }
    return snapshot;
}

inline bool inPalace(colorType side, int x, int y) {
    return ChessBoard::inKingArea(x, y, side);
}

inline bool rookAttacks(const Snapshot& snapshot, int sx, int sy, int tx, int ty) {
    if (!ChessBoard::inSameStraightLine(sx, sy, tx, ty)) {
        return false;
    }
    const int dx = (tx > sx) ? 1 : ((tx < sx) ? -1 : 0);
    const int dy = (ty > sy) ? 1 : ((ty < sy) ? -1 : 0);
    int cx = sx + dx;
    int cy = sy + dy;
    while (cx != tx || cy != ty) {
        if (get(snapshot, cx, cy).color != EMPTY) {
            return false;
        }
        cx += dx;
        cy += dy;
    }
    return true;
}

inline int countBetween(const Snapshot& snapshot, int sx, int sy, int tx, int ty) {
    if (!ChessBoard::inSameStraightLine(sx, sy, tx, ty)) {
        return 99;
    }
    const int dx = (tx > sx) ? 1 : ((tx < sx) ? -1 : 0);
    const int dy = (ty > sy) ? 1 : ((ty < sy) ? -1 : 0);
    int cx = sx + dx;
    int cy = sy + dy;
    int count = 0;
    while (cx != tx || cy != ty) {
        if (get(snapshot, cx, cy).color != EMPTY) {
            ++count;
        }
        cx += dx;
        cy += dy;
    }
    return count;
}

inline bool attacksSquare(const Snapshot& snapshot, int sx, int sy, colorType side,
                          stoneType type, int tx, int ty) {
    if (sx == tx && sy == ty) {
        return false;
    }

    const int dx = tx - sx;
    const int dy = ty - sy;
    switch (type) {
        case King:
            return inPalace(side, tx, ty) && std::abs(dx) + std::abs(dy) == 1;
        case Assistant:
            return inPalace(side, tx, ty) && std::abs(dx) == 1 && std::abs(dy) == 1;
        case Bishop: {
            if (!ChessBoard::inColorArea(tx, ty, side) || std::abs(dx) != 2 || std::abs(dy) != 2) {
                return false;
            }
            const int eyeX = sx + dx / 2;
            const int eyeY = sy + dy / 2;
            return get(snapshot, eyeX, eyeY).color == EMPTY;
        }
        case Knight:
            if (std::abs(dx) == 2 && std::abs(dy) == 1) {
                return get(snapshot, sx + (dx > 0 ? 1 : -1), sy).color == EMPTY;
            }
            if (std::abs(dx) == 1 && std::abs(dy) == 2) {
                return get(snapshot, sx, sy + (dy > 0 ? 1 : -1)).color == EMPTY;
            }
            return false;
        case Rook:
            return rookAttacks(snapshot, sx, sy, tx, ty);
        case Cannon:
            return ChessBoard::inSameStraightLine(sx, sy, tx, ty) && countBetween(snapshot, sx, sy, tx, ty) == 1;
        case Pawn: {
            const int forward = side == RED ? 1 : -1;
            if (dx == 0 && dy == forward) {
                return true;
            }
            return crossedRiver(side, sy) && std::abs(dx) == 1 && dy == 0;
        }
        default:
            return false;
    }
}

inline std::vector<AttackUnit> collectAttackers(const Snapshot& snapshot, colorType side, int tx, int ty) {
    std::vector<AttackUnit> result;
    result.reserve(16);
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = get(snapshot, x, y);
            if (g.color != side || g.type == None) {
                continue;
            }
            if (attacksSquare(snapshot, x, y, side, g.type, tx, ty)) {
                result.push_back({x, y, g.type, pieceValue(g.type)});
            }
        }
    }
    std::sort(result.begin(), result.end(), [](const AttackUnit& lhs, const AttackUnit& rhs) {
        if (lhs.value != rhs.value) {
            return lhs.value < rhs.value;
        }
        if (lhs.type != rhs.type) {
            return lhs.type < rhs.type;
        }
        if (lhs.x != rhs.x) {
            return lhs.x < rhs.x;
        }
        return lhs.y < rhs.y;
    });
    return result;
}

inline int rootContribution(int targetValue, int helperValue) {
    if (targetValue <= 0 || helperValue <= 0) {
        return 0;
    }
    const int raw = targetValue * 100 / helperValue;
    return std::clamp(raw, 35, 180);
}

inline int squareActivity(const Snapshot& snapshot, colorType side, stoneType type, int x, int y) {
    int score = 0;
    switch (type) {
        case Rook: {
            for (int d = 0; d < 4; ++d) {
                int tx = x + dx_strai[d];
                int ty = y + dy_strai[d];
                while (ChessBoard::inBoard(tx, ty)) {
                    const Grid g = get(snapshot, tx, ty);
                    if (g.color == EMPTY) {
                        score += 3;
                    } else {
                        if (g.color != side) {
                            score += 4;
                        }
                        break;
                    }
                    tx += dx_strai[d];
                    ty += dy_strai[d];
                }
            }
            if (x == 3 || x == 5) {
                score += 12;
            }
            if (advanceOf(side, y) >= 2) {
                score += 8;
            }
            break;
        }
        case Knight: {
            int jumps = 0;
            int blocked = 0;
            for (int d = 0; d < 8; ++d) {
                const int tx = x + dx_knight[d];
                const int ty = y + dy_knight[d];
                if (!ChessBoard::inBoard(tx, ty)) {
                    continue;
                }
                const int footX = x + dx_knight_foot[d];
                const int footY = y + dy_knight_foot[d];
                if (get(snapshot, footX, footY).color != EMPTY) {
                    ++blocked;
                    continue;
                }
                if (get(snapshot, tx, ty).color != side) {
                    ++jumps;
                }
            }
            score += jumps * 5 - blocked * 4;
            if (isNormalKnightSquare(side, x, y)) {
                score += 14;
            }
            break;
        }
        case Cannon: {
            int mobility = 0;
            int threats = 0;
            for (int d = 0; d < 4; ++d) {
                int tx = x + dx_strai[d];
                int ty = y + dy_strai[d];
                bool foundScreen = false;
                while (ChessBoard::inBoard(tx, ty)) {
                    const Grid g = get(snapshot, tx, ty);
                    if (!foundScreen) {
                        if (g.color == EMPTY) {
                            ++mobility;
                        } else {
                            foundScreen = true;
                        }
                    } else if (g.color != EMPTY) {
                        if (g.color != side) {
                            ++threats;
                        }
                        break;
                    }
                    tx += dx_strai[d];
                    ty += dy_strai[d];
                }
            }
            score += mobility * 2 + threats * 5;
            if (x == 4) {
                score += 14;
            }
            break;
        }
        case Pawn:
            score += advanceOf(side, y) * 3;
            break;
        default:
            break;
    }
    return score;
}

inline int seeGain(detail::Snapshot& snapshot, int tx, int ty, colorType sideToMove) {
    const Grid target = get(snapshot, tx, ty);
    if (target.color == EMPTY || target.type == None) {
        return 0;
    }

    const std::vector<AttackUnit> attackers = collectAttackers(snapshot, sideToMove, tx, ty);
    if (attackers.empty()) {
        return 0;
    }

    int bestGain = 0;
    for (const AttackUnit& attacker : attackers) {
        const Grid moving = get(snapshot, attacker.x, attacker.y);
        const Grid captured = get(snapshot, tx, ty);
        set(snapshot, attacker.x, attacker.y, Grid());
        set(snapshot, tx, ty, moving);

        const int gain = pieceValue(captured.type) - seeGain(snapshot, tx, ty, ChessBoard::oppColor(sideToMove));

        set(snapshot, attacker.x, attacker.y, moving);
        set(snapshot, tx, ty, captured);
        bestGain = std::max(bestGain, gain);
    }
    return bestGain;
}

} // namespace detail

inline RootInfo analyzeRoot(const ChessBoard& board, int x, int y) {
    RootInfo info;
    if (!ChessBoard::inBoard(x, y)) {
        return info;
    }

    const Grid target = board.getGridAt(x, y);
    if (target.color == EMPTY || target.type == None || target.type == King) {
        return info;
    }

    const detail::Snapshot snapshot = detail::makeSnapshot(board);
    const colorType opp = ChessBoard::oppColor(target.color);
    const std::vector<AttackUnit> supporters = detail::collectAttackers(snapshot, target.color, x, y);
    const std::vector<AttackUnit> attackers = detail::collectAttackers(snapshot, opp, x, y);
    const int targetValue = pieceValue(target.type);

    info.supportCount = static_cast<int>(supporters.size());
    info.attackCount = static_cast<int>(attackers.size());
    info.netCount = info.supportCount - info.attackCount;

    for (const AttackUnit& supporter : supporters) {
        info.weightedSupport += detail::rootContribution(targetValue, supporter.value);
    }
    for (const AttackUnit& attacker : attackers) {
        info.weightedAttack += detail::rootContribution(targetValue, attacker.value);
    }

    detail::Snapshot exchangeSnapshot = snapshot;
    info.exchangeScore = -detail::seeGain(exchangeSnapshot, x, y, opp);
    info.weightedNet = info.weightedSupport - info.weightedAttack + info.exchangeScore / 3;
    info.hasRoot = info.exchangeScore >= 0 && info.weightedNet >= -20;
    info.virtualRoot = info.supportCount > 0 && !info.hasRoot;
    info.inDanger = info.exchangeScore < 0 || info.weightedNet < -40;
    info.severelyHanging = info.exchangeScore <= -(targetValue / 3) || info.weightedNet < -120;
    return info;
}

inline int flankStrength(const ChessBoard& board, colorType side, int flank) {
    int score = 0;
    for (int x = 0; x < BOARDWIDTH; ++x) {
        for (int y = 0; y < BOARDHEIGHT; ++y) {
            const Grid g = board.getGridAt(x, y);
            if (g.color != side || !onFlank(x, flank)) {
                continue;
            }
            switch (g.type) {
                case Rook:      score += 18; break;
                case Cannon:    score += 14; break;
                case Knight:    score += 14; break;
                case Bishop:    score += 8;  break;
                case Assistant: score += 8;  break;
                case Pawn:      score += crossedRiver(side, y) ? 7 : 5; break;
                default: break;
            }
            if (g.type == Rook || g.type == Cannon || g.type == Knight || g.type == Pawn) {
                const RootInfo root = analyzeRoot(board, x, y);
                score += root.hasRoot ? 4 : 0;
                score -= root.inDanger ? 5 : 0;
            }
        }
    }
    return score;
}

inline int weakerFlank(const ChessBoard& board, colorType defender) {
    const int left = flankStrength(board, defender, 0);
    const int right = flankStrength(board, defender, 1);
    if (std::abs(left - right) < 4) {
        return -1;
    }
    return left < right ? 0 : 1;
}

inline MoveTacticalInfo analyzeMove(ChessBoard& board, const Move& move) {
    MoveTacticalInfo info;
    if (move.isInvalid()) {
        return info;
    }

    const Grid source = board.getGridAt(move.source_x, move.source_y);
    const Grid target = board.getGridAt(move.target_x, move.target_y);
    if (source.color == EMPTY || source.type == None) {
        return info;
    }

    const RootInfo sourceRoot = analyzeRoot(board, move.source_x, move.source_y);
    const detail::Snapshot beforeSnapshot = detail::makeSnapshot(board);
    const int beforeActivity = detail::squareActivity(beforeSnapshot, source.color, source.type,
                                                      move.source_x, move.source_y);

    board.makeMoveAssumeLegal(move);
    const RootInfo destinationRoot = analyzeRoot(board, move.target_x, move.target_y);
    const detail::Snapshot afterSnapshot = detail::makeSnapshot(board);
    const int afterActivity = detail::squareActivity(afterSnapshot, source.color, source.type,
                                                     move.target_x, move.target_y);
    board.undoMove();

    info.sourceExchangeScore = sourceRoot.exchangeScore;
    info.destinationExchangeScore = destinationRoot.exchangeScore;
    info.sourceWeightedNet = sourceRoot.weightedNet;
    info.destinationWeightedNet = destinationRoot.weightedNet;
    info.activityDelta = afterActivity - beforeActivity;
    info.destinationSafer = destinationRoot.exchangeScore >= sourceRoot.exchangeScore ||
                            destinationRoot.weightedNet > sourceRoot.weightedNet;
    info.canRootSafely = destinationRoot.exchangeScore >= 0 && destinationRoot.weightedNet >= -20;
    info.improvesByRunning = sourceRoot.inDanger && info.destinationSafer && info.activityDelta >= -6;

    if (target.color != EMPTY && target.color != source.color &&
        (source.type == Rook || source.type == Cannon || source.type == Knight)) {
        info.tradeScore = pieceValue(target.type) - pieceValue(source.type) / 4 +
                          info.destinationExchangeScore + info.activityDelta * 2;
        if (source.type == Rook && target.type == Rook && info.activityDelta < 0) {
            info.tradeScore -= 30;
        }
        if (source.type == Cannon && target.type == Cannon && info.destinationExchangeScore < 0) {
            info.tradeScore -= 40;
        }
        info.prefersTrade = info.tradeScore > 0;
    }

    return info;
}

} // namespace TacticalRules

#endif