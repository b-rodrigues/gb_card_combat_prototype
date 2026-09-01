#ifndef HUGE_MUSIC_H
#define HUGE_MUSIC_H

#include <gb/gb.h>
#include <stdint.h>
#include "hUGEDriver.h"

#define HUGE_MUSIC_BANK 6

void huge_music_init(void);
void huge_music_play(const hUGESong_t *song);
void huge_music_stop(void);
void huge_music_update(void);
void huge_music_pause(void);
void huge_music_resume(void);
uint8_t huge_music_is_playing(void);
void huge_music_mute_channel(uint8_t ch, uint8_t mute);

#endif /* HUGE_MUSIC_H */
