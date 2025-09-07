#pragma once
// BitMap encapsulates bit allocation (base BitIds), connectivity, and reverse
// lookup.

#include <string>
#include <vector>

#include "hdl/net/connectivity.hpp"

namespace hdl::elab {
struct ModuleSpec;
}

namespace hdl::net {

struct BitOwnerRef {
    enum class Kind { Port, Wire } mKind = Kind::Wire;
    uint32_t mOwnerIndex = 0; // port or wire index
    uint32_t mBitOffset = 0;  // LSB-first offset within owner
};

struct BitMap {
    Connectivity mConn;
    std::vector<BitId> mPortBase;         // base BitId per port index
    std::vector<BitId> mWireBase;         // base BitId per wire index
    std::vector<BitOwnerRef> mReverseMap; // size == mConn.size()

    void reset() {
        mConn = Connectivity{};
        mPortBase.clear();
        mWireBase.clear();
        mReverseMap.clear();
    }

    // Build allocation and reverse map from a ModuleSpec's declared
    // ports/wires.
    void build(const elab::ModuleSpec& spec);

    // Addressing
    BitId portBit(uint32_t pIdx, uint32_t bitOff) const {
        return mPortBase[pIdx] + bitOff;
    }
    BitId wireBit(uint32_t wIdx, uint32_t bitOff) const {
        return mWireBase[wIdx] + bitOff;
    }

    // Connectivity ops
    void alias(BitId a, BitId b) { mConn.alias(a, b); }
    net::NetId netId(BitId a) { return mConn.netId(a); }

    // Rendering using spec
    std::string renderBit(const elab::ModuleSpec& spec, BitId g) const;

    // Dump
    void dumpConnectivity(const elab::ModuleSpec& spec, std::ostream& os) {
        mConn.dump(os, [&](BitId b) { return renderBit(spec, b); });
    }
};

} // namespace hdl::net