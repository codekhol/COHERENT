Serial No. for installation: 47610000
This serial no. was used internally at MWC and doesn't belong to
anyone, so feel free to use.

The original COHERENT disk1 has a Y2K problem of course. If this
disk is used for installation the date will be 1970, because it is
unplausible what the hardware clock says. Besides that it doesn't
look great it also will cause problems with utilities like make.
The file disk1-4.2.10-y2k.dsk is disk1 with Y2K fixed 'date' and
'ATclock' commands, so better use this for an installation.

Installation command for the DDK is:

/etc/install Drv_420 /dev/fva0 1
