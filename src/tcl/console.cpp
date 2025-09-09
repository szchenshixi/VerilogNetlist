#include "hdl/tcl/console.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>
#include <sstream>

#ifdef HDL_HAVE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include "hdl/ast/decl.hpp"
#include "hdl/elab/elaborate.hpp"
#include "hdl/elab/spec.hpp"
#include "hdl/hier/instance.hpp"

// Register-all header (local to src tree)
#include "cmd/register_all.hpp"

namespace hdl::tcl {

static inline bool starts_with(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && std::equal(p.begin(), p.end(), s.begin());
}
static inline std::string lstrip_ws(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                return !std::isspace(ch);
            }));
    return s;
}
static inline std::vector<std::string> split_words(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok)
        out.push_back(tok);
    return out;
}

Console::Console(elab::ModuleLibrary& lib, const elab::ASTIndex& astIndex,
                 std::ostream& diag)
    : mLib(lib)
    , mAstIndex(astIndex)
    , mDiag(diag) {}

Console::~Console() {
    if (mInterp) {
        Tcl_DeleteInterp(mInterp);
        mInterp = nullptr;
    }
}

bool Console::init() {
    mInterp = Tcl_CreateInterp();
    if (!mInterp) return false;
    if (Tcl_Init(mInterp) != TCL_OK) {
        mDiag << "ERROR: Tcl_Init failed: " << Tcl_GetStringResult(mInterp)
              << "\n";
        return false;
    }
    registerAllBuiltins();
    return true;
}

void Console::registerAllBuiltins() { register_all_commands(*this); }

void Console::registerCommand(const std::string& name, const std::string& help,
                              Handler handler, Completer completer,
                              ReverseBuilder reverse) {
    mSubcmds[name] = Subcmd{name, help, handler, completer, reverse};
    // Create a top-level Tcl command
    Tcl_CreateObjCommand(
      mInterp, name.c_str(), &Console::TclCmd, this, nullptr);
}

bool Console::hasCommand(const std::string& name) const {
    return mSubcmds.find(name) != mSubcmds.end();
}

std::vector<std::string>
Console::computeReversePlan(const std::string& sub, const Args& args,
                            const Selection& preSel) const {
    auto it = mSubcmds.find(sub);
    if (it == mSubcmds.end() || !it->second.mReverse) return {};
    // const_cast to allow ReverseBuilder signature receiving Console&
    return it->second.mReverse(const_cast<Console&>(*this), sub, args, preSel);
}

// NEW: list commands and get help by name
std::vector<std::pair<std::string, std::string>>
Console::listCommands() const {
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(mSubcmds.size());
    for (const auto& kv : mSubcmds)
        out.emplace_back(kv.first, kv.second.mHelp);
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    return out;
}

bool Console::getCommandHelp(const std::string& name,
                             std::string& outHelp) const {
    auto it = mSubcmds.find(name);
    if (it == mSubcmds.end()) return false;
    outHelp = it->second.mHelp;
    return true;
}

int Console::evalLine(const std::string& line) {
    if (line.empty()) return 0;
    int code = Tcl_EvalEx(
      mInterp, line.c_str(), static_cast<int>(line.size()), TCL_EVAL_GLOBAL);
    if (code != TCL_OK) {
        mDiag << "Tcl error: " << Tcl_GetStringResult(mInterp) << "\n";
    } else {
        const char* res = Tcl_GetStringResult(mInterp);
        if (res && std::strlen(res) > 0) mDiag << res << "\n";
    }
    return (code == TCL_OK) ? 0 : 1;
}

#ifdef HDL_HAVE_READLINE
Console* Console::s_completion_self = nullptr;
char** Console::complt(const char* text, int start, int end) {
    (void)end;
    Console* self = s_completion_self;
    if (!self) return nullptr;

    std::string buf(rl_line_buffer ? rl_line_buffer : "");
    auto candidates = self->complete(buf, static_cast<size_t>(start));
    if (candidates.empty()) return nullptr;

    std::string cur(text ? text : "");
    std::vector<char*> arr;
    arr.reserve(candidates.size());
    for (auto& c : candidates)
        if (cur.empty() || starts_with(c, cur))
            arr.push_back(::strdup(c.c_str()));

    if (arr.empty()) return nullptr;

    char** matches =
      static_cast<char**>(std::malloc((arr.size() + 1) * sizeof(char*)));
    for (size_t i = 0; i < arr.size(); ++i)
        matches[i] = arr[i];
    matches[arr.size()] = nullptr;
    return matches;
}
#endif

