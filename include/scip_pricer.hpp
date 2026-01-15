#ifndef SCIP_PRICER_HPP
#define SCIP_PRICER_HPP

#include "scip_solver.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <map>

/* ===================================================================
   ARCHITECTURE OVERVIEW:
   
   This wrapper bridges SCIP's C API with modern C++. It uses:
   1. Singleton Pattern: ScipPricerManager - global registry
   2. Template Method Pattern: BasePricer - defines algorithm skeleton
   3. Factory Pattern: SimpleCuttingStockSolver - creates components
   4. Bridge Pattern: Connects C++ objects to SCIP C callbacks
   
   EXTENSION GUIDE:
   - New pricer type? Inherit from BasePricer
   - New problem type? Create new solver class like SimpleCuttingStockSolver
   - Need additional callbacks? Add virtual methods to BasePricer
   =================================================================== */

/* -------------------------------------------------------------------
   SCIP PRICER MANAGER (Singleton Pattern)
   
   Manages the mapping between SCIP pricer pointers and C++ pricer objects.
   Needed because SCIP callbacks can't directly access C++ objects.

   WHY SINGLETON: SCIP callbacks are C functions, cannot capture C++ objects.
   This manager maps SCIP's C pricer pointers to our C++ pricer objects.
   
   HOW TO USE: Always use getInstance() to access. Automatically cleans up.
   
   EXTENSION: If you need multiple managers (rare), change to non-singleton.
------------------------------------------------------------------- */
class ScipPricerManager {
private:
    static ScipPricerManager* instance_; // Static instance for singleton
    std::map<SCIP_PRICER*, class BasePricer*> pricerMap_; // Bridge mapping. The key is the SCIP pricer pointer, the value is the C++ pricer object.
    
    ScipPricerManager() = default; // Private constructor for singleton.
    
public:
    static ScipPricerManager& getInstance(); // retrieve singleton instance (only one manager exists)
    // Registration methods - called when pricers are created/destroyed
    void registerPricer(SCIP_PRICER* scipPricer, class BasePricer* pricer); // We can store the mapping between the SCIP pricer pointer and the C++ pricer object.
    void unregisterPricer(SCIP_PRICER* scipPricer);
    class BasePricer* getPricer(SCIP_PRICER* scipPricer); // Lookup method - used by static callbacks to find C++ object
};

/* -------------------------------------------------------------------
   BASE PRICER ABSTRACT CLASS - Template
   
   This is what users implement. The callbacks bridge to SCIP's C API.
   This is the ABSTRACT BASE CLASS that defines the interface for all pricers.
   Derived classes implement the actual pricing logic.
   TO CREATE NEW PRICER:
   1. Inherit from BasePricer
   2. Implement all pure virtual methods
   3. Add any additional data/methods needed
------------------------------------------------------------------- */
class BasePricer {
protected:
    SCIP* scip_; // Non-owning pointer to SCIP environment. Not owning means we don't manage its lifetime.
    SCIP_PRICER* scipPricer_; // SCIP's C pricer object (non-owning)
    std::string name_; // Pricer name for debugging and identification
    
public:
    BasePricer(SCIP* scip, const std::string& name); // Constructor
    virtual ~BasePricer();
    
    /* ===============================================================
       PURE VIRTUAL METHODS - MUST BE IMPLEMENTED BY DERIVED CLASSES
       
       These correspond to SCIP's pricer callbacks. Each method handles
       a specific phase of the pricing algorithm.
       =============================================================== */
    virtual SCIP_RETCODE scipRedcost(SCIP* scip, SCIP_PRICER* pricer,
                                 double* lowerbound,
                                 unsigned int* naddedvars,
                                 SCIP_Result* result) = 0; // Called when SCIP solves pricing problem for feasible LP
    virtual SCIP_RETCODE scipInit(SCIP* scip, SCIP_PRICER* pricer) = 0; // Called when pricer is initialized (transform constraints)
    virtual SCIP_RETCODE scipFree(SCIP* scip, SCIP_PRICER* pricer) = 0; // Called when pricer is freed (cleanup memory)
    
    /* ===============================================================
       ACCESSORS - public getters for pricer information
       =============================================================== */
    SCIP_PRICER* getScipPricer() const { return scipPricer_; } // Get SCIP pricer pointer
    const std::string& getName() const { return name_; }
    SCIP* getScip() const { return scip_; }
    
