#define BLUETOOTH_MANAGER_HOST_TEST
#include "../src/system/bluetooth_manager.cpp"
#include <cassert>
#include <cstdio>
int forwarded=0,stoppedCount=0;
void onGap(esp_gap_ble_cb_event_t,esp_ble_gap_cb_param_t*){++forwarded;}
void onGatt(esp_gatts_cb_event_t,int,esp_ble_gatts_cb_param_t*){++forwarded;}
void onStop(){++stoppedCount;}
int main(){
 using namespace bluetooth_manager;
 assert(!acquire(Owner::AirMouse,"mouse",onGap,onGatt,onStop));
 setEnabled(true);
 Lease previous=0;
 for(int i=0;i<100;++i){
   auto l=acquire(i%2?Owner::AirMouse:Owner::BadUsb,"HID",onGap,onGatt,onStop);
   assert(l && l!=previous && owns(l));
   assert(!acquire(Owner::Other,"other",nullptr,nullptr,nullptr));
   release(previous); assert(owns(l));
   esp_ble_gatts_cb_param_t p;
   mock::gatts(ESP_GATTS_CONNECT_EVT,1,&p);
   int before=forwarded;
   release(l);assert(!owns(l) && !failed() && forwarded==before);
   int ends=mock::deinits;release(l);assert(mock::deinits==ends);
   previous=l;
 }
 auto l=acquire(Owner::AirMouse,"mouse",onGap,onGatt,onStop);
 setEnabled(false);assert(!owns(l) && !enabled());
 setEnabled(true);
 mock::timeout=true;l=acquire(Owner::AirMouse,"mouse",onGap,onGatt,onStop);
 auto start=millis();release(l);assert(failed() && millis()-start==1500);
 assert(!acquire(Owner::BadUsb,"keys",nullptr,nullptr,nullptr));
 mock::timeout=false;setEnabled(false);setEnabled(true);assert(!failed());
 mock::registerError=true;
 assert(!acquire(Owner::AirMouse,"mouse",onGap,onGatt,onStop));
 assert(owner()==Owner::None && failed());
 mock::registerError=false;setEnabled(false);setEnabled(true);
 mock::initError=true;assert(!acquire(Owner::AirMouse,"mouse",onGap,onGatt,onStop));
 assert(failed());mock::initError=false;setEnabled(true);
 l=acquire(Owner::AirMouse,"mouse",onGap,onGatt,onStop);assert(l);release(l);
 std::puts("Bluetooth manager: 100 exclusive sessions, stale leases, shutdown, timeout and retry passed");
}
