#ifndef _REUDP_DATA_HEADER_H_
#define _REUDP_DATA_HEADER_H_

#include "common.h"
#include "exception.h"

/*
 * @file    data_header.h
 * @date    Apr 9, 2005
 * @author  Arto Jalkanen
 * @brief   Reads and writes reudp header
 *
 */
 
namespace reudp {
    class data_header {
    private:
        struct data {
            // The lower 4-bits holds version, rest reudp msg type
            ACE_Byte version_and_type;
            data() : version_and_type(0) {}
        } _data;
    public:     
        inline void read(msg_block_type *from, byte_t *dgramtype, byte_t *vers);
        inline void write(msg_block_type *to,
                          int dgramtype, int vers);
                   
        inline int version() const { return _data.version_and_type & 0xF; }
        inline int type()    const { return (int)(_data.version_and_type >> 4); }
        
        inline static size_t size() { return sizeof(ACE_Byte); } // data::version_and_type); }
    };

    inline void 
    data_header::read(msg_block_type *from, byte_t *dgramtype, byte_t *vers) {
        ACE_TRACE("reudp::data_header::read");
        
        _data.version_and_type = *(from->rd_ptr());
        from->rd_ptr(1);
        
        ACE_DEBUG((LM_DEBUG, "%Iread reudp version %d dgram type %d\n",
                   version(), type()));
        if (dgramtype) *dgramtype = type();
        if (vers)       *vers     = version();

#if 0       
        if (version() > REUDP_VERSION)
            // TODO ignore packet
        if (type() != dgram_type::user && 
            type() != dgram_type::ack)
            // TODO ignore packet
#endif      
    }
    
    inline void 
    data_header::write(msg_block_type *to, int dgramtype, int vers)
    {
        ACE_TRACE("reudp::data_header::write");
        size_t data_size = sizeof(_data.version_and_type);
        
        _data.version_and_type = (((dgramtype & 0xF) << 4) | vers);
        
        ACE_DEBUG((LM_DEBUG, "%Iinserted version %d and type %d: %x\n",
                   REUDP_VERSION, dgramtype, _data.version_and_type));
        
        to->copy((char *)&(_data.version_and_type), data_size);
    }
        
} // namespace reudp

#endif //_REUDP_DATA_HEADER_H_
