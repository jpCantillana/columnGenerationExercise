#include "../include/scip_pricer.hpp"
#include <iostream>

/* -------------------------------------------------------------------
   SCIP PRICER BASE CLASS
------------------------------------------------------------------- */

ScipPricer::ScipPricer(ScipSolver& solver, const std::string& name)
    : scip_(solver.get()), name_(name), pricer_(nullptr) {
    
    // We'll create the pricer in derived classes
}

ScipPricer::~ScipPricer() {
    // SCIP frees the pricer automatically
}

/* -------------------------------------------------------------------
   CUTTING STOCK PRICER IMPLEMENTATION
------------------------------------------------------------------- */

// Static callback wrappers
SCIP_DECL_PRICERFREE(CuttingStockPricer::scip_free_cb) {
    CuttingStockPricer* pricerObj = static_cast<CuttingStockPricer*>(
        SCIPpricerGetData(pricer));
    return pricerObj->scip_free(scip, pricer);
}

SCIP_DECL_PRICERINIT(CuttingStockPricer::scip_init_cb) {
    CuttingStockPricer* pricerObj = static_cast<CuttingStockPricer*>(
        SCIPpricerGetData(pricer));
    return pricerObj->scip_init(scip, pricer);
}

SCIP_DECL_PRICERREDCOST(CuttingStockPricer::scip_redcost_cb) {
    CuttingStockPricer* pricerObj = static_cast<CuttingStockPricer*>(
        SCIPpricerGetData(pricer));
    return pricerObj->scip_redcost(scip, pricer, result);
}

SCIP_DECL_PRICERFARKAS(CuttingStockPricer::scip_farkas_cb) {
    *result = SCIP_SUCCESS;
    return SCIP_OKAY;
}

// Constructor
CuttingStockPricer::CuttingStockPricer(ScipSolver& solver,
                                      const std::vector<int>& widths,
                                      int rollWidth)
    : ScipPricer(solver, "dp_pricer"), data_(nullptr) {
    
    // Create pricer data
    data_ = new PricerData(widths, rollWidth);
    
    // Include pricer in SCIP (matches original code)
    SCIP_CALL_EXCEPT(SCIPincludePricer(
        scip_, "dp_pricer", "DP Knapsack Pricer", 
        0,                    // priority
        TRUE,                 // delay pricing
        nullptr,              // pricerCopy
        scip_free_cb,         // pricerFree
        scip_init_cb,         // pricerInit
        nullptr,              // pricerExit
        nullptr,              // pricerInitsol
        nullptr,              // pricerExitsol
        scip_redcost_cb,      // pricerRedcost
        scip_farkas_cb,       // pricerFarkas
        (SCIP_PRICERDATA*)data_  // pricer data
    ));
    
    // Find and store the pricer
    pricer_ = SCIPfindPricer(scip_, "dp_pricer");
    if (pricer_ == nullptr) {
        throw std::runtime_error("Failed to find pricer after inclusion");
    }
    
    // Activate pricer
    SCIP_CALL_EXCEPT(SCIPactivatePricer(scip_, pricer_));
}

CuttingStockPricer::~CuttingStockPricer() {
    // Data is freed in scip_free callback
}

void CuttingStockPricer::setDemandConstraints(const std::vector<SCIP_CONS*>& cons) {
    data_->demandConss = cons;
}

void CuttingStockPricer::addInitialVar(SCIP_VAR* var) {
    data_->initialVars.push_back(var);
}

SCIP_RETCODE CuttingStockPricer::scip_free(SCIP* scip, SCIP_PRICER* pricer) {
    if (data_ != nullptr) {
        delete data_;
        data_ = nullptr;
        SCIPpricerSetData(pricer, nullptr);
    }
    return SCIP_OKAY;
}

SCIP_RETCODE CuttingStockPricer::scip_init(SCIP* scip, SCIP_PRICER* pricer) {
    // Transform constraints (like original code)
    for (size_t i = 0; i < data_->demandConss.size(); ++i) {
        SCIP_CONS* trans = nullptr;
        SCIP_CALL(SCIPgetTransformedCons(scip, data_->demandConss[i], &trans));
        data_->demandConss[i] = trans;
    }
    return SCIP_OKAY;
}

std::vector<double> CuttingStockPricer::getDuals(SCIP* scip) const {
    int n = data_->widths.size();
    std::vector<double> duals(n);
    for (int i = 0; i < n; ++i) {
        duals[i] = SCIPgetDualsolLinear(scip, data_->demandConss[i]);
    }
    return duals;
}

