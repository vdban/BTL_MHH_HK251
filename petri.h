#pragma once
#include "main.h"

#ifdef USE_GLPK
    #include <glpk.h>
#endif

class BDDWrapper {
private:
    
    std::vector<DdNode*> x_vars;    // Current state variables
    std::vector<DdNode*> xp_vars;   // Next state variables

public:
    BDDWrapper();
    ~BDDWrapper();
    DdManager* manager = nullptr;
    void Init(int num_places);
    DdNode* BuildMarkingBDD(const Marking& m);
    
    // Constructs the global transition relation R(x, x')
    DdNode* BuildTransitionRelation(const std::vector<std::vector<int>>& incidence,
                                    const std::vector<std::vector<int>>& input_matrix);

    // Computes Image(S) = Exists_x ( S(x) AND R(x, x') )
    DdNode* SymbolicImage(DdNode* current_bdd, DdNode* relation_bdd);
    DdNode* BuildDeadlockMask(const std::vector<std::vector<int>>& input_matrix);
    Marking PickOneMarking(DdNode* bdd);
    double CountStates(DdNode* bdd);
    DdNode* GetZero();
    DdNode* BDD_Or(DdNode* a, DdNode* b);
    DdNode* BDD_Minus(DdNode* a, DdNode* b);
    
    void Ref(DdNode* n);
    void Deref(DdNode* n);
};

struct Place {
    std::string id, name;
    int initial_marking;
    int index;
};

struct Transition {
    std::string id, name;
    int index;
};

class PetriNetAnalysis {
public:
    std::vector<Place> places;
    std::vector<Transition> transitions;
    
    std::vector<std::vector<int>> incidence_matrix; 
    std::vector<std::vector<int>> input_matrix;     
    
    Marking initial_marking;
    BDDWrapper bdd_mgr;

    // Task 1
    bool ParsePNML(const std::string& filename);
    void PrintInfo() const;
    // Task 2
    bool IsEnabled(const Marking& m, int t_idx) const;
    Marking Fire(const Marking& m, int t_idx) const;
    std::set<Marking> ComputeExplicit(long long& time_ms) const;

    // Task 3
    DdNode* ComputeSymbolic(long long& time_ms);
    //Task 4
    struct DeadlockResult {
        bool found;
        Marking deadlock_marking;
        long long time_ms;
    };
    DeadlockResult DetectDeadlock(DdNode* reachable_bdd);
#ifdef USE_GLPK
    bool IsDeadlockByILP(const Marking& m);
#endif

    //Task 5
    std::vector<std::string> place_ids;
    std::vector<int> objective_vector;
    long long GetTimeMs() const;


private:
    std::map<std::string, int> place_map;
    std::map<std::string, int> trans_map;
    void CollectPlaces(tinyxml2::XMLElement* root, int& p_idx);
    void CollectTransitions(tinyxml2::XMLElement* root, int& t_idx);
    void CollectArcs(tinyxml2::XMLElement* root);
    bool VerifyConsistency(tinyxml2::XMLElement* root); 
    bool CheckArcs(tinyxml2::XMLElement* root);

};