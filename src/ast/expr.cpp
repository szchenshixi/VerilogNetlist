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

uint32_t exprBitWidth(const Expr& e, const elab::ModuleSpec& m) {
    return e.visit([&](auto&& node) -> uint32_t {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IdExpr>) {
            auto* ent = findEntity(m, node.mName);
            return ent ? ent->width() : 0;
        } else if constexpr (std::is_same_v<T, ConstExpr>) {
            if (node.mWidth > 0) return static_cast<uint32_t>(node.mWidth);
            return minimalWidthForValue(node.mValue);
        } else if constexpr (std::is_same_v<T, ConcatExpr>) {
            uint32_t sum = 0;
            for (const auto& p : node.mParts) {
                sum += exprBitWidth(p, m);
            }
            return sum;
        } else if constexpr (std::is_same_v<T, SliceExpr>) {
            if (!node.mMsb || !node.mLsb) return 0u;
            return width_from_range(*node.mMsb, *node.mLsb);
        } else {
            return 0u;
        }
    });
}

static void exprToStringImpl(const Expr& e, std::ostream& os) {
    e.visit([&](auto&& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IdExpr>) {
            os << node.mName.str();
        } else if constexpr (std::is_same_v<T, ConstExpr>) {
            if (!node.mText.empty()) os << node.mText;
            else os << node.mWidth << "'d" << node.mValue;
        } else if constexpr (std::is_same_v<T, ConcatExpr>) {
            os << "{";
            for (size_t i = 0; i < node.mParts.size(); ++i) {
                exprToStringImpl(node.mParts[i], os);
                if (i + 1 < node.mParts.size()) os << ", ";
            }
            os << "}";
        } else if constexpr (std::is_same_v<T, SliceExpr>) {
            if (!node.mMsb || !node.mLsb) {
                os << "[?:?]";
                return;
            }
            os << node.mBaseId.str() << "[";
            exprToStringImpl(*node.mMsb, os);
            os << ":";
            exprToStringImpl(*node.mLsb, os);
            os << "]";
        } else if constexpr (std::is_same_v<T, OpExpr>) {
            const auto operandCount = node.mOperands.size();
            if (operandCount == 0) return;

            // Treat a single-operand subtraction as unary minus.
            if (node.mOp == OpType::Sub && operandCount == 1) {
                os << "-";
                const Expr& operand = node.mOperands.front();
                const bool wrap =
                  std::holds_alternative<OpExpr>(operand.mNode);
                if (wrap) os << "(";
                exprToStringImpl(operand, os);
                if (wrap) os << ")";
                return;
            }

            const char* opSymbol = node.mOp == OpType::Add ? " + " : " - ";

            for (size_t i = 0; i < operandCount; ++i) {
                const Expr& operand = node.mOperands[i];
                const bool wrap =
                  node.mOp == OpType::Sub && i > 0 &&
                  std::holds_alternative<OpExpr>(operand.mNode);

                if (wrap) os << "(";
                exprToStringImpl(operand, os);
                if (wrap) os << ")";

                if (i + 1 < operandCount) { os << opSymbol; }
            }
        }
    });
}

std::string exprToString(const Expr& e) {
    std::ostringstream oss;
    exprToStringImpl(e, oss);
    return oss.str();
}

