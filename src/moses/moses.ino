#include <SPI.h>
#include <mcp2515.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <string.h>

const byte rx_pin = 4;
const byte tx_pin = 5;

SoftwareSerial bt_serial(rx_pin, tx_pin);

struct can_frame canMsg;
MCP2515 mcp2515(10);

const uint32_t can_id_quilometragem = 0x419;
const uint32_t can_id_freio_de_mao = 0x419;
const uint32_t can_id_freio = 0x0fa;
const uint32_t can_id_cintos = 0x258;

JsonDocument can_json;

char old_buffer[128];

void setup() {
  Serial.begin(9600);
  bt_serial.begin(9600);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  can_json["freio_de_mao"] = "unk";
  can_json["freio"] = "unk";
  can_json["cintos"]["motorista"] = "unk";
  can_json["cintos"]["passageiro"] = "unk";

  delay(1000);

  Serial.println("\n");
  serializeJson(can_json, Serial);
  Serial.println("\n");
}

void loop() {
  bool is_enabled = false;

  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

    if (canMsg.can_id == can_id_freio_de_mao) {
      is_enabled = ((canMsg.data[0] & 0b1000) >> 3);

      if (is_enabled) {
        can_json["freio_de_mao"] = "on";
      } else {
        can_json["freio_de_mao"] = "off";
      }
    }

    if (canMsg.can_id == can_id_freio) {
      is_enabled = ((canMsg.data[0] & 0b1100) >> 2);

      if (is_enabled) {
        can_json["freio"] = "on";
      } else {
        can_json["freio"] = "off";
      }
    }

    if (canMsg.can_id == can_id_cintos) {

      is_enabled = ((canMsg.data[0] & 0b01000000) >> 6);
      if (!is_enabled) {
        can_json["cintos"]["motorista"] = "on";
      } else {
        can_json["cintos"]["motorista"] = "off";
      }

      is_enabled = ((canMsg.data[1] & 0b01000000) >> 6);
      if (!is_enabled) {
        can_json["cintos"]["passageiro"] = "on";
      } else {
        can_json["cintos"]["passageiro"] = "off";
      }
    }

    /*
    if (canMsg.can_id == can_id_quilometragem) {

    }
    */

    char buffer[128];
    String data;

    serializeJson(can_json, data);
    uint8_t data_len = data.length();

    data.toCharArray(buffer, data_len);

    // é preciso comparar o novo log com o antigo, se for
    // diferente os dados são enviados via Bluetooth
    if (strncmp(buffer, old_buffer, data_len) != 0) {
      for(uint8_t i = 0; i < data_len; i++) {
        bt_serial.write((char) buffer[i]);
        delay(8);
      }

      strncpy(old_buffer, buffer, data_len);
    }
  }
}