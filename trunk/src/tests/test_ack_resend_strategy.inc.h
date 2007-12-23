// Used for tests needing simulated passing of time
class my_configurator : public reudp::ack_resend_configurator {
public:
    reudp::time_value_type use_time;
    bool custom_time;
    
    inline my_configurator() : use_time(1000), custom_time(false) {}

    inline reudp::time_value_type gettimeofday() { 
        if (custom_time) {
            return use_time;
        }
        return reudp::ack_resend_configurator::gettimeofday(); 
    }
};

// RAII class for restoring configurator
class configurator_restore {
    my_configurator &obj;
    my_configurator state;
public:
    inline configurator_restore(my_configurator &o) : obj(o), state(o) {}
    inline ~configurator_restore() { obj = state; }
};

typedef reudp::ack_resend_strategy<
    test_timeout_strategy,
    test_peer_container,
    my_configurator
> strategy_type;

using reudp::ack_resend_strategy;

ssize_t 
simulate_send(
    strategy_type &t,
    const char *buf, 
    const reudp::addr_inet_type &addr,
    bool fail = false,
    bool gen_seq_number = true,
    uint32_t seq = 0,
    int dgram_type = strategy_type::dgram_user)
{
    strategy_type::aux_data ad;
    t.dgram_new(&ad, dgram_type, addr);
    
    if (!gen_seq_number) {
        ad.sequence = seq;
    }
    
    ssize_t buf_size = strlen(buf);
    
    if (fail)
        return t.send_failed(buf, buf_size, addr, ad);
        
    return t.send_success(buf, buf_size, addr, ad);    
}

inline ssize_t 
simulate_send_success(
    strategy_type &t,
    const char *buf, 
    const reudp::addr_inet_type &addr,
    bool gen_seq_number = true,
    uint32_t seq = 0,
    int dgram_type = strategy_type::dgram_user)
{
    return simulate_send(t, buf, addr, false, gen_seq_number, seq, dgram_type);
}

inline ssize_t 
simulate_send_fail(
    strategy_type &t,
    const char *buf, 
    const reudp::addr_inet_type &addr,
    bool gen_seq_number = true,
    uint32_t seq = 0,
    int dgram_type = strategy_type::dgram_user)
{
    return simulate_send(t, buf, addr, true, gen_seq_number, seq, dgram_type);
}

ssize_t
simulate_recv(
    strategy_type &t,
    const char *buf, 
    const reudp::addr_inet_type &addr,
    uint32_t seq) 
{
    strategy_type::aux_data ad;    
    ad.type_id  = strategy_type::dgram_user;
    ad.sequence = seq;
    
    return t.received(buf, strlen(buf), addr, ad);    
}

ssize_t
simulate_recv_ack(
    strategy_type &t,
    const reudp::addr_inet_type &addr,
    uint32_t seq) 
{
    strategy_type::aux_data ad;    
    ad.type_id  = strategy_type::dgram_ack;
    ad.sequence = seq;
    
    return t.received(NULL, 0, addr, ad);    
}

TEST(init) {
    strategy_type t;
    CHECK(t.queue_send_empty());
    CHECK_EQUAL(0U, t.queue_pending());
    CHECK_EQUAL(reudp::time_value_type::max_time, t.queue_send_when());
}

TEST(dgram_new) {
    strategy_type t;
    strategy_type::aux_data ad;
    reudp::addr_inet_type addr(80, INADDR_LOOPBACK);
    
    t.dgram_new(&ad, strategy_type::dgram_user, addr);    
    CHECK_EQUAL(strategy_type::dgram_user, ad.type_id);
    CHECK_EQUAL(0U, ad.sequence);

    t.dgram_new(&ad, strategy_type::dgram_user, addr);    
    CHECK_EQUAL(strategy_type::dgram_user, ad.type_id);
    CHECK_EQUAL(1U, ad.sequence);

    t.dgram_new(&ad, strategy_type::dgram_ack, addr);    
    CHECK_EQUAL(strategy_type::dgram_ack, ad.type_id);
    CHECK_EQUAL(2U, ad.sequence);
}

TEST(send_success_user) {
    strategy_type t;
    reudp::addr_inet_type addr(80, INADDR_LOOPBACK);
    
    ssize_t bytes = simulate_send_success(t, "1234", addr); 
    
    CHECK_EQUAL(4, bytes);
    // Now there should be something in the send queue, with
    // next retry in future (around 2 seconds)
    reudp::time_value_type now  = ACE_OS::gettimeofday();  
    reudp::time_value_type diff = t.queue_send_when() - now;
    
    CHECK(t.queue_send_empty());
    CHECK_EQUAL(1U, t.queue_pending());
    /*CHECK_CLOSE(diff, 
                reudp::time_value_type(2), 
                reudp::time_value_type(0, 500 * 1000));
    */
    // Then do another
    bytes = simulate_send_success(t, "1234", addr); // , false, 1);        
    CHECK_EQUAL(4, bytes);
    CHECK(t.queue_send_empty());
    CHECK_EQUAL(2U, t.queue_pending());    
}

