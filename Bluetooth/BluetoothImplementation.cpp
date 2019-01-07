#include "BluetoothImplementation.h"

namespace WPEFramework {

namespace Plugin {

    bool BluetoothImplementation::ConfigureBTAdapter()
    {
        int mgmtSocket;
        sockaddr_hci hciSocketAddr;
        struct mgmt_hdr mgmtHeader;
        struct mgmt_mode setMode;
        struct iovec iov[2];

        // Opening MGMT Socket.
        mgmtSocket = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);
        if (mgmtSocket < 0) {
            TRACE(Trace::Error, ("Failed to Open MGMT Socket"));
            return false;
        }

        memset(&hciSocketAddr, 0, sizeof(hciSocketAddr));
        hciSocketAddr.hci_family = AF_BLUETOOTH;
        hciSocketAddr.hci_dev = HCI_DEV_NONE;
        hciSocketAddr.hci_channel = HCI_CHANNEL_CONTROL;

        if (bind(mgmtSocket, (sockaddr *)&hciSocketAddr, sizeof(hciSocketAddr)) < 0) {
            TRACE(Trace::Error, ("Failed to bind MGMT Socket"));
            close(mgmtSocket);
            return false;
        }

        mgmtHeader.index = htobs(ADAPTER_INDEX);
        mgmtHeader.len = htobs(ONE_BYTE);
        setMode.val = htobs(ENABLE_MODE);

        iov[0].iov_base = &mgmtHeader;
        iov[0].iov_len = MGMT_HDR_SIZE;
        iov[1].iov_base = &setMode;
        iov[1].iov_len = ONE_BYTE;

        // Switch on the BT Adaptor.
        mgmtHeader.opcode = htobs(MGMT_OP_SET_POWERED);
        if (writev(mgmtSocket, iov, 2) < 0) {
            TRACE(Trace::Error, ("Failed to power on bluetooth adaptor"));
            close(mgmtSocket);
            return false;
        }

        // Enable Bondable on adaptor.
        mgmtHeader.opcode = htobs(MGMT_OP_SET_BONDABLE);
        if (writev(mgmtSocket, iov, 2) < 0) {
            TRACE(Trace::Error, ("Failed to enable Bondable"));
            close(mgmtSocket);
            return false;
        }

        // Enable Simple Secure Simple Pairing.
        mgmtHeader.opcode = htobs(MGMT_OP_SET_SSP);
        if (writev(mgmtSocket, iov, 2) < 0) {
            TRACE(Trace::Error, ("Failed to enable Simple Secure Simple Pairing"));
            close(mgmtSocket);
            return false;
        }

        // Enable Low Energy
        mgmtHeader.opcode = htobs(MGMT_OP_SET_LE);
        if (writev(mgmtSocket, iov, 2) < 0) {
            TRACE(Trace::Error, ("Failed to enable Low Energy"));
            close(mgmtSocket);
            return false;
        }

        // Enable Secure Connections
        mgmtHeader.opcode = htobs(MGMT_OP_SET_SECURE_CONN);
        if (writev(mgmtSocket, iov, 2) < 0) {
            TRACE(Trace::Error, ("Failed to enable Secure Connections"));
            close(mgmtSocket);
            return false;
        }

        sleep(1);
        close(mgmtSocket);

        return true;
    }

    void BluetoothImplementation::GetDeviceName(uint8_t* inputData, ssize_t inputDataLength, char* deviceName)
    {
        ssize_t offset = 0;

        while (offset < inputDataLength) {
            uint8_t fieldLength = inputData[0];
            ssize_t nameLength;

            /* Check for the end of EIR */
            if (fieldLength == 0)
                break;

            if (offset + fieldLength > inputDataLength)
                break;

            switch (inputData[1]) {
                case EIR_NAME_SHORT:
                case EIR_NAME_COMPLETE:
                    nameLength = fieldLength - 1;
                    memcpy(deviceName, &inputData[2], nameLength);
                    return;
            }

            offset += fieldLength + 1;
            inputData += fieldLength + 1;
        }
    }

