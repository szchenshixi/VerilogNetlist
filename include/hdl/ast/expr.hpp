#pragma once
// Expression AST using std::variant: IdExpr, ConstExpr, ConcatExpr, SliceExpr.

#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "hdl/util/id_string.hpp"

namespace hdl::elab {
struct ModuleSpec;
}
namespace hdl::ast {
struct Expr;
struct IdExpr {
    IdString mName;
};
struct ConstExpr {
    uint64_t mValue = 0;
    int mWidth = 0;    // 0 => infer minimal
    std::string mText; // pretty
};
struct ConcatExpr {
    // Parts MSB -> LSB
    std::vector<Expr> mParts;
};
struct SliceExpr {
    // Base must resolve to Id at elaboration; we store identifier directly.
    IdString mBaseId;
    std::shared_ptr<Expr> mMsb;
    std::shared_ptr<Expr> mLsb;
    // std::unique_ptr<Expr> mMsb;
    // std::unique_ptr<Expr> mLsb;
};
enum class OpType { Add, Sub };
struct OpExpr {
    OpType mOp;
    std::vector<Expr> mOperands;
};

struct Expr {
    using Variant =
      std::variant<IdExpr, ConstExpr, ConcatExpr, SliceExpr, OpExpr>;
    Variant mNode;

    Expr() = default;
    explicit Expr(Variant v)
        : mNode(std::move(v)) {}

    static Expr id(IdString n) { return Expr(IdExpr{n}); }
    static Expr number(uint64_t v, int w = 0, const std::string& t = "") {
        return Expr(ConstExpr{v, w, t});
    }
    static Expr concat(const std::vector<Expr>& parts) {
        return Expr(ConcatExpr{parts}); // Copy the vector
    }
    static Expr concat(std::vector<Expr>&& parts) {
        return Expr(ConcatExpr{std::move(parts)});
    }
    static Expr slice(IdString baseId, int msb, int lsb) {
        return slice(baseId, Expr::number(msb), Expr::number(lsb));
    }
    static Expr slice(const IdString& baseId, const Expr& msb,
                      const Expr& lsb) {
        return Expr(SliceExpr{baseId,
                              std::make_shared<Expr>(msb),   // Copy msb
                              std::make_shared<Expr>(lsb)}); // Copy lsb
    }
    static Expr slice(IdString baseId, Expr&& msb, Expr&& lsb) {
        return Expr(SliceExpr{baseId,
                              std::make_shared<Expr>(std::move(msb)),
                              std::make_shared<Expr>(std::move(lsb))});
    }

    static Expr unary(OpType op, const Expr& operand) {
        std::vector<Expr> operands_t;
        operands_t.push_back(operand); // Copy operand
        return Expr{OpExpr{op, std::move(operands_t)}};
    }
    static Expr unary(OpType op, Expr&& operand) {
        std::vector<Expr> operands_t;
        operands_t.push_back(std::move(operand));
        return Expr{OpExpr{op, std::move(operands_t)}};
    }

    static Expr binary(OpType op, const Expr& left, const Expr& right) {
        std::vector<Expr> operands_t;
        operands_t.reserve(2);
        operands_t.push_back(left);  // Copy left
        operands_t.push_back(right); // Copy right
        return Expr{OpExpr{op, std::move(operands_t)}};
    }
    static Expr binary(OpType op, Expr&& left, Expr&& right) {
        std::vector<Expr> operands_t;
        operands_t.reserve(2);
        operands_t.push_back(std::move(left));
        operands_t.push_back(std::move(right));
        return Expr{OpExpr{op, std::move(operands_t)}};
    }

    static Expr add(const Expr& left, const Expr& right) {
        return binary(OpType::Add, left, right);
    }
    static Expr add(Expr&& left, Expr&& right) {
        return binary(OpType::Add, std::move(left), std::move(right));
    }
    static Expr sub(const Expr& left, const Expr& right) {
        return binary(OpType::Sub, left, right);
    }
    static Expr sub(Expr&& left, Expr&& right) {
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
using ParamSpec = std::unordered_map<IdString, int64_t, IdString::Hash>;

// Helpers implemented in src/ast/expr.cpp
uint32_t exprBitWidth(const Expr& e, const elab::ModuleSpec& m);
std::string exprToString(const Expr& e);
int64_t exprToInt64(const Expr& e, const ParamSpec& p = {});

} // namespace hdl::ast