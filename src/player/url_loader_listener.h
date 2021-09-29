#pragma once

#include <mutex>


class IURLLoaderHandlerListener
{
public:
  virtual ~IURLLoaderHandlerListener() = default;

  virtual bool ReceiveData(const char* buffer, int32_t num_bytes) = 0;
};
