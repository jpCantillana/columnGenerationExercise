#ifndef SCIP_SOLVER_HPP
#define SCIP_SOLVER_HPP

#include <string>
#include <vector>
#include <scip/scip.h>
#include "scip_variable.hpp"
#include "scip_constraint.hpp"
#include "scip_exception.hpp"
#include <scip/scipdefplugins.h>

class ScipSolver {
private:
    SCIP* scip_;
    
public:
    ScipSolver(const std::string& name = "problem");
    ~ScipSolver() noexcept;
    
    // Delete copy operations
    ScipSolver(const ScipSolver&) = delete;
    ScipSolver& operator=(const ScipSolver&) = delete;
    
    // Move operations
    ScipSolver(ScipSolver&& other) noexcept;
    ScipSolver& operator=(ScipSolver&& other) noexcept;
    
    // Accessors
    SCIP* get() { return scip_; }
    const SCIP* get() const { return scip_; }
    
    // Objective sense
    void setMaximize();
    void setMinimize();
    
    // Solving
    void solve();
    SCIP_STATUS getStatus() const;
    double getObjectiveValue() const;
    
    // Variable factory
    ScipVariable createVariable(const std::string& name,
                               double lb, double ub, double obj,
                               SCIP_VARTYPE type = SCIP_VARTYPE_CONTINUOUS);
    
    // Constraint factory
    ScipConstraint createConstraint(const std::string& name,
                                   const std::vector<ScipVariable*>& variables,
                                   const std::vector<double>& coefficients,
                                   double lhs, double rhs);
    
    // Convenience methods
    ScipConstraint createLessEqualConstraint(const std::string& name,
                                            const std::vector<ScipVariable*>& variables,
                                            double rhs);
};

#endif // SCIP_SOLVER_HPP