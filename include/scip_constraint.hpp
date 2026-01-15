#ifndef SCIP_CONSTRAINT_HPP
#define SCIP_CONSTRAINT_HPP

#include <string>
#include <vector>
#include <scip/scip.h>
#include "scip_variable.hpp"

class ScipConstraint {
private:
    SCIP* scip_;                 // Non-owning reference
    SCIP_CONS* cons_;           // Owned SCIP constraint
    std::vector<SCIP_VAR*> vars_;      // Store raw pointers for SCIP
    std::vector<double> coeffs_;       // Store coefficients
    
public:
    // Constructor 1: For normal constraints with variables
    ScipConstraint(SCIP* scip, const std::string& name,
                   const std::vector<ScipVariable*>& variables,
                   const std::vector<double>& coefficients,
                   double lhs, double rhs);
    
    // Constructor 2: For empty constraints (column generation)
    ScipConstraint(SCIP* scip, const std::string& name,
                   double lhs, double rhs);
    
    ~ScipConstraint();
                   
    // No copying (constraints can't be copied in SCIP)
    ScipConstraint(const ScipConstraint&) = delete;
    ScipConstraint& operator=(const ScipConstraint&) = delete;
    
    // Move operations
    ScipConstraint(ScipConstraint&& other) noexcept;
    ScipConstraint& operator=(ScipConstraint&& other) noexcept;
    
    // Accessors
    SCIP_CONS* get() const { return cons_; }
    SCIP* getScip() const { return scip_; }
    
    // Get dual value (shadow price) - useful for column generation!
    double getDualValue() const;
    
    // Get constraint name
    std::string getName() const;

    // Add a variable to constraint (for column generation)
    void addVariable(ScipVariable* variable, double coefficient);

    // Get variables and coefficients (for debugging)
    const std::vector<SCIP_VAR*>& getRawVariables() const { return vars_; }
    const std::vector<double>& getCoefficients() const { return coeffs_; }
};

#endif // SCIP_CONSTRAINT_HPP