int64_t exprToInt64(const Expr& e, const ParamSpec& p) {
    return e.visit([&p](auto&& node) -> int64_t {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IdExpr>) {
            auto iter = p.find(node.mName);
            return iter == p.end() ? 0 : iter->second;
        } else if constexpr (std::is_same_v<T, ConstExpr>) {
            return static_cast<int64_t>(node.mValue);
        } else if constexpr (std::is_same_v<T, ConcatExpr>) {
            struct BitWidthHelper {
                const ParamSpec& env;

                static int valueBitWidth(int64_t v) {
                    if (v < 0) return 64;
                    uint64_t u = static_cast<uint64_t>(v);
                    if (u == 0) return 1;
                    int width = 0;
                    while (u != 0) {
                        ++width;
                        u >>= 1;
                    }
                    return width;
                }

                int operator()(const Expr& expr) const {
                    return expr.visit([this](auto&& sub) -> int {
                        using U = std::decay_t<decltype(sub)>;
                        if constexpr (std::is_same_v<U, ConstExpr>) {
                            if (sub.mWidth > 0) return sub.mWidth;
                            return valueBitWidth(
                              static_cast<int64_t>(sub.mValue));
                        } else if constexpr (std::is_same_v<U, IdExpr>) {
                            auto iter = env.find(sub.mName);
                            if (iter == env.end()) return 1;
                            return valueBitWidth(iter->second);
                        } else if constexpr (std::is_same_v<U, ConcatExpr>) {
                            int total = 0;
                            for (const auto& part : sub.mParts) {
                                total += (*this)(part);
                            }
                            return total;
                        } else if constexpr (std::is_same_v<U, SliceExpr>) {
                            if (!sub.mMsb || !sub.mLsb) return 0;
                            return width_from_range(*sub.mMsb, *sub.mLsb);
                        } else if constexpr (std::is_same_v<U, OpExpr>) {
                            int maxWidth = 0;
                            for (const auto& operand : sub.mOperands) {
                                int w = (*this)(operand);
                                if (w > maxWidth) maxWidth = w;
                            }
                            std::size_t count = sub.mOperands.size();
                            int extra = 0;
                            if (count > 1) {
                                std::size_t scaled = 1;
                                while (scaled < count) {
                                    ++extra;
                                    scaled <<= 1;
                                }
                            }
                            if (sub.mOp == OpType::Sub) ++extra;
                            return maxWidth + extra;
                        } else {
                            return 0;
                        }
                    });
                }
            };

            auto maskForWidth = [](int width) -> uint64_t {
                if (width <= 0) return 0;
                if (width >= 64) return ~uint64_t{0};
                return (uint64_t{1} << width) - 1;
            };

            auto safeShiftLeft = [](uint64_t value, int amount) -> uint64_t {
                if (amount <= 0) return value;
                if (amount >= 64) return 0;
                return value << static_cast<unsigned>(amount);
            };

            BitWidthHelper widthHelper{p};
            uint64_t result = 0;

            for (const auto& part : node.mParts) {
                const int width = widthHelper(part);
                const uint64_t partValue =
                  static_cast<uint64_t>(exprToInt64(part, p));
                result = safeShiftLeft(result, width);
                result |= partValue & maskForWidth(width);
            }

            return static_cast<int64_t>(result);
        } else if constexpr (std::is_same_v<T, SliceExpr>) {
            auto iter = p.find(node.mBaseId);
            const uint64_t baseValue =
              iter == p.end() ? 0 : static_cast<uint64_t>(iter->second);

            if (node.mMsb < node.mLsb) { return 0; }

            if (!node.mMsb || !node.mLsb) return 0;
            const int64_t width = width_from_range(*node.mMsb, *node.mLsb);
            if (width <= 0) return 0;

            auto maskForWidth = [](int w) -> uint64_t {
                if (w <= 0) return 0;
                if (w >= 64) return ~uint64_t{0};
                return (uint64_t{1} << w) - 1;
            };

            auto safeShiftRight = [](uint64_t value, int amount) -> uint64_t {
                if (amount <= 0) return value;
                if (amount >= 64) return 0;
                return value >> static_cast<unsigned>(amount);
            };

            uint64_t lsb = 0;
            if (node.mLsb) lsb = exprToInt64(*node.mLsb);
            const uint64_t shifted = safeShiftRight(baseValue, lsb);
            return static_cast<int64_t>(shifted & maskForWidth(width));
        } else if constexpr (std::is_same_v<T, OpExpr>) {
            if (node.mOperands.empty()) return 0;

            if (node.mOp == OpType::Add) {
                int64_t acc = 0;
                for (const auto& operand : node.mOperands) {
                    acc += exprToInt64(operand, p);
                }
                return acc;
            }

            int64_t acc = exprToInt64(node.mOperands.front(), p);
            if (node.mOperands.size() == 1) { return -acc; }
            for (std::size_t i = 1; i < node.mOperands.size(); ++i) {
                acc -= exprToInt64(node.mOperands[i], p);
            }
            return acc;
        }
    });
}
} // namespace hdl::ast