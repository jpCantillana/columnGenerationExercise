#ifndef SCIP_PRICER_HPP
#define SCIP_PRICER_HPP

#include "scip_solver.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <map>

/* -------------------------------------------------------------------
   SCIP PRICER MANAGER (Singleton Pattern)
   
   Manages the mapping between SCIP pricer pointers and C++ pricer objects.
   Needed because SCIP callbacks can't directly access C++ objects.
------------------------------------------------------------------- */
class ScipPricerManager {
private:
    static ScipPricerManager* instance_;
    std::map<SCIP_PRICER*, class BasePricer*> pricerMap_;
    
    ScipPricerManager() = default;
    
public:
    static ScipPricerManager& getInstance();
    
    void registerPricer(SCIP_PRICER* scipPricer, class BasePricer* pricer);
    void unregisterPricer(SCIP_PRICER* scipPricer);
    class BasePricer* getPricer(SCIP_PRICER* scipPricer);
};

/* -------------------------------------------------------------------
   BASE PRICER ABSTRACT CLASS
   
   This is what users implement. The callbacks bridge to SCIP's C API.
------------------------------------------------------------------- */
class BasePricer {
protected:
    SCIP* scip_;
    SCIP_PRICER* scipPricer_;
    std::string name_;
    
public:
    BasePricer(SCIP* scip, const std::string& name);
    virtual ~BasePricer();
    
    // Virtual methods to implement
    virtual SCIP_RETCODE scipRedcost(SCIP* scip, SCIP_PRICER* pricer,
                                 double* lowerbound,
                                 unsigned int* naddedvars,
                                 SCIP_Result* result) = 0;
    virtual SCIP_RETCODE scipInit(SCIP* scip, SCIP_PRICER* pricer) = 0;
    virtual SCIP_RETCODE scipFree(SCIP* scip, SCIP_PRICER* pricer) = 0;
    
    // Accessors
    SCIP_PRICER* getScipPricer() const { return scipPricer_; }
    const std::string& getName() const { return name_; }
    SCIP* getScip() const { return scip_; }
    
    // Static callback wrappers (these call the virtual methods)
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
------------------------------------------------------------------- */
class CuttingStockPricer : public BasePricer {
private:
    struct PricerData {
        std::vector<SCIP_CONS*> demandConss;
        std::vector<int> widths;
        int rollWidth;
        
        PricerData(const std::vector<int>& w, int rw) 
            : widths(w), rollWidth(rw) {}
    };
    
    PricerData* data_;
    
public:
    CuttingStockPricer(ScipSolver& solver,
                      const std::vector<int>& widths,
                      int rollWidth);
    
    ~CuttingStockPricer();
    
    // Implement virtual methods
    SCIP_RETCODE scipRedcost(SCIP* scip, SCIP_PRICER* pricer,
                        double* lowerbound,
                        unsigned int* naddedvars,
                        SCIP_Result* result) override;
    SCIP_RETCODE scipInit(SCIP* scip, SCIP_PRICER* pricer) override;
    SCIP_RETCODE scipFree(SCIP* scip, SCIP_PRICER* pricer) override;
    
    // Helper methods
    void setDemandConstraints(const std::vector<SCIP_CONS*>& cons);
    
private:
    std::vector<double> getDuals(SCIP* scip) const;
    double solveKnapsack(const std::vector<double>& duals,
                        std::vector<int>& pattern) const;
};

/* -------------------------------------------------------------------
   SIMPLE CUTTING STOCK SOLVER
------------------------------------------------------------------- */
class SimpleCuttingStockSolver {
private:
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