/* $Header: /src386/usr/include/sys/RCS/me.h,v 1.2 92/09/29 09:26:54 bin Exp Locker: bin $ */
#ifndef	ME_H
#define	ME_H
/*
 * Maynard Hard Disk Controller
 *
 * $Log:	me.h,v $
 * Revision 1.2  92/09/29  09:26:54  bin
 * updated with kernel 63 src
 * 
 * Revision 1.1	88/03/24  17:48:28	src
 * Initial revision
 * 
 * /usr/src/sys/RCS/hd.h,v 1.2 84/06/29 18:02:04
 */

/*
 * Hard disk (configurable) parameters
 */
#define	TBLOCK		((daddr_t) 0)
#define	TBOFF		0x1A0			/* Disk offset: hd_parm	*/
#define	SDEV		0x80			/* Special drive access */
#define NPARTN		4			/* 4 partitions/drive	*/
#define PARTN(dev)	(dev & 3)
#define	DRIVE(dev)	((dev & 0177) / NPARTN)
#define	SYS_DOS		1			/* System Flag: MS-DOS	*/
#define	SYS_COH		2			/* 		Coherent*/
typedef unsigned char uchar;

/*
 * Parameters as resident on hard disk
 */
struct hd_parm {
	uchar	hd_vol_id[16];			/* volume id		*/
	struct	hd_config_s {			/* Drive configuration	*/
		uchar	hdc_cyls_h, hdc_cyls_l;	/* # cylinders		*/
		uchar	hdc_heads;		/* # heads		*/
		uchar	hdc_rwcc_h, hdc_rwcc_l;	/* cyl #, reduced write	*/
		uchar	hdc_wpcc_h, hdc_wpcc_l;	/* cyl #, write precomp */
		uchar	hdc_eccl;		/* ecc length		*/
		uchar	hdc_step;		/* Stepper option	*/
		uchar	hdc_nspt;		/* # sectors/track	*/
		uchar	hdc_ship_h, hdc_ship_l;	/* Shipping zone cyl	*/
		uchar	hdc_fill[2];
	} hd_config;
	struct	hd_partn_s {			/* Partition description*/
		uchar	hdp_boot;		/* Boot indicator (0x80)*/
		uchar	hdp_bhd,hdp_bsec,hdp_bcyl; /* Begin hd,sec,cyl	*/
		uchar	hdp_sys;		/* System Indicator	*/
		uchar	hdp_ehd,hdp_esec,hdp_ecyl; /* Ending hd,sec,cyl */
		/* WARNING: Big Endians have to modify following decl	*/
		daddr_t	hdp_base;		/* Base blk # for part	*/
		daddr_t	hdp_size;		/* # blks in partition	*/
	} hd_partn[NPARTN];
	uchar	hd_sig[2];
};

/*
 * Parameter subset as resident in memory
 */
struct d_parm {
	daddr_t	d_size;				/* Maximum blk# in SDEV	*/
	struct	hd_config_s d_cfg;
	struct	d_partn_s {
		daddr_t p_base;			/* Base blk # for part	*/
		daddr_t p_size;			/* # blks in partition	*/
	} d_partn[NPARTN];
	uchar	d_init;
};

#endif
