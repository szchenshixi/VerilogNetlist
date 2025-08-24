#pragma once
// Elaborated module spec: ports, wires, BitMap, instances.

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "hdl/ast/decl.hpp"
#include "hdl/common.hpp"
#include "hdl/elab/bits.hpp"
#include "hdl/net/bitmap.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl {
namespace elab {

struct PortSpec {
    IdString mName;
    PortDirection mDir = PortDirection::In;
    ast::WireEntity mEnt; // range/width
    uint32_t width() const { return mEnt.width(); }
};

struct WireSpec {
    IdString mName;
    ast::WireEntity mEnt;
    uint32_t width() const { return mEnt.width(); }
};

namespace hier {
struct PortBinding {
    uint32_t mFormalIndex = 0; // index into callee->mPorts
    BitVector mActual;         // flattened actual bits in parent scope
};

struct ModuleInstance {
    IdString mName;
    const struct ModuleSpec* mCallee = nullptr;
    std::vector<PortBinding> mConnections;
};
} // namespace hier

struct ModuleSpec {
    IdString mName;
    std::vector<PortSpec> mPorts;
    std::vector<WireSpec> mWires;

    std::unordered_map<IdString, uint32_t, IdString::Hash> mPortIndex;
    std::unordered_map<IdString, uint32_t, IdString::Hash> mWireIndex;

    std::unordered_map<IdString, int64_t, IdString::Hash> mParamEnv;

    net::BitMap mBitMap;

    const ast::ModuleDecl* mDecl = nullptr; // back-pointer to AST

    std::vector<hier::ModuleInstance> mInstances;

    int findPortIndex(IdString n) const {
        auto it = mPortIndex.find(n);
        return it == mPortIndex.end() ? -1 : static_cast<int>(it->second);
    }
    int findWireIndex(IdString n) const {
        auto it = mWireIndex.find(n);
        return it == mWireIndex.end() ? -1 : static_cast<int>(it->second);
    }

    void dumpLayout(std::ostream& os) {
        os << "ModuleSpec " << mName.str() << " layout:\n";
        os << "  Ports:\n";
        for (size_t i = 0; i < mPorts.size(); ++i) {
            const auto& p = mPorts[i];
            os << "    [" << i << "] " << p.mName.str()
               << " dir=" << to_string(p.mDir) << " range=[" << p.mEnt.mMsb
               << ":" << p.mEnt.mLsb << "]" << " width=" << p.width() << "\n";
        }
        os << "  Wires:\n";
        for (size_t i = 0; i < mWires.size(); ++i) {
            const auto& w = mWires[i];
            os << "    [" << i << "] " << w.mName.str() << " range=["
               << w.mEnt.mMsb << ":" << w.mEnt.mLsb << "]"
               << " width=" << w.width() << "\n";
        }
    }

    void dumpConnectivity(std::ostream& os) {
        mBitMap.dumpConnectivity(*this, os);
    }

    std::string renderBit(net::BitId b) const {
        return mBitMap.renderBit(*this, b);
    }
};

// Library keyed by "name#paramSig"
using ModuleLibrary =
  std::unordered_map<std::string, std::unique_ptr<ModuleSpec>>;

} // namespace elab
} // namespace hdl