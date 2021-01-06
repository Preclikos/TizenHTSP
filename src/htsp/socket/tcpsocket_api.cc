#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/tcp_socket.h"
#include "ppapi/utility/completion_callback_factory.h"

#include "tcpsocket_api.h"


class TcpSocketAPI::Implement : public pp::TCPSocket {
 public:
  Implement(pp::Instance* instance, TcpSocketAPI* api)
      :	pp::TCPSocket(instance),
        api_(api),
        callback_factory_(this) {
  }

  virtual ~Implement() {}

  int32_t Connect(const pp::NetAddress &addr) {
    pp::CompletionCallback callback = callback_factory_.NewOptionalCallback(&Implement::DidConnect);
    int32_t result = pp::TCPSocket::Connect(addr, callback);
    if (result != PP_OK_COMPLETIONPENDING) {
      // In synchronous cases, consumes callback here and invokes callback
      // with PP_ERROR_ABORTED instead of result in order to avoid side effects
      // in DidConnect. DidConnect ignores this invocation and doesn't call
      // any delegate virtual method.
      callback.Run(PP_ERROR_ABORTED);
    }
    return result;
  }

  void DidConnect(int32_t result) {
      api_->TcpSocketDidOpen(result);
  }

  int32_t Read(char* buffer, int32_t bytes_to_read) {
    pp::CompletionCallback callback = callback_factory_.NewOptionalCallback(&Implement::DidRead);
    int32_t result = pp::TCPSocket::Read(buffer, bytes_to_read, callback);
    if (result != PP_OK_COMPLETIONPENDING) {
      // In synchronous cases, consumes callback here and invokes callback
      // with PP_ERROR_ABORTED instead of result in order to avoid side effects
      // in DidConnect. DidConnect ignores this invocation and doesn't call
      // any delegate virtual method.
      callback.Run(PP_ERROR_ABORTED);
    }
    return result;
  }

  void DidRead(int32_t result) {
      api_->TcpSocketDidRead(result);
  }

  int32_t Write(const char* buffer, int32_t bytes_to_write)
  {
	  pp::CompletionCallback callback = callback_factory_.NewOptionalCallback(&Implement::DidWrite);
	  int32_t result = pp::TCPSocket::Write(buffer, bytes_to_write, callback);
	  if (result != PP_OK_COMPLETIONPENDING) {
		// In synchronous cases, consumes callback here and invokes callback
		// with PP_ERROR_ABORTED instead of result in order to avoid side effects
		// in DidConnect. DidConnect ignores this invocation and doesn't call
		// any delegate virtual method.
		callback.Run(PP_ERROR_ABORTED);
	  }
	  return result;
  }

  void DidWrite(int32_t result) {
      api_->TcpSocketDidWrite(result);
  }
  /*
  void DidClose(int32_t result) {
     if (result == PP_ERROR_ABORTED)
       return;
     bool was_clean = GetCloseWasClean();
     if (!was_clean)
       api_->HandleWebSocketError();
     api_->WebSocketDidClose(was_clean, GetCloseCode(), GetCloseReason());
   }*/

 private:
  TcpSocketAPI* api_;
  pp::CompletionCallbackFactory<Implement> callback_factory_;
  pp::Var receive_message_var_;
};

TcpSocketAPI::TcpSocketAPI(pp::Instance* instance)
    : impl_(new Implement(instance, this)) {
}

TcpSocketAPI::~TcpSocketAPI() {
  delete impl_;
}

int32_t TcpSocketAPI::Connect(const pp::NetAddress &addr) {
  return impl_->Connect(addr);
}

int32_t TcpSocketAPI::Read(char* buffer, int32_t bytes_to_read) {
  return impl_->Read(buffer, bytes_to_read);
}

int32_t TcpSocketAPI::Write(char* buffer, int32_t bytes_to_write) {
  return impl_->Write(buffer, bytes_to_write);
}
