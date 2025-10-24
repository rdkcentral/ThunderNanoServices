<!-- Generated automatically, DO NOT EDIT! -->
<a id="head_TestAutomationUtils_Plugin"></a>
# TestAutomationUtils Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

TestAutomationUtils plugin for Thunder framework.

### Table of Contents

- [Introduction](#head_Introduction)
- [Description](#head_Description)
- [Configuration](#head_Configuration)
- [Interfaces](#head_Interfaces)
- [Methods](#head_Methods)
- [Notifications](#head_Notifications)

<a id="head_Introduction"></a>
# Introduction

<a id="head_Scope"></a>
## Scope

This document describes purpose and functionality of the TestAutomationUtils plugin. It includes detailed specification about its configuration, methods provided and notifications sent.

<a id="head_Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a id="head_Acronyms,_Abbreviations_and_Terms"></a>
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

<a id="head_References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20Thunder.docx)</a> | Thunder API Reference |

<a id="head_Description"></a>
# Description

TestAutomationUtils helps you to provide some special functionality convenient for testing.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a id="head_Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| callsign | string | mandatory | Plugin instance name (default: *TestAutomationUtils*) |
| classname | string | mandatory | Class name: *TestAutomationUtils* |
| locator | string | mandatory | Library name: *libThunderTestAutomationUtils.so* |
| startmode | string | mandatory | Determines in which state the plugin should be moved to at startup of the framework |

<a id="head_Interfaces"></a>
# Interfaces

This plugin implements the following interfaces:

- IMemory ([ITestAutomation.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITestAutomation.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- IComRpc ([ITestAutomation.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITestAutomation.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- ITestTextOptions ([ITestAutomation.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITestAutomation.h)) (version 1.0.0) (compliant format)
- ITestTextOptions::ITestLegacy ([ITestAutomation.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITestAutomation.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

- ITestTextOptions::ITestKeep ([ITestAutomation.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITestAutomation.h)) (version 1.0.0) (compliant format)
- ITestTextOptions::ITestCustom ([ITestAutomation.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITestAutomation.h)) (version 1.0.0) (compliant format)
- ITestUtils ([ITestAutomation.h](https://github.com/rdkcentral/ThunderInterfaces/blob/master/interfaces/ITestAutomation.h)) (version 1.0.0) (compliant format)
> This interface uses legacy ```lowercase``` naming convention. With the next major release the naming convention will change to ```camelCase```.

<a id="head_Methods"></a>
# Methods

The following methods are provided by the TestAutomationUtils plugin:

Built-in methods:

| Method | Description |
| :-------- | :-------- |
| [versions](#method_versions) | Retrieves a list of JSON-RPC interfaces offered by this service |
| [exists](#method_exists) | Checks if a JSON-RPC method or property exists |
| [register](#method_register) | Registers for an asynchronous JSON-RPC notification |
| [unregister](#method_unregister) | Unregisters from an asynchronous JSON-RPC notification |

Memory interface methods:

| Method | Description |
| :-------- | :-------- |
| [allocatememory](#method_allocatememory) | Allocates Memory as given size of MB |
| [freeallocatedmemory](#method_freeallocatedmemory) | Frees the allocated memory |

ComRpc interface methods:

| Method | Description |
| :-------- | :-------- |
| [testbigstring](#method_testbigstring) | Validates big string over proxy-stub with given length of KB |

TestTextOptions interface methods:

| Method | Description |
| :-------- | :-------- |
| [testStandard](#method_testStandard) | Validates standard which is camelCase |

TestTextOptions TestLegacy interface methods:

| Method | Description |
| :-------- | :-------- |
| [testlegacy](#method_testlegacy) | Validates legacy which is lowercase |

TestTextOptions TestKeep interface methods:

| Method | Description |
| :-------- | :-------- |
| [TestKeeP](#method_TestKeeP) | Validates keep which is as same as it's coded |

TestTextOptions TestCustom interface methods:

| Method | Description |
| :-------- | :-------- |
| [TESTCUSTOM](#method_TESTCUSTOM) | Validates custom generation |

TestUtils interface methods:

| Method | Description |
| :-------- | :-------- |
| [crash](#method_crash) | Causes a crash |

<a id="method_versions"></a>
## *versions [<sup>method</sup>](#head_Methods)*

Retrieves a list of JSON-RPC interfaces offered by this service.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | array | mandatory | A list ofsinterfaces with their version numbers<br>*Array length must be at most 255 elements.* |
| result[#] | object | mandatory | *...* |
| result[#].name | string | mandatory | Name of the interface |
| result[#].major | integer | mandatory | Major part of version number |
| result[#].minor | integer | mandatory | Minor part of version number |
| result[#].patch | integer | mandatory | Patch part of version version number |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.versions"
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": [
    {
      "name": "JController",
      "major": 1,
      "minor": 0,
      "patch": 0
    }
  ]
}
```

<a id="method_exists"></a>
## *exists [<sup>method</sup>](#head_Methods)*

Checks if a JSON-RPC method or property exists.

### Description

This method will return *True* for the following methods/properties: *versions, exists, register, unregister, allocatememory, freeallocatedmemory, testbigstring, testStandard, testlegacy, TestKeeP, TESTCUSTOM, crash*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.method | string | mandatory | Name of the method or property to look up |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | boolean | mandatory | Denotes if the method exists or not |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.exists",
  "params": {
    "method": "versions"
  }
}
```

#### Response

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": false
}
```

<a id="method_register"></a>
## *register [<sup>method</sup>](#head_Methods)*

Registers for an asynchronous JSON-RPC notification.

### Description

This method supports the following event names: *[testEvent](#notification_testEvent), [testevent](#notification_testevent), [TestEvent](#notification_TestEvent), [test_event](#notification_test_event)*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the notification to register for |
| params.id | string | mandatory | Client identifier |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_FAILED_REGISTERED``` | Failed to register for the notification (e.g. already registered) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.register",
  "params": {
    "event": "testEvent",
    "id": "myapp"
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

<a id="method_unregister"></a>
## *unregister [<sup>method</sup>](#head_Methods)*

Unregisters from an asynchronous JSON-RPC notification.

### Description

This method supports the following event names: *[testEvent](#notification_testEvent), [testevent](#notification_testevent), [TestEvent](#notification_TestEvent), [test_event](#notification_test_event)*.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.event | string | mandatory | Name of the notification to register for |
| params.id | string | mandatory | Client identifier |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_FAILED_UNREGISTERED``` | Failed to unregister from the notification (e.g. not yet registered) |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.unregister",
  "params": {
    "event": "testEvent",
    "id": "myapp"
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

<a id="method_allocatememory"></a>
## *allocatememory [<sup>method</sup>](#head_Methods)*

Allocates Memory as given size of MB.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.size | integer | mandatory | *...* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to allocate memory |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.allocatememory",
  "params": {
    "size": 0
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

<a id="method_freeallocatedmemory"></a>
## *freeallocatedmemory [<sup>method</sup>](#head_Methods)*

Frees the allocated memory.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to free allocated memory |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.freeallocatedmemory"
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

<a id="method_testbigstring"></a>
## *testbigstring [<sup>method</sup>](#head_Methods)*

Validates big string over proxy-stub with given length of KB.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.length | integer | mandatory | *...* |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to verify |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.testbigstring",
  "params": {
    "length": 0
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

<a id="method_testStandard"></a>
## *testStandard [<sup>method</sup>](#head_Methods)*

Validates standard which is camelCase.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.firstTestParam | integer | mandatory | *...* |
| params.secondTestParam | integer | mandatory | *...* |
| params.thirdTestParam | object | mandatory | *...* |
| params.thirdTestParam.testDetailsFirst | string | mandatory | *...* |
| params.thirdTestParam.testDetailsSecond | string | mandatory | *...* |
| params.fourthTestParam | string | mandatory | *...* (must be one of the following: *FIRST_OPTION, SECOND_OPTION, THIRD_OPTION*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to verify |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.testStandard",
  "params": {
    "firstTestParam": 0,
    "secondTestParam": 0,
    "thirdTestParam": {
      "testDetailsFirst": "...",
      "testDetailsSecond": "..."
    },
    "fourthTestParam": "SECOND_OPTION"
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

<a id="method_testlegacy"></a>
## *testlegacy [<sup>method</sup>](#head_Methods)*

Validates legacy which is lowercase.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.firsttestparam | integer | mandatory | *...* |
| params.secondtestparam | integer | mandatory | *...* |
| params.thirdtestparam | object | mandatory | *...* |
| params.thirdtestparam.testdetailsfirst | string | mandatory | *...* |
| params.thirdtestparam.testdetailssecond | string | mandatory | *...* |
| params.fourthtestparam | string | mandatory | *...* (must be one of the following: *FirstOption, SecondOption, ThirdOption*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to verify |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.testlegacy",
  "params": {
    "firsttestparam": 0,
    "secondtestparam": 0,
    "thirdtestparam": {
      "testdetailsfirst": "...",
      "testdetailssecond": "..."
    },
    "fourthtestparam": "SecondOption"
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

<a id="method_TestKeeP"></a>
## *TestKeeP [<sup>method</sup>](#head_Methods)*

Validates keep which is as same as it's coded.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.firstTestParaM | integer | mandatory | *...* |
| params.secondTestParaM | integer | mandatory | *...* |
| params.thirdTestParaM | object | mandatory | *...* |
| params.thirdTestParaM.testDetailsFirst | string | mandatory | *...* |
| params.thirdTestParaM.testDetailsSecond | string | mandatory | *...* |
| params.fourthTestParaM | string | mandatory | *...* (must be one of the following: *FIRST_OPTION, SECOND_OPTION, ThirdOption*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to verify |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.TestKeeP",
  "params": {
    "firstTestParaM": 0,
    "secondTestParaM": 0,
    "thirdTestParaM": {
      "testDetailsFirst": "...",
      "testDetailsSecond": "..."
    },
    "fourthTestParaM": "SECOND_OPTION"
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

<a id="method_TESTCUSTOM"></a>
## *TESTCUSTOM [<sup>method</sup>](#head_Methods)*

Validates custom generation.

### Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.FIRST_TEST_PARAM | integer | mandatory | *...* |
| params.SECOND_TEST_PARAM | integer | mandatory | *...* |
| params.THIRD_TEST_PARAM | object | mandatory | *...* |
| params.THIRD_TEST_PARAM.TestDetailsFirst | string | mandatory | *...* |
| params.THIRD_TEST_PARAM.TestDetailsSecond | string | mandatory | *...* |
| params.FOURTH_TEST_PARAM | string | mandatory | *...* (must be one of the following: *first_option, second_option, thirdoption*) |

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Errors

| Message | Description |
| :-------- | :-------- |
| ```ERROR_GENERAL``` | Failed to verify |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.TESTCUSTOM",
  "params": {
    "FIRST_TEST_PARAM": 0,
    "SECOND_TEST_PARAM": 0,
    "THIRD_TEST_PARAM": {
      "TestDetailsFirst": "...",
      "TestDetailsSecond": "..."
    },
    "FOURTH_TEST_PARAM": "second_option"
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

<a id="method_crash"></a>
## *crash [<sup>method</sup>](#head_Methods)*

Causes a crash.

### Parameters

This method takes no parameters.

### Result

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| result | null | mandatory | Always null |

### Example

#### Request

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.crash"
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

<a id="head_Notifications"></a>
# Notifications

Notifications are autonomous events triggered by the internals of the implementation and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the TestAutomationUtils plugin:

TestTextOptions interface events:

| Notification | Description |
| :-------- | :-------- |
| [testEvent](#notification_testEvent) |  |

TestTextOptions TestLegacy interface events:

| Notification | Description |
| :-------- | :-------- |
| [testevent](#notification_testevent) |  |

TestTextOptions TestKeep interface events:

| Notification | Description |
| :-------- | :-------- |
| [TestEvent](#notification_TestEvent) |  |

TestTextOptions TestCustom interface events:

| Notification | Description |
| :-------- | :-------- |
| [test event](#notification_test_event) |  |

<a id="notification_testEvent"></a>
## *testEvent [<sup>notification</sup>](#head_Notifications)*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.firstTestParam | integer | mandatory | *...* |
| params.secondTestParam | integer | mandatory | *...* |
| params.thirdTestParam | object | mandatory | *...* |
| params.thirdTestParam.testDetailsFirst | string | mandatory | *...* |
| params.thirdTestParam.testDetailsSecond | string | mandatory | *...* |
| params.fourthTestParam | string | mandatory | *...* (must be one of the following: *FIRST_OPTION, SECOND_OPTION, THIRD_OPTION*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.register",
  "params": {
    "event": "testEvent",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.testEvent",
  "params": {
    "firstTestParam": 0,
    "secondTestParam": 0,
    "thirdTestParam": {
      "testDetailsFirst": "...",
      "testDetailsSecond": "..."
    },
    "fourthTestParam": "SECOND_OPTION"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.testEvent``.

<a id="notification_testevent"></a>
## *testevent [<sup>notification</sup>](#head_Notifications)*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.firsttestparam | integer | mandatory | *...* |
| params.secondtestparam | integer | mandatory | *...* |
| params.thirdtestparam | object | mandatory | *...* |
| params.thirdtestparam.testdetailsfirst | string | mandatory | *...* |
| params.thirdtestparam.testdetailssecond | string | mandatory | *...* |
| params.fourthtestparam | string | mandatory | *...* (must be one of the following: *FirstOption, SecondOption, ThirdOption*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.register",
  "params": {
    "event": "testevent",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.testevent",
  "params": {
    "firsttestparam": 0,
    "secondtestparam": 0,
    "thirdtestparam": {
      "testdetailsfirst": "...",
      "testdetailssecond": "..."
    },
    "fourthtestparam": "SecondOption"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.testevent``.

<a id="notification_TestEvent"></a>
## *TestEvent [<sup>notification</sup>](#head_Notifications)*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.firstTestParam | integer | mandatory | *...* |
| params.secondTestParam | integer | mandatory | *...* |
| params.thirdTestParam | object | mandatory | *...* |
| params.thirdTestParam.testDetailsFirst | string | mandatory | *...* |
| params.thirdTestParam.testDetailsSecond | string | mandatory | *...* |
| params.fourthTestParam | string | mandatory | *...* (must be one of the following: *FIRST_OPTION, SECOND_OPTION, ThirdOption*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.register",
  "params": {
    "event": "TestEvent",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.TestEvent",
  "params": {
    "firstTestParam": 0,
    "secondTestParam": 0,
    "thirdTestParam": {
      "testDetailsFirst": "...",
      "testDetailsSecond": "..."
    },
    "fourthTestParam": "SECOND_OPTION"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.TestEvent``.

<a id="notification_test_event"></a>
## *test_event [<sup>notification</sup>](#head_Notifications)*

### Notification Parameters

| Name | Type | M/O | Description |
| :-------- | :-------- | :-------- | :-------- |
| params | object | mandatory | *...* |
| params.FIRST_TEST_PARAM | integer | mandatory | *...* |
| params.SECOND_TEST_PARAM | integer | mandatory | *...* |
| params.THIRD_TEST_PARAM | object | mandatory | *...* |
| params.THIRD_TEST_PARAM.TestDetailsFirst | string | mandatory | *...* |
| params.THIRD_TEST_PARAM.TestDetailsSecond | string | mandatory | *...* |
| params.FOURTH_TEST_PARAM | string | mandatory | *...* (must be one of the following: *first_option, second_option, thirdoption*) |

### Example

#### Registration

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "method": "TestAutomationUtils.1.register",
  "params": {
    "event": "test_event",
    "id": "myid"
  }
}
```

#### Notification

```json
{
  "jsonrpc": "2.0",
  "method": "myid.test_event",
  "params": {
    "FIRST_TEST_PARAM": 0,
    "SECOND_TEST_PARAM": 0,
    "THIRD_TEST_PARAM": {
      "TestDetailsFirst": "...",
      "TestDetailsSecond": "..."
    },
    "FOURTH_TEST_PARAM": "second_option"
  }
}
```

> The *client ID* parameter is passed within the notification designator, i.e. ``<client-id>.test_event``.

