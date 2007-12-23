#ifndef REUDP_SEQACK_DGRAM_H
#define REUDP_SEQACK_DGRAM_H

/**
 * @file    seqack_dgram.h
 * @date    Apr 6, 2005
 * @author  Arto Jalkanen
 * @brief   Represents a reliable UDP datagram
 *
 * Reads & writes UDP packets that have necessary information for sequence
 * numbers and acks.
 */

#include <ace/SOCK_Dgram.h>

#include "data_header.h"
#include "data_seqnum.h"

namespace reudp {
    
    class seqack_dgram : private ACE_SOCK_Dgram {
    private:
        data_header _dheader;
        data_seqnum _dseqnum;
        
        static const size_t _header_size;
    public:
        struct header_data {
            // Only lower 4 bits can be used in type_id
            reudp::byte_t      type_id;
            reudp::uint32_t    sequence;
            // TODO timestamp
        };
        
        seqack_dgram(); 
        seqack_dgram(const addr_type &local,
                     int protocol_family = ACE_PROTOCOL_FAMILY_INET,
                     int protocol = 0,
                     int reuse_addr = 0);
        virtual ~seqack_dgram();
        
        using ACE_SOCK_Dgram::dump;
        
        int open(const addr_type &local,
                 int             protocol_family = ACE_PROTOCOL_FAMILY_INET,
                 int             protocol = 0,
                 int             reuse_addr = 0);
    
        inline int close() { return ACE_SOCK_Dgram::close(); }

        inline int get_local_addr(addr_inet_type &a) const {
            return ACE_SOCK_Dgram::get_local_addr(a);
        }
        
        ssize_t send(const header_data &hd,
                     const void       *buf,
                     size_t            n,
                     const addr_type  &addr,
                     int              flags = 0);
    
        ssize_t recv(header_data      *hd,
                     void  *buf,
                     size_t n,
                     addr_type &addr,
                     int flags = 0);
                                     
        inline ACE_HANDLE get_handle() const { return ACE_SOCK_Dgram::get_handle(); }
    };
}

#endif //_REUDP_SEQACK_DGRAM_H_
