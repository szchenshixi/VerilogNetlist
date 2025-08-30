#pragma once
// Bit-level disjoint-set (union-find) connectivity with explicit BitId/NetId.

#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace hdl {
namespace net {

using BitId = uint32_t;
using NetId = uint32_t;

struct UnionFindBits {
    std::vector<BitId> mParent;
    std::vector<uint32_t> mRank;

    BitId addNode();
    void ensureSize(BitId n);
    BitId find(BitId x);
    void unite(BitId a, BitId b);
};

struct Connectivity {
    UnionFindBits mUf;
    BitId mNextId = 0;

    BitId allocRange(uint32_t width);
    BitId size() const;
    void alias(BitId a, BitId b);
    NetId netId(BitId id);
    std::vector<std::vector<BitId>> collectGroups();
    void dump(std::ostream& os,
              const std::function<std::string(BitId)>& renderBit);
};

} // namespace net
} // namespace hdl