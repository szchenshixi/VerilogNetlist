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
    std::vector<std::shared_ptr<Expr>> mParts;
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

    static std::shared_ptr<Expr> id(IdString n) {
        return std::make_shared<Expr>(IdExpr{n});
    }
    static std::shared_ptr<Expr> number(uint64_t v, int w = 0,
                                        const std::string& t = "") {
        return std::make_shared<Expr>(ConstExpr{v, w, t});
    }
    static std::shared_ptr<Expr>
    concat(std::vector<std::shared_ptr<Expr>> parts) {
        ConcatExpr c;
        c.mParts = std::move(parts);
        return std::make_shared<Expr>(std::move(c));
    }
    static std::shared_ptr<Expr> slice(IdString baseId, int msb, int lsb) {
        return std::make_shared<Expr>(SliceExpr{baseId, msb, lsb});
    }
};

// Forward decl
struct ModuleDecl;

// Helpers implemented in src/ast/expr.cpp
uint32_t exprBitWidth(const Expr& e, const ModuleDecl& m);
std::string exprToString(const Expr& e);

} // namespace hdl::ast