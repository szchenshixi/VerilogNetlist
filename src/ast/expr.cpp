#include <algorithm>
#include <sstream>

#include "hdl/ast/decl.hpp"
#include "hdl/ast/expr.hpp"

namespace hdl {
namespace ast {

static uint32_t minimalWidthForValue(uint64_t v) {
    if (v == 0) return 1;
    uint32_t w = 0;
    while (v) {
        v >>= 1;
        ++w;
    }
    return w;
}

static const WireEntity* findEntity(const ModuleDecl& m, IdString name) {
    if (auto* p = m.findPort(name)) return &p->mEnt;
    if (auto* w = m.findWire(name)) return &w->mEnt;
    return nullptr;
}

uint32_t exprBitWidth(const Expr& e, const ModuleDecl& m) {
    return std::visit(
      [&](auto&& node) -> uint32_t {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, IdExpr>) {
              auto* ent = findEntity(m, node.mName);
              return ent ? ent->width() : 0;
          } else if constexpr (std::is_same_v<T, ConstExpr>) {
              if (node.mWidth > 0) return static_cast<uint32_t>(node.mWidth);
              return minimalWidthForValue(node.mValue);
          } else if constexpr (std::is_same_v<T, ConcatExpr>) {
              uint32_t sum = 0;
              for (const auto& p : node.mParts)
                  sum += exprBitWidth(*p, m);
              return sum;
          } else if constexpr (std::is_same_v<T, SliceExpr>) {
              return width_from_range(node.mMsb, node.mLsb);
          } else {
              return 0u;
          }
      },
      e.mNode);
}

static void exprToStringImpl(const Expr& e, std::ostream& os) {
    std::visit(
      [&](auto&& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, IdExpr>) {
              os << node.mName.str();
          } else if constexpr (std::is_same_v<T, ConstExpr>) {
              if (!node.mText.empty()) os << node.mText;
              else os << node.mWidth << "'d" << node.mValue;
          } else if constexpr (std::is_same_v<T, ConcatExpr>) {
              os << "{";
              for (size_t i = 0; i < node.mParts.size(); ++i) {
                  exprToStringImpl(*node.mParts[i], os);
                  if (i + 1 < node.mParts.size()) os << ", ";
              }
              os << "}";
          } else if constexpr (std::is_same_v<T, SliceExpr>) {
              os << node.mBaseId.str() << "[" << node.mMsb << ":" << node.mLsb
                 << "]";
          }
      },
      e.mNode);
}

std::string exprToString(const Expr& e) {
    std::ostringstream oss;
    exprToStringImpl(e, oss);
    return oss.str();
}

} // namespace ast
} // namespace hdl