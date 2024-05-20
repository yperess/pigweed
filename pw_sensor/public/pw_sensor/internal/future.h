#pragma once

#include "pw_async2/dispatcher_base.h"
#include "pw_containers/intrusive_list.h"

namespace pw::sensor {

class ConfigurationFuture;

namespace internal {

class Future : public pw::IntrusiveList<Future>::Item {
 public:
  friend class pw::sensor::ConfigurationFuture;

 protected:
  void Wait(async2::Context& cx) {
    Wake();
    waker_ = cx.GetWaker(async2::WaitReason::Unspecified());
  }
  void Wake() {
    if (waker_.has_value()) {
      std::move(*waker_).Wake();
    }
  }

 private:
  std::optional<async2::Waker> waker_ = std::nullopt;
};

}  // namespace internal
}  // namespace pw::sensor
