#ifndef REUDP_STRATEGY_FILTER_BASE_H
#define REUDP_STRATEGY_FILTER_BASE_H

#include "../../common.h"
#include "../../config.h"
#include "../../exception.h"
#include "../../dgram_send_info.h"

namespace reudp {
namespace strategy {
namespace timeout {
    /**
     * Base class for filter strategies.
     * 
     * Each implementation of filter strategy should inherit
     * from this class and re-implement relevant functions.
     * The implementations are used as template parameters so
     * no virtual functions needed and overhead is non-existing.
     * 
     */
    class filter_base {
    public:        
        // Called when an ack is received
        inline void ack_received(
            const time_value_type &now,
            const dgram_send_info &si
        ) {}
        
        // Called when packet received
        inline void packet_received(
            const time_value_type &now,
            const dgram_send_info &si
        ) {}
    };
} // ns filter
} // ns strategy
} // ns reudp

#endif /*REUDP_STRATEGY_FILTER_BASE_H_*/
