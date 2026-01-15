#include "../include/cg_solver.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>

/* -------------------------------------------------------------------
   KNAPSACK PRICER IMPLEMENTATION
   
   Solves: max ∑ dual_i * x_i  s.t. ∑ width_i * x_i ≤ capacity, x_i ∈ {0,1}
   
   Uses dynamic programming (unbounded knapsack). For 0-1 knapsack,
   we would use different DP recurrence.
------------------------------------------------------------------- */
double KnapsackPricer::solvePricingProblem(const std::vector<double>& duals, 
                                           std::vector<int>& pattern) {
    int n = widths_.size();
    int W = capacity_;
    
    // Initialize DP arrays for unbounded knapsack
    std::vector<double> dp(W + 1, 0.0);
    std::vector<int> item(W + 1, -1);
    
    // Unbounded knapsack DP
    // We can use items multiple times (pattern can have multiple of same item)
    for (int w = 1; w <= W; ++w) {
        for (int i = 0; i < n; ++i) {
            if (widths_[i] <= w) {
                double newValue = dp[w - widths_[i]] + duals[i];
                if (newValue > dp[w]) {
                    dp[w] = newValue;
                    item[w] = i;
                }
            }
        }
    }
    
    // Find best value (could be at any width ≤ W)
    double bestValue = 0.0;
    int bestWidth = 0;
    for (int w = 1; w <= W; ++w) {
        if (dp[w] > bestValue) {
            bestValue = dp[w];
            bestWidth = w;
        }
    }
    
    // Reconstruct pattern
    pattern.assign(n, 0);
    int w = bestWidth;
    while (w > 0 && item[w] != -1) {
        int i = item[w];
        pattern[i]++;
        w -= widths_[i];
    }
    
    // Reduced cost = 1 - bestValue (for minimization, we want negative reduced cost)
    // In cutting stock: column cost = 1, so reduced cost = 1 - sum(dual_i * pattern_i)
    return 1.0 - bestValue;
}

/* -------------------------------------------------------------------
   CUTTING STOCK SOLVER IMPLEMENTATION
------------------------------------------------------------------- */

CuttingStockSolver::CuttingStockSolver(const std::vector<int>& widths,
                                       const std::vector<int>& demands,
                                       int capacity)
    : demands_(demands) {
    
    // Validate input
    if (widths.size() != demands.size()) {
        throw std::runtime_error("Widths and demands must have same size");
    }
    
    // Create pricer (factory pattern - easy to switch pricers!)
    pricer_ = std::make_unique<KnapsackPricer>(widths, capacity);
    
    // Create SCIP solver
    solver_ = std::make_unique<ScipSolver>("cutting_stock");
    solver_->setMinimize();
    
    // Setup the master problem
    setupMasterProblem();
    addInitialColumns();
}

void CuttingStockSolver::setupMasterProblem() {
    int n = pricer_->getNumItems();
    
    // Create demand constraints: sum(pattern_i * x_j) >= demand_i
    for (int i = 0; i < n; ++i) {
        std::vector<ScipVariable*> vars;  // Will add variables later
        std::vector<double> coeffs;
        
        // Create constraint with lower bound = demand
        auto cons = std::make_unique<ScipConstraint>(
            solver_->get(),
            "demand_" + std::to_string(i),
            vars, coeffs,
            demands_[i],                      // lhs = demand
            SCIPinfinity(solver_->get())      // rhs = infinity
        );
        
        constraints_.push_back(std::move(cons));
    }
}

void CuttingStockSolver::addInitialColumns() {
    int n = pricer_->getNumItems();
    
    // Add simple initial columns: one roll per item
    // This ensures the master problem is feasible
    for (int i = 0; i < n; ++i) {
        // Create variable for this pattern
        auto var = std::make_unique<ScipVariable>(
            solver_->get(),
            "init_" + std::to_string(i),
            0.0,                             // lower bound
            SCIPinfinity(solver_->get()),    // upper bound
            1.0,                             // objective coefficient (cost per roll)
            SCIP_VARTYPE_CONTINUOUS          // Continuous in LP relaxation
        );
        
        // Add to demand constraint i with coefficient 1
        constraints_[i]->addVariable(var.get(), 1.0);
        
        // Store variable
        variables_.push_back(std::move(var));
    }
}

