// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_PUBLIC_CPP_HID_FAKE_HID_MANAGER_H_
#define SERVICES_DEVICE_PUBLIC_CPP_HID_FAKE_HID_MANAGER_H_

#include <map>
#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/device/public/mojom/hid.mojom.h"

namespace device {

class FakeHidConnection : public mojom::HidConnection {
 public:
  explicit FakeHidConnection(mojom::HidDeviceInfoPtr device);
  ~FakeHidConnection() override;

  // mojom::HidConnection implementation:
  void Read(ReadCallback callback) override;
  void Write(uint8_t report_id,
             const std::vector<uint8_t>& buffer,
             WriteCallback callback) override;
  void GetFeatureReport(uint8_t report_id,
                        GetFeatureReportCallback callback) override;

  void SendFeatureReport(uint8_t report_id,
                         const std::vector<uint8_t>& buffer,
                         SendFeatureReportCallback callback) override;

 private:
  mojom::HidDeviceInfoPtr device_;
};

class FakeHidManager : public mojom::HidManager {
 public:
  FakeHidManager();
  ~FakeHidManager() override;

  void Bind(mojom::HidManagerRequest request);

  // mojom::HidManager implementation:
  void GetDevicesAndSetClient(mojom::HidManagerClientAssociatedPtrInfo client,
                              GetDevicesCallback callback) override;
  void GetDevices(GetDevicesCallback callback) override;
  void Connect(const std::string& device_guid,
               mojom::HidConnectionClientPtr connection_client,
               ConnectCallback callback) override;

  mojom::HidDeviceInfoPtr CreateAndAddDevice(uint16_t vendor_id,
                                             uint16_t product_id,
                                             const std::string& product_name,
                                             const std::string& serial_number,
                                             mojom::HidBusType bus_type);
  void AddDevice(mojom::HidDeviceInfoPtr device);
  void RemoveDevice(const std::string& guid);

 private:
  std::map<std::string, mojom::HidDeviceInfoPtr> devices_;
  mojo::AssociatedInterfacePtrSet<mojom::HidManagerClient> clients_;
  mojo::BindingSet<mojom::HidManager> bindings_;
};

}  // namespace device

#endif  // SERVICES_DEVICE_PUBLIC_CPP_HID_FAKE_HID_MANAGER_H_
