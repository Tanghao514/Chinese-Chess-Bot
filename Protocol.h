#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Common.h"
#include <string>
#include <vector>

#ifdef _BOTZONE_ONLINE
#include "jsoncpp/json.h"
#else
#include <json/json.h>
#endif

struct ProtocolMoveEntry {
    bool valid;
    Move move;

    ProtocolMoveEntry() : valid(false), move() {}
};

struct ParsedInput {
    std::vector<ProtocolMoveEntry> requests;
    std::vector<ProtocolMoveEntry> responses;
    Json::Value data;

    ParsedInput() : data(Json::Value(Json::objectValue)) {}
};

class Protocol {
public:
    static bool parseInput(const std::string& text, ParsedInput& out, std::string* errorMessage = nullptr);
    static Json::Value makeOutput(const Move& move, const Json::Value& data = Json::Value(Json::objectValue));
    static std::string writeJson(const Json::Value& root);

private:
    static bool parseMoveObject(const Json::Value& value, ProtocolMoveEntry& entry);
    static std::string posToString(int x, int y);
};

#endif