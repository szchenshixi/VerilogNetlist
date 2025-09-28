#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <variant>

#include "hdl/ast/decl.hpp"
#include "hdl/ast/expr.hpp"
#include "hdl/elab/elaborate.hpp"
#include "hdl/elab/spec.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl::elab {

static int64_t evalIntExpr(const ast::Expr& x, const ast::ParamEnv& params,
                           std::ostream* diag = nullptr) {
    return x.visit([&](const auto& node) -> int64_t {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::IdExpr>) {
            IdString n = node.mName;
            auto it = params.find(n);
            if (it == params.end()) {
                if (diag) {
                    (*diag) << "ERROR: Unknown parameter '" << n.str()
                            << "' in IntExpr\n";
                }
                return 0;
            }
            return it->second;
        } else if constexpr (std::is_same_v<T, ast::ConstExpr>) {
            return static_cast<int64_t>(node.mValue);
        } else if constexpr (std::is_same_v<T, ast::ConcatExpr>) {
            if (diag) {
                (*diag) << "ERROR: Int value evaluation is not supported in "
                           "concat expression yet "
                        << ast::exprToString(ast::Expr(node)) << "\n";
            }
            return 0;
        } else if constexpr (std::is_same_v<T, ast::SliceExpr>) {
            if (diag) {
                (*diag) << "ERROR: Int value evaluation is not supported in "
                           "slice expression yet "
                        << ast::exprToString(ast::Expr(node)) << "\n";
            }
            return 0;
        } else {
            if (diag) {
                (*diag) << "ERROR: Unknown expression type in int value "
                           "evaluation\n";
            }
            return 0;
        }
    });
}

std::string makeModuleKey(std::string_view nameText,
                          const ast::ParamEnv& params) {
    IdString name(nameText);
    std::vector<std::pair<std::string, int64_t>> v;
    v.reserve(params.size());
    for (auto& kv : params) {
        v.emplace_back(kv.first.str(), kv.second);
    }
    std::sort(
      v.begin(), v.end(), [](auto& a, auto& b) { return a.first < b.first; });
    std::ostringstream oss;
    oss << name.str();
    if (!v.empty()) oss << "#";
    for (size_t i = 0; i < v.size(); ++i) {
        oss << v[i].first << "=" << v[i].second;
        if (i + 1 < v.size()) oss << ",";
    }
    return oss.str();
}

static const ast::ParamEnv& defaultParamEnv(const ast::ModuleDecl& decl) {
    return decl.mParams;
}

static std::string buildPrefixedName(const std::vector<std::string>& stack,
                                     const std::string& base) {
    if (stack.empty()) { return base; }
    std::string prefix = stack.front();
    for (std::size_t i = 1; i < stack.size(); ++i) {
        prefix.push_back('_');
        prefix.append(stack[i]);
    }
    prefix.push_back('_');
    prefix.append(base);
    return prefix;
}

std::unique_ptr<ModuleSpec> elaborateModule(const ast::ModuleDecl& decl,
                                            const ast::ParamEnv& paramEnv) {
    auto spec = std::make_unique<ModuleSpec>();
    spec->mName = decl.mName;
    spec->mDecl = &decl;
    spec->mParamEnv = paramEnv;

    // Ports
    spec->mPorts.reserve(decl.mPorts.size());
    for (const auto& p : decl.mPorts) {
        PortSpec ps;
        ps.mName = p.mName;
        ps.mDir = p.mDir;
        ps.mEnt = p.mEnt;
        spec->mPortIndex.emplace(ps.mName,
                                 static_cast<uint32_t>(spec->mPorts.size()));
        spec->mPorts.push_back(ps);
    }
    // Wires
    spec->mWires.reserve(decl.mWires.size());
    for (const auto& w : decl.mWires) {
        WireSpec ws;
        ws.mName = w.mName;
        ws.mEnt = w.mEnt;
        spec->mWireIndex.emplace(ws.mName,
                                 static_cast<uint32_t>(spec->mWires.size()));
        spec->mWires.push_back(ws);
    }

    // Build bit map allocation
    spec->mBitMap.build(*spec);
    return spec;
}

