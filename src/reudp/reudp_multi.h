#ifndef REUDP_MULTI_H
#define REUDP_MULTI_H

#include "common.h"
#include "reudp.h"
#include "dgram_multi_t.h"

/**
 * @file    reudp_multi.h
 * @date    28.12.2007
 * @author  Arto Jalkanen
 * @brief   Base include for applications using multirecipient reudp
 * 
 * Defines different multirecipient datagram types.
 *
 */

namespace reudp {
    // Multirecipient definitions
    typedef dgram_multi_t<dgram_constant_timeout> dgram_multi_constant_timeout;
    typedef dgram_multi_t<dgram_variable_timeout> dgram_multi_variable_timeout;
    typedef dgram_multi_t<dgram> dgram_multi;
}

#endif // REUDP_MULTI_H
