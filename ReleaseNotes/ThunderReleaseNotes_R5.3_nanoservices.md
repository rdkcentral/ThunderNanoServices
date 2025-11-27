# ThunderNanoServices Release Notes R5.3

## introduction

This document describes the new features and changes introduced in ThunderNanoServices R5.3 (compared to the latest R5.2 release).


##  Changes and New Features

### Change: Multiple plugins changed for Thunder 5.3

Multiple plugins were changed for Thunder 5.3

### Feature: QA interfaces

Multiple plugins were added in the QA section which can be used for Thunder QA activities.

See [here](https://github.com/rdkcentral/ThunderNanoServices/tree/R5_3/tests) for these plugins.

### Feature: PluginInitializerService 

The plugin PluginInitializerService was added which can be used in case it is desirable to massively activate multiple plugins in parallel.
This plugin makes sure this will not overfload Thunder with these requests but activates them in a controlled manor.

The plugin can be found [here](https://github.com/rdkcentral/ThunderNanoServices/tree/R5_3/PluginInitializerService)

### Feature: examples

New examples were added [here](https://github.com/rdkcentral/ThunderNanoServices/tree/R5_3/examples) :
* CustomErrorCode
* GeneratorShowCase