    /* ===============================================================
       STATIC CALLBACK WRAPPERS - BRIDGE BETWEEN C AND C++
       
       These static methods are registered with SCIP's C API.
       When SCIP calls them, they:
       1. Look up the corresponding C++ object using ScipPricerManager
       2. Call the virtual method on that object
       3. Return the result to SCIP

       In other words, they adapt the C function signature to C++ member functions.
       
       WHY STATIC: C functions cannot be non-static member functions.
       =============================================================== */
    static SCIP_RETCODE scipFreeCallback(SCIP* scip, SCIP_PRICER* pricer);
    static SCIP_RETCODE scipInitCallback(SCIP* scip, SCIP_PRICER* pricer);
    static SCIP_RETCODE scipRedcostCallback(SCIP* scip, SCIP_PRICER* pricer,
                                       double* lowerbound, 
                                       unsigned int* naddedvars,
                                       SCIP_Result* result);
    static SCIP_RETCODE scipFarkasCallback(SCIP* scip, SCIP_PRICER* pricer,
                                      SCIP_Result* result);
};

/* -------------------------------------------------------------------
   CUTTING STOCK PRICER (Concrete Implementation)
   
   This is a SPECIFIC IMPLEMENTATION of BasePricer for cutting stock.
   It solves the knapsack subproblem via dynamic programming.
   
   TO CREATE ANOTHER PRICER (e.g., for vehicle routing):
   1. Copy this class structure
   2. Change the pricing logic in scipRedcost()
   3. Update the internal data structures
   4. Modify solveKnapsack() or replace with your subproblem solver
------------------------------------------------------------------- */
class CuttingStockPricer : public BasePricer { // Inherits from BasePricer
private:
    // Internal data structure - stores problem-specific data
    struct PricerData {
        std::vector<SCIP_CONS*> demandConss;
        std::vector<int> widths;
        int rollWidth;
        // Constructor
        PricerData(const std::vector<int>& w, int rw) 
            : widths(w), rollWidth(rw) {}
    };
    
    PricerData* data_; // Owned pointer to problem data. Owned means we manage its lifetime.
    
public:
    // Constructor
    CuttingStockPricer(ScipSolver& solver,
                      const std::vector<int>& widths,
                      int rollWidth);
    // Destructor
    ~CuttingStockPricer();
    
    // Implement virtual methods, as required by BasePricer
    SCIP_RETCODE scipRedcost(SCIP* scip, SCIP_PRICER* pricer,
                        double* lowerbound,
                        unsigned int* naddedvars,
                        SCIP_Result* result) override;
    SCIP_RETCODE scipInit(SCIP* scip, SCIP_PRICER* pricer) override;
    SCIP_RETCODE scipFree(SCIP* scip, SCIP_PRICER* pricer) override;
    
    // Helper methods, in this case for solving the knapsack subproblem, by setting constraints.
    void setDemandConstraints(const std::vector<SCIP_CONS*>& cons);
    
private:
    // Internal helper methods - business logic separated from SCIP boilerplate
    std::vector<double> getDuals(SCIP* scip) const;
    double solveKnapsack(const std::vector<double>& duals,
                        std::vector<int>& pattern) const;
};

/* -------------------------------------------------------------------
   SIMPLE CUTTING STOCK SOLVER (Factory Pattern)
   
   This is a HIGH-LEVEL WRAPPER that provides a simple API for users.
   It encapsulates the entire problem setup and solving process.
   
   TO CREATE ANOTHER SOLVER TYPE:
   1. Copy this class structure
   2. Modify constructor to take your problem parameters
   3. Update solve() method with your problem setup
   4. Add any additional solution query methods
------------------------------------------------------------------- */
class SimpleCuttingStockSolver {
private:
    // Composition over inheritance - solver owns its components
    std::unique_ptr<ScipSolver> solver_;
    std::unique_ptr<CuttingStockPricer> pricer_;
    std::vector<int> widths_;
    std::vector<int> demands_;
    int rollWidth_;
    
public:
    SimpleCuttingStockSolver(const std::vector<int>& widths,
                            const std::vector<int>& demands,
                            int rollWidth);
    
    double solve();
    
    void printSolution() const;
};

#endif // SCIP_PRICER_HPP