#include <memory>

#include "common.h"
#include "seqack_dgram.h"
#include "data_header.h"
#include "data_seqnum.h"
#include "exception.h"

namespace reudp {
    const size_t seqack_dgram::_header_size = data_header::size() +
                                              data_seqnum::size();
    
    seqack_dgram::seqack_dgram() {
        ACE_TRACE("reudp::seqack_dgram::seqack_dgram()");
    }
    
    seqack_dgram::seqack_dgram(
        const addr_type &local,
        int             protocol_family,
        int             protocol,
        int             reuse_addr)
    {
        open(local, protocol_family, protocol, reuse_addr); 
    }
    
    seqack_dgram::~seqack_dgram() {
        ACE_TRACE("reudp::seqack_dgram::~seqack_dgram()");
    }
    
    int
    seqack_dgram::open(
        const addr_type &local,
        int             protocol_family,
        int             protocol,
        int             reuse_addr)
    {
        ACE_TRACE("reudp::seqack_dgram::open()");
        // throw exception if open did not succeed
        if (-1 == ACE_SOCK_Dgram::open(local, protocol_family, 
                                       protocol, reuse_addr)) {
            ACE_ERROR((LM_ERROR, "error: %p\n", "open"));
            // TODO error string, TODO error code
            throw reudp::io_error("could not open");
        }
        return 0;
    }
    
    ssize_t
    seqack_dgram::send(
        const header_data &hd,
        const void     *buf,
        size_t          n,
        const addr_type &addr,
        int             flags)
    {
        ACE_TRACE("reudp::seqack_dgram::send()");

        ssize_t sent_bytes;
        size_t  total_size = _header_size + n;
        
        char           header_data_store[_header_size];
        msg_block_type header_block(header_data_store, _header_size);
        
        ACE_DEBUG((LM_DEBUG, "%Iwriting packet header (%d bytes)\n", _header_size));

        _dheader.write(&header_block, hd.type_id, REUDP_VERSION);
        _dseqnum.write(&header_block, hd.sequence);

        iovec vec[2];
        int   veclen = 1;
        vec[0].iov_base = header_block.base();
        vec[0].iov_len  = header_block.size();
        if (buf && n) {
            vec[1].iov_base = (char *)buf;
            vec[1].iov_len  = n;
            veclen++;
        }       
        ACE_DEBUG((LM_DEBUG, "%Isending data size %d\n", total_size));

        sent_bytes = ACE_SOCK_Dgram::send(vec, 
                                          veclen,
                                          addr, flags);
    
        if (sent_bytes == -1)
            ACE_ERROR((LM_WARNING, "%p\n", "seqack_dgram::send\n"));
        else    
            ACE_DEBUG((LM_DEBUG, "%Isend returned %d\n", sent_bytes));
            
        sent_bytes = (sent_bytes == (ssize_t)total_size  ?
                      sent_bytes - _header_size :
                      (size_t)-1);
        
        return sent_bytes;
    }
    
    ssize_t
    seqack_dgram::recv(
        header_data        *hd,
        void  *buf,
        size_t n,
        addr_type &addr,
        int flags) 
    {
        ACE_TRACE("reudp::seqack_dgram::recv()");

        char header_data_store[_header_size];
        msg_block_type header_block(header_data_store, _header_size);
        
        iovec vec[2];
        int   veclen = 1;
        memset(vec, 0, sizeof(vec));
        vec[0].iov_base = header_block.base();
        vec[0].iov_len  = header_block.size();
        if (buf && n) {
            vec[1].iov_base = static_cast<char *>(buf);
            vec[1].iov_len  = n;
            veclen++;
        }       
        header_block.wr_ptr(header_block.size());
        ssize_t bytes = -1;
        do {
            // Reset error value before call
            ACE_OS::last_error(0);
            bytes = ACE_SOCK_Dgram::recv(vec, veclen, 
                                         addr, flags);
            // This check is here because of strange Windows
            // specific UDP operation regarding localhost. TODO take
            // away if and when ACE has fixed this                                       
        } while (bytes == -1 && ACE_OS::last_error() == 10054);

        if (bytes >= (ssize_t)_header_size) {           
            bytes -= _header_size;
            // read the header          
            _dheader.read(&header_block, &hd->type_id, NULL);
            _dseqnum.read(&header_block, &hd->sequence);
        } else if (bytes > 0 && bytes < (ssize_t)_header_size) {
            ACE_DEBUG((LM_WARNING, "reudp::recv did not receive enough for header: " \
                                   "received %d bytes, header is %d bytes\n",
                                   bytes, _header_size));
            bytes = -1;
        } else if (bytes == -1) {
            ACE_ERROR((LM_WARNING, "%p\n", "reudp::seqack_dgram::recv"));
        }               
        return bytes; //_sock.recv(buf, n, addr, flags);        
    }   
} // namespace reudp
