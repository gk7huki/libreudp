#ifndef REUDP_DGRAM_MULTI_H
#define REUDP_DGRAM_MULTI_H

/**
 * @file    dgram_multi_t.h
 * @date    Apr 6, 2005
 * @author  Arto Jalkanen
 * @brief   Extends dgram with multirecipient sending
 */

#include <functional>

#include "common.h"

namespace reudp {   
    template <class T>
    class dgram_multi_t : public T {
        template <class _Value>
        class _nop : public std::unary_function<_Value, addr_type> {
        public:
            inline const addr_type &operator()(const _Value &i) {
                return i;
            }
        };      

    public:
        dgram_multi_t() {}
        virtual ~dgram_multi_t() {}

        template<typename _AddrIter>
        ssize_t 
        send_multi(
            const void      *buf,
            size_t           n,
            _AddrIter  first, 
            _AddrIter  last,
            int        flags = 0)
        {
            return send_multi(buf, n, first, last,
                              _nop<typename _AddrIter::value_type>(),
                              flags);
        }
        // Returns number of addresses that were successfully
        // sent to TODO instead of naive approach of just sending
        // to each host, should manage the memory manually so
        // that the same content is not copied for every packet.
        template<typename _AddrIter, 
                 typename _AddrTrans> // = _nop<typename _AddrIter::value_type> >
        ssize_t 
        send_multi(
            const void      *buf,
            size_t           n,
            _AddrIter  first, 
            _AddrIter  last,
            _AddrTrans trans,
            int        flags = 0)
        {
            ACE_DEBUG((LM_DEBUG, "reudp::send_multi\n"));
            ssize_t success = 0;
            ssize_t bytes;
            for (; first != last; ++first) {
                bytes = T::send(buf, n, trans(*first), flags);
                if (bytes != -1) success++;
                // TODO if EWOULDBLOCK maybe stop the loop
            }
            return success;
        }
    };
}

#endif
