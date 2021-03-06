.. SPDX-License-Identifier: GPL-2.0

=============================================
FORE Systems PCA-200E/SBA-200E ATM NIC driver
=============================================

This driver adds support for the FORE Systems 200E-series ATM adapters
to the Linux operating system. It is based on the earlier PCA-200E driver
written by Uwe Dannowski.

The driver simultaneously supports PCA-200E and SBA-200E adapters on
i386, alpha (untested), powerpc, sparc and sparc64 archs.

The intent is to enable the use of different models of FORE adapters at the
same time, by hosts that have several bus interfaces (such as PCI+SBUS,
or PCI+EISA).

Only PCI and SBUS devices are currently supported by the driver, but support
for other bus interfaces such as EISA should not be too hard to add.


Firmware Copyright Notice
-------------------------

Please read the fore200e_firmware_copyright file present
in the linux/drivers/atm directory for details and restrictions.


Firmware Updates
----------------

The FORE Systems 200E-series driver is shipped with firmware data being
uploaded to the ATM adapters at system boot time or at module loading time.
/*(DEBLOBBED)*/


Feedback
--------

Feedback is welcome. Please send success stories/bug reports/
patches/improvement/comments/flames to <lizzi@cnam.fr>.
