/***************************************************************************
 *   Copyright (C) 2009 by Duane Ellis                                     *
 *   openocd@duaneellis.com                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS for A PARTICULAR PURPOSE.  See the         *
 *   GNU General public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
****************************************************************************/

/* Some of the the lower level code was based on code supplied by
 * ATMEL under this copyright. */

/* BEGIN ATMEL COPYRIGHT */
/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2009, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */
/* END ATMEL COPYRIGHT */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "imp.h"
#include "at91sam3.h"
#include <helper/membuf.h>
#include <helper/time_support.h>

#define REG_NAME_WIDTH  (12)

// _BM_ begin ---------------------------------------------------

// at91sam3u series (has one or two flash banks)
#define FLASH_BANK0_BASE_U   0x00080000
#define FLASH_BANK1_BASE_U   0x00100000
 
// at91sam3s series (has always one flash bank)
#define FLASH_BANK_BASE_S   0x00400000

//#define FLASH_BANK0_BASE   0x00080000
//#define FLASH_BANK1_BASE   0x00100000

// _BM_ end -----------------------------------------------------

#define 	AT91C_EFC_FCMD_GETD                 (0x0) // (EFC) Get Flash Descriptor
#define 	AT91C_EFC_FCMD_WP                   (0x1) // (EFC) Write Page
#define 	AT91C_EFC_FCMD_WPL                  (0x2) // (EFC) Write Page and Lock
#define 	AT91C_EFC_FCMD_EWP                  (0x3) // (EFC) Erase Page and Write Page
#define 	AT91C_EFC_FCMD_EWPL                 (0x4) // (EFC) Erase Page and Write Page then Lock
#define 	AT91C_EFC_FCMD_EA                   (0x5) // (EFC) Erase All
// cmd6 is not present int he at91sam3u4/2/1 data sheet table 17-2
// #define 	AT91C_EFC_FCMD_EPL                  (0x6) // (EFC) Erase plane?
// cmd7 is not present int he at91sam3u4/2/1 data sheet table 17-2
// #define 	AT91C_EFC_FCMD_EPA                  (0x7) // (EFC) Erase pages?
#define 	AT91C_EFC_FCMD_SLB                  (0x8) // (EFC) Set Lock Bit
#define 	AT91C_EFC_FCMD_CLB                  (0x9) // (EFC) Clear Lock Bit
#define 	AT91C_EFC_FCMD_GLB                  (0xA) // (EFC) Get Lock Bit
#define 	AT91C_EFC_FCMD_SFB                  (0xB) // (EFC) Set Fuse Bit
#define 	AT91C_EFC_FCMD_CFB                  (0xC) // (EFC) Clear Fuse Bit
#define 	AT91C_EFC_FCMD_GFB                  (0xD) // (EFC) Get Fuse Bit
#define 	AT91C_EFC_FCMD_STUI                 (0xE) // (EFC) Start Read Unique ID
#define 	AT91C_EFC_FCMD_SPUI                 (0xF) // (EFC) Stop Read Unique ID

#define  offset_EFC_FMR   0
#define  offset_EFC_FCR   4
#define  offset_EFC_FSR   8
#define  offset_EFC_FRR   12


static float
_tomhz(uint32_t freq_hz)
{
	float f;

	f = ((float)(freq_hz)) / 1000000.0;
	return f;
}

// How the chip is configured.
struct sam3_cfg {
	uint32_t unique_id[4];

	uint32_t slow_freq;
	uint32_t rc_freq;
	uint32_t mainosc_freq;
	uint32_t plla_freq;
	uint32_t mclk_freq;
	uint32_t cpu_freq;
	uint32_t fclk_freq;
	uint32_t pclk0_freq;
	uint32_t pclk1_freq;
	uint32_t pclk2_freq;


#define SAM3_CHIPID_CIDR          (0x400E0740)
	uint32_t CHIPID_CIDR;
#define SAM3_CHIPID_EXID          (0x400E0744)
	uint32_t CHIPID_EXID;

#define SAM3_SUPC_CR              (0x400E1210)
	uint32_t SUPC_CR;

#define SAM3_PMC_BASE             (0x400E0400)
#define SAM3_PMC_SCSR             (SAM3_PMC_BASE + 0x0008)
	uint32_t PMC_SCSR;
#define SAM3_PMC_PCSR             (SAM3_PMC_BASE + 0x0018)
	uint32_t PMC_PCSR;
#define SAM3_CKGR_UCKR            (SAM3_PMC_BASE + 0x001c)
	uint32_t CKGR_UCKR;
#define SAM3_CKGR_MOR             (SAM3_PMC_BASE + 0x0020)
	uint32_t CKGR_MOR;
#define SAM3_CKGR_MCFR            (SAM3_PMC_BASE + 0x0024)
	uint32_t CKGR_MCFR;
#define SAM3_CKGR_PLLAR           (SAM3_PMC_BASE + 0x0028)
	uint32_t CKGR_PLLAR;
#define SAM3_PMC_MCKR             (SAM3_PMC_BASE + 0x0030)
	uint32_t PMC_MCKR;
#define SAM3_PMC_PCK0             (SAM3_PMC_BASE + 0x0040)
	uint32_t PMC_PCK0;
#define SAM3_PMC_PCK1             (SAM3_PMC_BASE + 0x0044)
	uint32_t PMC_PCK1;
#define SAM3_PMC_PCK2             (SAM3_PMC_BASE + 0x0048)
	uint32_t PMC_PCK2;
#define SAM3_PMC_SR               (SAM3_PMC_BASE + 0x0068)
	uint32_t PMC_SR;
#define SAM3_PMC_IMR              (SAM3_PMC_BASE + 0x006c)
	uint32_t PMC_IMR;
#define SAM3_PMC_FSMR             (SAM3_PMC_BASE + 0x0070)
	uint32_t PMC_FSMR;
#define SAM3_PMC_FSPR             (SAM3_PMC_BASE + 0x0074)
	uint32_t PMC_FSPR;
};


struct sam3_bank_private {
	int probed;
	// DANGER: THERE ARE DRAGONS HERE..
	// NOTE: If you add more 'ghost' pointers
	// be aware that you must *manually* update
	// these pointers in the function sam3_GetDetails()
	// See the comment "Here there be dragons"

	// so we can find the chip we belong to
	struct sam3_chip *pChip;
	// so we can find the orginal bank pointer
	struct flash_bank *pBank;
	unsigned bank_number;
	uint32_t controller_address;
	uint32_t base_address;
	bool present;
	unsigned size_bytes;
	unsigned nsectors;
	unsigned sector_size;
	unsigned page_size;
};

struct sam3_chip_details {
	// THERE ARE DRAGONS HERE..
	// note: If you add pointers here
	// becareful about them as they
	// may need to be updated inside
	// the function: "sam3_GetDetails()
	// which copy/overwrites the
	// 'runtime' copy of this structure
	uint32_t chipid_cidr;
	const char *name;

	unsigned n_gpnvms;
#define SAM3_N_NVM_BITS 3
	unsigned  gpnvm[SAM3_N_NVM_BITS];
	unsigned  total_flash_size;
	unsigned  total_sram_size;
	unsigned  n_banks;
#define SAM3_MAX_FLASH_BANKS 2
	// these are "initialized" from the global const data
	struct sam3_bank_private bank[SAM3_MAX_FLASH_BANKS];
};


struct sam3_chip {
	struct sam3_chip *next;
	int    probed;

	// this is "initialized" from the global const structure
	struct sam3_chip_details details;
	struct target *target;
	struct sam3_cfg cfg;

	struct membuf *mbuf;
};


struct sam3_reg_list {
	uint32_t address;  size_t struct_offset; const char *name;
	void (*explain_func)(struct sam3_chip *pInfo);
};


static struct sam3_chip *all_sam3_chips;

static struct sam3_chip *
get_current_sam3(struct command_context *cmd_ctx)
{
	struct target *t;
	static struct sam3_chip *p;

	t = get_current_target(cmd_ctx);
	if (!t) {
		command_print(cmd_ctx, "No current target?");
		return NULL;
	}

	p = all_sam3_chips;
	if (!p) {
		// this should not happen
		// the command is not registered until the chip is created?
		command_print(cmd_ctx, "No SAM3 chips exist?");
		return NULL;
	}

	while (p) {
		if (p->target == t) {
			return p;
		}
		p = p->next;
	}
	command_print(cmd_ctx, "Cannot find SAM3 chip?");
	return NULL;
}


