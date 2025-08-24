#pragma once
// Elaboration entry points with parameters, generate expansion, and instance
// linking.

#include <functional>
#include <memory>

#include "hdl/elab/flatten.hpp"
#include "hdl/elab/spec.hpp"

namespace hdl {
namespace elab {

// Make a canonical key for a module specialization (name + sorted params).
std::string makeModuleKey(
  std::string_view nameText,
  const std::unordered_map<IdString, int64_t, IdString::Hash>& params);

// Elaborate a module with a given parameter environment.
std::unique_ptr<ModuleSpec> elaborateModule(
  const ast::ModuleDecl& decl,
  const std::unordered_map<IdString, int64_t, IdString::Hash>& paramEnv);

// Apply continuous assigns to a module's BitMap connectivity.
void wireAssigns(ModuleSpec& spec);

// Build/lookup a ModuleSpec in the library (by param signature).
ModuleSpec& getOrCreateSpec(
  const ast::ModuleDecl& decl,
  const std::unordered_map<IdString, int64_t, IdString::Hash>& paramEnv,
  ModuleLibrary& lib);

// Link instances declared in spec.mDecl into spec.mInstances (incl. generate
// expansion).
using ASTIndex =
  std::unordered_map<IdString, std::reference_wrapper<const ast::ModuleDecl>,
                     IdString::Hash>;
void linkInstances(ModuleSpec& spec, const ASTIndex& astIndex,
                   ModuleLibrary& lib, std::ostream& diag);

// Hierarchy dump using ModuleSpec -> ModuleInstance -> ModuleSpec pattern.
namespace hier {

struct ScopeId {
    std::vector<uint32_t> mPath; // child instance indices along hierarchy
    std::string toString() const {
        if (mPath.empty()) return "<root>";
        std::string s;
        for (size_t i = 0; i < mPath.size(); ++i) {
            if (i) s.push_back('/');
            s += std::to_string(mPath[i]);
        }
        return s;
    }
};

struct PinKey {
    ScopeId mScope;
    uint32_t mPortIndex = 0; // child-side port index at end of scope path
};

// Dump instance hierarchy recursively.
void dumpInstanceTree(const ModuleSpec& top, std::ostream& os);

// Optional: derive a PinKey to a named port at a scope path.
bool makePinKey(const ModuleSpec& top, const ScopeId& scope, IdString portName,
                PinKey& out, std::ostream* diag = nullptr);

} // namespace hier

} // namespace elab
} // namespace hdl