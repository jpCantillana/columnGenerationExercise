#include "../include/scip_solver.hpp"
#include <stdexcept>

// Constructor
ScipSolver::ScipSolver(const std::string& name) : scip_(nullptr) {
    SCIP_CALL_EXCEPT( SCIPcreate(&scip_) );
    SCIP_CALL_EXCEPT( SCIPincludeDefaultPlugins(scip_) );
    SCIP_CALL_EXCEPT( SCIPcreateProbBasic(scip_, name.c_str()) );
    SCIP_CALL_EXCEPT( SCIPsetObjsense(scip_, SCIP_OBJSENSE_MINIMIZE) );
}

// Destructor
ScipSolver::~ScipSolver() noexcept {
    if (scip_ != nullptr) {
        SCIPfree(&scip_);
    }
}

// Move constructor
ScipSolver::ScipSolver(ScipSolver&& other) noexcept : scip_(other.scip_) {
    other.scip_ = nullptr;
}

// Move assignment
ScipSolver& ScipSolver::operator=(ScipSolver&& other) noexcept {
    if (this != &other) {
        if (scip_ != nullptr) {
            SCIPfree(&scip_);
        }
        scip_ = other.scip_;
        other.scip_ = nullptr;
    }
    return *this;
}

// Objective sense
void ScipSolver::setMaximize() {
    SCIP_CALL_EXCEPT( SCIPsetObjsense(scip_, SCIP_OBJSENSE_MAXIMIZE) );
}

void ScipSolver::setMinimize() {
    SCIP_CALL_EXCEPT( SCIPsetObjsense(scip_, SCIP_OBJSENSE_MINIMIZE) );
}

// Solving
void ScipSolver::solve() {
    SCIP_CALL_EXCEPT( SCIPsolve(scip_) );
}

SCIP_STATUS ScipSolver::getStatus() const {
    return SCIPgetStatus(scip_);
}

double ScipSolver::getObjectiveValue() const {
    return SCIPgetPrimalbound(scip_);
}

// Variable factory
ScipVariable ScipSolver::createVariable(const std::string& name,
                                       double lb, double ub, double obj,
                                       SCIP_VARTYPE type) {
    return ScipVariable(scip_, name, lb, ub, obj, type);
}

// Constraint factory
ScipConstraint ScipSolver::createConstraint(const std::string& name,
                                           const std::vector<ScipVariable*>& variables,
                                           const std::vector<double>& coefficients,
                                           double lhs, double rhs) {
    return ScipConstraint(scip_, name, variables, coefficients, lhs, rhs);
}

// Convenience method
ScipConstraint ScipSolver::createLessEqualConstraint(const std::string& name,
                                                    const std::vector<ScipVariable*>& variables,
                                                    double rhs) {
    std::vector<double> coeffs(variables.size(), 1.0);
    return createConstraint(name, variables, coeffs, -SCIPinfinity(scip_), rhs);
}