#pragma once

#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <tcl.h>

#include "hdl/elab/elaborate.hpp"
#include "hdl/elab/spec.hpp"
#include "hdl/util/id_string.hpp"

namespace hdl {
namespace tcl {

// Forward decl
class Console;

// Public, namespace-level selection types
struct SelRef {
    IdString mSpecKey; // library specialization key
    IdString mName;    // port/wire name
};

struct Selection {
    IdString mPrimaryKey;
    std::vector<IdString> mModuleKeys;
    std::vector<SelRef> mPorts;
    std::vector<SelRef> mWires;

    void clearAll() {
        mPrimaryKey = IdString();
        mModuleKeys.clear();
        mPorts.clear();
        mWires.clear();
    }
    bool hasModuleKey(IdString key) const {
        for (auto& k : mModuleKeys)
            if (k == key) return true;
        return false;
    }
    void addModuleKey(IdString key) {
        if (!hasModuleKey(key)) mModuleKeys.push_back(key);
    }
    void removeModuleKey(IdString key) {
        mModuleKeys.erase(
          std::remove(mModuleKeys.begin(), mModuleKeys.end(), key),
          mModuleKeys.end());
        if (mPrimaryKey == key) mPrimaryKey = IdString();
        mPorts.erase(
          std::remove_if(mPorts.begin(),
                         mPorts.end(),
                         [&](const SelRef& r) { return r.mSpecKey == key; }),
          mPorts.end());
        mWires.erase(
          std::remove_if(mWires.begin(),
                         mWires.end(),
                         [&](const SelRef& r) { return r.mSpecKey == key; }),
          mWires.end());
    }
    bool hasPort(const SelRef& ref) const {
        for (auto& r : mPorts)
            if (r.mSpecKey == ref.mSpecKey && r.mName == ref.mName)
                return true;
        return false;
    }
    bool hasWire(const SelRef& ref) const {
        for (auto& r : mWires)
            if (r.mSpecKey == ref.mSpecKey && r.mName == ref.mName)
                return true;
        return false;
    }
    void addPort(const SelRef& ref) {
        if (!hasPort(ref)) mPorts.push_back(ref);
    }
    void removePort(const SelRef& ref) {
        mPorts.erase(std::remove_if(mPorts.begin(),
                                    mPorts.end(),
                                    [&](const SelRef& r) {
                                        return r.mSpecKey == ref.mSpecKey &&
                                               r.mName == ref.mName;
                                    }),
                     mPorts.end());
    }
    void addWire(const SelRef& ref) {
        if (!hasWire(ref)) mWires.push_back(ref);
    }
    void removeWire(const SelRef& ref) {
        mWires.erase(std::remove_if(mWires.begin(),
                                    mWires.end(),
                                    [&](const SelRef& r) {
                                        return r.mSpecKey == ref.mSpecKey &&
                                               r.mName == ref.mName;
                                    }),
                     mWires.end());
    }
};

class Console {
  public:
    using Args = std::vector<std::string>;

    // Use function pointers for clarity and to avoid capturing lambdas.
    using Handler = int (*)(Console&, Tcl_Interp*, const Args&);
    using Completer = std::vector<std::string> (*)(Console&, const Args&);
    using ReverseBuilder =
      std::vector<std::string> (*)(Console&, const std::string& sub,
                                   const Args& args, const Selection& preSel);

    struct Subcmd {
        std::string mName;
        std::string mHelp;
        Handler mHandler = nullptr;
        Completer mCompleter = nullptr;
        ReverseBuilder mReverse = nullptr;
    };

    struct UndoEntry {
        std::string redoCmd; // textual forward command: "hdl <sub> <args...>"
        std::vector<std::string>
          undoCmds;        // textual reverse commands (0..N lines)
        std::string label; // subcommand name
    };

  public:
    Console(elab::ModuleLibrary& lib, const elab::ASTIndex& astIndex,
            std::ostream& diag);
    ~Console();

    bool init();
    int repl();
    int evalLine(const std::string& line);

    // Registration API
    void registerCommand(const std::string& name, const std::string& help,
                         Handler handler, Completer completer = nullptr,
                         ReverseBuilder reverse = nullptr);
    // Register all built-in commands
    void registerAllBuiltins();

    // Lookup and reverse-plan API (used by invert command)
    bool hasCommand(const std::string& name) const;
    std::vector<std::string> computeReversePlan(const std::string& sub,
                                                const Args& args,
                                                const Selection& preSel) const;

    // Helpers (public so command files can reuse them)
    static std::string makeCmdLine(const std::string& sub, const Args& args);
    static std::unordered_map<IdString, int64_t, IdString::Hash>
    parseParamTokens(const std::vector<std::string>& toks, size_t startIdx,
                     std::ostream& diag);

    // Completions helpers (optional use)
    std::vector<std::string>
    completeModules(const std::string& prefix) const; // AST names
    std::vector<std::string>
    completeSpecKeys(const std::string& prefix) const; // lib keys
    std::vector<std::string>
    completePortsForKey(const std::string& key,
                        const std::string& prefix) const;
    std::vector<std::string>
    completeWiresForKey(const std::string& key,
                        const std::string& prefix) const;
    std::vector<std::string> completeParams(IdString moduleName,
                                            const std::string& prefix) const;

    // Library helpers
    elab::ModuleSpec* getSpecByKey(IdString key);
    elab::ModuleSpec* getOrElabByName(
      IdString name,
      const std::unordered_map<IdString, int64_t, IdString::Hash>& env,
      IdString* outKey = nullptr);
    elab::ModuleSpec* currentPrimarySpec();

    bool resolvePortName(const elab::ModuleSpec& spec, const std::string& tok,
                         IdString& out) const;
    bool resolveWireName(const elab::ModuleSpec& spec, const std::string& tok,
                         IdString& out) const;

    // Undo/Redo entry points (used by cmd_undo/cmd_redo)
    int doUndo(Tcl_Interp* ip);
    int doRedo(Tcl_Interp* ip);

    // Accessors
    Tcl_Interp* interp() const { return mInterp; }
    Selection& selection() { return mSel; }
    const Selection& selection() const { return mSel; }
    elab::ModuleLibrary& lib() { return mLib; }
    const elab::ASTIndex& astIndex() const { return mAstIndex; }

#ifdef HDL_HAVE_READLINE
    static char** complt(const char* text, int start, int end);
    static Console* s_completion_self;
#endif

  private:
    // Tcl entrypoint and core dispatch
    static int TclHdl(ClientData cd, Tcl_Interp* interp, int objc,
                      Tcl_Obj* const objv[]);
    int dispatch(Tcl_Interp* interp, const Args& argv);

    static std::string toStd(Tcl_Obj* obj);
    static Args toArgs(int objc, Tcl_Obj* const objv[]);
    std::vector<std::string> complete(const std::string& line,
                                      size_t cursorPos);
    std::vector<std::string>
    completeSubcommand(const std::string& prefix) const;

    // Record an undo entry
    void recordUndo(const std::string& redoCmd,
                    const std::vector<std::string>& undoCmds,
                    const std::string& label);

  private:
    Tcl_Interp* mInterp = nullptr;
    std::unordered_map<std::string, Subcmd> mSubcmds;

    elab::ModuleLibrary& mLib;
    const elab::ASTIndex& mAstIndex;
    Selection mSel;
    std::ostream& mDiag;

    std::vector<UndoEntry> mUndo;
    std::vector<UndoEntry> mRedo;
    bool mInReplay = false; // avoid re-recording when running undo/redo
};

} // namespace tcl
} // namespace hdl