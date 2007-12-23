#include <ace/OS.h>
#include <string.h>
#include <iostream>

#include "../reudp/common.h"
#include "../reudp/ack_resend_strategy.h"
#include "../reudp/strategy/timeout/constant.h"
#include "../reudp/strategy/peer_container/peer_container_nop.h"

#include <UnitTest++.h>

typedef reudp::strategy::timeout::constant test_timeout_strategy;
typedef reudp::strategy::peer_container::peer_container_nop<test_timeout_strategy::peer_struct> test_peer_container;
SUITE(ack_resend_strategy_constant) {
    // Include the common tests for constant timeout
    #include "test_ack_resend_strategy.inc.h"
}
