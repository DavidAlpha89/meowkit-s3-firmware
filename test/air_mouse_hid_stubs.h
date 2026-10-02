#pragma once
#include <cstdint>
#include <vector>
#include <array>
using esp_gatt_if_t=int;
constexpr int ESP_GATT_CHAR_PROP_BIT_WRITE=8, BLE_ADDR_TYPE_PUBLIC=0;
constexpr int ESP_OK=0, ESP_GATT_IF_NONE=-1, ESP_GATT_OK=0, ESP_BT_STATUS_SUCCESS=0;
constexpr int ESP_GATT_AUTO_RSP=0, ESP_UUID_LEN_16=2;
constexpr int ESP_GATT_PERM_READ=1, ESP_GATT_PERM_WRITE_ENCRYPTED=2, ESP_GATT_PERM_READ_ENCRYPTED=4;
constexpr int ESP_GATT_CHAR_PROP_BIT_READ=2, ESP_GATT_CHAR_PROP_BIT_WRITE_NR=4, ESP_GATT_CHAR_PROP_BIT_NOTIFY=16;
constexpr int ADV_TYPE_IND=0, BLE_ADDR_TYPE_RANDOM=1, ADV_CHNL_ALL=7, ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY=0;
constexpr int ESP_LE_AUTH_BOND=1, ESP_IO_CAP_NONE=3, ESP_BLE_ENC_KEY_MASK=1, ESP_BLE_ID_KEY_MASK=2;
constexpr int ESP_BLE_SM_AUTHEN_REQ_MODE=0, ESP_BLE_SM_IOCAP_MODE=1, ESP_BLE_SM_MAX_KEY_SIZE=2;
constexpr int ESP_BLE_SM_SET_INIT_KEY=3, ESP_BLE_SM_SET_RSP_KEY=4, ESP_BLE_SEC_ENCRYPT=1;
enum esp_gap_ble_cb_event_t {ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT,
 ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT,ESP_GAP_BLE_ADV_START_COMPLETE_EVT,
 ESP_GAP_BLE_SEC_REQ_EVT,ESP_GAP_BLE_AUTH_CMPL_EVT};
enum esp_gatts_cb_event_t {ESP_GATTS_REG_EVT,ESP_GATTS_CREAT_ATTR_TAB_EVT,ESP_GATTS_START_EVT,
 ESP_GATTS_CONNECT_EVT,ESP_GATTS_DISCONNECT_EVT,ESP_GATTS_CONGEST_EVT,ESP_GATTS_WRITE_EVT};
struct esp_ble_gap_cb_param_t {
 struct {int status=0;} adv_data_raw_cmpl,scan_rsp_data_raw_cmpl,adv_start_cmpl;
 struct {struct {uint8_t bd_addr[6]={};} ble_req;
 struct {bool success=false; uint8_t bd_addr[6]={}; uint8_t fail_reason=0;} auth_cmpl;} ble_security;
};
struct esp_ble_gatts_cb_param_t {
 struct {uint16_t app_id=0; int status=0;} reg;
 struct {unsigned svc_inst_id=0; int status=0; uint16_t num_handle=0; uint16_t* handles=nullptr;} add_attr_tab;
 struct {int status=0;} start;
 struct {uint16_t conn_id=0; uint8_t remote_bda[6]={};} connect;
 struct {uint8_t reason=0;} disconnect;
 struct {bool congested=false;} congest;
 struct {bool is_prep=false; uint16_t handle=0,len=0; uint8_t* value=nullptr;} write;
};
struct esp_gatts_attr_db_t {
 struct {int auto_rsp=0;} attr_control;
 struct {int uuid_length; uint8_t* uuid_p; uint16_t perm,max_length,length; uint8_t* value;} att_desc;
};
struct esp_ble_adv_params_t {int adv_int_min,adv_int_max,adv_type,own_addr_type,channel_map,adv_filter_policy;};
struct esp_ble_conn_update_params_t {uint8_t bda[6]; int min_int,max_int,latency,timeout;};
namespace mock {
inline uint32_t now=0;
inline int inits=0,deinits=0,adverts=0,stops=0,disconnections=0,intervals=0,encryptionRequests=0;
inline bool failSend=false,failRegister=false;
inline std::vector<std::vector<uint8_t>> sent;
inline std::array<uint8_t,6> identity{};
}
inline uint32_t millis(){return mock::now;}
inline void delay(int n){mock::now+=n;}
struct BLEDevice {
 static void init(const char*){++mock::inits;}
 static void deinit(bool){++mock::deinits;}
};
inline int esp_ble_gap_start_advertising(esp_ble_adv_params_t*){++mock::adverts;return 0;}
inline int esp_ble_gap_stop_advertising(){++mock::stops;return 0;}
inline int esp_ble_gap_disconnect(uint8_t*){++mock::disconnections;return 0;}
inline int esp_ble_gap_security_rsp(uint8_t*,bool){return 0;}
inline int esp_ble_gatts_create_attr_tab(esp_gatts_attr_db_t*,int,int,int){return 0;}
inline int esp_ble_gatts_start_service(uint16_t){return 0;}
inline int esp_ble_gatts_set_attr_value(uint16_t,uint16_t,uint8_t*){return 0;}
inline int esp_ble_set_encryption(uint8_t*,int){++mock::encryptionRequests;return 0;}
inline int esp_ble_gap_update_conn_params(esp_ble_conn_update_params_t*){++mock::intervals;return 0;}
inline int esp_ble_gatts_send_indicate(int,uint16_t,uint16_t,uint16_t n,uint8_t* p,bool){
 if(mock::failSend)return -1;
 mock::sent.emplace_back(p,p+n); return 0;
}
inline int esp_ble_gap_set_security_param(int,void*,int){return 0;}
inline const uint8_t* esp_bt_dev_get_address(){static uint8_t a[]={0x24,1,2,3,4,5};return a;}
inline int esp_ble_gap_set_rand_addr(uint8_t* p){for(int i=0;i<6;++i)mock::identity[i]=p[i];return 0;}
inline int esp_ble_gap_register_callback(void(*)(esp_gap_ble_cb_event_t,esp_ble_gap_cb_param_t*)){return 0;}
inline int esp_ble_gatts_register_callback(void(*)(esp_gatts_cb_event_t,int,esp_ble_gatts_cb_param_t*)){return 0;}
inline int esp_ble_gap_config_adv_data_raw(uint8_t*,int){return 0;}
inline int esp_ble_gap_config_scan_rsp_data_raw(uint8_t*,int){return 0;}
inline int esp_ble_gatts_app_register(int){return mock::failRegister?-1:0;}
