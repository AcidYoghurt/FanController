#include <cstdio>
#include <cstring>

extern "C" {
using Device = void *;
static unsigned lastSpeed = 0;
static unsigned defaultCalls = 0;

int nvmlInit_v2() { return 0; }
int nvmlShutdown() { return 0; }
int nvmlDeviceGetCount_v2(unsigned *count) {
  *count = 1;
  return 0;
}
int nvmlDeviceGetHandleByIndex_v2(unsigned index, Device *device) {
  if (index != 0)
    return 1;
  *device = reinterpret_cast<Device>(1);
  return 0;
}
int nvmlDeviceGetName(Device, char *name, unsigned length) {
  std::snprintf(name, length, "Fake NVIDIA GPU");
  return 0;
}
int nvmlDeviceGetUUID(Device, char *uuid, unsigned length) {
  std::snprintf(uuid, length, "GPU-FAKE-0001");
  return 0;
}
int nvmlDeviceGetTemperature(Device, unsigned, unsigned *temperature) {
  *temperature = 62;
  return 0;
}
int nvmlDeviceGetNumFans(Device, unsigned *count) {
  *count = 2;
  return 0;
}
int nvmlDeviceGetFanSpeed_v2(Device, unsigned fan, unsigned *speed) {
  *speed = fan == 0 ? 35 : 42;
  return 0;
}
struct FanInfo { unsigned version; unsigned fan; unsigned speed; };
int nvmlDeviceGetFanSpeedRPM(Device, FanInfo *info) {
  info->speed = 1450 + info->fan * 120;
  return 0;
}
int nvmlDeviceGetMinMaxFanSpeed(Device, unsigned *minimum, unsigned *maximum) {
  *minimum = 20;
  *maximum = 100;
  return 0;
}
int nvmlDeviceSetFanSpeed_v2(Device, unsigned fan, unsigned speed) {
  lastSpeed = speed + fan * 1000;
  return 0;
}
int nvmlDeviceSetDefaultFanSpeed_v2(Device, unsigned) {
  ++defaultCalls;
  return 0;
}
const char *nvmlErrorString(int) { return "fake error"; }
unsigned fakeNvmlLastSpeed() { return lastSpeed; }
unsigned fakeNvmlDefaultCalls() { return defaultCalls; }
void fakeNvmlReset() { lastSpeed = 0; defaultCalls = 0; }
}
