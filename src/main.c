#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"

#include "transport/uart/ble_hci_uart.h"
#include "nimble/nimble_port.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

#include "esp_log.h"

static const char *device_name = "NimBLE-ESP32";

static void advertise(void);

static void
set_ble_addr(void)
{
    int rc;
    ble_addr_t addr;

    /* generate new non-resolvable private address */
    rc = ble_hs_id_gen_rnd(1, &addr);
    assert(rc == 0);

    /* set generated address */
    rc = ble_hs_id_set_rnd(addr.val);
    assert(rc == 0);
}

static int
adv_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("adv_event", "Advertising completed, termination code: %d\n",
                    event->adv_complete.reason);
        advertise();
        return 0;
    default:
        ESP_LOGE("adv_event", "Advertising event not handled\n");
        return 0;
    }
}

static void
advertise(void)
{
    int rc;
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;

    /* set adv parameters */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    memset(&fields, 0, sizeof(fields));

    /* Fill the fields with advertising data - flags, tx power level, name */
    fields.flags = BLE_HS_ADV_F_DISC_GEN;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    assert(rc == 0);

    ESP_LOGI("advertise", "Starting advertising...\n");

    /* As own address type we use hard-coded value, because we generate
       NRPA and by definition it's random */
    rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, 10000,
                           &adv_params, adv_event, NULL);
    assert(rc == 0);
}

static void
on_sync(void)
{
    set_ble_addr();

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
    ble_hci_uart_init();
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
