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
          struct NoInternTag {
              explicit NoInternTag() = default;
          };
          static inline constexpr NoInternTag NoIntern{};
      
          IdString()
              : mId(kInvalid) {}
      
          explicit IdString(std::string_view sv)
              : mId(internGlobal(sv)) {}
      
          IdString(std::string_view sv, NoInternTag)
              : mId(lookupGlobal(sv)) {}
      
          static IdString tryLookup(std::string_view sv) {
              return IdString(sv, NoIntern);
          }
      
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
      
          static uint32_t internGlobal(std::string_view sv);
          static uint32_t lookupGlobal(std::string_view sv);
          static const std::string& resolveGlobal(uint32_t id);
          static const std::string& getInvalidStr() {
              static const std::string kInvalidStr = "<Invalid>";
              return kInvalidStr;
          }
      
          struct Pool {
              struct TransparentHash {
                  using is_transparent = void;
      
                  size_t operator()(std::string_view sv) const noexcept {
                      return std::hash<std::string_view>{}(sv);
                  }
                  size_t operator()(const std::string& s) const noexcept {
                      return (*this)(std::string_view{s});
                  }
                  size_t operator()(const char* s) const noexcept {
                      return (*this)(std::string_view{s});
                  }
              };
      
              struct TransparentEqual {
                  using is_transparent = void;
      
                  bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
                      return lhs == rhs;
                  }
                  bool operator()(const std::string& lhs, const std::string& rhs) const noexcept {
                      return lhs == rhs;
                  }
                  bool operator()(const std::string& lhs, std::string_view rhs) const noexcept {
                      return std::string_view{lhs} == rhs;
                  }
                  bool operator()(std::string_view lhs, const std::string& rhs) const noexcept {
                      return lhs == std::string_view{rhs};
                  }
                  bool operator()(const char* lhs, std::string_view rhs) const noexcept {
                      return std::string_view{lhs} == rhs;
                  }
                  bool operator()(std::string_view lhs, const char* rhs) const noexcept {
                      return lhs == std::string_view{rhs};
                  }
              };
      
              std::vector<std::string> mPool;
              std::unordered_map<std::string, uint32_t, TransparentHash, TransparentEqual> mMap;
              std::mutex mMu;
          };
      
          static Pool& pool();
      };
} // namespace hdl