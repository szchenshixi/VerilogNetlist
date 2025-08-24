#pragma once
// Global interning-backed IdString. Construct with IdString("text").
// Intern pool is a private global singleton.

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hdl {

class IdString {
  public:
    IdString()
        : mId(kInvalid) {}
    explicit IdString(std::string_view sv)
        : mId(internGlobal(sv)) {}

    bool valid() const { return mId != kInvalid; }
    uint32_t id() const { return mId; }
    const std::string& str() const { return resolveGlobal(mId); }

    bool operator==(const IdString& o) const { return mId == o.mId; }
    bool operator!=(const IdString& o) const { return mId != o.mId; }
    bool operator<(const IdString& o) const { return mId < o.mId; }

    struct Hash {
        size_t operator()(const IdString& s) const noexcept {
            return std::hash<uint32_t>{}(s.mId);
        }
    };

  private:
    static constexpr uint32_t kInvalid = 0xFFFFFFFFu;
    uint32_t mId;

    static uint32_t internGlobal(std::string_view);
    static const std::string& resolveGlobal(uint32_t);
    static const std::string& getInvalidStr() {
        static const std::string kInvalidStr = "<Invalid>";
        return kInvalidStr;
    }

    // Internal pool
    struct Pool {
        std::vector<std::string> mPool;
        std::unordered_map<std::string, uint32_t> mMap;
        std::mutex mMu;
    };
    static Pool& pool();
};

} // namespace hdl