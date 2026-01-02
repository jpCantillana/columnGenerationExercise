#include "../include/scip_variable.hpp"
#include "../include/scip_constraint.hpp"
#include <stdexcept>
#include <scip/scipdefplugins.h>  // MUST BE FIRST!
#include <scip/scip.h>
#include <scip/scip_cons.h>  // ← ADD THIS! For SCIPcreateConsBasicLinear
#include "../include/scip_exception.hpp"

ScipConstraint::ScipConstraint(SCIP* scip, const std::string& name,
                               const std::vector<ScipVariable*>& variables,
                               const std::vector<double>& coefficients,
                               double lhs, double rhs)
    : scip_(scip), cons_(nullptr), coeffs_(coefficients) {
    
    // 1. Validate input
    if (variables.size() != coefficients.size()) {
        throw std::runtime_error("Variables and coefficients size mismatch");
    }
    if (variables.empty()) {
        throw std::runtime_error("Constraint must have at least one variable");
    }

    
    // 2. Convert ScipVariable* to SCIP_VAR* and store
    vars_.reserve(variables.size());
    for (const auto* var : variables) {
        if (var == nullptr) {
            throw std::runtime_error("Null variable pointer in constraint");
        }
        vars_.push_back(var->get());  // Use -> operator to get raw pointer
    }
    
    // 4. Create constraint in SCIP
    SCIP_CALL_EXCEPT(SCIPcreateConsBasicLinear(scip_, &cons_, name.c_str(),
                     vars_.size(), vars_.data(), coeffs_.data(),
                     lhs, rhs));
    
    // 5. Add to problem
    SCIP_CALL_EXCEPT(SCIPaddCons(scip_, cons_));
}

ScipConstraint::~ScipConstraint() noexcept {
    if (cons_ != nullptr) {
        SCIPreleaseCons(scip_, &cons_);
    }
}

double ScipConstraint::getDualValue() const {
    if (cons_ == nullptr) {
        throw std::runtime_error("Constraint not initialized");
    }
    return SCIPgetDualsolLinear(scip_, cons_);
}

ScipConstraint::ScipConstraint(ScipConstraint&& other) noexcept
    : scip_(other.scip_),
      cons_(other.cons_),
      vars_(std::move(other.vars_)),      // Vector move
      coeffs_(std::move(other.coeffs_)) { // Vector move
    
    other.cons_ = nullptr;
    // scip_ stays valid in other
}

// Move assignment  
ScipConstraint& ScipConstraint::operator=(ScipConstraint&& other) noexcept {
    if (this != &other) {
        // Release current
        if (cons_ != nullptr) {
            SCIPreleaseCons(scip_, &cons_);
        }
        
        // Transfer
        scip_ = other.scip_;
        cons_ = other.cons_;
        vars_ = std::move(other.vars_);
        coeffs_ = std::move(other.coeffs_);
        
        other.cons_ = nullptr;
    }
    return *this;
}

// Add variable to constraint (for column generation)
void ScipConstraint::addVariable(ScipVariable* variable, double coefficient) {
    if (cons_ == nullptr) {
        throw std::runtime_error("Constraint not initialized");
    }
    
    if (variable == nullptr) {
        throw std::runtime_error("Cannot add null variable to constraint");
    }
    
    // Add to SCIP
    SCIP_CALL_EXCEPT(SCIPaddCoefLinear(scip_, cons_, variable->get(), coefficient));
    
    // Update internal tracking
    vars_.push_back(variable->get());
    coeffs_.push_back(coefficient);
}