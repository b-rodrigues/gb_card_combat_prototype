#include "audio.h"

static MusicTrack current_track = MUSIC_NONE;
static uint8_t step_counter = 0;
static uint8_t note_index = 0;

#ifdef DEBUG_BUILD
volatile uint16_t g_audio_ticks = 0;
#endif

#define REST 0

static const uint16_t s_note_freqs[14] = {
    0x0000, /* 0: REST */
    0x0642, /* 1: NOTE_D4 */
    0x0627, /* 2: NOTE_CS4 */
    0x0672, /* 3: NOTE_E4 */
    0x0689, /* 4: NOTE_F4 */
    0x06B2, /* 5: NOTE_G4 */
    0x06D6, /* 6: NOTE_A4 */
    0x06E7, /* 7: NOTE_AS4 */
    0x0721, /* 8: NOTE_D5 */
    0x0759, /* 9: NOTE_G5 */
    0x074F, /* 10: NOTE_FS5 */
    0x0739, /* 11: NOTE_E5 */
    0x0714, /* 12: NOTE_CS5 */
    0x069E  /* 13: NOTE_FS4 */
};

/* Overworld: Mozart's "Lacrimosa" (Requiem K.626) */
static const uint8_t lacrimosa_notes[32] = {
    1, 1, 2, 1,  3, 4, 4, 0,
    6, 6, 7, 6,  5, 4, 3, 0,
    4, 4, 3, 4,  5, 6, 6, 0,
    5, 4, 3, 4,  3, 1, 1, 0
};

/* Battle: Vivaldi's "Summer" Presto (Four Seasons, RV 315 3rd mvt.) */
static const uint8_t summer_notes[32] = {
    5, 5, 7, 7,  8, 8, 9, 9,
    9, 10, 11, 8, 12, 8, 6, 6,
    7, 7, 6, 6,  5, 5, 13, 13,
    5, 6, 7, 8,  11, 10, 9, 0
};

static void play_note(uint16_t freq)
{
    if (freq == 0) {
        NR12_REG = 0x00;
        NR14_REG = 0x80;
        return;
    }
    NR10_REG = 0x00;
    NR11_REG = 0x80;
    NR12_REG = 0xF1;
    NR13_REG = (uint8_t)(freq & 0xFF);
    NR14_REG = 0x80 | (uint8_t)((freq >> 8) & 0x07);
}

void audio_init(void)
{
    NR52_REG = 0x80;
    NR50_REG = 0x77;
    NR51_REG = 0xFF;
    current_track = MUSIC_NONE;
    step_counter = 0;
    note_index = 0;

    TAC_REG = 0x00;
    TMA_REG = 0x00;
    TIMA_REG = 0x00;
    TAC_REG = 0x05;
}

void audio_play_music(MusicTrack track)
{
    if (current_track == track) return;
    current_track = track;
    step_counter = 0;
    note_index = 0;
    if (track == MUSIC_NONE) {
        play_note(0);
    }
}

MusicTrack audio_get_current_track(void)
{
    return current_track;
}

void audio_stop(void)
{
    audio_play_music(MUSIC_NONE);
}

void audio_update(void)
{
#ifdef DEBUG_BUILD
    g_audio_ticks++;
#endif
    if (current_track == MUSIC_NONE) return;

    if (current_track == MUSIC_OVERWORLD) {
        if (++step_counter >= 43) {
            step_counter = 0;
            play_note(s_note_freqs[lacrimosa_notes[note_index]]);
            note_index = (uint8_t)((note_index + 1) & 31);
        }
    } else if (current_track == MUSIC_BATTLE) {
        if (++step_counter >= 17) {
            step_counter = 0;
            play_note(s_note_freqs[summer_notes[note_index]]);
            note_index = (uint8_t)((note_index + 1) & 31);
        }
    }
}
