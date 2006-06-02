#ifndef _REUDP_ACK_RESEND_STRATEGY_H_
#define _REUDP_ACK_RESEND_STRATEGY_H_

#include <map>
#include <deque>
#include <queue>
#include <ace/OS.h>

#include "common.h"
#include "config.h"
#include "peer_info.h"
#include "dgram_send_info.h"

namespace reudp {   
    /**
     * @brief Resending strategy that can be used with seqack_adapter
     * 
     * Handles receiving of acks and resending if necessary sent
     * datagrams.
     * 
     * Has the following properties:
     * - datagrams are resent until an ack is received
     * - the order in which datagrams arrive is not necessarily the
     *   order in which they were sent
     * - duplicate datagrams are possible
     * - timeout of datagrams is constant for now (2 seconds)
     */
    class ack_resend_strategy {
    public:
        // These two required by seqack_adapter
        static const int dgram_user   = 0;
        static const int dgram_ack    = 1;
        // These are internal states
        static const int mask_resend  = 0xF;
        
        typedef int      dgram_type;

        struct aux_data {
            dgram_type type_id;
            uint32_t   sequence;
            // TODO timestamp;
            int        type_mask; // set if resend
            aux_data() : type_id(0), sequence(0), type_mask(0) {}
        };
        
    private:
        // Since this resend strategy does not even try to guarantee
        // packets arrive at the right order, the used sequence number
        // is shared. Thus, only one peer_info structure needed.
        // Since sequence number is shared amongst all peers, it is
        // a unique identifier for the sent datagrams and simplifies
        // finding the datagrams from different queues and maps
        peer_info _peer_info;
    
        typedef std::map<uint32_t, dgram_send_info> dgram_send_info_map_type;
        dgram_send_info_map_type               _dgram_send_info_map;
        
        // For now timeout is the same (2 secs)
        // for every host. TODO calculate this dynamically
        // between each host.
        // time_value_type _timeout;
        
        struct ack_data {
            aux_data       ad;
            addr_inet_type addr;
        };
        struct timeout_data {
            uint32_t        sequence;
            time_value_type when;
            inline bool operator<(const timeout_data &rd) const {
                return when > rd.when;
            }
        };
        
        // differend send queues.
        // _queue_ack    : acks to be sent to peer
        // _queue_send   : user datagrams that are waiting to be sent
        // _queue_timeout: ordered queue of sent datagrams that are scheduled
        //                 for timeout detection. Those that timeout are moved 
        //                 to _queue_send for retransmit
        typedef std::priority_queue<timeout_data> queue_timeout_type;
        std::deque<ack_data>    _queue_ack;
        std::deque<uint32_t>    _queue_send;
        queue_timeout_type _queue_timeout;
        
        bool _queue_ack_front(const void      **buf,
                              size_t           *n,
                              const addr_type **addr,
                              aux_data   *ad);
        bool _queue_send_front(const void      **buf,
                               size_t           *n,
                               const addr_type **addr,
                               aux_data   *ad);
        dgram_send_info &_create_send_info(const void      *buf,
                                           size_t           n,
                                           const addr_type &addr,
                                           const aux_data  &ad);

        inline 
        void _queue_timeout_push(uint32_t seq);

        packet_done_cb_type _packet_done_cb;
        void *_packet_done_par; 
        inline void _do_packet_done(int t, const void *buf, size_t n,
                                    const addr_type &addr);
        
    public:     
        ack_resend_strategy();
        ~ack_resend_strategy();
        
        /* start of interface required by seqack_adapter */
        void dgram_new(aux_data *ad, dgram_type t, const addr_type &addr);
        ssize_t send_success(const void      *buf,
                             size_t           n,
                             const addr_type &addr,
                             const aux_data  &ad);
                             
        ssize_t send_failed(const void      *buf,
                            size_t           n,
                            const addr_type &addr,
                            const aux_data  &ad);
                             
        ssize_t received(const void      *buf,
                         size_t           n,
                         const addr_type &addr_from,
                         const aux_data  &ad);

        inline bool   queue_send_empty();
        inline size_t queue_pending() const;
        inline time_value_type queue_send_when() const;

        /// Fills in addresses to the first item from the send queue
        /// for resending
        bool   queue_send_front(const void      **buf,
                                size_t           *n,
                                const addr_type **addr,
                                aux_data         *ad);
                                
