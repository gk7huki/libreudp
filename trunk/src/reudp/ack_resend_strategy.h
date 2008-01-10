#ifndef REUDP_ACK_RESEND_STRATEGY_H
#define REUDP_ACK_RESEND_STRATEGY_H

#include <map>
#include <deque>
#include <queue>
#include <memory>
#include <ace/OS.h>

#include "common.h"
#include "config.h"
#include "peer_info.h"
#include "dgram_send_info.h"
#include "exception.h"
#include "strategy/timeout/constant.h"
#include "strategy/peer_container/peer_container_nop.h"

namespace reudp {
    class ack_resend_configurator {
    public:
        inline time_value_type gettimeofday() {
            return ACE_OS::gettimeofday();
        } 
    };
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
     * - timeout strategy of datagrams is configured using
     *   a template strategy class.
     */
    template <class T = strategy::timeout::constant,
              class P = strategy::peer_container::peer_container_nop<typename T::peer_struct>,
              class C = ack_resend_configurator>
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
        T _strategy;
        P _peer_container;
        C _conf;
        
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
        void _queue_timeout_push(
            const dgram_send_info &si
        /*         
            const addr_inet_type &addr,
            uint32_t seq,
            uint32_t send_count */        
        );

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

        inline size_t queue_purge_timeout();
        inline bool   queue_send_empty();
        inline size_t queue_pending() const;
        inline time_value_type queue_send_when() const;

        inline C &configurator();
        inline T &strategy();
        
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

    template <class T, class P, class C> const int ack_resend_strategy<T,P,C>::dgram_user;
    template <class T, class P, class C> const int ack_resend_strategy<T,P,C>::dgram_ack;
    template <class T, class P, class C> const int ack_resend_strategy<T,P,C>::mask_resend;

    template <class T, class P, class C>   
    inline C &
    ack_resend_strategy<T,P,C>::configurator() { return _conf; }

    template <class T, class P, class C>   
    inline T &
    ack_resend_strategy<T,P,C>::strategy() { return _strategy; }
    
    // Purges from timeouted queue the packets that have received
    // ack already (sequence number not existing no more).
    // Returns the number of purged packets.
    // Using this function is not recommended, nor usually needed
    // in production usage. It is mainly useful for tests.
    // It is not too efficient because elements can not
    // be removed from priority_queue, instead a new priority_queue
    // is created with only the valid ones.
    template <class T, class P, class C>         
    size_t
    ack_resend_strategy<T,P,C>::queue_purge_timeout() {
        size_t removed = 0;
        queue_timeout_type new_queue;
        while (!_queue_timeout.empty()) {
            const timeout_data &td = _queue_timeout.top();
            if (_dgram_send_info_map.count(td.sequence)) {                
                new_queue.push(td);
            } else {
                ++removed;
            }
            _queue_timeout.pop();
        }
        
        _queue_timeout = new_queue;
        return removed;
    }
        
