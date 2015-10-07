#ifndef SRC_COMPONENTS_INCLUDE_TELEMETRY_MONITOR_RESOURCE_OBSERVEABLE_H
#define SRC_COMPONENTS_INCLUDE_TELEMETRY_MONITOR_RESOURCE_OBSERVEABLE_H
namespace telemetry_monitor {

template <class TelemetryObserver>
class TelemetryObservable {
public:
    virtual void SetTelemetryObserver(TelemetryObserver* observer) = 0;
};
}  // namespace telemetry_monitor
#endif  // SRC_COMPONENTS_INCLUDE_TELEMETRY_MONITOR_RESOURCE_OBSERVEABLE_H
