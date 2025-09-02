#include "hdl/vis/json.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace hdl {
namespace vis {

using nlohmann::json;

static inline int bitWidth(const elab::PortSpec& p) {
    return static_cast<int>(p.width());
}
static inline int bitWidth(const elab::WireSpec& w) {
    return static_cast<int>(w.width());
}

static std::string makePinId(const elab::hier::ModuleInstance& inst,
                             const elab::PortSpec& formal) {
    return inst.mName.str() + "." + formal.mName.str();
}

// Build nodes: wires, ports, and instances (with pins metadata).
static void buildNodes(const elab::ModuleSpec& spec, json& outNodes) {
    // Wires
    for (const auto& w : spec.mWires) {
        outNodes.push_back({{"id", w.mName.str()},
                            {"type", "wire"},
                            {"name", w.mName.str()},
                            {"msb", w.mEnt.mMsb},
                            {"lsb", w.mEnt.mLsb}});
    }
    // Ports (module-level)
    for (const auto& p : spec.mPorts) {
        outNodes.push_back({{"id", p.mName.str()},
                            {"type", "port"},
                            {"name", p.mName.str()},
                            {"dir", std::string(to_string(p.mDir))},
                            {"msb", p.mEnt.mMsb},
                            {"lsb", p.mEnt.mLsb}});
    }
    // Instances
    for (const auto& inst : spec.mInstances) {
        json jinst = {
          {"id", inst.mName.str()},
          {"type", "instance"},
          {"name", inst.mName.str()},
          {"module",
           inst.mCallee ? inst.mCallee->mName.str() : std::string("<null>")},
          {"pins", json::array()}};
        if (inst.mCallee) {
            for (const auto& fp : inst.mCallee->mPorts) {
                jinst["pins"].push_back(
                  {{"id", makePinId(inst, fp)},
                   {"name", fp.mName.str()},
                   {"dir", std::string(to_string(fp.mDir))},
                   {"width", bitWidth(fp)}});
            }
        }
        outNodes.push_back(std::move(jinst));
    }
}

// Group a formal binding's actual BitVector by owner (wire/port), keeping
// mapping.
struct Segment {
    std::string mOwnerId;
    elab::BitAtomKind mKind = elab::BitAtomKind::WireBit;
    int mFormalOffset0 = 0; // first formal bit index in this segment
    std::vector<std::pair<int, int>>
      mMapping; // {fromBit (actual owner), toBit (formal)}
};

static std::vector<Segment>
segmentsForBinding(const elab::ModuleSpec& spec,
                   const elab::hier::ModuleInstance& inst, int formalIdx,
                   const elab::BitVector& actual) {
    std::vector<Segment> segs;
    if (!inst.mCallee) return segs;
    // Formal width
    const auto& formal = inst.mCallee->mPorts[formalIdx];
    const int Wf = bitWidth(formal);
    (void)Wf;

    auto ownerName = [&](const elab::BitAtom& a) -> std::string {
        return a.mOwnerIndex.valid() ? a.mOwnerIndex.str()
                                  : std::string("<unknown>");
    };

    int i = 0;
    while (i < static_cast<int>(actual.size())) {
        const auto& a0 = actual[i];
        if (!(a0.mKind == elab::BitAtomKind::WireBit ||
              a0.mKind == elab::BitAtomKind::PortBit)) {
            // Skip constants (demo); in production, make a special segment
            // (Const)
            ++i;
            continue;
        }
        Segment s;
        s.mOwnerId = ownerName(a0);
        s.mKind = a0.mKind;
        s.mFormalOffset0 = i;

        int j = i;
        for (; j < static_cast<int>(actual.size()); ++j) {
            const auto& ax = actual[j];
            if (ax.mKind != s.mKind) break;
            if (ownerName(ax) != s.mOwnerId) break;
            const int toBit = j; // formal bit offset
            const int fromBit =
              static_cast<int>(ax.mBitIndex); // actual bit in owner
            s.mMapping.emplace_back(fromBit, toBit);
        }
        segs.push_back(std::move(s));
        i = j;
    }
    return segs;
}

static std::string dirToStr(PortDirection d) { return to_string(d); }

// Build edges from instance port bindings (using per-owner segments).
static void buildEdges(const elab::ModuleSpec& spec, json& outEdges) {
    for (const auto& inst : spec.mInstances) {
        if (!inst.mCallee) continue;

        for (const auto& pb : inst.mConnections) {
            const int formalIdx = static_cast<int>(pb.mFormalIndex);
            const auto& formal = inst.mCallee->mPorts[formalIdx];
            const std::string pinId = makePinId(inst, formal);
            const std::string formalDir = dirToStr(formal.mDir);

            auto segs = segmentsForBinding(spec, inst, formalIdx, pb.mActual);
            int segCount = 0;
            for (const auto& s : segs) {
                // Choose edge direction
                std::string fromId, toId;
                if (formal.mDir == PortDirection::In) {
                    // data flows owner -> instance pin
                    fromId = s.mOwnerId;
                    toId = pinId;
                } else if (formal.mDir == PortDirection::Out) {
                    fromId = pinId;
                    toId = s.mOwnerId;
                } else { // InOut: make bidirectional look; we still draw owner
                         // -> pin
                    fromId = s.mOwnerId;
                    toId = pinId;
                }

                json mapping = json::array();
                for (const auto& pr : s.mMapping) {
                    mapping.push_back(
                      {{"fromBit", pr.first}, {"toBit", pr.second}});
                }

                std::ostringstream eid;
                eid << "e_" << inst.mName.str() << "_" << formal.mName.str()
                    << "_" << segCount++ << "_"
                    << (formal.mDir == PortDirection::In ? "in" : "out");

                const int width = static_cast<int>(s.mMapping.size());
                std::ostringstream lab;
                lab << (formal.mDir == PortDirection::In
                          ? s.mOwnerId + " → " + pinId
                          : pinId + " → " + s.mOwnerId);

                outEdges.push_back({{"id", eid.str()},
                                    {"from", fromId},
                                    {"to", toId},
                                    {"width", width},
                                    {"label", lab.str()},
                                    {"mapping", mapping}});
            }
        }
    }
}

nlohmann::json buildViewJson(const elab::ModuleSpec& spec) {
    json view;
    view["key"] = spec.mName.str();
    view["title"] = spec.mName.str();
    view["description"] = "Module view exported from ModuleSpec (ports, "
                          "wires, instances, pins, and edges).";
    view["nodes"] = json::array();
    view["edges"] = json::array();
    view["timingPaths"] = json::array();

    buildNodes(spec, view["nodes"]);
    buildEdges(spec, view["edges"]);
    return view;
}

void addTimingPathsToViewJson(nlohmann::json& view,
                              const std::vector<TimingPath>& paths) {
    auto& arr = view["timingPaths"];
    for (const auto& p : paths) {
        nlohmann::json jp;
        jp["id"] = p.mId;
        jp["name"] = p.mName;
        jp["slack"] = p.mSlackNs;
        jp["delay"] = p.mDelayNs;
        jp["start"] = {{"node", p.mStart.mNode}, {"bit", p.mStart.mBit}};
        jp["end"] = {{"node", p.mEnd.mNode}, {"bit", p.mEnd.mBit}};
        nlohmann::json arcs = nlohmann::json::array();
        for (const auto& a : p.mArcs) {
            arcs.push_back({{"from", a.mFrom},
                            {"to", a.mTo},
                            {"bitFrom", a.mBitFrom},
                            {"bitTo", a.mBitTo},
                            {"delay", a.mDelayNs},
                            {"label", a.mLabel}});
        }
        jp["arcs"] = std::move(arcs);
        arr.push_back(std::move(jp));
    }
}

} // namespace vis
} // namespace hdl