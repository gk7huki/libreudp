#ifndef _REUDP_DGRAM_H
#define _REUDP_DGRAM_H

/**
 * @file    dgram.h
 * @date    Apr 6, 2005
 * @author  Arto Jalkanen
 * @brief   Represents a reliable UDP datagram
 *
 * Handles reading and writing the data from UDP packet that takes care of
 * ACKs, resends, timers that are needed for reliable UDP messaging.
 */

#include <ace/SOCK_Dgram.h>

#include "ack_resend_strategy.h"
#include "seqack_adapter.h"
#include "seqack_dgram.h"

namespace reudp {   
    class dgram : public seqack_adapter<seqack_dgram, ack_resend_strategy> {
    public:
        dgram() {}
        virtual ~dgram() {}
        
    };
}

#endif
