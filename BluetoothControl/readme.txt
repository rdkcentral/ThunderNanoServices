# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

Serial Console and Bluetooth on RaspberryPi:
----------------------------------------------------------------------------------------------
If you would like to enable bluetooth and a serial terminal on the pi, make sure the following
entries are enables in the following RPI files:

/boot/config.txt
enable_uart=1
core_freq=250

Note: /boot/config.txt must not include the following line:
dtoverlay=pi3-miniuart-bt

/boot/cmdline.txt
... console=ttyS0,115200

/etc/inittab:
ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100 # GENERIC_SERIAL


Implementation notes
--------------------
- during pairing discovery needs to be disabled and, if needed, re-enabled afterwards
