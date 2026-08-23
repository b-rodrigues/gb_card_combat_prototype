#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_DIALOGUE_LINES 8

/* Dialogue identifiers.  The engine defines only the NONE sentinel and the
 * start of the per-game content range; the game names its dialogues in
 * src/game/game_ids.h. */
typedef uint8_t DialogueId;

#define DIALOGUE_ID_NONE       0
#define DIALOGUE_ID_FIRST_GAME 0x80

typedef struct {
    DialogueId id;
    const char *speaker;
    uint8_t line_count;
    const char *lines[MAX_DIALOGUE_LINES];
    uint8_t completion_flag;   /* FlagId set when this dialogue ends (0 = none) */
} DialogueDefinition;

typedef struct {
    bool active;
    DialogueId id;
    uint8_t current_line;
    uint8_t line_count;
    const char *speaker;
    const char *lines[MAX_DIALOGUE_LINES];
    uint8_t completion_flag;
} DialogueState;

void dialogue_init(DialogueState *d);
void dialogue_register(const DialogueDefinition *table, uint8_t count, uint8_t bank);
void dialogue_start_def(DialogueState *d, DialogueId id);
bool dialogue_next(DialogueState *d);
DialogueId dialogue_end(DialogueState *d);

#endif /* DIALOGUE_H */

/* Registered dialogue table location + staging buffers: read/written by the
 * bank-2 start body (src/core/dialogue_banked.c). */
extern const DialogueDefinition *g_dialogue_table;
extern uint8_t g_dialogue_table_count;
extern char g_dlg_speaker[12];
extern char g_dlg_lines[MAX_DIALOGUE_LINES][21];
void dialogue_start_def_banked(void);
