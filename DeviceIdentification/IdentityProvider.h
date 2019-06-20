#pragma once

extern "C" {
   // The return value returns a pointer, pointing to the system identity. The length 
   // of the identity can be found in the length parameter. If the return value a
   // nullptr, the length parameter is treated as an error code.
   //
   // This function needs to be implemented by specific code for the platform.
   // this code can be found in this plugin under the implementation folder.

   extern const unsigned char* GetIdentity(unsigned char* length_error);
}
