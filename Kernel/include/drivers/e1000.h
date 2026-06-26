#pragma once

#include <typedefs.h>

// --- General Control & Status ---
#define E1000_CTRL      0x0000    // Device Control Register
#define E1000_STATUS    0x0008    // Device Status Register
#define E1000_EEPROM_PT 0x0014    // EEPROM Read Register
#define E1000_CTRL_EXT  0x0018    // Extended Device Control

// --- Interrupts ---
#define E1000_ICR       0x00C0    // Interrupt Cause Read (Reads and clears interrupts)
#define E1000_ICS       0x00C8    // Interrupt Cause Set
#define E1000_IMS       0x00D0    // Interrupt Mask Set/Read (Enables interrupts)
#define E1000_IMC       0x00D8    // Interrupt Mask Clear (Disables interrupts)

// --- Interrupt Mask Register (IMS) ---
#define IMS_RXT0                        (1 << 7)    // Fire interrupt on packet recieving
#define IMS_LSC                         (1 << 2)    // Fire interrupt when ethernet cable is plugged in or out
#define IMS_TXDW                        (1 << 0)     // Fire interrupt when packet is pushed to wire

// --- Receive (Rx) Registers ---
#define E1000_RCTL      0x0100    // Receive Control
#define E1000_RDBAL     0x2800    // Rx Descriptor Base Address Low
#define E1000_RDBAH     0x2804    // Rx Descriptor Base Address High
#define E1000_RDLEN     0x2808    // Rx Descriptor Length (Size in bytes)
#define E1000_RDH       0x2810    // Rx Descriptor Head
#define E1000_RDT       0x2818    // Rx Descriptor Tail
#define E1000_RAL0      0x5400    // Receive Address Low (MAC Address)
#define E1000_RAH0      0x5404    // Receive Address High (MAC Address + AV bit)
#define E1000_MTA       0x5200    // Multicast Table Array (128 entries)

// --- Recieve Control (RCTL) ---
#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC

// Buffer Sizes (RCTL)
#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))

// --- Transmit (Tx) Registers ---
#define E1000_TCTL      0x0400    // Transmit Control
#define E1000_TDBAL     0x3800    // Tx Descriptor Base Address Low
#define E1000_TDBAH     0x3804    // Tx Descriptor Base Address High
#define E1000_TDLEN     0x3808    // Tx Descriptor Length (Size in bytes)
#define E1000_TDH       0x3810    // Tx Descriptor Head
#define E1000_TDT       0x3818    // Tx Descriptor Tail

// Transmit Command
#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable

// TCTL Register
#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   (0x0F << 4) // Collision Threshold
#define TCTL_COLD_SHIFT                 (0x40 << 12)// Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision

#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3)    // Transmit Underrun

typedef struct {
    uint64_t addr;      // Physical Address of the 2KB Rx buffer landing zone
    uint16_t length;    // Hardware writes the length of the received packet here
    uint16_t checksum;  // Hardware calculates the packet checksum for you
    uint8_t  status;    // Status flags (Did we get a packet?)
    uint8_t  errors;    // Error flags (Did the packet collide or drop?)
    uint16_t special;   // VLAN Tags
} __attribute__((packed)) e1000_rx_desc ;

typedef struct {
    uint64_t addr;      // Physical Address of the packet data in your RAM
    uint16_t length;    // How many bytes you want the hardware to send
    uint8_t  cso;       // Checksum Offset
    uint8_t  cmd;       // Command Field (What should the hardware do?)
    uint8_t  status;    // Status Field (Did it finish sending?)
    uint8_t  css;       // Checksum Start
    uint16_t special;   // VLAN Tags
} __attribute__((packed)) e1000_tx_desc ;

typedef struct {
    uint8_t* mmio_base;
    e1000_rx_desc* rx_ring;
    uint16_t rx_tail;
    e1000_tx_desc* tx_ring;
    uint16_t tx_tail;
} e1000_device_t;