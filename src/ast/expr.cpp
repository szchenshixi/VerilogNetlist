#include <algorithm>
#include <sstream>

#include "hdl/ast/decl.hpp"
#include "hdl/ast/expr.hpp"
#include "hdl/elab/spec.hpp"

namespace hdl::ast {

static uint32_t minimalWidthForValue(uint64_t v) {
    if (v == 0) return 1;
    uint32_t w = 0;
    while (v) {
        v >>= 1;
        ++w;
    }
    return w;
}

static const elab::NetSpec* findEntity(const elab::ModuleSpec& m,
                                       IdString name) {
    if (int p = m.findPortIndex(name); p != -1) return &m.mPorts[p].mNet;
    if (int w = m.findWireIndex(name); w != -1) return &m.mWires[w].mNet;
    return nullptr;
}

void intExprToStringImpl(const IntExpr& e, std::ostream& os) {
    e.visit([&](auto&& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntId>) {
            os << node.mName.str();
        } else if constexpr (std::is_same_v<T, IntConst>) {
            os << node.mValue;
        } else if constexpr (std::is_same_v<T, IntOp>) {
            const auto operandCount = node.mOperands.size();
            if (operandCount == 0) return;

            // Treat a single-operand subtraction as unary minus.
            if (node.mOp == IntOp::Type::Sub && operandCount == 1) {
                os << "-";
                const IntExpr& operand = node.mOperands.front();
                const bool wrap = std::holds_alternative<IntOp>(operand.mNode);
                if (wrap) os << "(";
                intExprToStringImpl(operand, os);
                if (wrap) os << ")";
                return;
            }

            const char* opSymbol =
              node.mOp == IntOp::Type::Add ? " + " : " - ";

            for (size_t i = 0; i < operandCount; ++i) {
                const IntExpr& operand = node.mOperands[i];
                const bool wrap = node.mOp == IntOp::Type::Sub && i > 0 &&
                                  std::holds_alternative<IntOp>(operand.mNode);

                if (wrap) os << "(";
                intExprToStringImpl(operand, os);
                if (wrap) os << ")";

                if (i + 1 < operandCount) { os << opSymbol; }
            }
        }
    });
}

std::string intExprToString(const IntExpr& e) {}

int64_t evalIntExpr(const ast::IntExpr& x, const elab::ParamSpec& params,
                    std::ostream* diag) {
    return x.visit([&](const auto& node) -> int64_t {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::IntId>) {
            IdString n = node.mName;
            auto it = params.find(n);
            if (it == params.end()) {
                error(diag, "unknown parameter '" + n.str() + "' in IntExpr");
                return 0;
            }
            return it->second;
        } else if constexpr (std::is_same_v<T, ast::IntConst>) {
            return static_cast<int64_t>(node.mValue);
        } else {
            error(diag, "unknown expression type in int value evaluation");
            return 0;
        }
    });
}

void bvExprToStringImpl(const BVExpr& e, std::ostream& os) {
    e.visit([&](auto&& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BVId>) {
            os << node.mName.str();
        } else if constexpr (std::is_same_v<T, BVConst>) {
            if (!node.mText.empty()) os << node.mText;
            else os << node.mWidth << "'d" << node.mValue;
        } else if constexpr (std::is_same_v<T, BVConcat>) {
            os << "{";
            for (size_t i = 0; i < node.mParts.size(); ++i) {
                bvExprToStringImpl(node.mParts[i], os);
                if (i + 1 < node.mParts.size()) os << ", ";
            }
            os << "}";
        } else if constexpr (std::is_same_v<T, BVSlice>) {
            os << node.mBaseId.str() << "[";
            intExprToStringImpl(node.mMsb, os);
            os << ":";
            intExprToStringImpl(node.mLsb, os);
            os << "]";
        } else if constexpr (std::is_same_v<T, BVOp>) {
            const auto operandCount = node.mOperands.size();
            if (operandCount == 0) return;

            // Treat a single-operand subtraction as unary minus.
            if (node.mOp == OpType::Sub && operandCount == 1) {
                os << "-";
                const BVExpr& operand = node.mOperands.front();
                const bool wrap = std::holds_alternative<BVOp>(operand.mNode);
                if (wrap) os << "(";
                bvExprToStringImpl(operand, os);
                if (wrap) os << ")";
                return;
            }

            const char* opSymbol = node.mOp == OpType::Add ? " + " : " - ";

            for (size_t i = 0; i < operandCount; ++i) {
                const BVExpr& operand = node.mOperands[i];
                const bool wrap = node.mOp == OpType::Sub && i > 0 &&
                                  std::holds_alternative<BVOp>(operand.mNode);

                if (wrap) os << "(";
                bvExprToStringImpl(operand, os);
                if (wrap) os << ")";

                if (i + 1 < operandCount) { os << opSymbol; }
            }
        }
    });
}

std::string bvExprToString(const BVExpr& e) {
    std::ostringstream oss;
    bvExprToStringImpl(e, oss);
    return oss.str();
}

uint32_t bvExprBitWidth(const BVExpr& e, const elab::ModuleSpec& m) {
    return e.visit([&](auto&& node) -> uint32_t {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BVId>) {
            auto* ent = findEntity(m, node.mName);
            return ent ? ent->width() : 0;
        } else if constexpr (std::is_same_v<T, BVConst>) {
            if (node.mWidth > 0) return static_cast<uint32_t>(node.mWidth);
            return minimalWidthForValue(node.mValue);
        } else if constexpr (std::is_same_v<T, BVConcat>) {
            uint32_t sum = 0;
            for (const auto& p : node.mParts) {
                sum += bvExprBitWidth(p, m);
            }
            return sum;
        } else if constexpr (std::is_same_v<T, BVSlice>) {
            return widthFromRange(node.mMsb, node.mLsb, m.mEnv);
        } else {
            return 0u;
        }
    });
}
} // namespace hdl::ast