#include "petri.h"
#include <climits> 

// BDD WRAPPER IMPLEMENTATION
BDDWrapper::BDDWrapper() {
#ifndef NO_CUDD
    manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    Cudd_AutodynEnable(manager, CUDD_REORDER_SIFT);
#endif
}

BDDWrapper::~BDDWrapper() {
#ifndef NO_CUDD
    if (manager) Cudd_Quit(manager);
#endif
}

void BDDWrapper::Init(int num_places) {
#ifndef NO_CUDD
    x_vars.resize(num_places);
    xp_vars.resize(num_places);
    for (int i = 0; i < num_places; ++i) {
        x_vars[i] = Cudd_bddNewVar(manager);
        xp_vars[i] = Cudd_bddNewVar(manager);
    }
#endif
}

DdNode* BDDWrapper::GetZero() {
#ifndef NO_CUDD
    return Cudd_ReadLogicZero(manager);
#else
    return nullptr;
#endif
}

DdNode* BDDWrapper::BuildMarkingBDD(const Marking& m) {
#ifndef NO_CUDD
    DdNode* res = Cudd_ReadOne(manager); Cudd_Ref(res);
    for (size_t i = 0; i < m.size(); ++i) {
        DdNode* var = x_vars[i];
        DdNode* lit = (m[i] == 1) ? var : Cudd_Not(var);
        DdNode* tmp = Cudd_bddAnd(manager, res, lit); Cudd_Ref(tmp);
        Cudd_RecursiveDeref(manager, res); res = tmp;
    }
    return res;
#else
    return nullptr;
#endif
}

DdNode* BDDWrapper::BuildTransitionRelation(const std::vector<std::vector<int>>& incidence,
                                            const std::vector<std::vector<int>>& input) {
#ifndef NO_CUDD
    DdNode* R_total = Cudd_ReadLogicZero(manager); Cudd_Ref(R_total);
    
    for (size_t t = 0; t < incidence[0].size(); ++t) {
        DdNode* R_t = Cudd_ReadOne(manager); Cudd_Ref(R_t);

        // Pre-condition: Input places must have tokens
        for (size_t p = 0; p < incidence.size(); ++p) {
            if (input[p][t] > 0) {
                DdNode* tmp = Cudd_bddAnd(manager, R_t, x_vars[p]); Cudd_Ref(tmp);
                Cudd_RecursiveDeref(manager, R_t); R_t = tmp;
            }
        }

        // Post-condition: Update next state variables x'
        for (size_t p = 0; p < incidence.size(); ++p) {
            DdNode* next_state;
            if (incidence[p][t] == -1) next_state = Cudd_Not(xp_vars[p]);
            else if (incidence[p][t] == 1) next_state = xp_vars[p];
            else next_state = Cudd_bddXnor(manager, xp_vars[p], x_vars[p]); // Frame condition

            DdNode* tmp = Cudd_bddAnd(manager, R_t, next_state); Cudd_Ref(tmp);
            Cudd_RecursiveDeref(manager, R_t); R_t = tmp;
        }

        DdNode* tmp_total = Cudd_bddOr(manager, R_total, R_t); Cudd_Ref(tmp_total);
        Cudd_RecursiveDeref(manager, R_total); Cudd_RecursiveDeref(manager, R_t);
        R_total = tmp_total;
    }
    return R_total;
#else
    return nullptr;
#endif
}

DdNode* BDDWrapper::SymbolicImage(DdNode* current, DdNode* relation) {
#ifndef NO_CUDD
    DdNode* and_res = Cudd_bddAnd(manager, current, relation); Cudd_Ref(and_res);
    
    // Abstract x variables
    DdNode* cube = Cudd_ReadOne(manager); Cudd_Ref(cube);
    for (auto v : x_vars) {
        DdNode* tmp = Cudd_bddAnd(manager, cube, v); Cudd_Ref(tmp);
        Cudd_RecursiveDeref(manager, cube); cube = tmp;
    }

    DdNode* exist_res = Cudd_bddExistAbstract(manager, and_res, cube); Cudd_Ref(exist_res);
    Cudd_RecursiveDeref(manager, and_res); Cudd_RecursiveDeref(manager, cube);

    // Swap x' to x for the next iteration
    DdNode* res = Cudd_bddSwapVariables(manager, exist_res, x_vars.data(), xp_vars.data(), x_vars.size());
    Cudd_Ref(res); Cudd_RecursiveDeref(manager, exist_res);
    return res;
#else
    return nullptr;
#endif
}