double CuttingStockPricer::solveKnapsack(const std::vector<double>& duals,
                                        std::vector<int>& pattern) const {
    int n = data_->widths.size();
    int W = data_->rollWidth;
    
    // DP for unbounded knapsack
    std::vector<double> dp(W + 1, 0.0);
    std::vector<int> keepTrack(W + 1, -1);
    
    for (int w = 0; w <= W; ++w) {
        for (int i = 0; i < n; ++i) {
            if (data_->widths[i] <= w) {
                if (dp[w - data_->widths[i]] + duals[i] > dp[w]) {
                    dp[w] = dp[w - data_->widths[i]] + duals[i];
                    keepTrack[w] = i;
                }
            }
        }
    }
    
    double bestValue = dp[W];
    
    // Reconstruct pattern
    pattern.assign(n, 0);
    int w = W;
    while (w > 0 && keepTrack[w] != -1) {
        int itemIdx = keepTrack[w];
        pattern[itemIdx]++;
        w -= data_->widths[itemIdx];
    }
    
    return bestValue;
}

SCIP_RETCODE CuttingStockPricer::scip_redcost(SCIP* scip, SCIP_PRICER* pricer,
                                            SCIP_Result* result) {
    // Crucial: Always set result to SUCCESS
    *result = SCIP_SUCCESS;
    
    // Get duals
    auto duals = getDuals(scip);
    
    // Solve knapsack
    std::vector<int> pattern;
    double bestValue = solveKnapsack(duals, pattern);
    
    // If reduced cost is negative (value > 1.0), add pattern
    if (bestValue > 1.0 + 1e-6) {
        int n = data_->widths.size();
        
        // Create and add the priced variable (matches original exactly)
        SCIP_VAR* var = nullptr;
        std::string name = "pat_" + std::to_string(SCIPgetNLPCols(scip));
        SCIP_CALL(SCIPcreateVarBasic(scip, &var, name.c_str(), 
            0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_INTEGER));
        
        SCIP_CALL(SCIPvarSetInitial(var, TRUE));
        SCIP_CALL(SCIPvarSetRemovable(var, TRUE));
        SCIP_CALL(SCIPaddPricedVar(scip, var, 1.0));
        
        for (int i = 0; i < n; ++i) {
            if (pattern[i] > 0) {
                SCIP_CALL(SCIPaddCoefLinear(scip, data_->demandConss[i], var, 
                    (SCIP_Real)pattern[i]));
            }
        }
        
        std::cout << "DP found pattern with value: " << bestValue << "\n";
        SCIP_CALL(SCIPreleaseVar(scip, &var));
    }
    
    return SCIP_OKAY;
}

/* -------------------------------------------------------------------
   SIMPLE CUTTING STOCK SOLVER
------------------------------------------------------------------- */

SimpleCuttingStockSolver::SimpleCuttingStockSolver(
    const std::vector<int>& widths,
    const std::vector<int>& demands,
    int rollWidth)
    : widths_(widths), demands_(demands), rollWidth_(rollWidth) {
    
    // Create solver
    solver_ = std::make_unique<ScipSolver>("cutting_stock_dp");
    solver_->setMinimize();
    
    // Create pricer
    pricer_ = std::make_unique<CuttingStockPricer>(*solver_, widths, rollWidth);
}

double SimpleCuttingStockSolver::solve() {
    SCIP* scip = solver_->get();
    
    // 1. Create constraints (matches original)
    std::vector<SCIP_CONS*> conss;
    for (size_t i = 0; i < widths_.size(); ++i) {
        SCIP_CONS* cons;
        SCIP_CALL_EXCEPT(SCIPcreateConsBasicLinear(
            scip, &cons, "d", 
            0, nullptr, nullptr, 
            (double)demands_[i], 
            SCIPinfinity(scip)));
        
        SCIP_CALL_EXCEPT(SCIPsetConsModifiable(scip, cons, TRUE));
        SCIP_CALL_EXCEPT(SCIPaddCons(scip, cons));
        conss.push_back(cons);
    }
    
    pricer_->setDemandConstraints(conss);
    
    // 2. Initial columns (one roll per item)
    for (size_t i = 0; i < widths_.size(); ++i) {
        SCIP_VAR* var;
        SCIP_CALL_EXCEPT(SCIPcreateVarBasic(
            scip, &var, "init", 
            0.0, SCIPinfinity(scip), 
            1.0, SCIP_VARTYPE_CONTINUOUS));
        
        SCIP_CALL_EXCEPT(SCIPaddVar(scip, var));
        SCIP_CALL_EXCEPT(SCIPaddCoefLinear(scip, conss[i], var, 1.0));
        
        // Track initial variable
        pricer_->addInitialVar(var);
        
        SCIP_CALL_EXCEPT(SCIPreleaseVar(scip, &var));
    }
    
    // 3. Solve (SCIP will automatically call pricer)
    solver_->solve();
    
    // 4. Cleanup constraints
    for (auto cons : conss) {
        SCIP_CALL_EXCEPT(SCIPreleaseCons(scip, &cons));
    }
    
    return solver_->getObjectiveValue();
}

void SimpleCuttingStockSolver::printSolution() const {
    std::cout << "Solution: " << solver_->getObjectiveValue() << " rolls" << std::endl;
}