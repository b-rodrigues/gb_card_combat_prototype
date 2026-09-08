#ifndef HUGE_MUSIC_H
#define HUGE_MUSIC_H

#include <gb/gb.h>
#include <stdint.h>
#include "hUGEDriver.h"

#define HUGE_MUSIC_BANK 6
/* Second driver copy for bank-7 songs (bank 6 is full).  Renamed
 * exports (hUGE_*_b7) coexist with the bank-6 originals. */
#define HUGE_MUSIC_BANK_B7 7
extern void hUGE_init_b7(const hUGESong_t *song);
extern void hUGE_dosound_b7(void);
extern void hUGE_mute_channel_b7(enum hUGE_channel_t ch, enum hUGE_mute_t mute);

void huge_music_init(void);
void huge_music_play(const hUGESong_t *song);
/* Play a song from an explicit ROM bank (HUGE_MUSIC_BANK or
 * HUGE_MUSIC_BANK_B7).  huge_music_play() is the bank-6 shorthand. */
void huge_music_play_banked(const hUGESong_t *song, uint8_t bank);
void huge_music_stop(void);
void huge_music_update(void);
void huge_music_pause(void);
void huge_music_resume(void);
uint8_t huge_music_is_playing(void);
void huge_music_mute_channel(uint8_t ch, uint8_t mute);

#endif /* HUGE_MUSIC_H */
