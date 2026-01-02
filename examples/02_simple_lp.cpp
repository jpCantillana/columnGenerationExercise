// A complete, self-contained SCIP program
// Solves max 2x + y subject to x + y ≤ 3, x ≥ 0, y ≥ 0
// Prints solution and statistics
// This gives you the SCIP "template" you'll reuse

#include <iostream>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>

// in c++ the main code is always in the main function
int main() {
    // 1. Declare SCIP pointer. This is the SCIP workspace. We cannot directly access it.
    SCIP* scip = nullptr;
    
    // 2. Create SCIP environment
    SCIP_CALL( SCIPcreate(&scip) );
    
    // 3. Include default plugins (LP solver, presolving, etc.)
    SCIP_CALL( SCIPincludeDefaultPlugins(scip) );
    
    // 4. Create empty problem. This only names it and makes it a problem.
    // Allows for multiple problem solving by naming.
    SCIP_CALL( SCIPcreateProbBasic(scip, "simple_lp") );
    // add sense
    SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE) );
    
    // 5. Create variables x and y using the classes from scip.h
    // starting as a nullptr is a safety measure, prevents from garbage values
    SCIP_VAR* x = nullptr;
    SCIP_VAR* y = nullptr;
    
    // Create x (continuous, lower bound 0, upper bound infinity)
    SCIP_CALL( SCIPcreateVarBasic(scip, &x, "x", 0.0, SCIPinfinity(scip), 2.0, SCIP_VARTYPE_CONTINUOUS) );
    SCIP_CALL( SCIPaddVar(scip, x) );
    
    // Create y (similar, but objective coefficient 1.0)
    SCIP_CALL( SCIPcreateVarBasic(scip, &y, "y", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS) );
    SCIP_CALL( SCIPaddVar(scip, y) );
    
    // 6. Create constraint: x + y ≤ 3
    SCIP_CONS* cons = nullptr;
    SCIP_VAR* vars[2] = {x, y};
    double coeffs[2] = {1.0, 1.0};

    SCIP_CALL( SCIPcreateConsBasicLinear(scip, &cons, "capacity", 
                    2,
                    vars,
                    coeffs,
                    -SCIPinfinity(scip),
                    3.0) );
    
    // 7. Add constraint to problem
    SCIP_CALL( SCIPaddCons(scip, cons) );
    
    // 8. Solve the problem
    SCIP_CALL( SCIPsolve(scip) );
    
    // 9. Print solution if optimal
    if (SCIPgetStatus(scip) == SCIP_STATUS_OPTIMAL) {
        std::cout << "Optimal solution found!" << std::endl;
        std::cout << "Objective value: " << SCIPgetPrimalbound(scip) << std::endl;
        // How do we get x and y values?
        // Get the best solution found
        SCIP_SOL* sol = SCIPgetBestSol(scip);
        
        if (sol != nullptr) {
            // Get x value from solution
            SCIP_Real x_val = SCIPgetSolVal(scip, sol, x);
            SCIP_Real y_val = SCIPgetSolVal(scip, sol, y);
            
            std::cout << "x = " << x_val << std::endl;
            std::cout << "y = " << y_val << std::endl;
        }
    }
    
    // 10. Free constraint
    SCIP_CALL( SCIPreleaseCons(scip, &cons) );
    
    // 11. Free variables
    SCIP_CALL( SCIPreleaseVar(scip, &x) );
    SCIP_CALL( SCIPreleaseVar(scip, &y) );

    
    // 12. Free SCIP environment
    SCIP_CALL( SCIPfree(&scip) );
    
    return 0;
}