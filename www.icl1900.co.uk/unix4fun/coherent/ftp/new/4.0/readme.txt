This are the Bellcore MGR sources for COHERENT 4.0.

Because all existing archives with the software are more or less
broken in 2020, I have fixed it up, so that building on COHERENT 4.0
systems is possible. I was careful to not damage anything related
to SunOS, Minix and Linux sources, so this still might build on
the other platforms as well.

October 2020, Udo Munk


To build MGR on COHERENT GNU make and GNU m4 are required, the MWC
versions won't work. It is difficult nowadays to find the old
releases of these, so they are added here properly configured for
COHERENT 4.0, just run make to build.

November 2020, Udo Munk
