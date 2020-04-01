# **Amlogic Power Plugin Implementation Details**
This document enlists the platform specific implementation details of Amlogic Thunder Power Plugin.   

<p align="center">
  <img src="./AmlogicPowerPluginArchitecture.png?raw=true"/>
</p>

### **Supported Power Modes**
HP40A RDK Accelerator shall be supporting only two power modes `SuspendToRAM(STR)` and `Normal(ON)` operating mode.   

### **Build Configuration**
Use `POWER_PLATFORM_IMPL="Amlogic"` in the Thunder Power Plugin build configuration to select `Amlogic` implementation.   
