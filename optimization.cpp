#include "optimization.h"
#include <climits>
#include <set>
#include <vector>
#include <algorithm>

static int dotProduct(const std::vector<int>& m, const std::vector<int>& c) {
    int s = 0;
    size_t n = std::min(m.size(), c.size());
    for (size_t i = 0; i < n; ++i) s += m[i] * c[i];
    return s;
}

// lấy một marking đại diện từ BDD rồi flip một số bit,
// kiểm tra vẫn thuộc reachable set.
static bool pickNextMarkingCandidates(BDDWrapper& mgr,
                                      DdNode* set_bdd,
                                      std::vector<std::vector<int>>& out_candidates,
                                      int limit = 64) 
{
#ifndef NO_CUDD
    Marking base = mgr.PickOneMarking(set_bdd);
    if (base.empty()) return false;

    out_candidates.push_back(base);
    if (limit <= 1) return true;

    int n = (int)base.size();
    int produced = 1;

    for (int i = 0; i < n && produced < limit; ++i) {
        std::vector<int> neighb= base;
        neighb[i] = 1 - neighb[i]; // flip bit

        DdNode* m_bdd = mgr.BuildMarkingBDD(neighb); mgr.Ref(m_bdd);
        DdNode* inter = Cudd_bddAnd(mgr.manager, set_bdd, m_bdd); mgr.Ref(inter);
        bool ok = (inter != Cudd_ReadLogicZero(mgr.manager));
        mgr.Deref(m_bdd);
        mgr.Deref(inter);

        if (ok) {
            out_candidates.push_back(neighb);
            produced++;
        }
    }

    return true;
#else
    return false;
#endif
}

std::pair<std::vector<int>, int>
MarkingOptimizerBB::maxReachableMarking(const std::vector<std::string>& place_ids,
                                        DdNode* reachable_bdd,
                                        const std::vector<int>& objective_vector,
                                        PetriNetAnalysis& app) 
{
    int best_val = INT_MIN;
    std::vector<int> best_marking;

#ifndef NO_CUDD
    if (reachable_bdd) {
        std::vector<std::vector<int>> candidates;
        bool ok = pickNextMarkingCandidates(app.bdd_mgr, reachable_bdd, candidates, 128);

        if (ok && !candidates.empty()) {
            for (const auto& m : candidates) {
                int val = dotProduct(m, objective_vector);
                if (val > best_val) {
                    best_val = val;
                    best_marking = m;
                }
            }
        } else {
            Marking base = app.bdd_mgr.PickOneMarking(reachable_bdd);
            if (!base.empty()) {
                int n = (int)base.size();
                for (int i = 0; i < n; ++i) {
                    std::vector<int> m1 = base; m1[i] = 1 - m1[i];
                    DdNode* m_bdd = app.bdd_mgr.BuildMarkingBDD(m1); app.bdd_mgr.Ref(m_bdd);
                    DdNode* inter = Cudd_bddAnd(app.bdd_mgr.manager, reachable_bdd, m_bdd); app.bdd_mgr.Ref(inter);
                    if (inter != Cudd_ReadLogicZero(app.bdd_mgr.manager)) {
                        int val = dotProduct(m1, objective_vector);
                        if (val > best_val) { best_val = val; best_marking = m1; }
                    }
                    app.bdd_mgr.Deref(m_bdd);
                    app.bdd_mgr.Deref(inter);
                }
                int val_base = dotProduct(base, objective_vector);
                if (val_base > best_val) { best_val = val_base; best_marking = base; }
            }
        }

        if (best_val != INT_MIN) {
            return { best_marking, best_val };
        }
        // nếu chưa có, fallback xuống explicit
    }
#endif

    long long t_ms = 0;
    std::set<Marking> reachable = app.ComputeExplicit(t_ms);
    for (const auto& m : reachable) {
        int val = dotProduct(m, objective_vector);
        if (val > best_val) {
            best_val = val;
            best_marking = m;
        }
    }

    if (best_val == INT_MIN) {
        return { {}, INT_MIN };
    }
    return { best_marking, best_val };
}