static bool isConnectable(const BitAtom& a) {
    return a.mKind == BitAtomKind::PortBit || a.mKind == BitAtomKind::WireBit;
}

static net::BitId toBitId(const ModuleSpec& spec, const BitAtom& a) {
    if (a.mKind == BitAtomKind::PortBit) {
        int idx = spec.findPortIndex(a.mOwnerIndex);
        if (idx < 0) return UINT32_MAX;
        return spec.mBitMap.portBit(static_cast<uint32_t>(idx), a.mBitIndex);
    }
    if (a.mKind == BitAtomKind::WireBit) {
        int idx = spec.findWireIndex(a.mOwnerIndex);
        if (idx < 0) return UINT32_MAX;
        return spec.mBitMap.wireBit(static_cast<uint32_t>(idx), a.mBitIndex);
    }
    return UINT32_MAX;
}

void wireAssigns(ModuleSpec& spec) {
    if (!spec.mDecl) return;
    FlattenContext fc(spec, &std::cerr);

    for (const auto& asg : spec.mDecl->mAssigns) {
        auto L = fc.flattenExpr(asg.mLhs);
        auto R = fc.flattenExpr(asg.mRhs);
        if (L.size() != R.size()) {
            std::cerr << "ERROR: assign width mismatch in module "
                      << spec.mName.str()
                      << " (lhs=" << ast::exprToString(asg.mLhs)
                      << ", rhs=" << ast::exprToString(asg.mRhs) << ")\n";
            continue;
        }
        for (size_t i = 0; i < L.size(); ++i) {
            const auto& l = L[i];
            const auto& r = R[i];
            if (!isConnectable(l)) {
                std::cerr << "ERROR: LHS bit not assignable (const) at bit "
                          << i << "\n";
                continue;
            }
            if (!isConnectable(r)) {
                // Ignore constants in RHS for demo.
                continue;
            }
            net::BitId bl = toBitId(spec, l);
            net::BitId br = toBitId(spec, r);
            spec.mBitMap.alias(bl, br);
        }
    }
}

ModuleSpec& getOrCreateSpec(const ast::ModuleDecl& decl,
                            const ast::ParamEnv& paramEnv,
                            ModuleLibrary& lib) {
    std::string key = makeModuleKey(decl.mName.str(), paramEnv);
    auto it = lib.find(key);
    if (it != lib.end()) return *it->second;
    auto s = elaborateModule(decl, paramEnv);
    ModuleSpec& ref = *s;
    lib.emplace(key, std::move(s));
    wireAssigns(ref);
    return ref;
}

void expandGenerateBlock(const ModuleSpec& spec, const ast::GenBlock& block,
                         const ast::ParamEnv& env,
                         const std::vector<std::string>& nameStack,
                         std::vector<ast::InstanceDecl>& out,
                         std::ostream& diag);

void expandGenIf(const ModuleSpec& spec, const ast::GenIfDecl& decl,
                 const ast::ParamEnv& env, std::vector<std::string> nameStack,
                 std::vector<ast::InstanceDecl>& out, std::ostream& diag) {
    int64_t cond = evalIntExpr(decl.mCond, env, &diag);
    const ast::GenBlock& selected = (cond != 0) ? decl.mThen : decl.mElse;
    if (selected.mItems.empty()) { return; }
    if (decl.mLabel.valid()) { nameStack.push_back(decl.mLabel.str()); }
    expandGenerateBlock(spec, selected, env, nameStack, out, diag);
}

void expandGenFor(const ModuleSpec& spec, const ast::GenForDecl& decl,
                  const ast::ParamEnv& env, std::vector<std::string> nameStack,
                  std::vector<ast::InstanceDecl>& out, std::ostream& diag) {
    int64_t start = evalIntExpr(decl.mStart, env, &diag);
    int64_t limit = evalIntExpr(decl.mLimit, env, &diag);
    int64_t step = evalIntExpr(decl.mStep, env, &diag);
    if (step == 0) {
        diag << "ERROR: gen-for step is zero in " << spec.mName.str() << "\n";
        return;
    }
    if ((step > 0 && start >= limit) || (step < 0 && start <= limit)) {
        return;
    }

    std::string label = decl.mLabel.valid() ? decl.mLabel.str() : "gen";
    int64_t iter = 0;
    for (int64_t val = start; (step > 0) ? (val < limit) : (val > limit);
         val += step, ++iter) {
        ast::ParamEnv iterEnv = env;
        iterEnv[decl.mLoopVar] = val;

        auto iterStack = nameStack;
        iterStack.push_back(label + "_" + std::to_string(iter));
        expandGenerateBlock(spec, decl.mBody, iterEnv, iterStack, out, diag);
    }
}

