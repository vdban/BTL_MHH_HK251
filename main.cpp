#include "petri.h"
#include "optimization.h"

int main(int argc, char* argv[]) {
    // Mặc định chạy file test.pnml nếu không nhập tên file
    std::string filename = "test.pnml";
    
    // Nếu người dùng nhập tên file (ví dụ: ./app test_parallel.pnml)
    if (argc > 1) {
        filename = argv[1];
    }

    std::cout << "========================================\n";
    std::cout << "Running Testcase: " << filename << "\n";
    std::cout << "========================================\n";

    PetriNetAnalysis app;

    // --- TASK 1: Parsing ---
    if (!app.ParsePNML(filename)) {
        std::cerr << "[ERROR] Could not parse file: " << filename << "\n";
        std::cerr << "Make sure the file exists in the same folder.\n";
        return 1;
    }
    app.PrintInfo();

    // --- TASK 2: Explicit Reachability ---
    long long t2;
    auto res2 = app.ComputeExplicit(t2);
    std::cout << "[Task 2] Explicit Reachability: " << res2.size() 
              << " markings (" << t2 << " ms).\n";

    // --- TASK 3: Symbolic Reachability ---
    long long t3;
    DdNode* res3 = app.ComputeSymbolic(t3);
#ifndef NO_CUDD
    double num_states = app.bdd_mgr.CountStates(res3);
    int bdd_nodes = Cudd_DagSize(res3);
    std::cout << "[Task 3] Symbolic Reachability: " << num_states << " markings (" << t3 << " ms).\n";
    std::cout << "         Memory Complexity: " << bdd_nodes << " BDD nodes.\n";
#else
    std::cout << "[Task 3] Symbolic Reachability: Disabled (No CUDD).\n";
#endif
    // --- TASK 4: Deadlock Detection --- 
#ifndef NO_CUDD
    auto deadlock_res = app.DetectDeadlock(res3);
    
    if (deadlock_res.found) {
        std::cout << "[Task 4] Deadlock FOUND (" << deadlock_res.time_ms << " ms).\n";
        std::cout << "         Example Deadlock Marking: [ ";
        for (int val : deadlock_res.deadlock_marking) {
            std::cout << val << " ";
        }
        std::cout << "]\n";
    } else {
        std::cout << "[Task 4] No deadlock detected (" << deadlock_res.time_ms << " ms).\n";
    }
#else
    std::cout << "[Task 4] Deadlock Detection: Disabled (No CUDD).\n";
#endif

    //TASK 5
#ifndef NO_CUDD
    auto opt = MarkingOptimizerBB::maxReachableMarking(app.place_ids, res3, app.objective_vector, app);
#else
    auto opt = MarkingOptimizerBB::maxReachableMarking(app.place_ids, nullptr, app.objective_vector, app);
#endif

    if (opt.first.empty()) {
        std::cout << "[Task 5] Optimization: No reachable marking found.\n";
    } else {
        std::cout << "[Task 5] Optimal value = " << opt.second << "\n";
        std::cout << "         Marking = [ ";
        for (int v : opt.first) std::cout << v << " ";
        std::cout << "]\n";
    }

}