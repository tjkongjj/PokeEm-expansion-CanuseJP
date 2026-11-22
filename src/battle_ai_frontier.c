#include "global.h"
#include "battle.h"
#include "battle_ai_frontier.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_dynamax.h"
#include "battle_gimmick.h"
#include "battle_script_commands.h"
#include "battle_setup.h"
#include "battle_util.h"
#include "move.h"
#include "constants/abilities.h"
#include "constants/battle_ai.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_set_effect.h"
#include "constants/hold_effects.h"

// Damage the opponent can deal to battlerAtk this turn with its best move.
static u32 GetBestDamageTaken(enum BattlerId battlerAtk, enum BattlerId battlerDef)
{
    enum Move *moves = GetMovesArray(battlerDef);
    u32 best = 0;

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        if (IsMoveUnusable(i, moves[i], gAiLogicData->moveLimitations[battlerDef]))
            continue;
        best = max(best, AI_GetDamage(battlerDef, battlerAtk, i, AI_DEFENDING, gAiLogicData));
    }
    return best;
}

static bool32 IsAiFasterWith(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move)
{
    return AI_IsFaster(battlerAtk, battlerDef, move, GetPredictedMove(battlerAtk, battlerDef, gAiLogicData), CONSIDER_PRIORITY);
}

// Attacking every turn knocks the target out before the AI goes down.
static bool32 WinsDamageRace(enum BattlerId battlerAtk, enum BattlerId battlerDef, u32 taken)
{
    enum Move *moves = GetMovesArray(battlerAtk);
    u32 best = 0, bestIndex = 0;

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        u32 damage;
        if (IsMoveUnusable(i, moves[i], gAiLogicData->moveLimitations[battlerAtk]))
            continue;
        damage = AI_GetDamage(battlerAtk, battlerDef, i, AI_ATTACKING, gAiLogicData);
        if (damage > best)
        {
            best = damage;
            bestIndex = i;
        }
    }

    if (best == 0)
        return FALSE;
    if (taken == 0)
        return TRUE;

    u32 hitsToWin = DIV_ROUND_UP(gBattleMons[battlerDef].hp, best);
    u32 hitsToLose = DIV_ROUND_UP(gBattleMons[battlerAtk].hp, taken);
    if (IsAiFasterWith(battlerAtk, battlerDef, moves[bestIndex]))
        return hitsToWin <= hitsToLose;
    return hitsToWin < hitsToLose;
}

static u32 GetRecoveryAmount(enum BattlerId battlerAtk, enum Move move)
{
    u32 maxHP = gBattleMons[battlerAtk].maxHP;
    u32 weather = AI_GetWeather();
    u32 percent;

    switch (GetMoveEffect(move))
    {
    case EFFECT_RESTORE_HP:
    case EFFECT_SOFTBOILED:
    case EFFECT_ROOST:
        percent = 50;
        break;
    case EFFECT_MORNING_SUN:
    case EFFECT_SYNTHESIS:
    case EFFECT_MOONLIGHT:
        if (weather & B_WEATHER_SUN)
            percent = 66;
        else if (weather != B_WEATHER_NONE)
            percent = 25;
        else
            percent = 50;
        break;
    case EFFECT_SHORE_UP:
        percent = (weather & B_WEATHER_SANDSTORM) ? 66 : 50;
        break;
    case EFFECT_LIFE_DEW:
    case EFFECT_JUNGLE_HEALING:
        percent = 25;
        break;
    default:
        return 0; // Rest, drain moves and other recovery are judged by the usual AI.
    }

    if (gBattleMons[battlerAtk].volatiles.healBlockTimer)
        return 0;
    return min(maxHP * percent / 100, maxHP - gBattleMons[battlerAtk].hp);
}

// Recover when healing outpaces the opponent's damage and the AI survives to heal.
static s32 ScoreRecoveryRace(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, s32 score)
{
    u32 hp = gBattleMons[battlerAtk].hp;
    u32 heal = GetRecoveryAmount(battlerAtk, move);
    u32 taken;

    if (heal == 0 || hp * 100 > gBattleMons[battlerAtk].maxHP * 60)
        return score;
    if (CanAIFaintTarget(battlerAtk, battlerDef, 0))
        return score; // Take the KO instead.

    taken = GetBestDamageTaken(battlerAtk, battlerDef);
    if (IsAiFasterWith(battlerAtk, battlerDef, move) ? hp + heal <= taken : hp <= taken)
        return score; // The heal would come too late.
    if (WinsDamageRace(battlerAtk, battlerDef, taken))
        return score; // Keep attacking; healing only gives the target more turns.

    if (heal > taken)
        ADJUST_SCORE(BEST_EFFECT); // Each exchange leaves the AI healthier.
    else if (heal * 2 > taken)
        ADJUST_SCORE(WEAK_EFFECT);
    return score;
}