    void BluetoothImplementation::ScanningThread()
    {
        unsigned char hciEventBuf[HCI_MAX_EVENT_SIZE];
        char deviceAddress[20];
        char deviceName[50];
        struct hci_filter oldHciFilter;
        struct hci_filter newHciFilter;
        socklen_t oldHciFilterLen;
        evt_le_meta_event *leMetaEvent;
        le_advertising_info *leAdvertisingInfo;
        BTDeviceInfo jsonDiscoveredDevice;

        // Get old HCI filter.
        oldHciFilterLen = sizeof(oldHciFilter);
        getsockopt(_hciSocket, SOL_HCI, HCI_FILTER, &oldHciFilter, &oldHciFilterLen);

        // Setup new HCI filter.
        hci_filter_clear(&newHciFilter);
        hci_filter_set_ptype(HCI_EVENT_PKT, &newHciFilter);
        hci_filter_set_event(EVT_LE_META_EVENT, &newHciFilter);
        setsockopt(_hciSocket, SOL_HCI, HCI_FILTER, &newHciFilter, sizeof(newHciFilter));

        TRACE(Trace::Information, ("Trying to Lock _scanningThread"));
        _scanningThreadLock.Lock();
        TRACE(Trace::Information, ("Locked scanningThread"));

        while (_scanning) {
            read(_hciSocket, hciEventBuf, sizeof(hciEventBuf));
            leMetaEvent = (evt_le_meta_event *)(hciEventBuf + (1 + HCI_EVENT_HDR_SIZE));

            if (leMetaEvent->subevent != 0x02)
                continue;

            leAdvertisingInfo = (le_advertising_info *)(leMetaEvent->data + 1);
            ba2str(&leAdvertisingInfo->bdaddr,deviceAddress);

            memset(deviceName, 0, sizeof(deviceName));
            GetDeviceName(leAdvertisingInfo->data, leAdvertisingInfo->length, deviceName);

            if (_discoveredDeviceIdMap.find(deviceAddress) == _discoveredDeviceIdMap.end()) {
                TRACE(Trace::Information, ("Discovered Device Name : [%s] Address : [%s]", deviceName, deviceAddress));
                _discoveredDeviceIdMap.insert(std::make_pair(deviceAddress, deviceName));
                jsonDiscoveredDevice.Name = deviceName;
                jsonDiscoveredDevice.Address = deviceAddress;
                _jsonDiscoveredDevices.Add(jsonDiscoveredDevice);
            }
        }

        setsockopt(_hciSocket, SOL_HCI, HCI_FILTER, &oldHciFilter, sizeof(oldHciFilter));
        TRACE(Trace::Information, ("Trying to UnLock scanningThread"));
        _scanningThreadLock.Unlock();
        TRACE(Trace::Information, ("UnLocked scanningThread"));
    }

    bool BluetoothImplementation::Scan()
    {
        std::thread scanningThread;

        if (!_scanning) {
            TRACE(Trace::Information, ("Start BT Scan"));
            // Clearing previously discovered devices.
            _discoveredDeviceIdMap.clear();
            _jsonDiscoveredDevices.Clear();

            if (hci_le_set_scan_parameters(_hciSocket, SCAN_TYPE, htobs(SCAN_INTERVAL), htobs(SCAN_WINDOW), OWN_TYPE, SCAN_FILTER_POLICY, SCAN_TIMEOUT) < 0) {
                TRACE(Trace::Error, ("Failed to Set Scan Parameters"));
                return false;
            }

            if (hci_le_set_scan_enable(_hciSocket, SCAN_ENABLE, SCAN_FILTER_DUPLICATES, SCAN_TIMEOUT) < 0) {
                TRACE(Trace::Error, ("Failed to Start Scan"));
                return false;
            }

            _scanning = true;
            // Starting Scan Thread.
            scanningThread = std::thread(&BluetoothImplementation::ScanningThread, this);
            scanningThread.detach();

            TRACE(Trace::Information, ("BT Scan Started"));
            return true;
        } else {
            TRACE(Trace::Error, ("Scanning already started!"));
            return false;
        }
    }

