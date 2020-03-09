/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*****************************************************************************
 *  GreenPeak NVM LUT global variables definition:
 *   - gpNvm_elementBaseLUTElements
 *   - gpNvm_elementBaseLUTHeader
 *   - gpNvm_InheritedNvmVersionCrc
 *
 *  Selected by GP_NVM_LUT_* defines:
 *   - GP_NVM_LUT_CUSTOM1: Custom configuration 1 (used, e.g., by NOS).
 *   - Not defined or GP_NVM_LUT_DEFAULT: The default configuration.
 *****************************************************************************/

#if defined(GP_NVM_LUT_CUSTOM1)

/*****************************************************************************
 * GP_NVM_LUT_CUSTOM1: Used, e.g., by NOS.
 *****************************************************************************/

const ROM gpNvm_ElementDefine_t gpNvm_elementBaseLUTElements[] = {
    { 0x2000, 0x0002 } /* tagId  0 [0x00] compId 32 [0x20] ... unique tag ID 0x2000 size  2 [0x02] */
    ,
    { 0x2100, 0x0001 } /* tagId  0 [0x00] compId 33 [0x21] ... unique tag ID 0x2100 size  1 [0x01] */
    ,
    { 0x1900, 0x0004 } /* tagId  0 [0x00] compId 25 [0x19] ... unique tag ID 0x1900 size  4 [0x04] */
    ,
    { 0x1901, 0x0001 } /* tagId  1 [0x01] compId 25 [0x19] ... unique tag ID 0x1901 size  1 [0x01] */
    ,
    { 0x1902, 0x0034 } /* tagId  2 [0x02] compId 25 [0x19] ... unique tag ID 0x1902 size 52 [0x34] */
    ,
    { 0x1903, 0x0001 } /* tagId  3 [0x03] compId 25 [0x19] ... unique tag ID 0x1903 size  1 [0x01] */
    ,
    { 0x1904, 0x0002 } /* tagId  4 [0x04] compId 25 [0x19] ... unique tag ID 0x1904 size  2 [0x02] */
    ,
    { 0x1905, 0x0002 } /* tagId  5 [0x05] compId 25 [0x19] ... unique tag ID 0x1905 size  2 [0x02] */
    ,
    { 0x1906, 0x0025 } /* tagId  6 [0x06] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x1907, 0x0025 } /* tagId  7 [0x07] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x1908, 0x0025 } /* tagId  8 [0x08] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x1909, 0x0025 } /* tagId  9 [0x09] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x190A, 0x0025 } /* tagId 10 [0x0A] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x190B, 0x0025 } /* tagId 11 [0x0B] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x2C00, 0x0002 } /* tagId  0 [0x00] compId 44 [0x2C] ... unique tag ID 0x2C00 size  2 [0x02] */
    ,
    { 0x2C01, 0x0006 } /* tagId  1 [0x01] compId 44 [0x2C] ... unique tag ID 0x2C01 size  6 [0x06] */
    ,
    { 0x2900, 0x0002 } /* tagId  0 [0x00] compId 41 [0x29] ... unique tag ID 0x2900 size  2 [0x02] */
};

const ROM gpNvm_LookUpTableHeader_t gpNvm_elementBaseLUTHeader = {
    (UInt16)(sizeof(gpNvm_elementBaseLUTElements) / sizeof(gpNvm_ElementDefine_t)), /* number of elements in gpNvm_elementBaseLUTElements */
    (UInt16)(sizeof(gpNvm_elementBaseLUTElements) / sizeof(gpNvm_ElementDefine_t)), /* we are not reserving elements between inherited LUT and dynamic LUT so link to NVM LUT in first available element */
    0xFBED /* Has to be recalculated whenever gpNvm_elementBaseLUTElements is changed */
};

const ROM gpNvm_VersionCrc_t gpNvm_InheritedNvmVersionCrc = 0xcb69;

#else

/*****************************************************************************
 * GP_NVM_LUT_DEFAULT: Default.
 *****************************************************************************/

