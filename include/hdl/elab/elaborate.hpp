#pragma once
// Elaboration entry points with parameters, generate expansion, and instance
// linking.

#include <functional>
#include <memory>
#include <unordered_map>

#include "hdl/elab/flatten.hpp"
#include "hdl/elab/spec.hpp"

namespace hdl::elab {

// Make a canonical key for a module specialization (name + sorted params).
std::string makeModuleKey(std::string_view nameText, const ParamSpec& params);

// Elaborate a module with a given parameter environment.
ModuleSpec elaborateModule(const ast::ModuleDecl& decl,
                           const ParamSpec& paramEnv = {});

// Apply continuous assigns to a module's BitMap connectivity.
void wireAssigns(ModuleSpec& spec);

// Build/lookup a ModuleSpec in the library (by param signature).
ModuleSpec& getOrCreateSpec(const ast::ModuleDecl& decl,
                            const ParamSpec& paramEnv, ModuleSpecLib& lib);

// Link instances declared in spec.mDecl into spec.mInstances (incl. generate
// expansion).
// using ModuleDeclLib =
//   std::unordered_map<IdString, std::reference_wrapper<const
//   ast::ModuleDecl>,
//                      IdString::Hash>;

using ModuleDeclLib =
  std::unordered_map<IdString, const ast::ModuleDecl, IdString::Hash>;
void linkInstances(ModuleSpec& spec, const ModuleDeclLib& declLib,
                   ModuleSpecLib& specLib, std::ostream* diag);

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

} // namespace hdl::elab