// these are used to *initialize* the "pChip->details" structure.
static const struct sam3_chip_details all_sam3_details[] = {
    // Start at91sam3u* series
	{
		.chipid_cidr    = 0x28100960,
		.name           = "at91sam3u4e",
		.total_flash_size     = 256 * 1024,
		.total_sram_size      = 52 * 1024,
		.n_gpnvms       = 3,
		.n_banks        = 2,

		// System boots at address 0x0
		// gpnvm[1] = selects boot code
		//     if gpnvm[1] == 0
		//         boot is via "SAMBA" (rom)
		//     else
		//         boot is via FLASH
		//         Selection is via gpnvm[2]
		//     endif
		//
		// NOTE: banks 0 & 1 switch places
		//     if gpnvm[2] == 0
		//         Bank0 is the boot rom
		//      else
		//         Bank1 is the boot rom
		//      endif
//		.bank[0] = {
		{
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK0_BASE_U,
			.controller_address = 0x400e0800,
			.present = 1,
			.size_bytes = 128 * 1024,
			.nsectors   = 16,
			.sector_size = 8192,
			.page_size   = 256,
		  },

//		.bank[1] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 1,
			.base_address = FLASH_BANK1_BASE_U,
			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes = 128 * 1024,
			.nsectors   = 16,
			.sector_size = 8192,
			.page_size   = 256,
		  },
		},
	},

	{
		.chipid_cidr    = 0x281a0760,
		.name           = "at91sam3u2e",
		.total_flash_size     = 128 * 1024,
		.total_sram_size      =  36 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,

		// System boots at address 0x0
		// gpnvm[1] = selects boot code
		//     if gpnvm[1] == 0
		//         boot is via "SAMBA" (rom)
		//     else
		//         boot is via FLASH
		//         Selection is via gpnvm[2]
		//     endif
//		.bank[0] = {
		{
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK0_BASE_U,
			.controller_address = 0x400e0800,
			.present = 1,
			.size_bytes = 128 * 1024,
			.nsectors   = 16,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		  .bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,
		  },
		},
	},
	{
		.chipid_cidr    = 0x28190560,
		.name           = "at91sam3u1e",
		.total_flash_size     = 64 * 1024,
		.total_sram_size      = 20 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,

		// System boots at address 0x0
		// gpnvm[1] = selects boot code
		//     if gpnvm[1] == 0
		//         boot is via "SAMBA" (rom)
		//     else
		//         boot is via FLASH
		//         Selection is via gpnvm[2]
		//     endif
		//

//		.bank[0] = {
		{
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK0_BASE_U,
			.controller_address = 0x400e0800,
			.present = 1,
			.size_bytes =  64 * 1024,
			.nsectors   =  8,
			.sector_size = 8192,
			.page_size   = 256,
		  },

//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,
		  },
		},
	},

	{
		.chipid_cidr    = 0x28000960,
		.name           = "at91sam3u4c",
		.total_flash_size     = 256 * 1024,
		.total_sram_size      = 52 * 1024,
		.n_gpnvms       = 3,
		.n_banks        = 2,

		// System boots at address 0x0
		// gpnvm[1] = selects boot code
		//     if gpnvm[1] == 0
		//         boot is via "SAMBA" (rom)
		//     else
		//         boot is via FLASH
		//         Selection is via gpnvm[2]
		//     endif
		//
		// NOTE: banks 0 & 1 switch places
		//     if gpnvm[2] == 0
		//         Bank0 is the boot rom
		//      else
		//         Bank1 is the boot rom
		//      endif
		{
		  {
//		.bank[0] = {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK0_BASE_U,
			.controller_address = 0x400e0800,
			.present = 1,
			.size_bytes = 128 * 1024,
			.nsectors   = 16,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 1,
			.base_address = FLASH_BANK1_BASE_U,
			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes = 128 * 1024,
			.nsectors   = 16,
			.sector_size = 8192,
			.page_size   = 256,
		  },
		},
	},

	{
		.chipid_cidr    = 0x280a0760,
		.name           = "at91sam3u2c",
		.total_flash_size     = 128 * 1024,
		.total_sram_size      = 36 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,

		// System boots at address 0x0
		// gpnvm[1] = selects boot code
		//     if gpnvm[1] == 0
		//         boot is via "SAMBA" (rom)
		//     else
		//         boot is via FLASH
		//         Selection is via gpnvm[2]
		//     endif
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK0_BASE_U,
			.controller_address = 0x400e0800,
			.present = 1,
			.size_bytes = 128 * 1024,
			.nsectors   = 16,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,
		  },
		},
	},
	{
		.chipid_cidr    = 0x28090560,
		.name           = "at91sam3u1c",
		.total_flash_size     = 64 * 1024,
		.total_sram_size      = 20 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,

		// System boots at address 0x0
		// gpnvm[1] = selects boot code
		//     if gpnvm[1] == 0
		//         boot is via "SAMBA" (rom)
		//     else
		//         boot is via FLASH
		//         Selection is via gpnvm[2]
		//     endif
		//

		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK0_BASE_U,
			.controller_address = 0x400e0800,
			.present = 1,
			.size_bytes =  64 * 1024,
			.nsectors   =  8,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},

// _BM_ begin ---------------------------------------------------

	// Start at91sam3s* series

	// Note: The preliminary at91sam3s datasheet says on page 302
	// that the flash controller is at address 0x400E0800.
	// This is _not_ the case, the controller resides at address 0x400e0a0.
	{
		.chipid_cidr    = 0x28A00960,
		.name           = "at91sam3s4c",
		.total_flash_size     = 256 * 1024,
		.total_sram_size      = 48 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  256 * 1024,
			.nsectors   =  32,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},

	{
		.chipid_cidr    = 0x28900960,
		.name           = "at91sam3s4b",
		.total_flash_size     = 256 * 1024,
		.total_sram_size      = 48 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  256 * 1024,
			.nsectors   =  32,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},
	{
		.chipid_cidr    = 0x28800960,
		.name           = "at91sam3s4a",
		.total_flash_size     = 256 * 1024,
		.total_sram_size      = 48 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  256 * 1024,
			.nsectors   =  32,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},
	{
		.chipid_cidr    = 0x28AA0760,
		.name           = "at91sam3s2c",
		.total_flash_size     = 128 * 1024,
		.total_sram_size      = 32 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  128 * 1024,
			.nsectors   =  16,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},
	{
		.chipid_cidr    = 0x289A0760,
		.name           = "at91sam3s2b",
		.total_flash_size     = 128 * 1024,
		.total_sram_size      = 32 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  128 * 1024,
			.nsectors   =  16,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},
	{
		.chipid_cidr    = 0x288A0760,
		.name           = "at91sam3s2a",
		.total_flash_size     = 128 * 1024,
		.total_sram_size      = 32 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  128 * 1024,
			.nsectors   =  16,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},
	{
		.chipid_cidr    = 0x28A90560,
		.name           = "at91sam3s1c",
		.total_flash_size     = 64 * 1024,
		.total_sram_size      = 16 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  64 * 1024,
			.nsectors   =  8,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},
	{
		.chipid_cidr    = 0x28990560,
		.name           = "at91sam3s1b",
		.total_flash_size     = 64 * 1024,
		.total_sram_size      = 16 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  64 * 1024,
			.nsectors   =  8,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},
	{
		.chipid_cidr    = 0x28890560,
		.name           = "at91sam3s1a",
		.total_flash_size     = 64 * 1024,
		.total_sram_size      = 16 * 1024,
		.n_gpnvms       = 2,
		.n_banks        = 1,
		{
//		.bank[0] = {
		  {
			.probed = 0,
			.pChip  = NULL,
			.pBank  = NULL,
			.bank_number = 0,
			.base_address = FLASH_BANK_BASE_S,

			.controller_address = 0x400e0a00,
			.present = 1,
			.size_bytes =  64 * 1024,
			.nsectors   =  8,
			.sector_size = 8192,
			.page_size   = 256,
		  },
//		.bank[1] = {
		  {
			.present = 0,
			.probed = 0,
			.bank_number = 1,

		  },
		},
	},

// _BM_ end ---------------------------------------------------

	// terminate
	{
		.chipid_cidr	= 0,
		.name			= NULL,
	}
};

/* Globals above */
/***********************************************************************
 **********************************************************************
 **********************************************************************
 **********************************************************************
 **********************************************************************
 **********************************************************************/
/* *ATMEL* style code - from the SAM3 driver code */

/**
 * Get the current status of the EEFC and
 * the value of some status bits (LOCKE, PROGE).
 * @param pPrivate - info about the bank
 * @param v        - result goes here
 */
static int
EFC_GetStatus(struct sam3_bank_private *pPrivate, uint32_t *v)
{
	int r;
	r = target_read_u32(pPrivate->pChip->target, pPrivate->controller_address + offset_EFC_FSR, v);
	LOG_DEBUG("Status: 0x%08x (lockerror: %d, cmderror: %d, ready: %d)",
			  (unsigned int)(*v),
			  ((unsigned int)((*v >> 2) & 1)),
			  ((unsigned int)((*v >> 1) & 1)),
			  ((unsigned int)((*v >> 0) & 1)));

	return r;
}

/**
 * Get the result of the last executed command.
 * @param pPrivate - info about the bank
 * @param v        - result goes here
 */
static int
EFC_GetResult(struct sam3_bank_private *pPrivate, uint32_t *v)
{
	int r;
	uint32_t rv;
	r = target_read_u32(pPrivate->pChip->target, pPrivate->controller_address + offset_EFC_FRR, &rv);
	if (v) {
		*v = rv;
	}
	LOG_DEBUG("Result: 0x%08x", ((unsigned int)(rv)));
	return r;
}

