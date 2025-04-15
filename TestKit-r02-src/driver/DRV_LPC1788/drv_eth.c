// drv_eth.c - ETHERNET driver tasks
//
//   Copyright (c) 1997-2012.
//   T.E.S.T. srl
//

//
// This module is provided as a ETHERNET driver.
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

#define NULLSEMA ((SEMA)0)

//----------------------------------------------------------------------------
// only if we are well accepted

#ifdef USE_ETHERNET

/* EMAC Memory Buffer configuration for 16K Ethernet RAM. */
#define NUM_RX_FRAG         4           /* Num.of RX Fragments 4*1536= 6.0kB */
#define NUM_TX_FRAG         2           /* Num.of TX Fragments 3*1536= 4.6kB */
#define ETH_FRAG_SIZE       1536        /* Packet Fragment size 1536 Bytes   */

#define ETH_MAX_FLEN        1536        /* Max. Ethernet Frame Size          */

// EMAC variables located in AHB SRAM bank 0
unsigned long rx_desc_base[NUM_RX_FRAG * 2] __attribute__ ((aligned (4), section(".ahbram0"))) ;
unsigned long rx_stat_base[NUM_RX_FRAG * 2] __attribute__ ((aligned (4), section(".ahbram0"))) ;
unsigned long tx_desc_base[NUM_TX_FRAG * 2] __attribute__ ((aligned (4), section(".ahbram0"))) ;
unsigned long tx_stat_base[NUM_TX_FRAG * 1] __attribute__ ((aligned (4), section(".ahbram0"))) ;

unsigned char rx_buf_base[NUM_RX_FRAG * ETH_FRAG_SIZE] __attribute__ ((aligned (4), section(".ahbram0"))) ;
unsigned char tx_buf_base[NUM_TX_FRAG * ETH_FRAG_SIZE] __attribute__ ((aligned (4), section(".ahbram0"))) ;

#define RX_DESC_BASE        ((unsigned long)(&rx_desc_base[0]))
#define RX_STAT_BASE        ((unsigned long)(&rx_stat_base[0]))

#define TX_DESC_BASE        ((unsigned long)(&tx_desc_base[0]))
#define TX_STAT_BASE        ((unsigned long)(&tx_stat_base[0]))

#define RX_BUF_BASE         ((unsigned long)(&rx_buf_base[0]))
#define TX_BUF_BASE         ((unsigned long)(&tx_buf_base[0]))

// RX and TX descriptor and status definitions.
#define RX_DESC_PACKET(i)   rx_desc_base[(i)*2]
#define RX_DESC_CTRL(i)     rx_desc_base[1+((i)*2)]
#define RX_STAT_INFO(i)     rx_stat_base[(i)*2]
#define RX_STAT_HASHCRC(i)  rx_stat_base[1+((i)*2)]

#define TX_DESC_PACKET(i)   tx_desc_base[(i)*2]
#define TX_DESC_CTRL(i)     tx_desc_base[1+((i)*2)]
#define TX_STAT_INFO(i)     rx_stat_base[i]
#define RX_BUF(i)           ((unsigned long)(&rx_buf_base[(i)*ETH_FRAG_SIZE]))
#define TX_BUF(i)           ((unsigned long)(&tx_buf_base[(i)*ETH_FRAG_SIZE]))

// EMAC variables located in AHB SRAM bank 1
//#define RX_DESC_BASE        0x20000000
//#define RX_STAT_BASE        (RX_DESC_BASE + NUM_RX_FRAG*8)
//#define TX_DESC_BASE        (RX_STAT_BASE + NUM_RX_FRAG*8)
//#define TX_STAT_BASE        (TX_DESC_BASE + NUM_TX_FRAG*8)
//#define RX_BUF_BASE         (TX_STAT_BASE + NUM_TX_FRAG*4)
//#define TX_BUF_BASE         (RX_BUF_BASE  + NUM_RX_FRAG*ETH_FRAG_SIZE)

// RX and TX descriptor and status definitions.
//#define RX_DESC_PACKET(i)   (*(unsigned int *)(RX_DESC_BASE   + 8*i))
//#define RX_DESC_CTRL(i)     (*(unsigned int *)(RX_DESC_BASE+4 + 8*i))
//#define RX_STAT_INFO(i)     (*(unsigned int *)(RX_STAT_BASE   + 8*i))
//#define RX_STAT_HASHCRC(i)  (*(unsigned int *)(RX_STAT_BASE+4 + 8*i))
//#define TX_DESC_PACKET(i)   (*(unsigned int *)(TX_DESC_BASE   + 8*i))
//#define TX_DESC_CTRL(i)     (*(unsigned int *)(TX_DESC_BASE+4 + 8*i))
//#define TX_STAT_INFO(i)     (*(unsigned int *)(TX_STAT_BASE   + 4*i))
//#define RX_BUF(i)           (RX_BUF_BASE + ETH_FRAG_SIZE*i)
//#define TX_BUF(i)           (TX_BUF_BASE + ETH_FRAG_SIZE*i)

