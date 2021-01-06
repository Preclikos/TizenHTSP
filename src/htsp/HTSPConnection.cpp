#include "HTSPConnection.h"


extern "C"
{
	#include "libhts/htsmsg_binary.h"
	#include "libhts/sha1.h"
}

#include <condition_variable>
#include <chrono>
#include <mutex>

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
		m_connListener(m_conn),
		m_socket(pp_instance)
{
}

HTSPConnection::~HTSPConnection()
{

}
