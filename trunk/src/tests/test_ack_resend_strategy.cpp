#include <UnitTest++.h>
#include "../reudp/ack_resend_strategy.h"

TEST(JustDummyCheck) {
    reudp::ack_resend_strategy t;
    CHECK(t.queue_send_empty());
}
