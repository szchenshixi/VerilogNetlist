#include "hdl/net/bitmap.hpp"
#include "hdl/elab/spec.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl::net {

void BitMap::build(const elab::ModuleSpec& spec) {
    reset();
    mPortBase.resize(spec.mPorts.size(), 0);
    mWireBase.resize(spec.mWires.size(), 0);

    // Allocate ports
    for (size_t i = 0; i < spec.mPorts.size(); ++i) {
        auto w = spec.mPorts[i].width();
        BitId base = mConn.allocRange(w);
        mPortBase[i] = base;
    }
    // Allocate wires
    for (size_t i = 0; i < spec.mWires.size(); ++i) {
        auto w = spec.mWires[i].width();
        BitId base = mConn.allocRange(w);
        mWireBase[i] = base;
    }

    // Build reverse map
    mReverseMap.resize(mConn.size());
    for (size_t i = 0; i < spec.mPorts.size(); ++i) {
        for (uint32_t b = 0; b < spec.mPorts[i].width(); ++b) {
            mReverseMap[mPortBase[i] + b] = BitOwnerRef{
              BitOwnerRef::Kind::Port, static_cast<uint32_t>(i), b};
        }
    }
    for (size_t i = 0; i < spec.mWires.size(); ++i) {
        for (uint32_t b = 0; b < spec.mWires[i].width(); ++b) {
            mReverseMap[mWireBase[i] + b] = BitOwnerRef{
              BitOwnerRef::Kind::Wire, static_cast<uint32_t>(i), b};
        }
    }
}

std::string BitMap::renderBit(const elab::ModuleSpec& spec, BitId g) const {
    if (g >= mReverseMap.size()) {
        return "<out-of-range:" + std::to_string(g) + ">";
    }
    const auto& r = mReverseMap[g];
    if (r.mKind == BitOwnerRef::Kind::Port) {
        const auto& p = spec.mPorts[r.mOwnerIndex];
        int idx = (p.mEnt.mMsb >= p.mEnt.mLsb)
                    ? (p.mEnt.mLsb + static_cast<int>(r.mBitOffset))
                    : (p.mEnt.mLsb - static_cast<int>(r.mBitOffset));
        return "port " + p.mName.str() + "[" + std::to_string(idx) + "]";
    } else {
        const auto& w = spec.mWires[r.mOwnerIndex];
        int idx = (w.mEnt.mMsb >= w.mEnt.mLsb)
                    ? (w.mEnt.mLsb + static_cast<int>(r.mBitOffset))
                    : (w.mEnt.mLsb - static_cast<int>(r.mBitOffset));
        return "wire " + w.mName.str() + "[" + std::to_string(idx) + "]";
    }
}

} // namespace hdl::net