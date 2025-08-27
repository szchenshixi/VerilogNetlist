#pragma once
// BitAtom alias used by flattening and instance port bindings.

#include <cstdint>
#include <vector>

#include "hdl/util/id_string.hpp"

namespace hdl {
namespace elab {

enum class BitAtomKind { PortBit, WireBit, Const0, Const1 };

struct BitAtom {
    BitAtomKind mKind = BitAtomKind::WireBit;
    IdString mOwnerIndex;   // port or wire index depending on kind
    uint32_t mBitIndex = 0; // LSB-first offset
};

using BitVector = std::vector<BitAtom>;

} // namespace elab
} // namespace hdl