#ifndef REUDP_STRATEGY_PEER_CONTAINER_BASE_H
#define REUDP_STRATEGY_PEER_CONTAINER_BASE_H

#include "../../common.h"
#include "../../config.h"

namespace reudp {
namespace strategy {
namespace peer_container {
    /**
     * Base class for peer container strategies.
     * 
     * Peer container is a class that holds information for
     * each peer. It provides basic functions to retrieve and
     * store that information given peer's address.
     * 
     */
    template <class T>
    class peer_container_base {
        T _shared_peer;
    public:
        bool has_value(const addr_inet_type &addr) {
            return false;
        }
        
        T &operator[](const addr_inet_type &addr) {
            return _shared_peer;
        }
        void set_value(const addr_inet_type &addr, const T &value)
        {}
    };
} // ns peer_container
} // ns strategy
} // ns reudp

#endif /*REUDP_STRATEGY_PEER_CONTAINER_BASE_H*/
