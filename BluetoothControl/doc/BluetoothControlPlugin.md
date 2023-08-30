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
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The Bluetooth Control plugin allows Bluetooth device administration.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *BluetoothControl*) |
| classname | string | Class name: *BluetoothControl* |
| locator | string | Library name: *libWPEFrameworkBluetoothControl.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.interface | number | <sup>*(optional)*</sup> ID of the local Bluetooth interface |
| configuration?.autopasskeyconfirm | boolean | <sup>*(optional)*</sup> Enable automatic passkey confirmation (may pose a security risk) |
| configuration?.persistmac | boolean | <sup>*(optional)*</sup> Enable persistent Bluetooth address |
| configuration?.name | String | <sup>*(optional)*</sup> Name of the local Bluetooth interface |
| configuration?.shortname | String | <sup>*(optional)*</sup> Shortened name of the local Bluetooth interface |
| configuration?.class | number | <sup>*(optional)*</sup> Class of device value of the local Bluetooth interface |
| configuration?.uuids | array | <sup>*(optional)*</sup> UUIDs to include in the outbound EIR/AD blocks |
| configuration?.uuids[#] | object | <sup>*(optional)*</sup> (UUID entry) |
| configuration?.uuids[#]?.callsign | string | <sup>*(optional)*</sup> Callsign of the plugin providing the service |
| configuration?.uuids[#]?.uuid | string | <sup>*(optional)*</sup> UUID value (short or long) |
| configuration?.uuids[#]?.service | integer | <sup>*(optional)*</sup> Corresponding service bit in Class of Device value |

<a name="head.Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- [BluetoothControl.json](https://github.com/rdkcentral/ThunderInterfaces/blob/master/jsonrpc/BluetoothControl.json) (version 1.0.0) (compliant format)

<a name="head.Methods"></a>
# Methods

The following methods are provided by the BluetoothControl plugin:

BluetoothControl interface methods:

| Method | Description |
| :-------- | :-------- |
| [setdiscoverable](#method.setdiscoverable) | Starts advertising (or inquiry scanning), making the local interface visible by nearby Bluetooth devices |
| [stopdiscoverable](#method.stopdiscoverable) | Stops advertising (or inquiry scanning) operation |
| [scan](#method.scan) | Starts active discovery (or inquiry) of nearby Bluetooth devices |
| [stopscanning](#method.stopscanning) | Stops discovery (or inquiry) operation |
| [connect](#method.connect) | Connects to a Bluetooth device |
| [disconnect](#method.disconnect) | Disconnects from a connected Bluetooth device |
| [pair](#method.pair) | Pairs a Bluetooth device |
| [unpair](#method.unpair) | Unpairs a paired Bluetooth device |
| [abortpairing](#method.abortpairing) | Aborts pairing operation |
| [providepincode](#method.providepincode) | Provides a PIN-code for authentication during a legacy pairing process |
| [providepasskey](#method.providepasskey) | Provides a passkey for authentication during a pairing process |
| [confirmpasskey](#method.confirmpasskey) | Confirms a passkey for authentication during a pairing process |
| [forget](#method.forget) | Forgets a known Bluetooth device |
| [getdevicelist](#method.getdevicelist) | Retrieves a list of known remote Bluetooth devices |
| [getdeviceinfo](#method.getdeviceinfo) | Retrieves detailed information about a known Bluetooth device |

<a name="method.setdiscoverable"></a>
## *setdiscoverable [<sup>method</sup>](#head.Methods)*

Starts advertising (or inquiry scanning), making the local interface visible by nearby Bluetooth devices.

### Description

Please note that discoverable state in *Limited* mode for Bluetooth Classic is bounded to 30 seconds only.

Also see: [discoverablestarted](#event.discoverablestarted), [discoverablecomplete](#event.discoverablecomplete)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Discoverable type (must be one of the following: *Classic*, *LowEnergy*) |
| params?.mode | string | <sup>*(optional)*</sup> Discoverable mode (must be one of the following: *General*, *Limited*) (default: *General*) |
| params?.connectable | boolean | <sup>*(optional)*</sup> Selects connectable advertising (true, *LowEnergy* only) (default: *False*) |
| params?.duration | integer | <sup>*(optional)*</sup> Duration of the discoverable operation (in seconds) (default: *30*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed set discoverable state |
| 12 | ```ERROR_INPROGRESS``` | Discoverable state of selected type is already in progress |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.setdiscoverable",
  "params": {
    "type": "LowEnergy",
    "mode": "General",
    "connectable": false,
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

Stops advertising (or inquiry scanning) operation.

Also see: [discoverablecomplete](#event.discoverablecomplete)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Discoverable type (must be one of the following: *Classic*, *LowEnergy*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to top scanning |
| 5 | ```ERROR_ILLEGAL_STATE``` | Adapter is in not discoverable state of selected type |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.stopdiscoverable",
  "params": {
    "type": "LowEnergy"
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

Starts active discovery (or inquiry) of nearby Bluetooth devices.

Also see: [scanstarted](#event.scanstarted), [scancomplete](#event.scancomplete)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Scan type (must be one of the following: *Classic*, *LowEnergy*) |
| params?.mode | string | <sup>*(optional)*</sup> Scan mode (must be one of the following: *General*, *Limited*) (default: *General*) |
| params?.timeout | integer | <sup>*(deprecated)*</sup> <sup>*(optional)*</sup> Duration of the scan (in seconds) (default: *10*) |
| params?.duration | integer | <sup>*(optional)*</sup> Duration of the scan (in seconds) (default: *10*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to scan |
| 12 | ```ERROR_INPROGRESS``` | Scan of selected type is already in progress |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.scan",
  "params": {
    "type": "LowEnergy",
    "mode": "General",
    "duration": 60
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

Stops discovery (or inquiry) operation.

Also see: [scancomplete](#event.scancomplete)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.type | string | <sup>*(optional)*</sup> Scan type (must be one of the following: *Classic*, *LowEnergy*) (default: *LowEnergy*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 1 | ```ERROR_GENERAL``` | Failed to top scanning |
| 5 | ```ERROR_ILLEGAL_STATE``` | Scan of selected type is not in progress |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.stopscanning",
  "params": {
    "type": "LowEnergy"
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

This call also enables automatic reconnection of the device. If the device is currently not available it will be automatically connected as soon it becomes available.

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params?.type | string | <sup>*(optional)*</sup> Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) (default: *LowEnergy*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 5 | ```ERROR_ILLEGAL_STATE``` | Device not paired |
| 9 | ```ERROR_ALREADY_CONNECTED``` | Device already connected |
| 1 | ```ERROR_GENERAL``` | Failed to connect the device |
| 27 | ```ERROR_REQUEST_SUBMITTED``` | Device has not been connected but will be automatically connected when available |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.connect",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "LowEnergy"
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

This call also disables automatic reconnection. If the device is currently not connected it will not be reconnected when it becomes available.

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params?.type | string | <sup>*(optional)*</sup> Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) (default: *LowEnergy*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 36 | ```ERROR_ALREADY_RELEASED``` | Device not connected |
| 27 | ```ERROR_REQUEST_SUBMITTED``` | Device is currently not connected but it's autoconnection mode has been disabled |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.disconnect",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "LowEnergy"
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

PIN-code or passkey requests may appear during the pairing process. The process can be cancelled any time by calling the *abortpairing* method.

Also see: [devicestatechange](#event.devicestatechange), [pincoderequest](#event.pincoderequest), [passkeyrequest](#event.passkeyrequest), [passkeyconfirmrequest](#event.passkeyconfirmrequest)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params?.type | string | <sup>*(optional)*</sup> Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) (default: *LowEnergy*) |
| params?.capabilities | string | <sup>*(optional)*</sup> Pairing capabilities (must be one of the following: *DisplayOnly*, *DisplayYesNo*, *KeyboardOnly*, *NoInputNoOutput*, *KeyboardDisplay*) (default: *NoInputNoOutput*) |
| params?.timeout | integer | <sup>*(optional)*</sup> Maximum time allowed for the pairing process to complete (in seconds) (default: *20*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 9 | ```ERROR_ALREADY_CONNECTED``` | Device already paired |
| 1 | ```ERROR_GENERAL``` | Failed to pair the device |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.pair",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "LowEnergy",
    "capabilities": "NoInputNoOutput",
    "timeout": 60
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

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params?.type | string | <sup>*(optional)*</sup> Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) (default: *LowEnergy*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 36 | ```ERROR_ALREADY_RELEASED``` | Device not paired |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.unpair",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "LowEnergy"
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

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params?.type | string | <sup>*(optional)*</sup> Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) (default: *LowEnergy*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 5 | ```ERROR_ILLEGAL_STATE``` | Device not currently pairing |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.abortpairing",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "LowEnergy"
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

This method should be called upon receiving a *pincoderequest* event during a legacy pairing process. If the specified PIN-code is incorrect the pairing process will be aborted.

Also see: [devicestatechange](#event.devicestatechange), [pincoderequest](#event.pincoderequest)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |
| params.secret | string | A PIN-code string typically consisting of (but not limited to) four decimal digits |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 5 | ```ERROR_ILLEGAL_STATE``` | Device not currently pairing or PIN code has not been requested |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.providepincode",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "Classic",
    "secret": "0000"
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

This method should be called upon receiving a *passkeyrequest* event during pairing process. If the specified passkey is incorrect or empty the pairing process will be aborted.

Also see: [devicestatechange](#event.devicestatechange), [passkeyrequest](#event.passkeyrequest)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |
| params.secret | integer | A six-digit decimal number passkey |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 5 | ```ERROR_ILLEGAL_STATE``` | Device not currently pairing or a passkey has not been requested |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.providepasskey",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
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

<a name="method.confirmpasskey"></a>
## *confirmpasskey [<sup>method</sup>](#head.Methods)*

Confirms a passkey for authentication during a pairing process.

### Description

This method should be called upon receiving a *passkeyconfirmationrequest* event during a pairing process. If the confirmation is negative the pairing process will be aborted.

Also see: [devicestatechange](#event.devicestatechange), [passkeyconfirmrequest](#event.passkeyconfirmrequest)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |
| params.iscorrect | boolean | Specifies if the passkey sent in *passkeyconfirmrequest* event is correct (true) or incorrect (false) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |
| 5 | ```ERROR_ILLEGAL_STATE``` | Device is currently not pairing or passkey confirmation has not been requested |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.confirmpasskey",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "Classic",
    "iscorrect": true
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

Forgets a known Bluetooth device.

### Description

The device will no longer be listed and its status tracked. If the device is connected and/or paired it will be disconnected and unpaired.

Also see: [devicestatechange](#event.devicestatechange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.forget",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "LowEnergy"
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

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | List of devices |
| result[#] | object | (device entry) |
| result[#].address | string | Bluetooth address |
| result[#].type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |

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
      "address": "81:6F:B0:91:9B:FE",
      "type": "LowEnergy"
    }
  ]
}
```

<a name="method.getdeviceinfo"></a>
## *getdeviceinfo [<sup>method</sup>](#head.Methods)*

Retrieves detailed information about a known Bluetooth device.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.address | string | Bluetooth address |
| result.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |
| result?.name | string | <sup>*(optional)*</sup> Name of the device |
| result?.class | integer | <sup>*(optional)*</sup> Class of device |
| result?.appearance | integer | <sup>*(optional)*</sup> Appearance value |
| result?.services | array | <sup>*(optional)*</sup> List of supported services |
| result?.services[#] | string | <sup>*(optional)*</sup> Service UUID |
| result.connected | boolean | Indicates if the device is currently connected |
| result.paired | boolean | Indicates if the device is currently paired |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.getdeviceinfo",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "LowEnergy"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "Classic",
    "name": "Thunder Bluetooth Speaker",
    "class": 2360324,
    "appearance": 2113,
    "services": [
      "110a"
    ],
    "connected": true,
    "paired": true
  }
}
```

<a name="head.Properties"></a>
# Properties

The following properties are provided by the BluetoothControl plugin:

BluetoothControl interface properties:

| Property | Description |
| :-------- | :-------- |
| [adapters](#property.adapters) <sup>RO</sup> | List of local Bluetooth adapters |
| [adapter](#property.adapter) <sup>RO</sup> | Local Bluetooth adapter information |
| <sup>deprecated</sup> [devices](#property.devices) <sup>RO</sup> | List of known remote Bluetooth devices |
| <sup>deprecated</sup> [device](#property.device) <sup>RO</sup> | Remote Bluetooth device information |

<a name="property.adapters"></a>
## *adapters [<sup>property</sup>](#head.Properties)*

Provides access to the list of local Bluetooth adapters.

> This property is **read-only**.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | List of local Bluetooth adapters |
| result[#] | integer | Adapter ID |

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

### Value

> The *adapter id* argument shall be passed as the index to the property, e.g. *BluetoothControl.1.adapter@0*.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Local Bluetooth adapter information |
| result.id | integer | Interface ID number |
| result.interface | string | Interface name |
| result.address | string | Bluetooth address |
| result.type | string | Adapter type (must be one of the following: *Classic*, *LowEnergy*, *Dual*) |
| result.version | integer | Version |
| result?.manufacturer | integer | <sup>*(optional)*</sup> Manufacturer company identifer |
| result?.class | integer | <sup>*(optional)*</sup> Class of device |
| result?.name | string | <sup>*(optional)*</sup> Name |
| result?.shortname | string | <sup>*(optional)*</sup> Short name |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown adapter device |

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
    "address": "81:6F:B0:91:9B:FE",
    "type": "Dual",
    "version": 8,
    "manufacturer": 15,
    "class": 1060,
    "name": "Thunder Bluetooth Controller",
    "shortname": "Thunder"
  }
}
```

<a name="property.devices"></a>
## *devices [<sup>property</sup>](#head.Properties)*

Provides access to the list of known remote Bluetooth devices.

> This property is **read-only**.

> This API is **deprecated** and may be removed in the future. It is no longer recommended for use in new implementations.

### Value

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | array | List of known remote Bluetooth devices |
| result[#] | string | Bluetooth address |

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
    "81:6F:B0:91:9B:FE"
  ]
}
```

<a name="property.device"></a>
## *device [<sup>property</sup>](#head.Properties)*

Provides access to the remote Bluetooth device information.

> This property is **read-only**.

> This API is **deprecated** and may be removed in the future. It is no longer recommended for use in new implementations.

### Value

> The *device address* argument shall be passed as the index to the property, e.g. *BluetoothControl.1.device@81:6F:B0:91:9B:FE*.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object | Remote Bluetooth device information |
| result.address | string | Bluetooth address |
| result.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |
| result?.name | string | <sup>*(optional)*</sup> Name of the device |
| result?.class | integer | <sup>*(optional)*</sup> Class of device |
| result?.appearance | integer | <sup>*(optional)*</sup> Appearance value |
| result?.services | array | <sup>*(optional)*</sup> List of supported services |
| result?.services[#] | string | <sup>*(optional)*</sup> Service UUID |
| result.connected | boolean | Indicates if the device is currently connected |
| result.paired | boolean | Indicates if the device is currently paired |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 22 | ```ERROR_UNKNOWN_KEY``` | Unknown device |

### Example

#### Get Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "BluetoothControl.1.device@81:6F:B0:91:9B:FE"
}
```

#### Get Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "Classic",
    "name": "Thunder Bluetooth Speaker",
    "class": 2360324,
    "appearance": 2113,
    "services": [
      "110a"
    ],
    "connected": true,
    "paired": true
  }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the BluetoothControl plugin:

BluetoothControl interface events:

| Event | Description |
| :-------- | :-------- |
| [discoverablestarted](#event.discoverablestarted) | Notifies of entering the discoverable state |
| [discoverablecomplete](#event.discoverablecomplete) | Notifies of leaving the discoverable state |
| [scanstarted](#event.scanstarted) | Notifies of scan start |
| [scancomplete](#event.scancomplete) | Notifies of scan completion |
| [devicestatechange](#event.devicestatechange) | Notifies of device state changes |
| [pincoderequest](#event.pincoderequest) | Notifies of a PIN code request |
| [passkeyrequest](#event.passkeyrequest) | Notifies of a passkey request |
| [passkeyconfirmrequest](#event.passkeyconfirmrequest) | Notifies of a passkey confirmation request |

<a name="event.discoverablestarted"></a>
## *discoverablestarted [<sup>event</sup>](#head.Notifications)*

Notifies of entering the discoverable state.

### Description

Register to this event to be notified about entering the discoverable state

> If applicable, this notification may be sent out during registration, reflecting the current status.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Discoverable type (must be one of the following: *Classic*, *LowEnergy*) |
| params.mode | string | Discoverable mode (must be one of the following: *General*, *Limited*) |
| params?.connectable | boolean | <sup>*(optional)*</sup> Indicates connectable advertising (true, *LowEnergy* only) (default: *False*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.discoverablestarted",
  "params": {
    "type": "LowEnergy",
    "mode": "General",
    "connectable": false
  }
}
```

<a name="event.discoverablecomplete"></a>
## *discoverablecomplete [<sup>event</sup>](#head.Notifications)*

Notifies of leaving the discoverable state.

### Description

Register to this event to be notified about leaving the discoverable state

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Discoverable type (must be one of the following: *Classic*, *LowEnergy*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.discoverablecomplete",
  "params": {
    "type": "LowEnergy"
  }
}
```

<a name="event.scanstarted"></a>
## *scanstarted [<sup>event</sup>](#head.Notifications)*

Notifies of scan start.

### Description

Register to this event to be notified about device scan start

> If applicable, this notification may be sent out during registration, reflecting the current status.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Scan type (must be one of the following: *Classic*, *LowEnergy*) |
| params?.mode | string | <sup>*(optional)*</sup> Scan mode (must be one of the following: *General*, *Limited*) (default: *General*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.scanstarted",
  "params": {
    "type": "LowEnergy",
    "mode": "General"
  }
}
```

<a name="event.scancomplete"></a>
## *scancomplete [<sup>event</sup>](#head.Notifications)*

Notifies of scan completion.

### Description

Register to this event to be notified about device scan completion

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.type | string | Scan type (must be one of the following: *Classic*, *LowEnergy*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.scancomplete",
  "params": {
    "type": "LowEnergy"
  }
}
```

<a name="event.devicestatechange"></a>
## *devicestatechange [<sup>event</sup>](#head.Notifications)*

Notifies of device state changes.

### Description

Register to this event to be notified about device state changes

> If applicable, this notification may be sent out during registration, reflecting the current status.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |
| params.state | string | Device state (must be one of the following: *Pairing*, *Paired*, *Unpaired*, *Connected*, *Disconnected*) |
| params?.disconnectreason | string | <sup>*(optional)*</sup> Disconnection reason in case of *Disconnected* event (must be one of the following: *ConnectionTimeout*, *AuthenticationFailure*, *RemoteLowOnResources*, *RemotePoweredOff*, *TerminatedByRemote*, *TerminatedByHost*) |

> The *device type* argument shall be passed within the designator, e.g. *LowEnergy.client.events.1*.

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "LowEnergy.client.events.1.devicestatechange",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "LowEnergy",
    "state": "Disconnected",
    "disconnectreason": "ConnectionTimeout"
  }
}
```

<a name="event.pincoderequest"></a>
## *pincoderequest [<sup>event</sup>](#head.Notifications)*

Notifies of a PIN code request.

### Description

Register to this event to be notified about PIN code requests during a legacy pairing process. Upon receiving this event the client is required to respond with a *providepincode* call in order to complete the pairing procedure. The PIN code value would typically be collected by prompting the end-user. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted.<br><br>Note that this event will never be send for a Bluetooth LowEnergy device

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.pincoderequest",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "Classic"
  }
}
```

<a name="event.passkeyrequest"></a>
## *passkeyrequest [<sup>event</sup>](#head.Notifications)*

Notifies of a passkey request.

### Description

Register to this event to be notified about passkey requests that may be required during a pairing process. Upon receiving this event the client is required to respond with a *providepasskey* call in order to complete the pairing procedure. The passkey value would typically be collected by prompting the end-user. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.passkeyrequest",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "Classic"
  }
}
```

<a name="event.passkeyconfirmrequest"></a>
## *passkeyconfirmrequest [<sup>event</sup>](#head.Notifications)*

Notifies of a passkey confirmation request.

### Description

Register to this event to be notified about passkey confirmation requests that may required during a pairing process. Upon receiving this event the client is required to respond with a *passkeyconfirm* call in order to complete the pairing procedure. The passkey confirmation would typically be collected by prompting the end-user. If the client fails to respond before the pairing timeout elapses the pairing procedure will be aborted

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.address | string | Bluetooth address |
| params.type | string | Device type (must be one of the following: *Classic*, *LowEnergy*, *LowEnergyRandom*) |
| params.secret | integer | A six-digit decimal number passkey sent by the remote device for confirmation; may be 0 for a simple accept/forbid paring request |

### Example

```json
{
  "jsonrpc": "2.0",
  "method": "client.events.1.passkeyconfirmrequest",
  "params": {
    "address": "81:6F:B0:91:9B:FE",
    "type": "Classic",
    "secret": 123456
  }
}
```

