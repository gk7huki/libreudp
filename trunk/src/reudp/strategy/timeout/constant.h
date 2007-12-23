#ifndef REUDP_STRATEGY_TIMEOUT_CONSTANT_H
#define REUDP_STRATEGY_TIMEOUT_CONSTANT_H

#include "../../common.h"
#include "../../config.h"

namespace reudp {
namespace strategy {
namespace timeout {
    /**
     * Implements a constant timeout strategy
     * 
     * Each host has a constant timeout (default 2 seconds)
     */
    class constant {
    public:
        // Constant doesn't need individual peer structs
        struct peer_struct {};
        inline time_value_type next_resend_time(
            const time_value_type &now,
            const dgram_send_info & /*si*/,
            peer_struct & /*peer_struct */
        ) {
            return now + config::timeout();
        }
        // Called when an ack is received for existing peer.
        inline void ack_received(
            const time_value_type &now,
            const dgram_send_info &si,
            peer_struct &/*ps*/
        ) {}
        
        inline void packet_sent(
            const time_value_type &now,
            const dgram_send_info &si,
            peer_struct &/*ps*/
        ) {}
        inline uint32_t send_try_count(peer_struct &/*ps*/) {
            // By default return the value from config
            return config::send_try_count();
        }
        
    };
} // ns strategy
} // ns timeout
} // ns reudp

#endif /*REUDP_STRATEGY_TIMEOUT_CONSTANT_H_*/
