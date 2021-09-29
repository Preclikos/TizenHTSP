#include "hello_world.h"

#include "player/es_htsp_player_controller.h"
#include "htsp/HTSPConnection.h"
#include <sstream>
#include <iostream>
#include <malloc.h>

#include "logger.h"
#include "player/url_loader.h"

extern "C"
{
	#include "libhts/htsmsg.h"
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

#include <nacl_io/nacl_io.h>

HelloWorld::~HelloWorld() {/* UnregisterMessageHandler();
*/


}
const char* kEcho = "Echo from NaCl: ";

bool HelloWorld::Init(uint32_t argc, const char** argn, const char** argv) {
	InitNaClIO();

	Logger::InitializeInstance(this);
	htspConnection.Init();
/*
    av_register_all();

    const char *url = "http://192.168.1.210:9981/stream/channelid/874852563?profile=pass";
    AVFormatContext *s = NULL;

    int ret = avformat_open_input(&s, url, NULL, NULL);
    if (ret < 0)
    {
    	LOG_DEBUG("Error open %d", ret);
    }
    else
    {
    	LOG_DEBUG("Sucess open %d", ret);
    }
*/
	/*
    URLLoaderHandler* handler = URLLoaderHandler::Create(this, "http://192.168.1.210:9981/stream/channelid/874852563?profile=pass", *this);
    if (handler != NULL) {
      // Starts asynchronous download. When download is finished or when an
      // error occurs, |handler| posts the results back to the browser
      // vis PostMessage and self-destroys.
      handler->Start();
    }*/

	  player_thread_.message_loop().PostWork(
	    cc_factory_.NewCallback(
	        &HelloWorld::InitPlayer));

	  //auto loader = URLLoaderHandler::Create(this, "http://192.168.1.230:9981/stream/channelid/136089876?profile=pass");

	  //loader->Start();

	  LOG_INFO("Finished Init");

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

bool HelloWorld::ReceiveData(const char* buffer, int32_t num_bytes)
{
	controller->AddHttpData(buffer, num_bytes);
	return true;
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
