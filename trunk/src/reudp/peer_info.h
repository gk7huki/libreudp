#ifndef _REUDP_PEER_INFO_H_
#define _REUDP_PEER_INFO_H_

/*
 * @file    peer_info
 * @date    Apr 10, 2005
 * @author  Arto Jalkanen
 * @brief   Stores information about reudp peer
 * 
 * Contains information needed by reudp for reliable datagram transfer
 * to one peer. Currently, only the running sequence number is stored.
 *  
 */
#include "common.h"

namespace reudp {
    class peer_info {
        // Current sequence number used for sending
        reudp::uint32_t  _sequence;
        time_value_type  _ping;  // not used yet
    public:
        inline peer_info()                          : _sequence(0) {}
        inline peer_info(reudp::uint32_t start_seq) : _sequence(start_seq) {}
                
        /// Returns sequence number for the operation given as parameter
        inline reudp::uint32_t sequence() const { return _sequence; }
        /// Sets current sequence number
        inline void sequence(reudp::uint32_t s) { _sequence = s; }
        /// Add to the sequence number
        inline void sequence_add(int a = 1) { _sequence += a; }
    };
}


#endif //_REUDP_PEER_INFO_H_
