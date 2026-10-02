#pragma once
#include <cstdint>
using esp_err_t=int;
using esp_gatt_if_t=int;
constexpr int ESP_OK=0,ESP_BT_STATUS_SUCCESS=0;
constexpr int ESP_BLUEDROID_STATUS_UNINITIALIZED=0,ESP_BLUEDROID_STATUS_ENABLED=2;
enum esp_gap_ble_cb_event_t {ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT, GAP_OTHER};
enum esp_gatts_cb_event_t {ESP_GATTS_CONNECT_EVT,ESP_GATTS_DISCONNECT_EVT,GATTS_OTHER};
struct esp_ble_gap_cb_param_t {struct{int status=0;}adv_stop_cmpl;};
struct esp_ble_gatts_cb_param_t {struct{uint8_t remote_bda[6]={};}connect;};
using esp_gap_ble_cb_t=void(*)(esp_gap_ble_cb_event_t,esp_ble_gap_cb_param_t*);
using esp_gatts_cb_t=void(*)(esp_gatts_cb_event_t,int,esp_ble_gatts_cb_param_t*);
namespace mock {
inline uint32_t now=0;
inline int state=0,inits=0,deinits=0;
inline bool timeout=false,registerError=false,initError=false,stopPending=false,disconnectPending=false;
inline esp_gap_ble_cb_t gap=nullptr;
inline esp_gatts_cb_t gatts=nullptr;
}
struct BLEDevice {
 static void init(const char*) {++mock::inits;if(!mock::initError)mock::state=2;}
 static void deinit(bool) {++mock::deinits;mock::state=0;mock::gap=nullptr;mock::gatts=nullptr;}
};
struct SerialStub {template<class... T>void printf(const char*,T...){}};
inline SerialStub Serial;
inline uint32_t millis(){return mock::now;}
inline int esp_bluedroid_get_status(){return mock::state;}
inline int esp_ble_gap_register_callback(esp_gap_ble_cb_t p){mock::gap=p;return mock::registerError?-1:0;}
inline int esp_ble_gatts_register_callback(esp_gatts_cb_t p){mock::gatts=p;return 0;}
inline int esp_ble_gap_stop_advertising(){mock::stopPending=true;return 0;}
inline int esp_ble_gap_stop_scanning(){return 0;}
inline int esp_ble_gap_disconnect(uint8_t*){mock::disconnectPending=true;return 0;}
inline void delay(int n) {
 mock::now+=n;
 if(mock::timeout)return;
 if(mock::stopPending && mock::gap){
   mock::stopPending=false;esp_ble_gap_cb_param_t p;
   mock::gap(GAP_OTHER,&p); // An old profile callback during shutdown is discarded.
   p.adv_stop_cmpl.status=1; // Already stopped is a completed command.
   mock::gap(ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT,&p);
 }
 if(mock::disconnectPending && mock::gatts){
   mock::disconnectPending=false;esp_ble_gatts_cb_param_t p;
   mock::gatts(ESP_GATTS_DISCONNECT_EVT,1,&p);
 }
}
