#include "audio.h"
#include "telemetry.h"
#include "huge_music.h"
#include "huge_music_data.h"
#include "sfx_tables.h"

MusicTrack g_audio_current_track = MUSIC_NONE;
uint8_t g_sound_enabled = 1;
static uint8_t step_counter = 0;
static uint8_t note_index = 0;

/* ── SFX layer (transcribed tracker SFX) ────────────────────────────
 * Music runs on channel 1 (see play_note / audio_update below) or via
 * hUGEDriver.  Effect sounds use channels 2 and 4 so they never collide
 * with the CH1 music voice.  Each SFX id voices the step tables in
 * generated/sfx/sfx_tables.c (transcribed from the assets-sfx tracker
 * files by tools/transcribe_sfx.py): score CH1 renders to the CH2 voice, score
 * CH4 renders verbatim.  When hUGEDriver music is active, the used music
 * channels are muted during SFX playback and unmuted at the table end. */
#define SFX_NONE 0xFF
/* Cursor state shared with the bank-6 stepper (src/audio/sfx_step.c):
 * plain WRAM globals, readable from any bank. */
/* Harness-visible trigger log (AGENTS.md 53.7: semantic, not transport):
 * per-trigger SFX telemetry would flood the 32-entry gameplay ring and
 * evict gameplay events scenarios assert on, so triggers land here
 * instead: total count + last id, read by name from host tools. */
uint16_t g_sfx_played_count = 0;
uint8_t g_sfx_last_id = SFX_NONE;
uint8_t sfx_id = SFX_NONE;
uint8_t sfx_tick = 0;
uint8_t sfx_tone_idx = 0;
uint8_t sfx_noise_idx = 0;
static uint8_t sfx_div = 0;
static uint8_t sfx_muted = 0;

extern uint8_t sfx_step_tick(void);

void audio_play_sfx(uint8_t s)
{
    uint8_t voices;

    if (!g_sound_enabled) return;
    if (s > SFX_BLOCK) return;
    sfx_id = s;
    sfx_tick = 0;
    sfx_tone_idx = 0;
    sfx_noise_idx = 0;
    sfx_div = 0;
    sfx_muted = 0;
    voices = s_sfx_voices[s];
    if (voices & 0x01) {
        huge_music_mute_channel(HT_CH2, HT_CH_MUTE);
        sfx_muted |= 0x01;
    }
    if (voices & 0x02) {
        huge_music_mute_channel(HT_CH4, HT_CH_MUTE);
        sfx_muted |= 0x02;
    }
    g_sfx_played_count++;
    g_sfx_last_id = s;
}

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

/* Battle: Vivaldi's "Summer" Presto (Four Seasons, RV 315 3rd mvt.) - legacy fallback */
static const uint8_t summer_notes[32] = {
    5, 5, 7, 7,  8, 8, 9, 9,
    9, 10, 11, 8, 12, 8, 6, 6,
    7, 7, 6, 6,  5, 5, 13, 13,
    5, 6, 7, 8,  11, 10, 9, 0
};

/* Victory: 4-note rising fanfare (D4 G4 A4 D5) + closing rest,
 * one-shot -- plays once then falls silent until the next track
 * request. */
#define VICTORY_NOTE_COUNT 5   /* 4 notes + closing rest */
#define VICTORY_TICKS_PER_NOTE 20
static const uint8_t victory_notes[VICTORY_NOTE_COUNT] = {
    1, 5, 6, 8, 0           /* D4 G4 A4 D5 rest */
};

/* Title: a slow, open modal theme (D minor-ish), sparse and mysterious.
 * The waking whale / closed-sky motif. */
static const uint8_t title_notes[16] = {
    1, 4, 6, 8,  6, 4, 1, 0,
    13, 6, 5, 3,  1, 3, 4, 0
};

/* Town: warm, comfortable major-ish melody (F/C-ish), a friendly hub. */
static const uint8_t town_notes[24] = {
    4, 5, 6, 5,  4, 1, 3, 5,
    4, 3, 4, 5,  6, 5, 4, 3,
    4, 4, 5, 6,  8, 6, 5, 0
};

/* Dungeon / castle: tense, close, minor, with an ominous downward turn. */
static const uint8_t dungeon_notes[24] = {
    2, 2, 13, 2,  4, 4, 2, 2,
    13, 13, 11, 13,  4, 2, 13, 2,
    1, 1, 2, 1,  4, 5, 4, 0
};

