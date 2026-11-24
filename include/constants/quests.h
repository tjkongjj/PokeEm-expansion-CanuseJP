#ifndef GUARD_CONSTANTS_QUESTS_H
#define GUARD_CONSTANTS_QUESTS_H

// Quest system ported from Pokémon Dreamstone Mysteries by dsmyst
// (https://github.com/dsmyst/dreamstone-mysteries).
// Quest system design by Skeli; decomp implementation by pkmnsnfrm.

//questmenu scripting command params
#define QUEST_MENU_OPEN                 0   //opens the quest menu (questId = 0)
#define QUEST_MENU_UNLOCK_QUEST         1   //questId = QUEST_X (0-indexed)
#define QUEST_MENU_SET_ACTIVE           2   //questId = QUEST_X (0-indexed)
#define QUEST_MENU_SET_REWARD           3   //questId = QUEST_X (0-indexed)
#define QUEST_MENU_COMPLETE_QUEST       4   //questId = QUEST_X (0-indexed)
#define QUEST_MENU_CHECK_UNLOCKED       5   //checks if questId has been unlocked. Returns result to gSpecialVar_Result
#define QUEST_MENU_CHECK_INACTIVE       6   //check if a questID is inactive. Returns result to gSpecialVar_Result
#define QUEST_MENU_CHECK_ACTIVE         7   //checks if questId has been unlocked. Returns result to gSpecialVar_Result
#define QUEST_MENU_CHECK_REWARD         8   //checks if questId is in Reward state. Returns result to gSpecialVar_Result
#define QUEST_MENU_CHECK_COMPLETE       9   //checks if questId has been completed. Returns result to gSpecialVar_Result
#define QUEST_MENU_BUFFER_QUEST_NAME    10  //buffers a quest name to gStringVar1

// quest number defines
// Only a placeholder quest for now; real quest content is added by
// appending new QUEST_* ids above QUEST_COUNT and entries to
// sSideQuests in src/quests.c.
#define QUEST_TEST                      0
#define QUEST_COUNT                     (QUEST_TEST + 1)

#define SUB_QUEST_1                     0   //QUEST_TEST
#define SUB_QUEST_2                     1   //QUEST_TEST

#define QUEST_1_SUB_COUNT 2
#define SUB_QUEST_COUNT (QUEST_1_SUB_COUNT)

#define QUEST_ARRAY_COUNT (SUB_QUEST_COUNT > QUEST_COUNT ? SUB_QUEST_COUNT : QUEST_COUNT)
#endif // GUARD_CONSTANTS_QUESTS_H
