#pragma once
// Flatten expressions to BitVector (LSB-first), producing bit atoms.

#include <iostream>
#include <string>

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
    BitVector flattenSlice(const ast::BVSlice& s) const;
    BitVector flattenConcat(const ast::BVConcat& c) const;
    BitVector flattenExpr(const ast::BVExpr& e) const;

    void warn(const std::string& msg) const;
    void error(const std::string& msg) const;
};

} // namespace hdl::elab