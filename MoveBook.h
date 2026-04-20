#ifndef MOVEBOOK_H
#define MOVEBOOK_H
#include "Common.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>

// 开局谱模块：基于走法序列前缀匹配的真正开局库
// 支持镜像匹配（左右翻转），按权重选择候选谱步

namespace MoveBook {

// 将 Move 编码为四字符字符串 (如 "h2e2")
std::string encodeMove(const Move& m);

// 查询开局库，返回推荐走法（无匹配则返回无效 Move）
// history:    按实际对局顺序排列的历史走法
// legalMoves: 当前合法走法列表
Move probeBook(const std::vector<Move>& history,
               const std::vector<Move>& legalMoves);

} // namespace MoveBook
#endif
