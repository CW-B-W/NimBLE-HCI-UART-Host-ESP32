#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "os/os.h"
#include "sysinit/sysinit.h"
#include "log/log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

#include "nimble/nimble_port.h"

#include "esp_log.h"

static uint8_t g_own_addr_type;
static uint16_t conn_handle;
static const char *device_name = "NimBLE-ESP32";

/* adv_event() calls advertise(), so forward declaration is required */
static void advertise(void);

static int
adv_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("adv_event", "Advertising completed, termination code: %d\n",
                    event->adv_complete.reason);
        advertise();
        break;
    case BLE_GAP_EVENT_CONNECT:
        assert(event->connect.status == 0);
        ESP_LOGI("adv_event", "connection %s; status=%d\n",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
        conn_handle = event->connect.conn_handle;
        break;
    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        /* connected device requests update of connection parameters,
           and these are being filled in - NULL sets default values */
        ESP_LOGI("adv_event", "updating conncetion parameters...\n");
        event->conn_update_req.conn_handle = conn_handle;
        event->conn_update_req.peer_params = NULL;
        ESP_LOGI("adv_event", "connection parameters updated!\n");
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("adv_event", "disconnect; reason=%d\n",
        event->disconnect.reason);

        /* reset conn_handle */
        conn_handle = BLE_HS_CONN_HANDLE_NONE;

        /* Connection terminated; resume advertising */
        advertise();
        break;
    default:
        ESP_LOGE("adv_event", "Advertising event not handled,"
                    "event code: %u\n", event->type);
        break;
    }
    return 0;
}

static void
advertise(void)
{
    int rc;

    /* set adv parameters */
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    /* advertising payload is split into advertising data and advertising
       response, because all data cannot fit into single packet; name of device
       is sent as response to scan request */
    struct ble_hs_adv_fields rsp_fields;

    /* fill all fields and parameters with zeros */
    memset(&adv_params, 0, sizeof(adv_params));
    memset(&fields, 0, sizeof(fields));
    memset(&rsp_fields, 0, sizeof(rsp_fields));

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = BLE_UUID128(BLE_UUID128_DECLARE(
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff));
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 0;;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    rsp_fields.name = (uint8_t *)device_name;
    rsp_fields.name_len = strlen(device_name);
    rsp_fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    assert(rc == 0);

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);

    ESP_LOGI("advertise","Starting advertising...\n");

    rc = ble_gap_adv_start(g_own_addr_type, NULL, 100,
                           &adv_params, adv_event, NULL);
    assert(rc == 0);
}

static void
on_sync(void)
{
    int rc;

    /* g_own_addr_type will store type of addres our BSP uses */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);
    rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
    assert(rc == 0);
    /* begin advertising */
    advertise();
}

static void
on_reset(int reason)
{
    ESP_LOGI("on_reset", "Resetting state; reason=%d\n", reason);
}

void sys_init()
{
    nimble_port_init();
}

void app_main()
{
    sys_init();

    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    int rc = ble_svc_gap_device_name_set(device_name);
    assert(rc == 0);

    nimble_port_run();
}
