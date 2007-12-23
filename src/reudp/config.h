#ifndef REUDP_CONFIG_H
#define REUDP_CONFIG_H

#include <algorithm>
#include "common.h"

namespace reudp {
namespace config {
    struct obj {
        time_value_type timeout;
        size_t          send_try_count;
        
        obj();
    };
    
    extern obj _obj;
    
    inline const time_value_type &timeout() { return _obj.timeout; }
    inline void timeout(const time_value_type &t) {
        _obj.timeout = t;
        // Ensure reasonable limits to the timeout (1-10 secs)
        _obj.timeout.sec(std::max<size_t>(_obj.timeout.sec(), 1));
        _obj.timeout.sec(std::min<size_t>(_obj.timeout.sec(), 10));
        
        ACE_DEBUG((LM_DEBUG, "reudp::config::timeout now %d msecs\n",
                  _obj.timeout.msec()));
    }
    inline size_t send_try_count() { return _obj.send_try_count; }
    inline void   send_try_count(size_t t) { 
        _obj.send_try_count = t;
        // Ensure reasonable limits to the try count (1-10 tries)
        _obj.send_try_count = std::max<size_t>(_obj.send_try_count, 1);
        _obj.send_try_count = std::min<size_t>(_obj.send_try_count, 10);

        ACE_DEBUG((LM_DEBUG, "reudp::config::send_try_count now %d\n",
                  _obj.send_try_count));
    }
}

} // ns reudp

#endif //_REUDP_CONFIG_H_
