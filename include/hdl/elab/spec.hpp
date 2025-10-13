#pragma once
// Elaborated module spec: ports, wires, BitMap, instances.

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
    uint32_t width() const { return widthFromRange(mMsb, mLsb); }
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

struct ConnSpec {
    uint32_t mFormalIndex = 0; // index into callee->mPorts
    BitVector mActual;         // flattened actual bits in parent scope
};

struct InstanceSpec {
    IdString mName;
    const struct ModuleSpec* mCallee = nullptr;
    std::vector<ConnSpec> mConns;
};

struct ModuleSpec {
    IdString mName;
    const ast::ModuleDecl* mDecl = nullptr; // back-pointer to AST
    std::vector<InstanceSpec> mInstances;
    std::vector<PortSpec> mPorts;
    std::vector<WireSpec> mWires;

    std::unordered_map<IdString, uint32_t, IdString::Hash> mPortIndex;
    std::unordered_map<IdString, uint32_t, IdString::Hash> mWireIndex;

    ParamSpec mEnv;

    net::BitMap mBitMap;

    int findPortIndex(IdString n) const;
    int findWireIndex(IdString n) const;

    net::BitId portBit(IdString name, uint32_t bitOff) const;
    net::BitId wireBit(IdString name, uint32_t bitOff) const;

    void dumpLayout(std::ostream& os);
    void dumpConnectivity(std::ostream& os);

    std::string renderBit(net::BitId b) const;
};

// Library keyed by "name#paramSig"
using ModuleSpecLib = std::unordered_map<IdString, ModuleSpec, IdString::Hash>;
} // namespace hdl::elab