double BDDWrapper::CountStates(DdNode* bdd) {
#ifndef NO_CUDD
    return Cudd_CountMinterm(manager, bdd, x_vars.size());
#else
    return 0;
#endif
}

DdNode* BDDWrapper::BDD_Or(DdNode* a, DdNode* b) {
#ifndef NO_CUDD
    DdNode* r = Cudd_bddOr(manager, a, b); Cudd_Ref(r); return r;
#else
    return nullptr;
#endif
}

DdNode* BDDWrapper::BDD_Minus(DdNode* a, DdNode* b) {
#ifndef NO_CUDD
    DdNode* not_b = Cudd_Not(b);
    DdNode* r = Cudd_bddAnd(manager, a, not_b); Cudd_Ref(r); return r;
#else
    return nullptr;
#endif
}

void BDDWrapper::Ref(DdNode* n) { 
#ifndef NO_CUDD
    if(n) Cudd_Ref(n); 
#endif
}

void BDDWrapper::Deref(DdNode* n) { 
#ifndef NO_CUDD
    if(n) Cudd_RecursiveDeref(manager, n); 
#endif
}

// PETRI NET LOGIC
using namespace tinyxml2;
bool PetriNetAnalysis::CheckArcs(XMLElement* root) {
    if (!root) return true;
    bool valid = true;

    for (XMLElement* arc = root->FirstChildElement("arc"); arc; arc = arc->NextSiblingElement("arc")) {
        const char* id = arc->Attribute("id");
        const char* src = arc->Attribute("source");
        const char* tgt = arc->Attribute("target");
        std::string arcId = id ? id : "unknown";

        if (!src || !tgt) {
            std::cerr << "[ERROR] Arc '" << arcId << "' is missing source or target attributes.\n";
            valid = false;
            continue;
        }

        bool srcIsPlace = place_map.count(src);
        bool srcIsTrans = trans_map.count(src);
        bool tgtIsPlace = place_map.count(tgt);
        bool tgtIsTrans = trans_map.count(tgt);

        if (!srcIsPlace && !srcIsTrans) {
            std::cerr << "[ERROR] Arc '" << arcId << "': Source '" << src << "' does not exist.\n";
            valid = false;
        }
        if (!tgtIsPlace && !tgtIsTrans) {
            std::cerr << "[ERROR] Arc '" << arcId << "': Target '" << tgt << "' does not exist.\n";
            valid = false;
        }

        if (srcIsPlace && tgtIsPlace) {
            std::cerr << "[ERROR] Arc '" << arcId << "': Invalid connection Place -> Place (" << src << " -> " << tgt << ").\n";
            valid = false;
        }
        if (srcIsTrans && tgtIsTrans) {
            std::cerr << "[ERROR] Arc '" << arcId << "': Invalid connection Transition -> Transition (" << src << " -> " << tgt << ").\n";
            valid = false;
        }
    }

    for (XMLElement* page = root->FirstChildElement("page"); page; page = page->NextSiblingElement("page")) {
        // Gọi đệ quy với tên mới
        if (!CheckArcs(page)) valid = false;
    }
    return valid;
}

bool PetriNetAnalysis::VerifyConsistency(XMLElement* root) {
    bool passed = true;
    std::cout << "Verifying consistency...\n";

    for (const auto& pair : place_map) {
        if (trans_map.count(pair.first)) {
            std::cerr << "[ERROR] ID conflict: '" << pair.first << "' is used for both a Place and a Transition.\n";
            passed = false;
        }
    }
    if (!CheckArcs(root)) {
        passed = false;
    }

    if (passed) {
        std::cout << "Check success.\n";
    } else {
        std::cerr << "[ERROR] Could not parse file"  "\n";
        std::cerr << "Please check the PNML file.\n";
    }
    return passed;
}
void PetriNetAnalysis::CollectPlaces(XMLElement* root, int& p_idx) {
    if (!root) return;
    for (XMLElement* p = root->FirstChildElement("place"); p; p = p->NextSiblingElement("place")) {
        Place obj; 
        obj.id = p->Attribute("id"); 
        obj.index = p_idx++;
        
        XMLElement* init = p->FirstChildElement("initialMarking");
        if (init && init->FirstChildElement("text")) {
            obj.initial_marking = atoi(init->FirstChildElement("text")->GetText());
        } else {
            obj.initial_marking = 0;
        }
        
        places.push_back(obj);
        place_map[obj.id] = obj.index;
        place_ids.push_back(obj.id);
    }

    for (XMLElement* page = root->FirstChildElement("page"); page; page = page->NextSiblingElement("page")) {
        CollectPlaces(page, p_idx);
    }
}

