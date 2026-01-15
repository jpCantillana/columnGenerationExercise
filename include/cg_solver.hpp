#ifndef CG_SOLVER
#define CG_SOLVER

#include "scip_solver.hpp"
#include "scip_constraint.hpp"
#include "scip_variable.hpp"
#include <vector>
#include <memory>
#include <functional>

/* -------------------------------------------------------------------
   COLUMN GENERATION PRICER INTERFACE (Abstract Base Class)
   
   This provides a clean interface for implementing different pricers.
   Each pricer must implement the `solvePricingProblem` method which:
   1. Takes dual values as input
   2. Returns a pattern (vector of counts for each item)
   3. Returns the reduced cost of that pattern
   
   The pattern with negative reduced cost (for minimization) will be
   added to the master problem.
------------------------------------------------------------------- */
class BasePricer {
public:
    virtual ~BasePricer() = default;
    
    /**
     * Solve the pricing problem given dual values
     * @param duals Dual values from master problem constraints
     * @param pattern Output: vector of item counts in the pattern
     * @return The reduced cost of the pattern (patternValue - 1.0 for cutting stock)
     */
    virtual double solvePricingProblem(const std::vector<double>& duals, 
                                       std::vector<int>& pattern) = 0;
    
    /**
     * Get problem dimensions
     * @return Number of items/types
     */
    virtual size_t getNumItems() const = 0;
    
    /**
     * Get pattern width/weight limit
     * @return Maximum width/weight capacity
     */
    virtual double getCapacity() const = 0;
};

/* -------------------------------------------------------------------
   KNAPSACK PRICER (Concrete Implementation)
   
   Implements the knapsack pricing problem for cutting stock.
   This is a 0-1 knapsack solved via dynamic programming.
------------------------------------------------------------------- */
class KnapsackPricer : public BasePricer {
private:
    std::vector<int> widths_;    // Item widths/sizes
    int capacity_;               // Roll/knapsack capacity
    
public:
    KnapsackPricer(const std::vector<int>& widths, int capacity)
        : widths_(widths), capacity_(capacity) {}
    
    double solvePricingProblem(const std::vector<double>& duals, 
                               std::vector<int>& pattern) override;
    
    size_t getNumItems() const override { return widths_.size(); }
    double getCapacity() const override { return capacity_; }
};

/* -------------------------------------------------------------------
   CUTTING STOCK SOLVER (Main Wrapper Class)
   
   This class wraps the entire column generation process:
   1. Sets up the master problem
   2. Manages pricers
   3. Implements the column generation loop
   4. Provides a clean API for users
   
   Key concepts:
   - Master Problem: LP with covering constraints
   - Pricing Problem: Knapsack subproblem generating new columns
   - Column Generation: Iterative process between master and subproblem
------------------------------------------------------------------- */
class CuttingStockSolver {
private:
    // SCIP solver instance
    std::unique_ptr<ScipSolver> solver_;
    
    // Pricer instance
    std::unique_ptr<BasePricer> pricer_;
    
    // Problem data
    std::vector<int> demands_;
    std::vector<std::unique_ptr<ScipVariable>> variables_;
    std::vector<std::unique_ptr<ScipConstraint>> constraints_;
    
    // Algorithm parameters
    double pricing_tol_ = 1e-6;
    int max_iterations_ = 100;
    
    // Helper methods
    void setupMasterProblem();
    void addInitialColumns();
    bool generateColumns();
    void solveIntegerRestriction();
    
public:
    /**
     * Constructor for cutting stock problem
     * @param widths    Item widths/sizes
     * @param demands   Item demands
     * @param capacity  Roll capacity
     */
    CuttingStockSolver(const std::vector<int>& widths,
                       const std::vector<int>& demands,
                       int capacity);
    
    /**
     * Solve the cutting stock problem using column generation
     * @return Optimal objective value
     */
    double solve();
    
    /**
     * Get solution information
     * @return Vector of pattern usages in the solution
     */
    std::vector<double> getSolution() const;
    
    /**
     * Set algorithm parameters
     * @param pricing_tol   Reduced cost tolerance for stopping
     * @param max_iter      Maximum column generation iterations
     */
    void setParameters(double pricing_tol = 1e-6, int max_iter = 100) {
        pricing_tol_ = pricing_tol;
        max_iterations_ = max_iter;
    }
    
    /**
     * Print solution details
     */
    void printSolution() const;
};

#endif // CG_SOLVER