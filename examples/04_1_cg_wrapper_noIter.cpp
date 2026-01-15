#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/pricer.h"

#include <vector>
#include <iostream>
#include <string>
#include <cassert>

/* ----------------------------
   PRICER DATA (C-style)
---------------------------- */

typedef struct PricerData
{
    std::vector<SCIP_CONS*> demandConss;   // transformed constraints stored here
} PRICERDATA;


/* ----------------------------
   PRICER CALLBACKS
---------------------------- */

/** initialization method: transforms original constraints into solvable ones */
static
SCIP_DECL_PRICERINIT(pricerInit)
{
    PRICERDATA* data = (PRICERDATA*)SCIPpricerGetData(pricer);
    assert(data != nullptr);

    std::vector<SCIP_CONS*> transConss;
    transConss.reserve(data->demandConss.size());

    for(SCIP_CONS* orig : data->demandConss)
    {
        SCIP_CONS* trans = nullptr;
        // Get the transformed version of the original constraint
        SCIP_CALL( SCIPgetTransformedCons(scip, orig, &trans) );
        assert(trans != nullptr);
        transConss.push_back(trans);
    }

    // Replace original constraints with transformed ones
    data->demandConss.swap(transConss);

    return SCIP_OKAY;
}

static
SCIP_DECL_PRICERREDCOST(pricerRedcost)
{
    PRICERDATA* data = (PRICERDATA*)SCIPpricerGetData(pricer);
    assert(data != nullptr);

    std::cout << "\n--- PRICER CALLED ---" << std::endl;
    std::cout << "LP rows: " << SCIPgetNLPRows(scip) << std::endl;
    std::cout << "LP cols: " << SCIPgetNLPCols(scip) << std::endl;

    for(size_t i = 0; i < data->demandConss.size(); ++i)
    {
        // Dual values are retrieved from the transformed constraints
        SCIP_Real dual = SCIPgetDualsolLinear(scip, data->demandConss[i]);
        std::cout << "dual[" << i << "] = " << dual << std::endl;
    }

    *result = SCIP_SUCCESS;
    return SCIP_OKAY;
}

static
SCIP_DECL_PRICERFARKAS(pricerFarkas)
{
    *result = SCIP_SUCCESS;
    return SCIP_OKAY;
}

static
SCIP_DECL_PRICERFREE(pricerFree)
{
    PRICERDATA* data = (PRICERDATA*)SCIPpricerGetData(pricer);
    if( data != nullptr )
    {
        SCIPfreeBlockMemory(scip, &data);
        // std::cout << "Pricer data should be freed?" << std::endl;
    }
    return SCIP_OKAY;
}

/* ----------------------------
   INCLUDE PRICER
---------------------------- */

static
SCIP_RETCODE includePricer(SCIP* scip, PRICERDATA* data)
{
    SCIP_PRICER* pricer = nullptr;

    // Use the full include function to ensure all callbacks are mapped correctly
    SCIP_CALL( SCIPincludePricer(
        scip,
        "cut_pricer",        // name
        "stock pricer",      // description
        0,                   // priority
        TRUE,                // delay
        nullptr,             // pricer copy
        pricerFree,          // pricer free
        pricerInit,          // pricer init
        nullptr,             // pricer exit
        nullptr,             // pricer initsol
        nullptr,             // pricer exitsol
        pricerRedcost,       // pricer redcost
        pricerFarkas,        // pricer farkas
        (SCIP_PRICERDATA*)data
    ));

    return SCIP_OKAY;
}

/* ----------------------------
   MAIN
---------------------------- */

int main()
{
    SCIP* scip = nullptr;
    SCIP_CALL( SCIPcreate(&scip) );
    SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

    SCIP_CALL( SCIPcreateProbBasic(scip, "cg_master") );
    SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE) );

    /* Disable presolving to prevent it from optimizing away modifiable constraints */
    SCIP_CALL( SCIPsetIntParam(scip, "presolving/maxrounds", 0) );
    SCIP_CALL( SCIPsetLongintParam(scip, "limits/nodes", 1LL) ); 

    const int nItems = 3;
    int demand[nItems] = {4, 3, 2};
    std::vector<SCIP_CONS*> demandConss(nItems);

    for(int i = 0; i < nItems; ++i)
    {
        SCIP_CALL( SCIPcreateConsBasicLinear(
            scip, &demandConss[i], ("demand_" + std::to_string(i)).c_str(),
            0, nullptr, nullptr,
            (SCIP_Real)demand[i], SCIPinfinity(scip)
        ));
        // Constraints MUST be modifiable to add columns during pricing
        SCIP_CALL( SCIPsetConsModifiable(scip, demandConss[i], TRUE) );
        SCIP_CALL( SCIPaddCons(scip, demandConss[i]) );
    }

    for(int i = 0; i < nItems; ++i)
    {
        SCIP_VAR* var = nullptr;
        SCIP_CALL( SCIPcreateVarBasic(
            scip, &var, ("x_" + std::to_string(i)).c_str(),
            0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS
        ));
        SCIP_CALL( SCIPaddVar(scip, var) );
        SCIP_CALL( SCIPaddCoefLinear(scip, demandConss[i], var, 1.0) );
        SCIP_CALL( SCIPreleaseVar(scip, &var) );
    }

    PRICERDATA* pricerdata = new PRICERDATA(); 
    pricerdata->demandConss = demandConss;

    SCIP_CALL( includePricer(scip, pricerdata) );
    SCIP_CALL( SCIPactivatePricer(scip, SCIPfindPricer(scip, "cut_pricer")) );

    SCIP_CALL( SCIPsolve(scip) );

    // Cleanup
    for(int i = 0; i < nItems; ++i)
        SCIP_CALL( SCIPreleaseCons(scip, &demandConss[i]) );

    // SCIPfreeBlockMemory(scip, &pricerdata);
    SCIP_CALL( SCIPfree(&scip) );

    return 0;
}