static int
EFC_StartCommand(struct sam3_bank_private *pPrivate,
				 unsigned command, unsigned argument)
{
	uint32_t n,v;
	int r;
	int retry;

	retry = 0;
 do_retry:

    // Check command & argument
    switch (command) {

	case AT91C_EFC_FCMD_WP:
	case AT91C_EFC_FCMD_WPL:
	case AT91C_EFC_FCMD_EWP:
	case AT91C_EFC_FCMD_EWPL:
		// case AT91C_EFC_FCMD_EPL:
		// case AT91C_EFC_FCMD_EPA:
	case AT91C_EFC_FCMD_SLB:
	case AT91C_EFC_FCMD_CLB:
		n = (pPrivate->size_bytes / pPrivate->page_size);
		if (argument >= n) {
			LOG_ERROR("*BUG*: Embedded flash has only %u pages", (unsigned)(n));
		}
		break;

	case AT91C_EFC_FCMD_SFB:
	case AT91C_EFC_FCMD_CFB:
		if (argument >= pPrivate->pChip->details.n_gpnvms) {
			LOG_ERROR("*BUG*: Embedded flash has only %d GPNVMs",
					  pPrivate->pChip->details.n_gpnvms);
		}
		break;

	case AT91C_EFC_FCMD_GETD:
	case AT91C_EFC_FCMD_EA:
	case AT91C_EFC_FCMD_GLB:
	case AT91C_EFC_FCMD_GFB:
	case AT91C_EFC_FCMD_STUI:
	case AT91C_EFC_FCMD_SPUI:
		if (argument != 0) {
			LOG_ERROR("Argument is meaningless for cmd: %d", command);
		}
		break;
	default:
		LOG_ERROR("Unknown command %d", command);
		break;
    }

	if (command == AT91C_EFC_FCMD_SPUI) {
		// this is a very special situation.
		// Situation (1) - error/retry - see below
		//      And we are being called recursively
		// Situation (2) - normal, finished reading unique id
	} else {
		// it should be "ready"
		EFC_GetStatus(pPrivate, &v);
		if (v & 1) {
			// then it is ready
			// we go on
		} else {
			if (retry) {
				// we have done this before
				// the controller is not responding.
				LOG_ERROR("flash controller(%d) is not ready! Error", pPrivate->bank_number);
				return ERROR_FAIL;
			} else {
				retry++;
				LOG_ERROR("Flash controller(%d) is not ready, attempting reset",
						  pPrivate->bank_number);
				// we do that by issuing the *STOP* command
				EFC_StartCommand(pPrivate, AT91C_EFC_FCMD_SPUI, 0);
				// above is recursive, and further recursion is blocked by
				// if (command == AT91C_EFC_FCMD_SPUI) above
				goto do_retry;
			}
		}
	}

	v = (0x5A << 24) | (argument << 8) | command;
	LOG_DEBUG("Command: 0x%08x", ((unsigned int)(v)));
	r = target_write_u32(pPrivate->pBank->target,
						  pPrivate->controller_address + offset_EFC_FCR,
						  v);
	if (r != ERROR_OK) {
		LOG_DEBUG("Error Write failed");
	}
	return r;
}

/**
 * Performs the given command and wait until its completion (or an error).
 * @param pPrivate - info about the bank
 * @param command  - Command to perform.
 * @param argument - Optional command argument.
 * @param status   - put command status bits here
 */
static int
EFC_PerformCommand(struct sam3_bank_private *pPrivate,
					unsigned command,
					unsigned argument,
					uint32_t *status)
{

	int r;
	uint32_t v;
	long long ms_now, ms_end;

	// default
	if (status) {
		*status = 0;
	}

	r = EFC_StartCommand(pPrivate, command, argument);
	if (r != ERROR_OK) {
		return r;
	}

	ms_end = 500 + timeval_ms();


    do {
		r = EFC_GetStatus(pPrivate, &v);
		if (r != ERROR_OK) {
			return r;
		}
		ms_now = timeval_ms();
		if (ms_now > ms_end) {
			// error
			LOG_ERROR("Command timeout");
			return ERROR_FAIL;
		}
    }
    while ((v & 1) == 0)
		;

	// error bits..
	if (status) {
		*status = (v & 0x6);
	}
	return ERROR_OK;

}





/**
 * Read the unique ID.
 * @param pPrivate - info about the bank
 * The unique ID is stored in the 'pPrivate' structure.
 */
static int
FLASHD_ReadUniqueID (struct sam3_bank_private *pPrivate)
{
	int r;
	uint32_t v;
	int x;
	// assume 0
    pPrivate->pChip->cfg.unique_id[0] = 0;
    pPrivate->pChip->cfg.unique_id[1] = 0;
    pPrivate->pChip->cfg.unique_id[2] = 0;
    pPrivate->pChip->cfg.unique_id[3] = 0;

	LOG_DEBUG("Begin");
	r = EFC_StartCommand(pPrivate, AT91C_EFC_FCMD_STUI, 0);
	if (r < 0) {
		return r;
	}

	for (x = 0 ; x < 4 ; x++) {
		r = target_read_u32(pPrivate->pChip->target,
							 pPrivate->pBank->base + (x * 4),
							 &v);
		if (r < 0) {
			return r;
		}
		pPrivate->pChip->cfg.unique_id[x] = v;
	}

    r = EFC_PerformCommand(pPrivate, AT91C_EFC_FCMD_SPUI, 0, NULL);
	LOG_DEBUG("End: R=%d, id = 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			  r,
			  (unsigned int)(pPrivate->pChip->cfg.unique_id[0]),
			  (unsigned int)(pPrivate->pChip->cfg.unique_id[1]),
			  (unsigned int)(pPrivate->pChip->cfg.unique_id[2]),
			  (unsigned int)(pPrivate->pChip->cfg.unique_id[3]));
	return r;

}

/**
 * Erases the entire flash.
 * @param pPrivate - the info about the bank.
 */
static int
FLASHD_EraseEntireBank(struct sam3_bank_private *pPrivate)
{
	LOG_DEBUG("Here");
	return EFC_PerformCommand(pPrivate, AT91C_EFC_FCMD_EA, 0, NULL);
}



/**
 * Gets current GPNVM state.
 * @param pPrivate - info about the bank.
 * @param gpnvm    -  GPNVM bit index.
 * @param puthere  - result stored here.
 */
//------------------------------------------------------------------------------
static int
FLASHD_GetGPNVM(struct sam3_bank_private *pPrivate, unsigned gpnvm, unsigned *puthere)
{
	uint32_t v;
	int r;

	LOG_DEBUG("Here");
	if (pPrivate->bank_number != 0) {
		LOG_ERROR("GPNVM only works with Bank0");
		return ERROR_FAIL;
	}

	if (gpnvm >= pPrivate->pChip->details.n_gpnvms) {
		LOG_ERROR("Invalid GPNVM %d, max: %d, ignored",
				  gpnvm,pPrivate->pChip->details.n_gpnvms);
		return ERROR_FAIL;
	}

    // Get GPNVMs status
	r = EFC_PerformCommand(pPrivate, AT91C_EFC_FCMD_GFB, 0, NULL);
	if (r != ERROR_OK) {
		LOG_ERROR("Failed");
		return r;
	}

    r = EFC_GetResult(pPrivate, &v);

	if (puthere) {
		// Check if GPNVM is set
		// get the bit and make it a 0/1
		*puthere = (v >> gpnvm) & 1;
	}

	return r;
}




/**
 * Clears the selected GPNVM bit.
 * @param pPrivate info about the bank
 * @param gpnvm GPNVM index.
 * @returns 0 if successful; otherwise returns an error code.
 */
static int
FLASHD_ClrGPNVM(struct sam3_bank_private *pPrivate, unsigned gpnvm)
{
	int r;
	unsigned v;

	LOG_DEBUG("Here");
	if (pPrivate->bank_number != 0) {
		LOG_ERROR("GPNVM only works with Bank0");
		return ERROR_FAIL;
	}

	if (gpnvm >= pPrivate->pChip->details.n_gpnvms) {
		LOG_ERROR("Invalid GPNVM %d, max: %d, ignored",
				  gpnvm,pPrivate->pChip->details.n_gpnvms);
		return ERROR_FAIL;
	}

	r = FLASHD_GetGPNVM(pPrivate, gpnvm, &v);
	if (r != ERROR_OK) {
		LOG_DEBUG("Failed: %d",r);
		return r;
	}
	r = EFC_PerformCommand(pPrivate, AT91C_EFC_FCMD_CFB, gpnvm, NULL);
	LOG_DEBUG("End: %d",r);
	return r;
}



/**
 * Sets the selected GPNVM bit.
 * @param pPrivate info about the bank
 * @param gpnvm GPNVM index.
 */
static int
FLASHD_SetGPNVM(struct sam3_bank_private *pPrivate, unsigned gpnvm)
{
	int r;
	unsigned v;

	if (pPrivate->bank_number != 0) {
		LOG_ERROR("GPNVM only works with Bank0");
		return ERROR_FAIL;
	}

	if (gpnvm >= pPrivate->pChip->details.n_gpnvms) {
		LOG_ERROR("Invalid GPNVM %d, max: %d, ignored",
				  gpnvm,pPrivate->pChip->details.n_gpnvms);
		return ERROR_FAIL;
	}

	r = FLASHD_GetGPNVM(pPrivate, gpnvm, &v);
	if (r != ERROR_OK) {
		return r;
	}
	if (v) {
		// already set
		r = ERROR_OK;
	} else {
		// set it
		r = EFC_PerformCommand(pPrivate, AT91C_EFC_FCMD_SFB, gpnvm, NULL);
	}
	return r;
}


/**
 * Returns a bit field (at most 64) of locked regions within a page.
 * @param pPrivate info about the bank
 * @param v where to store locked bits
 */
static int
FLASHD_GetLockBits(struct sam3_bank_private *pPrivate, uint32_t *v)
{
	int r;
	LOG_DEBUG("Here");
    r = EFC_PerformCommand(pPrivate, AT91C_EFC_FCMD_GLB, 0, NULL);
	if (r == ERROR_OK) {
		r = EFC_GetResult(pPrivate, v);
	}
	LOG_DEBUG("End: %d",r);
	return r;
}


/**
 * Unlocks all the regions in the given address range.
 * @param pPrivate info about the bank
 * @param start_sector first sector to unlock
 * @param end_sector last (inclusive) to unlock
 */

static int
FLASHD_Unlock(struct sam3_bank_private *pPrivate,
			   unsigned start_sector,
			   unsigned end_sector)
{
	int r;
	uint32_t status;
	uint32_t pg;
	uint32_t pages_per_sector;

	pages_per_sector = pPrivate->sector_size / pPrivate->page_size;

    /* Unlock all pages */
    while (start_sector <= end_sector) {
		pg = start_sector * pages_per_sector;

        r = EFC_PerformCommand(pPrivate, AT91C_EFC_FCMD_CLB, pg, &status);
        if (r != ERROR_OK) {
            return r;
        }
        start_sector++;
    }

    return ERROR_OK;
}


/**
 * Locks regions
 * @param pPrivate - info about the bank
 * @param start_sector - first sector to lock
 * @param end_sector   - last sector (inclusive) to lock
 */
