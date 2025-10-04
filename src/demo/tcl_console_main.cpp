#include <iostream>
#include <memory>

#include "hdl/ast/decl.hpp"
#include "hdl/ast/expr.hpp"
#include "hdl/elab/elaborate.hpp"
#include "hdl/tcl/console.hpp"

using namespace hdl;
using namespace hdl::ast;
using namespace hdl::elab;

// Reuse the demo modules from your library for bootstrapping
static NetDecl W(int msb, int lsb) {
    return {Expr::number(msb), Expr::number(lsb)};
}
static PortDecl P(IdString n, PortDirection d, int msb, int lsb) {
    return PortDecl{n, d, W(msb, lsb)};
}
static WireDecl RW(IdString n, int msb, int lsb) {
    return WireDecl{n, W(msb, lsb)};
}

int main() {
    // Build a tiny default AST set so the console is usable out-of-the-box.
    IdString A("A"), Top("Top"), p_in("p_in"), p_out("p_out");
    IdString w0("w0"), w1("w1"), w2("w2"), w3("w3");
    IdString DO_EXTRA("DO_EXTRA"), REPL("REPL");
    // Module A
    ModuleDecl modA;
    modA.mName = A;
    modA.mPorts.push_back(P(p_in, PortDirection::In, 7, 0));
    modA.mPorts.push_back(P(p_out, PortDirection::Out, 7, 0));
    {
        auto lhs = Expr::id(p_out);
        auto rhs =
          Expr::concat({Expr::slice(p_in, 3, 0), Expr::slice(p_in, 7, 4)});
        modA.mAssigns.push_back(AssignStmt{lhs, rhs});
    }
    // Top
    ModuleDecl top;
    top.mName = Top;
    top.mParams.emplace(DO_EXTRA, 1);
    top.mParams.emplace(REPL, 2);
    top.mWires.push_back(RW(w0, 7, 0));
    top.mWires.push_back(RW(w1, 7, 0));
    top.mWires.push_back(RW(w2, 7, 0));
    top.mWires.push_back(RW(w3, 7, 0));
    {
        InstanceDecl inst;
        inst.mName = IdString("uA");
        inst.mTargetModule = A;
        inst.mConns.push_back(ConnDecl{p_in, Expr::id(w0)});
        inst.mConns.push_back(ConnDecl{p_out, Expr::id(w1)});
        top.mInstances.push_back(inst);
    }

    // AST index
    elab::ASTIndex idx;
    idx.emplace(modA.mName, std::cref(modA));
    idx.emplace(top.mName, std::cref(top));

    // Library
    ModuleLibrary lib;

    // Pre-elaborate A and Top defaults to make things easier
    ModuleSpec& specA = getOrCreateSpec(modA, {}, lib);
    ModuleSpec& specTop =
      getOrCreateSpec(top, {{DO_EXTRA, 1}, {REPL, 2}}, lib);
    linkInstances(specTop, idx, lib, std::cerr);

    // Start the Tcl console
    hdl::tcl::Console console(lib, idx, std::cerr);
    if (!console.init()) {
        std::cerr << "Failed to init Tcl console\n";
        return 1;
    }

    // Build the specialization key for Top with defaults so Console can find
    auto topEnv = ParamSpec{{DO_EXTRA, 1}, {REPL, 2}};
    IdString topKey(elab::makeModuleKey(Top.str(), topEnv));
    if (console.getSpecByKey(topKey)) {
        console.selection().mModuleKeys.push_back(topKey);
        console.selection().mPrimaryKey = topKey;
    }

    return console.repl();
}