/* MAC Configuration Register 1 */
#define MAC1_REC_EN         0x00000001  /* Receive Enable                    */
#define MAC1_PASS_ALL       0x00000002  /* Pass All Receive Frames           */
#define MAC1_RX_FLOWC       0x00000004  /* RX Flow Control                   */
#define MAC1_TX_FLOWC       0x00000008  /* TX Flow Control                   */
#define MAC1_LOOPB          0x00000010  /* Loop Back Mode                    */
#define MAC1_RES_TX         0x00000100  /* Reset TX Logic                    */
#define MAC1_RES_MCS_TX     0x00000200  /* Reset MAC TX Control Sublayer     */
#define MAC1_RES_RX         0x00000400  /* Reset RX Logic                    */
#define MAC1_RES_MCS_RX     0x00000800  /* Reset MAC RX Control Sublayer     */
#define MAC1_SIM_RES        0x00004000  /* Simulation Reset                  */
#define MAC1_SOFT_RES       0x00008000  /* Soft Reset MAC                    */

/* MAC Configuration Register 2 */
#define MAC2_FULL_DUP       0x00000001  /* Full Duplex Mode                  */
#define MAC2_FRM_LEN_CHK    0x00000002  /* Frame Length Checking             */
#define MAC2_HUGE_FRM_EN    0x00000004  /* Huge Frame Enable                 */
#define MAC2_DLY_CRC        0x00000008  /* Delayed CRC Mode                  */
#define MAC2_CRC_EN         0x00000010  /* Append CRC to every Frame         */
#define MAC2_PAD_EN         0x00000020  /* Pad all Short Frames              */
#define MAC2_VLAN_PAD_EN    0x00000040  /* VLAN Pad Enable                   */
#define MAC2_ADET_PAD_EN    0x00000080  /* Auto Detect Pad Enable            */
#define MAC2_PPREAM_ENF     0x00000100  /* Pure Preamble Enforcement         */
#define MAC2_LPREAM_ENF     0x00000200  /* Long Preamble Enforcement         */
#define MAC2_NO_BACKOFF     0x00001000  /* No Backoff Algorithm              */
#define MAC2_BACK_PRESSURE  0x00002000  /* Backoff Presurre / No Backoff     */
#define MAC2_EXCESS_DEF     0x00004000  /* Excess Defer                      */

/* Back-to-Back Inter-Packet-Gap Register */
#define IPGT_FULL_DUP       0x00000015  /* Recommended value for Full Duplex */
#define IPGT_HALF_DUP       0x00000012  /* Recommended value for Half Duplex */

/* Non Back-to-Back Inter-Packet-Gap Register */
#define IPGR_DEF            0x00000012  /* Recommended value                 */

/* Collision Window/Retry Register */
#define CLRT_DEF            0x0000370F  /* Default value                     */

/* PHY Support Register */
#define SUPP_SPEED          0x00000100  /* Reduced MII Logic Current Speed   */

/* Test Register */
#define TEST_SHCUT_PQUANTA  0x00000001  /* Shortcut Pause Quanta             */
#define TEST_TST_PAUSE      0x00000002  /* Test Pause                        */
#define TEST_TST_BACKP      0x00000004  /* Test Back Pressure                */

/* MII Management Configuration Register */
#define MCFG_SCAN_INC       0x00000001  /* Scan Increment PHY Address        */
#define MCFG_SUPP_PREAM     0x00000002  /* Suppress Preamble                 */
#define MCFG_CLK_SEL        0x0000001C  /* Clock Select Mask                 */
#define MCFG_RES_MII        0x00008000  /* Reset MII Management Hardware     */

#define MCFG_CLK_DIV28      0x0000001C  /* MDC = hclk / 28 */
#define MCFG_CLK_DIV36      0x00000020
#define MCFG_CLK_DIV64      0x0000003c

/* MII Management Command Register */
#define MCMD_READ           0x00000001  /* MII Read                          */
#define MCMD_SCAN           0x00000002  /* MII Scan continuously             */

#define MII_WR_TOUT         0x00050000  /* MII Write timeout count           */
#define MII_RD_TOUT         0x00050000  /* MII Read timeout count            */

/* MII Management Address Register */
#define MADR_REG_ADR        0x0000001F  /* MII Register Address Mask         */
#define MADR_PHY_ADR        0x00001F00  /* PHY Address Mask                  */

