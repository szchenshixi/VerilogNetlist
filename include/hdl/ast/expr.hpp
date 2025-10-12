#pragma once
// Expression AST using std::variant: BVId, BVConst, BVConcat, BVSlice.

#include <cstdint>
#include <variant>
#include <vector>

#include "hdl/common.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl::elab {
// Forward declaration
struct ModuleSpec;
} // namespace hdl::elab
namespace hdl::ast {
//******************************************************************************
// Front-end integer AST, e.g. genvar, parameter (only used before elaboration)
//******************************************************************************
// Int expressions are values that numerical values in math
struct IntExpr;
struct IntId {
    IdString mName;
};
struct IntConst {
    uint64_t mValue = 0;
};

struct IntOp {
    enum class Type { Add, Sub };
    Type mOp;
    std::vector<IntExpr> mOperands;
};

struct IntExpr {
    using Variant = std::variant<IntId, IntConst, IntOp>;
    Variant mNode;

    IntExpr() = default;
    explicit IntExpr(Variant v)
        : mNode(std::move(v)) {}

    static IntExpr id(IdString n) { return IntExpr(IntId{n}); }
    static IntExpr number(uint64_t v) { return IntExpr(IntConst{v}); }

    static IntExpr unary(IntOp::Type op, const IntExpr& operand) {
        std::vector<IntExpr> operands_t;
        operands_t.push_back(operand); // Copy operand
        return IntExpr{IntOp{op, std::move(operands_t)}};
    }
    static IntExpr unary(IntOp::Type op, IntExpr&& operand) {
        std::vector<IntExpr> operands_t;
        operands_t.push_back(std::move(operand));
        return IntExpr{IntOp{op, std::move(operands_t)}};
    }

    static IntExpr binary(IntOp::Type op, const IntExpr& left,
                          const IntExpr& right) {
        std::vector<IntExpr> operands_t;
        operands_t.reserve(2);
        operands_t.push_back(left);  // Copy left
        operands_t.push_back(right); // Copy right
        return IntExpr{IntOp{op, std::move(operands_t)}};
    }
    static IntExpr binary(IntOp::Type op, IntExpr&& left, IntExpr&& right) {
        std::vector<IntExpr> operands_t;
        operands_t.reserve(2);
        operands_t.push_back(std::move(left));
        operands_t.push_back(std::move(right));
        return IntExpr{IntOp{op, std::move(operands_t)}};
    }

    static IntExpr add(const IntExpr& left, const IntExpr& right) {
        return binary(IntOp::Type::Add, left, right);
    }
    static IntExpr add(IntExpr&& left, IntExpr&& right) {
        return binary(IntOp::Type::Add, std::move(left), std::move(right));
    }
    static IntExpr sub(const IntExpr& left, const IntExpr& right) {
        return binary(IntOp::Type::Sub, left, right);
    }
    static IntExpr sub(IntExpr&& left, IntExpr&& right) {
        return binary(IntOp::Type::Sub, std::move(left), std::move(right));
    }

    // Access methods
    template <typename T>
    bool is() const {
        return std::holds_alternative<T>(mNode);
    }
    template <typename T>
    const T& as() const {
        return std::get<T>(mNode);
    }

    // Use example:
    template <typename Visitor>
    auto visit(Visitor&& vis) const {
        return std::visit(std::forward<Visitor>(vis), mNode);
    }
};

struct BVExpr;
struct BVId {
    IdString mName;
};
struct BVConst {
    uint64_t mValue = 0;
    int mWidth = 0;    // 0 => infer minimal
    std::string mText; // pretty
};
struct BVConcat {
    // Parts MSB -> LSB
    std::vector<BVExpr> mParts;
};
struct BVSlice {
    // Base must resolve to Id at elaboration; we store identifier directly.
    IdString mBaseId;
    IntExpr mMsb;
    IntExpr mLsb;
};
enum class OpType { Add, Sub };
struct BVOp {
    OpType mOp;
    std::vector<BVExpr> mOperands;
};

struct BVExpr {
    using Variant =
      std::variant<BVId, BVConst, BVConcat, BVSlice, BVOp>;
    Variant mNode;

