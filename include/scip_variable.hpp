#ifndef SCIP_VARIABLE_HPP
#define SCIP_VARIABLE_HPP

#include <string>
#include <scip/scip.h>

class ScipVariable {
private:
    SCIP* scip_;      // Non-owning reference to SCIP environment
    SCIP_VAR* var_;   // Owned SCIP variable
    
public:
    // Constructor: creates and adds variable to SCIP
    ScipVariable(SCIP* scip, const std::string& name, 
                 double lb, double ub, double obj, 
                 SCIP_VARTYPE type = SCIP_VARTYPE_CONTINUOUS);
    
    // Destructor: releases variable
    ~ScipVariable();
    
    // Delete copy operations
    ScipVariable(const ScipVariable&) = delete;
    ScipVariable& operator=(const ScipVariable&) = delete;
    
    // Move operations
    ScipVariable(ScipVariable&& other) noexcept;
    ScipVariable& operator=(ScipVariable&& other) noexcept;
    
    // Accessors
    SCIP_VAR* get() const { return var_; }
    SCIP* getScip() const { return scip_; }
    
    // Get solution value (nullptr for current best solution)
    double getSolutionValue(SCIP_SOL* sol = nullptr) const;
    
    // Get name (optional)
    std::string getName() const;
};

#endif // SCIP_VARIABLE_HPP