/* (-lgl
 * 	COHERENT Version 3.0
 * 	Copyright (c) 1982, 1993 by Mark Williams Company.
 * 	All rights reserved. May not be copied without permission.
 -lgl) */
/*
 * ebcdic.h
 */

#ifndef	__EBCDIC_H__
#define	__EBCDIC_H__

/*
 * EBCDIC Macro Definitions.
 */

#define	E_NUL	0x00	/* Null			*/
#define	E_SOH	0x01	/* Start Of Header	*/
#define	E_STX	0x02	/* Start Of Text	*/
#define	E_ETX	0x03	/* End Of Text		*/
#define	E_HT	0x05	/* Horizontal Tab	*/
#define	E_DEL	0x07	/* Delete		*/
#define	E_VT	0x0B	/* Vertical Tabulation	*/
#define	E_FF	0x0C	/* Form Feed		*/
#define	E_CR	0x0D	/* Carriage Return	*/
#define	E_SO	0x0E	/* Stand Out		*/
#define	E_SI	0x0F	/* Stand In		*/
#define	E_DLE	0x10	/* Data Link Escape	*/
#define	E_DC1	0x11	/* Data Ctrl 1		*/
#define	E_DC2	0x12	/* Data Ctrl 2		*/
#define	E_DC3	0x13	/* Data Ctrl 3		*/
#define	E_BS	0x16	/* Backspace		*/
#define	E_CAN	0x18	/* Cancel		*/
#define	E_EM	0x19	/*			*/
#define	E_FS	0x1C	/*			*/
#define	E_GS	0x1D	/*			*/
#define	E_RS	0x1E	/*			*/
#define	E_US	0x1F	/*			*/
#define	E_LF	0x25	/* Line Feed		*/
#define	E_ETB	0x26	/*			*/
#define	E_ESC	0x27	/* Escape		*/
#define	E_ENQ	0x2D	/* Enquiry		*/
#define	E_ACK	0x2E	/* Acknowledge		*/
#define	E_BEL	0x2F	/* Bell			*/
#define	E_EOT	0x37	/* End Of Transmission	*/
#define	E_DC4	0x3C	/* Data Ctrl 4		*/
#define	E_NAK	0x3D	/* Negative Acknowledge	*/
#define	E_SYN	0x32	/* Synchronization	*/
#define	E_SUB	0x3F	/*			*/

#endif