void expandGenerateBlock(const ModuleSpec& spec, const ast::GenBlock& block,
                         const ast::ParamEnv& env,
                         const std::vector<std::string>& nameStack,
                         std::vector<ast::InstanceDecl>& out,
                         std::ostream& diag) {
    for (const auto& item : block.mItems) {
        if (std::holds_alternative<ast::InstanceDecl>(item)) {
            auto inst = std::get<ast::InstanceDecl>(item);
            if (!nameStack.empty()) {
                inst.mName =
                  IdString(buildPrefixedName(nameStack, inst.mName.str()));
            }
            out.push_back(std::move(inst));
        } else if (std::holds_alternative<ast::GenIfDecl>(item)) {
            const auto& gi = std::get<ast::GenIfDecl>(item);
            expandGenIf(spec, gi, env, nameStack, out, diag);
        } else if (std::holds_alternative<ast::GenForDecl>(item)) {
            const auto& gf = std::get<ast::GenForDecl>(item);
            expandGenFor(spec, gf, env, nameStack, out, diag);
        } else if (std::holds_alternative<ast::GenCaseDecl>(item)) {
            const auto& gc = std::get<ast::GenCaseDecl>(item);
            // expandGenCase(spec, gc, env, nameStack, out, diag);
        } else {
            std::cerr << "Error: Unknown GenItem type\n";
        }
    }
}

static void expandGenerates(const ModuleSpec& spec,
                            const ast::ModuleDecl& decl,
                            std::vector<ast::InstanceDecl>& out,
                            std::ostream& diag) {
    for (const auto& inst : decl.mInstances) {
        out.push_back(inst);
    }

    auto baseEnv = spec.mParamEnv;

    for (const auto& g : decl.mGenIfs) {
        expandGenIf(spec, g, baseEnv, /* nameStack */ {}, out, diag);
    }

    for (const auto& gf : decl.mGenFors) {
        expandGenFor(spec, gf, baseEnv, /* nameStack */ {}, out, diag);
    }
}

void linkInstances(ModuleSpec& spec, const ASTIndex& astIndex,
                   ModuleLibrary& lib, std::ostream& diag) {
    spec.mInstances.clear();
    if (!spec.mDecl) return;

    // Expand generate constructs and gather all instances to link.
    std::vector<ast::InstanceDecl> flatInsts;
    expandGenerates(spec, *spec.mDecl, flatInsts, diag);

    // Bind each instance
    for (const auto& idecl : flatInsts) {
        // Find callee AST
        auto itAst = astIndex.find(idecl.mTargetModule);
        if (itAst == astIndex.end()) {
            diag << "ERROR: Unknown module '" << idecl.mTargetModule.str()
                 << "' for instance " << idecl.mName.str() << " in module "
                 << spec.mName.str() << "\n";
            continue;
        }
        const ast::ModuleDecl& calleeDecl = itAst->second.get();

        // Build parameter environment for callee: defaults + overrides
        auto calleeParams = defaultParamEnv(calleeDecl);
        for (const auto& [key, val] : idecl.mParamOverrides) {
            calleeParams[key] = val;
        }

        // Get/Elaborate callee spec with parameters
        ModuleSpec& callee = getOrCreateSpec(calleeDecl, calleeParams, lib);

        // Create ModuleInstance with port bindings
        hier::ModuleInstance inst;
        inst.mName = idecl.mName;
        inst.mCallee = &callee;

        FlattenContext fc(spec, &diag);
        for (const auto& c : idecl.mConns) {
            int formalIdx = callee.findPortIndex(c.mFormal);
            if (formalIdx < 0) {
                diag << "ERROR: Unknown formal port '" << c.mFormal.str()
                     << "' on instance " << idecl.mName.str() << " in module "
                     << spec.mName.str() << "\n";
                continue;
            }
            uint32_t Wf = callee.mPorts[formalIdx].width();
            BitVector actual = fc.flattenExpr(c.mActual);
            if (actual.size() != Wf) {
                diag << "ERROR: Width mismatch binding " << idecl.mName.str()
                     << "." << c.mFormal.str() << " Wf=" << Wf
                     << " Wa=" << actual.size()
                     << " actual=" << ast::exprToString(c.mActual) << "\n";
                continue;
            }
            inst.mConnections.push_back(hier::PortBinding{
              static_cast<uint32_t>(formalIdx), std::move(actual)});
        }

        spec.mInstances.push_back(std::move(inst));
    }
}

