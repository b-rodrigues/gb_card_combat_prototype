#pragma bank 2

#include "dialogue.h"
#include "banked.h"

/* ── Banked dialogue start (src/core/dialogue.c dispatcher) ─────────
 * Scans the registered dialogue table DIRECTLY (same bank -- no
 * banked_copy, no per-row scratch) and stages speaker + lines into the
 * WRAM buffers.  Sets d->active on success; the fixed-bank wrapper emits
 * DIALOGUE_STARTED afterwards.  All pointers stored into DialogueState
 * target WRAM buffers, never bank 2 rodata. */

void dialogue_start_def_banked(void)
{
    DialogueState *d = (DialogueState *)g_bk_ptr_a;
    DialogueId id = g_bk_byte_a;
    uint8_t i;
    const DialogueDefinition *def = (const DialogueDefinition *)0;

    if (!d) return;

    for (i = 0; i < g_dialogue_table_count; i++) {
        if (g_dialogue_table[i].id == id) {
            def = &g_dialogue_table[i];
            break;
        }
    }
    if (!def) { g_dlg_speaker[0] = 0; return; }

    d->active = true;
    d->id = def->id;
    d->current_line = 0;
    d->line_count = (def->line_count > MAX_DIALOGUE_LINES) ? MAX_DIALOGUE_LINES : def->line_count;
    d->completion_flag = def->completion_flag;

    /* Speaker: NULL means "no named speaker" -- stage an empty WRAM string
     * (never a bank-2 literal address; DialogueState is read later from the
     * fixed bank).  Lines below keep their own NULL-tolerant check. */
    if (def->speaker) {
        for (i = 0; i < 11 && def->speaker[i]; i++) {
            g_dlg_speaker[i] = def->speaker[i];
        }
        g_dlg_speaker[i] = 0;
        d->speaker = g_dlg_speaker;
    } else {
        g_dlg_speaker[0] = 0;
        d->speaker = g_dlg_speaker;
    }

    for (i = 0; i < MAX_DIALOGUE_LINES; i++) {
        uint8_t j;
        const char *src = (i < d->line_count) ? def->lines[i] : (const char *)0;
        for (j = 0; j < 20 && src && src[j]; j++) g_dlg_lines[i][j] = src[j];
        g_dlg_lines[i][j] = 0;
        d->lines[i] = g_dlg_lines[i];
    }
}