    bool BluetoothImplementation::StopScan()
    {
        if (_scanning) {
            TRACE(Trace::Information, ("Stop BT Scan"));
            _scanning = false;
            TRACE(Trace::Information, ("Trying to Lock _scanningThread"));
            _scanningThreadLock.Lock();
            TRACE(Trace::Information, ("Locked scanningThread"));
            if (hci_le_set_scan_enable(_hciSocket, SCAN_DISABLE, SCAN_FILTER_DUPLICATES, SCAN_TIMEOUT) < 0) {
                TRACE(Trace::Error, ("Failed to Stop Scan"));
                TRACE(Trace::Information, ("Trying to UnLock scanningThread"));
                _scanningThreadLock.Unlock();
                TRACE(Trace::Information, ("UnLocked scanningThread"));
                return false;
            }
            TRACE(Trace::Information, ("Trying to UnLock scanningThread"));
            _scanningThreadLock.Unlock();
            TRACE(Trace::Information, ("UnLocked scanningThread"));
            TRACE(Trace::Information, ("Stopped BT Scan"));
            return true;
        } else {
            TRACE(Trace::Error, ("Scanning is not started!"));
            return false;
        }
    }

    string BluetoothImplementation::DiscoveredDevices()
    {
        std::string deviceInfoList;

        _jsonDiscoveredDevices.ToString(deviceInfoList);
        return deviceInfoList;
    }

    bool BluetoothImplementation::PairDevice(string deviceId, uint32_t& l2capSocket, sockaddr_l2& destinationSocket)
    {

        struct sockaddr_l2 sourceSocket;
        struct bt_security btSecurity;
        bdaddr_t sourceAddress;
        bdaddr_t destinationAddress;

        l2capSocket = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
        if (l2capSocket < 0) {
            TRACE(Trace::Error, ("Failed to Open L2CAP Socket"));
            return false;
        }

        memset(&sourceSocket, 0, sizeof(sourceSocket));
        hci_devba(_hciHandle, &sourceAddress);
        sourceSocket.l2_bdaddr = sourceAddress;
        sourceSocket.l2_family=AF_BLUETOOTH;
        sourceSocket.l2_cid = htobs(LE_ATT_CID);
        sourceSocket.l2_bdaddr_type = BDADDR_LE_PUBLIC;

        if (bind(l2capSocket, (sockaddr*)&sourceSocket , sizeof(sourceSocket)) < 0) {
            TRACE(Trace::Error, ("Failed to Bind the Device [%s]", deviceId.c_str()));
            close(l2capSocket);
            return false;
        }

        memset(&destinationSocket, 0, sizeof(destinationSocket));
        destinationSocket.l2_family = AF_BLUETOOTH;
        destinationSocket.l2_psm = 0;
        destinationSocket.l2_cid = htobs(LE_ATT_CID);
        destinationSocket.l2_bdaddr_type = BDADDR_LE_PUBLIC;
        str2ba(deviceId.c_str(), &destinationSocket.l2_bdaddr);

        memset(&btSecurity, 0, sizeof(btSecurity));
        btSecurity.level = BT_SECURITY_MEDIUM;
        if (setsockopt(l2capSocket, SOL_BLUETOOTH, BT_SECURITY, &btSecurity, sizeof(btSecurity))) {
            TRACE(Trace::Error, ("Failed to set L2CAP Security level for Device [%s]", deviceId.c_str()));
            close(l2capSocket);
            return false;
        }

        return true;
    }

