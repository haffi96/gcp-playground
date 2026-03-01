#pragma once

#include <future>
#include <string>

class IPubSubPublisher {
 public:
  virtual ~IPubSubPublisher() = default;
  virtual std::future<bool> Publish(std::string payload) = 0;
};
