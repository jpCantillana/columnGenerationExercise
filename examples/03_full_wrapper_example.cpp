// Clean example using the separated wrapper library

#include <iostream>
#include <vector>
#include "../include/scip_solver.hpp"

int main() {
    try {
        std::cout << "=== Full Wrapper Library Example ===" << std::endl;
        
        // 1. Create solver
        ScipSolver solver("simple_lp");
        solver.setMaximize();
        
        // 2. Create variables
        auto x = solver.createVariable("x", 0.0, SCIPinfinity(solver.get()), 2.0);
        auto y = solver.createVariable("y", 0.0, SCIPinfinity(solver.get()), 1.0);
        
        std::cout << "Created variables x and y" << std::endl;
        
        // 3. Create constraint: x + y ≤ 3
        std::vector<ScipVariable*> vars = {&x, &y};
        std::vector<double> coeffs = {1.0, 1.0};
        auto cons = solver.createConstraint("capacity", vars, coeffs,
                                           -SCIPinfinity(solver.get()), 3.0);
        
        std::cout << "Created constraint: x + y ≤ 3" << std::endl;
        
        // 4. Solve
        std::cout << "Solving..." << std::endl;
        solver.solve();
        
        // 5. Print results
        if (solver.getStatus() == SCIP_STATUS_OPTIMAL) {
            std::cout << "\n=== SOLUTION ===" << std::endl;
            std::cout << "Status: OPTIMAL" << std::endl;
            std::cout << "Objective value: " << solver.getObjectiveValue() << std::endl;
            std::cout << "x = " << x.getSolutionValue() << std::endl;
            std::cout << "y = " << y.getSolutionValue() << std::endl;
            std::cout << "Dual value (shadow price): " << cons.getDualValue() << std::endl;
            
            // Verify constraint
            double lhs = x.getSolutionValue() + y.getSolutionValue();
            std::cout << "x + y = " << lhs << " (≤ 3)" << std::endl;
            
            if (lhs > 3.0 + 1e-6) {
                std::cout << "WARNING: Constraint violated!" << std::endl;
            }
        }
        else {
            std::cout << "No optimal solution found." << std::endl;
            std::cout << "Status code: " << solver.getStatus() << std::endl;
        }
        
        std::cout << "\n=== Example completed successfully ===" << std::endl;
        
    } catch (const ScipException& e) {
        std::cerr << "\nSCIP ERROR: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}