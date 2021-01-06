/*
 * hello_world.h
 *
 *  Created on: Jan 3, 2021
 *      Author: jan.husak
 */
#ifndef SRC_HELLO_WORLD_H_
#define SRC_HELLO_WORLD_H_

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "htsp/socket/tcpsocket_api.h"
#include "htsp/socket/tcpsocketsync_api.cpp"
#include "htsp/HTSPConnection.h"
#include "htsp/IHTSPConnectionListener.h"
extern "C" {
	#include "libhts/htsmsg.h"
}

class HTSPConnection;

class HTSPResponse
{
public:
  HTSPResponse() = default;

  ~HTSPResponse()
  {
    if (m_msg)
      htsmsg_destroy(m_msg);

    Set(nullptr); // ensure signal is sent
  }

  htsmsg_t* Get(std::unique_lock<std::recursive_mutex>& lock, uint32_t timeout)
  {
	  m_cond.wait_for(lock, std::chrono::milliseconds(timeout), [this] { return m_flag == true; });
    htsmsg_t* r = m_msg;
    m_msg = nullptr;
    m_flag = false;
    return r;
  }

  void Set(htsmsg_t* msg)
  {
    m_msg = msg;
    m_flag = true;
    m_cond.notify_all();
  }

private:
  std::condition_variable_any m_cond;
  bool m_flag = false;
  htsmsg_t* m_msg = nullptr;
};


typedef std::map<uint32_t, HTSPResponse*> HTSPResponseList;

class HelloWorld : public pp::Instance, public IHTSPConnectionListener {
 public:
  /**
   * Your constructor needs to call the base class <code>pp::Instance(PP_Instance)</code>
   * constructor to properly initialize itself - it's the only way because
   * <code>pp::Instance(PP_Instance)</code> is the only public constructor.
   */
  explicit HelloWorld(PP_Instance instance)
      : pp::Instance(instance),
	    htspConnection( new HTSPConnection(this, *this)),
		rpcwebsocket_(this),
		socket_threadd_(this),
		cc_factory_(this){

//htspConnection = new HtspConnection(pp_instance());
  }

  virtual ~HelloWorld() override;

  /**
   * Handles messages from JS sent by <code>nacl_module.postMessage(...)</code>.
   * @see <code>HandleMessage</code> in instance.h file for more details.
   */
  void HandleMessage(const pp::Var& var_message) override;

  /**
   * Initializes this instance with provided arguments listed in the <embed>
   * tag.
   * @see <code>Init()</code> in instance.h file for more details.
   */
  bool Init(uint32_t, const char**, const char**) override;

  // IHTSPConnectionListener implementation
  void Disconnected() override;
  bool Connected(std::unique_lock<std::recursive_mutex>& lock) override;
  bool ProcessMessage(const std::string& method, htsmsg_t* msg) override;
  void ConnectionStateChange(const std::string& connectionString,
                             //PVR_CONNECTION_STATE newState,
                             const std::string& message) override;

private:
  void DispatchMessageMessageOnSideThread(int32_t result);
  bool IsBigEndian();
  uint16_t ConvertToNetEndian16(uint16_t x);
  htsmsg_t* SendAndWait0(std::unique_lock<std::recursive_mutex>& lock,
                                         const char* method,
                                         htsmsg_t* msg,
                                         int iResponseTimeout);
  bool SendMessage(const char*, htsmsg_t* msg);
  bool ReadMessage();
  bool SendHello(std::unique_lock<std::recursive_mutex>& lock);
  bool SendAuth(std::unique_lock<std::recursive_mutex>& lock, const std::string& user, const std::string& pass);
  std::string string2hexString(const uint8_t *v, const size_t s);

  pp::SimpleThread socket_threadd_;
  pp::CompletionCallbackFactory<HelloWorld> cc_factory_;
  TcpSocketSyncAPI rpcwebsocket_;
  void InitNaClIO();
  void MessageReadLoop(int32_t result);

  HTSPConnection* htspConnection;

  HTSPResponseList m_messages;
  mutable std::recursive_mutex m_mutex;
  uint32_t m_seq = 0;
  void* m_challenge = nullptr;
  int m_challengeLen = 0;
};

class HelloWorldModule : public pp::Module {
 public:
	pp::Instance* CreateInstance(PP_Instance instance) override {
    return new HelloWorld(instance);
  }
};

namespace pp {
Module* CreateModule() { return new HelloWorldModule(); }
}

#endif /* SRC_HELLO_WORLD_H_ */