bool CuttingStockSolver::generateColumns() {
    int n = pricer_->getNumItems();
    int iteration = 0;
    bool newColumnsAdded = false;
    
    // Column generation loop
    do {
        newColumnsAdded = false;
        iteration++;
        
        // 1. Solve current LP relaxation
        solver_->solve();
        
        // 2. Get dual values from constraints
        std::vector<double> duals(n);
        for (int i = 0; i < n; ++i) {
            duals[i] = constraints_[i]->getDualValue();
            std::cout << "Dual[" << i << "] = " << duals[i] << " ";
        }
        std::cout << std::endl;
        
        // 3. Solve pricing problem (knapsack)
        std::vector<int> pattern;
        double reducedCost = pricer_->solvePricingProblem(duals, pattern);
        
        std::cout << "Best reduced cost: " << reducedCost << std::endl;
        
        // 4. Check if column with negative reduced cost exists
        // For minimization: add column if reducedCost < -tolerance
        if (reducedCost < -pricing_tol_) {
            // Create new variable for this pattern
            std::string name = "pat_" + std::to_string(variables_.size());
            auto var = std::make_unique<ScipVariable>(
                solver_->get(),
                name,
                0.0,
                SCIPinfinity(solver_->get()),
                1.0,  // Each roll costs 1
                SCIP_VARTYPE_CONTINUOUS
            );
            
            // Add variable to constraints with pattern coefficients
            for (int i = 0; i < n; ++i) {
                if (pattern[i] > 0) {
                    constraints_[i]->addVariable(var.get(), pattern[i]);
                }
            }
            
            // Store the variable
            variables_.push_back(std::move(var));
            newColumnsAdded = true;
            
            std::cout << "Iteration " << iteration 
                      << ": Added pattern [";
            for (int i = 0; i < n; ++i) {
                if (pattern[i] > 0) std::cout << pattern[i] << "*" << i << " ";
            }
            std::cout << "] with reduced cost " 
                      << reducedCost << std::endl;
        }
        
        if (iteration >= max_iterations_) {
            std::cout << "Maximum iterations reached" << std::endl;
            break;
        }
        
    } while (newColumnsAdded);
    
    std::cout << "Column generation completed after " 
              << iteration << " iterations" << std::endl;
    std::cout << "LP objective: " << solver_->getObjectiveValue() 
              << std::endl;
    
    return !newColumnsAdded;  // True if no more columns to add
}

void CuttingStockSolver::solveIntegerRestriction() {
    // Switch variables to integer
    for (auto& var : variables_) {
        // In SCIP, we need to change variable type
        // This is simplified - actual implementation requires SCIPchgVarType
        // For now, we'll keep as continuous for demonstration
    }
    
    // Solve the integer problem
    solver_->solve();
}

double CuttingStockSolver::solve() {
    // Phase 1: Solve LP relaxation via column generation
    bool converged = generateColumns();
    
    if (!converged) {
        std::cout << "Warning: Column generation did not fully converge" 
                  << std::endl;
    }
    
    // Phase 2: Solve integer restriction (optional)
    // solveIntegerRestriction();
    
    return solver_->getObjectiveValue();
}

std::vector<double> CuttingStockSolver::getSolution() const {
    std::vector<double> solution;
    solution.reserve(variables_.size());
    
    for (const auto& var : variables_) {
        try {
            solution.push_back(var->getSolutionValue());
        } catch (const std::runtime_error& e) {
            solution.push_back(0.0);
        }
    }
    
    return solution;
}

void CuttingStockSolver::printSolution() const {
    auto solution = getSolution();
    double obj = solver_->getObjectiveValue();
    
    std::cout << "\n=== Cutting Stock Solution ===" << std::endl;
    std::cout << "Total rolls used: " << obj << std::endl;
    std::cout << "Number of patterns: " << variables_.size() << std::endl;
    
    std::cout << "\nPattern details:" << std::endl;
    for (size_t j = 0; j < variables_.size(); ++j) {
        double usage = solution[j];
        if (usage > 1e-6) {
            std::cout << "Pattern " << j << ": " 
                      << std::fixed << std::setprecision(2) << usage 
                      << " rolls used" << std::endl;
        }
    }
    
    // Check demand satisfaction
    int n = pricer_->getNumItems();
    std::cout << "\nDemand satisfaction:" << std::endl;
    for (int i = 0; i < n; ++i) {
        double total = 0.0;
        // We would need to track pattern compositions here
        // For simplicity, just show the constraint duals
        std::cout << "Item " << i << ": dual = " 
                  << constraints_[i]->getDualValue() << std::endl;
    }
}