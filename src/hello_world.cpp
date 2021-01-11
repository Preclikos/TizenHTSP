#include "hello_world.h"

#include "player/es_htsp_player_controller.h"
#include "htsp/HTSPConnection.h"
#include <sstream>
#include <iostream>
#include <malloc.h>

extern "C"
{
	#include "libhts/htsmsg.h"
}

#include <nacl_io/nacl_io.h>

HelloWorld::~HelloWorld() {/* UnregisterMessageHandler();
*/
}
const char* kEcho = "Echo from NaCl: ";

bool HelloWorld::Init(uint32_t argc, const char** argn, const char** argv) {
	InitNaClIO();
	htspConnection.Init();

	  player_thread_.message_loop().PostWork(
	    cc_factory_.NewCallback(
	        &HelloWorld::InitPlayer));

	return true;
}


void HelloWorld::InitPlayer(uint32_t result) {


	Samsung::NaClPlayer::Rect view_rect_ = Samsung::NaClPlayer::Rect((int)0, (int)0, (int)640, (int)480);
    controller->SetViewRect(view_rect_);
    controller->InitPlayer();
}
void HelloWorld::InitNaClIO() {
  nacl_io_init_ppapi(pp_instance(), pp::Module::Get()->get_browser_interface());
}

void HelloWorld::DispatchMessageMessageOnSideThread(int32_t result)
{
	 PostMessage("Test \n");

	 /*if(count < 10)
	 {
		 usleep(2000 * 1000);
		 count++;
		 socket_thread_.message_loop().PostWork(
	    cc_factory_.NewCallback(
	        &HelloWorld::DispatchMessageMessageOnSideThread));
	 }*/
}

void HelloWorld::HandleMessage(const pp::Var& message)
{

}

bool HelloWorld::ProcessMessage(const std::string& method, htsmsg_t* msg)
{
	uint32_t subId = 0;
	  if (!htsmsg_get_u32(msg, "subscriptionId", &subId))
	  {
		  controller->AddData(method, msg);
	    /* subscriptionId found - for a Demuxer */
	    /*for (auto* dmx : m_dmx)
	    {
	      if (dmx->GetSubscriptionId() == subId)
	        return dmx->ProcessMessage(method, msg);
	    }*/
	    return true;
	  }
	//count++;
	//char myString[10] = ""; // 4294967296 is the maximum for Uint32, so 10 characters it is
	//sprintf(myString, "%d", (long)count);
	//PostMessage(method + myString);
/*
  uint32_t pts = 0;
  htsmsg_get_u32(msg, "pts", &pts);
  PostMessage((char*)pts);*/
  //htsmsg_destroy(msg);

		controller->AddData(method, msg);
  return true;
}

void HelloWorld::ConnectionStateChange(const std::string& connectionString,
                                       //PVR_CONNECTION_STATE newState,
                                       const std::string& message)
{
  //kodi::addon::CInstancePVRClient::ConnectionStateChange(connectionString, newState, message);
}

bool HelloWorld::Connected(std::unique_lock<std::recursive_mutex>& lock)
{
  return true;
}

void HelloWorld::Disconnected()
{
  //m_asyncState.SetState(ASYNC_NONE);
}
