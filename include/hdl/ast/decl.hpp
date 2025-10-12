#pragma once
// AST-level declarations with IdString and Wire terminology.

#include <algorithm>
#include <variant>
#include <vector>

#include "hdl/ast/expr.hpp"
#include "hdl/common.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl::ast {

struct NetDecl {
    IntExpr mMsb = IntExpr::number(0);
    IntExpr mLsb = IntExpr::number(0);
};

struct PortDecl {
    IdString mName;
    PortDirection mDir = PortDirection::In;
    NetDecl mNet;
};

struct WireDecl {
    IdString mName;
    NetDecl mNet;
};

struct ConnDecl {
    IdString mFormal; // port name in callee
    BVExpr mActual;   // expression in caller module scope
};

struct AssignDecl {
    BVExpr mLhs;
    BVExpr mRhs;
};

struct InstanceDecl {
    IdString mName;
    IdString mTargetModule;
    ParamDecl mOverrides;
    std::vector<ConnDecl> mConns;
};

// Generate (minimal)
struct GenIfDecl;
struct GenForDecl;
struct GenCaseDecl;

using GenBody =
  std::variant<InstanceDecl, GenIfDecl, GenForDecl, GenCaseDecl>;

struct GenIfDecl {
    IdString mLabel;
    IntExpr mCond;
    std::vector<GenBody> mThenBlks;
    std::vector<GenBody> mElseBlks;
};

struct GenForDecl {
    IdString mLabel;
    IdString mLoopVar;
    IntExpr mStart;
    IntExpr mLimit;
    IntExpr mStep;
    std::vector<GenBody> mBlks;
};

struct GenCaseDecl {
    struct Item {
        std::vector<IntExpr> mChoices;
        bool mIsDefault = false;
        IdString mLabel;
        std::vector<GenBody> mBlks;
    };
    IdString mLabel;
    IntExpr mExpr;
    std::vector<Item> mItems;
};

struct GenBlockDecl {
    std::vector<InstanceDecl> mInstances;
    std::vector<AssignDecl> mAssigns;
    std::vector<NetDecl> mNets;
};

struct ModuleDecl {
    IdString mName;
    elab::ParamSpec mDefaults;
    std::vector<PortDecl> mPorts;
    std::vector<WireDecl> mWires;
    std::vector<AssignDecl> mAssigns;
    std::vector<InstanceDecl> mInstances;
    std::vector<GenBody> mGenBlks;

    int findPortIndex(IdString n) const;
    int findWireIndex(IdString n) const;
};
} // namespace hdl::ast