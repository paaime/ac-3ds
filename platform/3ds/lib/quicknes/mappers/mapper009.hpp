/* MMC2 (iNES mapper 9: Punch-Out!!), written for the Animal Crossing 3DS port
   from the NESdev wiki's description (https://www.nesdev.org/wiki/MMC2), in
   place of QuickNES's own, whose licence isn't stated. Part of this library,
   under its licence: GNU Lesser General Public License 2.1 or later (LICENSE).

   Each pattern table has two 4K banks, picked by a latch the PPU flips when it
   fetches tile $FD or $FE (Nes_Ppu_Impl::map_chr_addr): the $FD bank is the
   normal mapping, the $FE bank the alternate one (set_chr_bank_ex). */

#include "Nes_Mapper.h"

class Mapper009 : public Nes_Mapper {
	enum { prg_mask = 0x0F, chr_mask = 0x1F };
	enum { chr_0000_fd, chr_0000_fe, chr_1000_fd, chr_1000_fe, chr_reg_count };

	struct mmc2_state_t {
		uint8_t prg;                  // $A000: the 8K bank at $8000
		uint8_t chr [chr_reg_count];  // $B000-$E000
		uint8_t horizontal;           // $F000: 0 vertical mirroring, 1 horizontal
	} regs;

	void apply_chr( int reg )
	{
		nes_addr_t addr = reg < chr_1000_fd ? 0x0000 : 0x1000;
		if ( reg == chr_0000_fe || reg == chr_1000_fe )
			set_chr_bank_ex( addr, bank_4k, regs.chr [reg] );
		else
			set_chr_bank( addr, bank_4k, regs.chr [reg] );
	}

	void apply_mirroring()
	{
		if ( regs.horizontal )
			mirror_horiz();
		else
			mirror_vert();
	}

public:
	Mapper009()
	{
		register_state( &regs, sizeof regs );
	}

	virtual void apply_mapping()
	{
		set_prg_bank( 0x8000, bank_8k, regs.prg );
		set_prg_bank( 0xA000, bank_8k, last_bank - 2 ); // the last three 8K banks are fixed
		set_prg_bank( 0xC000, bank_8k, last_bank - 1 );
		set_prg_bank( 0xE000, bank_8k, last_bank );
		for ( int reg = 0; reg < chr_reg_count; reg++ )
			apply_chr( reg );
		apply_mirroring();
	}

	virtual void write( nes_time_t, nes_addr_t addr, int data )
	{
		switch ( addr >> 12 )
		{
		case 0xA:
			regs.prg = data & prg_mask;
			set_prg_bank( 0x8000, bank_8k, regs.prg );
			break;
		case 0xB:
		case 0xC:
		case 0xD:
		case 0xE: {
			int reg = (addr >> 12) - 0xB;
			regs.chr [reg] = data & chr_mask;
			apply_chr( reg );
			break;
		}
		case 0xF:
			regs.horizontal = data & 1;
			apply_mirroring();
			break;
		}
	}
};