    bool BluetoothImplementation::Pair(string deviceId)
    {
        struct sockaddr_l2 destinationSocket;
        struct l2cap_conninfo l2capConnInfo;
        struct connectedDeviceInfo connectedDevice;
        socklen_t l2capConnInfoLen;
        BTDeviceInfo jsonConnectedDevice;
        bdaddr_t sourceAddress;
        bdaddr_t destinationAddress;
        std::thread readingThread;

        if (!PairDevice(deviceId, connectedDevice.l2capSocket, destinationSocket)) {
            TRACE(Trace::Error, ("Failed to Pair the Device [%s]", deviceId.c_str()));
            return false;
        }

        if (connect(connectedDevice.l2capSocket, (sockaddr*)&destinationSocket, sizeof(destinationSocket)) != 0) {
            TRACE(Trace::Error, ("Failed to Connect the Device [%s]", deviceId.c_str()));
            close(connectedDevice.l2capSocket);
            return false;
        }

        connectedDevice.connected = true;

        l2capConnInfoLen = sizeof(l2capConnInfo);
        getsockopt(connectedDevice.l2capSocket, SOL_L2CAP, L2CAP_CONNINFO, &l2capConnInfo, &l2capConnInfoLen);
        TRACE(Trace::Information, ("L2CAP Connection Handle of Device [%s] is [%d]", deviceId.c_str(), l2capConnInfo.hci_handle));
        connectedDevice.connectionHandle = l2capConnInfo.hci_handle;

        hci_devba(_hciHandle, &sourceAddress);
        str2ba(deviceId.c_str(), &destinationAddress);
        if (CheckForHIDProfile(connectedDevice.l2capSocket)) {
            struct deviceInformation hidDeviceInfo;
            memset(&hidDeviceInfo, 0, sizeof(hidDeviceInfo));

            if (!ReadHIDDeviceInformation(connectedDevice.l2capSocket, &hidDeviceInfo)) {
                TRACE(Trace::Error, ("Failed to read HID Device Information"));
                close(connectedDevice.l2capSocket);
                return false;
            }

            connectedDevice.uhidFD = CreateHIDDeviceNode(sourceAddress, destinationAddress, &hidDeviceInfo);
            if (connectedDevice.uhidFD == 0) {
                TRACE(Trace::Error, ("Failed to create HID Device node"));
                close(connectedDevice.l2capSocket);
                return false;
            }

            if (!EnableInputReportNotification(connectedDevice.l2capSocket)) {
                TRACE(Trace::Error, ("Failed to enable input report notification"));
                close(connectedDevice.l2capSocket);
                return false;
            }

            if (_connectedDeviceInfoMap.find(deviceId.c_str()) == _connectedDeviceInfoMap.end()) {
                connectedDevice.name = hidDeviceInfo.name;
                _pairedDeviceInfoMap.insert(std::make_pair(deviceId.c_str(), connectedDevice));
                _connectedDeviceInfoMap.insert(std::make_pair(deviceId.c_str(), hidDeviceInfo.name));
                jsonConnectedDevice.Address = deviceId;
                jsonConnectedDevice.Name = hidDeviceInfo.name;
                _jsonPairedDevices.Add(jsonConnectedDevice);
                _jsonConnectedDevices.Add(jsonConnectedDevice);
            }

            readingThread= std::thread(&BluetoothImplementation::ReadingThread, this, deviceId);
            readingThread.detach();

            TRACE(Trace::Information, ("Connected BT Device [%s]", deviceId.c_str()));
            return true;

        } else {
            TRACE(Trace::Error, ("The Device [%s] doesn't support HID Profile", deviceId.c_str()));
            close(connectedDevice.l2capSocket);
            return false;
        }

        TRACE(Trace::Information, ("Paired BT Device. Device ID : [%s]", deviceId.c_str()));
        return true;
    }

    string BluetoothImplementation::PairedDevices()
    {
        std::string deviceInfoList;

        _jsonPairedDevices.ToString(deviceInfoList);
        return deviceInfoList;
    }

