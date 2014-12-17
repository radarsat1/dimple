# Getting Phantom Omni (firewire) working with Linux

Ubuntu 14.10 was used. Using your OpenHaptics distribution (OpenHapticsAE_Linux_v3_0 was used here),

cd OpenHapticsAE_Linux_v3_0

dpkg -i 'PHANTOM Device Drivers/32-bit/phantomdevicedrivers_4.3-3_i386.deb'

dpkg -i 'OpenHaptics-AE 3.0/32-bit/openhaptics-ae_3.0-2_i386.deb'

## Create a dummy raw1394 module

https://wiki.sofa-framework.org/tdev/wiki/HowTo/SensableWithoutRaw1394

## Make a dkms package for it

http://basilevsthecat.blogspot.ca/2011/11/how-to-build-dkms-debian-package.html

Ensure directory containing `raw1394.c` is called `dummy-raw1394-1`

cd dummy-raw1394-1

sudo apt-get install debhelper dh-make

dh_make --single --packagename dummy-raw1394 --native --copyright gpl2

Edit debian/rules:

    #!/usr/bin/make -f
    DPKG_EXPORT_BUILDFLAGS = 1
    include /usr/share/dpkg/default.mk
    
    VERSION=$(shell dpkg-parsechangelog |grep ^Version:|cut -d ' ' -f 2)
    
    %:
    	dh $@ --with dkms
    
    override_dh_install:
    	dh_install Makefile raw1394.c usr/src/dummy-raw1394-$(VERSION)/
    
    override_dh_dkms:
    	dh_dkms -V $(VERSION)
    
Edit debian/dummy-raw1394.dkms:

    PACKAGE_VERSION="#MODULE_VERSION#"
    PACKAGE_NAME="dummy-raw1394"
    CLEAN="make clean"
    BUILT_MODULE_NAME[0]="raw1394"
    BUILT_MODULE_LOCATION[0]="./"
    DEST_MODULE_LOCATION[0]="/kernel/extra"
    MAKE[1]="make"
    AUTOINSTALL="yes"
    
dpkg-buildpackage -us -uc -b

sudo dpkg -i ../dummy-raw1394_1_i386.deb

## Fake having old version of libraw1394 (hope it's compatible!)

cd /usr/lib/i386-linux-gnu

sudo ln -s libraw1394.so.11.1.0 libraw1394.so.8

## Set up permissions for /dev/fw* and autoload the dummy raw1394 module

sudoedit /etc/udev/rules.d/80-firewire.rules

    KERNEL=="fw*", GROUP="haptics", MODE="0664", RUN+="/sbin/modprobe raw1394"

sudo addgroup haptics

sudo usermod `whoami` -a -G haptics

Reboot.  Check that `raw1394.ko` loaded automatically and that you
have permission to access `/dev/fw*` (or whichever firewire device.)

lsmod | grep raw1394

ls -l /dev/fw*
