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

namespace hdl::elab {
struct NetSpec {
    int mMsb = 0;
    int mLsb = 0;
    uint32_t width() const { return width_from_range(mMsb, mLsb); }
};
struct PortSpec {
    IdString mName;
    PortDirection mDir = PortDirection::In;
    NetSpec mNet; // range/width
    uint32_t width() const { return mNet.width(); }
};

struct WireSpec {
    IdString mName;
    NetSpec mNet;
    uint32_t width() const { return mNet.width(); }
};

using ParamSpec = std::unordered_map<IdString, int64_t, IdString::Hash>;

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

    ParamSpec mParamEnv;

    net::BitMap mBitMap;

    const ast::ModuleDecl* mDecl = nullptr; // back-pointer to AST

    std::vector<hier::ModuleInstance> mInstances;

    int findPortIndex(IdString n) const;
    int findWireIndex(IdString n) const;

    net::BitId portBit(IdString name, uint32_t bitOff) const;
    net::BitId wireBit(IdString name, uint32_t bitOff) const;

    void dumpLayout(std::ostream& os);
    void dumpConnectivity(std::ostream& os);

    std::string renderBit(net::BitId b) const;
};

// Library keyed by "name#paramSig"
using ModuleLibrary = std::unordered_map<std::string, ModuleSpec>;

} // namespace hdl::elab