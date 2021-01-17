/*
 * IHTSPConnectionListener.h
 *
 *  Created on: Jan 6, 2021
 *      Author: jan.husak
 */

#pragma once


class IHttpDataListener
{
public:
  virtual ~IHttpDataListener() = default;
  virtual void AppendDataBytes(const char* buffer, int32_t num_bytes) = 0;
};