/* MII Management Indicators Register */
#define MIND_BUSY           0x00000001  /* MII is Busy                       */
#define MIND_SCAN           0x00000002  /* MII Scanning in Progress          */
#define MIND_NOT_VAL        0x00000004  /* MII Read Data not valid           */
#define MIND_MII_LINK_FAIL  0x00000008  /* MII Link Failed                   */

/* Command Register */
#define CR_RX_EN            0x00000001  /* Enable Receive                    */
#define CR_TX_EN            0x00000002  /* Enable Transmit                   */
#define CR_REG_RES          0x00000008  /* Reset Host Registers              */
#define CR_TX_RES           0x00000010  /* Reset Transmit Datapath           */
#define CR_RX_RES           0x00000020  /* Reset Receive Datapath            */
#define CR_PASS_RUNT_FRM    0x00000040  /* Pass Runt Frames                  */
#define CR_PASS_RX_FILT     0x00000080  /* Pass RX Filter                    */
#define CR_TX_FLOW_CTRL     0x00000100  /* TX Flow Control                   */
#define CR_RMII             0x00000200  /* Reduced MII Interface             */
#define CR_FULL_DUP         0x00000400  /* Full Duplex                       */

/* Status Register */
#define SR_RX_EN            0x00000001  /* Enable Receive                    */
#define SR_TX_EN            0x00000002  /* Enable Transmit                   */

/* Transmit Status Vector 0 Register */
#define TSV0_CRC_ERR        0x00000001  /* CRC error                         */
#define TSV0_LEN_CHKERR     0x00000002  /* Length Check Error                */
#define TSV0_LEN_OUTRNG     0x00000004  /* Length Out of Range               */
#define TSV0_DONE           0x00000008  /* Tramsmission Completed            */
#define TSV0_MCAST          0x00000010  /* Multicast Destination             */
#define TSV0_BCAST          0x00000020  /* Broadcast Destination             */
#define TSV0_PKT_DEFER      0x00000040  /* Packet Deferred                   */
#define TSV0_EXC_DEFER      0x00000080  /* Excessive Packet Deferral         */
#define TSV0_EXC_COLL       0x00000100  /* Excessive Collision               */
#define TSV0_LATE_COLL      0x00000200  /* Late Collision Occured            */
#define TSV0_GIANT          0x00000400  /* Giant Frame                       */
#define TSV0_UNDERRUN       0x00000800  /* Buffer Underrun                   */
#define TSV0_BYTES          0x0FFFF000  /* Total Bytes Transferred           */
#define TSV0_CTRL_FRAME     0x10000000  /* Control Frame                     */
#define TSV0_PAUSE          0x20000000  /* Pause Frame                       */
#define TSV0_BACK_PRESS     0x40000000  /* Backpressure Method Applied       */
#define TSV0_VLAN           0x80000000  /* VLAN Frame                        */

/* Transmit Status Vector 1 Register */
#define TSV1_BYTE_CNT       0x0000FFFF  /* Transmit Byte Count               */
#define TSV1_COLL_CNT       0x000F0000  /* Transmit Collision Count          */

/* Receive Status Vector Register */
#define RSV_BYTE_CNT        0x0000FFFF  /* Receive Byte Count                */
#define RSV_PKT_IGNORED     0x00010000  /* Packet Previously Ignored         */
#define RSV_RXDV_SEEN       0x00020000  /* RXDV Event Previously Seen        */
#define RSV_CARR_SEEN       0x00040000  /* Carrier Event Previously Seen     */
#define RSV_REC_CODEV       0x00080000  /* Receive Code Violation            */
#define RSV_CRC_ERR         0x00100000  /* CRC Error                         */
#define RSV_LEN_CHKERR      0x00200000  /* Length Check Error                */
#define RSV_LEN_OUTRNG      0x00400000  /* Length Out of Range               */
#define RSV_REC_OK          0x00800000  /* Frame Received OK                 */
#define RSV_MCAST           0x01000000  /* Multicast Frame                   */
#define RSV_BCAST           0x02000000  /* Broadcast Frame                   */
#define RSV_DRIB_NIBB       0x04000000  /* Dribble Nibble                    */
#define RSV_CTRL_FRAME      0x08000000  /* Control Frame                     */
#define RSV_PAUSE           0x10000000  /* Pause Frame                       */
#define RSV_UNSUPP_OPC      0x20000000  /* Unsupported Opcode                */
#define RSV_VLAN            0x40000000  /* VLAN Frame                        */

/* Flow Control Counter Register */
#define FCC_MIRR_CNT        0x0000FFFF  /* Mirror Counter                    */
#define FCC_PAUSE_TIM       0xFFFF0000  /* Pause Timer                       */

/* Flow Control Status Register */
#define FCS_MIRR_CNT        0x0000FFFF  /* Mirror Counter Current            */

