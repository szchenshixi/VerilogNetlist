#include <algorithm>
#include <cassert>

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

BitVector FlattenContext::flattenSlice(const ast::SliceExpr& s) const {
    IdString id = s.mBaseId;

    int pIdx = mSpec.findPortIndex(id);
    int wIdx = mSpec.findWireIndex(id);
    if (pIdx < 0 && wIdx < 0) {
        error("Unknown identifier in slice: " + id.str());
        return {};
    }

    int msb = s.mMsb, lsb = s.mLsb;
    int lo = std::min(msb, lsb);
    int hi = std::max(msb, lsb);
    uint32_t width = static_cast<uint32_t>(hi - lo + 1);

    BitVector v;
    v.reserve(width);

    auto computeOffset = [](int absIdx, const ast::NetEntity& ent) -> int {
        if (ent.mMsb >= ent.mLsb) return absIdx - ent.mLsb;
        else return ent.mLsb - absIdx;
    };

    if (pIdx >= 0) {
        const auto& ent = mSpec.mPorts[pIdx].mEnt;
        for (int abs = lo; abs <= hi; ++abs) {
            int off = computeOffset(abs, ent);
            if (off < 0 ||
                static_cast<uint32_t>(off) >= mSpec.mPorts[pIdx].width()) {
                error("Slice out of range on port " + id.str());
                return {};
            }
            v.push_back(
              BitAtom{BitAtomKind::PortBit, id, static_cast<uint32_t>(off)});
        }
    } else {
        const auto& ent = mSpec.mWires[wIdx].mEnt;
        for (int abs = lo; abs <= hi; ++abs) {
            int off = computeOffset(abs, ent);
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

BitVector FlattenContext::flattenConcat(const ast::ConcatExpr& c) const {
    BitVector res;
    for (int i = static_cast<int>(c.mParts.size()) - 1; i >= 0; --i) {
        BitVector part = flattenExpr(c.mParts[i]);
        res.insert(res.end(), part.begin(), part.end());
    }
    return res;
}

BitVector FlattenContext::flattenExpr(const ast::Expr& e) const {
    return e.visit([&](auto&& node) -> BitVector {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::IdExpr>) {
            return flattenId(node.mName);
        } else if constexpr (std::is_same_v<T, ast::ConstExpr>) {
            return flattenNumber(node.mValue, node.mWidth);
        } else if constexpr (std::is_same_v<T, ast::ConcatExpr>) {
            return flattenConcat(node);
        } else if constexpr (std::is_same_v<T, ast::SliceExpr>) {
            return flattenSlice(node);
        } else {
            return BitVector{};
        }
    });
}

} // namespace hdl::elab