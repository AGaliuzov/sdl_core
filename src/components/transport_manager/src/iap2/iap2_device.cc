#include "transport_manager/iap2/iap2_device.h"

namespace transport_manager {
namespace transport_adapter {

IAP2Device::IAP2Device(std::string name, DeviceUID device_uid)
    : Device(std::move(name), std::move(device_uid)) {}

bool IAP2Device::IsSameAs(const Device *other_device) const {
  if (!other_device)
    return false;

  return other_device->unique_device_id() == unique_device_id();
}

ApplicationList IAP2Device::GetApplicationList() const {
  return ApplicationList();
}

} // namespace transport_adapter
} // namesapce transport_manager
