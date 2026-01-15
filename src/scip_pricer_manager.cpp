#include "../include/scip_pricer.hpp"
#include <iostream>

/* -------------------------------------------------------------------
   SCIP PRICER MANAGER IMPLEMENTATION
------------------------------------------------------------------- */
ScipPricerManager* ScipPricerManager::instance_ = nullptr;

ScipPricerManager& ScipPricerManager::getInstance() {
    if (instance_ == nullptr) {
        instance_ = new ScipPricerManager();
    }
    return *instance_;
}

void ScipPricerManager::registerPricer(SCIP_PRICER* scipPricer, BasePricer* pricer) {
    pricerMap_[scipPricer] = pricer;
}

void ScipPricerManager::unregisterPricer(SCIP_PRICER* scipPricer) {
    pricerMap_.erase(scipPricer);
}

BasePricer* ScipPricerManager::getPricer(SCIP_PRICER* scipPricer) {
    auto it = pricerMap_.find(scipPricer);
    if (it != pricerMap_.end()) {
        return it->second;
    }
    return nullptr;
}

/* -------------------------------------------------------------------
   BASE PRICER IMPLEMENTATION
------------------------------------------------------------------- */
BasePricer::BasePricer(SCIP* scip, const std::string& name)
    : scip_(scip), scipPricer_(nullptr), name_(name) {}

BasePricer::~BasePricer() {
    if (scipPricer_ != nullptr) {
        ScipPricerManager::getInstance().unregisterPricer(scipPricer_);
    }
}

// Static callbacks that route to the appropriate C++ object
SCIP_RETCODE BasePricer::scipFreeCallback(SCIP* scip, SCIP_PRICER* pricer) {
    BasePricer* pricerObj = ScipPricerManager::getInstance().getPricer(pricer);
    if (pricerObj != nullptr) {
        return pricerObj->scipFree(scip, pricer);
    }
    return SCIP_OKAY;
}

SCIP_RETCODE BasePricer::scipInitCallback(SCIP* scip, SCIP_PRICER* pricer) {
    BasePricer* pricerObj = ScipPricerManager::getInstance().getPricer(pricer);
    if (pricerObj != nullptr) {
        return pricerObj->scipInit(scip, pricer);
    }
    return SCIP_OKAY;
}

SCIP_RETCODE BasePricer::scipRedcostCallback(SCIP* scip, SCIP_PRICER* pricer,
                                            double* lowerbound,
                                            unsigned int* naddedvars,
                                            SCIP_Result* result) {
    BasePricer* pricerObj = ScipPricerManager::getInstance().getPricer(pricer);
    if (pricerObj != nullptr) {
        return pricerObj->scipRedcost(scip, pricer, lowerbound, naddedvars, result);
    }
    return SCIP_OKAY;
}

SCIP_RETCODE BasePricer::scipFarkasCallback(SCIP* scip, SCIP_PRICER* pricer,
                                           SCIP_Result* result) {
    *result = SCIP_SUCCESS;
    return SCIP_OKAY;
}