    BVExpr() = default;
    explicit BVExpr(Variant v)
        : mNode(std::move(v)) {}

    static BVExpr id(IdString n) { return BVExpr(BVId{n}); }
    static BVExpr number(uint64_t v, int w = 0, const std::string& t = "") {
        return BVExpr(BVConst{v, w, t});
    }
    static BVExpr concat(const std::vector<BVExpr>& parts) {
        return BVExpr(BVConcat{parts}); // Copy the vector
    }
    static BVExpr concat(std::vector<BVExpr>&& parts) {
        return BVExpr(BVConcat{std::move(parts)});
    }
    // Unary slice serves as index
    static BVExpr slice(IdString baseId, int idx) {
        return slice(baseId, IntExpr::number(idx), IntExpr::number(idx));
    }
    static BVExpr slice(const IdString& baseId, const IntExpr& idx) {
        return BVExpr(BVSlice{baseId, idx, idx});
    }
    static BVExpr slice(IdString baseId, IntExpr&& idx) {
        return BVExpr(BVSlice{baseId, idx, std::move(idx)});
    }
    // Binary slice
    static BVExpr slice(IdString baseId, int msb, int lsb) {
        return slice(baseId, IntExpr::number(msb), IntExpr::number(lsb));
    }
    static BVExpr slice(const IdString& baseId, const IntExpr& msb,
                        const IntExpr& lsb) {
        return BVExpr(BVSlice{baseId, msb, lsb});
    }
    static BVExpr slice(IdString baseId, IntExpr&& msb, IntExpr&& lsb) {
        return BVExpr(BVSlice{baseId, std::move(msb), std::move(lsb)});
    }

    static BVExpr unary(OpType op, const BVExpr& operand) {
        std::vector<BVExpr> operands_t;
        operands_t.push_back(operand); // Copy operand
        return BVExpr{BVOp{op, std::move(operands_t)}};
    }
    static BVExpr unary(OpType op, BVExpr&& operand) {
        std::vector<BVExpr> operands_t;
        operands_t.push_back(std::move(operand));
        return BVExpr{BVOp{op, std::move(operands_t)}};
    }

    static BVExpr binary(OpType op, const BVExpr& left, const BVExpr& right) {
        std::vector<BVExpr> operands_t;
        operands_t.reserve(2);
        operands_t.push_back(left);  // Copy left
        operands_t.push_back(right); // Copy right
        return BVExpr{BVOp{op, std::move(operands_t)}};
    }
    static BVExpr binary(OpType op, BVExpr&& left, BVExpr&& right) {
        std::vector<BVExpr> operands_t;
        operands_t.reserve(2);
        operands_t.push_back(std::move(left));
        operands_t.push_back(std::move(right));
        return BVExpr{BVOp{op, std::move(operands_t)}};
    }

    static BVExpr add(const BVExpr& left, const BVExpr& right) {
        return binary(OpType::Add, left, right);
    }
    static BVExpr add(BVExpr&& left, BVExpr&& right) {
        return binary(OpType::Add, std::move(left), std::move(right));
    }
    static BVExpr sub(const BVExpr& left, const BVExpr& right) {
        return binary(OpType::Sub, left, right);
    }
    static BVExpr sub(BVExpr&& left, BVExpr&& right) {
        return binary(OpType::Sub, std::move(left), std::move(right));
    }

    // Access methods
    template <typename T>
    bool is() const {
        return std::holds_alternative<T>(mNode);
    }
    template <typename T>
    const T& as() const {
        return std::get<T>(mNode);
    }

    // Use example:
    template <typename Visitor>
    auto visit(Visitor&& vis) const {
        return std::visit(std::forward<Visitor>(vis), mNode);
    }
};

// Forward decl
struct ModuleDecl;

// Helpers implemented in src/ast/expr.cpp
std::string intExprToString(const IntExpr& e);
int64_t evalIntExpr(const ast::IntExpr& x, const elab::ParamSpec& params,
                    std::ostream* diag = nullptr);

std::string bvExprToString(const BVExpr& e);
uint32_t bvExprBitWidth(const BVExpr& e, const elab::ModuleSpec& m);
} // namespace hdl::ast