void PetriNetAnalysis::CollectTransitions(XMLElement* root, int& t_idx) {
    if (!root) return;

    for (XMLElement* t = root->FirstChildElement("transition"); t; t = t->NextSiblingElement("transition")) {
        Transition obj; 
        obj.id = t->Attribute("id"); 
        obj.index = t_idx++;
        transitions.push_back(obj);
        trans_map[obj.id] = obj.index;
    }

    for (XMLElement* page = root->FirstChildElement("page"); page; page = page->NextSiblingElement("page")) {
        CollectTransitions(page, t_idx);
    }
}

void PetriNetAnalysis::CollectArcs(XMLElement* root) {
    if (!root) return;

    for (XMLElement* arc = root->FirstChildElement("arc"); arc; arc = arc->NextSiblingElement("arc")) {
        std::string src = arc->Attribute("source");
        std::string tgt = arc->Attribute("target");
        
        if (place_map.count(src) && trans_map.count(tgt)) {
            int p = place_map[src], t = trans_map[tgt];
            incidence_matrix[p][t] -= 1; 
            input_matrix[p][t] = 1;
        } else if (trans_map.count(src) && place_map.count(tgt)) {
            int t = trans_map[src], p = place_map[tgt];
            incidence_matrix[p][t] += 1;
        }
    }

    for (XMLElement* page = root->FirstChildElement("page"); page; page = page->NextSiblingElement("page")) {
        CollectArcs(page);
    }
}

bool PetriNetAnalysis::ParsePNML(const std::string& filename) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        std::cerr << "[ERROR] Could not open file: " << filename << "\n";
        std::cerr << "    File not found. Make sure the file exists in the same folder \n";
        return false;
    }
    
    XMLElement* root_node = doc.FirstChildElement("pnml")->FirstChildElement("net");
    if (!root_node) {
        std::cerr << "[ERROR] Invalid PNML structure.\n"; 
        return false;
    }

    places.clear(); transitions.clear(); place_map.clear(); trans_map.clear();

    int p_idx = 0;
    CollectPlaces(root_node, p_idx);

    int t_idx = 0;
    CollectTransitions(root_node, t_idx);

    if (places.empty() && transitions.empty()) {
        std::cerr << "[ERROR] No places or transitions found in PNML file!\n";
        return false;
    }

    if (!VerifyConsistency(root_node)) {
        return false;
    }
    size_t np = places.size(); 
    size_t nt = transitions.size();
    incidence_matrix.assign(np, std::vector<int>(nt, 0));
    input_matrix.assign(np, std::vector<int>(nt, 0));
    initial_marking.resize(np);
    for(auto& p : places) initial_marking[p.index] = p.initial_marking;

    CollectArcs(root_node);
    objective_vector.resize(place_ids.size(), 1);

    return true;
}

void PetriNetAnalysis::PrintInfo() const {
    std::cout << "Parsed: " << places.size() << " places, " << transitions.size() << " transitions.\n";
}

bool PetriNetAnalysis::IsEnabled(const Marking& m, int t) const {
    for(size_t p=0; p<places.size(); ++p) if(input_matrix[p][t] > m[p]) return false;
    return true;
}

Marking PetriNetAnalysis::Fire(const Marking& m, int t) const {
    Marking n = m;
    for(size_t p=0; p<places.size(); ++p) n[p] += incidence_matrix[p][t];
    return n;
}

