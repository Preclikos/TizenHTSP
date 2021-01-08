#ifndef SRC_TCPSOCKET_HTSP_API_H_
#define SRC_TCPSOCKET_HTSP_API_H_

#pragma once


extern "C"
{
	#include "libhts/htsmsg_binary.h"
	#include "libhts/sha1.h"
}

#include <map>

#include "IHTSPConnectionListener.h"
#include "ppapi/cpp/instance.h"
#include <thread>
#include <mutex>
#include <memory>
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "ppapi/cpp/tcp_socket.h"

class HTSPResponse;

typedef std::map<uint32_t, HTSPResponse*> HTSPResponseList;

class HTSPConnection
{
public:
	  HTSPConnection(pp::Instance* pp_instance, IHTSPConnectionListener& connListener);
	  ~HTSPConnection();

	  void Init();

private:
	  void Register(int32_t result);

	  htsmsg_t* SendAndWait0(std::unique_lock<std::recursive_mutex>& lock,
	                                         const char* method,
	                                         htsmsg_t* msg,
	                                         int iResponseTimeout);

	  bool SendMessage(const char*, htsmsg_t* msg);

	  void MessageReadLoop(int32_t result);
	  void MessageReadLoop2(int32_t result);
	  void SocketRead(int32_t result, int32_t lenght, char* buffer);
	  bool ReadMessage();

	  bool SendHello(std::unique_lock<std::recursive_mutex>& lock);
	  bool SendAuth(std::unique_lock<std::recursive_mutex>& lock, const std::string& user, const std::string& pass);

	  bool IsBigEndian();
	  uint16_t ConvertToNetEndian16(uint16_t x);

	  uint32_t m_seq = 0;
	  void* m_challenge = nullptr;
	  int m_challengeLen = 0;

	  HTSPResponseList m_messages;
	  mutable std::recursive_mutex m_mutex;

	  IHTSPConnectionListener& m_connListener;

	  pp::Instance* instance;
	  pp::TCPSocket socket;

	  pp::SimpleThread message_read_thread_;
	  pp::SimpleThread handle_thread;
	  pp::CompletionCallbackFactory<HTSPConnection> cc_factory_;

      void TcpSocketDidOpen(int32_t result);

};

#endif /* SRC_TCPSOCKET_HTSP_API_H_ */
