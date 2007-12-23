#ifndef REUDP_H
#define REUDP_H

#include "common.h"
#include "seqack_adapter.h"
#include "seqack_dgram.h"
#include "ack_resend_strategy.h"
#include "strategy/timeout/constant.h"
#include "strategy/timeout/jacobson_karn.h"
#include "strategy/peer_container/peer_container_nop.h"
#include "strategy/peer_container/peer_container_map.h"

/**
 * @file    reudp.h
 * @date    23.12.2007
 * @author  Arto Jalkanen
 * @brief   Base include for applications using reudp
 * 
 * Defines different datagram types and the default
 * datagram type. ReUDP currently provides these
 * datagram types:
 * - dgram_constant_timeout
 *   - Uses a constant timeout for each packet and each peer
 * - dgram_variable_timeout
 *   - The timeout varies depending on the peer's history
 * 
 * The default datagram type (dgram) uses variable timeout. 
 *
 */

namespace reudp {
    typedef reudp::ack_resend_strategy<
        strategy::timeout::constant,
        strategy::peer_container::peer_container_nop<
            strategy::timeout::constant::peer_struct
        >
    > constant_timeout_strategy;

    typedef reudp::ack_resend_strategy<
        strategy::timeout::jacobson_karn,
        strategy::peer_container::peer_container_map<
            strategy::timeout::jacobson_karn::peer_struct
        >
    > variable_timeout_strategy;
    
    typedef seqack_adapter<seqack_dgram, constant_timeout_strategy>
            dgram_constant_timeout;
    typedef seqack_adapter<seqack_dgram, variable_timeout_strategy>
            dgram_variable_timeout;
            
    typedef dgram_variable_timeout dgram;
}

#endif // REUDP_H
