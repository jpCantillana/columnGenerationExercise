#include "../include/scip_variable.hpp"
#include <stdexcept>
#include "../include/scip_exception.hpp"

ScipVariable::ScipVariable(SCIP* scip, const std::string& name,
                           double lb, double ub, double obj,
                           SCIP_VARTYPE type)
    : scip_(scip), var_(nullptr) {
    
    // Create variable with SCIPcreateVarBasic
    const char* cname = name.c_str();
    SCIP_CALL_EXCEPT(SCIPcreateVarBasic(scip, &var_, cname, lb, ub, obj, type ));
    
    // Add variable to problem with SCIPaddVar
    SCIP_CALL_EXCEPT(SCIPaddVar(scip, var_));

}

ScipVariable::~ScipVariable() noexcept {
    if (var_ != nullptr) {
        // Don't throw in destructor - ignore return code
        SCIPreleaseVar(scip_, &var_);
    }
}

ScipVariable::ScipVariable(ScipVariable&& other) noexcept
    : scip_(other.scip_), var_(other.var_) {
    
    other.var_ = nullptr;  // Important!
}

ScipVariable& ScipVariable::operator=(ScipVariable&& other) noexcept {
    if (this != &other) {
        // Release current variable if it exists
        if (var_ != nullptr) {
            SCIPreleaseVar(scip_, &var_);
        }
        
        // Transfer ownership
        scip_ = other.scip_;
        var_ = other.var_;
        other.var_ = nullptr;  // Important!
    }
    return *this;
}

double ScipVariable::getSolutionValue(SCIP_SOL* sol) const {
    // Check if var_ is valid
    if (var_ == nullptr) {
        throw std::runtime_error("Variable not initialized or moved from");
    }
    // Get solution (default to best solution)
    if (sol == nullptr) {
        sol = SCIPgetBestSol(scip_);
        if (sol == nullptr) {
            throw std::runtime_error("No solution available");
        }
    }

    return SCIPgetSolVal(scip_, sol, var_);
}

std::string ScipVariable::getName() const {
    if (var_ == nullptr) {
        return "[invalid variable]";
    }
    return SCIPvarGetName(var_);  // SCIP function to get variable name
}