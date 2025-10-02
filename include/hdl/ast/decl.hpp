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
    Expr mMsb = Expr::number(0);
    Expr mLsb = Expr::number(0);
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

using GenBlock =
  std::variant<InstanceDecl, GenIfDecl, GenForDecl, GenCaseDecl>;

struct GenIfDecl {
    IdString mLabel;
    Expr mCond;
    std::vector<GenBlock> mThenBlks;
    std::vector<GenBlock> mElseBlks;
};

struct GenForDecl {
    IdString mLabel;
    IdString mLoopVar;
    Expr mStart;
    Expr mLimit;
    Expr mStep;
    std::vector<GenBlock> mBlks;
};

struct GenCaseDecl {
    struct Item {
        std::vector<Expr> mChoices;
        bool mIsDefault = false;
        IdString mLabel;
        std::vector<GenBlock> mBlks;
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
    std::vector<GenBlock> mGenBlks;

    int findPortIndex(IdString n) const;
    int findWireIndex(IdString n) const;
};
} // namespace hdl::ast