/* Receive Filter Control Register */
#define RFC_UCAST_EN        0x00000001  /* Accept Unicast Frames Enable      */
#define RFC_BCAST_EN        0x00000002  /* Accept Broadcast Frames Enable    */
#define RFC_MCAST_EN        0x00000004  /* Accept Multicast Frames Enable    */
#define RFC_UCAST_HASH_EN   0x00000008  /* Accept Unicast Hash Filter Frames */
#define RFC_MCAST_HASH_EN   0x00000010  /* Accept Multicast Hash Filter Fram.*/
#define RFC_PERFECT_EN      0x00000020  /* Accept Perfect Match Enable       */
#define RFC_MAGP_WOL_EN     0x00001000  /* Magic Packet Filter WoL Enable    */
#define RFC_PFILT_WOL_EN    0x00002000  /* Perfect Filter WoL Enable         */

/* Receive Filter WoL Status/Clear Registers */
#define WOL_UCAST           0x00000001  /* Unicast Frame caused WoL          */
#define WOL_BCAST           0x00000002  /* Broadcast Frame caused WoL        */
#define WOL_MCAST           0x00000004  /* Multicast Frame caused WoL        */
#define WOL_UCAST_HASH      0x00000008  /* Unicast Hash Filter Frame WoL     */
#define WOL_MCAST_HASH      0x00000010  /* Multicast Hash Filter Frame WoL   */
#define WOL_PERFECT         0x00000020  /* Perfect Filter WoL                */
#define WOL_RX_FILTER       0x00000080  /* RX Filter caused WoL              */
#define WOL_MAG_PACKET      0x00000100  /* Magic Packet Filter caused WoL    */

/* Interrupt Status/Enable/Clear/Set Registers */
#define INT_RX_OVERRUN      0x00000001  /* Overrun Error in RX Queue         */
#define INT_RX_ERR          0x00000002  /* Receive Error                     */
#define INT_RX_FIN          0x00000004  /* RX Finished Process Descriptors   */
#define INT_RX_DONE         0x00000008  /* Receive Done                      */
#define INT_TX_UNDERRUN     0x00000010  /* Transmit Underrun                 */
#define INT_TX_ERR          0x00000020  /* Transmit Error                    */
#define INT_TX_FIN          0x00000040  /* TX Finished Process Descriptors   */
#define INT_TX_DONE         0x00000080  /* Transmit Done                     */
#define INT_SOFT_INT        0x00001000  /* Software Triggered Interrupt      */
#define INT_WAKEUP          0x00002000  /* Wakeup Event Interrupt            */

/* Power Down Register */
#define PD_POWER_DOWN       0x80000000  /* Power Down MAC                    */

/* RX Descriptor Control Word */
#define RCTRL_SIZE          0x000007FF  /* Buffer size mask                  */
#define RCTRL_INT           0x80000000  /* Generate RxDone Interrupt         */

/* RX Status Hash CRC Word */
#define RHASH_SA            0x000001FF  /* Hash CRC for Source Address       */
#define RHASH_DA            0x001FF000  /* Hash CRC for Destination Address  */

/* RX Status Information Word */
#define RINFO_SIZE          0x000007FF  /* Data size in bytes                */
#define RINFO_CTRL_FRAME    0x00040000  /* Control Frame                     */
#define RINFO_VLAN          0x00080000  /* VLAN Frame                        */
#define RINFO_FAIL_FILT     0x00100000  /* RX Filter Failed                  */
#define RINFO_MCAST         0x00200000  /* Multicast Frame                   */
#define RINFO_BCAST         0x00400000  /* Broadcast Frame                   */
#define RINFO_CRC_ERR       0x00800000  /* CRC Error in Frame                */
#define RINFO_SYM_ERR       0x01000000  /* Symbol Error from PHY             */
#define RINFO_LEN_ERR       0x02000000  /* Length Error                      */
#define RINFO_RANGE_ERR     0x04000000  /* Range Error (exceeded max. size)  */
#define RINFO_ALIGN_ERR     0x08000000  /* Alignment Error                   */
#define RINFO_OVERRUN       0x10000000  /* Receive overrun                   */
#define RINFO_NO_DESCR      0x20000000  /* No new Descriptor available       */
#define RINFO_LAST_FLAG     0x40000000  /* Last Fragment in Frame            */
#define RINFO_ERR           0x80000000  /* Error Occured (OR of all errors)  */

#define RINFO_ERR_MASK     (RINFO_FAIL_FILT | RINFO_CRC_ERR   | RINFO_SYM_ERR | \
                            RINFO_LEN_ERR   | RINFO_ALIGN_ERR | RINFO_OVERRUN)

