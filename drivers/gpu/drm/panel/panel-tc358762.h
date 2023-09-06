
#ifndef __PANEL_TC358762_H__
#define __PANEL_TC358762_H__

/* DSI D-PHY Layer Registers */
#define	D0W_DPHYCONTTX		0x0004		/* Data Lane 0 DPHY TX */
#define	CLW_DPHYCONTRX		0x0020		/* Clock Lane DPHY RX */
#define	D0W_DPHYCONTRX		0x0024		/* Data Land 0 DPHY Rx */
#define	D1W_DPHYCONTRX		0x0028		/* Data Lane 1 DPHY Rx */
#define	D2W_DPHYCONTRX		0x002c		/* Data Lane 2 DPHY Rx */
#define	D3W_DPHYCONTRX		0x0030		/* Data Lane 3 DPHY Rx */
#define	COM_DPHYCONTRX		0x0038		/* DPHY Rx Common */
#define	MON_DPHYRX			0x003C		/* */
#define	CLW_CNTRL			0x0040		/* Clock Lane */
#define	D0W_CNTRL			0x0044		/* Data Lane 0 */
#define	D1W_CNTRL			0x0048		/* Data Lane 1 */
#define	DFTMODE_CNTRL		0x0054		/* DFT Mode */

#define	RXCKCOUNT_CNTRL		0x0058		/*  */
#define	RXCKCOUNT_STATUS	0x005C		/*  */
#define	RXCKCOUNT_LENGTH1	0x0060		/*  */
#define	RXCKCOUNT_LENGTH2	0x0064		/*  */
#define	RXCKCOUNT_RESULT1	0x0068		/*  */
#define	RXCKCOUNT_RESULT2	0x006C		/*  */
#define	PATTERNCAP_MODE		0x00C0		/*  */
#define	MISR_INT			0x00D4		/*  */
#define	MISR_MASK			0x00D8		/*  */
#define	MISR_COUNT			0x00E0		/*  */
#define	MISR_RESULT1		0x00E4		/*  */
#define	MISR_RESULT2		0x00E8		/*  */

/* DSI PPI Layer Registers */
#define	PPI_STARTPPI		0x0104		/* Start control bit */
#define	PPI_BUSYPPI			0x0108		/* Busy bit */
#define	PPI_LINEINITCNT		0x0110		/* Line In initialization */
#define	PPI_LPTXTIMECNT		0x0114		/* LPTX timing signal */
#define	PPI_CLS_ATMR		0x0140		/* Analog timer fcn */
#define	PPI_D0S_ATMR		0x0144		/* Analog timer fcn Lane 0 */
#define	PPI_D1S_ATMR		0x0148		/* Analog timer fcn Lane 1 */
#define	PPI_D0S_CLRSIPOCOUNT	0x0164		/* Assertion timer Lane 0 */
#define	PPI_D1S_CLRSIPOCOUNT	0x0168		/* Assertion timer Lane 1 */
#define CLS_PRE				0x0180		/* PHY IO cntr */
#define D0S_PRE				0x0184		/* PHY IO cntr */
#define D1S_PRE				0x0188		/* PHY IO cntr */
#define CLS_PREP			0x01A0		/* PHY IO cntr */
#define D0S_PREP			0x01A4		/* PHY IO cntr */
#define D1S_PREP			0x01A8		/* PHY IO cntr */
#define CLS_ZERO			0x01C0		/* PHY IO cntr */
#define	D0S_ZERO			0x01C4		/* PHY IO cntr */
#define	D1S_ZERO			0x01C8		/* PHY IO cntr */
#define PPI_CLRFLG			0x01E0		/* PRE cntrs */
#define PPI_CLRSIPO			0x01E4		/* Clear SIPO */
#define PPI_HSTimeout		0x01F0		/* HS RX timeout */
#define PPI_HSTimeoutEnable	0x01F4		/* Enable HS Rx Timeout */

/* DSI Protocol Layer Registers */
#define DSI_STARTDSI		0x0204		/* DSI TX start bit */
#define DSI_BUSYDSI			0x0208		/* DSI busy bit */
#define DSI_LANEENABLE		0x0210		/* Lane enable */
#define DSI_LANESTATUS0		0x0214		/* HS Rx mode */
#define DSI_LANESTATUS1		0x0218		/* ULPS or STOP state */
#define DSI_INTSTATUS		0x0220		/* Interrupt status */
#define DSI_INTMASK			0x0224		/* Interrupt mask */
#define DSI_INTCLR			0x0228		/* Interrupt clear */
#define DSI_LPTXTO			0x0230		/* LP Tx Cntr */
#define DSI_MODE			0x0260		/*  */
#define DSI_PAYLOAD0		0x0268		/*  */
#define DSI_PAYLOAD1		0x026C		/*  */
#define DSI_SHORTPKTDAT		0x0270		/*  */
#define DSI_SHORTPKTREQ		0x0274		/*  */
#define DSI_BTASTA			0x0278		/*  */
#define DSI_BTACLR			0x027C		/*  */

