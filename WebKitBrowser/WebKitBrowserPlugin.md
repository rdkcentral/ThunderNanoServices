<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.WebKit_Browser_Plugin"></a>
# WebKit Browser Plugin

**Version: 1.0**

WebKitBrowser functionality for WPEFramework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the WebKitBrowser plugin. It includes detailed specification of its configuration, methods provided and notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers on the interface described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

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
| <a name="ref.WPEF">[WPEF](https://github.com/WebPlatformForEmbedded/WPEFramework/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | WPEFramework API Reference |

<a name="head.Description"></a>
# Description

The WebKitBrowser plugin provides web browsing functionality based on the WebKit engine.

The plugin is designed to be loaded and executed within the WPEFramework. For more information on WPEFramework refer to [[WPEF](#ref.WPEF)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *WebKitBrowser*) |
| classname | string | Class name: *WebKitBrowser* |
| locator | string | Library name: *libWPEFrameworkWebKitBrowser.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration?.useragent | string | <sup>*(optional)*</sup> The UserAgent used during communication with the web server |
| configuration?.url | string | <sup>*(optional)*</sup> The URL that is loaded upon starting the browser |
| configuration?.injectedbundle | string | <sup>*(optional)*</sup> The name of the .so loaded to extend the HTML5 with customized JavaScript handlers |
| configuration?.cookiestorage | string | <sup>*(optional)*</sup> Post-fixed path to the persistent location on where cookies will be stored |
| configuration?.localstorage | string | <sup>*(optional)*</sup> Post-fixed path to the persistent location on where local-storage will be stored |
| configuration?.certificatecheck | boolean | <sup>*(optional)*</sup> If enabled, SSL certificate error will lead to a page error |
| configuration?.javascript | object | <sup>*(optional)*</sup>  |
| configuration?.javascript?.useLLInt | boolean | <sup>*(optional)*</sup> Enable Low Level Interpreter |
| configuration?.javascript?.useJIT | boolean | <sup>*(optional)*</sup> Enable JIT |
| configuration?.javascript?.useDFG | boolean | <sup>*(optional)*</sup> Enable Data-Flow-Graph-JIT compiler |
| configuration?.javascript?.useFTL | boolean | <sup>*(optional)*</sup> Enable Faster-Than-Light-JIT compiler |
| configuration?.javascript?.useDOM | boolean | <sup>*(optional)*</sup> Enable the DOM-JIT compiler |
| configuration?.javascript?.dumpOptions | string | <sup>*(optional)*</sup> Dump options: 0 = None, 1 = Overridden only, 2 = All, 3 = Verbose |
| configuration?.secure | boolean | <sup>*(optional)*</sup> If false, every domain can access any other domain, otherwise fill in fields in whitelist definition table |
| configuration?.whitelist | object | <sup>*(optional)*</sup>  |
| configuration?.whitelist?.origin | string | <sup>*(optional)*</sup> Origin domain allowed to access domains in domain |
| configuration?.whitelist?.domain | array | <sup>*(optional)*</sup>  |
| configuration?.whitelist?.domain[#] | string | <sup>*(optional)*</sup> Domain allowed to access from origin |
| configuration?.whitelist?.subdomain | string | <sup>*(optional)*</sup> whether it is also OK to access subdomains of domains listed in domain |

<a name="head.Methods"></a>
# Methods

The following API is provided by the plugin via JSON-RPC:

- [status](#method.status)
- [hide](#method.hide)
- [show](#method.show)
- [seturl](#method.seturl)

StateControl methods:

- [suspend](#method.suspend)
- [resume](#method.resume)

This API follows the JSON-RPC 2.0 specification. Refer to [[JSON-RPC](#ref.JSON-RPC)] for more information.


<a name="method.status"></a>
## *status*

Retrieves the WebKit Browser information.

### Description

With this method current running state can be retrieved from the WebKit Browser.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.url | string | The currently loaded URL in the browser |
| result.fps | number | The current number of frames per second the browser is rendering |
| result.suspended | boolean | Determines if the browser is in suspended mode (true) or resumed mode (false) |
| result.hidden | boolean | Determines if the browser is hidden (true) or visible (false) |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WebKitBrowser.1.status"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": {
        "url": "https://www.google.com", 
        "fps": 30, 
        "suspended": false, 
        "hidden": false
    }
}
```
<a name="method.hide"></a>
## *hide*

Hides the WebKit Browser.

### Description

With this method rendering of the WebKit Browser can be stopped. Hiding an already hidden browser has no effect.

Also see: [visibilitychange](#event.visibilitychange)

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WebKitBrowser.1.hide"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.show"></a>
## *show*

Shows the WebKit Browser.

### Description

With this method rendering of the WebKit Browser can be (re)started. Showing a not hidden browser has no effect.

Also see: [visibilitychange](#event.visibilitychange)

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WebKitBrowser.1.show"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.seturl"></a>
## *seturl*

Sets a URL in the WebKit Browser.

### Description

With this method a new URL can be loaded in the WebKit Browser.

Also see: [urlchange](#event.urlchange)

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.url | string | The URL to load |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 15 | ```ERROR_INCORRECT_URL``` | Incorrect URL given |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WebKitBrowser.1.seturl", 
    "params": {
        "url": "https://www.google.com"
    }
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.suspend"></a>
## *suspend <sup>(StateControl)</sup>*

Suspends the service.

### Description

With this method the service can be suspended. Suspending when already in suspended mode has no effect.

Also see: [statechange](#event.statechange)

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WebKitBrowser.1.suspend"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="method.resume"></a>
## *resume <sup>(StateControl)</sup>*

Resumes the service.

### Description

With this method the service can be resumed from suspended mode. Resuming from a not suspended mode has no effect.

Also see: [statechange](#event.statechange)

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "WebKitBrowser.1.resume"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[WPEF](#ref.WPEF)] for information on how to register for a notification.

The following notifications are provided by the plugin:

- [urlchange](#event.urlchange)
- [visibilitychange](#event.visibilitychange)
- [pageclosure](#event.pageclosure)

StateControl events:

- [statechange](#event.statechange)

<a name="event.urlchange"></a>
## *urlchange*

Signals a URL change in the browser.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.url | string | The URL that has been loaded or requested |
| params.loaded | boolean | Determines if the URL has just been loaded (true) or if URL change has been requested (false) |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.urlchange", 
    "params": {
        "url": "https://www.google.com", 
        "loaded": false
    }
}
```
<a name="event.visibilitychange"></a>
## *visibilitychange*

Signals a visibility change of the browser.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.hidden | boolean | Determines if the browser has been hidden (true) or made visible (false) |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.visibilitychange", 
    "params": {
        "hidden": false
    }
}
```
<a name="event.pageclosure"></a>
## *pageclosure*

Notifies that the web page requests to close its window.

### Parameters

This event has no parameters.

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.pageclosure"
}
```
<a name="event.statechange"></a>
## *statechange <sup>(StateControl)</sup>*

Signals a state change of the service.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.suspended | boolean | Determines if the service has entered suspended state (true) or resumed state (false) |

### Example

```json
{
    "jsonrpc": "2.0", 
    "method": "client.events.1.statechange", 
    "params": {
        "suspended": false
    }
}
```
