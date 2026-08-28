#ifndef AUDIO_H
#define AUDIO_H

#include <gb/gb.h>
#include <stdint.h>

typedef enum {
    MUSIC_NONE,
    MUSIC_OVERWORLD,
    MUSIC_BATTLE,
    MUSIC_VICTORY,   /* one-shot 4-note fanfare; ends in silence */
    MUSIC_TITLE,     /* title screen theme (loops) */
    MUSIC_TOWN,      /* town / hub theme (loops) */
    MUSIC_DUNGEON,   /* castle / dungeon theme (loops) */
    MUSIC_BOSS       /* final-boss theme (loops) */
} MusicTrack;

/* Master sound enable (title menu SOUND ON/OFF).  Gates both music and
 * SFX: audio_play_music() no-ops (leaving any currently playing track
 * ringing) and audio_play_sfx() drops the note.  When set back ON the next
 * audio_play_music() call restores the requested track. */
extern uint8_t g_sound_enabled;

void audio_init(void);
void audio_play_music(MusicTrack track);

/* Channel-2 one-shot SFX ids.  Distinct pitches: SFX_CURSOR is the short
 * navigation blip (menu open, cursor move, sound toggle); SFX_CONFIRM is a
 * lower-pitch variant for selections (NEW GAME / CONTINUE). */
enum {
    SFX_CURSOR = 0,
    SFX_CONFIRM = 1
};
void audio_play_sfx(uint8_t sfx);
void audio_update(void);

/* Current track (WRAM): readable directly from bank-2 code, which cannot
 * call this fixed-bank getter. */
extern MusicTrack g_audio_current_track;
MusicTrack audio_get_current_track(void);

#ifdef DEBUG_BUILD
/* Per-music-clock-tick counter, incremented once per audio_update() call
 * (i.e. once per ISR tick).  Host-side tools (tools/verify_music.py) sample
 * it every frame to assert the music clock never stalls during LCD-off
 * screen/map transitions (see AGENTS.md Music contract).  Not present in
 * release builds. */
extern volatile uint16_t g_audio_ticks;
#endif

#endif /* AUDIO_H */