/* TX Descriptor Control Word */
#define TCTRL_SIZE          0x000007FF  /* Size of data buffer in bytes      */
#define TCTRL_OVERRIDE      0x04000000  /* Override Default MAC Registers    */
#define TCTRL_HUGE          0x08000000  /* Enable Huge Frame                 */
#define TCTRL_PAD           0x10000000  /* Pad short Frames to 64 bytes      */
#define TCTRL_CRC           0x20000000  /* Append a hardware CRC to Frame    */
#define TCTRL_LAST          0x40000000  /* Last Descriptor for TX Frame      */
#define TCTRL_INT           0x80000000  /* Generate TxDone Interrupt         */

/* TX Status Information Word */
#define TINFO_COL_CNT       0x01E00000  /* Collision Count                   */
#define TINFO_DEFER         0x02000000  /* Packet Deferred (not an error)    */
#define TINFO_EXCESS_DEF    0x04000000  /* Excessive Deferral                */
#define TINFO_EXCESS_COL    0x08000000  /* Excessive Collision               */
#define TINFO_LATE_COL      0x10000000  /* Late Collision Occured            */
#define TINFO_UNDERRUN      0x20000000  /* Transmit Underrun                 */
#define TINFO_NO_DESCR      0x40000000  /* No new Descriptor available       */
#define TINFO_ERR           0x80000000  /* Error Occured (OR of all errors)  */

/* ENET Device Revision ID */
#define OLD_EMAC_MODULE_ID  0x39022000  /* Rev. ID for first rev '-'         */

/* DP83848C PHY Registers */
#define PHY_REG_BMCR        0x00        /* Basic Mode Control Register       */
#define PHY_REG_BMSR        0x01        /* Basic Mode Status Register        */
#define PHY_REG_IDR1        0x02        /* PHY Identifier 1                  */
#define PHY_REG_IDR2        0x03        /* PHY Identifier 2                  */
#define PHY_REG_ANAR        0x04        /* Auto-Negotiation Advertisement    */
#define PHY_REG_ANLPAR      0x05        /* Auto-Neg. Link Partner Abitily    */
#define PHY_REG_ANER        0x06        /* Auto-Neg. Expansion Register      */
#define PHY_REG_ANNPTR      0x07        /* Auto-Neg. Next Page TX            */

/* PHY Extended Registers */
#define PHY_REG_STS         0x10        /* Status Register                   */
#define PHY_REG_MICR        0x11        /* MII Interrupt Control Register    */
#define PHY_REG_MISR        0x12        /* MII Interrupt Status Register     */
#define PHY_REG_FCSCR       0x14        /* False Carrier Sense Counter       */
#define PHY_REG_RECR        0x15        /* Receive Error Counter             */
#define PHY_REG_PCSR        0x16        /* PCS Sublayer Config. and Status   */
#define PHY_REG_RBR         0x17        /* RMII and Bypass Register          */
#define PHY_REG_LEDCR       0x18        /* LED Direct Control Register       */
#define PHY_REG_PHYCR       0x19        /* PHY Control Register              */
#define PHY_REG_10BTSCR     0x1A        /* 10Base-T Status/Control Register  */
#define PHY_REG_CDCTRL1     0x1B        /* CD Test Control and BIST Extens.  */
#define PHY_REG_EDCR        0x1D        /* Energy Detect Control Register    */

#define PHY_FULLD_100M      0x2100      /* Full Duplex 100Mbit               */
#define PHY_HALFD_100M      0x2000      /* Half Duplex 100Mbit               */
#define PHY_FULLD_10M       0x0100      /* Full Duplex 10Mbit                */
#define PHY_HALFD_10M       0x0000      /* Half Duplex 10MBit                */
#define PHY_AUTO_NEG        0x3000      /* Select Auto Negotiation           */

#define DP83848C_DEF_ADR    0x0100      /* Default PHY device address        */
#define DP83848C_ID         0x20005C90  /* PHY Identifier                    */

#define LAN8720_ID          0x0007C0F0  /* PHY Identifier                    */

#ifdef CBUG
#define USE_ETHERNET_DEBUG
#endif // CBUG

static unsigned short *rxptr;
static unsigned short *txptr;

//----------------------------------------------------------------------------
// SwapBytes

static unsigned short SwapBytes(unsigned short Data)
{
    return( (Data >> 8) | (Data << 8) ) ;
}

//----------------------------------------------------------------------------
// write to external ethernet PHY chip

void WriteToPHY(int reg, int writeval)
{
    unsigned int loop ;

    // Set up address to access in MII Mgmt Address Register
    LPC_EMAC->MADR = DP83848C_DEF_ADR | reg;
    // Write value into MII Mgmt Write Data Register
    LPC_EMAC->MWTD = writeval;
    // Loop whilst write to PHY completes
    for (loop = 0; loop < MII_WR_TOUT ; loop++) {
        if ((LPC_EMAC->MIND & MIND_BUSY) == 0)
            break ;
    }
}

