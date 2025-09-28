#pragma once
// Expression AST using std::variant: IdExpr, ConstExpr, ConcatExpr, SliceExpr.

#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "hdl/util/id_string.hpp"

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
    int mMsb = 0;
    int mLsb = 0;
};

struct Expr {
    using Variant = std::variant<IdExpr, ConstExpr, ConcatExpr, SliceExpr>;
    Variant mNode;

    Expr() = default;
    explicit Expr(Variant v)
        : mNode(std::move(v)) {}

    // Factory methods that return by value
    static Expr id(IdString n) { return Expr(IdExpr{n}); }

    static Expr number(uint64_t v, int w = 0, const std::string& t = "") {
        return Expr(ConstExpr{v, w, t});
    }

    static Expr concat(std::vector<Expr> parts) {
        return Expr(ConcatExpr{std::move(parts)});
    }

    static Expr slice(IdString baseId, int msb, int lsb) {
        return Expr(SliceExpr{baseId, msb, lsb});
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
    // concat_expr.visit([](const auto& expr) { handle each expression type });
    template <typename Visitor>
    auto visit(Visitor&& vis) const {
        return std::visit(std::forward<Visitor>(vis), mNode);
    }
};

// Forward decl
struct ModuleDecl;

// Helpers implemented in src/ast/expr.cpp
uint32_t exprBitWidth(const Expr& e, const ModuleDecl& m);
std::string exprToString(const Expr& e);

} // namespace hdl::ast