namespace hier {

static void dumpRecur(const ModuleSpec& spec, std::ostream& os,
                      const ScopeId& scope, int indent) {
    os << Indent(indent) << "Module '" << spec.mName.str()
       << "' scope=" << scope.toString() << "\n";

    if (!spec.mInstances.empty()) {
        os << Indent(indent + 2) << "Instances (" << spec.mInstances.size()
           << "):\n";
    }

    for (size_t idx = 0; idx < spec.mInstances.size(); ++idx) {
        const auto& inst = spec.mInstances[idx];
        os << Indent(indent + 4) << "[" << idx << "] " << inst.mName.str()
           << " : "
           << (inst.mCallee ? inst.mCallee->mName.str()
                            : std::string("<null>"))
           << "\n";
        if (!inst.mConnections.empty()) {
            os << Indent(indent + 6) << "Connections:\n";
            for (const auto& b : inst.mConnections) {
                const auto& p = inst.mCallee->mPorts[b.mFormalIndex];
                os << Indent(indent + 8) << p.mName.str() << " ("
                   << to_string(p.mDir) << ") <= [";
                for (size_t i = 0; i < b.mActual.size(); ++i) {
                    const auto& a = b.mActual[i];
                    std::string label;
                    if (a.mKind == BitAtomKind::PortBit) {
                        label = "port " + a.mOwnerIndex.str() + "[off " +
                                std::to_string(a.mBitIndex) + "]";
                    } else if (a.mKind == BitAtomKind::WireBit) {
                        label = "wire " + a.mOwnerIndex.str() + "[off " +
                                std::to_string(a.mBitIndex) + "]";
                    } else {
                        label = (a.mKind == BitAtomKind::Const1) ? "1" : "0";
                    }
                    os << label;
                    if (i + 1 < b.mActual.size()) os << ", ";
                }
                os << "]\n";
            }
        }

        // Recurse into callee spec
        if (inst.mCallee) {
            ScopeId childScope = scope;
            childScope.mPath.push_back(static_cast<uint32_t>(idx));
            dumpRecur(*inst.mCallee, os, childScope, indent + 4);
        }
    }
}

void dumpInstanceTree(const ModuleSpec& top, std::ostream& os) {
    ScopeId root;
    dumpRecur(top, os, root, 0);
}

bool makePinKey(const ModuleSpec& top, const ScopeId& scope, IdString portName,
                PinKey& out, std::ostream* diag) {
    const ModuleSpec* cur = &top;
    for (size_t depth = 0; depth < scope.mPath.size(); ++depth) {
        uint32_t idx = scope.mPath[depth];
        if (idx >= cur->mInstances.size()) {
            if (diag) {
                (*diag) << "ERROR: Scope path index " << idx
                        << " out of range at depth " << depth << "\n";
            }
            return false;
        }
        const auto& inst = cur->mInstances[idx];
        if (!inst.mCallee) {
            if (diag) {
                (*diag) << "ERROR: Null callee at depth " << depth << "\n";
            }
            return false;
        }
        cur = inst.mCallee;
    }
    int pIdx = cur->findPortIndex(portName);
    if (pIdx < 0) {
        if (diag) { (*diag) << "ERROR: No such port in module\n"; }
        return false;
    }
    out.mScope = scope;
    out.mPortIndex = static_cast<uint32_t>(pIdx);
    return true;
}

} // namespace hier

} // namespace hdl::elab