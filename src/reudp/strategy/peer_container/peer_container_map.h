#ifndef REUDP_STRATEGY_PEER_CONTAINER_MAP_H
#define REUDP_STRATEGY_PEER_CONTAINER_MAP_H

#include <map>

#include "../../common.h"
#include "peer_container_base.h"

namespace reudp {
namespace strategy {
namespace peer_container {
    /**
     * A peer container backed by map
     * 
     * Peer container is a class that holds information for
     * each peer. It provides basic functions to retrieve and
     * store that information given peer's address.
     * 
     */
    template <class T>
    class peer_container_map : public peer_container_base<T> {
        typedef std::map<addr_inet_type, T> _container_type;
        _container_type _container;
    public:
        bool has_value(const addr_inet_type &addr)
        {
            return _container.count(addr) > 0;
        }
        
        T &operator[](const addr_inet_type &addr)
        {
            return _container[addr];
        }
        void set_value(const addr_inet_type &addr, const T &value)
        {
            _container[addr] = value;
        }
    };
} // ns peer_container
} // ns strategy
} // ns reudp

#endif /*REUDP_STRATEGY_PEER_CONTAINER_BASE_H*/
