#include "../include/scip_pricer.hpp"
#include <iostream>

int main() {
    // Problem data
    int rollWidth = 100;
    std::vector<int> itemWidths = {20, 35, 50};
    std::vector<int> demands = {40, 30, 20};
    
    std::cout << "=== Cutting Stock Problem ===" << std::endl;
    std::cout << "Roll width: " << rollWidth << std::endl;
    for (size_t i = 0; i < itemWidths.size(); ++i) {
        std::cout << "Item " << i << ": width=" << itemWidths[i] 
                  << ", demand=" << demands[i] << std::endl;
    }
    
    try {
        // Create and solve
        SimpleCuttingStockSolver solver(itemWidths, demands, rollWidth);
        double result = solver.solve();
        
        solver.printSolution();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}