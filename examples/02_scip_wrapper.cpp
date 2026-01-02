// examples/02_scip_wrapper.cpp
#include <iostream>
#include <memory>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "../include/scip_variable.hpp"
#include "../include/scip_constraint.hpp"

class ScipException : public std::runtime_error {
public:
    ScipException(const std::string& msg, SCIP_RETCODE retcode)
        : std::runtime_error(msg + " (SCIP error: " + std::to_string(retcode) + ")") {}
};

// Helper to convert SCIP_CALL to exception
#define SCIP_CALL_EXCEPT(x) do { \
    SCIP_RETCODE retcode = (x); \
    if (retcode != SCIP_OKAY) { \
        throw ScipException("SCIP call failed", retcode); \
    } \
} while (false)

class ScipSolver {
private:
    SCIP* scip_;
    
public:
    // Constructor: initialize SCIP
    ScipSolver(const std::string& name = "problem") {
        // Create SCIP environment
        SCIP_CALL_EXCEPT( SCIPcreate(&scip_) );
        // Include default plugins
        SCIP_CALL_EXCEPT( SCIPincludeDefaultPlugins(scip_) );
        // Create problem with given name
        SCIP_CALL_EXCEPT( SCIPcreateProbBasic(scip_, name.c_str()) );
        // Set default to maximize? Or provide method?
        SCIP_CALL_EXCEPT( SCIPsetObjsense(scip_, SCIP_OBJSENSE_MINIMIZE) );
    }
    
    // Destructor: clean up
    ~ScipSolver() noexcept {
        if (scip_ != nullptr) {
            // Ignore return code in destructor
            SCIPfree(&scip_);
        }
    }

    // Delete copy constructor/assignment
    ScipSolver(const ScipSolver&) = delete;
    ScipSolver& operator=(const ScipSolver&) = delete;
    
    // Allow move
    ScipSolver(ScipSolver&& other) noexcept : scip_(other.scip_) {
        other.scip_ = nullptr;
    }
    
    ScipSolver& operator=(ScipSolver&& other) noexcept {
        if (this != &other) {
            if (scip_ != nullptr) {
                SCIPfree(&scip_);
            }
            scip_ = other.scip_;
            other.scip_ = nullptr;
        }
        return *this;
    }
    
    // Get raw SCIP pointer (for advanced use)
    SCIP* get() { return scip_; }
    const SCIP* get() const { return scip_; }
    
    // Set objective sense
    void setMaximize() {
        SCIP_CALL_EXCEPT( SCIPsetObjsense(scip_, SCIP_OBJSENSE_MAXIMIZE) );
    }
    
    void setMinimize() {
        SCIP_CALL_EXCEPT( SCIPsetObjsense(scip_, SCIP_OBJSENSE_MINIMIZE) );
    }
    
    // Solve the problem
    void solve() {
        SCIP_CALL_EXCEPT( SCIPsolve(scip_) );
    }
    
    // Get solution status
    SCIP_STATUS getStatus() const {
        return SCIPgetStatus(scip_);
    }
    
    // Get objective value
    double getObjectiveValue() const {
       return  SCIPgetPrimalbound(scip_);
    }
    
    // Factory method for variables - NOW RETURNS ScipVariable!
    ScipVariable createVariable(const std::string& name,
                               double lb, double ub, double obj,
                               SCIP_VARTYPE type = SCIP_VARTYPE_CONTINUOUS) {
        return ScipVariable(scip_, name, lb, ub, obj, type);
    }

    // Factory method for constraints
    ScipConstraint createConstraint(const std::string& name,
                                    const std::vector<ScipVariable*>& variables,
                                    const std::vector<double>& coefficients,
                                    double lhs, double rhs) {
        return ScipConstraint(scip_, name, variables, coefficients, lhs, rhs);
    }

    // Convenience method for common case: sum(vars) ≤ rhs
    ScipConstraint createLessEqualConstraint(const std::string& name,
                                             const std::vector<ScipVariable*>& variables,
                                             double rhs) {
        std::vector<double> coeffs(variables.size(), 1.0);
        return ScipConstraint(scip_, name, variables, coeffs, 
                             -SCIPinfinity(scip_), rhs);
    }
};

int main() {
    try {
        // Create solver
        ScipSolver solver("simple_lp");
        solver.setMaximize();
        
        // Create variables using ScipVariable wrapper!
        auto x = solver.createVariable("x", 0.0, SCIPinfinity(solver.get()), 2.0);
        auto y = solver.createVariable("y", 0.0, SCIPinfinity(solver.get()), 1.0);
        // Get raw SCIP for constraint (we'll wrap constraints later)
        SCIP* scip = solver.get();
        
        // 6. Create constraint: x + y ≤ 3
        // Create constraint using wrapper!
        std::vector<ScipVariable*> var_ptrs = {&x, &y}; //Note: pointers
        std::vector<double> coeffs = {1.0, 1.0};
        auto cons = solver.createConstraint("capacity", var_ptrs, coeffs, 
                                           -SCIPinfinity(solver.get()), 3.0);

        // Solve
        solver.solve();
        
        // Print results
        if (solver.getStatus() == SCIP_STATUS_OPTIMAL) {
            std::cout << "Objective: " << solver.getObjectiveValue() << std::endl;
        
            // Use ScipVariable's getSolutionValue()!
            std::cout << "x = " << x.getSolutionValue() << std::endl;
            std::cout << "y = " << y.getSolutionValue() << std::endl;
            std::cout << "x + y = " << (x.getSolutionValue() + y.getSolutionValue()) 
                      << " (should be ≤ 3)" << std::endl;
            // NEW: Get dual value (shadow price)
            std::cout << "Dual value of constraint: " << cons.getDualValue() << std::endl;
        }
        else {
            std::cout << "No optimal solution. Status: " << solver.getStatus() << std::endl;
        }
        
    } catch (const ScipException& e) {
        std::cerr << "SCIP error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}