//----------------------------------------------------------------------------
// read from external ethernet PHY chip

unsigned short ReadFromPHY(unsigned char reg)
{
    unsigned int loop ;

    // Set up address to access in MII Mgmt Address Register
    LPC_EMAC->MADR = DP83848C_DEF_ADR | reg;
    // Trigger a PHY read via MII Mgmt Command Register
    LPC_EMAC->MCMD = MCMD_READ;
    // Loop whilst read from PHY completes
    for (loop = 0; loop < MII_RD_TOUT; loop++) {
        if ((LPC_EMAC->MIND & MIND_BUSY) == 0)
            break ;
    }
    LPC_EMAC->MCMD = 0; // Cancel read
    // Returned value is in MII Mgmt Read Data Register
    return(LPC_EMAC->MRDD) ;
}

//----------------------------------------------------------------------------
// ethernetstart

void ethernetstart(void)
{
    unsigned int value = 0, phyid1, phyid2;
    volatile unsigned int loop;
    unsigned phy_in_use = 0;
    unsigned phy_linkstatus_reg;
    unsigned phy_linkstatus_mask;

    // Power Up the EMAC controller.
    LPC_SC->PCONP |= CLKPWR_PCONP_PCENET ;

    // Set up MAC Configuration Register 1
    LPC_EMAC->MAC1 = MAC1_RES_TX | MAC1_RES_MCS_TX | MAC1_RES_RX |
                     MAC1_RES_MCS_RX |MAC1_SIM_RES | MAC1_SOFT_RES ;

    // Set up MAC Command Register
    LPC_EMAC->Command = CR_REG_RES | CR_TX_RES | CR_RX_RES | CR_PASS_RUNT_FRM ;

    // Short delay
    //for (loop = 100; loop; loop--);
    tickwait(25) ;

    // Set up MAC Configuration Register 1 to pass all receive frames
    LPC_EMAC->MAC1 = MAC1_PASS_ALL ;
    // Set up MAC Configuration Register 2 to append CRC and pad out frames
    LPC_EMAC->MAC2 = MAC2_CRC_EN | MAC2_PAD_EN ;

    // Set Ethernet Maximum Frame Register
    LPC_EMAC->MAXF = ETH_MAX_FLEN ;
    // Set Collision Window / Retry Register
    LPC_EMAC->CLRT = CLRT_DEF ;
    // Set Non Back-to-Back Inter-Packet-Gap Register
    LPC_EMAC->IPGR = IPGR_DEF ;

    // Enable Reduced MII interface.
    LPC_EMAC->MCFG = MCFG_CLK_DIV64 | MCFG_RES_MII ;

    // for (loop = 100; loop; loop--);
    tickwait(25) ;

    LPC_EMAC->MCFG = MCFG_CLK_DIV64;

    // Set MAC Command Register to enable Reduced MII interface
    // and prevent runt frames being filtered out
    LPC_EMAC->Command = CR_RMII | CR_PASS_RUNT_FRM | CR_PASS_RX_FILT;

    // Put PHY into reset mode
    WriteToPHY(PHY_REG_BMCR, 0x8000) ;

    // Loop until hardware reset completes
    for (loop = 0; loop < 0x100000; loop++) {
        value = ReadFromPHY (PHY_REG_BMCR);
        if (!(value & 0x8000)) {
            // Reset has completed
            break;
        }
    }

    // Just check this actually is a DP83848C PHY
    phyid1 = ReadFromPHY(PHY_REG_IDR1);
    phyid2 = ReadFromPHY(PHY_REG_IDR2);

    if (((phyid1 << 16) | (phyid2 & 0xFFF0)) == DP83848C_ID) {
        phy_in_use =  DP83848C_ID;
#ifdef USE_ETHERNET_DEBUG
        printf("ETH: phy_in_use DP83848C\n") ;
#endif // USE_ETHERNET_DEBUG
    } else if (((phyid1 << 16) | (phyid2 & 0xFFF0)) == LAN8720_ID) {
        phy_in_use = LAN8720_ID;
#ifdef USE_ETHERNET_DEBUG
        printf("ETH: phy_in_use LAN8720\n") ;
#endif // USE_ETHERNET_DEBUG
    } else {
#ifdef USE_ETHERNET_DEBUG
        printf("ETH: phy_in_use UNKNOWN\n") ;
#endif // USE_ETHERNET_DEBUG
    }

    if (phy_in_use != 0) {
        // Safe to configure the PHY device

        // Set PHY to autonegotiation link speed
        WriteToPHY (PHY_REG_BMCR, PHY_AUTO_NEG);
        // loop until autonegotiation completes
        for (loop = 0; loop < 0x100000; loop++) {
            value = ReadFromPHY (PHY_REG_BMSR);
            if (value & 0x0020) {
                // Autonegotiation has completed
                break;
            }
        }
    }

    phy_linkstatus_reg = PHY_REG_STS;		// Default to DP83848C
    phy_linkstatus_mask = 0x0001;

    if (phy_in_use == LAN8720_ID) {
        phy_linkstatus_reg = PHY_REG_BMSR;
        phy_linkstatus_mask = 0x0004;
    }

    // Now check the link status
#ifdef USE_ETHERNET_DEBUG
    printf("ETH: check link status...") ;
#endif // USE_ETHERNET_DEBUG
    for (loop = 0 ; loop < 0x10000 ; loop++) {
        value = ReadFromPHY(phy_linkstatus_reg);
        if (value & phy_linkstatus_mask) {
            // The link is on
#ifdef USE_ETHERNET_DEBUG
            printf(" link is on") ;
#endif // USE_ETHERNET_DEBUG

            // Now configure for full/half duplex mode
            if (value & 0x0004) {
                // We are in full duplex is enabled mode
#ifdef USE_ETHERNET_DEBUG
                printf(" full duplex") ;
#endif // USE_ETHERNET_DEBUG
                LPC_EMAC->MAC2    |= MAC2_FULL_DUP;
                LPC_EMAC->Command |= CR_FULL_DUP;
                LPC_EMAC->IPGT     = IPGT_FULL_DUP;
            } else {
                // Otherwise we are in half duplex mode
#ifdef USE_ETHERNET_DEBUG
                printf(" half duplex") ;
#endif // USE_ETHERNET_DEBUG
                LPC_EMAC->IPGT = IPGT_HALF_DUP;
            }

            // Now configure 100MBit or 10MBit mode
            if (value & 0x0002) {
                // 10MBit mode
#ifdef USE_ETHERNET_DEBUG
                printf(" at 10MHz") ;
#endif // USE_ETHERNET_DEBUG
                LPC_EMAC->SUPP = 0;
            } else {
                // 100MBit mode
#ifdef USE_ETHERNET_DEBUG
                printf(" at 100MHz") ;
#endif // USE_ETHERNET_DEBUG
                LPC_EMAC->SUPP = SUPP_SPEED;
            }
#ifdef USE_ETHERNET_DEBUG
            printf("\n") ;
#endif // USE_ETHERNET_DEBUG
            break;
        }
    }
    if (loop >= 0x10000) {
#ifdef USE_ETHERNET_DEBUG
        printf(" NO link\n") ;
#endif // USE_ETHERNET_DEBUG
    }

    // Now set the Ethernet MAC Address registers
    // NOTE - MAC address must be unique on the network!
    LPC_EMAC->SA0 = (MYMAC_1 << 8) | MYMAC_2; // Station address 0 Reg
    LPC_EMAC->SA1 = (MYMAC_3 << 8) | MYMAC_4; // Station address 1 Reg
    LPC_EMAC->SA2 = (MYMAC_5 << 8) | MYMAC_6; // Station address 2 Reg

    // Now initialise the Rx descriptors
    for (loop = 0; loop < NUM_RX_FRAG; loop++) {
        RX_DESC_PACKET(loop)  = RX_BUF(loop);
        RX_DESC_CTRL(loop)    = RCTRL_INT | (ETH_FRAG_SIZE-1);
        RX_STAT_INFO(loop)    = 0;
        RX_STAT_HASHCRC(loop) = 0;
    }

    // Set up the Receive Descriptor Base address register
    LPC_EMAC->RxDescriptor    = RX_DESC_BASE;
    // Set up the Receive Status Base address register
    LPC_EMAC->RxStatus        = RX_STAT_BASE;
    // Setup the Receive Number of Descriptor register
    LPC_EMAC->RxDescriptorNumber = NUM_RX_FRAG-1;
    //  Set Receive Consume Index register to 0
    LPC_EMAC->RxConsumeIndex  = 0;

    // Now initialise the Tx descriptors
    for (loop = 0; loop < NUM_TX_FRAG; loop++) {
        TX_DESC_PACKET(loop) = TX_BUF(loop);
        TX_DESC_CTRL(loop)   = 0;
        TX_STAT_INFO(loop)   = 0;
    }

    // Set up the Transmit Descriptor Base address register
    LPC_EMAC->TxDescriptor    = TX_DESC_BASE;
    // Set up the Transmit Status Base address register
    LPC_EMAC->TxStatus        = TX_STAT_BASE;
    // Setup the Transmit Number of Descriptor register
    LPC_EMAC->TxDescriptorNumber = NUM_TX_FRAG-1;
    //  Set Transmit Consume Index register to 0
    LPC_EMAC->TxProduceIndex  = 0;

    // Receive Broadcast and Perfect Match Packets
    LPC_EMAC->RxFilterCtrl = RFC_BCAST_EN | RFC_PERFECT_EN;

    // Enable interrupts MAC Module Control Interrupt Enable Register
    LPC_EMAC->IntEnable = INT_RX_DONE | INT_TX_DONE;

    // Reset all ethernet interrupts in MAC module
    LPC_EMAC->IntClear  = 0xFFFF;

    // Finally enable receive and transmit mode in ethernet core
    LPC_EMAC->Command  |= (CR_RX_EN | CR_TX_EN);
    LPC_EMAC->MAC1     |= MAC1_REC_EN;
}

