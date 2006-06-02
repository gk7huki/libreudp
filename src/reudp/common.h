#ifndef _REUDP_COMMON_H_
#define _REUDP_COMMON_H_

#include <ace/Log_Msg.h>
#include <ace/Addr.h>
#include <ace/INET_Addr.h>
#include <ace/Time_Value.h>
#include "message_block.h"

/**
 * @file    common.h
 * @date    Apr 9, 2005
 * @author  Arto Jalkanen
 * @brief   Some common definitions for reudp
 *
 */

#define REUDP_VERSION 0

namespace reudp {
    typedef message_block     msg_block_type;       
    typedef ACE_Addr          addr_type;
    typedef ACE_INET_Addr     addr_inet_type;
    typedef ACE_Time_Value    time_value_type;
    typedef ACE_UINT32        uint32_t;
    typedef ACE_Byte          byte_t;
    
    // Packet done (timeout/successfull send usually) callback
    typedef int (*packet_done_cb_type)(int, void  *param,
                                       const void *buf, 
                                       size_t n,
                                       const addr_type &addr);
    
    namespace packet_done {
        static const int success = 1;
        static const int timeout = 2;
        static const int failure = 3;
    }
    
    // various error codes
    namespace error_code {
        enum {
            out_of_memory   = 1,
            invalid_dgram   = 2,
            call_error      = 3,
            internal        = 4,            
        };
    }
}

#endif //_REUDP_COMMON_H_
