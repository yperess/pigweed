#include "pw_async2/dispatcher_base.h"
#include "pw_async2/poll.h"
#include "pw_sensor/config.h"

namespace pw::sensor {

pw::async2::Poll<> ConfigurationFuture::Pend(pw::async2::Context& cx) {
  if (last_result_.IsReady()) {
    return last_result_;
  }
  last_result_ = pend_impl_(cx_, in_, out_);
  if (!last_result_.IsReady()) {
    future_.Wait(cx);
  } else {
    future_.Wake();
  }
  return last_result_;
}

}  // namespace pw::sensor