//----------------------------------------------------------------------------
// ethernetstop

void ethernetstop(void)
{
    // Power down the EMAC controller.
    LPC_SC->PCONP &= ~CLKPWR_PCONP_PCENET ;
}

//----------------------------------------------------------------------------
// writes a word in little-endian byte order to TX_BUFFER

void WriteFrame_EthMAC(unsigned short Data)
{
    *txptr++ = Data;
}

//----------------------------------------------------------------------------

void eth_CopyToFrame_EthMAC(void *Source, unsigned int Size)
{
    unsigned int index;
    unsigned short *pSource;

    pSource = Source;
    Size = (Size + 1) & 0xFFFE;    // round up Size to next even number
    while (Size > 0) {
        WriteFrame_EthMAC(*pSource++);
        Size -= 2;
    }

    index = LPC_EMAC->TxProduceIndex;
    if (++index == NUM_TX_FRAG)
        index = 0;
    LPC_EMAC->TxProduceIndex = index;
}


//----------------------------------------------------------------------------
// reads a word in little-endian byte order from RX_BUFFER

unsigned short eth_ReadFrame_EthMAC(void)
{
    return (*rxptr++);
}

//----------------------------------------------------------------------------

unsigned short eth_ReadFrameBE_EthMAC(void)
{
    unsigned short ReturnValue;

    ReturnValue = SwapBytes(*rxptr++) ;
    return(ReturnValue);
}

