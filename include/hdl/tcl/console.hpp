#pragma once

#include <algorithm>
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

namespace hdl::tcl {

// Public, namespace-level selection types (interned via IdString)
struct SelRef {
    IdString mSpecKey; // library specialization key (interned)
    IdString mName;    // port/wire name (interned)
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

    // Function-pointer based handlers to avoid capturing lambdas in the core
    using Handler = int (*)(Console&, Tcl_Interp*, const Args&);
    using Completer = std::vector<std::string> (*)(Console&, const Args&);
    using ReverseBuilder =
      std::vector<std::string> (*)(Console&, const std::string& sub,
                                   const Args& args, const Selection& preSel);

    struct Subcmd {
        std::string mName;
        std::string mHelp; // one-line description (include usage if you wish)
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
    Console(elab::ModuleSpecLib& lib, const elab::ModuleDeclLib& ModuleDeclLib,
            std::ostream& diag);
    ~Console();

    bool init();
    int repl();
    int evalLine(const std::string& line);

    // Registration API: registers a top-level Tcl command with name.
    void registerCommand(const std::string& name, const std::string& help,
                         Handler handler, Completer completer = nullptr,
                         ReverseBuilder reverse = nullptr);

    // Lookup and reverse-plan API (used by invert command)
    bool hasCommand(const std::string& name) const;
    std::vector<std::string> computeReversePlan(const std::string& sub,
                                                const Args& args,
                                                const Selection& preSel) const;

    // NEW: public registry queries for help/commands
    std::vector<std::pair<std::string, std::string>>
    listCommands() const; // (name, help)
    bool getCommandHelp(const std::string& name, std::string& outHelp) const;

    // Utilities (public so command files can reuse them)
    static std::string makeCmdLine(const std::string& sub, const Args& args);
    static elab::ParamSpec
    parseParamTokens(const std::vector<std::string>& toks, size_t startIdx,
                     std::ostream* diag);

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
    std::vector<std::string> completeParams(const std::string& moduleName,
                                            const std::string& prefix) const;

    // Library helpers
    elab::ModuleSpec* getSpecByKey(std::string key);
    elab::ModuleSpec* getOrElabByName(std::string name,
                                      const elab::ParamSpec& env,
                                      IdString* outKey = nullptr);
    elab::ModuleSpec* currentPrimarySpec();

    bool resolvePortName(const elab::ModuleSpec& spec, const std::string& tok,
                         IdString& out) const;
    bool resolveWireName(const elab::ModuleSpec& spec, const std::string& tok,
                         IdString& out) const;

    // Undo/Redo entry points
    int doUndo(Tcl_Interp* ip);
    int doRedo(Tcl_Interp* ip);

    // Accessors
    Tcl_Interp* interp() const { return mInterp; }
    Selection& selection() { return mSel; }
    const Selection& selection() const { return mSel; }
    elab::ModuleSpecLib& specLib() { return mSpecLib; }
    const elab::ModuleDeclLib& declLib() const { return mDeclLib; }

    // Register all built-in commands (implemented in
    // src/tcl/cmd/register_all.cpp)
    void registerAllBuiltins();
#ifdef HDL_HAVE_READLINE
    static char** complt(const char* text, int start, int end);
    static char* compltGen(const char* text, int state);
#endif
    static Console* sSelf;
    static void signalHandler(int signal);

  private:
    // Tcl entrypoint for all top-level commands
    static int TclCmd(ClientData cd, Tcl_Interp* interp, int objc,
                      Tcl_Obj* const objv[]);
    int dispatchCommand(Tcl_Interp* interp, const std::string& cmdName,
                        const Args& args);

    static std::string toStd(Tcl_Obj* obj);
    static Args toArgs(int objc, Tcl_Obj* const objv[]);
    std::vector<std::string> complete(const std::string& line);
    std::vector<std::string>
    completeCommandNames(const std::string& prefix) const;

    // Record an undo entry
    void recordUndo(const std::string& redoCmd,
                    const std::vector<std::string>& undoCmds,
                    const std::string& label);

  private:
    Tcl_Interp* mInterp = nullptr;
    std::unordered_map<std::string, Subcmd> mSubcmds;

    elab::ModuleSpecLib& mSpecLib;
    const elab::ModuleDeclLib& mDeclLib;
    Selection mSel;
    std::ostream& mDiag;

    std::vector<UndoEntry> mUndo;
    std::vector<UndoEntry> mRedo;
    bool mInReplay = false; // avoid re-recording when running undo/redo

  private:
    void warn(const std::string& msg) const;
    void error(const std::string& msg) const;
};

} // namespace hdl::tcl