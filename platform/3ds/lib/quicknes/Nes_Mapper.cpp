
// Nes_Emu 0.7.0. http://www.slack.net/~ant/

#include "Nes_Mapper.h"

#include <cstdio>
#include <string.h>
#include "Nes_Core.h"

/* Copyright (C) 2004-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include "blargg_source.h"

/* Changed for the Animal Crossing 3DS port (2026-10-09): only the mappers the
   port ships are listed (0-4, 7, and its own MMC2, mappers/mapper009.hpp),
   allocated with blargg_new (blargg_common.h). */

/*
  New mapping distribution by Sergio Martin (eien86)
  https://github.com/SergioMartin86/jaffarPlus
*/
#include "mappers/mapper000.hpp"
#include "mappers/mapper001.hpp"
#include "mappers/mapper002.hpp"
#include "mappers/mapper003.hpp"
#include "mappers/mapper004.hpp"
#include "mappers/mapper007.hpp"
#include "mappers/mapper009.hpp"

Nes_Mapper::Nes_Mapper()
{
	emu_ = NULL;
	static char c;
	state = &c; // TODO: state must not be null?
	state_size = 0;
}

Nes_Mapper::~Nes_Mapper()
{
}

// Sets mirroring, maps first 8K CHR in, first and last 16K of PRG,
// intercepts writes to upper half of memory, and clears registered state.
void Nes_Mapper::default_reset_state()
{
	int mirroring = cart_->mirroring();
	if ( mirroring & 8 )
		mirror_full();
	else if ( mirroring & 1 )
		mirror_vert();
	else
		mirror_horiz();
	
	set_chr_bank( 0, bank_8k, 0 );
	
	set_prg_bank( 0x8000, bank_16k, 0 );
	set_prg_bank( 0xC000, bank_16k, last_bank );
	
	intercept_writes( 0x8000, 0x8000 );
	
	memset( state, 0, state_size );
}

void Nes_Mapper::reset()
{
	default_reset_state();
	reset_state();
	apply_mapping();
}

void mapper_state_t::write( const void* p, unsigned long s )
{
	size = s;
	memcpy( data, p, s );
}

int mapper_state_t::read( void* p, unsigned long s ) const
{
	if ( (long) s > size )
		s = size;
	memcpy( p, data, s );
	return s;
}

void Nes_Mapper::save_state( mapper_state_t& out )
{
	out.write( state, state_size );
}

void Nes_Mapper::load_state( mapper_state_t const& in )
{
	default_reset_state();
	read_state( in );
	apply_mapping();
}

void Nes_Mapper::read_state( mapper_state_t const& in )
{
	memset( state, 0, state_size );
	in.read( state, state_size );
	apply_mapping();
}

// Timing

void Nes_Mapper::irq_changed() { emu_->irq_changed(); }
	
nes_time_t Nes_Mapper::next_irq( nes_time_t ) { return no_irq; }

void Nes_Mapper::a12_clocked() { }

void Nes_Mapper::run_until( nes_time_t ) { }

void Nes_Mapper::end_frame( nes_time_t ) { }

bool Nes_Mapper::ppu_enabled() const { return emu().ppu.w2001 & 0x08; }

// Sound

int Nes_Mapper::channel_count() const { return 0; }

void Nes_Mapper::set_channel_buf( int, Blip_Buffer* ) { }

void Nes_Mapper::set_treble( blip_eq_t const& ) { }

// Memory mapping

void Nes_Mapper::set_prg_bank( nes_addr_t addr, bank_size_t bs, int bank )
{
	int bank_size = 1 << bs;
	
	int bank_count = cart_->prg_size() >> bs;
	if ( bank < 0 )
		bank += bank_count;
	
	if ( bank >= bank_count )
		bank %= bank_count;
	
	emu().map_code( addr, bank_size, cart_->prg() + (bank << bs) );
	
	if ( unsigned (addr - 0x6000) < 0x2000 )
		emu().enable_prg_6000();
}

void Nes_Mapper::set_chr_bank( nes_addr_t addr, bank_size_t bs, int bank )
{
	emu().ppu.render_until( emu().clock() ); 
	emu().ppu.set_chr_bank( addr, 1 << bs, bank << bs );
}

void Nes_Mapper::set_chr_bank_ex( nes_addr_t addr, bank_size_t bs, int bank )
{
	emu().ppu.render_until( emu().clock() ); 
	emu().ppu.set_chr_bank_ex( addr, 1 << bs, bank << bs );
}

void Nes_Mapper::set_exgrafix( uint8_t const* exram, int chr_high )
{
	emu().ppu.render_until( emu().clock() );
	emu().ppu.set_exgrafix( exram, chr_high );
}

void Nes_Mapper::mirror_manual( int page0, int page1, int page2, int page3 )
{
	emu().ppu.render_bg_until( emu().clock() ); 
	emu().ppu.set_nt_banks( page0, page1, page2, page3 );
}

#ifndef NDEBUG
int Nes_Mapper::handle_bus_conflict( nes_addr_t addr, int data )
{
	return data;
}
#endif


Nes_Mapper* Nes_Mapper::create( Nes_Cart const* cart, Nes_Core* emu )
{
  // Getting cartdrige mapper code
  int mapperCode = cart->mapper_code();
	
  // Storage for the mapper, NULL by default	
  Nes_Mapper* mapper = NULL;

  // Now checking if the detected mapper code is supported. A switch
  // is faster than the old if-chain (which paid for every comparison
  // unconditionally on every cart load) and guards against the silent
  // footgun where two entries for the same mapperCode would have both
  // executed -- the second would have leaked the first.
  switch ( mapperCode )
  {
    case   0: mapper = blargg_new<Mapper000>(); break;
    case   1: mapper = blargg_new<Mapper001>(); break;
    case   2: mapper = blargg_new<Mapper002>(); break;
    case   3: mapper = blargg_new<Mapper003>(); break;
    case   4: mapper = blargg_new<Mapper004>(); break;
    case   7: mapper = blargg_new<Mapper007>(); break;
    case   9: mapper = blargg_new<Mapper009>(); break;
    default: break;
  }

  // If no mapper was found, return null (error) now 
  if (mapper == NULL)
  {
	fprintf(stderr, "Could not find mapper for code: %u\n", mapperCode);
    return NULL;
  } 

  // Assigning backwards pointers to cartdrige and emulator now
  mapper->cart_ = cart;
  mapper->emu_ = emu;

  // Returning successfully created mapper
  return mapper;
}
