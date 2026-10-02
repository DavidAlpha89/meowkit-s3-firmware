#define AIR_MOUSE_HID_HOST_TEST
#include "../src/app/app_06/air_mouse_hid.cpp"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

void bootSession() {
    CHECK(air_mouse_hid::begin());
    esp_ble_gatts_cb_param_t p{};
    p.reg.app_id=APP_ID; gatts(ESP_GATTS_REG_EVT,1,&p);
    uint16_t table[16]; for(int i=0;i<16;++i)table[i]=100+i;
    p.add_attr_tab={0,0,16,table}; gatts(ESP_GATTS_CREAT_ATTR_TAB_EVT,1,&p);
    p.add_attr_tab={1,0,5,table}; gatts(ESP_GATTS_CREAT_ATTR_TAB_EVT,1,&p);
    gatts(ESP_GATTS_START_EVT,1,&p); gatts(ESP_GATTS_START_EVT,1,&p);
    esp_ble_gap_cb_param_t g{};
    gap(ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT,&g);
    gap(ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT,&g);
    CHECK(!air_mouse_hid::connected());
    p.connect.conn_id=3; gatts(ESP_GATTS_CONNECT_EVT,1,&p);
    CHECK(air_mouse_hid::connected() && !air_mouse_hid::ready());
    g.ble_security.auth_cmpl.success=true; gap(ESP_GAP_BLE_AUTH_CMPL_EVT,&g);
    CHECK(!air_mouse_hid::ready());
    uint8_t c[]={1,0}; p.write={false,111,2,c}; gatts(ESP_GATTS_WRITE_EVT,1,&p);
    air_mouse_hid::tick(); CHECK(air_mouse_hid::ready());
}
int main() {
    using namespace air_mouse_hid;
    bootSession(); const auto initial=mock::identity;
    CHECK((initial[0]&0xC0)==0xC0);
    int starts=mock::inits; CHECK(begin()); CHECK(mock::inits==starts);
    buttons(1); mock::now+=10; tick(); CHECK(mock::sent.back()[0]==1);
    mock::now+=10; CHECK(move(12,-4)); CHECK(mock::sent.back()[0]==1);
    buttons(0); mock::failSend=true; mock::now+=10; tick(); CHECK(head!=tail);
    mock::failSend=false; tick(); CHECK(head==tail && mock::sent.back()[0]==0);
    clickRight(); mock::now+=10; tick(); CHECK(mock::sent.back()[0]==2);
    mock::now+=10; tick(); CHECK(mock::sent.back()[0]==0);
    buttons(1); mock::now+=10; tick();
    esp_ble_gatts_cb_param_t p{}; gatts(ESP_GATTS_DISCONNECT_EVT,1,&p); tick();
    CHECK(!ready() && head==tail && desiredButtons==0);
    const int advBefore=mock::adverts;
    end(); gatts(ESP_GATTS_DISCONNECT_EVT,1,&p);
    CHECK(mock::adverts==advBefore); CHECK(!move(1,1));
    int ends=mock::deinits; end(); CHECK(mock::deinits==ends);
    for(int i=0;i<30;++i){bootSession(); CHECK(mock::identity==initial); end();}
    bootSession();
    uint8_t bootMode=0, enabled[]={1,0};
    p.write={false,108,1,&bootMode}; gatts(ESP_GATTS_WRITE_EVT,1,&p); CHECK(!ready());
    p.write={false,115,2,enabled}; gatts(ESP_GATTS_WRITE_EVT,1,&p); CHECK(ready());
    mock::now+=10; CHECK(move(1,2)); CHECK(mock::sent.back().size()==3);
    end(); mock::failRegister=true; CHECK(!begin() && failed()); end();
    mock::failRegister=false; CHECK(begin()); mock::now+=5001; tick(); CHECK(failed()); end();
    std::puts("Air Mouse HID: pairing gates, drag/release, backpressure, boot mode, failure and 30 sessions passed");
}
