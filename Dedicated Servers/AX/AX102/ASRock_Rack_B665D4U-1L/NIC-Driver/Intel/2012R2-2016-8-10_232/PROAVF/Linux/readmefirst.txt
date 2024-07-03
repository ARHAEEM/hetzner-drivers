This release includes iavf Linux* Virtual Function Drivers for Intel(R)
Ethernet Network Connections.

The iavf driver supports devices based on the following controllers:
  * Intel(R) Ethernet Controller E810-C
  * Intel(R) Ethernet Controller E810-XXV
  * Intel(R) Ethernet Connection E822-C
  * Intel(R) Ethernet Connection E822-L
  * Intel(R) Ethernet Connection E823-C
  * Intel(R) Ethernet Connection E823-L
  * Intel(R) Ethernet Controller I710
  * Intel(R) Ethernet Controller X710
  * Intel(R) Ethernet Controller XL710
  * Intel(R) Ethernet Network Connection X722
  * Intel(R) Ethernet Controller XXV710
  * Intel(R) Ethernet Controller V710

iavf-x.x.x.tar.gz

Due to the continuous development of the Linux kernel, the drivers are updated
more often than the bundled releases. The latest driver can be found on
http://e1000.sourceforge.net (and also on http://downloadcenter.intel.com).

This release includes RPM packages that contain:
* Driver signed with Intel's private key in precompiled kernel module form
* Complete source code for above driver
* Intel's public key

This release includes the Intel public key to allow you to authenticate the
signed driver in secure boot mode. To authenticate the signed driver, you must
place Intel's public key in the UEFI Secure Boot key database.

If you decide to recompile the .ko module from the provided source files, the
new .ko module will not be signed with any key. To use this .ko module in
Secure Boot mode, you must sign it yourself with your own private key and add
your public key to the UEFI Secure Boot key database.

The driver kernel module for a specific kernel version can be used with errata
kernels within the same minor OS version, unless the errata kernel broke kABI.
Whenever you update your kernel with an errata kernel, you must reinstall the
driver RPM package.