    bool BluetoothImplementation::SendCommand(uint32_t l2capSocket, uint8_t opcode, handleRange param1, uint16_t param2, uint16_t param3, characteristicInfo param4, ssize_t paramLength, uint8_t* data, uint8_t response)
    {
        struct iovec iov[paramLength];
        struct iovec iov1;
        int dataLength;

        iov[0].iov_base = &opcode;
        iov[0].iov_len = sizeof(opcode);

        if (opcode == ATT_OP_READ_BLOB_REQ || opcode == ATT_OP_WRITE_REQ) {
            iov[1].iov_base = &param4;
            iov[1].iov_len = sizeof(param4);

        } else {
            iov[1].iov_base = &param1;
            iov[1].iov_len = sizeof(param1);
        }

        if (paramLength > 2) {
            iov[2].iov_base = &param2;
            iov[2].iov_len = sizeof(param2);

            if (paramLength == 4) {
                iov[3].iov_base = &param3;
                iov[3].iov_len = sizeof(param3);
            }
        }

        if (writev(l2capSocket, iov, paramLength) < 0) {
            TRACE(Trace::Error, ("Couldn't Write to L2CAP Socket"));
            return false;
        }

        memset(data, 0, DATA_BUFFER_LENGTH);
        iov1.iov_base = data;
        iov1.iov_len = DATA_BUFFER_LENGTH;

        while (dataLength = readv(l2capSocket, &iov1, 1)) {
            if (dataLength < 0) {
                TRACE(Trace::Error, ("Couldn't Read to L2CAP Socket"));
                return false;
            }

            if (data[0] == ATT_OP_ERROR) {
                TRACE(Trace::Information, ("SendCommand Error [ATT_OP_ERROR], Opcode - [%02x]", opcode));
                break;
            }

            if (data[0] == response) {
#if 0
                // For Debugging Purpose.
                for (ssize_t index=0 ; index < DATA_BUFFER_LENGTH ; index++)
                    TRACE(Trace::Information, (" %02x",data[index]));
#endif
                break;
            }
        }

        return true;
    }

    bool BluetoothImplementation::CheckForHIDProfile(uint32_t l2capSocket)
    {
        uint8_t dataBuffer[DATA_BUFFER_LENGTH];
        struct handleRange range;
        struct characteristicInfo characteristic;

        range.startHandle = htobs(0x0001);
        range.endHandle = htobs(0xffff);
        memset(&characteristic, 0, sizeof(characteristic));

        // Checking for HID Profile.
        SendCommand(l2capSocket, htobs(ATT_OP_FIND_BY_TYPE_REQ), range, htobs(PRIMARY_SERVICE_UUID), htobs(HID_UUID), characteristic, 4, dataBuffer, htobs(ATT_OP_FIND_BY_TYPE_RESP));
        if (dataBuffer[0] == ATT_OP_ERROR) {
            TRACE(Trace::Error, ("The Device not support HID Profile"));
            return false;
        } else if (dataBuffer[0] == ATT_OP_FIND_BY_TYPE_RESP) {
            TRACE(Trace::Information, ("The Device support HID Profile"));
            return true;
        }
    }

