# ThunderNanoServices Release Notes R5.2

## introduction

This document describes the new features and changes introduced in ThunderNanoServices R5.2 (compared to the latest R5.1 release).


##  Changes and New Features

### Change: TestAutomation plugin

The TestAutomation plugin was extended to extend the possibilities and test new Thunder features. It is now possible to make the Plugin crash (for testing handling of crashing plugins, e.g. in combination with the Monitor plugin) and to test the new JSON-RPC casing options.

### Change: IDictionary

The Dictionary plugin was adapted to make it work with the improved Dictionary interface, give it a JSON-RPC interface, and fix some of the existing issues with it.
Please note the RESTful interface was removed and persisting the non volatile keys is not working at the moment (as it was broken anyway).

### Feature: JS engine

A new plugin was added that serves as a JavaScript engine. 

### Feature: example

New examples were added:
* dynamic JSON-RPC error messages and error codes
* SecurityAgent test tool
* PluginSmartInterfaceType example
