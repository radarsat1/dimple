# Getting the Novint Falcon working with Linux

Ubuntu 14.10 was used.  Dimple contains a patch that adds support for
libnifalcon to CHAI, which it then uses to access the Novint Falcon.

This document explains how to make use of it.  Essentially it is not
handled by Dimple's "bootstrap" system, since configuration is a
little complicated and may require manual intervention.  Instead, the
Dimple build system assumes that libnifalcon is installed and
accessible via pkg-config.

## Get libnifalcon source and build it

Get the require Ubuntu packages:

    $ sudo apt-get install libusb-1.0-0-dev cmake-curses-gui libboost-all-dev

### libftd2xx

Get Linux driver from http://www.ftdichip.com/Drivers/D2XX.htm, called
`libftd2xx1.1.12.tar.gz`.

    $ tar -xzf libftd2xx1.1.12.tar.gz

After untarring, cd to the libftd2xx directory and create a Makefile,

    .PHONY: install
    install:
    	install -m 755 build/x86_64/libftd2xx.so.1.1.12 /usr/lib/
    	install -m 755 build/x86_64/libftd2xx.a /usr/lib/
    	install -m 644 ftd2xx.h /usr/include/
    	install -m 644 WinTypes.h /usr/include/
    	cd /usr/lib; ln -s libftd2xx.so.1.1.12 libftd2xx.so.1
    	cd /usr/lib; ln -s libftd2xx.so.1.1.12 libftd2xx.so

Now use `sudo checkinstall` to install it while creating a Debian
package that may be easily uninstalled.

### libnifalcon

Clone http://github.com/qdot/libnifalcon.git

cd libnifalcon
mkdir build
cd build

As of this writing, there are some problems with the pkg-config file
included in libnifle:

1. Change PC_LIBRARY_NAME in CMakeLists.txt (line 133) from
"libnifalcon" to "nifalcon".

2. Comment out or remove the following, line 139,

    set(PC_LINK_FLAGS "${PC_LINK_FLAGS}-lnifalcon_comm_libusb ")

Run,

    $ ccmake ..

and ensure that it is able to detect `libftd2xx`.  Note, I did not
have success using the `libusb-1.0` communications, as there seems to
be an issue using it for uploading the device firmware.

## Install libnifalcon and ensure it can be found

Install it either locally, or if you wish to install it system wide,
it is recommended to use checkinstall, similar to what was done above
for libftdt2xx.

Eitehr way, use pkg-config to check that libnifalcon can be found in
your environment.  Dimple and CHAI 3D will not use libnifalcon
succesfully unless it can be found via pkg-config:

    pkg-config --libs libnifalcon

## Test libnifalcon

Test using command,

    findfalcons

Other tests are,

    falcon_led
    falcon_test_cli cube

If a Falcon is "found" but "Cannot find falcon" is displayed, then
make sure to check the permissions of the device node, located under
`/dev/bus/usb`.  You can figure out which one by the command,

    $ dmesg | grep 'FALCON'
    [ 8119.359004] usb 4-2: Product: FALCON HAPTIC

Seeing that the "path" is 4-2, look up the following information,

    $ cat /sys/bus/usb/devices/4-2/busnum
    4
    $ cat /sys/bus/usb/devices/4-2/devnum
    6

So the device node is located at,

    $ ls -l /dev/bus/usb/004/006
    crw-rw-r-- 1 root root 189, 389 Jan  7 15:47 /dev/bus/usb/004/006

## Changing permissions

You should run Dimple as a regular user, which means allowing access
to the device.

In this case the device permissions can be made permissible by,

    $ sudo chmod a+rw /dev/bus/usb/004/006

Note that this must be done every time a Falcon is plugged in.
Therefore, a rule under `/etc/udev/rules.d` should be added setting
the group permissions appropriately.  Such a rule is included in
libnifalcon, in the "linux" folder.  However, it needs a small change
to be up to date for current udev.  Open
`libnifalcon/linux/40-novint-falcon-udev.rules` in an editor and
change any mention of `SYSFS` to `ATTRS`:

    SUBSYSTEM=="usb", ACTION=="add", ENV{DEVTYPE}=="usb_device", SYSFS{idVendor}=="0403", SYSFS{idProduct}=="cb48", MODE="0664", GROUP="plugdev"

becomes,

    SUBSYSTEM=="usb", ACTION=="add", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="cb48", MODE="0664", GROUP="plugdev"

Then copy it to `/etc/udev/rules.d/`.  If you unplug and plug in the
haptic device, it should now have permissions,

    crw-rw-r-- 1 root root 189, 389 Jan  7 15:47 /dev/bus/usb/004/006

## Enable libnifalcon usage in CHAI 3D's Makefile

In the dimple directory, run "configure" and ensure that libnifalcon
is found:

    $ ./configure 2>&1 | grep nifalcon
    checking for main in -lnifalcon... yes

Now go to the chai3d directory and recompile:

    $ cd libdeps/chai3d/linux
    $ make clean; make

While it is building, check that the flag
`-D_DISABLE_LIBNIFALCON_DEVICE_SUPPORT` is **not** present in the
compiler flags.

On success, try to compile Dimple again:

    $ cd ../../..
    $ make

At this point, the HapticsSim portion of Dimple should initialize with
success.  If not, check that `findfalcons` and `falcon_led` works
successfully.

In general, some manual configuration of CHAI under the
`libdeps/chai3d` folder may be needed to ensure libnifalcon support is
correctly included.  YMMV.
