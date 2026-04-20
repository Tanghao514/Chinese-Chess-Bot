#include "AIPlayer.h"
#include "Protocol.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    std::string input = buffer.str();

    ParsedInput parsed;
    std::string errorMessage;
    if (!Protocol::parseInput(input, parsed, &errorMessage)) {
        Json::Value out = Protocol::makeOutput(Move(), Json::Value(Json::objectValue));
        std::cout << Protocol::writeJson(out) << std::endl;
        return 0;
    }

    ChessBoard board;
    const size_t turnID = parsed.responses.size();

    for (size_t i = 0; i < turnID; ++i) {
        if (i < parsed.requests.size() && parsed.requests[i].valid) {
            board.makeMoveAssumeLegal(parsed.requests[i].move);
        }
        if (i < parsed.responses.size() && parsed.responses[i].valid) {
            board.makeMoveAssumeLegal(parsed.responses[i].move);
        }
    }

    if (turnID < parsed.requests.size() && parsed.requests[turnID].valid) {
        board.makeMoveAssumeLegal(parsed.requests[turnID].move);
    }

    std::vector<Move> legalMovesBasic;
    std::vector<Move> legalMovesWithForbidden;
    board.generateMoves(legalMovesBasic);
    board.generateMovesWithForbidden(legalMovesWithForbidden);

    const auto containsMove = [](const std::vector<Move>& moves, const Move& move) {
        return std::find(moves.begin(), moves.end(), move) != moves.end();
    };

    Move bestMove;
    AIPlayer ai;
    bestMove = ai.getBestMove(board);

    if (!legalMovesWithForbidden.empty()) {
        if (bestMove.isInvalid() || !containsMove(legalMovesWithForbidden, bestMove)) {
            bestMove = legalMovesWithForbidden[0];
        }
    } else {
        if (bestMove.isInvalid() || !containsMove(legalMovesBasic, bestMove)) {
            if (!legalMovesBasic.empty()) {
                bestMove = legalMovesBasic[0];
            } else {
                bestMove = Move();
            }
        }
    }

    Json::Value outData = parsed.data;
    if (!outData.isObject()) {
        outData = Json::Value(Json::objectValue);
    }

    Json::Value out = Protocol::makeOutput(bestMove, outData);
    std::cout << Protocol::writeJson(out) << std::endl;
    return 0;
}
