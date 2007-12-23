#ifndef REUDP_STRATEGY_PEER_CONTAINER_NOP_H
#define REUDP_STRATEGY_PEER_CONTAINER_NOP_H

#include "../../common.h"
#include "peer_container_base.h"

namespace reudp {
namespace strategy {
namespace peer_container {
    /**
     * A no-operation container. Used when there is no need for
     * containing any peer specific values.
     * 
     */
    template <class T>
    class peer_container_nop : public peer_container_base<T> {
    public:        
    };
} // ns peer_container
} // ns strategy
} // ns reudp

#endif /*REUDP_STRATEGY_PEER_CONTAINER_NOP_H*/
