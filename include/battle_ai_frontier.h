#ifndef GUARD_BATTLE_AI_FRONTIER_H
#define GUARD_BATTLE_AI_FRONTIER_H

// Extra judgement for the さいきょう setting (AI_FLAG_FRONTIER_EXPERT).
s32 AI_FrontierExpert(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, s32 score);
// Called after a move was chosen; may hold Dynamax or Terastallization for a later turn.
void FrontierAI_ReconsiderGimmick(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move);

#endif // GUARD_BATTLE_AI_FRONTIER_H
