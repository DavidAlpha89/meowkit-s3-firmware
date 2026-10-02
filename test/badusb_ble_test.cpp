#define BADUSB_BLE_HOST_TEST
#include "../src/app/app_08/badusb_ble.cpp"
#include <cassert>
#include <cstdio>
void boot(){
 assert(bu_ble_begin());
 esp_ble_gatts_cb_param_t p{};
 p.reg.app_id=APP_ID;gatts(ESP_GATTS_REG_EVT,1,&p);
 uint16_t t[25];for(int i=0;i<25;++i)t[i]=100+i;
 p.add_attr_tab={0,0,25,t};gatts(ESP_GATTS_CREAT_ATTR_TAB_EVT,1,&p);
 p.add_attr_tab={1,0,5,t};gatts(ESP_GATTS_CREAT_ATTR_TAB_EVT,1,&p);
 gatts(ESP_GATTS_START_EVT,1,&p);gatts(ESP_GATTS_START_EVT,1,&p);
 p.connect.conn_id=1;gatts(ESP_GATTS_CONNECT_EVT,1,&p);
 assert(!bu_ble_connected());
 esp_ble_gap_cb_param_t g{};g.ble_security.auth_cmpl.success=true;
 gap(ESP_GAP_BLE_AUTH_CMPL_EVT,&g);assert(!bu_ble_connected());
 uint8_t c[]={1,0};p.write={false,111,2,c};gatts(ESP_GATTS_WRITE_EVT,1,&p);
 assert(bu_ble_connected());
}
int main(){
 boot();bu_ble_write('A');
 assert(mock::sent.size()==2 && mock::sent[0][0]==2 && mock::sent[0][2]==4);
 for(auto b:mock::sent.back())assert(b==0);
 bu_ble_press(128);assert(mock::sent.back()[0]==1);
 bu_ble_end();for(auto b:mock::sent.back())assert(b==0);
 int n=mock::deinits;bu_ble_end();assert(mock::deinits==n);
 for(int i=0;i<30;++i){boot();bu_ble_write('x');bu_ble_end();}
 boot();mock::failSend=true;bu_ble_write('a');assert(bu_ble_failed());
 mock::failSend=false;bu_ble_end();for(auto b:mock::sent.back())assert(b==0);
 boot();esp_ble_gatts_cb_param_t p;gatts(ESP_GATTS_DISCONNECT_EVT,1,&p);
 auto count=mock::sent.size();bu_ble_print("must not replay");assert(mock::sent.size()==count);
 bu_ble_end();bluetooth_manager::setEnabled(false);assert(!bu_ble_begin());
 bluetooth_manager::setEnabled(true);boot();bluetooth_manager::setEnabled(false);
 assert(!bu_ble_connected());bu_ble_end();
 std::puts("BadUSB BLE: readiness, neutral release, send failure, disconnect and 30 sessions passed");
}
