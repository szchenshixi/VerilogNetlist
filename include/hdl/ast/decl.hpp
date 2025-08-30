#pragma once
// AST-level declarations with IdString and Wire terminology.

#include <memory>
#include <variant>
#include <vector>

#include "hdl/ast/expr.hpp"
#include "hdl/common.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl {
namespace ast {

struct NetEntity {
    int mMsb = 0;
    int mLsb = 0;
    uint32_t width() const { return width_from_range(mMsb, mLsb); }
};

struct PortDecl {
    IdString mName;
    PortDirection mDir = PortDirection::In;
    NetEntity mEnt;
};

struct WireDecl {
    IdString mName;
    NetEntity mEnt;
};

struct ConnDecl {
    IdString mFormal;              // port name in callee
    std::shared_ptr<Expr> mActual; // expression in caller module scope
};

struct AssignStmt {
    std::shared_ptr<Expr> mLhs;
    std::shared_ptr<Expr> mRhs;
};

struct ParamDecl {
    IdString mName;
    int64_t mValue = 0;
};

struct IntExpr {
    // Either a literal or a parameter reference.
    std::variant<int64_t, IdString> mV;
};
inline IntExpr intConst(int64_t x) {
    IntExpr e;
    e.mV = x;
    return e;
}
inline IntExpr intParam(IdString n) {
    IntExpr e;
    e.mV = n;
    return e;
}

struct ParamAssign {
    IdString mName;
    IntExpr mValue;
};

struct InstanceDecl {
    IdString mName;
    IdString mTargetModule;
    std::vector<ParamAssign> mParamOverrides;
    std::vector<ConnDecl> mConns;
};

// Generate (minimal)
struct GenForDecl {
    IdString mLabel;   // name prefix
    IdString mLoopVar; // unused in this demo
    IntExpr mStart;
    IntExpr mLimit; // iterate i in [start, limit) by +step
    IntExpr mStep;
    std::vector<InstanceDecl> mBody; // replicated body
};

struct GenIfDecl {
    IdString mLabel; // name prefix
    IntExpr mCond;   // nonzero => thenInsts, else => elseInsts
    std::vector<InstanceDecl> mThenInsts;
    std::vector<InstanceDecl> mElseInsts;
};

struct ModuleDecl {
    IdString mName;
    std::vector<ParamDecl> mParams;
    std::vector<PortDecl> mPorts;
    std::vector<WireDecl> mWires;
    std::vector<AssignStmt> mAssigns;
    std::vector<InstanceDecl> mInstances;
    std::vector<GenForDecl> mGenFors;
    std::vector<GenIfDecl> mGenIfs;

    const PortDecl* findPort(IdString n) const {
        for (auto& p : mPorts)
            if (p.mName == n) return &p;
        return nullptr;
    }
    const WireDecl* findWire(IdString n) const {
        for (auto& x : mWires)
            if (x.mName == n) return &x;
        return nullptr;
    }
};

} // namespace ast
} // namespace hdl