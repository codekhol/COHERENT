Serial No. for installation: 47610000
This serial no. was used internally at MWC and doesn't belong to
anyone, so feel free to use.

Please be aware that 4.0.1 is a BETA test release, not a production release.
Also the kernel on the 3.5" disks won't work, use the 5.25" disk images
for an installation. And of course use the Y2K fixed installation/update
disks, else the system date will be 1970.

Installation command for the DDK is:
/etc/install Drv_200 /dev/fva1 1

The DDK will update the system to r68.

Do not install the r72 and r74 updates on a system with DDK, the DDK will
not work correct anymore. Either build a r68 system with DDK or an updated
one.

Install the updates with:
/etc/install CohUpd /dev/fva1 2

The updates are needed for running Bellcore MGR or ASC X11R5.
