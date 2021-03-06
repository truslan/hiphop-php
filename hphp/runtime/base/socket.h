/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2013 Facebook, Inc. (http://www.facebook.com)     |
   | Copyright (c) 1998-2010 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
*/

#ifndef incl_HPHP_SOCKET_H_
#define incl_HPHP_SOCKET_H_

#include "hphp/runtime/base/file.h"
#include <sys/types.h>
#include <sys/socket.h>

#define SOCKET_ERROR(sock, msg, errn)                           \
  sock->setError(errn);                                         \
  raise_warning("%s [%d]: %s", msg, errn,                       \
                  Util::safe_strerror(errn).c_str())            \

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

/**
 * TCP/UDP sockets.
 */
class Socket : public File {
public:
  // We cannot use object allocation for this class, because
  // we need to support pfsockopen() that can make a socket persistent.

  Socket();
  Socket(int sockfd, int type, const char *address = nullptr, int port = 0,
         double timeout = 0);
  virtual ~Socket();

  static StaticString s_class_name;
  // overriding ResourceData
  CStrRef o_getClassNameHook() const { return s_class_name; }

  // implementing File
  virtual bool open(CStrRef filename, CStrRef mode);
  virtual bool close();
  virtual int64_t readImpl(char *buffer, int64_t length);
  virtual int64_t writeImpl(const char *buffer, int64_t length);
  virtual bool eof();
  virtual Array getMetaData();
  virtual int64_t tell();

  // check if the socket is still open
  virtual bool checkLiveness();

  void setError(int err);
  int getError() const { return m_error;}
  static int getLastError();
  int getType() const { return m_type;}

  // This is only for updating a local copy of timeouts set by setsockopt()
  // outside of this class.
  void setTimeout(struct timeval &tv);

  bool setBlocking(bool blocking);

  std::string getAddress() const { return m_address; }
  int         getPort() const    { return m_port; }
protected:
  std::string m_address;
  int m_port;

  int m_type;
  int m_error;
  bool m_eof;

  int m_timeout; // in micro-seconds;
  bool m_timedOut;

  int64_t m_bytesSent;

  bool closeImpl();
  bool waitForData();
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // incl_HPHP_SOCKET_H_
