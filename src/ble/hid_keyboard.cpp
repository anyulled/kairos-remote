#include "ble/hid_keyboard.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string.h>

namespace {

struct HidKeyEvent {
    uint8_t modifier;
    uint8_t keycode;
};

struct InputReport {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
};

// Standard Boot-compatible HID Keyboard Report Descriptor (Report ID 1).
const uint8_t kHidReportMap[] = {
    USAGE_PAGE(1),      0x01, // Generic Desktop
    USAGE(1),           0x06, // Keyboard
    COLLECTION(1),      0x01, // Application
    REPORT_ID(1),       0x01,
    USAGE_PAGE(1),      0x07, // Key Codes
    USAGE_MINIMUM(1),   0xE0,
    USAGE_MAXIMUM(1),   0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1),     0x01,
    REPORT_COUNT(1),    0x08,
    HIDINPUT(1),        0x02, // Modifier byte
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x08,
    HIDINPUT(1),        0x01, // Reserved byte
    REPORT_COUNT(1),    0x05,
    REPORT_SIZE(1),     0x01,
    USAGE_PAGE(1),      0x08, // LEDs
    USAGE_MINIMUM(1),   0x01,
    USAGE_MAXIMUM(1),   0x05,
    HIDOUTPUT(1),       0x02, // Num/Caps/Scroll LEDs
    REPORT_COUNT(1),    0x01,
    REPORT_SIZE(1),     0x03,
    HIDOUTPUT(1),       0x01, // Padding
    REPORT_COUNT(1),    0x06,
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,
    USAGE_PAGE(1),      0x07,
    USAGE_MINIMUM(1),   0x00,
    USAGE_MAXIMUM(1),   0x65,
    HIDINPUT(1),        0x00, // 6-key rollover array
    END_COLLECTION(0),
};

NimBLEServer *s_server = nullptr;
NimBLEHIDDevice *s_hid = nullptr;
NimBLECharacteristic *s_inputReport = nullptr;
NimBLECharacteristic *s_outputReport = nullptr;
QueueHandle_t s_keyQueue = nullptr;
TaskHandle_t s_txTask = nullptr;
volatile bool s_connected = false;
uint8_t s_lastBatteryPct = 100;

void startFastAdvertising() {
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    if (adv && !adv->isAdvertising()) {
        adv->start();
    }
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) override {
        (void)desc;
        s_connected = true;
        // Request responsive connection interval (11.25 - 22.5 ms) for snappy slide transitions.
        if (pServer && desc) {
            pServer->updateConnParams(desc->conn_handle, 9, 18, 0, 200);
        }
        Serial.println("[BLE] Host connected");
    }

    void onDisconnect(NimBLEServer *pServer) override {
        (void)pServer;
        s_connected = false;
        if (s_keyQueue) {
            xQueueReset(s_keyQueue);
        }
        Serial.println("[BLE] Host disconnected, restarting advertising");
        startFastAdvertising();
    }
};

ServerCallbacks s_serverCallbacks;

void hidTxTask(void *arg) {
    (void)arg;
    HidKeyEvent ev = {};
    for (;;) {
        if (xQueueReceive(s_keyQueue, &ev, portMAX_DELAY) == pdTRUE) {
            if (!s_connected || !s_inputReport) {
                continue;
            }
            InputReport press = {};
            press.modifiers = ev.modifier;
            press.keys[0] = ev.keycode;
            s_inputReport->setValue(reinterpret_cast<uint8_t *>(&press), sizeof(press));
            s_inputReport->notify();

            vTaskDelay(pdMS_TO_TICKS(18));

            InputReport release = {};
            s_inputReport->setValue(reinterpret_cast<uint8_t *>(&release), sizeof(release));
            s_inputReport->notify();

            vTaskDelay(pdMS_TO_TICKS(12));
        }
    }
}

} // namespace

namespace BleHidKeyboard {

bool begin(const char *deviceName) {
    if (s_server) {
        return true;
    }

    if (!s_keyQueue) {
        s_keyQueue = xQueueCreate(16, sizeof(HidKeyEvent));
        if (!s_keyQueue) {
            return false;
        }
    }

    NimBLEDevice::init(deviceName ? deviceName : "Kairos-remote");
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    s_server = NimBLEDevice::createServer();
    s_server->setCallbacks(&s_serverCallbacks);
    s_server->advertiseOnDisconnect(true);

    s_hid = new NimBLEHIDDevice(s_server);
    s_inputReport = s_hid->inputReport(1);
    s_outputReport = s_hid->outputReport(1);

    s_hid->manufacturer("Kairos");
    s_hid->pnp(0x02, 0xE502, 0xA111, 0x0100);
    s_hid->hidInfo(0x00, 0x01);
    s_hid->reportMap(const_cast<uint8_t *>(kHidReportMap), sizeof(kHidReportMap));
    s_hid->startServices();
    s_hid->setBatteryLevel(s_lastBatteryPct);

    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    adv->setAppearance(HID_KEYBOARD);
    adv->addServiceUUID(s_hid->hidService()->getUUID());
    adv->setScanResponse(true);
    adv->setMinInterval(0x20); // 20 ms
    adv->setMaxInterval(0x40); // 40 ms
    adv->start();

    if (!s_txTask) {
        xTaskCreatePinnedToCore(hidTxTask, "kairos_hid_tx", 3072, nullptr, 2, &s_txTask, 0);
    }

    return true;
}

void end() {
    s_connected = false;
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    if (adv) {
        adv->stop();
    }
    NimBLEDevice::deinit(true);
    s_server = nullptr;
    s_hid = nullptr;
    s_inputReport = nullptr;
    s_outputReport = nullptr;
}

bool sendKey(uint8_t keycode, uint8_t modifier) {
    if (!s_connected || !s_keyQueue || keycode == 0) {
        return false;
    }
    const HidKeyEvent ev = {modifier, keycode};
    return xQueueSend(s_keyQueue, &ev, 0) == pdTRUE;
}

bool sendChar(char c) {
    uint8_t modifier = HID_MOD_NONE;
    uint8_t keycode = 0;
    if (!asciiToHidKey(c, modifier, keycode)) {
        return false;
    }
    return sendKey(keycode, modifier);
}

bool isConnected() {
    return s_connected;
}

void setBatteryLevel(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    s_lastBatteryPct = percent;
    if (s_hid) {
        s_hid->setBatteryLevel(percent);
    }
}

} // namespace BleHidKeyboard