    bool BluetoothImplementation::ReadHIDDeviceInformation(uint32_t l2capSocket, struct deviceInformation* deviceInfo)
    {
        uint16_t nameHandle;
        uint16_t reportMapHandle;
        uint16_t reportMapOffsets[] = { 0x00, 0x16, 0x2c, 0x42, 0x58, 0x6e, 0x84, 0x9a };
        uint8_t deviceName[DATA_BUFFER_LENGTH];
        uint8_t dataBuffer[DATA_BUFFER_LENGTH];

        struct handleRange range;
        range.startHandle = htobs(0x0001);
        range.endHandle = htobs(0xffff);

        struct characteristicInfo characteristic;
        memset(&characteristic, 0, sizeof(characteristic));

        // Getting Device Vendor ID, Product ID, Version.
        SendCommand(l2capSocket, htobs(ATT_OP_READ_BY_TYPE_REQ), range, htobs(PNP_UUID), 0, characteristic, 3, dataBuffer, htobs(ATT_OP_READ_BY_TYPE_RESP));
        memcpy(&deviceInfo->vendorID, &dataBuffer[5], sizeof(deviceInfo->vendorID));
        memcpy(&deviceInfo->productID, &dataBuffer[7], sizeof(deviceInfo->productID));
        memcpy(&deviceInfo->version, &dataBuffer[9], sizeof(deviceInfo->version));

        // Getting Device Name.
        SendCommand(l2capSocket, htobs(ATT_OP_READ_BY_TYPE_REQ), range, htobs(DEVICE_NAME_UUID), 0, characteristic, 3, dataBuffer, htobs(ATT_OP_READ_BY_TYPE_RESP));
        memset(deviceName, 0, sizeof(deviceName));
        memcpy(&nameHandle, &dataBuffer[2], sizeof(nameHandle));
        TRACE(Trace::Information, ("Device Name handle is [0x%02x]", nameHandle));

        characteristic.handle = htobs(nameHandle);
        characteristic.offset = htobs(0x00);
        SendCommand(l2capSocket, htobs(ATT_OP_READ_BLOB_REQ), range, 0, 0, characteristic, 2, dataBuffer, htobs(ATT_OP_READ_BLOB_RESP));
        memcpy(&deviceName, &dataBuffer[1], sizeof(deviceName));
        std::stringstream nameString;
        nameString << deviceName;
        deviceInfo->name = nameString.str();
        TRACE(Trace::Information, ("Device Name is [%s]", deviceInfo->name.c_str()));
        deviceInfo->country = 0;

        // Getting Report Descriptor Map.
        range.startHandle = htobs(0x0001);
        range.endHandle = htobs(0xffff);
        SendCommand(l2capSocket, ATT_OP_READ_BY_TYPE_REQ, range, htobs(REPORT_MAP_UUID), 0, characteristic, 3, dataBuffer, htobs(ATT_OP_READ_BY_TYPE_RESP));
        memcpy(&reportMapHandle, &dataBuffer[2], sizeof(reportMapHandle));
        TRACE(Trace::Information, ("Device Report Descriptor Map handle is [0x%02x]", reportMapHandle));

        ssize_t index = 0;
        characteristic.handle = htobs(reportMapHandle);
        for (index=0; index < 8; index++) {
            characteristic.offset = htobs(reportMapOffsets[index]);
            SendCommand(l2capSocket, htobs(ATT_OP_READ_BLOB_REQ), range, 0, 0, characteristic, 2, dataBuffer, htobs(ATT_OP_READ_BLOB_RESP));

            for (ssize_t count=1; count < DATA_BUFFER_LENGTH; count++)
                deviceInfo->reportMap.push_back(dataBuffer[count]);
        }

        TRACE(Trace::Information, ("Device Information : Name - [%s] Vendor ID - [%d] Product ID - [%d] Version - [%d]", deviceInfo->name.c_str(), deviceInfo->vendorID, deviceInfo->productID, deviceInfo->version));

#if 0
        // Commenting this Information as it is not needed for normal case.
        TRACE(Trace::Information, ("Report Map :"));
        for (index=0; index < deviceInfo->reportMap.size(); index++)
            TRACE(Trace::Critical, (" %02x", deviceInfo->reportMap[index]));
#endif
        return true;
    }