static int
FLASHD_Lock(struct sam3_bank_private *pPrivate,
			 unsigned start_sector,
			 unsigned end_sector)
{
	uint32_t status;
	uint32_t pg;
	uint32_t pages_per_sector;
	int r;

	pages_per_sector = pPrivate->sector_size / pPrivate->page_size;

    /* Lock all pages */
    while (start_sector <= end_sector) {
		pg = start_sector * pages_per_sector;

        r = EFC_PerformCommand(pPrivate, AT91C_EFC_FCMD_SLB, pg, &status);
        if (r != ERROR_OK) {
            return r;
        }
        start_sector++;
    }
    return ERROR_OK;
}


/****** END SAM3 CODE ********/

/* begin helpful debug code */

static void
sam3_sprintf(struct sam3_chip *pChip , const char *fmt, ...)
{
	va_list ap;
    // _BM_ from here
	//va_start(ap,fmt);

	if (pChip->mbuf == NULL) {
		return;
	}

    // _BM_ to here
	va_start(ap,fmt);

	membuf_vsprintf(pChip->mbuf, fmt, ap);
	va_end(ap);
}

// print the fieldname, the field value, in dec & hex, and return field value
static uint32_t
sam3_reg_fieldname(struct sam3_chip *pChip,
					const char *regname,
					uint32_t value,
					unsigned shift,
					unsigned width)
{
	uint32_t v;
	int hwidth, dwidth;


	// extract the field
	v = value >> shift;
	v = v & ((1 << width)-1);
	if (width <= 16) {
		hwidth = 4;
		dwidth = 5;
	} else {
		hwidth = 8;
		dwidth = 12;
	}

	// show the basics
	sam3_sprintf(pChip, "\t%*s: %*d [0x%0*x] ",
				  REG_NAME_WIDTH, regname,
				  dwidth, v,
				  hwidth, v);
	return v;
}


static const char _unknown[] = "unknown";
static const char * const eproc_names[] = {
	_unknown,					// 0
	"arm946es",					// 1
	"arm7tdmi",					// 2
	"cortex-m3",				// 3
	"arm920t",					// 4
	"arm926ejs",				// 5
	_unknown,					// 6
	_unknown,					// 7
	_unknown,					// 8
	_unknown,					// 9
	_unknown,					// 10
	_unknown,					// 11
	_unknown,					// 12
	_unknown,					// 13
	_unknown,					// 14
	_unknown,					// 15
};

#define nvpsize2 nvpsize		// these two tables are identical
static const char * const nvpsize[] = {
	"none",						//  0
	"8K bytes",					//  1
	"16K bytes",				//  2
	"32K bytes",				//  3
	_unknown,					//  4
	"64K bytes",				//  5
	_unknown,					//  6
	"128K bytes",				//  7
	_unknown,					//  8
	"256K bytes",				//  9
	"512K bytes",				// 10
	_unknown,					// 11
	"1024K bytes",				// 12
	_unknown,					// 13
	"2048K bytes",				// 14
	_unknown,					// 15
};


static const char * const sramsize[] = {
	"48K Bytes",				//  0
	"1K Bytes",					//  1
	"2K Bytes",					//  2
	"6K Bytes",					//  3
	"112K Bytes",				//  4
	"4K Bytes",					//  5
	"80K Bytes",				//  6
	"160K Bytes",				//  7
	"8K Bytes",					//  8
	"16K Bytes",				//  9
	"32K Bytes",				// 10
	"64K Bytes",				// 11
	"128K Bytes",				// 12
	"256K Bytes",				// 13
	"96K Bytes",				// 14
	"512K Bytes",				// 15

};

static const struct archnames { unsigned value; const char *name; } archnames[] = {
	{ 0x19,  "AT91SAM9xx Series"						},
	{ 0x29,  "AT91SAM9XExx Series"						},
	{ 0x34,  "AT91x34 Series"							},
	{ 0x37,  "CAP7 Series"								},
	{ 0x39,  "CAP9 Series"								},
	{ 0x3B,  "CAP11 Series"								},
	{ 0x40,  "AT91x40 Series"							},
	{ 0x42,  "AT91x42 Series"							},
	{ 0x55,  "AT91x55 Series"							},
	{ 0x60,  "AT91SAM7Axx Series"						},
	{ 0x61,  "AT91SAM7AQxx Series"						},
	{ 0x63,  "AT91x63 Series"							},
	{ 0x70,  "AT91SAM7Sxx Series"						},
	{ 0x71,  "AT91SAM7XCxx Series"						},
	{ 0x72,  "AT91SAM7SExx Series"						},
	{ 0x73,  "AT91SAM7Lxx Series"						},
	{ 0x75,  "AT91SAM7Xxx Series"						},
	{ 0x76,  "AT91SAM7SLxx Series"						},
	{ 0x80,  "ATSAM3UxC Series (100-pin version)"		},
	{ 0x81,  "ATSAM3UxE Series (144-pin version)"		},
	{ 0x83,  "ATSAM3AxC Series (100-pin version)"		},
	{ 0x84,  "ATSAM3XxC Series (100-pin version)"		},
	{ 0x85,  "ATSAM3XxE Series (144-pin version)"		},
	{ 0x86,  "ATSAM3XxG Series (208/217-pin version)"	},
	{ 0x88,  "ATSAM3SxA Series (48-pin version)"		},
	{ 0x89,  "ATSAM3SxB Series (64-pin version)"		},
	{ 0x8A,  "ATSAM3SxC Series (100-pin version)"		},
	{ 0x92,  "AT91x92 Series"							},
	{ 0xF0,  "AT75Cxx Series"							},
	{ -1, NULL },

};

static const char * const nvptype[] = {
	"rom", // 0
	"romless or onchip flash", // 1
	"embedded flash memory", // 2
	"rom(nvpsiz) + embedded flash (nvpsiz2)", //3
	"sram emulating flash", // 4
	_unknown, // 5
	_unknown, // 6
	_unknown, // 7

};

static const char *_yes_or_no(uint32_t v)
{
	if (v) {
		return "YES";
	} else {
		return "NO";
	}
}

static const char * const _rc_freq[] = {
	"4 MHz", "8 MHz", "12 MHz", "reserved"
};

static void
sam3_explain_ckgr_mor(struct sam3_chip *pChip)
{
	uint32_t v;
	uint32_t rcen;

	v = sam3_reg_fieldname(pChip, "MOSCXTEN", pChip->cfg.CKGR_MOR, 0, 1);
	sam3_sprintf(pChip, "(main xtal enabled: %s)\n",
				  _yes_or_no(v));
	v = sam3_reg_fieldname(pChip, "MOSCXTBY", pChip->cfg.CKGR_MOR, 1, 1);
	sam3_sprintf(pChip, "(main osc bypass: %s)\n",
				  _yes_or_no(v));
	rcen = sam3_reg_fieldname(pChip, "MOSCRCEN", pChip->cfg.CKGR_MOR, 2, 1);
	sam3_sprintf(pChip, "(onchip RC-OSC enabled: %s)\n",
				  _yes_or_no(rcen));
	v = sam3_reg_fieldname(pChip, "MOSCRCF", pChip->cfg.CKGR_MOR, 4, 3);
	sam3_sprintf(pChip, "(onchip RC-OSC freq: %s)\n",
				  _rc_freq[v]);

	pChip->cfg.rc_freq = 0;
	if (rcen) {
		switch (v) {
		default:
			pChip->cfg.rc_freq = 0;
		case 0:
			pChip->cfg.rc_freq = 4 * 1000 * 1000;
			break;
		case 1:
			pChip->cfg.rc_freq = 8 * 1000 * 1000;
			break;
		case 2:
			pChip->cfg.rc_freq = 12* 1000 * 1000;
			break;
		}
	}

	v = sam3_reg_fieldname(pChip,"MOSCXTST", pChip->cfg.CKGR_MOR, 8, 8);
	sam3_sprintf(pChip, "(startup clks, time= %f uSecs)\n",
				  ((float)(v * 1000000)) / ((float)(pChip->cfg.slow_freq)));
	v = sam3_reg_fieldname(pChip, "MOSCSEL", pChip->cfg.CKGR_MOR, 24, 1);
	sam3_sprintf(pChip, "(mainosc source: %s)\n",
				  v ? "external xtal" : "internal RC");

	v = sam3_reg_fieldname(pChip,"CFDEN", pChip->cfg.CKGR_MOR, 25, 1);
	sam3_sprintf(pChip, "(clock failure enabled: %s)\n",
				 _yes_or_no(v));
}



static void
sam3_explain_chipid_cidr(struct sam3_chip *pChip)
{
	int x;
	uint32_t v;
	const char *cp;

	sam3_reg_fieldname(pChip, "Version", pChip->cfg.CHIPID_CIDR, 0, 5);
	sam3_sprintf(pChip,"\n");

	v = sam3_reg_fieldname(pChip, "EPROC", pChip->cfg.CHIPID_CIDR, 5, 3);
	sam3_sprintf(pChip, "%s\n", eproc_names[v]);

	v = sam3_reg_fieldname(pChip, "NVPSIZE", pChip->cfg.CHIPID_CIDR, 8, 4);
	sam3_sprintf(pChip, "%s\n", nvpsize[v]);

	v = sam3_reg_fieldname(pChip, "NVPSIZE2", pChip->cfg.CHIPID_CIDR, 12, 4);
	sam3_sprintf(pChip, "%s\n", nvpsize2[v]);

	v = sam3_reg_fieldname(pChip, "SRAMSIZE", pChip->cfg.CHIPID_CIDR, 16,4);
	sam3_sprintf(pChip, "%s\n", sramsize[ v ]);

	v = sam3_reg_fieldname(pChip, "ARCH", pChip->cfg.CHIPID_CIDR, 20, 8);
	cp = _unknown;
	for (x = 0 ; archnames[x].name ; x++) {
		if (v == archnames[x].value) {
			cp = archnames[x].name;
			break;
		}
	}

	sam3_sprintf(pChip, "%s\n", cp);

	v = sam3_reg_fieldname(pChip, "NVPTYP", pChip->cfg.CHIPID_CIDR, 28, 3);
	sam3_sprintf(pChip, "%s\n", nvptype[ v ]);

	v = sam3_reg_fieldname(pChip, "EXTID", pChip->cfg.CHIPID_CIDR, 31, 1);
	sam3_sprintf(pChip, "(exists: %s)\n", _yes_or_no(v));
}