int Console::repl() {
#ifdef HDL_HAVE_READLINE
    Console::s_completion_self = this;
    rl_attempted_completion_function = &Console::complt;
#endif
    mDiag << "HDL Tcl console. Type: help\n";
    mDiag << "Press Ctrl+D to exit.\n";
    while (true) {
#ifdef HDL_HAVE_READLINE
        char* line = readline("> ");
        if (!line) break;
        std::string s(line);
        free(line);
        s = lstrip_ws(s);
        if (s.empty()) continue;
        add_history(s.c_str());
        (void)evalLine(s);
#else
        std::string s;
        mDiag << "> " << std::flush;
        if (!std::getline(std::cin, s)) break;
        s = lstrip_ws(s);
        if (s.empty()) continue;
        (void)evalLine(s);
#endif
    }
    mDiag << "Bye.\n";
    return 0;
}

int Console::TclCmd(ClientData cd, Tcl_Interp* interp, int objc,
                    Tcl_Obj* const objv[]) {
    auto* self = reinterpret_cast<Console*>(cd);
    if (!self) return TCL_ERROR;
    if (objc < 1) return TCL_ERROR;
    std::string cmdName = toStd(objv[0]);
    Args args;
    args.reserve((size_t)std::max(0, objc - 1));
    for (int i = 1; i < objc; ++i)
        args.push_back(toStd(objv[i]));
    return self->dispatchCommand(interp, cmdName, args);
}

int Console::dispatchCommand(Tcl_Interp* interp, const std::string& cmdName,
                             const Args& args) {
    auto it = mSubcmds.find(cmdName);
    if (it == mSubcmds.end()) {
        std::string msg = "unknown command: " + cmdName;
        Tcl_SetObjResult(interp, Tcl_NewStringObj(msg.c_str(), -1));
        return TCL_ERROR;
    }
    Selection preSel = mSel; // snapshot for reverse-builder
    int code = it->second.mHandler ? it->second.mHandler(*this, interp, args)
                                   : TCL_ERROR;
    if (code == TCL_OK && !mInReplay && it->second.mReverse) {
        auto undoCmds = it->second.mReverse(*this, cmdName, args, preSel);
        if (!undoCmds.empty()) {
            recordUndo(makeCmdLine(cmdName, args), undoCmds, cmdName);
        }
    }
    return code;
}

std::string Console::toStd(Tcl_Obj* obj) {
    int len = 0;
    const char* s = Tcl_GetStringFromObj(obj, &len);
    return std::string(s, (size_t)len);
}
Console::Args Console::toArgs(int objc, Tcl_Obj* const objv[]) {
    Args out;
    out.reserve((size_t)objc);
    for (int i = 0; i < objc; ++i)
        out.push_back(toStd(objv[i]));
    return out;
}

std::vector<std::string> Console::complete(const std::string& line,
                                           size_t cursorPos) {
    // (void)cursorPos;
    // auto toks = split_words(line);
    // if (toks.empty()) return {"hdl"};
    // if (toks[0] != "hdl") {
    //     if (toks.size() == 1 && starts_with("hdl", toks[0])) return {"hdl"};
    //     return {};
    // }
    // if (toks.size() == 1) return completeSubcommand("");
    // if (toks.size() == 2) return completeSubcommand(toks[1]);
    // const std::string& sub = toks[1];
    // auto it = mSubcmds.find(sub);
    // if (it != mSubcmds.end() && it->second.mCompleter)
    //     return it->second.mCompleter(*this, toks);
    // return {};
    (void)cursorPos;
    auto toks = split_words(line);
    const bool endsSpace = (!line.empty() && std::isspace(line.back()));
    if (endsSpace) toks.push_back("");

    auto sortUnique = [](std::vector<std::string> v) {
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
        return v;
    };

    // At empty prompt -> list all commands
    if (toks.empty()) return completeCommandNames("");

    // Completing the command name
    if (toks.size() == 1) return completeCommandNames(toks[0]);

    // Command args: delegate to that command's completer
    const std::string& cmd = toks[0];
    auto it = mSubcmds.find(cmd);
    if (it != mSubcmds.end() && it->second.mCompleter)
        return it->second.mCompleter(*this, toks);
    return {};
}