/* Boss: rapid, driving, urgent -- a fast ostinato against a rising answer. */
static const uint8_t boss_notes[32] = {
    2, 2, 13, 2,  4, 4, 2, 2,
    13, 13, 11, 13,  5, 5, 13, 5,
    5, 5, 4, 5,  6, 6, 5, 6,
    8, 8, 6, 8,  4, 2, 13, 0
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
    g_audio_current_track = MUSIC_NONE;
    step_counter = 0;
    note_index = 0;
    huge_music_init();

    TAC_REG = 0x00;
    TMA_REG = 0x00;
    TIMA_REG = 0x00;
    /* TACF_START | TACF_65KHZ = 0x06: 65536 Hz clock -> TIMA overflows at
     * 256 Hz (TMA=0), one audio_update() per overflow (crt0.s timer ISR).
     * 0x05 (TACF_16KHZ) would select the 262144 Hz clock -> a 1024 Hz music
     * clock, 4x too fast. */
    TAC_REG = 0x06;
}

void audio_play_music(MusicTrack track)
{
    if (g_audio_current_track == track) return;
    if (!g_sound_enabled) return;
    /* Suppress the ISR while switching: set MUSIC_NONE first so the
     * timer interrupt never sees the new track with a stale note_index
     * (which could immediately kill a one-shot like MUSIC_VICTORY). */
    g_audio_current_track = MUSIC_NONE;
    step_counter = 0;
    note_index = 0;
    huge_music_stop();
    play_note(0);

    g_audio_current_track = track;
    if (track == MUSIC_BATTLE) {
        huge_music_play(&song_battle);
    } else if (track == MUSIC_DESOLATE) {
        huge_music_play(&song_desolate_landscape);
    } else if (track == MUSIC_FOREST) {
        huge_music_play(&song_forest);
    } else if (track == MUSIC_BOSS) {
        huge_music_play(&song_boss_fight);
    } else if (track == MUSIC_TOWN) {
        huge_music_play(&song_village);
    }

    /* Centralized MUSIC_CHANGED telemetry (AGENTS.md 8): emitted only when
     * the track actually changes, so callers never forget it. */
    telemetry_emit(EVENT_MUSIC_CHANGED, track, 0, 0, 0);
}

MusicTrack audio_get_current_track(void)
{
    return g_audio_current_track;
}

/* Per-track playback parameters (indexed by MusicTrack).  Tables live
 * here in the fixed bank because the timer ISR calls audio_update()
 * directly.  len is the note count; loops wrap via mask/compare,
 * VICTORY (one_shot) falls silent after its last note. */
static const uint8_t *const s_track_notes[MUSIC_FOREST + 1] = {
    0, lacrimosa_notes, summer_notes, victory_notes,
    title_notes, town_notes, dungeon_notes, boss_notes, 0, 0
};
static const uint8_t s_track_len[MUSIC_FOREST + 1] = {
    0, 32, 32, VICTORY_NOTE_COUNT, 16, 24, 24, 32, 0, 0
};
static const uint8_t s_track_ticks[MUSIC_FOREST + 1] = {
    0, 43, 17, VICTORY_TICKS_PER_NOTE, 60, 40, 30, 12, 0, 0
};

void audio_update(void)
{
    const uint8_t *notes;
    uint8_t len;

#ifdef DEBUG_BUILD
    g_audio_ticks++;
#endif

    /* Step the transcribed SFX at the 64 Hz tracker rate through the
     * bank-6 stepper body (same inline select-6/call/restore-1 discipline
     * as huge_music_update, which also runs in this ISR). At the table
     * end the used voices are silenced and their music channels unmuted. */
    if (sfx_id <= SFX_BLOCK) {
        if (++sfx_div >= 4) {
            sfx_div = 0;
            *(volatile uint8_t *)0x2000 = HUGE_MUSIC_BANK;
            if (sfx_step_tick()) {
                if (sfx_muted & 0x01) {
                    NR22_REG = 0x00;
                    huge_music_mute_channel(HT_CH2, HT_CH_PLAY);
                }
                if (sfx_muted & 0x02) {
                    NR42_REG = 0x00;
                    huge_music_mute_channel(HT_CH4, HT_CH_PLAY);
                }
                sfx_muted = 0;
                sfx_id = SFX_NONE;
            }
            *(volatile uint8_t *)0x2000 = 1;
        }
    }

    if (g_audio_current_track == MUSIC_NONE) return;

    if (huge_music_is_playing()) {
        huge_music_update();
        return;
    }

    notes = s_track_notes[g_audio_current_track];
    if (!notes) return;
    len = s_track_len[g_audio_current_track];

    if (++step_counter >= s_track_ticks[g_audio_current_track]) {
        step_counter = 0;
        if (note_index < len) {
            play_note(s_note_freqs[notes[note_index]]);
            note_index++;
            if (note_index >= len) {
                if (g_audio_current_track == MUSIC_VICTORY) {
                    /* One-shot jingle: silence until the next track
                     * request. */
                    g_audio_current_track = MUSIC_NONE;
                }
                note_index = 0;
            }
        }
    }
}
