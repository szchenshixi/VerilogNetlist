#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>

#include "hdl/ast/decl.hpp"
#include "hdl/ast/expr.hpp"
#include "hdl/elab/elaborate.hpp"
#include "hdl/elab/spec.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl::elab {

static int64_t evalIntExpr(
  const ast::IntExpr& x,
  const std::unordered_map<IdString, int64_t, IdString::Hash>& params,
  std::ostream* diag = nullptr) {
    if (std::holds_alternative<int64_t>(x.mV)) return std::get<int64_t>(x.mV);
    IdString n = std::get<IdString>(x.mV);
    auto it = params.find(n);
    if (it == params.end()) {
        if (diag) {
            (*diag) << "ERROR: Unknown parameter '" << n.str()
                    << "' in IntExpr\n";
        }
        return 0;
    }
    return it->second;
}

std::string makeModuleKey(
  std::string_view nameText,
  const std::unordered_map<IdString, int64_t, IdString::Hash>& params) {
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

static std::unordered_map<IdString, int64_t, IdString::Hash>
defaultParamEnv(const ast::ModuleDecl& decl) {
    std::unordered_map<IdString, int64_t, IdString::Hash> env;
    for (auto& p : decl.mParams)
        env[p.mName] = p.mValue;
    return env;
}

std::unique_ptr<ModuleSpec> elaborateModule(
  const ast::ModuleDecl& decl,
  const std::unordered_map<IdString, int64_t, IdString::Hash>& paramEnv) {
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
        int idx = spec.findPortIndex(a.mOwnerIndex);
        if (idx < 0) return UINT32_MAX;
        return spec.mBitMap.wireBit(static_cast<uint32_t>(idx), a.mBitIndex);
    }
    return UINT32_MAX;
}

void wireAssigns(ModuleSpec& spec) {
    if (!spec.mDecl) return;
    FlattenContext fc(spec, &std::cerr);

    for (const auto& asg : spec.mDecl->mAssigns) {
        if (!asg.mLhs || !asg.mRhs) continue;
        auto L = fc.flattenExpr(*asg.mLhs);
        auto R = fc.flattenExpr(*asg.mRhs);
        if (L.size() != R.size()) {
            std::cerr << "ERROR: assign width mismatch in module "
                      << spec.mName.str()
                      << " (lhs=" << ast::exprToString(*asg.mLhs)
                      << ", rhs=" << ast::exprToString(*asg.mRhs) << ")\n";
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

ModuleSpec& getOrCreateSpec(
  const ast::ModuleDecl& decl,
  const std::unordered_map<IdString, int64_t, IdString::Hash>& paramEnv,
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

static void expandGenerates(const ModuleSpec& spec,
                            const ast::ModuleDecl& decl,
                            std::vector<ast::InstanceDecl>& out,
                            std::ostream& diag) {
    // Copy original instances
    for (const auto& i : decl.mInstances) {
        out.push_back(i);
    }

    // gen-if
    for (const auto& g : decl.mGenIfs) {
        int64_t c = evalIntExpr(g.mCond, spec.mParamEnv, &diag);
        const auto& list = (c != 0) ? g.mThenInsts : g.mElseInsts;
        for (size_t idx = 0; idx < list.size(); ++idx) {
            ast::InstanceDecl inst = list[idx];
            if (g.mLabel.valid()) {
                std::string newName = g.mLabel.str() + "_" + inst.mName.str();
                inst.mName = IdString(newName);
            }
            out.push_back(std::move(inst));
        }
    }

    // gen-for
    for (const auto& gf : decl.mGenFors) {
        int64_t start = evalIntExpr(gf.mStart, spec.mParamEnv, &diag);
        int64_t limit = evalIntExpr(gf.mLimit, spec.mParamEnv, &diag);
        int64_t step = evalIntExpr(gf.mStep, spec.mParamEnv, &diag);
        if (step == 0) {
            diag << "ERROR: gen-for step is zero in " << spec.mName.str()
                 << "\n";
            continue;
        }
        if ((step > 0 && start >= limit) || (step < 0 && start <= limit)) {
            continue;
        }
        int64_t iter = 0;
        for (int64_t i = start; (step > 0) ? (i < limit) : (i > limit);
             i += step, ++iter) {
            for (const auto& templ : gf.mBody) {
                ast::InstanceDecl inst = templ;
                std::string prefix =
                  gf.mLabel.valid() ? gf.mLabel.str() : "gen";
                std::string newName =
                  prefix + "_" + std::to_string(iter) + "_" + inst.mName.str();
                inst.mName = IdString(newName);
                out.push_back(std::move(inst));
            }
        }
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
        for (const auto& ov : idecl.mParamOverrides) {
            int64_t val = evalIntExpr(ov.mValue, spec.mParamEnv, &diag);
            calleeParams[ov.mName] = val;
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
            BitVector actual = fc.flattenExpr(*c.mActual);
            if (actual.size() != Wf) {
                diag << "ERROR: Width mismatch binding " << idecl.mName.str()
                     << "." << c.mFormal.str() << " Wf=" << Wf
                     << " Wa=" << actual.size()
                     << " actual=" << ast::exprToString(*c.mActual) << "\n";
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