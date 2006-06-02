#ifndef _REUDP_MESSAGE_BLOCK_H_
#define _REUDP_MESSAGE_BLOCK_H_

/*
 * @file    message_block.h
 * @date    Apr 10, 2005
 * @author  Arto Jalkanen
 * @brief   Provides a facility for reudp to create efficiently messages to send
 *
 * A sort of decorator for ACE's ACE_Message_Block class, that provides
 * only the functions that are needed by reudp. To reduce number of checks
 * in the calling code, throws exceptions.
 */
#include <ace/Basic_Types.h>
#include <ace/Message_Block.h>

namespace reudp {
    class message_block : protected ACE_Message_Block {
    private:
    public:
        message_block();
        message_block(size_t size);
        message_block(const char *data, size_t len = 0);
        
        virtual ~message_block();
    
        using ACE_Message_Block::release;
        using ACE_Message_Block::size;
        using ACE_Message_Block::length;
        using ACE_Message_Block::space;
        using ACE_Message_Block::base;
        using ACE_Message_Block::rd_ptr;
        using ACE_Message_Block::wr_ptr;
        using ACE_Message_Block::reset;
        
        int copy (const char *buf, size_t n);
    };
}

#endif //_MESSAGE_BLOCK_H_
