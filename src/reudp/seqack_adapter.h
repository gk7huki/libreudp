#ifndef REUDP_SEQACK_ADAPTER_H
#define REUDP_SEQACK_ADAPTER_H

/**
 * @file    seqack_adapter.h
 * @date    Apr 13, 2005
 * @author  Arto Jalkanen
 * @brief   Provides a standard interface for sending/receiving reliable UDP
 *
 * Provides a standard socket-like interface, plus some auxiliary methods,
 * for supporting sending UDP packets reliably via using parametarized
 * types for socket and resending strategy interface.
 */

#include "common.h"
#include "exception.h"

namespace reudp {
    /**
     * @brief Provides a standard interface for sending/receiving reliable UDP
     * 
     * Provides methods that faciliate reliable udp trasport. Is parameterized
     * with a strategy that provides the message store and resending logic
     * so that different strategies can be tried relatively easily.
     * The strategy must conform to the following interface (see source for
     * details):
     * - methods
     *   - dgram_new
     *     - called when a new datagram is about to be sent,
     *       returns information in aux_data structure that
     *       is used to initialize the packet
     *   - send_success
     *     - called when a packet has been sent successfully
     *   - send_failure
     *     - called when sending a packet failed
     *   - received
     *     - called when packet read from socket
     *   - queue_send_empty
     *     - returns true if send queue is empty (no packets to send)
     *   - queue_pending
     *     - returns the number of packets waiting to be sent in the send queue
     *   - queue_send_front
     *     - returns the first packet from the send queue for sending
     *   - queue_send_when
     *     - returns the approximate time when send should be called next.
     *       time_value_type::max_time if no need currently for sending
     *       (when more material recv:d or sent can change). You
     *       should not use the value returned by this call if
     *       queue_send_empty returns false. If queue_send_empty
     *       returns false then send() should be called immediately
     *       and queue_send_empty may not return a valid value.
     *   - packet_done_cb
     *     - set a callback that is called when a packet's final
     *       'fate' is determined, ie. was it a success, gived up due
     *       to socket or other failure, or did it timeout since the
     *       recipient did not respond.
     * 
     * - types
     *   - structure aux_data that has to contain at least the
     *     following fields:
     *     - type_id   (numerical 0-16)
     *     - sequence  (uint32)
     *   - constants that provides at least the following identifiers for
     *     different packet types:
     *     - dgram_user
     *     - dgram_ack
     */
    template <class socket_type, class resend_strategy> 
    class seqack_adapter {
        typedef typename socket_type::header_data   _socket_data;
        typedef typename resend_strategy::aux_data  _rsstgy_data;
        
        resend_strategy _rsstgy;
        socket_type     _socket;

        // These transforms might have to be parameterized, but for now
        // this will suffice        
        inline void _ack_resend_to_seqack(_socket_data *hd,
                                          const _rsstgy_data &ad) 
        {
            hd->type_id  = (ad.type_id == resend_strategy::dgram_ack ?
                            resend_strategy::dgram_ack :
                            resend_strategy::dgram_user);
            
            hd->sequence = ad.sequence;
        }
        inline void _seqack_to_ack_resend(_rsstgy_data *ad,
                                          const _socket_data &hd) 
        {
            ad->type_id  = hd.type_id;          
            ad->sequence = hd.sequence;
        }
        
    public:
        seqack_adapter() {}
        virtual ~seqack_adapter() {}
        resend_strategy &resend_strategy_object() { return _rsstgy; }
        
        int open(const addr_type &local,
                 int             protocol_family = ACE_PROTOCOL_FAMILY_INET,
                 int             protocol = 0,
                 int             reuse_addr = 0)
        {
            return _socket.open(local, protocol_family, protocol, reuse_addr);
        }

        inline int get_local_addr(addr_inet_type &a) const {
            return _socket.get_local_addr(a);
        }
            
        inline int close() { 
            _rsstgy.reset();
            return _socket.close(); 
        }

        inline void packet_done_cb(packet_done_cb_type cb, void *param) {
            _rsstgy.packet_done_cb(cb, param);
        }

        // Returns true if there is a need to call send (possibly without
        // any data) to send data from the queues (acks, timeouted resends etc.)
        inline bool needs_to_send() {
            return !_rsstgy.queue_send_empty();
        }
        inline time_value_type needs_to_send_when() const {
            return _rsstgy.queue_send_when();
        }
        ssize_t send(const void      *buf,
                     size_t           n,
                     const addr_type &addr,
                     int             flags = 0) 
        {
            bool    queue_sent = false;
            ssize_t bytes      = -1;
            do {
                const void         *buffer;
                size_t              size;
                const addr_type    *address;
                
                _socket_data hd;
                _rsstgy_data ad;
                
                // First try sending what ever is queued for sending
                if (!_rsstgy.queue_send_empty()) {
                    _rsstgy.queue_send_front(&buffer,
                                             &size,
                                             &address,
                                             &ad);
                } else {
                    queue_sent = true;
                    if (buf) {
                        // When everything that is queued for sending has been
                        // sent, send the main data.
                        _rsstgy.dgram_new(&ad, resend_strategy::dgram_user, addr);
                        buffer  = buf;
                        size    = n;
                        address = &addr;
                    } else {
                        // If send was called with NULL buffer, assume only
                        // queued stuff was wanted for sending
                        break;
                    }               
                }

                _ack_resend_to_seqack(&hd, ad);
                
                // Reset the error status before sending so that the
                // reason for the error can be determined afterwards.
                ACE_OS::last_error(0);
                bytes = _socket.send(hd, buffer, size, *address, flags);                
                bytes = (bytes == (ssize_t)size ? 
                        _rsstgy.send_success(buffer, size, *address, ad) :
                        _rsstgy.send_failed(buffer,  size, *address, ad));
                                            
            } while (bytes != -1 && !queue_sent);

            if (bytes == -1 && !queue_sent && buf) {
                // If an error came up during purging of the queue
                // must give resend strategy the datagram that was
                // to be sent as a failure
                _rsstgy_data ad;
                _rsstgy.dgram_new(&ad, resend_strategy::dgram_user, addr);
                bytes = _rsstgy.send_failed(buf, n, addr, ad);
            }

            return bytes;
        }
        
        ssize_t recv(void  *buf,
                     size_t n,
                     addr_type &addr,
                     int flags = 0)
        {
            ACE_TRACE("reudp::seqack_adapter::recv");
            ssize_t bytes = -1;         

            _socket_data hd;
            _rsstgy_data ad; // _rsstgy_data ad;
            do {
                bytes = _socket.recv(&hd, buf, n, addr, flags);
                if (bytes < 0) return -1;

                _seqack_to_ack_resend(&ad, hd);
                
                bytes = _rsstgy.received(buf, bytes, addr, ad);
            } while (ad.type_id != resend_strategy::dgram_user);
            
            return bytes;
        }
                                     
        inline ACE_HANDLE get_handle() const { return _socket.get_handle(); }           
    };
}


#endif //_SEQACK_ADAPTER_H_
