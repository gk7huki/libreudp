#ifndef REUDP_STRATEGY_TIMEOUT_JACOBSONKARN_H
#define REUDP_STRATEGY_TIMEOUT_JACOBSONKARN_H

#include <map>
#include <iostream>

#include "../../common.h"
#include "../../config.h"
#include "../../dgram_send_info.h"

namespace reudp {
namespace strategy {
namespace timeout {
    // Impemented as specified in rfc2988:
    // http://www.faqs.org/rfcs/rfc2988.html
    class jacobson_karn {
    public:    
        static const int32_t rto_min = 1000; // 1sec
        static const int32_t rto_max = 32000; // 32sec
        struct peer_struct {
            static const int32_t rto_def    = 3000U;
            static const int32_t srtt_def   = 0U;
            static const int32_t rttvar_def = 750U; // 750ms
            int32_t rto;
            int32_t srtt;
            int32_t rttvar;
            int32_t rttdelta;
            bool first;
            
            peer_struct() : 
                rto(rto_def),
                srtt(srtt_def),
                rttvar(rttvar_def),
                first(true)
            {}
        };
    private:
        typedef std::map<addr_inet_type, peer_struct> _peers_type;
        _peers_type _peers;
        
    public:
        inline time_value_type next_resend_time(
            const time_value_type &now,
            const dgram_send_info &/*si*/,
            peer_struct &peer_struct
        ) {
            return now + time_value_type(0, peer_struct.rto * 1000);                       
        }
        inline uint32_t send_try_count(peer_struct &/*ps*/) {
            // By default return the value from config
            return config::send_try_count();
        }
        inline void packet_sent(
            const time_value_type &now,
            const dgram_send_info &si,
            peer_struct &peer_struct
        ) {}
        
        // Called when an ack is received for existing peer.
        inline void ack_received(
            const time_value_type &now,
            const dgram_send_info &si,
            peer_struct &p
        ) {
            // Karn's algorithm instructs us to handle the
            // retransimission ambiguity problem by only
            // taking into account reply's RTT if 
            // only one packet has been sent. If more than
            // one packet sent, then we can not know which
            // one the ack is for, so it is discarded.
            if (si.send_count() > 1) {
                return;
            }
            int32_t rtt   = (now - si.base_time()).msec();
            if (p.first) {
            	p.first = false;
            	p.srtt = rtt;
            	p.rttvar = rtt >> 1;
            } else {
	            p.rttvar += (abs(p.srtt - rtt) - p.rttvar) >> 2;
	            p.srtt   += (rtt - p.srtt) >> 3;
            }
            p.rto = p.srtt + (p.rttvar << 2);
        	p.rto = std::max(rto_min, p.rto);
            p.rto = std::min(rto_max, p.rto);

            ACE_DEBUG((LM_DEBUG,
                       "rtt   : %d\n"
                       "rttvar: %d\n"
                       "srtt  : %d\n"
                       "rto   : %d\n",
                       rtt, p.rttvar, p.srtt, p.rto));
        }                
    }; // jacobson_karn
} // ns timeout
} // ns strategy
} // ns reudp

#endif /*REUDP_STRATEGY_TIMEOUT_JACOBSONKARN_H*/