    template <class T, class P, class C>         
    bool
    ack_resend_strategy<T,P,C>::queue_send_empty() {
        if (_queue_ack.size() > 0) return false;

        time_value_type now = _conf.gettimeofday();       
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
                typename T::peer_struct &ps = _peer_container[si.addr()];
                _strategy.send_timeout(now, si, ps);
                
                // if (si.send_count() < 3)
                ACE_DEBUG((LM_DEBUG, "%Idgram %d has been resent %d/%d times\n",
                                     seq, si.send_count(), 
                                     _strategy.send_try_count(ps)));
                if (si.send_count() < _strategy.send_try_count(ps))
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

    template <class T, class P, class C>         
    inline time_value_type 
    ack_resend_strategy<T,P,C>::queue_send_when() const {
        if (_queue_timeout.empty())
            return time_value_type::max_time;
        return _queue_timeout.top().when;
    }
    
    template <class T, class P, class C>         
    inline size_t
    ack_resend_strategy<T,P,C>::queue_pending() const {
        return _queue_ack.size()     + 
               _queue_timeout.size() + 
               _queue_send.size();
    }

    template <class T, class P, class C>         
    inline void 
    ack_resend_strategy<T,P,C>::packet_done_cb(packet_done_cb_type cb, void *param) {
        _packet_done_cb  = cb;
        _packet_done_par = param;
        ACE_DEBUG((LM_DEBUG, "ack_resend_strategy: setting packet_done " \
                  "cb/par to %d/%d\n", _packet_done_cb, _packet_done_par));
    }

    template <class T, class P, class C>         
    inline void
    ack_resend_strategy<T,P,C>::_do_packet_done(int t, const void *buf, size_t n,
                                         const addr_type &addr)
    {
        if (_packet_done_cb) {
            ACE_DEBUG((LM_DEBUG, "ack_resend_strategy: calling packet_done " \
                      "callback for ptr %d\n", _packet_done_cb));
            _packet_done_cb(t, _packet_done_par, buf, n, addr);
        }
    }
    
    template <class T, class P, class C>         
    void
    ack_resend_strategy<T,P,C>::_queue_timeout_push(
        const dgram_send_info &si
    ) {
        // add this datagram to timeout queue
        time_value_type now = _conf.gettimeofday();
        timeout_data td;
        td.sequence = si.sequence(); // seq;
        td.when     = _strategy.next_resend_time(now, si, _peer_container[si.addr()]);
                            
        _queue_timeout.push(td);
        
        ACE_DEBUG((LM_DEBUG, "%Iadded seq %u to timeout queue (size %d), " \
                             "%u ms from now\n", td.sequence,
                             _queue_timeout.size(), 
                             (td.when - now).msec()));
    }

    template <class T, class P, class C>         
    ack_resend_strategy<T,P,C>::ack_resend_strategy() {
        // _timeout = time_value_type(2);
        _packet_done_cb  = NULL;
        _packet_done_par = NULL;
    }
    template <class T, class P, class C>         
    ack_resend_strategy<T,P,C>::~ack_resend_strategy() {
        reset();
        // todo clear maps
    }
    template <class T, class P, class C>         
    void
    ack_resend_strategy<T,P,C>::reset() {
        _dgram_send_info_map.clear();
        _queue_ack.clear();
        _queue_send.clear();
        while (_queue_timeout.size() > 0) _queue_timeout.pop();
    }
    template <class T, class P, class C>         
    void 
    ack_resend_strategy<T,P,C>::dgram_new(aux_data *ad, dgram_type t, 
                                   const addr_type &addr_to)
                                    
    {
        ACE_TRACE("reudp::ack_resend_strategy<T,P,C>::dgram_new()");
        // The addr must be cast to inet_addr, we need full IP address and
        // port for the map.
        const addr_inet_type *addr = 
            dynamic_cast<const addr_inet_type *>(&addr_to);
        if (!addr)
            throw reudp::call_error(
                "reudp::ack_resend_strategy::dgram_new():" \
                "invalid address given, must be inet addr"
            );
                    
        ad->type_id  = t;
        ad->sequence = _peer_info.sequence();

        _peer_info.sequence_add();
    }
    
    template <class T, class P, class C>         
    ssize_t 
    ack_resend_strategy<T,P,C>::send_success(const void      *buf,
                                      size_t           n,
                                      const addr_type &addr,
                                      const aux_data  &ad) 
    { 
        ACE_TRACE("reudp::ack_resend_strategy::send_success()");

        switch (ad.type_id | ad.type_mask) {
        case dgram_ack:
            return send_success_ack(buf, n, addr, ad);
        case dgram_user:
            return send_success_user(buf, n, addr, ad);
        case dgram_user|mask_resend:
            return send_success_resend(buf, n, addr, ad);
        default:
            throw reudp::unexpected_errorf(
                "unrecognized dgram type %d, mask %d",
                ad.type_id, ad.type_mask
            );
        }
    }
    
    template <class T, class P, class C>         
    ssize_t
    ack_resend_strategy<T,P,C>::send_success_ack(const void      *buf,
                                          size_t           n,
                                          const addr_type &addr,
                                          const aux_data  &ad) 
    {                  
        ACE_DEBUG((LM_DEBUG, "%Iack sent successfully, removing from queue " \
                             "sequence %u\n", ad.sequence));
        // Just a little check first
        if (ad.sequence != _queue_ack.front().ad.sequence)
            throw reudp::unexpected_errorf(
                "sequences do not match, got %u, " \
                "in queue %u",
                ad.sequence, 
                _queue_ack.front().ad.sequence
            );

        // So... the front ack can be removed
        _queue_ack.pop_front();
        
        return (ssize_t)n;
    }

    template <class T, class P, class C>         
    ssize_t
    ack_resend_strategy<T,P,C>::send_success_user(const void      *buf,
                                           size_t           n,
                                           const addr_type &addr_to,
                                           const aux_data  &ad) 
    {                  
        if (_dgram_send_info_map.count(ad.sequence) > 0)
            throw reudp::unexpected_errorf(
                "reudp::ack_resend_strategy::send_success: " \
                "sequence %u already existed", ad.sequence
            );

        dgram_send_info &si = _create_send_info(buf, n, addr_to, ad);
        si.send_count_add();        
        _queue_timeout_push(si); // si.addr(), ad.sequence, 1);
                                            
        return (ssize_t)n; 
    }

    template <class T, class P, class C>         
    ssize_t
    ack_resend_strategy<T,P,C>::send_success_resend(const void      *buf,
                                             size_t           n,
                                             const addr_type &addr,
                                             const aux_data  &ad) 
    {
        if (_dgram_send_info_map.count(ad.sequence) == 0)
            throw reudp::unexpected_errorf(
                "reudp::ack_resend_strategy::send_success_resend: " \
                "sequence %u not found fron send_info_map!", ad.sequence
            );

        dgram_send_info &si = _dgram_send_info_map[ad.sequence];        
        si.send_count_add();

        ACE_DEBUG((LM_DEBUG, "%Iincreased send count to %d for " \
                             "sequence %u\n", si.send_count(),
                             ad.sequence));
        
        _queue_timeout_push(si); // si.addr(), ad.sequence, si.send_count());
        _queue_send.pop_front();
        
        return (ssize_t)n; 
    }
                         
    template <class T, class P, class C>         
    ssize_t 
    ack_resend_strategy<T,P,C>::send_failed(
        const void      *buf,
        size_t           n,
        const addr_type &addr,
        const aux_data  &ad)
    { 
        ACE_TRACE("reudp::ack_resend_strategy::send_failed()");

        // If error was EWOULDBLOCK, do some special processing     
        int le = ACE_OS::last_error();
        
        switch (ad.type_id | ad.type_mask) {
        case dgram_ack:
            break;
        case dgram_user:
            if (le == EWOULDBLOCK) {
                // Have to add this one to the dgram send info map and send
                // queue for resending when send called next time
                ACE_DEBUG((LM_DEBUG, "%Iack_resend_strategy::send_failed " \
                           "due to EWOULDBLOCK, will try sending " \
                           "later, seq %u, send_queue size %d\n", ad.sequence,
                           _queue_send.size() + 1));
                _create_send_info(buf, n, addr, ad);
                _queue_send.push_back(ad.sequence);
                // Since stored for sending as soon as possible, let caller
                // think sending was successfull.
                return n;
            } else {
                _do_packet_done(packet_done::failure, buf, n, addr);
            }
            break; // return send_success_user(buf, n, addr, ad);
        case dgram_user|mask_resend:
            if (le == EWOULDBLOCK) {
                ACE_DEBUG((LM_DEBUG, "%Iack_resend_strategy::send_failed " \
                           "due to EWOULDBLOCK, will try resending " \
                           "later, seq %d\n", ad.sequence));
            } else {    
                // Remove from resend queue and from map
                ACE_ASSERT(_queue_send.front() == ad.sequence);
                ACE_DEBUG((LM_DEBUG, "%Iack_resend_strategy::send_failed " \
                                     "resend of seq %u failed, removing " \
                                     "from send queue and datagram info map",
                                     ad.sequence));

                _queue_send.pop_front();
                _dgram_send_info_map.erase(ad.sequence);
                _do_packet_done(packet_done::failure, buf, n, addr);
            }
            break;
        default:
            throw reudp::unexpected_errorf(
                "unrecognized dgram type %d, mask %d",
                ad.type_id, ad.type_mask
            );
        }
        return (ssize_t)-1; 
    }
                         
    template <class T, class P, class C>         
    ssize_t 
    ack_resend_strategy<T,P,C>::received(const void      *buf,
                                  size_t           n,
                                  const addr_type &addr_from,
                                  const aux_data  &ad) 
    { 
        ACE_TRACE("reudp::ack_resend_strategy::received()");
        const addr_inet_type *addr = 
            dynamic_cast<const addr_inet_type *>(&addr_from);

        if (!addr)
            throw reudp::call_error(
                "reudp::ack_resend_strategy::received():" \
                "invalid address given, must be inet addr"
            );

        switch (ad.type_id) {
        case dgram_ack:
            return received_ack(buf, n, *addr, ad);
        case dgram_user:
            return received_user(buf, n, *addr, ad);
        default:
            ACE_DEBUG((LM_WARNING, 
            "reudp::received invalid datagram with type %d, ignoring packet\n", 
            ad.type_id));
            return 0;
        }
        
        return (ssize_t)n; 
    }

    template <class T, class P, class C>         
    ssize_t 
    ack_resend_strategy<T,P,C>::received_user(const void           *buf,
                                       size_t                n,
                                       const addr_inet_type &addr,
                                       const aux_data       &ad) 
    { 
        ACE_TRACE("reudp::ack_resend_strategy::received_user()");
        
        ack_data a;
        a.ad   = ad;
        a.addr = addr;
        
        a.ad.type_id = dgram_ack;
        
        _queue_ack.push_back(a);
        ACE_DEBUG((LM_DEBUG, "%Ischeduling sending ack to %s:%u, seq %u, " \
                             "size of ack queue now %d\n",
                             a.addr.get_host_addr(),
                             a.addr.get_port_number(),
                             ad.sequence, _queue_ack.size()));
        return (ssize_t)n; 
    }

    template <class T, class P, class C>         
    ssize_t 
    ack_resend_strategy<T,P,C>::received_ack(const void           *buf,
                                      size_t                n,
                                      const addr_inet_type &addr,
                                      const aux_data       &ad) 
    { 
        ACE_TRACE("reudp::ack_resend_strategy::received_ack()");

        dgram_send_info_map_type::iterator i = 
          _dgram_send_info_map.find(ad.sequence);
          
        if (i == _dgram_send_info_map.end()) { // _dgram_send_info_map.count(ad.sequence) == 0) {
            ACE_DEBUG((LM_WARNING, "%Iack_resend_strategy::received_ack: " \
                                   "dgram_send_info not found for seq %u\n",
                                   ad.sequence));
        } else {
            const addr_inet_type &to = i->second.addr();
            _strategy.ack_received(_conf.gettimeofday(), 
                                   i->second,
                                   _peer_container[to]);
            // TODO maybe pass on the data to the callback too.
            _do_packet_done(packet_done::success, NULL, 0, to);
                        
            // TODO maybe check that received from the same address that the ack
            // was sent to, to make spoofing harder. Might cause trouble
            // with NATted nodes though?
            _dgram_send_info_map.erase(i); // ad.sequence);
            ACE_DEBUG((LM_DEBUG, "%Ireudp::ack_resend_strategy::receive_ack: " \
                                 "received ack from %s:%u, removed seq %u, " \
                                 "waiting acks for %d dgrams\n",
                                 addr.get_host_addr(),
                                 addr.get_port_number(),
                                 ad.sequence, _dgram_send_info_map.size()));
        }
        return (ssize_t)n; 
    }   
    
    template <class T, class P, class C>         
    bool
    ack_resend_strategy<T,P,C>::queue_send_front(
        const void      **buf,
        size_t           *n,
        const addr_type **addr,
        aux_data   *ad)
    {
        if (_queue_ack.size())
            return _queue_ack_front(buf, n, addr, ad);
        if (_queue_send.size())
            return _queue_send_front(buf, n, addr, ad);
        return true;
    }

    template <class T, class P, class C>         
    bool
    ack_resend_strategy<T,P,C>::_queue_ack_front(const void      **buf,
                                          size_t           *n,
                                          const addr_type **addr,
                                          aux_data   *ad)
    {
        const ack_data &a = _queue_ack.front();
        *buf  = NULL;
        *n    = 0;
        *addr = &a.addr;
        *ad   = a.ad;
        
        ACE_DEBUG((LM_DEBUG, "%Ireturning ack dgram for sending to %s:%u, " \
                             "sequence %u\n", a.addr.get_host_addr(),
                             a.addr.get_port_number(), a.ad.sequence));
        return true;
    }

    template <class T, class P, class C>         
    bool
    ack_resend_strategy<T,P,C>::_queue_send_front(const void      **buf,
                                          size_t           *n,
                                          const addr_type **addr,
                                          aux_data   *ad)
    {       
        uint32_t seq              = _queue_send.front();
        const dgram_send_info &si = _dgram_send_info_map[seq];
        aux_data rad;
        
        ad->sequence   = si.sequence();
        ad->type_id    = dgram_user;
        ad->type_mask  = mask_resend;
        
        *buf  = static_cast<const void *>(si.data_block()->base());
        *n    = si.data_block()->size();
        *addr = &si.addr();
        
        ACE_DEBUG((LM_DEBUG, "%Ireturning dgram for resending to %s:%u, " \
                             "sequence %u\n", si.addr().get_host_addr(),
                             si.addr().get_port_number(), ad->sequence));
        return true;
    }

    template <class T, class P, class C>         
    dgram_send_info &
    ack_resend_strategy<T,P,C>::_create_send_info(const void      *buf,
                                           size_t           n,
                                           const addr_type &addr_to,
                                           const aux_data  &ad) 
    {
        // The addr must be cast to inet_addr, we need full IP address and
        // port for the map.
        const addr_inet_type *addr = 
            dynamic_cast<const addr_inet_type *>(&addr_to);
        if (!addr)
            throw reudp::call_error(
                "reudp::ack_resend_strategy::_create_send_info():" \
                "invalid address given, need inet addr"
            );
        
        msg_block_type  *data_block = new msg_block_type(n);
        std::auto_ptr<msg_block_type> db_aptr(data_block);
        data_block->copy(static_cast<const char *>(buf), n);

        ACE_DEBUG((LM_DEBUG, "%Ifinding/creating dgram_send_info for " \
                             "sequence %u\n", ad.sequence));
        dgram_send_info &si = _dgram_send_info_map[ad.sequence];        
        si.sequence(ad.sequence);
        si.data_block(db_aptr.release());
        si.addr(*addr);
        si.base_time(_conf.gettimeofday());
        return si;
    }

}

#endif //_ACK_RESEND_STRATEGY_H_
