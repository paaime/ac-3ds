#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quicknes_3ds.h"

extern "C" {
#include "nes_audio_3ds.h"
#include "nes_pixels.h"
#include "nes_video_3ds.h"
}

#define NES_SAMPLE_RATE 32728 /* the DSP's own output rate (ndsp resamples anything else) */
#define MAX_SAMPLES     2048  /* one frame: about 545 */

static const unsigned WRAM_BYTES = Nes_Emu::low_mem_size;

static_assert(sizeof(Nes_Emu::rgb_t) == 3, "nes_colors read as RGB triplets");

extern "C" {
/* Battery RAM: famicom.cpp copies the save in after pc_fixnes_init, out before pc_fixnes_cleanup. */
unsigned char* emuPrgRAM;
unsigned int emuPrgRAMsize;

void pc_fixnes_init(uint8_t* ines_data, int ines_size);
void pc_fixnes_write_wram(unsigned int ofs, const uint8_t* data, unsigned int size);
void pc_fixnes_sync_wram(uint8_t* dst_wram);
void pc_fixnes_reset(void);
void pc_fixnes_set_input(uint8_t buttons);
uint16_t* pc_fixnes_frame(void);
void pc_fixnes_render_frame(uint16_t* frame);
void pc_fixnes_cleanup(void);
}

static struct {
    Nes_Emu* emu;
    uint8_t* pixels;
    uint8_t buttons; /* bit 0 A, 1 B, 2 Select, 3 Start, 4 Up, 5 Down, 6 Left, 7 Right */
    bool frame_error_seen;
    uint16_t lut[NES_PIXELS_LUT_SIZE];
    uint16_t frame[Nes_Emu::image_width * Nes_Emu::image_height];
    int16_t samples[MAX_SAMPLES];
} N;

static void release_emulator(void)
{
    blargg_delete(N.emu);
    N.emu = NULL;
    free(N.pixels);
    N.pixels = NULL;
    emuPrgRAM = NULL;
    emuPrgRAMsize = 0;
}

static const char* start_emulator(const uint8_t* ines_data, long ines_size)
{
    N.emu = blargg_new<Nes_Emu>(); /* not new: the global operator new is JSystem's (the game heap) */
    if (N.emu == NULL) {
        return "out of memory";
    }
    Mem_File_Reader reader(ines_data, ines_size);
    const char* err = N.emu->load_ines(Auto_File_Reader(reader));
    if (err == NULL) {
        err = N.emu->set_sample_rate(NES_SAMPLE_RATE);
    }
    if (err == NULL) {
        N.emu->set_equalizer(Nes_Emu::nes_eq); /* again: loading set it on the silent buffer */
    }
    if (err == NULL) {
        N.pixels = (uint8_t*)malloc((size_t)Nes_Emu::buffer_width * N.emu->buffer_height());
        err = N.pixels == NULL ? "out of memory" : NULL;
    }
    return err;
}

void pc_fixnes_init(uint8_t* ines_data, int ines_size)
{
    release_emulator();
    memset(N.frame, 0, sizeof N.frame);
    N.buttons = 0;
    N.frame_error_seen = false;
    if (ines_data == NULL || ines_size <= 0) {
        fprintf(stderr, "[nes] no game data\n");
        return;
    }
    const char* err = start_emulator(ines_data, ines_size);
    if (err != NULL) {
        fprintf(stderr, "[nes] the game didn't start: %s\n", err);
        release_emulator();
        return;
    }
    N.emu->set_pixels(N.pixels, Nes_Emu::buffer_width);
    emuPrgRAM = N.emu->high_mem();
    emuPrgRAMsize = Nes_Emu::high_mem_size;
    const Nes_Cart* cart = N.emu->cart();
    fprintf(stderr, "[nes] mapper %d, %ld KB PRG, %ld KB CHR\n", cart->mapper_code(), cart->prg_size() / 1024,
            cart->chr_size() / 1024);
}

void pc_fixnes_write_wram(unsigned int ofs, const uint8_t* data, unsigned int size)
{
    if (N.emu != NULL && data != NULL && ofs <= WRAM_BYTES && size <= WRAM_BYTES - ofs) {
        memcpy(N.emu->low_mem() + ofs, data, size);
    }
}

void pc_fixnes_sync_wram(uint8_t* dst_wram)
{
    if (N.emu != NULL && dst_wram != NULL) {
        memcpy(dst_wram, N.emu->low_mem(), WRAM_BYTES);
    }
}

void pc_fixnes_reset(void)
{
    if (N.emu != NULL) {
        N.emu->reset(false);
    }
}

void pc_fixnes_set_input(uint8_t buttons)
{
    N.buttons = buttons;
}

uint16_t* pc_fixnes_frame(void)
{
    if (N.emu == NULL) {
        return N.frame;
    }
    const char* err = N.emu->emulate_frame(N.buttons);
    if (err != NULL && !N.frame_error_seen) {
        fprintf(stderr, "[nes] frame: %s\n", err);
        N.frame_error_seen = true;
    }
    long samples = N.emu->read_samples(N.samples, MAX_SAMPLES);
    if (samples > 0) {
        nes_audio_write(N.samples, (uint32_t)samples, NES_SAMPLE_RATE);
    }
    const Nes_Emu::frame_t& f = N.emu->frame();
    nes_pixels_lut(N.lut, f.palette, f.palette_begin, f.palette_size, &Nes_Emu::nes_colors[0].red,
                   Nes_Emu::color_table_size);
    nes_pixels_to_rgb565(N.frame, f.pixels, f.pitch, Nes_Emu::image_width, Nes_Emu::image_height, N.lut);
    return N.frame;
}

void pc_fixnes_render_frame(uint16_t* frame)
{
    nes_video_show(frame);
}

void pc_fixnes_cleanup(void)
{
    if (N.emu != NULL && N.emu->error_count() != 0) {
        fprintf(stderr, "[nes] %lu undefined CPU instructions\n", N.emu->error_count());
    }
    release_emulator();
    nes_audio_stop();
    nes_video_release();
}
