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

ProgressionState *progression_get(GameState *state, ProgressionTarget target)
{
    uint8_t i;
    if (!state || target.type == PROG_TYPE_NONE) return NULL;
    for (i = 0; i < state->progression.count; i++) {
        if (state->progression.entries[i].target.type == target.type &&
            state->progression.entries[i].target.id == target.id) {
            return &state->progression.entries[i].state;
        }
    }
    return NULL;
}

bool progression_ensure(GameState *state, ProgressionTarget target,
                        uint8_t level, uint16_t progress)
{
    ProgressionState *ps;
    if (!state || target.type == PROG_TYPE_NONE) return false;

    ps = progression_get(state, target);
    if (!ps) {
        if (state->progression.count >= MAX_PROGRESSION_TARGETS) return false;
        ps = &state->progression.entries[state->progression.count].state;
        state->progression.entries[state->progression.count].target = target;
        state->progression.count++;
    }
    ps->level = level;
    ps->progress = progress;
    return true;
}

ProgressionAddResult progression_add(GameState *state, ProgressionTarget target,
                                     uint16_t amount)
{
    ProgressionAddResult result;
    const ProgressionDefinition *def;
    ProgressionState *ps;
    uint8_t i;

    result.level_before = 0;
    result.level_after = 0;
    result.crossed = false;
    if (!state || target.type == PROG_TYPE_NONE) return result;

    def = progression_get_def(target.type);
    if (!def) return result;

    ps = progression_get(state, target);
    if (!ps) {
        if (state->progression.count >= MAX_PROGRESSION_TARGETS) return result;
        ps = &state->progression.entries[state->progression.count].state;
        state->progression.entries[state->progression.count].target = target;
        state->progression.count++;
        ps->level = 1;
        ps->progress = 0;
    }

    result.level_before = ps->level;
    ps->progress = (uint16_t)(ps->progress + amount);
    telemetry_emit(EVENT_PROGRESSION_GAINED, (uint8_t)target.type,
                   (uint8_t)(target.id & 0xFF),
                   (uint8_t)(amount & 0xFF), ps->level);

    for (i = ps->level; i < def->max_level && i <= def->threshold_count; i++) {
        if (ps->progress >= def->thresholds[i - 1]) {
            ps->progress = (uint16_t)(ps->progress - def->thresholds[i - 1]);
            ps->level++;
            telemetry_emit(EVENT_LEVEL_UP, (uint8_t)target.type,
                           (uint8_t)(target.id & 0xFF), ps->level, 0);
        } else {
            break;
        }
    }

    result.level_after = ps->level;
    result.crossed = (result.level_after != result.level_before);
    return result;
}