// Stall a Dynamaxed opponent: a Substitute set up first soaks its Max Moves.
static s32 ScoreAgainstDynamax(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, s32 score)
{
    if (GetActiveGimmick(battlerDef) != GIMMICK_DYNAMAX || GetActiveGimmick(battlerAtk) == GIMMICK_DYNAMAX)
        return score;

    switch (GetMoveEffect(move))
    {
    case EFFECT_SUBSTITUTE:
        if (!gBattleMons[battlerAtk].volatiles.substitute
         && gBattleMons[battlerAtk].hp > gBattleMons[battlerAtk].maxHP / 4
         && IsAiFasterWith(battlerAtk, battlerDef, move))
            ADJUST_SCORE(BEST_EFFECT);
        break;
    default:
        break;
    }
    return score;
}

static bool32 DealsIndirectDamage(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move)
{
    enum Ability defAbility = gAiLogicData->abilities[battlerDef];
    enum Move partnerMove = MOVE_NONE;

    switch (GetMoveEffect(move))
    {
    case EFFECT_NON_VOLATILE_STATUS:
        switch (GetMoveNonVolatileStatus(move))
        {
        case MOVE_EFFECT_POISON:
        case MOVE_EFFECT_TOXIC:
            return AI_CanPoison(battlerAtk, battlerDef, defAbility, move, partnerMove);
        case MOVE_EFFECT_BURN:
            return AI_CanBurn(battlerAtk, battlerDef, defAbility, GetPartnerBattler(battlerAtk), move, partnerMove);
        default:
            return FALSE;
        }
    case EFFECT_LEECH_SEED:
        return !gBattleMons[battlerDef].volatiles.leechSeed && !IS_BATTLER_OF_TYPE(battlerDef, TYPE_GRASS);
    case EFFECT_WEATHER:
        return (GetMoveWeatherType(move) == BATTLE_WEATHER_SANDSTORM || GetMoveWeatherType(move) == BATTLE_WEATHER_HAIL)
            && !(AI_GetWeather() & B_WEATHER_DAMAGING_ANY);
    case EFFECT_STEALTH_ROCK:
        return !IsHazardOnSide(GetBattlerSide(battlerDef), HAZARDS_STEALTH_ROCK);
    case EFFECT_ROAR:
        // Forcing out a Wonder Guard user hurts it on hazards and brings in a hittable target.
        return CountUsablePartyMons(battlerDef) > 0;
    default:
        return FALSE;
    }
}

// Wonder Guard only blocks direct hits that aren't super effective. Expect the target to
// Terastallize away its weaknesses, so prefer status, weather, hazards and phazing.
static s32 ScoreAgainstWonderGuard(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, s32 score)
{
    bool32 hasSuperEffective = FALSE;
    bool32 canTera = gBattleStruct->gimmick.usableGimmick[battlerDef] == GIMMICK_TERA;

    if (gAiLogicData->abilities[battlerDef] != ABILITY_WONDER_GUARD)
        return score;

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move aiMove = gBattleMons[battlerAtk].moves[i];
        if (!IsBattleMoveStatus(aiMove) && !IsMoveUnusable(i, aiMove, gAiLogicData->moveLimitations[battlerAtk])
         && gAiLogicData->effectiveness[battlerAtk][battlerDef][i] >= UQ_4_12(2.0))
            hasSuperEffective = TRUE;
    }

    if (DealsIndirectDamage(battlerAtk, battlerDef, move))
    {
        if (!hasSuperEffective || canTera)
            ADJUST_SCORE(BEST_EFFECT);
        else
            ADJUST_SCORE(DECENT_EFFECT);
    }
    else if (!hasSuperEffective && IsBattleMoveStatus(move) && IsStatRaisingMove(move))
    {
        ADJUST_SCORE(AWFUL_EFFECT); // Boosts are wasted while no attack can land.
    }
    return score;
}

static bool32 IsStatusMoveBlockedBySubstitute(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move)
{
    enum Ability ability = gAiLogicData->abilities[battlerAtk];

    if (GetMoveEffect(move) == EFFECT_TRANSFORM && GetConfig(B_TRANSFORM_SUBSTITUTE_FAIL) < GEN_5)
        return FALSE;
    if (!WouldSubstituteBlockMove(battlerAtk, battlerDef, ability, move))
        return FALSE;

    switch (AI_GetBattlerMoveTargetType(battlerAtk, move))
    {
    case TARGET_SELECTED:
    case TARGET_OPPONENT:
        return TRUE;
    case TARGET_RANDOM:
    case TARGET_BOTH:
        // A spread or randomly targeted move can still reach the other foe.
        return !HasTwoOpponents(battlerAtk)
            || WouldSubstituteBlockMove(battlerAtk, GetPartnerBattler(battlerDef), ability, move);
    default:
        return FALSE;
    }
}

