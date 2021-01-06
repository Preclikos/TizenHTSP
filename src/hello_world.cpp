/**
 * Copyright (c) 2015, Samsung Electronics Co., Ltd
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *  * Neither the name of Samsung Electronics nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief
 * This is a simple hello world NaCl module based on C++ PPAPI interfaces.
 * It waits for a string type message form JavaScript and sends back an echo
 * message.
 * For more information about essential NaCl application structure visit:
 * @see https://developer.chrome.com/native-client/devguide/coding/application-structure
 */

#include "hello_world.h"

#include "htsp/socket/tcpsocket_api.h"
#include "ppapi/cpp/net_address.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <malloc.h>

extern "C" {
	#include "libhts/htsmsg.h"
	#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

#include <nacl_io/nacl_io.h>

HelloWorld::~HelloWorld() {/* UnregisterMessageHandler();
*/
}
const char* kEcho = "Echo from NaCl: ";

bool HelloWorld::Init(uint32_t argc, const char** argn, const char** argv) {
	InitNaClIO();
	socket_threadd_.Start();

	return true;
}

void HelloWorld::InitNaClIO() {
  nacl_io_init_ppapi(pp_instance(), pp::Module::Get()->get_browser_interface());
}

uint16_t HelloWorld::ConvertToNetEndian16(uint16_t x) {
  if (IsBigEndian())
    return x;
  else
    return (x << 8) | (x >> 8);
}

bool HelloWorld::IsBigEndian() {
  union {
    uint32_t integer32;
    uint8_t integer8[4];
  } data = { 0x01020304 };
  return data.integer8[0] == 1;
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
	PP_NetAddress_IPv4 ipv4_addr = { ConvertToNetEndian16(9982), { 192, 168, 1, 230 } };
	const pp::NetAddress netAddr = pp::NetAddress(this, ipv4_addr);

	rpcwebsocket_.Connect(netAddr);

	socket_threadd_.message_loop().PostWork(
	    cc_factory_.NewCallback(
	        &HelloWorld::MessageReadLoop));

	std::unique_lock<std::recursive_mutex> lock(m_mutex);
	SendHello(lock);


	   std::unique_lock<std::recursive_mutex> locks(m_mutex);
	   SendAuth(locks, "test", "test");

}
bool HelloWorld::SendHello(std::unique_lock<std::recursive_mutex>& lock)
{
  /* Build message */
  htsmsg_t* msg = htsmsg_create_map();
  htsmsg_add_str(msg, "clientname", "Kodi Media Center");
  htsmsg_add_u32(msg, "htspversion", 34);
  //htsmsg_add_u32(msg, "htspversion", HTSP_CLIENT_VERSION);

  /* Send and Wait */
  msg = SendAndWait0(lock, "hello", msg, -1);
  if (!msg)
    return false;

  /* Process */

  /* Basic Info */
  const char* webroot = htsmsg_get_str(msg, "webroot");
  //m_serverName = htsmsg_get_str(msg, "servername");
  //m_serverVersion = htsmsg_get_str(msg, "serverversion");
  //m_htspVersion = htsmsg_get_u32_or_default(msg, "htspversion", 0);
  //m_webRoot = webroot ? webroot : "";
  //Logger::Log(LogLevel::LEVEL_DEBUG, "connected to %s / %s (HTSPv%d)", m_serverName.c_str(),
  //            m_serverVersion.c_str(), m_htspVersion);

  /* Capabilities */
  htsmsg_t* cap = htsmsg_get_list(msg, "servercapability");
  if (cap)
  {
    htsmsg_field_t* f = nullptr;
    HTSMSG_FOREACH(f, cap)
    {
    	/*
      if (f->hmf_type == HMF_STR)
        m_capabilities.emplace_back(f->hmf_str);
        */
    }
  }

  /* Authentication */
  const void* chal = nullptr;
  size_t chal_len = 0;
  htsmsg_get_bin(msg, "challenge", &chal, &chal_len);
  if (chal && chal_len)
  {
    m_challenge = malloc(chal_len);
    m_challengeLen = chal_len;
    std::memcpy(m_challenge, chal, chal_len);
  }

  htsmsg_destroy(msg);
  return true;
}

bool HelloWorld::SendAuth(std::unique_lock<std::recursive_mutex>& lock,
                              const std::string& user,
                              const std::string& pass)
{
  htsmsg_t* msg = htsmsg_create_map();
  htsmsg_add_str(msg, "username", user.c_str());

  /* Add Password */
  // Note: we MUST send a digest or TVH will not evaluate the
  struct HTSSHA1* sha = static_cast<struct HTSSHA1*>(malloc(hts_sha1_size));
  uint8_t d[20];
  hts_sha1_init(sha);
  hts_sha1_update(sha, reinterpret_cast<const uint8_t*>(pass.c_str()), pass.length());
  if (m_challenge)
    hts_sha1_update(sha, static_cast<const uint8_t*>(m_challenge), m_challengeLen);
  hts_sha1_final(sha, d);
  htsmsg_add_bin(msg, "digest", d, sizeof(d));
  free(sha);

  /* Send and Wait */
  msg = SendAndWait0(lock, "authenticate", msg, -1);

  if (!msg)
    return 0;
/*
  if (m_htspVersion >= 26)
  {

    Log received permissions
    Logger::Log(LogLevel::LEVEL_INFO, "  Received permissions:");

    uint32_t u32 = 0;
    if (!htsmsg_get_u32(msg, "admin", &u32))
      Logger::Log(LogLevel::LEVEL_INFO, "  administrator              : %i", u32);
    if (!htsmsg_get_u32(msg, "streaming", &u32))
      Logger::Log(LogLevel::LEVEL_INFO, "  HTSP streaming             : %i", u32);
    if (!htsmsg_get_u32(msg, "dvr", &u32))
      Logger::Log(LogLevel::LEVEL_INFO, "  HTSP DVR                   : %i", u32);
    if (!htsmsg_get_u32(msg, "faileddvr", &u32))
      Logger::Log(LogLevel::LEVEL_INFO, "  Failed/aborted DVR         : %i", u32);
    if (!htsmsg_get_u32(msg, "anonymous", &u32))
      Logger::Log(LogLevel::LEVEL_INFO, "  anonymous HTSP only        : %i", u32);
    if (!htsmsg_get_u32(msg, "limitall", &u32))
      Logger::Log(LogLevel::LEVEL_INFO, "  global connection limit    : %i", u32);
    if (!htsmsg_get_u32(msg, "limitdvr", &u32))
      Logger::Log(LogLevel::LEVEL_INFO, "  DVR connection limit       : %i", u32);
    if (!htsmsg_get_u32(msg, "limitstreaming", &u32))
      Logger::Log(LogLevel::LEVEL_INFO, "  streaming connection limit : %i", u32);

  }
*/
  htsmsg_destroy(msg);
  return 1;
}

htsmsg_t* HelloWorld::SendAndWait0(std::unique_lock<std::recursive_mutex>& lock,
                                       const char* method,
                                       htsmsg_t* msg,
                                       int iResponseTimeout)
{
  if (iResponseTimeout == -1)
    iResponseTimeout = 2000;

  /* Add Sequence number */
  uint32_t seq = ++m_seq;
  htsmsg_add_u32(msg, "seq", seq);

  HTSPResponse resp;
  m_messages[seq] = &resp;

  /* Send Message (bypass TX check) */
  if (!SendMessage(method, msg))
  {
    m_messages.erase(seq);
    //Logger::Log(LogLevel::LEVEL_ERROR, "Command %s failed: failed to transmit", method);
    return nullptr;
  }

  /* Wait for response */
  msg = resp.Get(lock, iResponseTimeout);
  m_messages.erase(seq);
  if (!msg)
  {
    //Logger::Log(LogLevel::LEVEL_ERROR, "Command %s failed: No response received", method);
    //if (!m_suspended)
      //Disconnect();

    return nullptr;
  }

  /* Check result for errors and announce. */
  uint32_t noaccess = 0;
  if (!htsmsg_get_u32(msg, "noaccess", &noaccess) && noaccess)
  {
    // access denied
    //Logger::Log(LogLevel::LEVEL_ERROR, "Command %s failed: Access denied", method);
    htsmsg_destroy(msg);
    return nullptr;
  }
  else
  {
    const char* strError = htsmsg_get_str(msg, "error");
    if (strError)
    {
      //Logger::Log(LogLevel::LEVEL_ERROR, "Command %s failed: %s", method, strError);
      htsmsg_destroy(msg);
      return nullptr;
    }
  }

  return msg;
}

std::string HelloWorld::string2hexString(const uint8_t *v, const size_t s) {
	  std::stringstream ss;

	  ss << std::hex << std::setfill('0');

	  for (int i = 0; i < s; i++) {
	    ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
	  }
	  return ss.str();
	}

bool HelloWorld::SendMessage(const char* method, htsmsg_t* msg)
{
	htsmsg_add_str(msg, "method", method);

    void* buf = nullptr;
    size_t len = 0;
    int e = htsmsg_binary_serialize(msg, &buf, &len, -1);
    htsmsg_destroy(msg);

    if (e < 0)
      return false;

    char* buffer = (char*)buf;
    int64_t c = rpcwebsocket_.Write(buffer, len);
	free(buffer);

	  if (c != static_cast<int64_t>(len))
	  {
	    //Logger::Log(LogLevel::LEVEL_ERROR, "Command %s failed: failed to write to socket", method);
	    //if (!m_suspended)
	      //Disconnect();

	    return false;
	  }

	  return true;
}

void HelloWorld::MessageReadLoop(int32_t result)
{

		ReadMessage();

		socket_threadd_.message_loop().PostWork(
		    cc_factory_.NewCallback(
		        &HelloWorld::MessageReadLoop));
}

bool HelloWorld::ReadMessage()
{
	uint8_t* lenbf = static_cast<uint8_t*>(malloc(4));
	int32_t status = rpcwebsocket_.Read((char*)lenbf, 4);

	if(status == 0)
	{
		return false;
	}
	int32_t msg_len = (lenbf[0] << 24) + (lenbf[1] << 16) + (lenbf[2] << 8) + lenbf[3];
	free(lenbf);

	if(msg_len == 0)
	{
		return false;
	}

	uint8_t* buf = static_cast<uint8_t*>(malloc(msg_len));
	rpcwebsocket_.Read((char*)buf, msg_len);
	htsmsg_t* msg = htsmsg_binary_deserialize(buf, msg_len, buf);
	/* Do not free buf here. Already done by htsmsg_binary_deserialize. */
	//free(buf);

	  if (!msg)
	  {
	    return false;
	  }

	  /* Sequence number - response */
	  uint32_t seq = 0;
	  if (htsmsg_get_u32(msg, "seq", &seq) == 0)
	  {

	    std::lock_guard<std::recursive_mutex> lock(m_mutex);
	    HTSPResponseList::iterator it = m_messages.find(seq);
	    if (it != m_messages.end())
	    {
	      it->second->Set(msg);
	      return true;
	    }
	  }

	  /* Get method */
	  const char* method = htsmsg_get_str(msg, "method");
	  if (!method)
	  {
	    htsmsg_destroy(msg);
	    return true;
	  }

	  /* Pass (if return is true, message is finished) */
	  //if (m_connListener.ProcessMessage(method, msg))
	    //htsmsg_destroy(msg);

	  return true;
}

bool HelloWorld::ProcessMessage(const std::string& method, htsmsg_t* msg)
{
  return false;
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