TEST(send_failed_user) {
    strategy_type t;
    reudp::addr_inet_type addr(80, INADDR_LOOPBACK);
    strategy_type::aux_data ad;

    ssize_t bytes = simulate_send_fail(t, "1234", addr);    
        
    CHECK_EQUAL(-1, bytes);
    CHECK(t.queue_send_empty());
    CHECK_EQUAL(0U, t.queue_pending());
    
    // Special case if sending of the packet would be blocked,
    // then ack_resend saves the data to be sent later.
    // send_failed returns then bytes as if it was success.
    ACE_OS::last_error(EWOULDBLOCK);    
    bytes = simulate_send_fail(t, "1234", addr);    
    
    CHECK_EQUAL(4, bytes);
    CHECK(!t.queue_send_empty());
    CHECK_EQUAL(1U, t.queue_pending());         
}

TEST(received_user) {
    strategy_type t;
    reudp::addr_inet_type addr(80, INADDR_LOOPBACK);
        
    ssize_t bytes = simulate_recv(t, "1234", addr, 10);     
    CHECK_EQUAL(4, bytes);
    // At least ack should be in send queue.
    CHECK(!t.queue_send_empty());
    CHECK_EQUAL(1U, t.queue_pending());

    bytes = simulate_recv(t, "1234", addr, 10);     
    CHECK_EQUAL(4, bytes);
    // At least acks should be in send queue.
    CHECK(!t.queue_send_empty());
    CHECK_EQUAL(2U, t.queue_pending());
}

    
TEST(received_ack) {
    strategy_type t;
    reudp::addr_inet_type addr(80, INADDR_LOOPBACK);

    // First simulate sending of the datagram
    simulate_send_success(t, "1234", addr, false, 1);    
    ssize_t bytes = simulate_recv_ack(t, addr, 1);

    CHECK_EQUAL(0, bytes);
    CHECK_EQUAL(1U, t.queue_purge_timeout());
    CHECK(t.queue_send_empty());
    CHECK_EQUAL(0U, t.queue_pending());

    // Test with multiple senders using same sequence number
    std::vector<reudp::addr_inet_type> addrs(3);
    addrs[0] = reudp::addr_inet_type("111.111.111.111:80");
    addrs[1] = reudp::addr_inet_type("222.222.222.222:80");
    addrs[2] = reudp::addr_inet_type("333.333.333.333:80");
    
    for (size_t seq = 0; seq < 3; ++seq) {
        // First simulate sending of the datagram
        simulate_send_success(t, "1234", addrs[seq], false, seq);    
    }

    // First test that receiving an invalid sequence ack does nothing
    bytes = simulate_recv_ack(t, addrs[0], 10);
    CHECK_EQUAL(0U, t.queue_purge_timeout());
    CHECK(t.queue_send_empty());
    CHECK_EQUAL(3U, t.queue_pending());
    
    // Acks for first and last
    simulate_recv_ack(t, addrs[0], 0);
    simulate_recv_ack(t, addrs[2], 2);

    CHECK_EQUAL(2U, t.queue_purge_timeout());
    CHECK(t.queue_send_empty());
    CHECK_EQUAL(1U, t.queue_pending());
}

TEST(queue_send_empty) {
    reudp::addr_inet_type addr(80, INADDR_LOOPBACK);

    {   
        // Test that sends go to queue after timeout
        strategy_type t;
        configurator_restore g(t.configurator());

        t.configurator().custom_time = true;
        
        simulate_send_success(t, "1234", addr); 
        CHECK(t.queue_send_empty());
        t.configurator().use_time += reudp::time_value_type(10);     
        CHECK(!t.queue_send_empty());
    }
    {   
        // Test that acks go to queue
        strategy_type t;
        simulate_recv(t, "1234", addr, 1); 
        CHECK(!t.queue_send_empty());        
    }
    {   
        // Test that failed sends go to queue
        strategy_type t;
        simulate_send_fail(t, "1234", addr, 1); 
        CHECK(!t.queue_send_empty());        
    }    
        
}

struct packets_fixture : public UnitTest::Test {
    // Ugly
    UnitTest::TestResults &testResults_;
    
