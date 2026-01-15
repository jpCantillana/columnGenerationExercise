#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/pricer.h"

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <cassert>

/* ----------------------------
   PRICER DATA
---------------------------- */
struct PricerData
{
    std::vector<SCIP_CONS*> demandConss;
    std::vector<int> widths;
    int rollWidth;

    PricerData(const std::vector<int>& w, int rw) : widths(w), rollWidth(rw) {}
};

/* ----------------------------
   PRICER CALLBACKS
---------------------------- */

static
SCIP_DECL_PRICERFREE(pricerFree)
{
    PricerData* data = (PricerData*)SCIPpricerGetData(pricer);
    if(data != nullptr) {
        delete data;
        SCIPpricerSetData(pricer, nullptr);
    }
    return SCIP_OKAY;
}

static
SCIP_DECL_PRICERINIT(pricerInit)
{
    PricerData* data = (PricerData*)SCIPpricerGetData(pricer);
    for(size_t i = 0; i < data->demandConss.size(); ++i) {
        SCIP_CONS* trans = nullptr;
        SCIP_CALL( SCIPgetTransformedCons(scip, data->demandConss[i], &trans) );
        data->demandConss[i] = trans;
    }
    return SCIP_OKAY;
}

static
SCIP_DECL_PRICERREDCOST(pricerRedcost)
{
    PricerData* data = (PricerData*)SCIPpricerGetData(pricer);
    // Crucial: Always set result to SUCCESS to avoid "pricing aborted" errors
    *result = SCIP_SUCCESS; 

    int nItems = data->widths.size();
    int W = data->rollWidth;

    // 1. Get Duals
    std::vector<SCIP_Real> duals(nItems);
    for(int i = 0; i < nItems; ++i)
        duals[i] = SCIPgetDualsolLinear(scip, data->demandConss[i]);

    // 2. Solve Knapsack via Dynamic Programming
    // dp[w] stores max dual value for width w
    // keepTrack[w] stores which item was last added to achieve that value
    std::vector<SCIP_Real> dp(W + 1, 0.0);
    std::vector<int> keepTrack(W + 1, -1);

    for (int w = 0; w <= W; ++w) {
        for (int i = 0; i < nItems; ++i) {
            if (data->widths[i] <= w) {
                if (dp[w - data->widths[i]] + duals[i] > dp[w]) {
                    dp[w] = dp[w - data->widths[i]] + duals[i];
                    keepTrack[w] = i;
                }
            }
        }
    }

    SCIP_Real bestValue = dp[W];

    // 3. If Reduced Cost is negative (Value > 1.0), extract pattern and add variable
    if (bestValue > 1.0 + 1e-6) {
        std::vector<int> pattern(nItems, 0);
        int tempW = W;
        while (tempW > 0 && keepTrack[tempW] != -1) {
            int itemIdx = keepTrack[tempW];
            pattern[itemIdx]++;
            tempW -= data->widths[itemIdx];
        }

        // Create and add the Priced Variable
        SCIP_VAR* var = nullptr;
        std::string name = "pat_" + std::to_string(SCIPgetNLPCols(scip));
        SCIP_CALL( SCIPcreateVarBasic(scip, &var, name.c_str(), 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_INTEGER) );
        SCIP_CALL( SCIPvarSetInitial(var, TRUE) );
        SCIP_CALL( SCIPvarSetRemovable(var, TRUE) );
        SCIP_CALL( SCIPaddPricedVar(scip, var, 1.0) );

        for (int i = 0; i < nItems; ++i) {
            if (pattern[i] > 0)
                SCIP_CALL( SCIPaddCoefLinear(scip, data->demandConss[i], var, (SCIP_Real)pattern[i]) );
        }

        std::cout << "DP found pattern with value: " << bestValue << "\n";
        SCIP_CALL( SCIPreleaseVar(scip, &var) );
    }

    return SCIP_OKAY;
}

static
SCIP_DECL_PRICERFARKAS(pricerFarkas)
{
    *result = SCIP_SUCCESS;
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
    SCIP_CALL( SCIPcreateProbBasic(scip, "cutting_stock_dp") );
    SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE) );

    // Problem Data
    int rollWidth = 100;
    std::vector<int> itemWidths = {20, 35, 50};
    std::vector<int> demands = {40, 30, 20};

    // 1. Constraints
    std::vector<SCIP_CONS*> conss;
    for(size_t i = 0; i < itemWidths.size(); ++i) {
        SCIP_CONS* cons;
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &cons, "d", 0, nullptr, nullptr, (double)demands[i], SCIPinfinity(scip)) );
        SCIP_CALL( SCIPsetConsModifiable(scip, cons, TRUE) );
        SCIP_CALL( SCIPaddCons(scip, cons) );
        conss.push_back(cons);
    }

    // 2. Initial Columns (One roll per item)
    for(size_t i = 0; i < itemWidths.size(); ++i) {
        SCIP_VAR* var;
        SCIP_CALL( SCIPcreateVarBasic(scip, &var, "init", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS) );
        SCIP_CALL( SCIPaddVar(scip, var) );
        SCIP_CALL( SCIPaddCoefLinear(scip, conss[i], var, 1.0) );
        SCIP_CALL( SCIPreleaseVar(scip, &var) );
    }

    // 3. Pricer setup
    PricerData* pricerdata = new PricerData(itemWidths, rollWidth);
    pricerdata->demandConss = conss;

    SCIP_CALL( SCIPincludePricer(scip, "dp_pricer", "DP Knapsack Pricer", 0, TRUE, 
                                 nullptr, pricerFree, pricerInit, nullptr, nullptr, nullptr, 
                                 pricerRedcost, pricerFarkas, (SCIP_PRICERDATA*)pricerdata) );
    SCIP_CALL( SCIPactivatePricer(scip, SCIPfindPricer(scip, "dp_pricer")) );

    // 4. Solve
    SCIP_CALL( SCIPsolve(scip) );

    for(auto c : conss) SCIP_CALL( SCIPreleaseCons(scip, &c) );
    SCIP_CALL( SCIPfree(&scip) );

    return 0;
}