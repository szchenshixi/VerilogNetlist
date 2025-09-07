#pragma once
// Flatten expressions to BitVector (LSB-first), producing bit atoms.

#include <iostream>
#include <string>
#include <vector>

#include "hdl/ast/expr.hpp"
#include "hdl/elab/bits.hpp"
#include "hdl/elab/spec.hpp"

namespace hdl::elab {

struct FlattenContext {
    const ModuleSpec& mSpec;
    std::ostream* mDiag = nullptr;

    FlattenContext(const ModuleSpec& s, std::ostream* d = nullptr)
        : mSpec(s)
        , mDiag(d) {}

    BitVector flattenId(IdString name) const;
    BitVector flattenNumber(uint64_t value, int width) const;
    BitVector flattenSlice(const ast::SliceExpr& s) const;
    BitVector flattenConcat(const ast::ConcatExpr& c) const;
    BitVector flattenExpr(const ast::Expr& e) const;

    void error(const std::string& msg) const {
        if (mDiag) (*mDiag) << "ERROR: " << msg << "\n";
    }
};

} // namespace hdl::elab