    std::map<const char *, reudp::addr_inet_type> addr;
    std::map<const char *, const char *> data;
    packets_fixture(const char *test_name, UnitTest::TestResults &tr) 
        : UnitTest::Test(test_name),
          testResults_(tr) {
        addr["ack1"] = reudp::addr_inet_type("111.111.111.111:80"); 
        addr["ack2"] = reudp::addr_inet_type("111.222.222.222:80"); 
        data["ack1"] = "recvdata1";
        data["ack2"] = "recvdata2";
        
        addr["blk1"] = reudp::addr_inet_type("122.111.111.111:80"); 
        addr["blk2"] = reudp::addr_inet_type("122.222.222.222:80"); 
        data["blk1"] = "blkdata1";
        data["blk2"] = "blkdata2";

        addr["snd1"] = reudp::addr_inet_type("133.111.111.111:80"); 
        addr["snd2"] = reudp::addr_inet_type("133.222.222.222:80"); 
        data["snd1"] = "senddata1";
        data["snd2"] = "senddata2";
    }
    void check_queue_send_front(
        strategy_type &t,
        const reudp::addr_inet_type &e_addr,
        const char *e_buf,
        uint32_t e_seq) 
    {
        CHECK(!t.queue_send_empty());
        
        const void *buf;
        size_t n;
        const reudp::addr_type *addr;
        strategy_type::aux_data ad;
        
        t.queue_send_front(&buf, &n, &addr, &ad);

        std::cerr << "e_addr: " << e_addr.get_host_addr() << std::endl;
        std::cerr << "addr  : " << dynamic_cast<const reudp::addr_inet_type *>(addr)->get_host_addr() << std::endl;        
        CHECK_EQUAL(e_addr.get_host_addr(), 
                    dynamic_cast<const reudp::addr_inet_type *>(addr)->get_host_addr());    
        // Assume ack expected to be obtained from send queue if buf is NULL
        if (e_buf == NULL) {
            CHECK_EQUAL(strategy_type::dgram_ack, ad.type_id);
        } else {
            std::string e_bufstr(e_buf);
            std::string bufstr(reinterpret_cast<const char *>(buf), n);
            CHECK_EQUAL(e_bufstr, bufstr);
            CHECK_EQUAL(strategy_type::dgram_user, ad.type_id);
        }    
        CHECK_EQUAL(e_seq, ad.sequence);
        
        // Do send_sucess for the obtained item so that it won't
        // be gotten for subsequent queue_send_front
        CHECK_EQUAL(n, (size_t)t.send_success(buf, n, *addr, ad));
    }
};

TEST(queue_send_front) {
    std::cerr << "***** queue_send_front ******" << std::endl;
    packets_fixture f(m_details.testName, testResults_);

    // Test everything oacjet type can be obtained from
    // the send queue 
    strategy_type t;
    my_configurator &c = t.configurator();
    configurator_restore g(c);
    t.configurator().custom_time = true;

    // Acks to received data go to send queue in correct order
    simulate_recv(t, f.data["ack1"], f.addr["ack1"], 1); 
    simulate_recv(t, f.data["ack2"], f.addr["ack2"], 1); 
    
    // Sends that would block go to send queue in correct order
    ACE_OS::last_error(EWOULDBLOCK);
    simulate_send_fail(t, f.data["blk1"], f.addr["blk1"]);
    simulate_send_fail(t, f.data["blk2"], f.addr["blk2"]);
    ACE_OS::last_error(0);
    
    // Sends go to queue after timeout, in correct order
    simulate_send_success(t, f.data["snd1"], f.addr["snd1"]); 
    simulate_send_success(t, f.data["snd2"], f.addr["snd2"]);
    // To cause the above packets to timeout 
    t.configurator().use_time += reudp::time_value_type(10);     

    // Now check everything would be sent in correct order
    // from:
    // 1. Acks, 2. Resends
    f.check_queue_send_front(t, f.addr["ack1"], NULL, 1U);
    f.check_queue_send_front(t, f.addr["ack2"], NULL, 1U);

    f.check_queue_send_front(t, f.addr["blk1"], f.data["blk1"], 0U);
    f.check_queue_send_front(t, f.addr["blk2"], f.data["blk2"], 1U);

    f.check_queue_send_front(t, f.addr["snd1"], f.data["snd1"], 2U);
    f.check_queue_send_front(t, f.addr["snd2"], f.data["snd2"], 3U);

    // And now the queue should be empty
    CHECK(t.queue_send_empty());
    std::cerr << "***** /queue_send_front ******" << std::endl;
    
}

// Tests packet timeouts gracefully
TEST(packet_timeout) {
    packets_fixture f(m_details.testName, testResults_);

    strategy_type t;
    my_configurator &c = t.configurator();
    configurator_restore g(c);
    c.custom_time = true;
    
    simulate_send_success(t, f.data["snd1"], f.addr["snd1"]);
    // Then let time pass and emulate resending without
    // getting ack. Advance simulated time by one
    // second at a time
    size_t num_sends = 1;
    reudp::time_value_type start_time = c.use_time;
    for (; t.queue_pending() > 0 && 
           c.use_time  < start_time + reudp::time_value_type(30); 
           c.use_time += reudp::time_value_type(1)) 
    {
        if (!t.queue_send_empty()) {
            f.check_queue_send_front(t, f.addr["snd1"], f.data["snd1"], 0U);
            num_sends++;
        } 
    }    
    
    CHECK_EQUAL(reudp::config::send_try_count(), num_sends);
}
