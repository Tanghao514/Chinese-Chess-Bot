#include "Protocol.h"

bool Protocol::parseMoveObject(const Json::Value& value, ProtocolMoveEntry& entry) {
    entry.valid = false;
    entry.move = Move();

    if (!value.isObject()) return false;
    if (!value.isMember("source") || !value.isMember("target")) return false;
    if (!value["source"].isString() || !value["target"].isString()) return false;

    std::string source = value["source"].asString();
    std::string target = value["target"].asString();

    if (source == "-1" || target == "-1") {
        entry.valid = false;
        return true;
    }

    if (source.size() != 2 || target.size() != 2) return false;

    entry.move = Move(source, target);
    entry.valid = true;
    return true;
}

bool Protocol::parseInput(const std::string& text, ParsedInput& out, std::string* errorMessage) {
    Json::Reader reader;
    Json::Value root;

    if (!reader.parse(text, root)) {
        if (errorMessage) *errorMessage = reader.getFormatedErrorMessages();
        return false;
    }

    out.requests.clear();
    out.responses.clear();
    out.data = root.isMember("data") ? root["data"] : Json::Value(Json::objectValue);

    if (root.isMember("requests") && root["requests"].isArray()) {
        for (Json::Value::UInt i = 0; i < root["requests"].size(); ++i) {
            ProtocolMoveEntry entry;
            if (!parseMoveObject(root["requests"][i], entry)) {
                if (errorMessage) *errorMessage = "invalid requests format";
                return false;
            }
            out.requests.push_back(entry);
        }
    }

    if (root.isMember("responses") && root["responses"].isArray()) {
        for (Json::Value::UInt i = 0; i < root["responses"].size(); ++i) {
            ProtocolMoveEntry entry;
            if (!parseMoveObject(root["responses"][i], entry)) {
                if (errorMessage) *errorMessage = "invalid responses format";
                return false;
            }
            out.responses.push_back(entry);
        }
    }

    return true;
}

std::string Protocol::posToString(int x, int y) {
    return std::string(1, pgnint2char(x)) + std::string(1, int2char(y));
}

Json::Value Protocol::makeOutput(const Move& move, const Json::Value& data) {
    Json::Value ret;
    if (move.isInvalid()) {
        ret["response"]["source"] = std::string("-1");
        ret["response"]["target"] = std::string("-1");
    } else {
        ret["response"]["source"] = posToString(move.source_x, move.source_y);
        ret["response"]["target"] = posToString(move.target_x, move.target_y);
    }
    ret["data"] = data;
    return ret;
}

std::string Protocol::writeJson(const Json::Value& root) {
    Json::FastWriter writer;
    return writer.write(root);
}