/* DSI General Registers */
#define	DSIERRCNT			0x0300		/* DSI Error Count */
#define	DSISIGMOD			0x0304		/*  */

/* DSI Application Layer Registers */
#define APLCTRL				0x0400		/* Application Layer Cntrl */
#define APLSTAT				0x0404		/*  */
#define APLERR				0x0408		/* Application Layer Cntrl */
#define PWRMOD				0x040C		/*  */
#define RDPKTLN				0x0410		/* Packet length */
#define PXLFMT				0x0414		/*  */
#define MEMWRCMD			0x0418		/*  */

#define LCDCTRL0			0x0420		/*  */
#define HSR					0x0424		/*  */
#define HDISPR				0x0428		/*  */
#define VSR					0x042C		/*  */
#define VDISPR				0x0430		/*  */
#define VFUEN				0x0434		/*  */

#define DBICTRL				0x0440		/*  */

/* Video Path Registers */
#define	SPICTRL				0x0450		/*  */
#define SPITCR1				0x0454		/*  */

/* System Registers */
#define SYSSTAT				0x0460		/* System Status */
#define SYSCTRL				0x0464		/*  */
#define SYSPLL1				0x0468		/*  */
#define SYSPLL2				0x046C		/*  */
#define SYSPLL3				0x0470		/*  */
#define SYSPMCTRL			0x047c		/*  */

/* GPIO Registers */
#define GPIOC				0x0480		/* GPIO Control */
#define GPIOO				0x0484		/* GPIO Output */
#define GPIOI				0x0488		/* GPIO Input */

/* I2C Registers */
#define I2CCLKCTRL			0x0490		/* LVDS Mux Input - Bit 0 to 3 */

/* Chip Revision Registers */
#define IDREG				0x04A0		/* Chip and Revision ID */

/* Debug Register */
#define DEBUG01				0x04C0		/*  */

/* Command Queue Register */
#define WCMDQUE				0x0500		/*  */
#define RCMDQUE				0x0504		/*  */

#define SPI_SEL_CS0			0x0002		/*  */


/*DSI DCS commands */
#define DCS_READ_NUM_ERRORS     0x05
#define DCS_READ_POWER_MODE     0x0a
#define DCS_READ_MADCTL         0x0b
#define DCS_READ_PIXEL_FORMAT   0x0c
#define DCS_RDDSDR              0x0f
#define DCS_SLEEP_IN            0x10
#define DCS_SLEEP_OUT           0x11
#define DCS_DISPLAY_OFF         0x28
#define DCS_DISPLAY_ON          0x29
#define DCS_COLUMN_ADDR         0x2a
#define DCS_PAGE_ADDR           0x2b
#define DCS_MEMORY_WRITE        0x2c
#define DCS_TEAR_OFF            0x34
#define DCS_TEAR_ON             0x35
#define DCS_MEM_ACC_CTRL        0x36
#define DCS_PIXEL_FORMAT        0x3a
#define DCS_BRIGHTNESS          0x51
#define DCS_CTRL_DISPLAY        0x53
#define DCS_WRITE_CABC          0x55
#define DCS_READ_CABC           0x56
#define DCS_GET_ID1             0xda
#define DCS_GET_ID2             0xdb
#define DCS_GET_ID3             0xdc

#define TC358762_WIDTH		960
#define TC358762_HEIGHT		540
#define TC358762_PCLK		41600
#define TC358762_HFP		176
#define TC358762_HSW		20
#define TC358762_HBP		86
#define TC358762_VFP		9
#define TC358762_VSW		3
#define TC358762_VBP		12

//#define DISPLAY_DEBUG
#ifdef DISPLAY_DEBUG
typedef enum {
	PANELCTL_LEVEL_SHUTDOWN = 0,
	PANELCTL_LEVEL_HALT,
	PANELCTL_LEVEL_SLEEP,
	PANELCTL_LEVEL_STANDBY,
	PANELCTL_LEVEL_DISPOFF,
	PANELCTL_LEVEL_ACTIVE,
	PANELCTL_LEVEL_COUNT
} PANELCTL_RUNLEVEL;
#endif // DSIPLAY_DEBUG

#endif