std::vector<std::string>
Console::completeCommandNames(const std::string& prefix) const {
    std::vector<std::string> r;
    r.reserve(mSubcmds.size());
    for (auto& kv : mSubcmds)
        if (prefix.empty() || starts_with(kv.first, prefix))
            r.push_back(kv.first);
    std::sort(r.begin(), r.end());
    return r;
}

// Public helpers

std::string Console::makeCmdLine(const std::string& sub, const Args& args) {
    std::ostringstream oss;
    oss << sub;
    for (size_t i = 0; i < args.size(); ++i)
        oss << ' ' << args[i];
    return oss.str();
}

std::unordered_map<IdString, int64_t, IdString::Hash>
Console::parseParamTokens(const std::vector<std::string>& toks,
                          size_t startIdx, std::ostream& diag) {
    std::unordered_map<IdString, int64_t, IdString::Hash> env;
    for (size_t i = startIdx; i < toks.size(); ++i) {
        const std::string& t = toks[i];
        auto eq = t.find('=');
        if (eq == std::string::npos || eq == 0 || eq + 1 >= t.size()) {
            diag << "WARN: ignoring param token (expect NAME=VALUE): " << t
                 << "\n";
            continue;
        }
        std::string name = t.substr(0, eq);
        std::string val = t.substr(eq + 1);
        try {
            long long v = std::stoll(val);
            env[IdString(name)] = v;
        } catch (...) {
            diag << "WARN: non-integer param value: " << t << "\n";
        }
    }
    return env;
}

std::vector<std::string>
Console::completeModules(const std::string& prefix) const {
    std::vector<std::string> r;
    for (auto& kv : mAstIndex) {
        const auto& name = kv.first.str();
        if (prefix.empty() || name.rfind(prefix, 0) == 0) r.push_back(name);
    }
    std::sort(r.begin(), r.end());
    return r;
}
std::vector<std::string>
Console::completeSpecKeys(const std::string& prefix) const {
    std::vector<std::string> r;
    for (auto& kv : mLib)
        if (prefix.empty() || kv.first.rfind(prefix, 0) == 0)
            r.push_back(kv.first);
    std::sort(r.begin(), r.end());
    return r;
}
std::vector<std::string>
Console::completePortsForKey(const std::string& key,
                             const std::string& prefix) const {
    std::vector<std::string> r;
    auto it = mLib.find(key);
    if (it == mLib.end()) return r;
    const auto& spec = *it->second;
    for (auto& p : spec.mPorts) {
        auto s = p.mName.str();
        if (prefix.empty() || s.rfind(prefix, 0) == 0) r.push_back(s);
    }
    std::sort(r.begin(), r.end());
    return r;
}
std::vector<std::string>
Console::completeWiresForKey(const std::string& key,
                             const std::string& prefix) const {
    std::vector<std::string> r;
    auto it = mLib.find(key);
    if (it == mLib.end()) return r;
    const auto& spec = *it->second;
    for (auto& w : spec.mWires) {
        auto s = w.mName.str();
        if (prefix.empty() || s.rfind(prefix, 0) == 0) r.push_back(s);
    }
    std::sort(r.begin(), r.end());
    return r;
}
std::vector<std::string>
Console::completeParams(IdString moduleName,
                        const std::string& /*prefix*/) const {
    std::vector<std::string> r;
    auto it = mAstIndex.find(moduleName);
    if (it == mAstIndex.end()) return r;
    const ast::ModuleDecl& d = it->second.get();
    for (auto& p : d.mParams)
        r.push_back(p.mName.str() + "=");
    std::sort(r.begin(), r.end());
    return r;
}

