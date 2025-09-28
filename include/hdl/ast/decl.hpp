#pragma once
// AST-level declarations with IdString and Wire terminology.

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "hdl/ast/expr.hpp"
#include "hdl/common.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl::ast {

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
    IdString mFormal; // port name in callee
    Expr mActual;     // expression in caller module scope
};

struct AssignStmt {
    Expr mLhs;
    Expr mRhs;
};

using ParamEnv = std::unordered_map<IdString, int64_t, IdString::Hash>;

struct InstanceDecl {
    IdString mName;
    IdString mTargetModule;
    ParamEnv mParamOverrides;
    std::vector<ConnDecl> mConns;
};

// Generate (minimal)
struct GenIfDecl;
struct GenForDecl;
struct GenCaseDecl;

using GenItem = std::variant<InstanceDecl, GenIfDecl, GenForDecl, GenCaseDecl>;

struct GenBlock {
    std::vector<GenItem> mItems;

    bool empty() const { return mItems.empty(); }
    void clear() { mItems.clear(); }
};

struct GenIfDecl {
    IdString mLabel;
    Expr mCond;
    GenBlock mThen;
    GenBlock mElse;
};

struct GenForDecl {
    IdString mLabel;
    IdString mLoopVar;
    Expr mStart;
    Expr mLimit;
    Expr mStep;
    GenBlock mBody;
};

struct GenCaseDecl {
    struct Item {
        std::vector<Expr> mChoices;
        bool mIsDefault = false;
        IdString mLabel;
        GenBlock mBody;
    };
    IdString mLabel;
    Expr mExpr;
    std::vector<Item> mItems;
};

struct ModuleDecl {
    IdString mName;
    ParamEnv mParams;
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

} // namespace hdl::ast