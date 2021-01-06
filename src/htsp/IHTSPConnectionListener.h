/*
 * IHTSPConnectionListener.h
 *
 *  Created on: Jan 6, 2021
 *      Author: jan.husak
 */

#pragma once

#include <mutex>

extern "C"
{
#include "libhts/htsmsg.h"
}


class IHTSPConnectionListener
{
public:
  virtual ~IHTSPConnectionListener() = default;

  virtual void Disconnected() = 0;
  virtual bool Connected(std::unique_lock<std::recursive_mutex>& lock) = 0;
  virtual bool ProcessMessage(const std::string& method, htsmsg_t* msg) = 0;
  virtual void ConnectionStateChange(const std::string& connectionString,
                                     //PVR_CONNECTION_STATE newState,
                                     const std::string& message) = 0;
};
