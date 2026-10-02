#pragma once
// Profile-unit-test stand-in. The manager itself is tested separately.
namespace bluetooth_manager {
enum class Owner { None, AirMouse, BadUsb, Other };
using Lease=uint32_t;
inline bool allowed=true;
inline Lease current=0,sequence=0;
inline void (*stop)()=nullptr;
inline bool owns(Lease l) { return l && l==current; }
inline bool enabled() { return allowed; }
inline bool failed() { return false; }
inline Lease acquire(Owner,const char* name,
    void(*)(esp_gap_ble_cb_event_t,esp_ble_gap_cb_param_t*),
    void(*)(esp_gatts_cb_event_t,esp_gatt_if_t,esp_ble_gatts_cb_param_t*),void(*s)()) {
    if(!allowed || current) return 0;
    BLEDevice::init(name); stop=s; current=++sequence; return current;
}
inline void release(Lease l) {
    if(!owns(l)) return;
    if(stop)stop();
    current=0; BLEDevice::deinit(false);
}
inline void setEnabled(bool on) { if(!on)release(current); allowed=on; }
}