//----------------------------------------------------------------------------

void eth_CopyFromFrame_EthMAC(void *Dest, unsigned short Size)
{
    unsigned short *pDest;

    pDest = Dest;
    while (Size > 1) {
        *pDest++ = eth_ReadFrame_EthMAC();
        Size -= 2;
    }

    if (Size) {                                         // check for leftover byte...
        *(unsigned char *)pDest = (char)eth_ReadFrame_EthMAC();// the LAN-Controller will return 0
    }                                                   // for the highbyte
}

//----------------------------------------------------------------------------
// does a dummy read on frame-I/O-port
// NOTE: only an even number of bytes is read!

void eth_DummyReadFrame_EthMAC(unsigned short Size) // discards an EVEN number of bytes
{                                                   // from RX-fifo
    while (Size > 1) {
        eth_ReadFrame_EthMAC();
        Size -= 2;
    }
}

//----------------------------------------------------------------------------

void eth_RequestSend(unsigned short FrameSize)
{
    unsigned int index;
    index  = LPC_EMAC->TxProduceIndex;
    txptr = (unsigned short *)TX_DESC_PACKET(index);
    TX_DESC_CTRL(index) = FrameSize | TCTRL_LAST;
}

//----------------------------------------------------------------------------
// check if ready to accept the
// frame we want to send

//unsigned int Rdy4Tx(void)
//{
//    // One the LPC the ethernet controller transmits
//    // much faster than the CPU can load its buffers
//    // so will always be ready to accept frame
//    return (1);
//}

//----------------------------------------------------------------------------
// Reads  length of received ethernet frame and checks
// if destination address is a broadcast message or not.
// Then returns the frame length

unsigned short eth_StartReadingFrame(void)
{
    unsigned short ReceiveLength;
    unsigned int index;

    index = LPC_EMAC->RxConsumeIndex;
    ReceiveLength = (RX_STAT_INFO(index) & RINFO_SIZE) - 3;
    rxptr = (unsigned short *)RX_DESC_PACKET(index);
    return(ReceiveLength);
}

//----------------------------------------------------------------------------

void eth_StopReadingFrame(void)
{
    unsigned int index;
    index = LPC_EMAC->RxConsumeIndex;
    if (++index == NUM_RX_FRAG)
        index = 0;
    LPC_EMAC->RxConsumeIndex = index;
}

//----------------------------------------------------------------------------
// check if frame has been received

int eth_CheckIfFrameReceived(void)
{
    if (LPC_EMAC->RxProduceIndex != LPC_EMAC->RxConsumeIndex)
        return(1);  // Yes, packet received
    else
        return(0);
}
#endif // USE_ETHERNET
