/*
Copyright (c) 2017 ayevuhn

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 * @file Exceptions.h
 *
 * Contains the classes Timeout, Disconnect,
 * NoConnection and Other error which are
 * all derived from runtime_error. They are
 * thrown by some functions of the class
 * TcpEndpoint.
 */

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <stdexcept>
#include <cstring>


namespace spw
{

static const int UNKNOWN_ERROR = -522;
static const char *UNKNOWN_ERROR_STR = "Unknown Error";

/**
 * @class Timeout
 *
 * Thrown when timeout occurs.
 */
class Timeout : public std::runtime_error
{
public:

  Timeout(const std::string &message) :
    runtime_error(message)
  { }

};

/**
 * @class Disconnect
 *
 * Thrown when peer disconnects during
 * communication.
 */
class Disconnect : public std::runtime_error
{
public:

  Disconnect(const std::string &message) :
    runtime_error(message)
  { }

};


/**
 * @class NoConnection
 *
 * Thrown when calling a function that needs
 * a connection before a connection was
 * established (e. g. sendBytes()).
 */
class NoConnection : public std::runtime_error
{
public:

  NoConnection(const std::string &message) :
    runtime_error(message)
  { }
};

/**
 * @class OtherError
 *
 * Thrown when some other error occurs. OtherError
 * tries to get the current error from
 * the errno variable. If errno does not
 * indicate any error OtherError will
 * contain the Message specified in the
 * const string UNKNOWN_ERROR_STR.
 */
class OtherError : public std::runtime_error
{
public:

  OtherError() :
    runtime_error(""),
    m_errnum(0), m_errstring(NULL)
  {
    checkError();
  }

  int getErrno() const { return m_errnum; }

  //@override
  const char* what() const noexcept
  {
    return m_errstring;
  }

  ~OtherError()
  {
    delete[] m_errstring;
  }

private:

  /// See whether errno is set.
  void checkError()
  {

    const char *pstr = NULL;
    size_t strbuffsize = 0;


    if(errno != 0)
    { // errno is set, determine buffer size
      // for error string from strerror()
      m_errnum = errno;
      pstr = strerror(errno);
      strbuffsize = strlen(pstr) + 1;
    }
    else
    { // errno is not set, determine buffer size
      // for the UNKNOWN_ERROR string
      m_errnum = UNKNOWN_ERROR;
      pstr = UNKNOWN_ERROR_STR;
      strbuffsize = strlen(UNKNOWN_ERROR_STR) + 1;
    }
    // Allocate space for error string and
    // copy into m_errstring.
    m_errstring = new char[strbuffsize];
    strcpy(m_errstring, pstr);
  }

  int m_errnum;
  char* m_errstring;

};

}

#endif /* EXCEPTIONS_H_ */
