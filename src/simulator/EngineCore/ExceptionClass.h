///   File: ExceptionClass.h


#ifndef EXCEPTIONCLASS_H
#define EXCEPTIONCLASS_H

#include <exception>
#include <string>

class ExceptionClass : public std::exception
{
public:
    ExceptionClass(const char *msg) throw();
    ExceptionClass(std::string msg) throw();
    virtual ~ExceptionClass() throw() {};

    const char* what() const throw();

private:
    const char *exceptionMsg;
};

#endif // EXCEPTIONCLASS_H
