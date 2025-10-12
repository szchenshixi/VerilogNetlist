#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <variant>

#include "hdl/ast/decl.hpp"
#include "hdl/ast/expr.hpp"
#include "hdl/common.hpp"
#include "hdl/elab/elaborate.hpp"
#include "hdl/elab/spec.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl::elab {

std::string makeModuleKey(std::string_view nameText,
                          const elab::ParamSpec& params) {
    std::vector<std::pair<std::string, int64_t>> v;
    v.reserve(params.size());
    for (auto& kv : params) {
        v.emplace_back(kv.first.str(), kv.second);
    }
    std::sort(
      v.begin(), v.end(), [](auto& a, auto& b) { return a.first < b.first; });
    std::ostringstream oss;
    oss << nameText;
    if (!v.empty()) oss << "#";
    for (size_t i = 0; i < v.size(); ++i) {
        oss << v[i].first << "=" << v[i].second;
        if (i + 1 < v.size()) oss << ",";
    }
    return oss.str();
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

ModuleSpec elaborateModule(const ast::ModuleDecl& decl,
                           const elab::ParamSpec& overrides) {
    ModuleSpec spec;
    spec.mName = decl.mName;
    spec.mDecl = &decl;
    spec.mEnv = decl.mDefaults;
    update(spec.mEnv, overrides);

    // Ports
    spec.mPorts.reserve(decl.mPorts.size());
    for (const auto& p : decl.mPorts) {
        PortSpec ps;
        ps.mName = p.mName;
        ps.mDir = p.mDir;
        ps.mNet.mMsb = ast::evalIntExpr(p.mNet.mMsb, overrides);
        ps.mNet.mLsb = ast::evalIntExpr(p.mNet.mLsb, overrides);
        spec.mPortIndex.emplace(ps.mName,
                                static_cast<uint32_t>(spec.mPorts.size()));
        spec.mPorts.push_back(std::move(ps));
    }
    // Wires
    spec.mWires.reserve(decl.mWires.size());
    for (const auto& w : decl.mWires) {
        WireSpec ws;
        ws.mName = w.mName;
        ws.mNet.mMsb = ast::evalIntExpr(w.mNet.mMsb, overrides);
        ws.mNet.mLsb = ast::evalIntExpr(w.mNet.mLsb, overrides);
        spec.mWireIndex.emplace(ws.mName,
                                static_cast<uint32_t>(spec.mWires.size()));
        spec.mWires.push_back(std::move(ws));
    }

    // Build bit map allocation
    spec.mBitMap.build(spec);
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
                      << " (lhs=" << ast::bvExprToString(asg.mLhs)
                      << ", rhs=" << ast::bvExprToString(asg.mRhs) << ")\n";
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
                            const elab::ParamSpec& overrides,
                            ModuleSpecLib& specLib) {
    elab::ParamSpec env = decl.mDefaults;
    update(/* out */ env, overrides);
    IdString key(makeModuleKey(decl.mName.str(), env));
    auto it = specLib.find(key);
    if (it != specLib.end()) return it->second;
    ModuleSpec ms = elaborateModule(decl, env);
    specLib.emplace(key, std::move(ms)); // ms is invalid now
    wireAssigns(specLib[key]);
    return specLib[key];
}

void expandGenBlk(const ModuleSpec& spec, const ast::GenBody& block,
                  const elab::ParamSpec& env,
                  const std::vector<std::string>& nameStack,
                  std::vector<ast::InstanceDecl>& out, std::ostream* diag);

void expandGenIf(const ModuleSpec& spec, const ast::GenIfDecl& decl,
                 const elab::ParamSpec& env,
                 std::vector<std::string> nameStack,
                 std::vector<ast::InstanceDecl>& out, std::ostream* diag) {
    int64_t cond = evalIntExpr(decl.mCond, env, diag);
    const auto& selected = (cond != 0) ? decl.mThenBlks : decl.mElseBlks;
    if (selected.empty()) { return; }
    if (decl.mLabel.valid()) { nameStack.push_back(decl.mLabel.str()); }
    for (auto& blk : selected) {
        expandGenBlk(spec, blk, env, nameStack, out, diag);
    }
}

void expandGenFor(const ModuleSpec& spec, const ast::GenForDecl& decl,
                  const elab::ParamSpec& env,
                  std::vector<std::string> nameStack,
                  std::vector<ast::InstanceDecl>& out, std::ostream* diag) {
    int64_t start = evalIntExpr(decl.mStart, env, diag);
    int64_t limit = evalIntExpr(decl.mLimit, env, diag);
    int64_t step = evalIntExpr(decl.mStep, env, diag);
    if (step == 0) {
        error(diag, "gen-for step is zero in " + spec.mName.str());
        return;
    }
    if ((step > 0 && start >= limit) || (step < 0 && start <= limit)) {
        return;
    }

    std::string label = decl.mLabel.valid() ? decl.mLabel.str() : "gen";
    int64_t iter = 0;
    for (int64_t val = start; (step > 0) ? (val < limit) : (val > limit);
         val += step, ++iter) {
        elab::ParamSpec iterEnv = env;
        iterEnv[decl.mLoopVar] = val;

        auto iterStack = nameStack;
        iterStack.push_back(label + "_" + std::to_string(iter));
        for (auto& blk : decl.mBlks) {
            expandGenBlk(spec, blk, iterEnv, iterStack, out, diag);
        }
    }
}

void expandGenBlk(const ModuleSpec& spec, const ast::GenBody& block,
                  const elab::ParamSpec& env,
                  const std::vector<std::string>& nameStack,
                  std::vector<ast::InstanceDecl>& out, std::ostream* diag) {
    if (std::holds_alternative<ast::InstanceDecl>(block)) {
        auto inst = std::get<ast::InstanceDecl>(block);
        if (!nameStack.empty()) {
            inst.mName =
              IdString(buildPrefixedName(nameStack, inst.mName.str()));
        }
        out.push_back(std::move(inst));
    } else if (std::holds_alternative<ast::GenIfDecl>(block)) {
        const auto& gi = std::get<ast::GenIfDecl>(block);
        expandGenIf(spec, gi, env, nameStack, out, diag);
    } else if (std::holds_alternative<ast::GenForDecl>(block)) {
        const auto& gf = std::get<ast::GenForDecl>(block);
        expandGenFor(spec, gf, env, nameStack, out, diag);
    } else if (std::holds_alternative<ast::GenCaseDecl>(block)) {
        const auto& gc = std::get<ast::GenCaseDecl>(block);
        // expandGenCase(spec, gc, env, nameStack, out, diag);
    } else {
        std::cerr << "Error: Unknown GenItem type\n";
    }
}

static void expandGenerates(const ModuleSpec& spec,
                            const ast::ModuleDecl& decl,
                            std::vector<ast::InstanceDecl>& out,
                            std::ostream* diag) {
    for (const auto& inst : decl.mInstances) {
        out.push_back(inst);
    }

    auto baseEnv = spec.mEnv;

    for (const auto& gb : decl.mGenBlks) {
        expandGenBlk(spec, gb, baseEnv, /* nameStack */ {}, out, diag);
    }
}

void linkInstances(ModuleSpec& spec, const ModuleDeclLib& declLib,
                   ModuleSpecLib& spceLib, std::ostream* diag) {
    spec.mInstances.clear();
    if (!spec.mDecl) return;

    // Expand generate constructs and gather all instances to link.
    std::vector<ast::InstanceDecl> flatInsts;
    expandGenerates(spec, *spec.mDecl, flatInsts, diag);

    // Bind each instance
    for (const auto& idecl : flatInsts) {
        auto it = declLib.find(idecl.mTargetModule);
        if (it == declLib.end()) {
            error(diag,
                  "unknown module '" + idecl.mTargetModule.str() +
                    "' for instance " + idecl.mName.str() + " in module " +
                    spec.mName.str());
            continue;
        }
        const ast::ModuleDecl& calleeDecl = it->second;

        // Build parameter environment for callee: defaults + overrides
        elab::ParamSpec calleeParams = calleeDecl.mDefaults;
        for (const auto& [key, val] : idecl.mOverrides) {
            if (auto it = calleeDecl.mDefaults.find(key);
                it == calleeDecl.mDefaults.end()) {
                warn(diag,
                     "unknown parameter in instance declare " +
                       idecl.mName.str() + ":" + key.str());
            }
            calleeParams[key] = ast::evalIntExpr(val, spec.mEnv);
        }

        // Get/Elaborate callee spec with parameters
        ModuleSpec& callee =
          getOrCreateSpec(calleeDecl, calleeParams, spceLib);

        // Create ModuleInstance with port bindings
        hier::ModuleInstance inst;
        inst.mName = idecl.mName;
        inst.mCallee = &callee;

        FlattenContext fc(spec, diag);
        for (const auto& c : idecl.mConns) {
            int formalIdx = callee.findPortIndex(c.mFormal);
            if (formalIdx < 0) {
                error(diag,
                      "unknown formal port '" + c.mFormal.str() +
                        "' on instance " + idecl.mName.str() + " in module " +
                        spec.mName.str());
                continue;
            }
            uint32_t Wf = callee.mPorts[formalIdx].width();
            BitVector actual = fc.flattenExpr(c.mActual);
            if (actual.size() != Wf) {
                error(diag,
                      "width mismatch binding " + idecl.mName.str() + "." +
                        c.mFormal.str() + " Wf=" + std::to_string(Wf) +
                        " Wa=" + std::to_string(actual.size()) +
                        " actual=" + ast::bvExprToString(c.mActual));
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
            error(diag,
                  "scope path index " + std::to_string(idx) +
                    " out of range at depth " + std::to_string(depth));
            return false;
        }
        const auto& inst = cur->mInstances[idx];
        if (!inst.mCallee) {
            error(diag, "null callee at depth " + std::to_string(depth));
            return false;
        }
        cur = inst.mCallee;
    }
    int pIdx = cur->findPortIndex(portName);
    if (pIdx < 0) {
        error(diag, "no such port in module");
        return false;
    }
    out.mScope = scope;
    out.mPortIndex = static_cast<uint32_t>(pIdx);
    return true;
}
} // namespace hier
} // namespace hdl::elab