    uint32_t BluetoothImplementation::CreateHIDDeviceNode(bdaddr_t sourceAddress, bdaddr_t destinationAddress, struct deviceInformation* hidDeviceInfo)
    {
        struct uhid_event uhidEvent;
        uint8_t reportDescriptor[250];
        uint32_t uhidFD;

        uhidFD = open(UHID_PATH, O_RDWR | O_CLOEXEC);

        if (uhidFD < 0) {
            TRACE(Trace::Error, ("Failed to open UHID node"));
            return -1;
        }

        // Creating UHID Node.
        memset(&uhidEvent, 0, sizeof(uhidEvent));
        uhidEvent.type = UHID_CREATE;
        uhidEvent.u.create.bus = BUS_BLUETOOTH;
        uhidEvent.u.create.vendor = hidDeviceInfo->vendorID;
        uhidEvent.u.create.product = hidDeviceInfo->productID;
        uhidEvent.u.create.version = hidDeviceInfo->version;
        uhidEvent.u.create.country = hidDeviceInfo->country;
        std::copy(hidDeviceInfo->reportMap.begin(), hidDeviceInfo->reportMap.end(), reportDescriptor);
        uhidEvent.u.create.rd_data = reportDescriptor;
        uhidEvent.u.create.rd_size = sizeof(reportDescriptor);
        strcpy((char*)uhidEvent.u.create.name, hidDeviceInfo->name.c_str());
        ba2str(&sourceAddress, (char *)uhidEvent.u.create.phys);
        ba2str(&destinationAddress, (char *)uhidEvent.u.create.uniq);

        if (write(uhidFD, &uhidEvent, sizeof(uhidEvent)) < 0) {
            TRACE(Trace::Error, ("Failed to create UHID node"));
            close(uhidFD);
            return -1;
        }

        TRACE(Trace::Information, ("Successfully created UHID node"));
        return uhidFD;
    }

    bool BluetoothImplementation::EnableInputReportNotification(uint32_t l2capSocket)
    {
        uint16_t clientConfigurationHandle;
        uint16_t reportHandle;
        uint8_t dataBuffer[DATA_BUFFER_LENGTH];

        struct handleRange range;
        range.startHandle = htobs(0x0001);
        range.endHandle = htobs(0xffff);

        struct characteristicInfo characteristic;
        memset(&characteristic, 0, sizeof(characteristic));

        // Enable Input Report Notifications.
        SendCommand(l2capSocket, htobs(ATT_OP_READ_BY_TYPE_REQ), range, htobs(REPORT_UUID), 0, characteristic, 3, dataBuffer, htobs(ATT_OP_READ_BY_TYPE_RESP));
        int reportHandles[] = { dataBuffer[2], dataBuffer[2] + 4, dataBuffer[2] + 8 };

        for (auto iterator : reportHandles) {
            memcpy(&reportHandle, &iterator, sizeof(reportHandle));
            clientConfigurationHandle = reportHandle + 1;
            TRACE(Trace::Information, ("Report Handle - [0x%02x] Client Characteristic Configuration Handle - [0x%02x]", reportHandle, clientConfigurationHandle));

            characteristic.handle = htobs(clientConfigurationHandle);
            characteristic.offset = htobs(ENABLE_MODE);
            SendCommand(l2capSocket, htobs(ATT_OP_WRITE_REQ), range, 0, 0, characteristic, 2, dataBuffer, htobs(ATT_OP_WRITE_RESP));
        }

        return true;
    }

    void BluetoothImplementation::ReadingThread(string deviceId)
    {
        struct iovec iov;
        uint8_t dataBuffer[4096];
        struct pollfd pollSocket;
        int result;
        int dataLength;
        int count;

        const auto& iterator = _pairedDeviceInfoMap.find(deviceId.c_str());
        if (iterator != _pairedDeviceInfoMap.end()) {
            pollSocket.fd = iterator->second.l2capSocket;
            pollSocket.events = POLLIN;

            TRACE(Trace::Information, ("Starting Reading Thread for device : [%s]", deviceId.c_str()));

            while (result = poll(&pollSocket, 1, -1)) {

                if (!iterator->second.connected)
                    break;

                if (result < 0) {
                    TRACE(Trace::Error, ("Cannot Poll for L2CAP Socket"));
                    break;
                }

                if (pollSocket.revents & POLLIN) {
                    dataLength = read(iterator->second.l2capSocket, dataBuffer, sizeof(dataBuffer));

                    if (dataBuffer[0] == ATT_OP_HANDLE_NOTIFY) {
                        struct uhid_event uhidEvent;
                        memset(&uhidEvent, 0, sizeof(uhidEvent));
                        uhidEvent.type = UHID_INPUT;
                        uhidEvent.u.input.size = dataLength;
                        uhidEvent.u.input.data[0] = 0x01;
                        count = 0;
                        for (ssize_t index=2; index < dataLength ; index++) {
                            if (dataBuffer[index] != 0x00) {
                                count++;
                                uhidEvent.u.input.data[count] = dataBuffer[index];
                            }
                        }

                        iov.iov_base = &uhidEvent;
                        iov.iov_len = sizeof(uhidEvent);

                        if (writev(iterator->second.uhidFD, &iov, 1) < 0)
                            TRACE(Trace::Error, ("Cannot write to UHID Node"));
                    }
                }
            }

            TRACE(Trace::Information, ("Reading Thread Finished for device : [%s]", deviceId.c_str()));
        } else
            TRACE(Trace::Information, ("Failed to start Reading Thread for device : [%s]", deviceId.c_str()));
    }


