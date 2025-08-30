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
    ast::NetEntity mEnt; // range/width
    uint32_t width() const { return mEnt.width(); }
};

struct WireSpec {
    IdString mName;
    ast::NetEntity mEnt;
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

    int findPortIndex(IdString n) const;
    int findWireIndex(IdString n) const;

    net::BitId portBit(IdString name, uint32_t bitOff) const;
    net::BitId wireBit(IdString name, uint32_t bitOff) const;

    void dumpLayout(std::ostream& os);
    void dumpConnectivity(std::ostream& os);

    std::string renderBit(net::BitId b) const;
};

// Library keyed by "name#paramSig"
using ModuleLibrary =
  std::unordered_map<std::string, std::unique_ptr<ModuleSpec>>;

} // namespace elab
} // namespace hdl