std::set<Marking> PetriNetAnalysis::ComputeExplicit(long long& time) const {
    auto start = std::chrono::high_resolution_clock::now();
    std::set<Marking> visited; std::queue<Marking> q;
    visited.insert(initial_marking); q.push(initial_marking);
    
    while(!q.empty()){
        Marking u = q.front(); q.pop();
        for(size_t t=0; t<transitions.size(); ++t){
            if(IsEnabled(u, t)){
                Marking v = Fire(u, t);
                if(visited.find(v) == visited.end()){ visited.insert(v); q.push(v); }
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return visited;
}

DdNode* PetriNetAnalysis::ComputeSymbolic(long long& time) {
#ifndef NO_CUDD
    auto start = std::chrono::high_resolution_clock::now();
    bdd_mgr.Init(places.size());
    
    DdNode* M_reach = bdd_mgr.BuildMarkingBDD(initial_marking); bdd_mgr.Ref(M_reach);
    DdNode* M_new = M_reach; bdd_mgr.Ref(M_new);
    DdNode* R = bdd_mgr.BuildTransitionRelation(incidence_matrix, input_matrix); bdd_mgr.Ref(R);
    
    // Fixpoint Iteration
    while (M_new != bdd_mgr.GetZero()) { 
        DdNode* M_next = bdd_mgr.SymbolicImage(M_new, R);
        DdNode* diff = bdd_mgr.BDD_Minus(M_next, M_reach);
        
        if (diff == bdd_mgr.GetZero()) {
             bdd_mgr.Deref(diff); bdd_mgr.Deref(M_next); break; 
        }

        bdd_mgr.Deref(M_new); M_new = diff;
        DdNode* u = bdd_mgr.BDD_Or(M_reach, M_new);
        bdd_mgr.Deref(M_reach); M_reach = u;
        bdd_mgr.Deref(M_next);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return M_reach;
#else
    std::cout << "[WARN] CUDD not found. Skipping Task 3.\n";
    time = 0;
    return nullptr;
#endif
}

DdNode* BDDWrapper::BuildDeadlockMask(const std::vector<std::vector<int>>& input_matrix) {
#ifndef NO_CUDD
    DdNode* all_dead = Cudd_ReadOne(manager); 
    Cudd_Ref(all_dead);
    
    size_t num_places = input_matrix.size();
    if (num_places == 0) return all_dead; 
    size_t num_trans = input_matrix[0].size();

    for (size_t t = 0; t < num_trans; ++t) {
        bool has_input = false;
        for (size_t p = 0; p < num_places; ++p) {
            if (input_matrix[p][t] > 0) {
                has_input = true;
                break;
            }
        }
        
        if (!has_input) {
            Cudd_RecursiveDeref(manager, all_dead);
            return Cudd_ReadLogicZero(manager);
        }
        
        DdNode* t_enabled = Cudd_ReadOne(manager); 
        Cudd_Ref(t_enabled);
        
        for (size_t p = 0; p < num_places; ++p) {
            if (input_matrix[p][t] > 0) {
                DdNode* tmp = Cudd_bddAnd(manager, t_enabled, x_vars[p]); 
                Cudd_Ref(tmp);
                Cudd_RecursiveDeref(manager, t_enabled);
                t_enabled = tmp;
            }
        }
        DdNode* t_disabled = Cudd_Not(t_enabled);
        DdNode* tmp_dead = Cudd_bddAnd(manager, all_dead, t_disabled); 
        Cudd_Ref(tmp_dead);
        
        Cudd_RecursiveDeref(manager, all_dead);
        Cudd_RecursiveDeref(manager, t_enabled); 
        
        all_dead = tmp_dead;
    }
    
    return all_dead;
#else
    return nullptr;
#endif
}

Marking BDDWrapper::PickOneMarking(DdNode* bdd) {
    Marking m(x_vars.size(), 0);
#ifndef NO_CUDD
    if (bdd == Cudd_ReadLogicZero(manager)) return m; 

    DdNode* current = bdd;
    Cudd_Ref(current); 

    for (size_t i = 0; i < x_vars.size(); ++i) {
        DdNode* var = x_vars[i];
        
        DdNode* try_one = Cudd_bddAnd(manager, current, var); 
        Cudd_Ref(try_one);
        
        if (try_one != Cudd_ReadLogicZero(manager)) {
            m[i] = 1;
            Cudd_RecursiveDeref(manager, current);
            current = try_one;
        } else {
            Cudd_RecursiveDeref(manager, try_one);
            m[i] = 0;
            
            DdNode* try_zero = Cudd_bddAnd(manager, current, Cudd_Not(var)); 
            Cudd_Ref(try_zero);
            
            Cudd_RecursiveDeref(manager, current);
            current = try_zero;
        }
    }
    Cudd_RecursiveDeref(manager, current);
#endif
    return m;
}

#ifdef USE_GLPK
bool PetriNetAnalysis::IsDeadlockByILP(const Marking& m) {
    glp_prob *lp = glp_create_prob();
    glp_set_prob_name(lp, "Deadlock_Verification");
    
    glp_set_obj_dir(lp, GLP_MAX); 
    
    glp_term_out(GLP_OFF);
    
    int T = transitions.size();
    if (T == 0) {
        glp_delete_prob(lp);
        return true; 
    }
    
    glp_add_cols(lp, T);
    for (int t = 0; t < T; ++t) {
        glp_set_col_kind(lp, t + 1, GLP_BV); 
        glp_set_obj_coef(lp, t + 1, 1.0);    
    }

    int constraint_count = 0;
    for (int t = 0; t < T; ++t) {
        for (size_t p = 0; p < places.size(); ++p) {
            if (input_matrix[p][t] > 0 && m[p] == 0) {
                
                constraint_count++;
                glp_add_rows(lp, 1);
                
                int ind[2] = {0, t + 1};     
                double val[2] = {0.0, 1.0};  
                glp_set_mat_row(lp, constraint_count, 1, ind, val);
                
                glp_set_row_bnds(lp, constraint_count, GLP_FX, 0.0, 0.0);
                
                break; 
            }
        }
    }
    
    glp_iocp parm;
    glp_init_iocp(&parm);
    parm.msg_lev = GLP_MSG_OFF; 
    parm.presolve = GLP_ON;     
    
    int status = glp_intopt(lp, &parm);
    
    bool is_deadlock = true; 
    
    if (status == 0) { 
        double max_enabled = glp_mip_obj_val(lp);
        
        if (max_enabled > 0.5) {
            is_deadlock = false;
        }
    }
    
    glp_delete_prob(lp);
    
    return is_deadlock;
}
#endif

PetriNetAnalysis::DeadlockResult PetriNetAnalysis::DetectDeadlock(DdNode* reachable_bdd) {
    auto start = std::chrono::high_resolution_clock::now();
    DeadlockResult result = {false, {}, 0};
    
#ifndef NO_CUDD
    std::cout << "[Task 4] Using BDD to generate deadlock candidates ...\n";
    
    DdNode* dead_mask = bdd_mgr.BuildDeadlockMask(input_matrix);
    bdd_mgr.Ref(dead_mask);
    
    DdNode* intersection = Cudd_bddAnd(bdd_mgr.manager, reachable_bdd, dead_mask);
    bdd_mgr.Ref(intersection);
    
    if (intersection != bdd_mgr.GetZero()) {
        Marking candidate = bdd_mgr.PickOneMarking(intersection);
        
#ifdef USE_GLPK
        std::cout << "[Task 4] Using GLPK ILP to verify deadlock...\n";
        
        if (IsDeadlockByILP(candidate)) {
            result.found = true;
            result.deadlock_marking = candidate;
            std::cout << "[Task 4] Deadlock CONFIRMED by ILP.\n";
        } else {
            
            std::cout << "[Task 4]  Candidate rejected by ILP (False positive).\n";
        }
#else
        std::cout << "[WARN] ILP not available. Accepting BDD result directly.\n";
        result.found = true;
        result.deadlock_marking = candidate;
#endif

    } else {
        std::cout << "[Task 4] Intersection is empty. No deadlock reachable.\n";
    }
    
    bdd_mgr.Deref(dead_mask);
    bdd_mgr.Deref(intersection);
#else
    std::cout << "[WARN] CUDD not available for Task 4.\n";
#endif
    
    auto end = std::chrono::high_resolution_clock::now();
    result.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    return result;
}

long long PetriNetAnalysis::GetTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()).count();
    return ms;
}
