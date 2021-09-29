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
#include "htsp/HTSPConnection.h"
#include "htsp/IHTSPConnectionListener.h"
#include "player/common.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "player/es_htsp_player_controller.h"
#include "player/url_loader_listener.h"

using Samsung::NaClPlayer::Rect;

class HelloWorld : public pp::Instance, public IHTSPConnectionListener, public IURLLoaderHandlerListener {
 public:
  /**
   * Your constructor needs to call the base class <code>pp::Instance(PP_Instance)</code>
   * constructor to properly initialize itself - it's the only way because
   * <code>pp::Instance(PP_Instance)</code> is the only public constructor.
   */
  explicit HelloWorld(PP_Instance instance)
      : pp::Instance(instance),
		player_thread_(this),
		cc_factory_(this),
		controller(std::make_shared<EsHtspPlayerController>(this, this)),
		htspConnection(this, *this)
  {
	  player_thread_.Start();
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
  bool ReceiveData(const char* buffer, int32_t num_bytes) override;

private:
  void InitPlayer(uint32_t result);
  void DispatchMessageMessageOnSideThread(int32_t result);
  void InitNaClIO();
  int32_t count = 0;
  HTSPConnection htspConnection;
  std::shared_ptr<EsHtspPlayerController> controller;
  pp::SimpleThread player_thread_;
  pp::CompletionCallbackFactory<HelloWorld> cc_factory_;

  Samsung::NaClPlayer::Rect view_rect_;

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
