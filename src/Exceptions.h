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
 * Contains the class Exception.
 */

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <stdexcept>
#include <cstring>
#include <map>

namespace spw
{

  /**
   * @class Exception
   * 
   * Exception is derived from runtime_error. It can be
   * thrown by the follwoing functions of the class
   * TcpEndpoint:
   * - waitForConnection()
   * - connectToHost()
   * - sendBytes()
   * - sendString()
   * - receiveString()
   * - receiveBytes()
   * - getOwnIpAddress()
  */
class Exception : public std::runtime_error
{
public:

  enum Reason {TIMEOUT, DISCONNECT,
               NO_PEERS, PEER_NONEXISTANT,
               OTHER};
  
  //This constructor lets the user specify the
  //Reason and the message string. If no message
  //is specified it will use a standard message.
  //Also if the user choses the Reason to be OTHER
  //and doesn't provide any message string this
  //constructor will check whether an error number
  //from errno.h is set and if so use it's message
  //string.
  Exception(Reason r,
            const std::string &message = "") :
    runtime_error(""),
    reason_for_error(r),
    linux_errno(0),
    what_message(message)
  {
    if(message.empty())
    {
      what_message = reason_strings[(int)r];
      if(r == OTHER) checkIfLinuxError();
    }
  }

  Reason reason() { return reason_for_error; }

  //isTimeout() and isDisconnect() are just more convenient
  //ways to check whether the reason for the
  //function was a timeout or a disconnect. Timeouts
  //and disconnects happen very often and therefore need to
  //be handled all the time.
  bool isTimeout() { return reason_for_error == TIMEOUT; } 
  bool isDisconnect() { return reason_for_error == DISCONNECT; }

  
  bool isLinuxError() const { return linux_errno != 0; }

  int linuxErrno() const { return linux_errno; }

  //@override
  const char* what() const noexcept { return what_message.c_str(); }

private:

  static const size_t reason_count = 5;

  //The standard message strings associated with the
  //Reason enum.
  const std::string reason_strings[reason_count] = {
      "Timed out.", "Peer disconnected.",
      "No peers available.", "Peer does not exist.",
      "Unknown error."
  };

  //Check whether a linux error number from
  //errno.h is set. If it is then this function
  //modifies the Exception object to contain
  //the error number and error string.
  void checkIfLinuxError()
  {
    if(errno != 0)
    {
      linux_errno = errno;
      what_message = std::string(strerror(errno));
    }
  }

  Reason reason_for_error;
  int linux_errno;
  std::string what_message;

};

}

#endif /* EXCEPTIONS_H_ */
