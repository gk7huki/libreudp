#ifndef REUDP_DGRAM_SEND_INFO_H
#define REUDP_DGRAM_SEND_INFO_H

/*
 * @file    dgram_send_info
 * @date    Apr 10, 2005
 * @author  Arto Jalkanen
 * @brief   Stores information about sent datagram
 * 
 * Contains information needed for resending a packet. Has 
 * the sent datagram's sequence number, base timestamp value 
 * (the time when first datagram was sent) and
 * pointer and size of the datagram content.
 *  
 */
#include "common.h"

namespace reudp {
    class dgram_send_info {
        msg_block_type *_data_block;
        uint32_t        _sequence;
        uint32_t        _send_count;
        time_value_type _base_timestamp;
        addr_inet_type  _addr;
        
    public:
        dgram_send_info() : _data_block(NULL),
                            _sequence(0),
                            _send_count(0)
                            {}
                            
        ~dgram_send_info() {
            delete _data_block;
        }
        inline msg_block_type *data_block() const { return _data_block; }
        inline void            data_block(msg_block_type *db) {
            _data_block = db; 
        }
            
        inline uint32_t sequence() const     { return _sequence; }
        inline void     sequence(uint32_t s) { _sequence = s;    }

        inline uint32_t send_count() const     { return _send_count; }
        inline void     send_count(uint32_t s) { _send_count = s;    }
        inline void     send_count_add(uint32_t a=1) { _send_count += a; }
        
        inline const time_value_type &base_time() const { return _base_timestamp; }
        inline void                   base_time(const time_value_type &t) { 
            _base_timestamp = t;
        }

        inline const addr_inet_type &addr() const { return _addr; }
        inline void                  addr(const addr_inet_type &a) { 
            _addr = a;
        }

    };
}

#endif //_REUDP_DGRAM_SEND_INFO_H_
