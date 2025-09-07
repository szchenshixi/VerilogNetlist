#include "hdl/net/connectivity.hpp"

namespace hdl::net {
// -------------------------------------------
// UnionFindBits
BitId UnionFindBits::addNode() {
    BitId idx = static_cast<BitId>(mParent.size());
    mParent.push_back(idx);
    mRank.push_back(0);
    return idx;
}

void UnionFindBits::ensureSize(BitId n) {
    while (mParent.size() < n) {
        addNode();
    }
}

BitId UnionFindBits::find(BitId x) {
    if (mParent[x] != x) mParent[x] = find(mParent[x]);
    return mParent[x];
}

void UnionFindBits::unite(BitId a, BitId b) {
    a = find(a);
    b = find(b);
    if (a == b) return;
    if (mRank[a] < mRank[b]) std::swap(a, b);
    mParent[b] = a;
    if (mRank[a] == mRank[b]) mRank[a]++;
}

// End of UnionFindBits
// -------------------------------------------
// Connectivity
BitId Connectivity::allocRange(uint32_t width) {
    BitId base = mNextId;
    mUf.ensureSize(base + width);
    mNextId += width;
    return base;
}

BitId Connectivity::size() const { return mNextId; }

void Connectivity::alias(BitId a, BitId b) {
    if (a >= mNextId || b >= mNextId) return; // guard for demo
    mUf.unite(a, b);
}

NetId Connectivity::netId(BitId id) {
    if (id >= mNextId) return id;
    return mUf.find(id);
}

std::vector<std::vector<BitId>> Connectivity::collectGroups() {
    std::unordered_map<NetId, std::vector<BitId>> m;
    m.reserve(mNextId);
    for (BitId i = 0; i < mNextId; ++i) {
        m[mUf.find(i)].push_back(i);
    }
    std::vector<std::vector<BitId>> g;
    g.reserve(m.size());
    for (auto& kv : m)
        g.push_back(std::move(kv.second));
    return g;
}

void Connectivity::dump(std::ostream& os,
                        const std::function<std::string(BitId)>& renderBit) {
    auto groups = collectGroups();
    os << "Connectivity groups (" << groups.size() << "):\n";
    for (auto& grp : groups) {
        os << "  { ";
        for (size_t i = 0; i < grp.size(); ++i) {
            os << renderBit(grp[i]);
            if (i + 1 < grp.size()) os << ", ";
        }
        os << " }\n";
    }
}
// End of Connectivity
// -------------------------------------------

} // namespace hdl::net