#pragma once

#include <map>

#include "IHTSPConnectionListener.h"
#include "ppapi/cpp/instance.h"
#include "socket/tcpsocketsync_api.cpp"

class HTSPResponse;

typedef std::map<uint32_t, HTSPResponse*> HTSPResponseList;

class HTSPConnection
{
public:
	  HTSPConnection(pp::Instance* pp_instance, IHTSPConnectionListener& connListener);
	  ~HTSPConnection();


private:
	  IHTSPConnectionListener& m_connListener;
	  TcpSocketSyncAPI m_socket;
};
