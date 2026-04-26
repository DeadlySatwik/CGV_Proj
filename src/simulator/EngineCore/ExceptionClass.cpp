///   File: ExceptionClass.cpp


#include "ExceptionClass.h"

ExceptionClass::ExceptionClass(const char *msg) throw()
{
    exceptionMsg = msg;
}

ExceptionClass::ExceptionClass(std::string msg) throw()
{
    exceptionMsg = msg.c_str();
}

const char *ExceptionClass::what() const throw()
{
    return exceptionMsg;
}
