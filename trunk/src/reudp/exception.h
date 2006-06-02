#ifndef _REUDP_EXCEPTION_H_
#define _REUDP_EXCEPTION_H_
/**
 * @file    exception.h
 * @date    Apr 9, 2005
 * @author  Arto Jalkanen
 * @brief   defines exceptions that reudp might throw
 *
 * Fatal errors will be thrown by reudp as exceptions. This file
 * defines used exception classes. Each exception carries a code
 * and an additional error string.
 */

#include <exception>
#include <stdarg.h>

#include "common.h"
 
namespace reudp {
    class exception : public std::exception {
    private:
        const char *_str;
    public:
        exception(const char *reason) :
          _str(reason) {
            ACE_TRACE("reudp::exception()");
            ACE_ERROR((LM_ERROR, "reudp::exception: %s\n", reason));
        }
        virtual ~exception() throw();
        virtual const char *what() const throw() { return _str; }
    };
    
    class exceptionf : public reudp::exception {
    private:
        char _str_store[256];
    protected:
        inline void _initv(const char *format, va_list a) {
            vsnprintf(_str_store, sizeof(_str_store)/sizeof(_str_store[0]), 
                      format, a);           
        }
        struct initv_ctor {};
        exceptionf(const char *format, initv_ctor)
            : exception(format) {}
    public:
        exceptionf(const char *format, ...) 
            : exception(format) {
            va_list a;
            va_start(a, format);
            _initv(format, a);
            va_end(a);

            // ACE_ERROR((LM_ERROR, "reudp::exception: %s\n", _str_store));         
        }
        virtual ~exceptionf() throw();
        virtual const char *what() const throw() { return _str_store; }
    };

    template <typename ParentType = exception>
    class exception_class : public ParentType {
    public:
        exception_class(const char *reason) : ParentType(reason) {}
    };
    
    template <typename ParentType = exceptionf>
    class exception_classf : public ParentType {
    public:
        exception_classf(const char *format, ...) 
          : ParentType(format, typename ParentType::initv_ctor()) {
            va_list a;
            va_start(a, format);
            ParentType::_initv(format, a);
            va_end(a);
        }
    };
    
    #define _REUDP_EXCEPTION_CLASS(name) \
        typedef exception_class<>  name;\
        typedef exception_classf<> name##f;

    #define _REUDP_EXCEPTION_SUBCLASS(name, parent) \
        typedef exception_class<parent>  name;\
        typedef exception_classf<parent> name##f;
            
    _REUDP_EXCEPTION_CLASS(call_error)
    _REUDP_EXCEPTION_CLASS(unexpected_error)
    _REUDP_EXCEPTION_CLASS(mem_alloc_error)
    // TODO error code for io_error?
    _REUDP_EXCEPTION_CLASS(io_error)
}
 
#endif //_REUDP_EXCEPTION_H_
