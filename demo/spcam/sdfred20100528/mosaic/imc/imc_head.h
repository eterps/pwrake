/* ---------------------------------------------------------------------------
 *		                 imc_head.h
 *  --------------------------------------------------------------------------
 *
 *  This include file defines a structure common to iraf imh and pix file.
 *  This file is a NOT independent c-include file but to be included by imc.h
 *
 *                                                     Maki Sekiguchi
 * ---------------------------------------------------------------------------
 */

				/* start_addres   size      contents */
	char id[12];		/*    0x000        0xc  Header id ="imhdr" */
	int imhsize;   		/*    0x00c	   0x4	imh file size in WORD */
	DTYP dtype;		/*    0x010	   0x4  data type =3..8,11 */
	int naxis;		/*    0x014	   0x4  # of axis */
	int npx;		/*    0x018	   0x4  # of x pixels in image */
	int npy;		/*    0x01c	   0x4  # of y pixels in image */
	int ukwn1;		/*    0x020	   0x4  always = 1 ? */
	int ukwn2;		/*    0x024	   0x4  always = 1 ? */
	int ukwn3;		/*    0x028	   0x4  always = 1 ? */
	int ukwn4;		/*    0x02c	   0x4  always = 1 ? */
	int ukwn5;		/*    0x030	   0x4  always = 1 ? */

	int ndatx;		/*    0x034	   0x4  # of x data in file */
	int ndaty;		/*    0x038	   0x4  # of y data in file */
	int ukwn6;   		/*    0x03c        0x4  always = 1 ? */
	int ukwn7; 	  	/*    0x040        0x4  always = 1 ? */
	int ukwn8;   		/*    0x044        0x4  always = 1 ? */
	int ukwn9;   		/*    0x048        0x4  always = 1 ? */
	int ukwn10;   		/*    0x04c        0x4  always = 1 ? */
	int ukwn11;   		/*    0x050        0x4  don't know what */
	int ukwn12;   		/*    0x054        0x4  don't know what */

	int off1;		/*    0x058	   0x4  some offset ? */
	int off2;		/*    0x05c	   0x4  some offset ? */
	int off3;		/*    0x060	   0x4  some offset ? */
	int ukwn13;   		/*    0x064        0x4  don't know what */
	int ukwn14;   		/*    0x068        0x4  don't know what */

	int tcreate;		/*    0x06c	   0x4  seconds since creation */
	int tmodify;		/*    0x070        0x4  seconds since last modi */
	int ukwn15;   		/*    0x074        0x4  don't know what */
	int ukwn16;   		/*    0x078        0x4  don't know what */
	int ukwn17;   		/*    0x07c        0x4  don't know what */

	char void_1[284];	/*    0x080        284=0x11c Not used */
	char pixnam[160];	/*    0x19c        160=0xa0  Name of pixel file */
	char imhnam[160];	/*    0x23c        160=0xa0  Name of header file */
				/*    0x2dc					 */

/* ------------------------------- end of imc_head.h ------------------------ */
