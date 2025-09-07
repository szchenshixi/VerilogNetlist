#include "hdl/elab/spec.hpp"

namespace hdl::elab {
int ModuleSpec::findPortIndex(IdString n) const {
    auto it = mPortIndex.find(n);
    return it == mPortIndex.end() ? -1 : static_cast<int>(it->second);
}
int ModuleSpec::findWireIndex(IdString n) const {
    auto it = mWireIndex.find(n);
    return it == mWireIndex.end() ? -1 : static_cast<int>(it->second);
}

net::BitId ModuleSpec::portBit(IdString name, uint32_t bitOff) const {
    int idx = findPortIndex(name);
    if (idx < 0) return UINT32_MAX;
    if (bitOff >= mPorts[idx].width()) return UINT32_MAX;
    return mBitMap.portBit(static_cast<uint32_t>(idx), bitOff);
}

net::BitId ModuleSpec::wireBit(IdString name, uint32_t bitOff) const {
    int idx = findWireIndex(name);
    if (idx < 0) return UINT32_MAX;
    if (bitOff >= mWires[idx].width()) return UINT32_MAX;
    return mBitMap.wireBit(static_cast<uint32_t>(idx), bitOff);
}

void ModuleSpec::dumpLayout(std::ostream& os) {
    os << "ModuleSpec " << mName.str() << " layout:\n";
    os << "  Ports:\n";
    for (size_t i = 0; i < mPorts.size(); ++i) {
        const auto& p = mPorts[i];
        os << "    [" << i << "] " << p.mName.str()
           << " dir=" << to_string(p.mDir) << " range=[" << p.mEnt.mMsb << ":"
           << p.mEnt.mLsb << "]" << " width=" << p.width() << "\n";
    }
    os << "  Wires:\n";
    for (size_t i = 0; i < mWires.size(); ++i) {
        const auto& w = mWires[i];
        os << "    [" << i << "] " << w.mName.str() << " range=["
           << w.mEnt.mMsb << ":" << w.mEnt.mLsb << "]"
           << " width=" << w.width() << "\n";
    }
}

void ModuleSpec::dumpConnectivity(std::ostream& os) {
    mBitMap.dumpConnectivity(*this, os);
}

std::string ModuleSpec::renderBit(net::BitId b) const {
    return mBitMap.renderBit(*this, b);
}
} // namespace hdl::elab