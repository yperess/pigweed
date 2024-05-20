#include <zephyr/rtio/rtio.h>

#include "pw_sensor/context.h"

namespace pw::sensor::zephyr {

class ZephyrSensorContext : public SensorContextBase {
  public:
  ZephyrSensorContext() {}
};

}  // namespace pw::sensor::zephyr
