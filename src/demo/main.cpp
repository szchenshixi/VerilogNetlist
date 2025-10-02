#include <iostream>
#include <memory>

#include "hdl/ast/decl.hpp"
#include "hdl/elab/elaborate.hpp"
#include "hdl/elab/spec.hpp"
#include "hdl/util/id_string.hpp"
#include "hdl/vis/json.hpp"

using namespace hdl;
using namespace hdl::ast;
using namespace hdl::elab;

static NetDecl makeNet(int msb, int lsb) {
    NetDecl e;
    e.mMsb = Expr::number(msb);
    e.mLsb = Expr::number(lsb);
    return e;
}

static ast::PortDecl makePort(IdString name, PortDirection dir, int msb,
                              int lsb) {
    ast::PortDecl p;
    p.mName = name;
    p.mDir = dir;
    p.mNet = makeNet(msb, lsb);
    return p;
}

static ast::WireDecl makeWire(IdString name, int msb, int lsb) {
    ast::WireDecl w;
    w.mName = name;
    w.mNet = makeNet(msb, lsb);
    return w;
}

int main() {
    // Symbols
    IdString A_sym = IdString("A");
    IdString Top_sym = IdString("Top");
    IdString p_in = IdString("p_in");
    IdString p_out = IdString("p_out");
    IdString w0 = IdString("w0");
    IdString w1 = IdString("w1");
    IdString w2 = IdString("w2");
    IdString w3 = IdString("w3");
    IdString DO_EXTRA = IdString("DO_EXTRA");
    IdString REPL = IdString("REPL");
    IdString uA = IdString("uA");
    IdString uA_extra = IdString("uA_extra");
    IdString uA_rep = IdString("uA_rep");
    IdString g_if = IdString("g_if");
    IdString g_for = IdString("g_for");

    // Module A
    ast::ModuleDecl modA;
    modA.mName = A_sym;
    modA.mPorts.push_back(makePort(p_in, PortDirection::In, 7, 0));
    modA.mPorts.push_back(makePort(p_out, PortDirection::Out, 7, 0));
    {
        auto lhs = ast::Expr::id(p_out);
        auto s0 = ast::Expr::slice(p_in, 3, 0);
        auto s1 = ast::Expr::slice(p_in, 7, 4);
        std::vector<ast::Expr> parts;
        parts.emplace_back(s0); // MSB part
        parts.emplace_back(s1); // LSB part
        auto rhs = ast::Expr::concat(std::move(parts));
        ast::AssignStmt asg;
        asg.mLhs = lhs;
        asg.mRhs = rhs;
        modA.mAssigns.push_back(std::move(asg));
    }

    // Top with params and generates
    ast::ModuleDecl top;
    top.mName = Top_sym;
    top.mParams.emplace(DO_EXTRA, 1);
    top.mParams.emplace(REPL, 2);
    top.mWires.push_back(makeWire(w0, 7, 0));
    top.mWires.push_back(makeWire(w1, 7, 0));
    top.mWires.push_back(makeWire(w2, 7, 0));
    top.mWires.push_back(makeWire(w3, 7, 0));
    {
        ast::InstanceDecl inst;
        inst.mName = uA;
        inst.mTargetModule = A_sym;
        ast::ConnDecl c0{p_in, ast::Expr::id(w0)};
        ast::ConnDecl c1{p_out, ast::Expr::id(w1)};
        inst.mConns.push_back(std::move(c0));
        inst.mConns.push_back(std::move(c1));
        top.mInstances.push_back(std::move(inst));
    }
    {
        ast::GenIfDecl gi;
        gi.mLabel = g_if;
        gi.mCond = ast::Expr::id(DO_EXTRA);
        ast::InstanceDecl uAx;
        uAx.mName = uA_extra;
        uAx.mTargetModule = A_sym;
        ast::ConnDecl c0{p_in, ast::Expr::id(w2)};
        ast::ConnDecl c1{p_out, ast::Expr::id(w3)};
        uAx.mConns.push_back(std::move(c0));
        uAx.mConns.push_back(std::move(c1));
        gi.mThenBlks.push_back(std::move(uAx));
        top.mGenBlks.push_back(std::move(gi));
    }
    {
        ast::GenForDecl gf;
        gf.mLabel = g_for;
        gf.mLoopVar = IdString("i");
        gf.mStart = ast::Expr::number(0);
        gf.mLimit = ast::Expr::id(REPL);
        gf.mStep = ast::Expr::number(1);
        ast::InstanceDecl uAr;
        uAr.mName = uA_rep;
        uAr.mTargetModule = A_sym;
        ast::ConnDecl c0{p_in, ast::Expr::id(w0)};
        ast::ConnDecl c1{p_out, ast::Expr::id(w1)};
        uAr.mConns.push_back(std::move(c0));
        uAr.mConns.push_back(std::move(c1));
        gf.mBlks.push_back(std::move(uAr));
        top.mGenBlks.push_back(std::move(gf));
    }

    // Build AST index
    elab::ASTIndex astIndex;
    astIndex.emplace(modA.mName, std::cref(modA));
    astIndex.emplace(top.mName, std::cref(top));

    // Build module library and elaborate with parameters
    ModuleLibrary lib;

    // Elaborate A (no params)
    auto envA = ParamEnv{};
    elab::ModuleSpec& A = getOrCreateSpec(modA, envA, lib);

    // Elaborate Top with defaults
    auto envTop = ParamEnv{
      {DO_EXTRA, 1}, {REPL, 2}};
    elab::ModuleSpec& Top = getOrCreateSpec(top, envTop, lib);

    // Link instances inside each module
    linkInstances(Top, astIndex, lib, std::cerr);
    linkInstances(A, astIndex, lib, std::cerr);

    // Print layouts
    std::cout << "=== Layouts ===\n";
    A.dumpLayout(std::cout);
    Top.dumpLayout(std::cout);

    // Print connectivity
    std::cout << "\n=== Connectivity: A ===\n";
    A.dumpConnectivity(std::cout);

    std::cout << "\n=== Connectivity: Top ===\n";
    Top.dumpConnectivity(std::cout);

    // Dump hierarchy starting from Top
    std::cout << "\n=== Instance Hierarchy (ModuleSpec -> ModuleInstance ->"
                 "ModuleSpec) ===\n";
    hier::dumpInstanceTree(Top, std::cout);

    // Sample PinKey: first child of Top, port p_in
    std::cout << "\n=== PinKey sample ===\n";
    hier::ScopeId s;
    if (!Top.mInstances.empty()) s.mPath.push_back(0);
    hier::PinKey pk;
    if (hier::makePinKey(Top, s, p_in, pk, &std::cerr)) {
        std::cout << "PinKey scope=" << pk.mScope.toString()
                  << " portIndex=" << pk.mPortIndex << "\n";
    }

    // Export Top view JSON
    auto jTop = vis::buildViewJson(Top);

    // Add mock timing path
    vis::TimingPath tp;
    tp.mId = "tp0";
    tp.mName = "w0[4] -> uA.p_in[4] -> uA.p_out[0] -> w1[0]";
    tp.mSlackNs = -0.12;
    tp.mDelayNs = 1.42;
    tp.mStart = {"w0", 4};
    tp.mEnd = {"w1", 0};
    tp.mArcs.push_back({"w0", "uA.p_in", 4, 4, 0.10, "net w0[4]"});
    tp.mArcs.push_back({"uA.p_in", "uA.p_out", 4, 0, 1.20, "assign 4→0"});
    tp.mArcs.push_back({"uA.p_out", "w1", 0, 0, 0.12, "net w1[0]"});
    vis::addTimingPathsToViewJson(jTop, {tp});

    // Write to file
    vis::writeJsonFile("view_top.json", jTop);

    std::cout << "Wrote view_top.json (load it in the visualizer).\n";

    std::cout << "\nDone.\n";
    return 0;
}
