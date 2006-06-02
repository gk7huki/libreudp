#ifndef _REUDP_DATA_SEQNUM_H_
#define _REUDP_DATA_SEQNUM_H_

#include "common.h"
#include "exception.h"

/*
 * @file    data_seqnum.h
 * @date    Apr 9, 2005
 * @author  Arto Jalkanen
 * @brief   Reads and writes reudp sequence number information
 *
 */
 
namespace reudp {
    class data_seqnum {
    private:
        struct data {
            // Data is held in network byte order
            reudp::uint32_t seq;
            data() : seq(0) {}
        } _data;
        
    public:     
        inline void read(msg_block_type *from, reudp::uint32_t *seq);
        inline void write(msg_block_type *to,  reudp::uint32_t  seq);
                   
        // Returns the sequence number in host byte order
        inline reudp::uint32_t sequence() const { return ACE_NTOHL(_data.seq); }
        inline static size_t size() { return sizeof(reudp::uint32_t); } // data::version_and_type); }
        
    };
    
    
    inline void
    data_seqnum::read(msg_block_type *from, reudp::uint32_t *seq) {
        ACE_TRACE("reudp::data_seqnum::read");
        
        memcpy(&_data.seq, from->rd_ptr(), sizeof(_data.seq));
        from->rd_ptr(sizeof(_data.seq));
                        
        ACE_DEBUG((LM_DEBUG, "%Iread sequence number %u\n",
                   sequence()));
        if (seq) *seq = sequence();     
    }
    
    inline void
    data_seqnum::write(msg_block_type *to,
                       reudp::uint32_t seq)
    {
        ACE_TRACE("reudp::data_seqnum::write");
        size_t data_size = sizeof(_data.seq);
        
        _data.seq = ACE_HTONL(seq);     
        to->copy((char *)&(_data.seq), data_size);
                
        ACE_DEBUG((LM_DEBUG, "%Iinserted sequence number %d\n", seq));
    }
        
} // namespace reudp

#endif //_REUDP_DATA_SEQNUM_H_