static s32 ScoreSubstituteDamage(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, s32 score)
{
    u32 subHP = gBattleMons[battlerDef].volatiles.substituteHP;

    if (subHP != 0 && IsBattle1v1() && !IsBattleMoveStatus(move))
    {
        u32 slot = gAiThinkingStruct->movesetIndex;
        u32 damage = AI_GetDamage(battlerAtk, battlerDef, slot, AI_ATTACKING, gAiLogicData);

        if (damage == 0)
            return score;
        if (!WouldSubstituteBlockMove(battlerAtk, battlerDef, gAiLogicData->abilities[battlerAtk], move))
            ADJUST_SCORE(damage >= subHP ? BEST_EFFECT : WEAK_EFFECT);
        else if (damage >= subHP)
            ADJUST_SCORE(DECENT_EFFECT);
    }
    return score;
}

s32 AI_FrontierExpert(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, s32 score)
{
    if (IsTargetingPartner(battlerAtk, battlerDef))
        return score;

    if (IsBattleMoveStatus(move)
     && ((GetMoveEffect(move) == EFFECT_SUBSTITUTE && gBattleMons[battlerAtk].volatiles.substitute)
      || IsStatusMoveBlockedBySubstitute(battlerAtk, battlerDef, move)))
        ADJUST_AND_RETURN_SCORE(NO_DAMAGE_OR_FAILS);

    score = ScoreRecoveryRace(battlerAtk, battlerDef, move, score);
    score = ScoreAgainstDynamax(battlerAtk, battlerDef, move, score);
    score = ScoreAgainstWonderGuard(battlerAtk, battlerDef, move, score);
    return ScoreSubstituteDamage(battlerAtk, battlerDef, move, score);
}

static bool32 ShouldSaveDynamaxOnAttack(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move)
{
    if (!IsBattle1v1() || !IsBattlerAlive(battlerDef) || IsBattlerAlly(battlerAtk, battlerDef))
        return FALSE;

    // A Max Move cannot help against a type immunity. Check the converted move,
    // since its type interactions can differ from the original attack's.
    if (AI_GetMoveEffectiveness(GetMaxMove(battlerAtk, move), battlerAtk, battlerDef) == 0)
        return TRUE;

    // Take a safe KO without spending Dynamax. Leave special attacks and
    // situations in which the extra HP could matter to the usual decision.
    if (GetMoveEffect(move) != EFFECT_HIT
     || MoveHasAdditionalEffect(move, MOVE_EFFECT_RECHARGE)
     || gBattleMons[battlerDef].volatiles.substitute
     || CanEndureHit(battlerAtk, battlerDef, move)
     || !IsAiFasterWith(battlerAtk, battlerDef, move)
     || CanTargetFaintAi(battlerDef, battlerAtk))
        return FALSE;

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        if (gBattleMons[battlerAtk].moves[i] == move
         && gAiLogicData->moveAccuracy[battlerAtk][battlerDef][i] >= 100)
        {
            uq4_12_t effectiveness;
            struct SimulatedDamage damage = AI_CalcDamageSaveBattlers(move, battlerAtk, battlerDef, &effectiveness, GIMMICK_NONE, GIMMICK_NONE);
            return damage.minimum >= gBattleMons[battlerDef].hp;
        }
    }
    return FALSE;
}

void FrontierAI_ReconsiderGimmick(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move)
{
    enum Gimmick gimmick = gBattleStruct->gimmick.usableGimmick[battlerAtk];
    bool32 threatened;

    if (gimmick != GIMMICK_DYNAMAX && gimmick != GIMMICK_TERA)
        return;
    if (!IsBattleMoveStatus(move))
    {
        if (gimmick == GIMMICK_DYNAMAX
         && (gAiThinkingStruct->aiFlags[battlerAtk] & AI_FLAG_SMART_DYNAMAX)
         && ShouldSaveDynamaxOnAttack(battlerAtk, battlerDef, move))
            SetAIUsingGimmick(battlerAtk, NO_GIMMICK);
        return;
    }

    threatened = CanTargetFaintAi(battlerDef, battlerAtk);
    // While Dynamaxed, status moves become Max Guard, and a Tera type does nothing for
    // a status move. Set up first and save the gimmick for an attacking turn, unless
    // it is needed now to survive (Dynamax HP or the Tera type's resistances).
    if (!threatened || gimmick == GIMMICK_DYNAMAX)
    {
        if (gimmick == GIMMICK_DYNAMAX && threatened && CountUsablePartyMons(battlerAtk) == 0)
            return; // Last Pokémon: Dynamax to survive rather than set up.
        SetAIUsingGimmick(battlerAtk, NO_GIMMICK);
    }
}