    bool BluetoothImplementation::Connect(string deviceId)
    {
        struct l2cap_conninfo l2capConnInfo;
        socklen_t l2capConnInfoLen;
        std::thread readingThread;
        struct sockaddr_l2 destinationSocket;
        uint32_t l2capSocket;

        const auto& iterator = _pairedDeviceInfoMap.find(deviceId.c_str());
        if (iterator != _pairedDeviceInfoMap.end()) {

            if (!PairDevice(deviceId, l2capSocket, destinationSocket)) {
                TRACE(Trace::Error, ("Failed to Pair the Device [%s]", deviceId.c_str()));
                return false;
            }

            if (connect(l2capSocket, (sockaddr*)&destinationSocket, sizeof(destinationSocket)) != 0) {
                TRACE(Trace::Error, ("Failed to Connect the Device [%s]", deviceId.c_str()));
                close(l2capSocket);
                return false;
            }

            iterator->second.connected = true;

            l2capConnInfoLen = sizeof(l2capConnInfo);
            getsockopt(l2capSocket, SOL_L2CAP, L2CAP_CONNINFO, &l2capConnInfo, &l2capConnInfoLen);
            TRACE(Trace::Information, ("L2CAP Connection Handle of Device [%s] is [%d]", deviceId.c_str(), l2capConnInfo.hci_handle));
            iterator->second.connectionHandle = l2capConnInfo.hci_handle;

            _connectedDeviceInfoMap.insert(std::make_pair(deviceId.c_str(), iterator->second.name));

            readingThread= std::thread(&BluetoothImplementation::ReadingThread, this, deviceId);
            readingThread.detach();

        }
        return true;
    }

    string BluetoothImplementation::ConnectedDevices()
    {
        std::string deviceInfoList;
        BTDeviceInfo jsonConnectedDevice;

        _jsonConnectedDevices.Clear();
        if (_connectedDeviceInfoMap.size() > 0) {
            for (auto& connectedDevice : _connectedDeviceInfoMap) {
                jsonConnectedDevice.Address = connectedDevice.first;
                jsonConnectedDevice.Name = connectedDevice.second;
                _jsonConnectedDevices.Add(jsonConnectedDevice);
            }
        }

        _jsonConnectedDevices.ToString(deviceInfoList);
        return deviceInfoList;
    }

    bool BluetoothImplementation::Disconnect(string deviceId)
    {
        const auto& iterator = _pairedDeviceInfoMap.find(deviceId.c_str());
        if (iterator != _pairedDeviceInfoMap.end()) {
            hci_disconnect(_hciSocket, iterator->second.connectionHandle, 0, SCAN_TIMEOUT);
            iterator->second.connected = false;
            close(iterator->second.l2capSocket);
            TRACE(Trace::Information, ("Updating Connected Device Info Map"));
            _connectedDeviceInfoMap.erase(deviceId);

            TRACE(Trace::Information, ("Disconnected BT Device. Device ID : [%s]", deviceId.c_str()));

            return true;
        }

        TRACE(Trace::Error, ("Invalid BT Device ID. Device ID : [%s]", deviceId.c_str()));
        return false;
    }

    SERVICE_REGISTRATION(BluetoothImplementation, 1, 0);

}
} /* namespace WPEFramework::Plugin */
