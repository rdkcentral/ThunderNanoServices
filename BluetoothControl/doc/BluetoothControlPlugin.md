<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.Bluetooth_Control_Plugin"></a>
# Bluetooth Control Plugin

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

BluetoothControl plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Interfaces](#head.Interfaces)
- [Methods](#head.Methods)
- [Properties](#head.Properties)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the BluetoothControl plugin. It includes detailed specification about its configuration, methods and properties as well as sent notifications.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times, but each instance the instance name, callsign, must be unique. |

<a name="head.References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The Bluetooth Control plugin allows Bluetooth device administration.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *BluetoothControl*) |
| classname | string | mandatory | Class name: *BluetoothControl* |
| locator | string | mandatory | Library name: *libThunderBluetoothControl.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |
| configuration | object | optional | *...* |
| configuration?.interface | integer | optional | ID of the local Bluetooth interface |
| configuration?.autopasskeyconfirm | boolean | optional | Enable automatic passkey confirmation (may pose a security risk) |
| configuration?.persistmac | boolean | optional | Enable persistent Bluetooth address |
| configuration?.name | String | optional | Name of the local Bluetooth interface |
| configuration?.shortname | String | optional | Shortened name of the local Bluetooth interface |
| configuration?.class | integer | optional | Class of device value of the local Bluetooth interface |
| configuration?.uuids | array | optional | UUIDs to include in the outbound EIR/AD blocks |
| configuration?.uuids[#] | object | optional | (UUID entry) |
| configuration?.uuids[#]?.callsign | string | optional | Callsign of the plugin providing the service |
| configuration?.uuids[#]?.uuid | string | optional | UUID value (short or long) |
| configuration?.uuids[#]?.service | integer | optional | Corresponding service bit in Class of Device value |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IBluetoothControl ([IBluetooth.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/IBluetooth.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a name="head.Methods"></a>
# Methods

The following methods are provided by the BluetoothControl plugin:

BluetoothControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [setdiscoverable](#method.setdiscoverable) | Starts LE advertising or BR/EDR inquiry scanning, making the local interface visible for nearby Bluetooth devices |
| [stopdiscoverable](#method.stopdiscoverable) | Stops LE advertising or BR/EDR inquiry scanning operation |
| [scan](#method.scan) | Starts LE active discovery or BR/EDR inquiry of nearby Bluetooth devices |
| [stopscanning](#method.stopscanning) | Stops LE discovery or BR/EDR inquiry operation |
| [connect](#method.connect) | Connects to a Bluetooth device |
| [disconnect](#method.disconnect) | Disconnects from a connected Bluetooth device |
| [pair](#method.pair) | Pairs a Bluetooth device |
| [unpair](#method.unpair) | Unpairs a paired Bluetooth device |
| [abortpairing](#method.abortpairing) | Aborts pairing operation |
| [providepincode](#method.providepincode) | Provides a PIN-code for authentication during a legacy pairing process |
| [confirmpasskey](#method.confirmpasskey) | Confirms a passkey for authentication during a BR/EDR SSP pairing processs |
| [providepasskey](#method.providepasskey) | Provides a passkey for authentication during a pairing process |
| [forget](#method.forget) | Forgets a previously seen Bluetooth device |
| [getdevicelist](#method.getdevicelist) | Retrieves a list of known remote Bluetooth devices |
| [getdeviceinfo](#method.getdeviceinfo) | Retrieves detailed information about a known Bluetooth device |

<a name="method.setdiscoverable"></a>
## *setdiscoverable [<sup>method</sup>](#head.Methods)*

Starts LE advertising or BR/EDR inquiry scanning, making the local interface visible for nearby Bluetooth devices.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy*) |
| params?.mode | string | optional | Advertising or inquiry scanning mode (must be one of the following: *General, Limited*) (default: *General*) |
| params?.connectable | boolean | optional | Specifies if LE advertising should report the device is connectable (LE-only) (default: *True*) |
| params?.duration | integer | optional | Time span of the discoverable state in seconds (default: *30*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | The adapter does not support selected discovery type |
| ```ERROR_INPROGRESS``` | Discoverable state of selected type is already in progress |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.setdiscoverable",
  "params": {
    "type": "Classic",
    "mode": "General",
    "connectable": true,
    "duration": 30
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.stopdiscoverable"></a>
## *stopdiscoverable [<sup>method</sup>](#head.Methods)*

Stops LE advertising or BR/EDR inquiry scanning operation.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | The adapter does not support selected discovery type |
| ```ERROR_ILLEGAL_STATE``` | The adapter is in not discoverable state of selected type |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.stopdiscoverable",
  "params": {
    "type": "Classic"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.scan"></a>
## *scan [<sup>method</sup>](#head.Methods)*

Starts LE active discovery or BR/EDR inquiry of nearby Bluetooth devices.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy*) |
| params?.mode | string | optional | Discovery or inquiry mode (scan picks up only devices discoverable in paricular mode) (must be one of the following: *General, Limited*) (default: *General*) |
| params?.duration | integer | optional | Time span of the discovery in seconds (default: *12*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | The adapter does not support selected scan type |
| ```ERROR_INPROGRESS``` | Scan of selected type is already in progress |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.scan",
  "params": {
    "type": "Classic",
    "mode": "General",
    "duration": 12
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.stopscanning"></a>
## *stopscanning [<sup>method</sup>](#head.Methods)*

Stops LE discovery or BR/EDR inquiry operation.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNAVAILABLE``` | The adapter does not support selected scan type |
| ```ERROR_ILLEGAL_STATE``` | Scan of selected type is not in progress |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.stopscanning",
  "params": {
    "type": "Classic"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.connect"></a>
## *connect [<sup>method</sup>](#head.Methods)*

Connects to a Bluetooth device.

### Description

This call also enables automatic reconnection of the device. If the device is currently not available it will be automatically connected as soon it becomes available. This call is asynchronous.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |
| ```ERROR_INPROGRESS``` | The host adapter is currently busy |
| ```ERROR_ILLEGAL_STATE``` | The device is not paired |
| ```ERROR_ALREADY_CONNECTED``` | The device is already connected |
| ```ERROR_REQUEST_SUBMITTED``` | The device has not been connected, but will be automatically connected when it becomes available |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.connect",
  "params": {
    "address": "...",
    "type": "Classic"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.disconnect"></a>
## *disconnect [<sup>method</sup>](#head.Methods)*

Disconnects from a connected Bluetooth device.

### Description

This call also disables automatic reconnection. If the device is currently not connected it will not be reconnected when it again becomes available.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |
| ```ERROR_INPROGRESS``` | The host adapter is currently busy |
| ```ERROR_ALREADY_RELEASED``` | The device is not connected |
| ```ERROR_REQUEST_SUBMITTED``` | The device is currently not connected, but it's automatic reconnection mode has been disabled |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.disconnect",
  "params": {
    "address": "...",
    "type": "Classic"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.pair"></a>
## *pair [<sup>method</sup>](#head.Methods)*

Pairs a Bluetooth device.

### Description

PIN-code or passkey requests may appear during the pairing process. The process can be cancelled any time by calling *abortPairing*. This call is asynchronous.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| params?.capabilities | string | optional | Host device pairing capabilities (must be one of the following: *DisplayOnly, DisplayYesNo, KeyboardDisplay, KeyboardOnly, NoInputNoOutput*) (default: *NoInputNoOutput*) |
| params?.timeout | integer | optional | Time allowed for the pairing process to complete (default: *10*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |
| ```ERROR_INPROGRESS``` | The host adapter is currently busy |
| ```ERROR_ALREADY_CONNECTED``` | The device is already paired |
| ```ERROR_GENERAL``` | Failed to pair |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.pair",
  "params": {
    "address": "...",
    "type": "Classic",
    "capabilities": "DisplayOnly",
    "timeout": 10
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.unpair"></a>
## *unpair [<sup>method</sup>](#head.Methods)*

Unpairs a paired Bluetooth device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |
| ```ERROR_INPROGRESS``` | The host adapter is currently busy |
| ```ERROR_ALREADY_RELEASED``` | The device is not paired |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.unpair",
  "params": {
    "address": "...",
    "type": "Classic"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.abortpairing"></a>
## *abortpairing [<sup>method</sup>](#head.Methods)*

Aborts pairing operation.

### Description

This call is asynchronous.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |
| ```ERROR_ILLEGAL_STATE``` | The device not currently pairing |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.abortpairing",
  "params": {
    "address": "...",
    "type": "Classic"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.providepincode"></a>
## *providepincode [<sup>method</sup>](#head.Methods)*

Provides a PIN-code for authentication during a legacy pairing process.

### Description

This method should be called upon receiving a "pinCodeRequest" event during a legacy pairing process. If the specified PIN-code is incorrect the pairing process will be aborted.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| params.secret | string | mandatory | A PIN code, typically 4 ASCII digits<br>*String length must be at most 16 chars.* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |
| ```ERROR_ILLEGAL_STATE``` | The device not currently pairing or a PIN code has not been requested |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.providepincode",
  "params": {
    "address": "...",
    "type": "Classic",
    "secret": "1234"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.confirmpasskey"></a>
## *confirmpasskey [<sup>method</sup>](#head.Methods)*

Confirms a passkey for authentication during a BR/EDR SSP pairing processs.

### Description

This method should be called upon receiving a passkeyConfirmationRequest event during a pairing process. If the confirmation is negative the pairing process will be aborted.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| params.accept | boolean | mandatory | Confirm pairing (normally if the presented passkey is correct) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown device |
| ```ERROR_ILLEGAL_STATE``` | The device is currently not pairing or passkey confirmation has not been requested |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.confirmpasskey",
  "params": {
    "address": "...",
    "type": "Classic",
    "accept": false
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.providepasskey"></a>
## *providepasskey [<sup>method</sup>](#head.Methods)*

Provides a passkey for authentication during a pairing process.

### Description

This method should be called upon receiving a "passkeyRequest" event during pairing process. If the specified passkey is incorrect or empty the pairing process will be aborted.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| params.secret | integer | mandatory | A decimal six-digit passkey value<br>*Value must be in range [0..999999].* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | Unknown device |
| ```ERROR_ILLEGAL_STATE``` | The device not currently pairing or a passkey has not been requested |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.providepasskey",
  "params": {
    "address": "...",
    "type": "Classic",
    "secret": 123456
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.forget"></a>
## *forget [<sup>method</sup>](#head.Methods)*

Forgets a previously seen Bluetooth device.

### Description

The device will no longer be listed and its status tracked. If paired the device must be unpaired first.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |
| ```ERROR_ILLEGAL_STATE``` | The device is paired |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.forget",
  "params": {
    "address": "...",
    "type": "Classic"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": null
}
```

<a name="method.getdevicelist"></a>
## *getdevicelist [<sup>method</sup>](#head.Methods)*

Retrieves a list of known remote Bluetooth devices.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | *...* |
| result[#] | object | mandatory | *...* |
| result[#].address | string | mandatory | Bluetooth address |
| result[#].type | string | mandatory | Bluetooth device type (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| result[#].name | string | mandatory | Bluetooth short name |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.getdevicelist"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "address": "...",
      "type": "Classic",
      "name": "..."
    }
  ]
}
```

<a name="method.getdeviceinfo"></a>
## *getdeviceinfo [<sup>method</sup>](#head.Methods)*

Retrieves detailed information about a known Bluetooth device.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | *...* |
| result.address | string | mandatory | *...* |
| result.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| result?.name | string | optional | Device local name |
| result?.version | integer | optional | Device version |
| result?.manufacturer | integer | optional | Company Identification Code (CIC) |
| result?.class | integer | optional | Class of Device (CoD) value<br>*Value must be in range [0..16777215].* |
| result?.appearance | integer | optional | Appearance value (LE-only) |
| result?.services | array | optional | A list of supported service UUIDs |
| result?.services[#] | string | optional | *...* |
| result.paired | boolean | mandatory | Specifies if the device is currently paired |
| result.connected | boolean | mandatory | Specifies if the device is currently connected |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.getdeviceinfo",
  "params": {
    "address": "...",
    "type": "Classic"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "address": "...",
    "type": "Classic",
    "name": "...",
    "version": 0,
    "manufacturer": 305,
    "class": 2360324,
    "appearance": 2113,
    "services": [
      "..."
    ],
    "paired": false,
    "connected": false
  }
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the BluetoothControl plugin:

BluetoothControl interface properties:

| Property | R/W | Description |
| :-------- | :-------- | :-------- |
| [adapters](#property.adapters) | read-only | List of local Bluetooth adapters |
| [adapter](#property.adapter) | read-only | Local Bluetooth adapter information |
| [devices](#property.devices) <sup>deprecated</sup> | read-only | List of known remote Bluetooth LE devices |
| [device](#property.device) <sup>deprecated</sup> | read-only | Remote Bluetooth LE device information |

<a name="property.adapters"></a>
## *adapters [<sup>property</sup>](#head.Properties)*

Provides access to the list of local Bluetooth adapters.

> This property is **read-only**.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of local Bluetooth adapters |
| result[#] | integer | mandatory | *...* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.adapters"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    0
  ]
}
```

<a name="property.adapter"></a>
## *adapter [<sup>property</sup>](#head.Properties)*

Provides access to the local Bluetooth adapter information.

> This property is **read-only**.

> The *adapter* parameter shall be passed as the index to the property, e.g. ``BluetoothControl.1.adapter@<adapter>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| adapter | integer | mandatory | Adapter index |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Local Bluetooth adapter information |
| result.id | integer | mandatory | Adapter ID number |
| result.interface | string | mandatory | Interface name |
| result.address | string | mandatory | Bluetooth address |
| result.type | string | mandatory | Adapter type (must be one of the following: *Classic, Dual, LowEnergy*) |
| result.version | integer | mandatory | Version |
| result?.manufacturer | integer | optional | Company Identification Code (CIC) |
| result?.class | integer | optional | Class of Device (CoD) value<br>*Value must be in range [0..16777215].* |
| result?.name | string | optional | Name |
| result?.shortname | string | optional | Shortened name |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The adapter ID is invalid |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.adapter@0"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "id": 0,
    "interface": "hci0",
    "address": "...",
    "type": "Classic",
    "version": 0,
    "manufacturer": 305,
    "class": 2360324,
    "name": "...",
    "shortname": "..."
  }
}
```

<a name="property.devices"></a>
## *devices [<sup>property</sup>](#head.Properties)*

Provides access to the list of known remote Bluetooth LE devices.

> This property is **read-only**.

> ``devices`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | List of known remote Bluetooth LE devices |
| result[#] | string | mandatory | *...* |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.devices"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    "..."
  ]
}
```

<a name="property.device"></a>
## *device [<sup>property</sup>](#head.Properties)*

Provides access to the remote Bluetooth LE device information.

> This property is **read-only**.

> ``device`` is an alternative name for this property. This name is **deprecated** and may be removed in the future. It is not recommended for use in new implementations.

> The *deviceaddress* parameter shall be passed as the index to the property, e.g. ``BluetoothControl.1.device@<deviceaddress>``.

### Index

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| deviceaddress | string | mandatory | *...* |

### Value

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | object | mandatory | Remote Bluetooth LE device information |
| result.address | string | mandatory | Bluetooth address |
| result.type | string | mandatory | Device type (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| result?.name | string | optional | Device name |
| result?.class | integer | optional | Class of Device (CoD) value<br>*Value must be in range [0..16777215].* |
| result?.appearance | integer | optional | Appearance value (LE only) |
| result?.services | opaque object | optional | Array of supported service UUIDs |
| result.paired | boolean | mandatory | Specifies if the device is currently paired |
| result.connected | boolean | mandatory | Specifies if the device is currently connected |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_UNKNOWN_KEY``` | The device is not known |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.device@xyz"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "address": "...",
    "type": "Classic",
    "name": "...",
    "class": 2360324,
    "appearance": 2113,
    "services": {},
    "paired": false,
    "connected": false
  }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothControl plugin:

BluetoothControl interface events:

| Notification | Description |
| :-------- | :-------- |
| [pincoderequest](#notification.pincoderequest) | Notifies of a PIN code request during authenticated BR/EDR legacy pairing process |
| [passkeyconfirmrequest](#notification.passkeyconfirmrequest) | Notifies of a user confirmation request during authenticated BR/EDR SSP pairing process |
| [passkeyrequest](#notification.passkeyrequest) | Notifies of a passkey supply request during authenticated LE pairing process |
| [passkeydisplayrequest](#notification.passkeydisplayrequest) | Notifies of a passkey presentation request during authenticated LE pairing process |
| [discoverablestarted](#notification.discoverablestarted) | Reports entering the discoverable state |
| [discoverablecomplete](#notification.discoverablecomplete) | Reports leaving the discoverable state |
| [scanstarted](#notification.scanstarted) | Reports start of scanning |
| [scancomplete](#notification.scancomplete) | Reports end of scanning |
| [devicestatechanged](#notification.devicestatechanged) | disconnectReason If disconnected specifies the cause of disconnection |

<a name="notification.pincoderequest"></a>
## *pincoderequest [<sup>notification</sup>](#head.Notifications)*

Notifies of a PIN code request during authenticated BR/EDR legacy pairing process.

### Description

Upon receiving this event the client is required to respond with "providePinCode" call in order to complete the pairing procedure. The PIN code value is typically collected by prompting the end-user on the local device. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "pincoderequest",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.pincoderequest",
  "params": {
    "address": "...",
    "type": "Classic"
  }
}
```

<a name="notification.passkeyconfirmrequest"></a>
## *passkeyconfirmrequest [<sup>notification</sup>](#head.Notifications)*

Notifies of a user confirmation request during authenticated BR/EDR SSP pairing process.

### Description

Upon receiving this event the client is required to respond with "confirmPasskey" call in order to complete the pairing procedure. The passkey confirmation is typically collected by prompting the end-user on the local device. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| params?.secret | integer | optional | A six-digit decimal number sent by the remote device to be presented to the end-user for confirmation on the local device (e.g 123456). The passkey may be omitted for simple yes/no paring<br>*Value must be in range [0..999999].* |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "passkeyconfirmrequest",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.passkeyconfirmrequest",
  "params": {
    "address": "...",
    "type": "Classic",
    "secret": 0
  }
}
```

<a name="notification.passkeyrequest"></a>
## *passkeyrequest [<sup>notification</sup>](#head.Notifications)*

Notifies of a passkey supply request during authenticated LE pairing process.

### Description

Upon receiving this event the client is required to respond with "providePasskey" call in order to complete the pairing procedure. The passkey value is typically collected by prompting the end-user on the local device. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "passkeyrequest",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.passkeyrequest",
  "params": {
    "address": "...",
    "type": "Classic"
  }
}
```

<a name="notification.passkeydisplayrequest"></a>
## *passkeydisplayrequest [<sup>notification</sup>](#head.Notifications)*

Notifies of a passkey presentation request during authenticated LE pairing process.

### Description

Upon receiving this event the client is required to display the passkey on the local device. The end-user on the remote device will need to enter this passkey to complete the pairing procedure. If end-user fails to respond before the pairing timeout elapses the pairing procedure will be aborted.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| params.secret | integer | mandatory | A six-digit decimal number to be displayed on the local device (e.g 123456)<br>*Value must be in range [0..999999].* |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "passkeydisplayrequest",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.passkeydisplayrequest",
  "params": {
    "address": "...",
    "type": "Classic",
    "secret": 0
  }
}
```

<a name="notification.discoverablestarted"></a>
## *discoverablestarted [<sup>notification</sup>](#head.Notifications)*

Reports entering the discoverable state.

> If applicable, this notification may be sent out during registration, reflecting the current status.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy*) |
| params.mode | string | mandatory | Advertising or inquiry scanning mode (must be one of the following: *General, Limited*) |
| params?.connectable | boolean | optional | Specifies if LE advertising reports that the device is connectable (LE-only) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "discoverablestarted",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.discoverablestarted",
  "params": {
    "type": "Classic",
    "mode": "General",
    "connectable": false
  }
}
```

<a name="notification.discoverablecomplete"></a>
## *discoverablecomplete [<sup>notification</sup>](#head.Notifications)*

Reports leaving the discoverable state.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "discoverablecomplete",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.discoverablecomplete",
  "params": {
    "type": "Classic"
  }
}
```

<a name="notification.scanstarted"></a>
## *scanstarted [<sup>notification</sup>](#head.Notifications)*

Reports start of scanning.

> If applicable, this notification may be sent out during registration, reflecting the current status.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy*) |
| params.mode | string | mandatory | Discovery or inquiry mode (must be one of the following: *General, Limited*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "scanstarted",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.scanstarted",
  "params": {
    "type": "Classic",
    "mode": "General"
  }
}
```

<a name="notification.scancomplete"></a>
## *scancomplete [<sup>notification</sup>](#head.Notifications)*

Reports end of scanning.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "scancomplete",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.scancomplete",
  "params": {
    "type": "Classic"
  }
}
```

<a name="notification.devicestatechanged"></a>
## *devicestatechanged [<sup>notification</sup>](#head.Notifications)*

disconnectReason If disconnected specifies the cause of disconnection.

> If applicable, this notification may be sent out during registration, reflecting the current status.

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.address | string | mandatory | *...* |
| params.type | string | mandatory | *...* (must be one of the following: *Classic, LowEnergy, LowEnergyRandom*) |
| params.state | string | mandatory | *...* (must be one of the following: *Connected, Disconnected, Paired, Pairing, Unpaired*) |
| params?.disconnectreason | string | optional | *...* (must be one of the following: *AuthenticationFailure, ConnectionTimeout, RemoteLowOnResources, RemotePoweredOff, TerminatedByHost, TerminatedByRemote*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.register",
  "params": {
    "event": "devicestatechanged",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.devicestatechanged",
  "params": {
    "address": "...",
    "type": "Classic",
    "state": "Pairing",
    "disconnectreason": "ConnectionTimeout"
  }
}
```

