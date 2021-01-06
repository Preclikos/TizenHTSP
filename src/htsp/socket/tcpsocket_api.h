/*
 * tcpsocket_api.h
 *
 *  Created on: Jan 3, 2021
 *      Author: jan.husak
 */

#ifndef SRC_TCPSOCKET_API_H_
#define SRC_TCPSOCKET_API_H_

#include "ppapi/cpp/instance.h"
#include "ppapi/c/ppb_tcp_socket.h"
#include "ppapi/cpp/net_address.h"

class CompletionCallback;
class Instance;
class Var;

/// The <code>WebSocketAPI</code> class
class TcpSocketAPI {
 public:
  /// Constructs a WebSocketAPI object.
  explicit TcpSocketAPI(pp::Instance* instance);
  /// Destructs a WebSocketAPI object.
  virtual ~TcpSocketAPI();

  int32_t Connect(const pp::NetAddress &addr);
  virtual void TcpSocketDidOpen(int32_t result) = 0;

  int32_t Read(char* buffer, int32_t bytes_to_read);
  virtual void TcpSocketDidRead(int32_t result) = 0;

  int32_t Write(char* buffer, int32_t bytes_to_write);
  virtual void TcpSocketDidWrite(int32_t result) = 0;

 private:
  class Implement;
  Implement* impl_;
};

#endif /* SRC_TCPSOCKET_API_H_ */
