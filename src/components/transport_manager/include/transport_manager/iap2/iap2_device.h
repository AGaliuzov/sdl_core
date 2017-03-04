#pragma once

#include <string>

#include "transport_manager/transport_adapter/device.h"

namespace transport_manager {
namespace transport_adapter {

class IAP2Device: public Device {

public:
  /**
   * @brief IAP2Device
   * @param name
   * @param device_uid
   */
  IAP2Device(std::string name, DeviceUID device_uid);

private:
  /**
   * @brief IsSameAs
   * @param other_device
   * @return
   */
  bool IsSameAs(const Device* other_device) const final;

  /**
   * @brief GetApplicationList
   * @return
   */
  ApplicationList GetApplicationList() const final;
};

} // namespace transport_adapter
} // namesapce transport_manager