static void
sam3_explain_ckgr_mcfr(struct sam3_chip *pChip)
{
	uint32_t v;


	v = sam3_reg_fieldname(pChip, "MAINFRDY", pChip->cfg.CKGR_MCFR, 16, 1);
	sam3_sprintf(pChip, "(main ready: %s)\n", _yes_or_no(v));

	v = sam3_reg_fieldname(pChip, "MAINF", pChip->cfg.CKGR_MCFR, 0, 16);

	v = (v * pChip->cfg.slow_freq) / 16;
	pChip->cfg.mainosc_freq = v;

	sam3_sprintf(pChip, "(%3.03f Mhz (%d.%03dkhz slowclk)\n",
				 _tomhz(v),
				 pChip->cfg.slow_freq / 1000,
				 pChip->cfg.slow_freq % 1000);

}

static void
sam3_explain_ckgr_plla(struct sam3_chip *pChip)
{
	uint32_t mula,diva;

	diva = sam3_reg_fieldname(pChip, "DIVA", pChip->cfg.CKGR_PLLAR, 0, 8);
	sam3_sprintf(pChip,"\n");
	mula = sam3_reg_fieldname(pChip, "MULA", pChip->cfg.CKGR_PLLAR, 16, 11);
	sam3_sprintf(pChip,"\n");
	pChip->cfg.plla_freq = 0;
	if (mula == 0) {
		sam3_sprintf(pChip,"\tPLLA Freq: (Disabled,mula = 0)\n");
	} else if (diva == 0) {
		sam3_sprintf(pChip,"\tPLLA Freq: (Disabled,diva = 0)\n");
	} else if (diva == 1) {
		pChip->cfg.plla_freq = (pChip->cfg.mainosc_freq * (mula + 1));
		sam3_sprintf(pChip,"\tPLLA Freq: %3.03f MHz\n",
					 _tomhz(pChip->cfg.plla_freq));
	}
}


static void
sam3_explain_mckr(struct sam3_chip *pChip)
{
	uint32_t css, pres, fin = 0;
	int pdiv = 0;
	const char *cp = NULL;

	css = sam3_reg_fieldname(pChip, "CSS", pChip->cfg.PMC_MCKR, 0, 2);
	switch (css & 3) {
	case 0:
		fin = pChip->cfg.slow_freq;
		cp = "slowclk";
		break;
	case 1:
		fin = pChip->cfg.mainosc_freq;
		cp  = "mainosc";
		break;
	case 2:
		fin = pChip->cfg.plla_freq;
		cp  = "plla";
		break;
	case 3:
		if (pChip->cfg.CKGR_UCKR & (1 << 16)) {
			fin = 480 * 1000 * 1000;
			cp = "upll";
		} else {
			fin = 0;
			cp  = "upll (*ERROR* UPLL is disabled)";
		}
		break;
	default:
		assert(0);
		break;
	}

	sam3_sprintf(pChip, "%s (%3.03f Mhz)\n",
				  cp,
				  _tomhz(fin));
	pres = sam3_reg_fieldname(pChip, "PRES", pChip->cfg.PMC_MCKR, 4, 3);
	switch (pres & 0x07) {
	case 0:
		pdiv = 1;
		cp = "selected clock";
	case 1:
		pdiv = 2;
		cp = "clock/2";
		break;
	case 2:
		pdiv = 4;
		cp = "clock/4";
		break;
	case 3:
		pdiv = 8;
		cp = "clock/8";
		break;
	case 4:
		pdiv = 16;
		cp = "clock/16";
		break;
	case 5:
		pdiv = 32;
		cp = "clock/32";
		break;
	case 6:
		pdiv = 64;
		cp = "clock/64";
		break;
	case 7:
		pdiv = 6;
		cp = "clock/6";
		break;
	default:
		assert(0);
		break;
	}
	sam3_sprintf(pChip, "(%s)\n", cp);
	fin = fin / pdiv;
	// sam3 has a *SINGLE* clock -
	// other at91 series parts have divisors for these.
	pChip->cfg.cpu_freq = fin;
	pChip->cfg.mclk_freq = fin;
	pChip->cfg.fclk_freq = fin;
	sam3_sprintf(pChip, "\t\tResult CPU Freq: %3.03f\n",
				  _tomhz(fin));
}

#if 0
static struct sam3_chip *
target2sam3(struct target *pTarget)
{
	struct sam3_chip *pChip;

	if (pTarget == NULL) {
		return NULL;
	}

	pChip = all_sam3_chips;
	while (pChip) {
		if (pChip->target == pTarget) {
			break; // return below
		} else {
			pChip = pChip->next;
		}
	}
	return pChip;
}
#endif

static uint32_t *
sam3_get_reg_ptr(struct sam3_cfg *pCfg, const struct sam3_reg_list *pList)
{
	// this function exists to help
	// keep funky offsetof() errors
	// and casting from causing bugs

	// By using prototypes - we can detect what would
	// be casting errors.

	return ((uint32_t *)(((char *)(pCfg)) + pList->struct_offset));
}


#define SAM3_ENTRY(NAME, FUNC)  { .address = SAM3_ ## NAME, .struct_offset = offsetof(struct sam3_cfg, NAME), #NAME, FUNC }
static const struct sam3_reg_list sam3_all_regs[] = {
	SAM3_ENTRY(CKGR_MOR , sam3_explain_ckgr_mor),
	SAM3_ENTRY(CKGR_MCFR , sam3_explain_ckgr_mcfr),
	SAM3_ENTRY(CKGR_PLLAR , sam3_explain_ckgr_plla),
	SAM3_ENTRY(CKGR_UCKR , NULL),
	SAM3_ENTRY(PMC_FSMR , NULL),
	SAM3_ENTRY(PMC_FSPR , NULL),
	SAM3_ENTRY(PMC_IMR , NULL),
	SAM3_ENTRY(PMC_MCKR , sam3_explain_mckr),
	SAM3_ENTRY(PMC_PCK0 , NULL),
	SAM3_ENTRY(PMC_PCK1 , NULL),
	SAM3_ENTRY(PMC_PCK2 , NULL),
	SAM3_ENTRY(PMC_PCSR , NULL),
	SAM3_ENTRY(PMC_SCSR , NULL),
	SAM3_ENTRY(PMC_SR , NULL),
	SAM3_ENTRY(CHIPID_CIDR , sam3_explain_chipid_cidr),
	SAM3_ENTRY(CHIPID_EXID , NULL),
	SAM3_ENTRY(SUPC_CR, NULL),

	// TERMINATE THE LIST
	{ .name = NULL }
};
#undef SAM3_ENTRY




static struct sam3_bank_private *
get_sam3_bank_private(struct flash_bank *bank)
{
	return (struct sam3_bank_private *)(bank->driver_priv);
}

/**
 * Given a pointer to where it goes in the structure,
 * determine the register name, address from the all registers table.
 */
static const struct sam3_reg_list *
sam3_GetReg(struct sam3_chip *pChip, uint32_t *goes_here)
{
	const struct sam3_reg_list *pReg;

	pReg = &(sam3_all_regs[0]);
	while (pReg->name) {
		uint32_t *pPossible;

		// calculate where this one go..
		// it is "possibly" this register.

		pPossible = ((uint32_t *)(((char *)(&(pChip->cfg))) + pReg->struct_offset));

		// well? Is it this register
		if (pPossible == goes_here) {
			// Jump for joy!
			return pReg;
		}

		// next...
		pReg++;
	}
	// This is *TOTAL*PANIC* - we are totally screwed.
	LOG_ERROR("INVALID SAM3 REGISTER");
	return NULL;
}


static int
sam3_ReadThisReg(struct sam3_chip *pChip, uint32_t *goes_here)
{
	const struct sam3_reg_list *pReg;
	int r;

	pReg = sam3_GetReg(pChip, goes_here);
	if (!pReg) {
		return ERROR_FAIL;
	}

	r = target_read_u32(pChip->target, pReg->address, goes_here);
	if (r != ERROR_OK) {
		LOG_ERROR("Cannot read SAM3 register: %s @ 0x%08x, Err: %d\n",
				  pReg->name, (unsigned)(pReg->address), r);
	}
	return r;
}



static int
sam3_ReadAllRegs(struct sam3_chip *pChip)
{
	int r;
	const struct sam3_reg_list *pReg;

	pReg = &(sam3_all_regs[0]);
	while (pReg->name) {
		r = sam3_ReadThisReg(pChip,
								  sam3_get_reg_ptr(&(pChip->cfg), pReg));
		if (r != ERROR_OK) {
			LOG_ERROR("Cannot read SAM3 registere: %s @ 0x%08x, Error: %d\n",
					  pReg->name, ((unsigned)(pReg->address)), r);
			return r;
		}

		pReg++;
	}

	return ERROR_OK;
}


