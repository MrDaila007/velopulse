#pragma once

#include <bluefruit.h>
#include <stdint.h>

#include "csc_measurement.h"

namespace bike {

// Arduino/Bluefruit CSC GATT client (central). BikeComp stays a peripheral for
// the companion app and concurrently connects to a CYCPLUS C3/S3 (or any CSC
// sensor) as a central.
class CscCentral {
 public:
  bool begin();
  void setBond(const CscBondData& bond);
  const CscBondData& bond() const { return bond_; }

  void startPairing(uint16_t duration_s, uint32_t now_ms);
  void forgetBond();
  void service(uint32_t now_ms);

  CscSnapshot snapshot(uint32_t now_ms) const;
  bool takePendingBondSave(CscBondData& out);
  CscWheelDelta takeWheelDelta();
  bool wheelSpeedSourceActive(uint32_t now_ms) const;
  bool connected() const;
  bool shouldConnectTo(ble_gap_evt_adv_report_t* report, const char* name,
                       bool has_csc_uuid) const;
  void handleDisconnect();

 private:
  void startScanning();
  void stopScanning();
  bool pairingActive(uint32_t now_ms) const;

  CscBondData bond_{};
  CscMotionTracker tracker_{};
  bool started_ = false;
  bool pairing_ = false;
  bool bond_save_pending_ = false;
  uint32_t pairing_until_ms_ = 0;
};

}  // namespace bike