const gpNvm_ElementDefine_t gpNvm_elementBaseLUTElements[] = {
    { 0x2000, 0x0002 } /* tagId  0 [0x00] compId 32 [0x20] ... unique tag ID 0x2000 size  2 [0x02] */
    ,
    { 0x2100, 0x0001 } /* tagId  0 [0x00] compId 33 [0x21] ... unique tag ID 0x2100 size  1 [0x01] */
    ,
    { 0x1900, 0x0004 } /* tagId  0 [0x00] compId 25 [0x19] ... unique tag ID 0x1900 size  4 [0x04] */
    ,
    { 0x1901, 0x0001 } /* tagId  1 [0x01] compId 25 [0x19] ... unique tag ID 0x1901 size  1 [0x01] */
    ,
    { 0x1902, 0x0034 } /* tagId  2 [0x02] compId 25 [0x19] ... unique tag ID 0x1902 size 52 [0x34] */
    ,
    { 0x1903, 0x0001 } /* tagId  3 [0x03] compId 25 [0x19] ... unique tag ID 0x1903 size  1 [0x01] */
    ,
    { 0x1904, 0x0002 } /* tagId  4 [0x04] compId 25 [0x19] ... unique tag ID 0x1904 size  2 [0x02] */
    ,
    { 0x1905, 0x0002 } /* tagId  5 [0x05] compId 25 [0x19] ... unique tag ID 0x1905 size  2 [0x02] */
    ,
    { 0x1906, 0x0025 } /* tagId  6 [0x06] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x1907, 0x0025 } /* tagId  7 [0x07] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x1908, 0x0025 } /* tagId  8 [0x08] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x1909, 0x0025 } /* tagId  9 [0x09] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x190A, 0x0025 } /* tagId 10 [0x0A] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x190B, 0x0025 } /* tagId 11 [0x0B] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x190C, 0x0025 } /* tagId 12 [0x0C] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x190D, 0x0025 } /* tagId 13 [0x0D] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x190E, 0x0025 } /* tagId 14 [0x0E] compId 25 [0x19] ... unique tag ID 0x1906 size 37 [0x25] */
    ,
    { 0x2A00, 0x0028 } /* tagId  0 [0x00] compId 42 [0x2A] ... unique tag ID 0x2A00 size 40 [0x28] */
    ,
    { 0x2C00, 0x0001 } /* tagId  0 [0x00] compId 44 [0x2C] ... unique tag ID 0x2C00 size  1 [0x01] */
    ,
    { 0x2C01, 0x0009 } /* tagId  1 [0x01] compId 44 [0x2C] ... unique tag ID 0x2C01 size  9 [0x09] */
    ,
    { 0x3100, 0x0028 } /* tagId  0 [0x00] compId 49 [0x31] ... unique tag ID 0x3100 size 40 [0x28] */
    ,
    { 0x7F00, 0x0002 } /* tagId  0 [0x00] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F01, 0x0002 } /* tagId  1 [0x01] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F02, 0x0002 } /* tagId  2 [0x02] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F03, 0x0002 } /* tagId  3 [0x03] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F04, 0x0002 } /* tagId  4 [0x04] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F05, 0x0002 } /* tagId  5 [0x05] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F06, 0x0002 } /* tagId  6 [0x06] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F07, 0x0002 } /* tagId  7 [0x07] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F08, 0x0002 } /* tagId  8 [0x08] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F09, 0x0004 } /* tagId  9 [0x09] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F0A, 0x0004 } /* tagId 10 [0x0A] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F0B, 0x0004 } /* tagId 11 [0x0B] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F0C, 0x0004 } /* tagId 12 [0x0C] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F0D, 0x0004 } /* tagId 13 [0x0D] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F0E, 0x0004 } /* tagId 14 [0x0E] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F0F, 0x0004 } /* tagId 15 [0x0F] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F10, 0x0004 } /* tagId 16 [0x10] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F11, 0x0004 } /* tagId 17 [0x11] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F12, 0x0002 } /* tagId 18 [0x12] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F13, 0x0002 } /* tagId 19 [0x13] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F14, 0x0002 } /* tagId 20 [0x14] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F15, 0x0002 } /* tagId 21 [0x15] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F16, 0x0002 } /* tagId 22 [0x16] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F17, 0x0002 } /* tagId 23 [0x17] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F18, 0x0002 } /* tagId 24 [0x18] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F19, 0x0002 } /* tagId 25 [0x19] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F1A, 0x0002 } /* tagId 26 [0x1A] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F1B, 0x0004 } /* tagId 27 [0x1B] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F1C, 0x0004 } /* tagId 28 [0x1C] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F1D, 0x0004 } /* tagId 29 [0x1D] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F1E, 0x0004 } /* tagId 30 [0x1E] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F1F, 0x0004 } /* tagId 31 [0x1F] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F20, 0x0004 } /* tagId 32 [0x20] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F21, 0x0004 } /* tagId 33 [0x21] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F22, 0x0004 } /* tagId 34 [0x22] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F23, 0x0004 } /* tagId 35 [0x23] compId 127[0x7F] ... unique tag ID 0x7F00 size  4 [0x04] */
    ,
    { 0x7F24, 0x0002 } /* tagId 36 [0x24] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F25, 0x0002 } /* tagId 37 [0x25] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F26, 0x0002 } /* tagId 38 [0x26] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F27, 0x0002 } /* tagId 39 [0x27] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F28, 0x0002 } /* tagId 40 [0x28] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F29, 0x0002 } /* tagId 41 [0x29] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F2A, 0x0002 } /* tagId 42 [0x2A] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F2B, 0x0002 } /* tagId 43 [0x2B] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F2C, 0x0002 } /* tagId 44 [0x2C] compId 127[0x7F] ... unique tag ID 0x7F00 size  2 [0x02] */
    ,
    { 0x7F2D, 0x0009 } /* tagId 45 [0x2D] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F2E, 0x0009 } /* tagId 46 [0x2E] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F2F, 0x0009 } /* tagId 47 [0x2F] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F30, 0x0009 } /* tagId 48 [0x30] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F31, 0x0009 } /* tagId 49 [0x31] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F32, 0x0009 } /* tagId 50 [0x32] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F33, 0x0009 } /* tagId 51 [0x33] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F34, 0x0009 } /* tagId 52 [0x34] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F35, 0x0009 } /* tagId 53 [0x35] compId 127[0x7F] ... unique tag ID 0x7F00 size  9 [0x09] */
    ,
    { 0x7F36, 0x0001 } /* tagId 54 [0x36] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F37, 0x0001 } /* tagId 55 [0x37] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F38, 0x0001 } /* tagId 56 [0x38] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F39, 0x0001 } /* tagId 57 [0x39] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F3A, 0x0001 } /* tagId 58 [0x3A] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F3B, 0x0001 } /* tagId 59 [0x3B] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F3C, 0x0001 } /* tagId 60 [0x3C] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F3D, 0x0001 } /* tagId 61 [0x3D] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F3E, 0x0001 } /* tagId 62 [0x3E] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F3F, 0x0001 } /* tagId 63 [0x3F] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F40, 0x0001 } /* tagId 64 [0x40] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F41, 0x0001 } /* tagId 65 [0x41] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F42, 0x0001 } /* tagId 66 [0x42] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F43, 0x0001 } /* tagId 67 [0x43] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F44, 0x0001 } /* tagId 68 [0x44] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F45, 0x0001 } /* tagId 69 [0x45] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F46, 0x0001 } /* tagId 70 [0x46] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x7F47, 0x0001 } /* tagId 71 [0x47] compId 127[0x7F] ... unique tag ID 0x7F00 size  1 [0x01] */
    ,
    { 0x9800, 0x000E } /* tagId  0 [0x00] compId 152[0x98] ... unique tag ID 0x9800 size 14 [0x0E] */
    ,
    { 0x9801, 0x000E } /* tagId  1 [0x01] compId 152[0x98] ... unique tag ID 0x9801 size 14 [0x0E] */
    ,
    { 0x9802, 0x000E } /* tagId  2 [0x02] compId 152[0x98] ... unique tag ID 0x9802 size 14 [0x0E] */
    ,
    { 0x9803, 0x000E } /* tagId  3 [0x03] compId 152[0x98] ... unique tag ID 0x9803 size 14 [0x0E] */
    ,
    { 0x9804, 0x000E } /* tagId  4 [0x04] compId 152[0x98] ... unique tag ID 0x9804 size 14 [0x0E] */
    ,
    { 0x9805, 0x000E } /* tagId  5 [0x05] compId 152[0x98] ... unique tag ID 0x9805 size 14 [0x0E] */
    ,
    { 0x9806, 0x000E } /* tagId  6 [0x06] compId 152[0x98] ... unique tag ID 0x9806 size 14 [0x0E] */
    ,
    { 0x9807, 0x000E } /* tagId  7 [0x07] compId 152[0x98] ... unique tag ID 0x9807 size 14 [0x0E] */
    ,
    { 0x9808, 0x000E } /* tagId  8 [0x08] compId 152[0x98] ... unique tag ID 0x9808 size 14 [0x0E] */
    ,
    { 0x9809, 0x0009 } /* tagId  9 [0x09] compId 152[0x98] ... unique tag ID 0x9809 size  9 [0x09] */
    ,
    { 0x980A, 0x0009 } /* tagId 10 [0x0A] compId 152[0x98] ... unique tag ID 0x980A size  9 [0x09] */
    ,
    { 0x980B, 0x0009 } /* tagId 11 [0x0B] compId 152[0x98] ... unique tag ID 0x980B size  9 [0x09] */
    ,
    { 0x980C, 0x0009 } /* tagId 12 [0x0C] compId 152[0x98] ... unique tag ID 0x980C size  9 [0x09] */
    ,
    { 0x980D, 0x0009 } /* tagId 13 [0x0D] compId 152[0x98] ... unique tag ID 0x980D size  9 [0x09] */
    ,
    { 0x980E, 0x0009 } /* tagId 14 [0x0E] compId 152[0x98] ... unique tag ID 0x980E size  9 [0x09] */
    ,
    { 0x980F, 0x0009 } /* tagId 15 [0x0F] compId 152[0x98] ... unique tag ID 0x980F size  9 [0x09] */
    ,
    { 0x9810, 0x0009 } /* tagId 16 [0x10] compId 152[0x98] ... unique tag ID 0x9810 size  9 [0x09] */
    ,
    { 0x9811, 0x0009 } /* tagId 17 [0x11] compId 152[0x98] ... unique tag ID 0x9811 size  9 [0x09] */
    ,
    { 0x9812, 0x0001 } /* tagId 18 [0x12] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x9813, 0x0001 } /* tagId 19 [0x13] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x9814, 0x0001 } /* tagId 20 [0x14] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x9815, 0x0001 } /* tagId 21 [0x15] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x9816, 0x0001 } /* tagId 22 [0x16] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x9817, 0x0001 } /* tagId 23 [0x17] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x9818, 0x0001 } /* tagId 24 [0x18] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x9819, 0x0001 } /* tagId 25 [0x19] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x981A, 0x0001 } /* tagId 26 [0x1A] compId 152[0x98] ... unique tag ID 0x9812 size  1 [0x01] */
    ,
    { 0x0100, 0x0009 } /* tagId 0  [0x00] compId 1  [0x01] ... unique tag ID 0x0100 size  1 [0x01] */
};

const gpNvm_LookUpTableHeader_t gpNvm_elementBaseLUTHeader = {
    (UInt16)(sizeof(gpNvm_elementBaseLUTElements) / sizeof(gpNvm_ElementDefine_t)), /* number of elements in gpNvm_elementBaseLUTElements */
    (UInt16)(sizeof(gpNvm_elementBaseLUTElements) / sizeof(gpNvm_ElementDefine_t)), /* we are not reserving elements between inherited LUT and dynamic LUT so link to NVM LUT in first available element */
    0x5192 /* Has to be recalculated whenever gpNvm_elementBaseLUTElements is changed */
};

const gpNvm_VersionCrc_t gpNvm_InheritedNvmVersionCrc = 0xaba7;

#endif
