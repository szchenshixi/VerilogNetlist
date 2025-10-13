#include <iostream>
#include <memory>

#include "hdl/ast/decl.hpp"
#include "hdl/ast/expr.hpp"
#include "hdl/elab/elaborate.hpp"
#include "hdl/tcl/console.hpp"

using namespace hdl;
using namespace hdl::ast;
using namespace hdl::elab;

template <typename T>
constexpr auto makeIntExpr(T&& value) {
    if constexpr (std::is_integral_v<std::decay_t<T>>) {
        return IntExpr::number(value);
    } else {
        return std::forward<T>(value);
    }
}

template <typename T1, typename T2>
static NetDecl n(T1&& msb, T2&& lsb) {
    return {makeIntExpr(msb), makeIntExpr(lsb)};
}
template <typename T1, typename T2>
static PortDecl p(IdString name, PortDirection dir, T1 msb, T2 lsb) {
    return PortDecl{name, dir, n(msb, lsb)};
}
template <typename T1, typename T2>
static WireDecl w(IdString name, T1 msb, T2 lsb) {
    return WireDecl{name, n(msb, lsb)};
}

int main() {
    // Build a tiny default AST set so the console is usable out-of-the-box.
    IdString A("A"), Top("Top"), p_in("p_in"), p_out("p_out");
    IdString w0("w0"), w1("w1"), w2("w2"), w3("w3");
    IdString DO_EXTRA("DO_EXTRA"), REPL("REPL");

    // Module declarations
    ModuleDeclLib declLib;
    // Module specifications
    ModuleSpecLib specLib;
    {
        // Module A
        ModuleDecl modA;
        modA.mName = A;
        modA.mPorts.push_back(p(p_in, PortDirection::In, 7, 0));
        modA.mPorts.push_back(p(p_out, PortDirection::Out, 7, 0));
        {
            auto lhs = BVExpr::id(p_out);
            auto rhs = BVExpr::concat(
              {BVExpr::slice(p_in, 3, 0), BVExpr::slice(p_in, 7, 4)});
            modA.mAssigns.push_back(AssignDecl{lhs, rhs});
        }
        // Top
        ModuleDecl top;
        top.mName = Top;
        top.mDefaults.emplace(DO_EXTRA, 1);
        top.mDefaults.emplace(REPL, 2);
        top.mWires.push_back(w(w0, 7, 0));
        top.mWires.push_back(w(w1, 7, 0));
        top.mWires.push_back(w(w2, 7, 0));
        top.mWires.push_back(w(w3, 7, 0));
        {
            InstanceDecl inst;
            inst.mName = IdString("uA");
            inst.mTargetModule = A;
            inst.mConns.push_back(ConnDecl{p_in, BVExpr::id(w0)});
            inst.mConns.push_back(ConnDecl{p_out, BVExpr::id(w1)});
            top.mInstances.push_back(inst);
        }

        declLib.emplace(modA.mName, std::move(modA));
        declLib.emplace(top.mName, std::move(top));
    }

    // Pre-elaborate A and Top defaults to make things easier
    ModuleSpec& specA = getOrCreateSpec(declLib[A], {}, specLib);
    ModuleSpec& specTop =
      getOrCreateSpec(declLib[Top], {{DO_EXTRA, 1}, {REPL, 2}}, specLib);
    linkInstances(specTop, declLib, specLib, &std::cerr);

    // Start the Tcl console
    hdl::tcl::Console console(specLib, declLib, std::cerr);
    if (!console.init()) {
        std::cerr << "Failed to init Tcl console\n";
        return 1;
    }

    // Build the specialization key for Top with defaults so Console can find
    auto topKey = makeModuleKey(specTop.mName.str(), specTop.mEnv);
    if (console.getSpecByKey(topKey)) {
        console.selection().mModuleKeys.push_back(IdString(topKey));
        console.selection().mPrimaryKey = IdString(topKey);
    }

    return console.repl();
}