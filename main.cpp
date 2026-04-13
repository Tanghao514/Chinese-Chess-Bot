#include "AIPlayer.h"
#include "Protocol.h"
#include <iostream>
#include <sstream>
#include <string>

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

    // 回放历史走法：只检查基本合法性，不检查禁着（重复局面）规则。
    // 对手的走法由平台校验，我方的历史走法已被平台接受，
    // 用 isMoveValidWithForbidden 会在对手制造重复局面时错误拒绝，导致输出空着 → INVALID_INPUT_VERDICT。
    for (size_t i = 0; i < turnID; ++i) {
        if (i < parsed.requests.size() && parsed.requests[i].valid) {
            if (!board.isMoveValid(parsed.requests[i].move)) {
                Json::Value out = Protocol::makeOutput(Move(), Json::Value(Json::objectValue));
                std::cout << Protocol::writeJson(out) << std::endl;
                return 0;
            }
            board.makeMoveAssumeLegal(parsed.requests[i].move);
        }

        if (i < parsed.responses.size() && parsed.responses[i].valid) {
            if (!board.isMoveValid(parsed.responses[i].move)) {
                Json::Value out = Protocol::makeOutput(Move(), Json::Value(Json::objectValue));
                std::cout << Protocol::writeJson(out) << std::endl;
                return 0;
            }
            board.makeMoveAssumeLegal(parsed.responses[i].move);
        }
    }

    if (turnID < parsed.requests.size() && parsed.requests[turnID].valid) {
        if (!board.isMoveValid(parsed.requests[turnID].move)) {
            Json::Value out = Protocol::makeOutput(Move(), Json::Value(Json::objectValue));
            std::cout << Protocol::writeJson(out) << std::endl;
            return 0;
        }
        board.makeMoveAssumeLegal(parsed.requests[turnID].move);
    }

    AIPlayer ai;
    Move bestMove = ai.getBestMove(board);

    Json::Value outData = parsed.data;
    if (!outData.isObject()) {
        outData = Json::Value(Json::objectValue);
    }

    Json::Value out = Protocol::makeOutput(bestMove, outData);
    std::cout << Protocol::writeJson(out) << std::endl;
    return 0;
}