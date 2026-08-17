#include "rpg/progression.h"
#include "telemetry.h"
#include <stddef.h>

/* ── Static progression definitions (rules), separate from state ─────
 * Fixed lookup tables (Game Boy friendly).  Threshold entries are the
 * progress needed to advance FROM level N to level N+1. */
static const uint16_t g_hero_thresholds[] = {100, 250, 450, 700, 1000, 1400, 1900, 2500, 3200, 4000};
static const uint16_t g_weapon_thresholds[] = {50, 120, 220, 350, 520, 750, 1020};
static const uint16_t g_companion_thresholds[] = {60, 150, 280, 450, 660, 900, 1200};

static const ProgressionDefinition g_progression_defs[] = {
    { 11, g_hero_thresholds, 10 },
    { 8,  g_weapon_thresholds, 7 },
    { 8,  g_companion_thresholds, 7 }
};

const ProgressionDefinition *progression_get_def(uint8_t target_type)
{
    if (target_type >= 1 && target_type <= 3) {
        return &g_progression_defs[target_type - 1];
    }
    return NULL;
}

ProgressionState *progression_get(GameState *state, uint8_t target_type, uint16_t target_id)
{
    uint8_t i;
    if (!state || target_type == PROG_TYPE_NONE) return NULL;
    for (i = 0; i < state->progression.count; i++) {
        if (state->progression.entries[i].target.type == target_type &&
            state->progression.entries[i].target.id == target_id) {
            return &state->progression.entries[i].state;
        }
    }
    return NULL;
}

bool progression_ensure(GameState *state, uint8_t target_type, uint16_t target_id,
                        uint8_t level, uint16_t progress)
{
    ProgressionState *ps;
    if (!state || target_type == PROG_TYPE_NONE) return false;

    ps = progression_get(state, target_type, target_id);
    if (!ps) {
        if (state->progression.count >= MAX_PROGRESSION_TARGETS) return false;
        ps = &state->progression.entries[state->progression.count].state;
        state->progression.entries[state->progression.count].target.type = target_type;
        state->progression.entries[state->progression.count].target.id = target_id;
        state->progression.count++;
    }
    ps->level = level;
    ps->progress = progress;
    return true;
}

bool progression_add(GameState *state, uint8_t target_type, uint16_t target_id,
                     uint16_t amount, ProgressionAddResult *out_result)
{
    const ProgressionDefinition *def;
    ProgressionState *ps;
    uint8_t i;
    uint8_t level_before;

    if (out_result) {
        out_result->level_before = 0;
        out_result->level_after = 0;
        out_result->crossed = false;
    }
    if (!state || target_type == PROG_TYPE_NONE) return false;

    def = progression_get_def(target_type);
    if (!def) return false;

    ps = progression_get(state, target_type, target_id);
    if (!ps) {
        if (state->progression.count >= MAX_PROGRESSION_TARGETS) return false;
        ps = &state->progression.entries[state->progression.count].state;
        state->progression.entries[state->progression.count].target.type = target_type;
        state->progression.entries[state->progression.count].target.id = target_id;
        state->progression.count++;
        ps->level = 1;
        ps->progress = 0;
    }

    level_before = ps->level;
    ps->progress = (uint16_t)(ps->progress + amount);
    telemetry_emit(EVENT_PROGRESSION_GAINED, target_type,
                   (uint8_t)(target_id & 0xFF),
                   (uint8_t)(amount & 0xFF), ps->level);

    for (i = ps->level; i < def->max_level && i <= def->threshold_count; i++) {
        if (ps->progress >= def->thresholds[i - 1]) {
            ps->progress = (uint16_t)(ps->progress - def->thresholds[i - 1]);
            ps->level++;
            telemetry_emit(EVENT_LEVEL_UP, target_type,
                           (uint8_t)(target_id & 0xFF), ps->level, 0);
        } else {
            break;
        }
    }

    if (out_result) {
        out_result->level_before = level_before;
        out_result->level_after = ps->level;
        out_result->crossed = (ps->level != level_before);
    }
    return (ps->level != level_before);
}
