#include "../include/cg_solver.hpp"
#include <iostream>

int main() {
    // Problem data from original example
    int rollWidth = 100;
    std::vector<int> itemWidths = {20, 35, 50};
    std::vector<int> demands = {40, 30, 20};
    
    std::cout << "=== Cutting Stock Problem ===" << std::endl;
    std::cout << "Roll width: " << rollWidth << std::endl;
    std::cout << "Items: ";
    for (size_t i = 0; i < itemWidths.size(); ++i) {
        std::cout << "width=" << itemWidths[i] << ", demand=" << demands[i] << " | ";
    }
    std::cout << std::endl;
    
    try {
        // Create solver
        CuttingStockSolver solver(itemWidths, demands, rollWidth);
        
        // Set parameters - be more lenient
        solver.setParameters(1e-4, 100);
        
        std::cout << "\nStarting column generation..." << std::endl;
        
        // Solve the problem
        double result = solver.solve();
        
        // Print solution
        solver.printSolution();
        
        std::cout << "\nOptimal solution uses approximately " 
                  << result << " rolls" << std::endl;
        
        // Calculate theoretical lower bound (sum(width*demand)/rollWidth)
        double totalWidth = 0;
        for (size_t i = 0; i < itemWidths.size(); ++i) {
            totalWidth += itemWidths[i] * demands[i];
        }
        double lowerBound = totalWidth / rollWidth;
        std::cout << "Theoretical lower bound: " << lowerBound << std::endl;
        std::cout << "Gap: " << result - lowerBound << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "At line: " << __LINE__ << std::endl;
        return 1;
    }
    
    return 0;
}