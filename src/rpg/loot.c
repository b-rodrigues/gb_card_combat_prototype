#include "rpg/loot.h"

/* Fixed-bank home for the loot module's shared WRAM state only.
 *
 * Everything else about drops lives in bank 2
 * (src/game/loot_drop_banked.c -- gate/profile/roll) or behind the
 * synth trampoline dispatched by card_get_def() (src/rpg/loot_banked.c
 * -- identity tables).  The fixed bank sits hard against the 0x8000
 * boundary (AGENTS.md 55.5): overflowing it corrupts bank 2 and hangs
 * the guest, so the drop path deliberately costs almost no fixed code.
 */

/* Derived-id result byte: written by the bank-2 drop/synth bodies,
 * read by the fixed-bank wrappers. */
uint8_t g_loot_id;

/* Isolated loot RNG state.  Lives in fixed-bank _DATA (WRAM, always
 * mapped, initialized by CRT0 -- which also runs under the harness,
 * which breaks at main) so both bank-2 bodies advance one deterministic
 * stream inline, exactly like g_rng_state (src/debug/rng.h).  Seeded
 * from the main seed by rng_set_seed() (src/debug/rng.c); victory
 * drops must NOT consume the shared game RNG (docs/loot.md §34.5). */
uint16_t g_loot_rng_state = 0x1234;
