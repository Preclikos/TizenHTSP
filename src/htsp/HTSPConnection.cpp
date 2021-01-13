#include "HTSPConnection.h"


#include "ppapi/cpp/net_address.h"
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <functional>
#include <limits>
#include <utility>
#include <thread>

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

HTSPConnection::HTSPConnection(pp::Instance* pp_instance, IHTSPConnectionListener& m_conn) :
		socket(pp_instance),
		instance(pp_instance),
		m_connListener(m_conn),
		cc_factory_(this),
		handle_thread(instance),
		message_read_thread_(instance)
{
	handle_thread.Start();
	message_read_thread_.Start();
}

HTSPConnection::~HTSPConnection()
{

}


void HTSPConnection::Init()
{
	PP_NetAddress_IPv4 ipv4_addr = { ConvertToNetEndian16(9982), { 192, 168, 1, 230 } };
	const pp::NetAddress netAddr = pp::NetAddress(instance, ipv4_addr);

	pp::CompletionCallback callback = cc_factory_.NewCallback(&HTSPConnection::TcpSocketDidOpen);
	    int32_t result = socket.Connect(netAddr, callback);
}

void HTSPConnection::Register(int32_t result)
{
	std::unique_lock<std::recursive_mutex> lock(m_mutex);
	bool hello = SendHello(lock);
	if(hello)
		//instance->PostMessage("hello-t");

    //std::unique_lock<std::recursive_mutex> locks(m_mutex);
    bool auth = SendAuth(lock, "test", "test");
    //if(auth)
    	//instance->PostMessage("auth-t");

    htsmsg_t* msg = htsmsg_create_map();
    htsmsg_add_u32(msg, "subscriptionId", 10);
    htsmsg_add_u32(msg, "channelId", 136089876);
    //htsmsg_add_str(msg, "profile", "webtv-h264-aac-matroska");
    SendMessage("subscribe", msg);
}

bool HTSPConnection::SendHello(std::unique_lock<std::recursive_mutex>& lock)
{
  /* Build message */
  htsmsg_t* msg = htsmsg_create_map();
  htsmsg_add_str(msg, "clientname", "Tizen HTSP Client");
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

bool HTSPConnection::SendAuth(std::unique_lock<std::recursive_mutex>& lock,
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

htsmsg_t* HTSPConnection::SendAndWait0(std::unique_lock<std::recursive_mutex>& lock,
                                       const char* method,
                                       htsmsg_t* msg,
                                       int iResponseTimeout)
{
  if (iResponseTimeout == -1)
    iResponseTimeout = 10000;

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
    	instance->PostMessage(strError);
      //Logger::Log(LogLevel::LEVEL_ERROR, "Command %s failed: %s", method, strError);
      htsmsg_destroy(msg);
      return nullptr;
    }
  }

  return msg;
}

bool HTSPConnection::SendMessage(const char* method, htsmsg_t* msg)
{
	htsmsg_add_str(msg, "method", method);

    void* buf = nullptr;
    size_t len = 0;
    int e = htsmsg_binary_serialize(msg, &buf, &len, -1);
    htsmsg_destroy(msg);

    if (e < 0)
      return false;

    char* buffer = (char*)buf;
    pp::CompletionCallback callback =  pp::BlockUntilComplete();
    int64_t c = socket.Write(buffer, len, callback);

    /*
    message_write_thread_.message_loop().PostWork(
	    cc_factory_.NewCallback(
	        &HTSPConnection::WriteAsync, buffer, len));

    std::unique_lock<std::mutex> lck(mutex_w);
    condVar_w.wait(lck);
    int64_t c = m_socket.Write(buffer, len);
	free(buffer);
*/
	  if (c != static_cast<int64_t>(len))
	  {
	    //Logger::Log(LogLevel::LEVEL_ERROR, "Command %s failed: failed to write to socket", method);
	    //if (!m_suspended)
	      //Disconnect();

	    return false;
	  }

	  return true;
}

void HTSPConnection::MessageReadLoop(int32_t result)
{

		if(!ReadMessage())
		{
			return;
		}

		message_read_thread_.message_loop().PostWork(
		    cc_factory_.NewCallback(
		        &HTSPConnection::MessageReadLoop));

}

bool HTSPConnection::ReadMessage()
{
	int32_t size = 4;
	uint8_t* lenbf = static_cast<uint8_t*>(malloc(size));
	int32_t status = socket.Read((char*)lenbf, size, pp::BlockUntilComplete());

	if(status != size)
	{
		instance->PostMessage("Lenght Size Read Wrong");
		return false;
	}

	int32_t msg_len = (lenbf[0] << 24) + (lenbf[1] << 16) + (lenbf[2] << 8) + lenbf[3];
	free(lenbf);

	uint8_t* buffer = static_cast<uint8_t*>(malloc(msg_len));
	uint8_t* buffer_partial = static_cast<uint8_t*>(malloc(msg_len));
	int32_t status_p = socket.Read((char*)buffer_partial, msg_len, pp::BlockUntilComplete());

	if(status_p != msg_len)
	{
		for (int var = 0; var < status_p; ++var) {
			buffer[var] = buffer_partial[var];
		}

		int32_t remaining = msg_len - status_p;
		while(remaining > 0)
		{
			int32_t status_partial = socket.Read((char*)buffer_partial, remaining, pp::BlockUntilComplete());

			int32_t continue_at_block = msg_len - remaining;
			for (int var = 0; var < status_partial; ++var) {
				buffer[continue_at_block] = buffer_partial[var];
				continue_at_block++;
			}


			remaining = remaining - status_partial;

		}
		free(buffer_partial);
	}
	else
	{
		memcpy(buffer, buffer_partial, status_p);
		free(buffer_partial);
	}


	htsmsg_t* msg = htsmsg_binary_deserialize(buffer, msg_len, buffer);
	/* Do not free buf here. Already done by htsmsg_binary_deserialize. */
	//free(buf);

	  if (!msg)
	  {
		  instance->PostMessage("Message Parse Error");
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
	  if (m_connListener.ProcessMessage(method, msg))
	    htsmsg_destroy(msg);

	  return true;
}

void  HTSPConnection::TcpSocketDidOpen(int32_t result){
	message_read_thread_.message_loop().PostWork(
	    cc_factory_.NewCallback(
	        &HTSPConnection::MessageReadLoop));

	handle_thread.message_loop().PostWork(
	    cc_factory_.NewCallback(
	        &HTSPConnection::Register));

}

uint16_t HTSPConnection::ConvertToNetEndian16(uint16_t x) {
  if (IsBigEndian())
    return x;
  else
    return (x << 8) | (x >> 8);
}

bool HTSPConnection::IsBigEndian() {
  union {
    uint32_t integer32;
    uint8_t integer8[4];
  } data = { 0x01020304 };
  return data.integer8[0] == 1;
}
