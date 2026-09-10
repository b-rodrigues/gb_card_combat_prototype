#include "huge_music.h"

static const hUGESong_t *s_current_huge_song = 0;
static uint8_t s_huge_playing = 0;
static uint8_t s_huge_tick_divider = 0;
/* ROM bank holding the active song + its driver copy.  Bank 6 holds the
 * driver and the six original songs; bank 7 holds the second driver copy
 * and the mimic song (bank 6 is full). */
static uint8_t s_huge_music_bank = HUGE_MUSIC_BANK;

void huge_music_init(void)
{
    s_current_huge_song = 0;
    s_huge_playing = 0;
    s_huge_tick_divider = 0;
    s_huge_music_bank = HUGE_MUSIC_BANK;
}

void huge_music_play(const hUGESong_t *song)
{
    huge_music_play_banked(song, HUGE_MUSIC_BANK);
}

void huge_music_play_banked(const hUGESong_t *song, uint8_t bank)
{
    if (!song) {
        huge_music_stop();
        return;
    }

    __critical {
        s_huge_playing = 0;
        s_current_huge_song = song;
        s_huge_music_bank = bank;
        *(volatile uint8_t *)0x2000 = bank;
        if (bank == HUGE_MUSIC_BANK_B7) {
            hUGE_init_b7(song);
        } else {
            hUGE_init(song);
        }
        *(volatile uint8_t *)0x2000 = 1;
        s_huge_tick_divider = 0;
        s_huge_playing = 1;
    }
}

void huge_music_stop(void)
{
    __critical {
        s_huge_playing = 0;
        s_current_huge_song = 0;
        /* Silence active sound channels */
        NR12_REG = 0x00;
        NR14_REG = 0x80;
        NR22_REG = 0x00;
        NR24_REG = 0x80;
        NR32_REG = 0x00;
        NR42_REG = 0x00;
        NR44_REG = 0x80;
    }
}

void huge_music_pause(void)
{
    s_huge_playing = 0;
}

void huge_music_resume(void)
{
    if (s_current_huge_song) {
        s_huge_playing = 1;
    }
}

uint8_t huge_music_is_playing(void)
{
    return s_huge_playing;
}

void huge_music_mute_channel(uint8_t ch, uint8_t mute)
{
    __critical {
        *(volatile uint8_t *)0x2000 = s_huge_music_bank;
        if (s_huge_music_bank == HUGE_MUSIC_BANK_B7) {
            hUGE_mute_channel_b7((enum hUGE_channel_t)ch, (enum hUGE_mute_t)mute);
        } else {
            hUGE_mute_channel((enum hUGE_channel_t)ch, (enum hUGE_mute_t)mute);
        }
        *(volatile uint8_t *)0x2000 = 1;
    }
}

void huge_music_mute_channel_isr(uint8_t ch, uint8_t mute)
{
    /* ISR-context variant of huge_music_mute_channel: NO __critical.
     * SDCC's __critical emits di/ei -- the ei() would re-enable nested
     * timer interrupts while this already runs inside the timer ISR
     * (256 Hz), stacking ISR frames until the WRAM below the stack is
     * smashed (observed as ghost joypad input: pad_state/prev_pad_state
     * accumulate pressed bits and releases never register).  Inside the
     * ISR no user code can run concurrently, so no di is needed either;
     * just switch banks, call the driver, restore home bank. */
    *(volatile uint8_t *)0x2000 = s_huge_music_bank;
    if (s_huge_music_bank == HUGE_MUSIC_BANK_B7) {
        hUGE_mute_channel_b7((enum hUGE_channel_t)ch, (enum hUGE_mute_t)mute);
    } else {
        hUGE_mute_channel((enum hUGE_channel_t)ch, (enum hUGE_mute_t)mute);
    }
    *(volatile uint8_t *)0x2000 = 1;
}

void huge_music_update(void)
{
    if (!s_huge_playing) return;

    /* 256 Hz TIMA timer -> 64 Hz tracker tick (every 4 timer interrupts) */
    if (++s_huge_tick_divider >= 4) {
        s_huge_tick_divider = 0;
        *(volatile uint8_t *)0x2000 = s_huge_music_bank;
        if (s_huge_music_bank == HUGE_MUSIC_BANK_B7) {
            hUGE_dosound_b7();
        } else {
            hUGE_dosound();
        }
        *(volatile uint8_t *)0x2000 = 1;
    }
}
