#include <string.h>

#include "message_block.h"
#include "exception.h"

namespace reudp {
    message_block::message_block() 
      : ACE_Message_Block() {}
      
    message_block::message_block(size_t size) 
      : ACE_Message_Block(size) 
    {
        ACE_TRACE("reudp::message_block::message_block(size_t)");
        if (ACE_Message_Block::size() != size)
            throw reudp::mem_alloc_errorf(
                "message_block::ctor: failed reserving %d bytes",
                size);
    }

    message_block::message_block(const char *data, size_t size)
      : ACE_Message_Block(data, size) 
    {
        ACE_TRACE("reudp::message_block::message_block(const char *, size)");
        if (ACE_Message_Block::size() != size)
            throw reudp::mem_alloc_errorf(
                "message_block::ctor: failed reserving %d bytes",
                size);
    }
    
    message_block::~message_block() {
        ACE_TRACE("reudp::message_block::~message_block()");
    }
    
    int 
    message_block::copy (const char *buf, size_t n) {
        ACE_TRACE("reudp::message_block::copy()");

        if (-1 == ACE_Message_Block::copy((const char *)buf, n))
            throw reudp::mem_alloc_errorf(
                "failed copying %d bytes for message block from %p", n, buf);
        return 0;               
    }
}