        inline void packet_done_cb(packet_done_cb_type cb, void *param);
        // Clears the resend queues etc.
        void reset();
        /* end of interface required by seqack_adapter */   

        ssize_t send_success_ack(const void      *buf,
                                 size_t           n,
                                 const addr_type &addr,
                                 const aux_data  &ad);
        ssize_t send_success_user(const void      *buf,
                                  size_t           n,
                                  const addr_type &addr,
                                  const aux_data  &ad);
        ssize_t send_success_resend(const void      *buf,
                                    size_t           n,
                                    const addr_type &addr,
                                    const aux_data  &ad);

        ssize_t received_ack(const void           *buf,
                             size_t                n,
                             const addr_inet_type &addr_from,
                             const aux_data       &ad);
        ssize_t received_user(const void           *buf,
                             size_t                n,
                             const addr_inet_type &addr_from,
                             const aux_data       &ad);
                
        // size_t size_queue_send(); 
    };
    
    inline bool
    ack_resend_strategy::queue_send_empty() {
        if (_queue_ack.size() > 0) return false;

        time_value_type now = ACE_OS::gettimeofday();       
        while (_queue_timeout.size() > 0) {
            const timeout_data &td = _queue_timeout.top();
            // If the inspected element timed out and then add the
            // sequence to sending queue and remove it from
            // timeout queue.
            ACE_DEBUG((LM_DEBUG, "%Inext timeout in %ds%dus, now %ds%dus (seq %u)\n",
                      td.when.sec(), td.when.usec(), 
                      now.sec(), now.usec(), td.sequence));
            if (td.when <= now) {
                _queue_send.push_back(td.sequence);
                _queue_timeout.pop();
            } else {
                break;
            }
        }

        while (_queue_send.size() > 0) {
            uint32_t seq = _queue_send.front();
            // If the sequence still exists in send dgram map,
            // then it can be sent again so return immediately
            // false. Otherwise discard elements until send queue
            // is exhausted.
            if (_dgram_send_info_map.count(seq)) {
                const dgram_send_info &si = _dgram_send_info_map[seq];
                // if (si.send_count() < 3)
                if (si.send_count() < config::send_try_count())
                    return false;
                ACE_DEBUG((LM_DEBUG, "%Idgram %d has been resent %d times " \
                                     "without reply, giving up\n",
                                     seq, si.send_count()));
                // TODO maybe pass on the data to the callback too.
                _do_packet_done(packet_done::timeout, NULL, 0, si.addr());
            }
            _queue_send.pop_front();
        }
        
        return true;
    }

    inline time_value_type 
    ack_resend_strategy::queue_send_when() const {
        if (_queue_timeout.empty())
            return time_value_type::max_time;
        return _queue_timeout.top().when;
    }
    
    inline size_t
    ack_resend_strategy::queue_pending() const {
        return _queue_timeout.size() + _queue_send.size();
    }

    inline void 
    ack_resend_strategy::packet_done_cb(packet_done_cb_type cb, void *param) {
        _packet_done_cb  = cb;
        _packet_done_par = param;
        ACE_DEBUG((LM_DEBUG, "ack_resend_strategy: setting packet_done " \
                  "cb/par to %d/%d\n", _packet_done_cb, _packet_done_par));
    }

    inline void
    ack_resend_strategy::_do_packet_done(int t, const void *buf, size_t n,
                                         const addr_type &addr)
    {
        if (_packet_done_cb) {
            ACE_DEBUG((LM_DEBUG, "ack_resend_strategy: calling packet_done " \
                      "callback for ptr %d\n", _packet_done_cb));
            _packet_done_cb(t, _packet_done_par, buf, n, addr);
        }
    }
    
    inline void
    ack_resend_strategy::_queue_timeout_push(uint32_t seq) {
            // add this datagram to timeout queue
        timeout_data td;
        td.sequence = seq;
        td.when     = ACE_OS::gettimeofday();
        // td.when    += _timeout;
        td.when    += config::timeout(); // _timeout;
                
        _queue_timeout.push(td);
        
        ACE_DEBUG((LM_DEBUG, "%Iadded seq %u to timeout queue (size %d), " \
                             "%u ms from now\n", td.sequence,
                             _queue_timeout.size(), config::timeout().msec()));
    }

}

#endif //_ACK_RESEND_STRATEGY_H_