static int
sam3_GetInfo(struct sam3_chip *pChip)
{
	const struct sam3_reg_list *pReg;
	uint32_t regval;

	membuf_reset(pChip->mbuf);


	pReg = &(sam3_all_regs[0]);
	while (pReg->name) {
		// display all regs
		LOG_DEBUG("Start: %s", pReg->name);
LOG_DEBUG("-1-");
		regval = *sam3_get_reg_ptr(&(pChip->cfg), pReg);
LOG_DEBUG("-2- %d, %s %08x %08x", REG_NAME_WIDTH, pReg->name, pReg->address, regval);
		sam3_sprintf(pChip, "%*s: [0x%08x] -> 0x%08x\n",
					 REG_NAME_WIDTH,
					 pReg->name,
					 pReg->address,
					 regval);
LOG_DEBUG("-3-");
 		if (pReg->explain_func) {
			(*(pReg->explain_func))(pChip);
		}
		LOG_DEBUG("End: %s", pReg->name);
		pReg++;
	}
	sam3_sprintf(pChip,"   rc-osc: %3.03f MHz\n", _tomhz(pChip->cfg.rc_freq));
	sam3_sprintf(pChip,"  mainosc: %3.03f MHz\n", _tomhz(pChip->cfg.mainosc_freq));
	sam3_sprintf(pChip,"     plla: %3.03f MHz\n", _tomhz(pChip->cfg.plla_freq));
	sam3_sprintf(pChip," cpu-freq: %3.03f MHz\n", _tomhz(pChip->cfg.cpu_freq));
	sam3_sprintf(pChip,"mclk-freq: %3.03f MHz\n", _tomhz(pChip->cfg.mclk_freq));


	sam3_sprintf(pChip, " UniqueId: 0x%08x 0x%08x 0x%08x 0x%08x\n",
				  pChip->cfg.unique_id[0],
				  pChip->cfg.unique_id[1],
				  pChip->cfg.unique_id[2],
				  pChip->cfg.unique_id[3]);


	return ERROR_OK;
}


static int
sam3_erase_check(struct flash_bank *bank)
{
	int x;

	LOG_DEBUG("Here");
	if (bank->target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}
	if (0 == bank->num_sectors) {
		LOG_ERROR("Target: not supported/not probed\n");
		return ERROR_FAIL;
	}

	LOG_INFO("sam3 - supports auto-erase, erase_check ignored");
	for (x = 0 ; x < bank->num_sectors ; x++) {
		bank->sectors[x].is_erased = 1;
	}

	LOG_DEBUG("Done");
	return ERROR_OK;
}

