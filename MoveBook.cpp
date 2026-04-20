#include "MoveBook.h"
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <cctype>

namespace MoveBook {

struct BookLine {
    std::string moves;
    int weight = 0;
};

static std::vector<BookLine> g_book;
static bool g_loaded = false;

static std::string trim(const std::string& s) {
    size_t l = 0, r = s.size();
    while (l < r && std::isspace(static_cast<unsigned char>(s[l]))) ++l;
    while (r > l && std::isspace(static_cast<unsigned char>(s[r - 1]))) --r;
    return s.substr(l, r - l);
}

static std::vector<std::string> splitMoves(const std::string& seq) {
    std::vector<std::string> res;
    if (seq.size() % 4 != 0) return res;
    for (size_t i = 0; i + 3 < seq.size(); i += 4) {
        res.push_back(seq.substr(i, 4));
    }
    return res;
}

static std::string mirrorStr(const std::string& s) {
    if (s.size() != 4) return s;
    auto mirrorFile = [](char c) -> char {
        return static_cast<char>('a' + ('i' - c));
    };
    std::string t = s;
    t[0] = mirrorFile(t[0]);
    t[2] = mirrorFile(t[2]);
    return t;
}

static Move decodeMove(const std::string& s) {
    if (s.size() != 4) return Move();
    return Move(std::string() + s[0] + s[1], std::string() + s[2] + s[3]);
}

static bool isLegal(const Move& m, const std::vector<Move>& legalMoves) {
    for (const auto& mv : legalMoves) {
        if (mv == m) return true;
    }
    return false;
}

static bool parseCppStyleLine(const std::string& s, BookLine& out) {
    auto q1 = s.find('"');
    if (q1 == std::string::npos) return false;
    auto q2 = s.find('"', q1 + 1);
    if (q2 == std::string::npos) return false;

    std::string seq = s.substr(q1 + 1, q2 - q1 - 1);
    auto comma = s.find(',', q2);
    if (comma == std::string::npos) return false;

    std::string num = trim(s.substr(comma + 1));
    while (!num.empty() && (num.back() == ',' || num.back() == '}')) {
        num.pop_back();
    }
    num = trim(num);
    if (num.empty()) return false;

    out.moves = seq;
    out.weight = std::atoi(num.c_str());
    return !out.moves.empty();
}

static bool parseTabStyleLine(const std::string& s, BookLine& out) {
    std::size_t p = s.find_first_of("\t ");
    if (p == std::string::npos) return false;

    std::string seq = trim(s.substr(0, p));
    std::string tail = trim(s.substr(p + 1));
    if (seq.empty() || tail.empty()) return false;

    out.moves = seq;
    out.weight = std::atoi(tail.c_str());
    return out.weight > 0;
}

static void loadBookFromFile() {
    if (g_loaded) return;
    g_loaded = true;

    std::ifstream fin("BOOK.txt");
    if (!fin.good()) return;

    std::string line;
    while (std::getline(fin, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line.rfind("//", 0) == 0) continue;
        if (line.find("BOOK[]") != std::string::npos) continue;
        if (line == "{" || line == "};") continue;

        BookLine entry;
        if (parseCppStyleLine(line, entry) || parseTabStyleLine(line, entry)) {
            if (!entry.moves.empty() && entry.weight > 0) {
                g_book.push_back(std::move(entry));
            }
        }
    }
}

std::string encodeMove(const Move& m) {
    return std::string(1, pgnint2char(m.source_x)) +
           std::string(1, int2char(m.source_y)) +
           std::string(1, pgnint2char(m.target_x)) +
           std::string(1, int2char(m.target_y));
}

Move probeBook(const std::vector<Move>& history,
               const std::vector<Move>& legalMoves) {
    if (legalMoves.empty()) return Move();

    loadBookFromFile();
    if (g_book.empty()) return Move();

    std::vector<std::string> histStrs;
    histStrs.reserve(history.size());
    for (const auto& m : history) {
        histStrs.push_back(encodeMove(m));
    }
    const int histSize = static_cast<int>(histStrs.size());

    if (histSize > 20) return Move();

    struct Candidate {
        Move move;
        int weight;
    };
    std::vector<Candidate> candidates;

    for (const auto& line : g_book) {
        const auto parsed = splitMoves(line.moves);
        if (static_cast<int>(parsed.size()) <= histSize) continue;

        bool match = true;
        for (int j = 0; j < histSize; ++j) {
            if (histStrs[j] != parsed[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            Move m = decodeMove(parsed[histSize]);
            if (isLegal(m, legalMoves)) {
                bool found = false;
                for (auto& c : candidates) {
                    if (c.move == m) {
                        c.weight += line.weight;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    candidates.push_back({m, line.weight});
                }
            }
        }

        match = true;
        for (int j = 0; j < histSize; ++j) {
            if (histStrs[j] != mirrorStr(parsed[j])) {
                match = false;
                break;
            }
        }
        if (match) {
            Move m = decodeMove(mirrorStr(parsed[histSize]));
            if (isLegal(m, legalMoves)) {
                bool found = false;
                for (auto& c : candidates) {
                    if (c.move == m) {
                        c.weight += line.weight;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    candidates.push_back({m, line.weight});
                }
            }
        }
    }

    if (candidates.empty()) return Move();

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.weight > b.weight;
              });

    return candidates[0].move;
}

} // namespace MoveBook
