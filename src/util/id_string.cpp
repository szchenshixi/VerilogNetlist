#include "hdl/util/id_string.hpp"

namespace hdl {

IdString::Pool& IdString::pool() {
    static Pool p;
    return p;
}

uint32_t IdString::internGlobal(std::string_view sv) {
    auto& p = pool();
    std::lock_guard<std::mutex> lock(p.mMu);
    auto it = p.mMap.find(std::string(sv));
    if (it != p.mMap.end()) return it->second;
    uint32_t id = static_cast<uint32_t>(p.mPool.size());
    p.mPool.emplace_back(sv);
    p.mMap.emplace(p.mPool.back(), id);
    return id;
}

const std::string& IdString::resolveGlobal(uint32_t id) {
    auto& p = pool();
    if (id == kInvalid || id >= p.mPool.size()) { return getInvalidStr(); }
    return p.mPool[id];
}

} // namespace hdl