elab::ModuleSpec* Console::getSpecByKey(IdString key) {
    if (key == IdString()) return nullptr;
    auto it = mLib.find(key.str());
    if (it == mLib.end()) return nullptr;
    return it->second.get();
}
elab::ModuleSpec* Console::getOrElabByName(
  IdString name,
  const std::unordered_map<IdString, int64_t, IdString::Hash>& env,
  IdString* outKey) {
    auto itAst = mAstIndex.find(name);
    if (itAst == mAstIndex.end()) return nullptr;
    elab::ModuleSpec& s =
      elab::getOrCreateSpec(itAst->second.get(), env, mLib);
    elab::linkInstances(s, mAstIndex, mLib, mDiag);
    IdString key(elab::makeModuleKey(name.str(), env));
    if (outKey) *outKey = key;
    return &s;
}
elab::ModuleSpec* Console::currentPrimarySpec() {
    if (mSel.mPrimaryKey == IdString()) return nullptr;
    return getSpecByKey(mSel.mPrimaryKey);
}
bool Console::resolvePortName(const elab::ModuleSpec& spec,
                              const std::string& tok, IdString& out) const {
    bool num = !tok.empty() && std::all_of(tok.begin(), tok.end(), ::isdigit);
    if (num) {
        int idx = 0;
        try {
            idx = std::stoi(tok);
        } catch (...) { return false; }
        if (idx < 0 || (size_t)idx >= spec.mPorts.size()) return false;
        out = spec.mPorts[(size_t)idx].mName;
        return true;
    }
    IdString n(tok);
    if (spec.findPortIndex(n) >= 0) {
        out = n;
        return true;
    }
    return false;
}
bool Console::resolveWireName(const elab::ModuleSpec& spec,
                              const std::string& tok, IdString& out) const {
    bool num = !tok.empty() && std::all_of(tok.begin(), tok.end(), ::isdigit);
    if (num) {
        int idx = 0;
        try {
            idx = std::stoi(tok);
        } catch (...) { return false; }
        if (idx < 0 || (size_t)idx >= spec.mWires.size()) return false;
        out = spec.mWires[(size_t)idx].mName;
        return true;
    }
    IdString n(tok);
    if (spec.findWireIndex(n) >= 0) {
        out = n;
        return true;
    }
    return false;
}

// Undo/redo
void Console::recordUndo(const std::string& redoCmd,
                         const std::vector<std::string>& undoCmds,
                         const std::string& label) {
    mUndo.push_back(UndoEntry{redoCmd, undoCmds, label});
    mRedo.clear();
}
int Console::doUndo(Tcl_Interp* ip) {
    if (mUndo.empty()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("nothing to undo", -1));
        return TCL_OK;
    }
    UndoEntry e = mUndo.back();
    mUndo.pop_back();
    mInReplay = true;
    for (auto& cmd : e.undoCmds) {
        int code =
          Tcl_EvalEx(mInterp, cmd.c_str(), (int)cmd.size(), TCL_EVAL_GLOBAL);
        if (code != TCL_OK) {
            mInReplay = false;
            std::string msg = "undo failed at: " + cmd +
                              " error: " + Tcl_GetStringResult(mInterp);
            Tcl_SetObjResult(ip, Tcl_NewStringObj(msg.c_str(), -1));
            return TCL_ERROR;
        }
    }
    mInReplay = false;
    mRedo.push_back(e);
    Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
    return TCL_OK;
}
int Console::doRedo(Tcl_Interp* ip) {
    if (mRedo.empty()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("nothing to redo", -1));
        return TCL_OK;
    }
    UndoEntry e = mRedo.back();
    mRedo.pop_back();
    mInReplay = true;
    int code = Tcl_EvalEx(
      mInterp, e.redoCmd.c_str(), (int)e.redoCmd.size(), TCL_EVAL_GLOBAL);
    mInReplay = false;
    if (code != TCL_OK) {
        std::string msg = "redo failed: " + e.redoCmd + " error: " +
                          std::string(Tcl_GetStringResult(mInterp));
        Tcl_SetObjResult(ip, Tcl_NewStringObj(msg.c_str(), -1));
        return TCL_ERROR;
    }
    mUndo.push_back(e);
    Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
    return TCL_OK;
}

} // namespace hdl::tcl