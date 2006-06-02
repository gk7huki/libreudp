#include <memory>

#include "ack_resend_strategy.h"
#include "exception.h"

using namespace std;

namespace reudp {
    ack_resend_strategy::ack_resend_strategy() {
        // _timeout = time_value_type(2);
        _packet_done_cb  = NULL;
        _packet_done_par = NULL;
    }
    ack_resend_strategy::~ack_resend_strategy() {
        reset();
        // todo clear maps
    }
    void
    ack_resend_strategy::reset() {
        _dgram_send_info_map.clear();
        _queue_ack.clear();
        _queue_send.clear();
        while (_queue_timeout.size() > 0) _queue_timeout.pop();
    }
    void 
    ack_resend_strategy::dgram_new(aux_data *ad, dgram_type t, 
                                   const addr_type &addr_to)
                                    
    {
        ACE_TRACE("reudp::ack_resend_strategy::dgram_new()");
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
    
    ssize_t 
    ack_resend_strategy::send_success(const void      *buf,
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
    
    ssize_t
    ack_resend_strategy::send_success_ack(const void      *buf,
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

    ssize_t
    ack_resend_strategy::send_success_user(const void      *buf,
                                           size_t           n,
                                           const addr_type &addr_to,
                                           const aux_data  &ad) 
    {                  
        if (_dgram_send_info_map.count(ad.sequence) > 0)
            throw reudp::unexpected_errorf(
                "reudp::ack_resend_strategy::send_success: " \
                "sequence %u already existed", ad.sequence
            );

        _create_send_info(buf, n, addr_to, ad).send_count_add();        
        _queue_timeout_push(ad.sequence);
                                            
        return (ssize_t)n; 
    }

    ssize_t
    ack_resend_strategy::send_success_resend(const void      *buf,
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
        
        _queue_timeout_push(ad.sequence);
        _queue_send.pop_front();
        
        return (ssize_t)n; 
    }
                         
    ssize_t 
    ack_resend_strategy::send_failed(
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
                         
    ssize_t 
    ack_resend_strategy::received(const void      *buf,
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

    ssize_t 
    ack_resend_strategy::received_user(const void           *buf,
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

    ssize_t 
    ack_resend_strategy::received_ack(const void           *buf,
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
            // TODO maybe pass on the data to the callback too.
            _do_packet_done(packet_done::success, NULL, 0, to);
                        
            // TODO maybe check that received from the same address that the ack
            // was sent to, to make spoofing harder.
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
    
    bool
    ack_resend_strategy::queue_send_front(const void      **buf,
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

    bool
    ack_resend_strategy::_queue_ack_front(const void      **buf,
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

    bool
    ack_resend_strategy::_queue_send_front(const void      **buf,
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

    dgram_send_info &
    ack_resend_strategy::_create_send_info(const void      *buf,
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
        auto_ptr<msg_block_type> db_aptr(data_block);
        data_block->copy(static_cast<const char *>(buf), n);

        ACE_DEBUG((LM_DEBUG, "%Ifinding/creating dgram_send_info for " \
                             "sequence %u\n", ad.sequence));
        dgram_send_info &si = _dgram_send_info_map[ad.sequence];        
        si.sequence(ad.sequence);
        si.data_block(db_aptr.release());
        si.addr(*addr);
                
        return si;
    }
} // namespace reudp
