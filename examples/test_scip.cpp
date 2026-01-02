#include <iostream>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>

int main() {
    std::cout << "Testing SCIP installation..." << std::endl;
    
    SCIP* scip = nullptr;
    SCIP_CALL(SCIPcreate(&scip));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));
    
    std::cout << "SCIP version: " << SCIPversion() << std::endl;
    
    SCIPfree(&scip);
    return 0;
}