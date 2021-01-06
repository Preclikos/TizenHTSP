#ifndef SRC_TCPSOCKETSYNC_API_CPP_
#define SRC_TCPSOCKETSYNC_API_CPP_

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/utility/websocket/websocket_api.h"
#include "tcpsocket_api.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "ppapi/utility/completion_callback_factory.h"

#include <iostream>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <future>

class TcpSocketSyncAPI : protected TcpSocketAPI
{
public:
	TcpSocketSyncAPI(pp::Instance* ppinstance)
        : TcpSocketAPI(ppinstance),
		  socket_thread_(ppinstance),
		  read_thread_(ppinstance),
		  m_ppinstance(ppinstance),
		  callback_(this),
		  writingStatus(PP_OK_COMPLETIONPENDING),
		  readingStatus(PP_OK_COMPLETIONPENDING),
		  connectionStatus(PP_OK_COMPLETIONPENDING)
		{
			socket_thread_.Start();
			read_thread_.Start();
		}
    virtual ~TcpSocketSyncAPI() {}
    int32_t Connect(const pp::NetAddress &addr) {
    	//connectionStatus = PP_OK_COMPLETIONPENDING;
    	socket_thread_.message_loop().PostWork(callback_.NewCallback(
    	      &TcpSocketSyncAPI::ConnectAsync, addr));
    	/*
    	while(connectionStatus == PP_OK_COMPLETIONPENDING)
    	{
    		usleep(100);
    	}

    	return connectionStatus;
    	*/
    	std::unique_lock<std::mutex> lck(mutex_c);
    	condVar_c.wait(lck);

		return connectionStatus;

    }

    void ConnectAsync(int32_t result, const pp::NetAddress &addr)
    {
    	TcpSocketAPI::Connect(addr);
        if (result != PP_OK) {
        	condVar_c.notify_one();
            return;
        }
    }

    int32_t Read(char* buffer, int32_t bytes_to_read) {
    	//readingStatus = PP_OK_COMPLETIONPENDING;
    	read_thread_.message_loop().PostWork(callback_.NewCallback(
  	      &TcpSocketSyncAPI::ReadAsync, buffer, bytes_to_read));
    	/*while(readingStatus == PP_OK_COMPLETIONPENDING)
    	{
    		usleep(100);
    	}

    	return readingStatus;
    	    	}
    	    	*/
		std::unique_lock<std::mutex> lck(mutex_r);
		condVar_r.wait(lck);

		return readingStatus;

    }

    void ReadAsync(int32_t result, char* buffer, int32_t bytes_to_read)
    {
        if (result != PP_OK) {
        	condVar_r.notify_one();
          return;
        }
		TcpSocketAPI::Read(buffer, bytes_to_read);
    }


    int32_t Write(char* buffer, int32_t bytes_to_write) {
    	//writingStatus = PP_OK_COMPLETIONPENDING;
    	socket_thread_.message_loop().PostWork(callback_.NewCallback(
  	      &TcpSocketSyncAPI::WriteAsync, buffer, bytes_to_write));
    	/*while(writingStatus == PP_OK_COMPLETIONPENDING)
    	{
    		usleep(100);
    	}*/
    	//return writingStatus;

		std::unique_lock<std::mutex> lck(mutex_w);
		condVar_w.wait(lck);

		return writingStatus;
    }

    void WriteAsync(int32_t result, char* buffer, int32_t bytes_to_write)
    {
        if (result != PP_OK) {
        	condVar_w.notify_one();
          return;
        }
    	TcpSocketAPI::Write(buffer, bytes_to_write);
    }

protected:
    virtual void TcpSocketDidOpen(int32_t result)
    {
    	connectionStatus = result;
    	condVar_c.notify_one();
    }

    virtual void TcpSocketDidRead(int32_t result)
    {
    	readingStatus = result;
    	condVar_r.notify_one();
    }

    virtual void TcpSocketDidWrite(int32_t result)
    {
    	writingStatus = result;
    	condVar_w.notify_one();
    }


private:
    pp::SimpleThread socket_thread_;
    pp::SimpleThread read_thread_;
    pp::CompletionCallbackFactory<TcpSocketSyncAPI> callback_;

    TcpSocketSyncAPI(const TcpSocketSyncAPI&);
    TcpSocketSyncAPI& operator=(const TcpSocketSyncAPI&);

    int32_t readingStatus;
    int32_t writingStatus;
    int32_t connectionStatus;


    std::mutex mutex_c;
    std::condition_variable condVar_c;

    std::mutex mutex_r;
    std::condition_variable condVar_r;

    std::mutex mutex_w;
    std::condition_variable condVar_w;

    int32_t TimeOut = 5000;
    pp::Instance * const m_ppinstance;
};

#endif /* SRC_TCPSOCKETSYNC_API_CPP_ */
