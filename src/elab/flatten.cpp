#include <algorithm>
#include <cassert>

#include "hdl/ast/expr.hpp"
#include "hdl/common.hpp"
#include "hdl/elab/flatten.hpp"

namespace hdl::elab {

BitVector FlattenContext::flattenId(IdString name) const {
    BitVector v;
    int pIdx = mSpec.findPortIndex(name);
    if (pIdx >= 0) {
        auto w = mSpec.mPorts[pIdx].width();
        v.reserve(w);
        for (uint32_t i = 0; i < w; ++i) {
            v.push_back(BitAtom{BitAtomKind::PortBit, name, i});
        }
        return v;
    }
    int wIdx = mSpec.findWireIndex(name);
    if (wIdx >= 0) {
        auto w = mSpec.mWires[wIdx].width();
        v.reserve(w);
        for (uint32_t i = 0; i < w; ++i) {
            v.push_back(BitAtom{BitAtomKind::WireBit, name, i});
        }
        return v;
    }
    error("Unknown identifier: " + name.str());
    return v;
}

BitVector FlattenContext::flattenNumber(uint64_t value, int width) const {
    BitVector v;
    if (width <= 0) {
        error("Number literal without width is not supported in flattenNumber "
              "(demo)");
        return v;
    }
    v.reserve(width);
    for (int i = 0; i < width; ++i) {
        bool bit = (value >> i) & 1ULL;
        v.push_back(BitAtom{bit ? BitAtomKind::Const1 : BitAtomKind::Const0,
                            IdString{},
                            static_cast<uint32_t>(i)});
    }
    return v;
}

BitVector FlattenContext::flattenSlice(const ast::BVSlice& s) const {
    IdString id = s.mBaseId;

    int pIdx = mSpec.findPortIndex(id);
    int wIdx = mSpec.findWireIndex(id);
    if (pIdx < 0 && wIdx < 0) {
        error("Unknown identifier in slice: " + id.str());
        return {};
    }

    int64_t msb = ast::evalIntExpr(s.mMsb, mSpec.mEnv);
    int64_t lsb = ast::evalIntExpr(s.mLsb, mSpec.mEnv);
    int64_t lo = std::min(msb, lsb);
    int64_t hi = std::max(msb, lsb);
    int64_t width = static_cast<int64_t>(hi - lo + 1);

    BitVector v;
    v.reserve(width);

    auto computeOffset = [](int absIdx, const NetSpec& net) -> int {
        if (net.mMsb >= net.mLsb) return absIdx - net.mLsb;
        else return net.mLsb - absIdx;
    };

    if (pIdx >= 0) {
        const auto& net = mSpec.mPorts[pIdx].mNet;
        for (int abs = lo; abs <= hi; ++abs) {
            int off = computeOffset(abs, net);
            if (off < 0 ||
                static_cast<uint32_t>(off) >= mSpec.mPorts[pIdx].width()) {
                error("Slice out of range on port " + id.str());
                return {};
            }
            v.push_back(
              BitAtom{BitAtomKind::PortBit, id, static_cast<uint32_t>(off)});
        }
    } else {
        const auto& net = mSpec.mWires[wIdx].mNet;
        for (int abs = lo; abs <= hi; ++abs) {
            int off = computeOffset(abs, net);
            if (off < 0 ||
                static_cast<uint32_t>(off) >= mSpec.mWires[wIdx].width()) {
                error("Slice out of range on wire " + id.str());
                return {};
            }
            v.push_back(
              BitAtom{BitAtomKind::WireBit, id, static_cast<uint32_t>(off)});
        }
    }

    return v; // LSB-first
}

BitVector FlattenContext::flattenConcat(const ast::BVConcat& c) const {
    BitVector res;
    for (int i = static_cast<int>(c.mParts.size()) - 1; i >= 0; --i) {
        BitVector part = flattenExpr(c.mParts[i]);
        res.insert(res.end(), part.begin(), part.end());
    }
    return res;
}

BitVector FlattenContext::flattenExpr(const ast::BVExpr& e) const {
    return e.visit([&](auto&& node) -> BitVector {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::BVId>) {
            return flattenId(node.mName);
        } else if constexpr (std::is_same_v<T, ast::BVConst>) {
            return flattenNumber(node.mValue, node.mWidth);
        } else if constexpr (std::is_same_v<T, ast::BVConcat>) {
            return flattenConcat(node);
        } else if constexpr (std::is_same_v<T, ast::BVSlice>) {
            return flattenSlice(node);
        } else {
            return BitVector{};
        }
    });
}
void FlattenContext::warn(const std::string& msg) const {
    ::hdl::warn(mDiag, msg);
}
void FlattenContext::error(const std::string& msg) const {
    ::hdl::error(mDiag, msg);
}
} // namespace hdl::elab