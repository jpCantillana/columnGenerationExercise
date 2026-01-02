#ifndef SCIP_EXCEPTION_HPP
#define SCIP_EXCEPTION_HPP

#include <stdexcept>
#include <scip/scip.h>

class ScipException : public std::runtime_error {
public:
    ScipException(const std::string& msg, SCIP_RETCODE retcode)
        : std::runtime_error(msg + " (SCIP error: " + std::to_string(retcode) + ")") {}
};

#define SCIP_CALL_EXCEPT(x) do { \
    SCIP_RETCODE retcode = (x); \
    if (retcode != SCIP_OKAY) { \
        throw ScipException("SCIP call failed", retcode); \
    } \
} while (false)

#endif // SCIP_EXCEPTION_HPP