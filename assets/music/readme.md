# Game Boy Tracker Music (`assets/music/`)

Music by **Putosaure**.

This directory holds the source `.uge` (hUGETracker) files for the Game Boy RPG soundtrack.

---

## 1. Tracker & Tooling

* **Tracker**: [hUGETracker](https://nickfa.ro/huge-tracker/) (v1.0.11+).
* **CLI Converter**: `uge2source` (packaged in the Nix environment).
* **Driver**: [hUGEDriver](https://github.com/SuperDisk/hUGEDriver) (v1.0+).
* **Object Bridge**: `tools/rgb2sdas.py` (converts RGBDS 0.10.x objects to SDAS format for GBDK-4 linking).

All tools are provided deterministically via `nix develop`.

---

## 2. Audio Pipeline Architecture

```
assets/music/*.uge
       │
       ▼ (uge2source -b 6 <symbol>)
generated/music/*.c  [#pragma bank 6]
       │
       ▼ (lcc)
build/debug/music/*.o  [ROM Bank 6]
       ▲
       │ (rgbasm-huge + rgb2sdas -b 6)
lib/hUGEDriver/src/hUGEDriver.asm ────────┘
```

### Key Components

1. **ROM Bank 6 Placement**:
   * All converted `.uge` songs and the hUGEDriver engine code reside in **ROM Bank 6** (`_CODE_6`).
   * This strictly preserves the fixed 32 KB Bank 0/1 memory budget (`_CODE`/`_HOME`).
2. **Timer-Driven Tick Rate (64 Hz)**:
   * Game audio runs on the hardware **Timer interrupt** (TIMA overflow at 256 Hz, see `AGENTS.md` §35).
   * `audio_update()` in `src/audio/audio.c` divides the 256 Hz timer clock by 4 to tick `hUGE_dosound()` at a steady **64 Hz** (`huge_music_update()`), regardless of visual rendering, screen wipes, or CPU load.
3. **Interrupt & Bank Safety**:
   * API calls (`huge_music_play`, `huge_music_mute_channel`, `huge_music_stop`) are critical sections (`__critical`) that switch to Bank 6, perform driver operations, and immediately restore the home bank (`HOME_BANK 1`).
4. **SFX Coexistence (Channel Muting)**:
   * When tone sound effects (CH2) or noise bursts (CH4) play, the game calls `huge_music_mute_channel(HT_CH2 / HT_CH4, HT_CH_MUTE)`.
   * When the SFX envelope finishes, the channel is unmuted (`HT_CH_PLAY`) so the tracker music resumes cleanly without channel interference or clicks.

---

## 3. Adding or Updating Tracks

1. **Authoring**:
   * Create or modify `.uge` tracks using `hUGETracker` (or launch via `nix develop --command hUGETracker`).
   * Respect standard Game Boy hardware channels:
     * **CH1**: Pulse with frequency sweep (lead melody)
     * **CH2**: Pulse (harmony / chords; shared with tone SFX)
     * **CH3**: Custom 4-bit Waveforms (basslines)
     * **CH4**: Noise channel (percussion / drums; shared with noise SFX)
2. **Build Rule**:
   * Save the file under `assets/music/<track_name>.uge`.
   * In the `Makefile`, add a rule to generate the C source into `generated/music/`:
     ```makefile
     $(GENERATED_MUSIC_DIR)/<track_name>.c: assets/music/<track_name>.uge | $(GENERATED_MUSIC_DIR)
     	$(UGE2SOURCE) "$<" -b 6 song_<track_name> "$@"
     ```
3. **C Code Binding**:
   * Declare the song in `src/audio/huge_music_data.h`:
     ```c
     extern const hUGESong_t song_<track_name>;
     ```
   * Hook into `audio_play_music()` in `src/audio/audio.c` using `huge_music_play(&song_<track_name>)`.

