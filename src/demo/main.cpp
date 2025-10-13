#include <iostream>

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
    e.mMsb = IntExpr::number(msb);
    e.mLsb = IntExpr::number(lsb);
    return e;
}

static PortDecl makePort(IdString name, PortDirection dir, int msb, int lsb) {
    PortDecl p;
    p.mName = name;
    p.mDir = dir;
    p.mNet = makeNet(msb, lsb);
    return p;
}

static WireDecl makeWire(IdString name, int msb, int lsb) {
    WireDecl w;
    w.mName = name;
    w.mNet = makeNet(msb, lsb);
    return w;
}

int main() {
    // Symbols
    IdString A = IdString("A");
    IdString Top = IdString("Top");
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

    // Module declaration
    ModuleDeclLib declLib;
    // Module specification
    ModuleSpecLib specLib;
    {
        // Module A
        ModuleDecl declA;
        declA.mName = A;
        declA.mPorts.push_back(makePort(p_in, PortDirection::In, 7, 0));
        declA.mPorts.push_back(makePort(p_out, PortDirection::Out, 7, 0));
        {
            auto lhs = BVExpr::id(p_out);
            auto s0 = BVExpr::slice(p_in, 3, 0);
            auto s1 = BVExpr::slice(p_in, 7, 4);
            std::vector<BVExpr> parts;
            parts.emplace_back(s0); // MSB part
            parts.emplace_back(s1); // LSB part
            auto rhs = BVExpr::concat(std::move(parts));
            AssignDecl asg;
            asg.mLhs = lhs;
            asg.mRhs = rhs;
            declA.mAssigns.push_back(std::move(asg));
        }

        // Top with params and generates
        ModuleDecl declTop;
        declTop.mName = Top;
        declTop.mDefaults.emplace(DO_EXTRA, 1);
        declTop.mDefaults.emplace(REPL, 2);
        declTop.mWires.push_back(makeWire(w0, 7, 0));
        declTop.mWires.push_back(makeWire(w1, 7, 0));
        declTop.mWires.push_back(makeWire(w2, 7, 0));
        declTop.mWires.push_back(makeWire(w3, 7, 0));
        {
            InstanceDecl inst;
            inst.mName = uA;
            inst.mTargetModule = A;
            ConnDecl c0{p_in, BVExpr::id(w0)};
            ConnDecl c1{p_out, BVExpr::id(w1)};
            inst.mConns.push_back(std::move(c0));
            inst.mConns.push_back(std::move(c1));
            declTop.mInstances.push_back(std::move(inst));
        }
        {
            GenIfDecl gi;
            gi.mLabel = g_if;
            gi.mCond = IntExpr::id(DO_EXTRA);
            InstanceDecl uAx;
            uAx.mName = uA_extra;
            uAx.mTargetModule = A;
            ConnDecl c0{p_in, BVExpr::id(w2)};
            ConnDecl c1{p_out, BVExpr::id(w3)};
            uAx.mConns.push_back(std::move(c0));
            uAx.mConns.push_back(std::move(c1));
            gi.mThenBlks.push_back(std::move(uAx));
            declTop.mGenBlks.push_back(std::move(gi));
        }
        {
            GenForDecl gf;
            gf.mLabel = g_for;
            gf.mLoopVar = IdString("i");
            gf.mStart = IntExpr::number(0);
            gf.mLimit = IntExpr::id(REPL);
            gf.mStep = IntExpr::number(1);
            InstanceDecl uAr;
            uAr.mName = uA_rep;
            uAr.mTargetModule = A;
            ConnDecl c0{p_in, BVExpr::id(w0)};
            ConnDecl c1{p_out, BVExpr::id(w1)};
            uAr.mConns.push_back(std::move(c0));
            uAr.mConns.push_back(std::move(c1));
            gf.mBlks.push_back(std::move(uAr));
            declTop.mGenBlks.push_back(std::move(gf));
        }

        declLib.emplace(declA.mName, std::move(declA));
        declLib.emplace(declTop.mName, std::move(declTop));
    }

    // Elaborate A (no params)
    auto envA = ParamSpec{};
    ModuleSpec& modA = getOrCreateSpec(declLib[A], envA, specLib);

    // Elaborate Top with defaults
    auto envTop = ParamSpec{{DO_EXTRA, 1}, {REPL, 2}};
    ModuleSpec& modTop = getOrCreateSpec(declLib[Top], envTop, specLib);

    // Link instances inside each module
    linkInstances(modTop, declLib, specLib, &std::cerr);
    linkInstances(modA, declLib, specLib, &std::cerr);

    // Print layouts
    std::cout << "=== Layouts ===\n";
    modA.dumpLayout(std::cout);
    modTop.dumpLayout(std::cout);

    // Print connectivity
    std::cout << "\n=== Connectivity: A ===\n";
    modA.dumpConnectivity(std::cout);

    std::cout << "\n=== Connectivity: Top ===\n";
    modTop.dumpConnectivity(std::cout);

    // Dump hierarchy starting from Top
    std::cout << "\n=== Instance Hierarchy (ModuleSpec -> InstanceSpec ->"
                 "ModuleSpec) ===\n";
    hier::dumpInstanceTree(modTop, std::cout);

    // Sample PinKey: first child of Top, port p_in
    std::cout << "\n=== PinKey sample ===\n";
    hier::ScopeId s;
    if (!modTop.mInstances.empty()) s.mPath.push_back(0);
    hier::PinKey pk;
    if (hier::makePinKey(modTop, s, p_in, pk, &std::cerr)) {
        std::cout << "PinKey scope=" << pk.mScope.toString()
                  << " portIndex=" << pk.mPortIndex << "\n";
    }

    // Export Top view JSON
    auto jTop = vis::buildViewJson(modTop);

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