static int
sam3_protect_check(struct flash_bank *bank)
{
	int r;
	uint32_t v=0;
	unsigned x;
	struct sam3_bank_private *pPrivate;

	LOG_DEBUG("Begin");
	if (bank->target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	pPrivate = get_sam3_bank_private(bank);
	if (!pPrivate) {
		LOG_ERROR("no private for this bank?");
		return ERROR_FAIL;
	}
	if (!(pPrivate->probed)) {
		return ERROR_FLASH_BANK_NOT_PROBED;
	}

	r = FLASHD_GetLockBits(pPrivate , &v);
	if (r != ERROR_OK) {
		LOG_DEBUG("Failed: %d",r);
		return r;
	}

	for (x = 0 ; x < pPrivate->nsectors ; x++) {
		bank->sectors[x].is_protected = (!!(v & (1 << x)));
	}
	LOG_DEBUG("Done");
	return ERROR_OK;
}

FLASH_BANK_COMMAND_HANDLER(sam3_flash_bank_command)
{
	struct sam3_chip *pChip;

	pChip = all_sam3_chips;

	// is this an existing chip?
	while (pChip) {
		if (pChip->target == bank->target) {
			break;
		}
		pChip = pChip->next;
	}

	if (!pChip) {
		// this is a *NEW* chip
		pChip = calloc(1, sizeof(struct sam3_chip));
		if (!pChip) {
			LOG_ERROR("NO RAM!");
			return ERROR_FAIL;
		}
		pChip->target = bank->target;
		// insert at head
		pChip->next = all_sam3_chips;
		all_sam3_chips = pChip;
		pChip->target = bank->target;
		// assumption is this runs at 32khz
		pChip->cfg.slow_freq = 32768;
		pChip->probed = 0;
		pChip->mbuf = membuf_new();
		if (!(pChip->mbuf)) {
			LOG_ERROR("no memory");
			return ERROR_FAIL;
		}
	}

// _BM_ begin ---------------------------------------------------
	switch (bank->base) {
	default:
		LOG_ERROR("Address 0x%08x invalid bank address (try 0x%08x or 0x%08x \
			[at91sam3u series] or 0x%08x [at91sam3s series])",
 				  ((unsigned int)(bank->base)),
				  ((unsigned int)(FLASH_BANK0_BASE_U)),
				  ((unsigned int)(FLASH_BANK1_BASE_U)),
				  ((unsigned int)(FLASH_BANK_BASE_S)));
//		LOG_ERROR("Address 0x%08x invalid bank address (try 0x%08x or 0x%08x)",
//				  ((unsigned int)(bank->base)),
//				  ((unsigned int)(FLASH_BANK0_BASE)),
//				  ((unsigned int)(FLASH_BANK1_BASE)));
		return ERROR_FAIL;
		break;

	// at91sam3u series
	case FLASH_BANK0_BASE_U:
 		bank->driver_priv = &(pChip->details.bank[0]);
 		bank->bank_number = 0;
 		pChip->details.bank[0].pChip = pChip;
 		pChip->details.bank[0].pBank = bank;
 		break;

	case FLASH_BANK1_BASE_U:
 		bank->driver_priv = &(pChip->details.bank[1]);
 		bank->bank_number = 1;
 		pChip->details.bank[1].pChip = pChip;
 		pChip->details.bank[1].pBank = bank;
 		break;

	// at91sam3s series
	case FLASH_BANK_BASE_S:
		bank->driver_priv = &(pChip->details.bank[0]);
		bank->bank_number = 0;
		pChip->details.bank[0].pChip = pChip;
		pChip->details.bank[0].pBank = bank;
 		break;
	}
// _BM_ end ---------------------------------------------------

	// we initialize after probing.
	return ERROR_OK;
}

static int
sam3_GetDetails(struct sam3_bank_private *pPrivate)
{
	const struct sam3_chip_details *pDetails;
	struct sam3_chip *pChip;
	void *vp;
	struct flash_bank *saved_banks[SAM3_MAX_FLASH_BANKS];

	unsigned x;
	const char *cp;

	LOG_DEBUG("Begin");
	pDetails = all_sam3_details;
	while (pDetails->name) {
		if (pDetails->chipid_cidr == pPrivate->pChip->cfg.CHIPID_CIDR) {
			break;
		} else {
			pDetails++;
		}
	}
	if (pDetails->name == NULL) {
		LOG_ERROR("SAM3 ChipID 0x%08x not found in table (perhaps you can this chip?)",
				  (unsigned int)(pPrivate->pChip->cfg.CHIPID_CIDR));
		// Help the victim, print details about the chip
		membuf_reset(pPrivate->pChip->mbuf);
		membuf_sprintf(pPrivate->pChip->mbuf,
						"SAM3 CHIPID_CIDR: 0x%08x decodes as follows\n",
						pPrivate->pChip->cfg.CHIPID_CIDR);
		sam3_explain_chipid_cidr(pPrivate->pChip);
		cp = membuf_strtok(pPrivate->pChip->mbuf, "\n", &vp);
		while (cp) {
			LOG_INFO("%s", cp);
			cp = membuf_strtok(NULL, "\n", &vp);
		}
		return ERROR_FAIL;
	}

	// DANGER: THERE ARE DRAGONS HERE

	// get our pChip - it is going
	// to be over-written shortly
	pChip = pPrivate->pChip;

	// Note that, in reality:
	//
	//     pPrivate = &(pChip->details.bank[0])
	// or  pPrivate = &(pChip->details.bank[1])
	//

	// save the "bank" pointers
	for (x = 0 ; x < SAM3_MAX_FLASH_BANKS ; x++) {
		saved_banks[ x ] = pChip->details.bank[x].pBank;
	}

	// Overwrite the "details" structure.
	memcpy(&(pPrivate->pChip->details),
			pDetails,
			sizeof(pPrivate->pChip->details));

	// now fix the ghosted pointers
	for (x = 0 ; x < SAM3_MAX_FLASH_BANKS ; x++) {
		pChip->details.bank[x].pChip = pChip;
		pChip->details.bank[x].pBank = saved_banks[x];
	}

	// update the *BANK*SIZE*

	LOG_DEBUG("End");
	return ERROR_OK;
}



static int
_sam3_probe(struct flash_bank *bank, int noise)
{
	unsigned x;
	int r;
	struct sam3_bank_private *pPrivate;


	LOG_DEBUG("Begin: Bank: %d, Noise: %d", bank->bank_number, noise);
	if (bank->target->state != TARGET_HALTED)
	{
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	pPrivate = get_sam3_bank_private(bank);
	if (!pPrivate) {
		LOG_ERROR("Invalid/unknown bank number\n");
		return ERROR_FAIL;
	}

	r = sam3_ReadAllRegs(pPrivate->pChip);
	if (r != ERROR_OK) {
		return r;
	}


	LOG_DEBUG("Here");
	r = sam3_GetInfo(pPrivate->pChip);
	if (r != ERROR_OK) {
		return r;
	}
	if (!(pPrivate->pChip->probed)) {
		pPrivate->pChip->probed = 1;
		LOG_DEBUG("Here");
		r = sam3_GetDetails(pPrivate);
		if (r != ERROR_OK) {
			return r;
		}
	}

	// update the flash bank size
	for (x = 0 ; x < SAM3_MAX_FLASH_BANKS ; x++) {
		if (bank->base == pPrivate->pChip->details.bank[0].base_address) {
			bank->size =  pPrivate->pChip->details.bank[0].size_bytes;
			break;
		}
	}

	if (bank->sectors == NULL) {
		bank->sectors     = calloc(pPrivate->nsectors, (sizeof((bank->sectors)[0])));
		if (bank->sectors == NULL) {
			LOG_ERROR("No memory!");
			return ERROR_FAIL;
		}
		bank->num_sectors = pPrivate->nsectors;

		for (x = 0 ; ((int)(x)) < bank->num_sectors ; x++) {
			bank->sectors[x].size         = pPrivate->sector_size;
			bank->sectors[x].offset       = x * (pPrivate->sector_size);
			// mark as unknown
			bank->sectors[x].is_erased    = -1;
			bank->sectors[x].is_protected = -1;
		}
	}

	pPrivate->probed = 1;

	r = sam3_protect_check(bank);
	if (r != ERROR_OK) {
		return r;
	}

	LOG_DEBUG("Bank = %d, nbanks = %d",
			  pPrivate->bank_number , pPrivate->pChip->details.n_banks);
	if ((pPrivate->bank_number + 1) == pPrivate->pChip->details.n_banks) {
		// read unique id,
		// it appears to be associated with the *last* flash bank.
		FLASHD_ReadUniqueID(pPrivate);
	}

	return r;
}

static int
sam3_probe(struct flash_bank *bank)
{
	return _sam3_probe(bank, 1);
}

static int
sam3_auto_probe(struct flash_bank *bank)
{
	return _sam3_probe(bank, 0);
}



static int
sam3_erase(struct flash_bank *bank, int first, int last)
{
	struct sam3_bank_private *pPrivate;
	int r;

	LOG_DEBUG("Here");
	if (bank->target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	r = sam3_auto_probe(bank);
	if (r != ERROR_OK) {
		LOG_DEBUG("Here,r=%d",r);
		return r;
	}

	pPrivate = get_sam3_bank_private(bank);
	if (!(pPrivate->probed)) {
		return ERROR_FLASH_BANK_NOT_PROBED;
	}

	if ((first == 0) && ((last + 1)== ((int)(pPrivate->nsectors)))) {
		// whole chip
		LOG_DEBUG("Here");
		return FLASHD_EraseEntireBank(pPrivate);
	}
	LOG_INFO("sam3 auto-erases while programing (request ignored)");
	return ERROR_OK;
}

static int
sam3_protect(struct flash_bank *bank, int set, int first, int last)
{
	struct sam3_bank_private *pPrivate;
	int r;

	LOG_DEBUG("Here");
	if (bank->target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	pPrivate = get_sam3_bank_private(bank);
	if (!(pPrivate->probed)) {
		return ERROR_FLASH_BANK_NOT_PROBED;
	}

	if (set) {
		r = FLASHD_Lock(pPrivate, (unsigned)(first), (unsigned)(last));
	} else {
		r = FLASHD_Unlock(pPrivate, (unsigned)(first), (unsigned)(last));
	}
	LOG_DEBUG("End: r=%d",r);

	return r;

}


static int
sam3_info(struct flash_bank *bank, char *buf, int buf_size)
{
	if (bank->target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}
	buf[ 0 ] = 0;
	return ERROR_OK;
}

static int
sam3_page_read(struct sam3_bank_private *pPrivate, unsigned pagenum, uint8_t *buf)
{
	uint32_t adr;
	int r;

	adr = pagenum * pPrivate->page_size;
	adr += adr + pPrivate->base_address;

	r = target_read_memory(pPrivate->pChip->target,
							adr,
							4, /* THIS*MUST*BE* in 32bit values */
							pPrivate->page_size / 4,
							buf);
	if (r != ERROR_OK) {
		LOG_ERROR("SAM3: Flash program failed to read page phys address: 0x%08x", (unsigned int)(adr));
	}
	return r;
}

// The code below is basically this:
// compiled with
// arm-none-eabi-gcc -mthumb -mcpu = cortex-m3 -O9 -S ./foobar.c -o foobar.s
//
// Only the *CPU* can write to the flash buffer.
// the DAP cannot... so - we download this 28byte thing
// Run the algorithm - (below)
// to program the device
//
// ========================================
// #include <stdint.h>
//
// struct foo {
//   uint32_t *dst;
//   const uint32_t *src;
//   int   n;
//   volatile uint32_t *base;
//   uint32_t   cmd;
// };
//
//
// uint32_t sam3_function(struct foo *p)
// {
//   volatile uint32_t *v;
//   uint32_t *d;
//   const uint32_t *s;
//   int   n;
//   uint32_t r;
//
//   d = p->dst;
//   s = p->src;
//   n = p->n;
//
//   do {
//     *d++ = *s++;
//   } while (--n)
//     ;
//
//   v = p->base;
//
//   v[ 1 ] = p->cmd;
//   do {
//     r = v[8/4];
//   } while (!(r&1))
//     ;
//   return r;
// }
// ========================================



static const uint8_t
sam3_page_write_opcodes[] = {
	//  24 0000 0446     		mov	r4, r0
	0x04,0x46,
	//  25 0002 6168     		ldr	r1, [r4, #4]
	0x61,0x68,
	//  26 0004 0068     		ldr	r0, [r0, #0]
	0x00,0x68,
	//  27 0006 A268     		ldr	r2, [r4, #8]
	0xa2,0x68,
	//  28              		@ lr needed for prologue
	//  29              	.L2:
	//  30 0008 51F8043B 		ldr	r3, [r1], #4
	0x51,0xf8,0x04,0x3b,
	//  31 000c 12F1FF32 		adds	r2, r2, #-1
	0x12,0xf1,0xff,0x32,
	//  32 0010 40F8043B 		str	r3, [r0], #4
	0x40,0xf8,0x04,0x3b,
	//  33 0014 F8D1     		bne	.L2
	0xf8,0xd1,
	//  34 0016 E268     		ldr	r2, [r4, #12]
	0xe2,0x68,
	//  35 0018 2369     		ldr	r3, [r4, #16]
	0x23,0x69,
	//  36 001a 5360     		str	r3, [r2, #4]
	0x53,0x60,
	//  37 001c 0832     		adds	r2, r2, #8
	0x08,0x32,
	//  38              	.L4:
	//  39 001e 1068     		ldr	r0, [r2, #0]
	0x10,0x68,
	//  40 0020 10F0010F 		tst	r0, #1
	0x10,0xf0,0x01,0x0f,
	//  41 0024 FBD0     		beq	.L4
	0xfb,0xd0,
	//  42              	.done:
	//  43 0026 FEE7     		b	.done
	0xfe,0xe7
};


static int
sam3_page_write(struct sam3_bank_private *pPrivate, unsigned pagenum, uint8_t *buf)
{
	uint32_t adr;
	uint32_t status;
	int r;

	adr = pagenum * pPrivate->page_size;
	adr += (adr + pPrivate->base_address);

	LOG_DEBUG("Wr Page %u @ phys address: 0x%08x", pagenum, (unsigned int)(adr));
	r = target_write_memory(pPrivate->pChip->target,
							 adr,
							 4, /* THIS*MUST*BE* in 32bit values */
							 pPrivate->page_size / 4,
							 buf);
	if (r != ERROR_OK) {
		LOG_ERROR("SAM3: Failed to write (buffer) page at phys address 0x%08x", (unsigned int)(adr));
		return r;
	}

	r = EFC_PerformCommand(pPrivate,
							// send Erase & Write Page
							AT91C_EFC_FCMD_EWP,
							pagenum,
							&status);

	if (r != ERROR_OK) {
		LOG_ERROR("SAM3: Error performing Erase & Write page @ phys address 0x%08x", (unsigned int)(adr));
	}
	if (status & (1 << 2)) {
		LOG_ERROR("SAM3: Page @ Phys address 0x%08x is locked", (unsigned int)(adr));
		return ERROR_FAIL;
	}
	if (status & (1 << 1)) {
		LOG_ERROR("SAM3: Flash Command error @phys address 0x%08x", (unsigned int)(adr));
		return ERROR_FAIL;
	}
	return ERROR_OK;
}





static int
sam3_write(struct flash_bank *bank,
		   uint8_t *buffer,
		   uint32_t offset,
		   uint32_t count)
{
	int n;
	unsigned page_cur;
	unsigned page_end;
	int r;
	unsigned page_offset;
	struct sam3_bank_private *pPrivate;
	uint8_t *pagebuffer;

	// incase we bail further below, set this to null
	pagebuffer = NULL;

	// ignore dumb requests
	if (count == 0) {
		r = ERROR_OK;
		goto done;
	}

	if (bank->target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		r = ERROR_TARGET_NOT_HALTED;
		goto done;
	}

	pPrivate = get_sam3_bank_private(bank);
	if (!(pPrivate->probed)) {
		r = ERROR_FLASH_BANK_NOT_PROBED;
		goto done;
	}


	if ((offset + count) > pPrivate->size_bytes) {
		LOG_ERROR("Flash write error - past end of bank");
		LOG_ERROR(" offset: 0x%08x, count 0x%08x, BankEnd: 0x%08x",
				  (unsigned int)(offset),
				  (unsigned int)(count),
				  (unsigned int)(pPrivate->size_bytes));
		r = ERROR_FAIL;
		goto done;
	}

	pagebuffer = malloc(pPrivate->page_size);
	if( !pagebuffer ){
		LOG_ERROR("No memory for %d Byte page buffer", (int)(pPrivate->page_size));
		r = ERROR_FAIL;
		goto done;
	}

	// what page do we start & end in?
	page_cur = offset / pPrivate->page_size;
	page_end = (offset + count - 1) / pPrivate->page_size;

	LOG_DEBUG("Offset: 0x%08x, Count: 0x%08x", (unsigned int)(offset), (unsigned int)(count));
	LOG_DEBUG("Page start: %d, Page End: %d", (int)(page_cur), (int)(page_end));

	// Special case: all one page
	//
	// Otherwise:
	//    (1) non-aligned start
	//    (2) body pages
	//    (3) non-aligned end.

	// Handle special case - all one page.
	if (page_cur == page_end) {
		LOG_DEBUG("Special case, all in one page");
		r = sam3_page_read(pPrivate, page_cur, pagebuffer);
		if (r != ERROR_OK) {
			goto done;
		}

		page_offset = (offset & (pPrivate->page_size-1));
		memcpy(pagebuffer + page_offset,
				buffer,
				count);

		r = sam3_page_write(pPrivate, page_cur, pagebuffer);
		if (r != ERROR_OK) {
			goto done;
		}
		r = ERROR_OK;
		goto done;
	}

	// non-aligned start
	page_offset = offset & (pPrivate->page_size - 1);
	if (page_offset) {
		LOG_DEBUG("Not-Aligned start");
		// read the partial
		r = sam3_page_read(pPrivate, page_cur, pagebuffer);
		if (r != ERROR_OK) {
			goto done;
		}

		// over-write with new data
		n = (pPrivate->page_size - page_offset);
		memcpy(pagebuffer + page_offset,
				buffer,
				n);

		r = sam3_page_write(pPrivate, page_cur, pagebuffer);
		if (r != ERROR_OK) {
			goto done;
		}

		count  -= n;
		offset += n;
		buffer += n;
		page_cur++;
	}

	// intermediate large pages
	// also - the final *terminal*
	// if that terminal page is a full page
	LOG_DEBUG("Full Page Loop: cur=%d, end=%d, count = 0x%08x",
			  (int)page_cur, (int)page_end, (unsigned int)(count));

	while ((page_cur < page_end) &&
		   (count >= pPrivate->page_size)) {
		r = sam3_page_write(pPrivate, page_cur, buffer);
		if (r != ERROR_OK) {
			goto done;
		}
		count    -= pPrivate->page_size;
		buffer   += pPrivate->page_size;
		page_cur += 1;
	}

	// terminal partial page?
	if (count) {
		LOG_DEBUG("Terminal partial page, count = 0x%08x", (unsigned int)(count));
		// we have a partial page
		r = sam3_page_read(pPrivate, page_cur, pagebuffer);
		if (r != ERROR_OK) {
			goto done;
		}
		// data goes at start
		memcpy(pagebuffer, buffer, count);
		r = sam3_page_write(pPrivate, page_cur, pagebuffer);
		if (r != ERROR_OK) {
			goto done;
		}
		buffer += count;
		count  -= count;
	}
	LOG_DEBUG("Done!");
	r = ERROR_OK;
 done:
	if( pagebuffer ){
		free(pagebuffer);
	}
	return r;
}

COMMAND_HANDLER(sam3_handle_info_command)
{
	struct sam3_chip *pChip;
	void *vp;
	const char *cp;
	unsigned x;
	int r;

	pChip = get_current_sam3(CMD_CTX);
	if (!pChip) {
		return ERROR_OK;
	}

	r = 0;

	// bank0 must exist before we can do anything
	if (pChip->details.bank[0].pBank == NULL) {
		x = 0;
	need_define:
		command_print(CMD_CTX,
					   "Please define bank %d via command: flash bank %s ... ",
					   x,
					   at91sam3_flash.name);
		return ERROR_FAIL;
	}

	// if bank 0 is not probed, then probe it
	if (!(pChip->details.bank[0].probed)) {
		r = sam3_auto_probe(pChip->details.bank[0].pBank);
		if (r != ERROR_OK) {
			return ERROR_FAIL;
		}
	}
	// above guarantees the "chip details" structure is valid
	// and thus, bank private areas are valid
	// and we have a SAM3 chip, what a concept!


	// auto-probe other banks, 0 done above
    for (x = 1 ; x < SAM3_MAX_FLASH_BANKS ; x++) {
		// skip banks not present
		if (!(pChip->details.bank[x].present)) {
			continue;
		}

		if (pChip->details.bank[x].pBank == NULL) {
			goto need_define;
		}

		if (pChip->details.bank[x].probed) {
			continue;
		}

		r = sam3_auto_probe(pChip->details.bank[x].pBank);
		if (r != ERROR_OK) {
			return r;
		}
	}


	r = sam3_GetInfo(pChip);
	if (r != ERROR_OK) {
		LOG_DEBUG("Sam3Info, Failed %d\n",r);
		return r;
	}


	// print results
	cp = membuf_strtok(pChip->mbuf, "\n", &vp);
	while (cp) {
		command_print(CMD_CTX,"%s", cp);
		cp = membuf_strtok(NULL, "\n", &vp);
	}
	return ERROR_OK;
}

COMMAND_HANDLER(sam3_handle_gpnvm_command)
{
	unsigned x,v;
	int r,who;
	struct sam3_chip *pChip;

	pChip = get_current_sam3(CMD_CTX);
	if (!pChip) {
		return ERROR_OK;
	}

	if (pChip->target->state != TARGET_HALTED) {
		LOG_ERROR("sam3 - target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}


	if (pChip->details.bank[0].pBank == NULL) {
		command_print(CMD_CTX, "Bank0 must be defined first via: flash bank %s ...",
					   at91sam3_flash.name);
		return ERROR_FAIL;
	}
	if (!pChip->details.bank[0].probed) {
		r = sam3_auto_probe(pChip->details.bank[0].pBank);
		if (r != ERROR_OK) {
			return r;
		}
	}


	switch (CMD_ARGC) {
	default:
		command_print(CMD_CTX,"Too many parameters\n");
		return ERROR_COMMAND_SYNTAX_ERROR;
		break;
	case 0:
		who = -1;
		goto showall;
		break;
	case 1:
		who = -1;
		break;
	case 2:
		if ((0 == strcmp(CMD_ARGV[0], "show")) && (0 == strcmp(CMD_ARGV[1], "all"))) {
			who = -1;
		} else {
			uint32_t v32;
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], v32);
			who = v32;
		}
		break;
	}

	if (0 == strcmp("show", CMD_ARGV[0])) {
		if (who == -1) {
showall:
			r = ERROR_OK;
			for (x = 0 ; x < pChip->details.n_gpnvms ; x++) {
				r = FLASHD_GetGPNVM(&(pChip->details.bank[0]), x, &v);
				if (r != ERROR_OK) {
					break;
				}
				command_print(CMD_CTX, "sam3-gpnvm%u: %u", x, v);
			}
			return r;
		}
		if ((who >= 0) && (((unsigned)(who)) < pChip->details.n_gpnvms)) {
			r = FLASHD_GetGPNVM(&(pChip->details.bank[0]), who, &v);
			command_print(CMD_CTX, "sam3-gpnvm%u: %u", who, v);
			return r;
		} else {
			command_print(CMD_CTX, "sam3-gpnvm invalid GPNVM: %u", who);
			return ERROR_COMMAND_SYNTAX_ERROR;
		}
	}

	if (who == -1) {
		command_print(CMD_CTX, "Missing GPNVM number");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (0 == strcmp("set", CMD_ARGV[0])) {
		r = FLASHD_SetGPNVM(&(pChip->details.bank[0]), who);
	} else if ((0 == strcmp("clr", CMD_ARGV[0])) ||
			   (0 == strcmp("clear", CMD_ARGV[0]))) { // quietly accept both
		r = FLASHD_ClrGPNVM(&(pChip->details.bank[0]), who);
	} else {
		command_print(CMD_CTX, "Unkown command: %s", CMD_ARGV[0]);
		r = ERROR_COMMAND_SYNTAX_ERROR;
	}
	return r;
}

COMMAND_HANDLER(sam3_handle_slowclk_command)
{
	struct sam3_chip *pChip;

	pChip = get_current_sam3(CMD_CTX);
	if (!pChip) {
		return ERROR_OK;
	}


	switch (CMD_ARGC) {
	case 0:
		// show
		break;
	case 1:
	{
		// set
		uint32_t v;
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], v);
		if (v > 200000) {
			// absurd slow clock of 200Khz?
			command_print(CMD_CTX,"Absurd/illegal slow clock freq: %d\n", (int)(v));
			return ERROR_COMMAND_SYNTAX_ERROR;
		}
		pChip->cfg.slow_freq = v;
		break;
	}
	default:
		// error
		command_print(CMD_CTX,"Too many parameters");
		return ERROR_COMMAND_SYNTAX_ERROR;
		break;
	}
	command_print(CMD_CTX, "Slowclk freq: %d.%03dkhz",
				   (int)(pChip->cfg.slow_freq/ 1000),
				   (int)(pChip->cfg.slow_freq% 1000));
	return ERROR_OK;
}

static const struct command_registration at91sam3_exec_command_handlers[] = {
	{
		.name = "gpnvm",
		.handler = sam3_handle_gpnvm_command,
		.mode = COMMAND_EXEC,
		.usage = "[('clr'|'set'|'show') bitnum]",
		.help = "Without arguments, shows all bits in the gpnvm "
			"register.  Otherwise, clears, sets, or shows one "
			"General Purpose Non-Volatile Memory (gpnvm) bit.",
	},
	{
		.name = "info",
		.handler = sam3_handle_info_command,
		.mode = COMMAND_EXEC,
		.help = "Print information about the current at91sam3 chip"
			"and its flash configuration.",
	},
	{
		.name = "slowclk",
		.handler = sam3_handle_slowclk_command,
		.mode = COMMAND_EXEC,
		.usage = "[clock_hz]",
		.help = "Display or set the slowclock frequency "
			"(default 32768 Hz).",
	},
	COMMAND_REGISTRATION_DONE
};
static const struct command_registration at91sam3_command_handlers[] = {
	{
		.name = "at91sam3",
		.mode = COMMAND_ANY,
		.help = "at91sam3 flash command group",
		.chain = at91sam3_exec_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

struct flash_driver at91sam3_flash = {
	.name = "at91sam3",
	.commands = at91sam3_command_handlers,
	.flash_bank_command = sam3_flash_bank_command,
	.erase = sam3_erase,
	.protect = sam3_protect,
	.write = sam3_write,
	.probe = sam3_probe,
	.auto_probe = sam3_auto_probe,
	.erase_check = sam3_erase_check,
	.protect_check = sam3_protect_check,
	.info = sam3_info,
};
