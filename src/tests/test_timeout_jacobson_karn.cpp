#include <UnitTest++.h>
#include <ace/OS.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <map>
#include "../reudp/strategy/timeout/jacobson_karn.h"
#include "../reudp/dgram_send_info.h"
#include "../reudp/config.h"

using namespace reudp;
using namespace reudp::strategy;

SUITE(timeout_jacobson_karn) {

struct fixture_addr {
    typedef std::vector<addr_inet_type> addr_container_type;
    typedef std::map<
        addr_inet_type, 
        timeout::jacobson_karn::peer_struct
    > peer_struct_container_type;
    addr_container_type addr;
    peer_struct_container_type ps_map;
    timeout::jacobson_karn tjk;
    time_value_type now;
    dgram_send_info si;
    
    fixture_addr() {
        addr.push_back(addr_inet_type("111.111.111.111:80"));
        addr.push_back(addr_inet_type("121.111.111.111:81"));
        addr.push_back(addr_inet_type("131.111.111.111:81"));
        // Set the basetime to something other than 0 
        now.set(123, 123456);
        si = create_send_info(addr[0], now);
    }
    
    dgram_send_info &create_send_info(
        const addr_inet_type &addr,
        const time_value_type &now
    ) {
        si.addr(addr);
        si.base_time(now);
        return si;
    } 
    
};

// Checks that the initial retransimission timeout is what expected.
// rto is in microseconds
TEST_FIXTURE(fixture_addr, initial_rto) {
    CHECK_EQUAL(3000, timeout::jacobson_karn::peer_struct().rto); // rto(addr[0]));
}

// srtt is in microseconds
TEST_FIXTURE(fixture_addr, initial_srtt) {
    CHECK_EQUAL(0, timeout::jacobson_karn::peer_struct().srtt);
}

// Checks that the initial smoothed mean deviation estimator is what expected
TEST_FIXTURE(fixture_addr, initial_rttvar) {
    // .75 seconds = 750 milliseconds  
    CHECK_EQUAL(750, timeout::jacobson_karn::peer_struct().rttvar);
}

// Checks that the retransmission timeout doubles after each packet timeout
TEST_FIXTURE(fixture_addr, rto_doubles) {
    const timeout::jacobson_karn::peer_struct &ps = ps_map[addr[0]];
    int32_t rto_initial = ps.rto;
    // Simulate a packet miss
    tjk.send_timeout(now, si, ps_map[addr[0]]);
    // .75 seconds = 750 milliseconds  
    CHECK_EQUAL(rto_initial * 2, ps.rto);
}

// Checks that the retransmission timeout doubling doesn't exceed bounds
TEST_FIXTURE(fixture_addr, rto_doubles_limitd_by_max) {    
    const timeout::jacobson_karn::peer_struct &ps = ps_map[addr[0]];
    int32_t rto_previous = 0;
    do {
        rto_previous = ps.rto;
        // Simulate a packet miss
        tjk.send_timeout(now, si, ps_map[addr[0]]);
        CHECK(ps.rto <= timeout::jacobson_karn::rto_max);
    } while (rto_previous < ps.rto && ps.rto < timeout::jacobson_karn::rto_max);
}

TEST_UTILITY(until_ack,\
    (fixture_addr &f,
     const reudp::time_value_type &start,
     int32_t advance_msec
     )   
) {
    timeout::jacobson_karn &tjk = f.tjk;
    const reudp::dgram_send_info &si = f.si;
    timeout::jacobson_karn::peer_struct &ps = f.ps_map[si.addr()];
      
    time_value_type now = start;
    tjk.packet_sent(now, si, ps);

    int32_t  srtt = ps.srtt;
    int32_t  rttvar = ps.rttvar;
    int32_t  rtt = advance_msec;
    bool     update_done = !ps.first;
    
    now += time_value_type(0, rtt * 1000);
    tjk.ack_received(now, si, ps);

    int32_t exp_srtt;
    int32_t exp_rttvar;
    int32_t exp_rto;
    if (!update_done) {
        exp_srtt   = rtt;
        exp_rttvar = rtt / 2;
        exp_rto    = exp_srtt + 4 * exp_rttvar;        
    } else {
        exp_rttvar = rttvar * 3 / 4 + (srtt - rtt) / 4;
        exp_srtt   = srtt * 7 / 8 + rtt / 8;
        exp_rto    = exp_srtt + 4 * exp_rttvar;
    }
    exp_rto = std::max(timeout::jacobson_karn::rto_min, exp_rto);
    exp_rto = std::min(timeout::jacobson_karn::rto_max, exp_rto);
    CHECK_CLOSE(exp_rttvar, ps.rttvar, 5); 
    CHECK_CLOSE(exp_srtt,   ps.srtt,   5); 
    CHECK_CLOSE(exp_rto,    ps.rto,    5);
}

TEST_FIXTURE(fixture_addr, rto_rtt1) {
    TEST_UTILITY_FUNC(until_ack)(*this, now, 1000);
    // The ack is received one second after transmission,
    // check that the retransmission is in reasonable
    // bounds knowing *this and that rvtt startup value is
    // 3000 (or REUDP_TIMEOUT_JK)

    CHECK_CLOSE(3000, ps_map[si.addr()].rto, 500); 
}

// Check that RTO approaches the real due time RTT
TEST_FIXTURE(fixture_addr, rto_rtt_approach) {
    int32_t prev = 0;
    std::cout << "\n **** START" << std::endl;
    for (unsigned i = 0; i < 10; i++) {
        create_send_info(addr[0], now);
        TEST_UTILITY_FUNC(until_ack)(*this, now, 1000);
        int32_t rto = ps_map[si.addr()].rto;
        if (prev != 0) {        
            CHECK(rto <= prev);
        }
        prev = rto;
        now += time_value_type(0, 1000 * 1000);
    }
    std::cout << "/START" << std::endl;     
}

TEST_FIXTURE(fixture_addr, rto_rtt1_1) {
    TEST_UTILITY_FUNC(until_ack)(*this, now, 1000);
}

/*
TEST_FIXTURE(fixture_addr, rto_rtt2) {
    dgram_send_info si1 = create_send_info(addr[0], now);
    dgram_send_info si2 = create_send_info(addr[1], now);
    
    TEST_UTILITY_FUNC(until_ack)(this, now, 1000); 
    TEST_UTILITY_FUNC(until_ack)(tjk, now, 1000, si2);
}
*/

// Checks that resend is scheduled to be done 
// at expected interwalls if no ack is received
TEST_FIXTURE(fixture_addr, resend_interwalls) {
    tjk.packet_sent(now, si, ps_map[si.addr()]);
    CHECK_CLOSE((now + time_value_type(3)).msec(), 
                tjk.next_resend_time(now, si, ps_map[si.addr()]).msec(),
                time_value_type(0, 10).msec()); 
}

TEST_FIXTURE(fixture_addr, rto_minimum_1sec) {
    int32_t prev    = 0;
    int32_t min_rto = 1000;
    int32_t rtt_set = 0 * 1000;
    std::cout << "\n **** START" << std::endl;
    for (unsigned i = 0; i < 10; i++) {
        create_send_info(addr[0], now);
        TEST_UTILITY_FUNC(until_ack)(*this, now, rtt_set);
        int32_t rto = ps_map[si.addr()].rto;
        CHECK(rto >= min_rto);
        prev = rto;
        now += time_value_type(0, rtt_set);
    }
    CHECK_EQUAL(min_rto, prev);
    std::cout << "/START" << std::endl;     
}

TEST_FIXTURE(fixture_addr, rto_maximum_32sec) {
    int32_t prev    = 0;
    int32_t max_rto = 32 * 1000;
    int32_t rtt_set = 64 * 1000;
    std::cout << "\n **** START" << std::endl;
    for (unsigned i = 0; i < 10; i++) {
        create_send_info(addr[0], now);
        TEST_UTILITY_FUNC(until_ack)(*this, now, rtt_set);
        int32_t rto = ps_map[si.addr()].rto;
        CHECK(rto <= max_rto);
        prev = rto;
        now += time_value_type(0, rtt_set);
    }
    CHECK_EQUAL(max_rto, prev);
    std::cout << "/START" << std::endl;     
}

} // SUITE()
