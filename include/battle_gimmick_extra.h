#ifndef GUARD_BATTLE_GIMMICK_EXTRA_H
#define GUARD_BATTLE_GIMMICK_EXTRA_H

#include "battle_gimmick.h"

bool32 HasBattlerUsedAnyGimmick(enum BattlerId battler);
bool32 CanUseSelectedGimmickWithMove(enum BattlerId battler, enum Move move);
void ClearGimmickAsActivated(enum BattlerId battler, enum Gimmick gimmick);

#endif // GUARD_BATTLE_GIMMICK_EXTRA_H
