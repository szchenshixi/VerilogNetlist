#pragma once
// BitAtom alias used by flattening and instance port bindings.

#include <cstdint>
#include <vector>

namespace hdl {
namespace elab {

enum class BitAtomKind { PortBit, WireBit, Const0, Const1 };

struct BitAtom {
    BitAtomKind mKind = BitAtomKind::WireBit;
    uint32_t mOwnerIndex = 0; // port or wire index depending on kind
    uint32_t mBitIndex = 0;   // LSB-first offset
};

using BitVector = std::vector<BitAtom>;

} // namespace elab
} // namespace hdl