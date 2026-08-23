#include "csc_central.h"

#include <Arduino.h>
#include <bluefruit.h>
#include <string.h>

namespace bike {
namespace {

constexpr uint16_t kCscServiceUuid16 = 0x1816;
constexpr uint16_t kCscMeasurementUuid16 = 0x2A5B;
constexpr uint16_t kDefaultPairingS = 90;
constexpr uint8_t kCscMeasMaxLen = 11;

BLEClientService g_csc_service(kCscServiceUuid16);
BLEClientCharacteristic g_csc_measurement(kCscMeasurementUuid16);

CscCentral* g_client = nullptr;

volatile uint8_t g_meas_buf[kCscMeasMaxLen] = {};
volatile uint8_t g_meas_len = 0;
volatile bool g_meas_ready = false;
volatile bool g_csc_connected = false;
char g_peer_name[kCscBondNameSize] = {};
ble_gap_addr_t g_peer_addr = {};

void copyName(char* dest, size_t dest_len, const char* src) {
  if (dest == nullptr || dest_len == 0) return;
  memset(dest, 0, dest_len);
  if (src == nullptr) return;
  size_t n = 0;
  while (src[n] != '\0' && n + 1u < dest_len) {
    dest[n] = src[n];
    ++n;
  }
}

bool parseAdvertisedName(ble_gap_evt_adv_report_t* report, char* name,
                         size_t name_len) {
  if (name == nullptr || name_len == 0) return false;
  memset(name, 0, name_len);
  uint8_t buffer[31] = {};
  uint8_t len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, buffer, sizeof(buffer));
  if (len == 0) {
    len = Bluefruit.Scanner.parseReportByType(
        report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, buffer, sizeof(buffer));
  }
  if (len == 0) return false;
  if (len >= name_len) len = static_cast<uint8_t>(name_len - 1u);
  memcpy(name, buffer, len);
  name[len] = '\0';
  return true;
}

bool reportMatchesBond(ble_gap_evt_adv_report_t* report,
                       const CscBondData& bond) {
  if (!cscBondIsValid(bond) || report == nullptr) return false;
  return report->peer_addr.addr_type == bond.address_type &&
         memcmp(report->peer_addr.addr, bond.address, kCscAddressSize) == 0;
}

void onCscNotify(BLEClientCharacteristic*, uint8_t* data, uint16_t len) {
  if (data == nullptr || len == 0) return;
  const uint8_t copy_len =
      len > kCscMeasMaxLen ? kCscMeasMaxLen : static_cast<uint8_t>(len);
  memcpy(const_cast<uint8_t*>(g_meas_buf), data, copy_len);
  g_meas_len = copy_len;
  g_meas_ready = true;
}

void onCentralConnect(uint16_t conn_handle) {
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  if (conn == nullptr) {
    Bluefruit.disconnect(conn_handle);
    return;
  }

  char name[32] = {};
  conn->getPeerName(name, sizeof(name));
  copyName(g_peer_name, sizeof(g_peer_name), name);
  g_peer_addr = conn->getPeerAddr();

  if (!g_csc_service.discover(conn_handle)) {
    Serial.println("CSC: no 0x1816 service, disconnect");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  if (!g_csc_measurement.discover()) {
    Serial.println("CSC: no measurement char, disconnect");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  g_csc_measurement.enableNotify();
  g_csc_connected = true;

  Serial.print("CSC: connected ");
  Serial.println(g_peer_name[0] != '\0' ? g_peer_name : "(unnamed)");
}

void onCentralDisconnect(uint16_t, uint8_t reason) {
  g_csc_connected = false;
  g_meas_ready = false;
  if (g_client != nullptr) g_client->handleDisconnect();
  Serial.print("CSC: disconnect reason=0x");
  Serial.println(reason, HEX);
}

void onScanReport(ble_gap_evt_adv_report_t* report) {
  if (g_client == nullptr || report == nullptr) return;
  if (Bluefruit.Central.connected()) {
    return;
  }

  char name[32] = {};
  parseAdvertisedName(report, name, sizeof(name));
  const bool uuid_ok =
      Bluefruit.Scanner.checkReportForUuid(report, g_csc_service.uuid);
  if (g_client->shouldConnectTo(report, name, uuid_ok)) {
    Bluefruit.Central.connect(report);
    return;
  }
  Bluefruit.Scanner.resume();
}

}  // namespace

bool CscCentral::begin() {
  g_client = this;
  g_csc_service.begin();
  g_csc_measurement.setNotifyCallback(onCscNotify);
  g_csc_measurement.begin();

  Bluefruit.Central.setConnectCallback(onCentralConnect);
  Bluefruit.Central.setDisconnectCallback(onCentralDisconnect);

  Bluefruit.Scanner.setRxCallback(onScanReport);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);
  Bluefruit.Scanner.useActiveScan(true);
  started_ = true;

  if (cscBondIsValid(bond_)) {
    startScanning();
  } else {
    startPairing(kDefaultPairingS, millis());
  }
  return true;
}

void CscCentral::setBond(const CscBondData& bond) { bond_ = bond; }

void CscCentral::startPairing(uint16_t duration_s, uint32_t now_ms) {
  if (!started_) return;
  if (duration_s == 0) duration_s = kDefaultPairingS;
  pairing_ = true;
  pairing_until_ms_ = now_ms + static_cast<uint32_t>(duration_s) * 1000u;
  Serial.print("CSC: pairing window s=");
  Serial.println(duration_s);
  startScanning();
}

void CscCentral::forgetBond() {
  if (Bluefruit.Central.connected()) {
    Bluefruit.disconnect(g_csc_service.connHandle());
  }
  clearCscBond(bond_);
  tracker_.reset();
  pairing_ = false;
  bond_save_pending_ = true;
  stopScanning();
}

void CscCentral::setPhoneConnected(bool connected) {
  phone_connected_ = connected;
  if (connected) {
    stopScanning();
    Serial.println("CSC: scan paused (phone connected)");
    return;
  }
  if (!g_csc_connected && (cscBondIsValid(bond_) || pairing_)) {
    startScanning();
    Serial.println("CSC: scan resumed (phone gone)");
  }
}

void CscCentral::startScanning() {
  if (!started_) return;
  if (phone_connected_) return;
  if (Bluefruit.Scanner.isRunning()) return;
  if (Bluefruit.Central.connected()) return;
  Bluefruit.Scanner.start(0);
}

void CscCentral::stopScanning() {
  if (Bluefruit.Scanner.isRunning()) {
    Bluefruit.Scanner.stop();
  }
}

bool CscCentral::pairingActive(uint32_t now_ms) const {
  return pairing_ && static_cast<int32_t>(now_ms - pairing_until_ms_) < 0;
}

void CscCentral::service(uint32_t now_ms) {
  if (!started_) return;

  if (pairing_ && !pairingActive(now_ms)) {
    pairing_ = false;
    Serial.println("CSC: pairing window closed");
    if (!cscBondIsValid(bond_) && !g_csc_connected) {
      stopScanning();
    }
  }

  if (g_meas_ready) {
    uint8_t local[kCscMeasMaxLen];
    const uint8_t len = g_meas_len;
    memcpy(local, const_cast<uint8_t*>(g_meas_buf), len);
    g_meas_ready = false;
    CscMeasurement measurement;
    if (parseCscMeasurement(local, len, measurement)) {
      tracker_.ingest(measurement, now_ms);
    }
  }
  tracker_.poll(now_ms);

  if (g_csc_connected && !cscBondIsValid(bond_)) {
    CscBondData next = {};
    memcpy(next.address, g_peer_addr.addr, kCscAddressSize);
    next.address_type = g_peer_addr.addr_type;
    next.flags = kCscBondFlagValid;
    copyName(next.name, sizeof(next.name), g_peer_name);
    bond_ = next;
    bond_save_pending_ = true;
    pairing_ = false;
    Serial.println("CSC: bond saved");
  }

  if (!g_csc_connected && !phone_connected_ &&
      (cscBondIsValid(bond_) || pairingActive(now_ms))) {
    startScanning();
  }
}

CscSnapshot CscCentral::snapshot(uint32_t now_ms) const {
  CscSnapshot snap;
  snap.connected = g_csc_connected;
  snap.pairing = pairingActive(now_ms);
  snap.bonded = cscBondIsValid(bond_);
  snap.wheel_present = tracker_.wheelPresent();
  snap.crank_present = tracker_.crankPresent();
  snap.cadence_valid = tracker_.cadenceValid();
  snap.speed_source_active = tracker_.speedSourceActive(now_ms);
  snap.cadence_x10 = tracker_.cadenceX10();
  snap.last_crank_event_age_ms = tracker_.lastCrankEventAgeMs(now_ms);
  snap.last_wheel_event_age_ms = tracker_.lastWheelEventAgeMs(now_ms);
  copyName(snap.name, sizeof(snap.name),
           g_peer_name[0] != '\0' ? g_peer_name : bond_.name);
  return snap;
}

bool CscCentral::takePendingBondSave(CscBondData& out) {
  if (!bond_save_pending_) return false;
  out = bond_;
  bond_save_pending_ = false;
  return true;
}

CscWheelDelta CscCentral::takeWheelDelta() { return tracker_.takeWheelDelta(); }

bool CscCentral::wheelSpeedSourceActive(uint32_t now_ms) const {
  return tracker_.speedSourceActive(now_ms);
}

bool CscCentral::connected() const { return g_csc_connected; }

void CscCentral::handleDisconnect() { tracker_.onDisconnect(); }

bool CscCentral::shouldConnectTo(ble_gap_evt_adv_report_t* report,
                                 const char* name, bool has_csc_uuid) const {
  if (report == nullptr) return false;
  const bool bonded = cscBondIsValid(bond_);
  if (bonded) return reportMatchesBond(report, bond_);
  if (!pairing_) return false;
  return has_csc_uuid || cscNameLooksLikeCycplus(name);
}

}  // namespace bike
