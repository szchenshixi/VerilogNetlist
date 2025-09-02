#pragma once

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "hdl/elab/bits.hpp" // BitAtomKind, BitAtom
#include "hdl/elab/spec.hpp" // ModuleSpec, instances, ports, wires

namespace hdl {
namespace vis {

// Timing model you can fill from your STA
struct TimingArc {
    std::string mFrom; // node id or pin id ("inst.port" or "wire" or "port")
    std::string mTo;
    int mBitFrom = 0;
    int mBitTo = 0;
    double mDelayNs = 0.0;
    std::string mLabel; // optional
};

struct Endpoint {
    std::string mNode; // node or pin id
    int mBit = 0;
};

struct TimingPath {
    std::string mId;
    std::string mName;
    double mSlackNs = 0.0;
    double mDelayNs = 0.0;
    Endpoint mStart;
    Endpoint mEnd;
    std::vector<TimingArc> mArcs;
};

// Build a module-local “view” JSON that matches the visualizer schema.
// It exports:
// - nodes: wires, ports, instances (with pins)
// - edges: for each instance binding, edges from owner (wire/port) to instance
// pin (or reverse for outputs)
// - Optional: you can merge your STA data via addTimingPathsToViewJson
nlohmann::json buildViewJson(const elab::ModuleSpec& spec);

// Merge timing paths into a view JSON (adds "timingPaths" array).
void addTimingPathsToViewJson(nlohmann::json& view,
                              const std::vector<TimingPath>& paths);

// Convenience: write JSON to a file
inline void writeJsonFile(const std::string& path, const nlohmann::json& j) {
    std::ofstream ofs(path);
    if (!ofs)
        throw std::runtime_error("Cannot open file for writing: " + path);
    ofs << j.dump(2) << std::endl;
}

} // namespace vis
} // namespace hdl