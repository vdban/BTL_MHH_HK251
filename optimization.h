#pragma once
#include <vector>
#include <string>
#include <utility>
#include "petri.h"

class MarkingOptimizerBB {
public:
    //(marking tối ưu, giá trị max). Nếu không có, trả về ([], INT_MIN)
    static std::pair<std::vector<int>, int> maxReachableMarking(
        const std::vector<std::string>& place_ids,
        DdNode* reachable_bdd,                    
        const std::vector<int>& objective_vector,
        PetriNetAnalysis& app   
    );
};
