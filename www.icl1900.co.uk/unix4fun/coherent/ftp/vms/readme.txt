COHERENT-4.2.10-NOV-2020.ova Virtualbox VM with COHERENT 4.2.10,
			DDK 4.2.10, X11.
			MWC sources for kernels, development tools, system
			libraries etc. under /u1/src,
			GNU compilers 2.5.6 under /u1/gnu-old,
			complete GNU bits under /u1/gnu,
			Users have no passwort set.
COHERENT-4.2.10-disk1-NOV-2020.vmdk.gz
			Just the disk image from above VM for usage with
			other virtualizations.

COHERENT-4.2.14-NOV-2020.ova Virtualbox VM with COHERENT 4.2.14,
			DDK 4.2.14, X11,
			MWC sources for kernel 4.2.14 in /usr/src/sys,
			complete GNU bits under /u1/gnu,
			Users have no passwort set.
COHERENT-4.2.14-disk1-NOV-2020.vmdk.gz
			Just the disk image from above VM for usage with
			other virtualizations.
COHERENT-4.2.14-gr.ova	Virtualbox VM with 4.2.14 as above, but with
			loadable keyboard driver and german keyboard
			table loaded.

The 4.2 VM's can be used with Virtualbox, QEMU and PCem, for example:

pcem-coherent-4.2.10-NOV-2020.img.gz
pcem-coherent-4.2.14-NOV-2020.img.gz
			Type 47 harddisk image, 1015 cyls, 63 secs, 16 heads
			converted from above Virtualbox disk images. System
			configuration modified to run the color X server.
			For usage with PCem configured with ET4000 SVGA.

<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

pcem-coherent-4.0.68-NOV-2020.img.gz
			Type 32 harddisk image, 1024 cyls, 17 secs, 15 heads
			with installation of COHERENT 4.0.1 and DDK.
			The bitmap graphics driver and vgalib for console
			graphics is installed. The Bellcore MGR window
			manager is installed, but has issues. On PCem
			MGR will not work because of problems with the tty
			line discipline. On QEMU it runs fine because
			there the serial ports cannot be misconfigured.

pcem-coherent-4.0.72-NOV-2020.img.gz
			Type 32 harddisk image, 1024 cyls, 17 secs, 15 heads
			with installation of COHERENT 4.0.1 updated to r72.
			The bitmap graphics driver and vgalib for console
			graphics is installed. The Bellcore MGR window
			manager is installed and working.
			For usage with PCem, also runs on QEMU 5.1.

pcem-coherent-4.0.74-NOV-2020.img.gz
			Type 32 harddisk image, 1024 cyls, 17 secs, 15 heads
			with installation of COHERENT 4.0.1 updated to r74.
			The bitmap graphics driver and vgalib for console
			graphics is installed. The Bellcore MGR window
			manager is installed and working.
			Kernel r78 sources under /usr/src/sys, the bitmap
			grahics driver is added. Kernel builds and boots
			OK, but graphics not working.
			Also has the full 4.0 sources for everything under
			/usr/src/sys, active dev tools and runtime libraries
			are build from the sources and tested.
			For usage with PCem, doesn't run on QEMU 5.1.

<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

pcem-coherent-3.1-MAR-2018.img.gz
			Type 36 harddisk image, 1024 cyls, 17 secs, 8 heads
			with installation of COHERENT 3.0, updated to 3.1
			and DDK. Development system, no autoboot, type
			'coherent' to boot DDK build kernel. Harddisk has
			4MB swap partition and swapping is enabled in the
			DDK kernel. For usage with PCem, doesn't run on
			QEMU 5.1.

pcem-coherent-3.2-NOV-2020.img.gz
			Type 36 harddisk image, 1024 cyls, 17 secs, 8 heads
			with installation of COHERENT 3.0, updated to 3.1,
			updated to 3.2. Development system, no autoboot,
			type 'coherent' to boot the 3.2.0 kernel.
			Includes complete COHERENT 3.2.1 sources and build
			scripts, so that the whole 3.2.1 system can be build
			with this development system on a second harddisk.
			Also additional third party software included and
			installed under /usr/local as well as some under
			user udo.
			For usage with PCem, also works on Qemu 5.1.

<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
