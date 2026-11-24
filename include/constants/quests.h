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
// All quests are achievement-style entries (badges, Frontier symbols,
// Pokedex milestones, Hall of Fame entries). They are all visible and
// active from the start of the game and are advanced automatically by
// the checks in sQuestChecks (src/quests.c) when the quest menu opens.
// Rewards are not implemented yet.
#define QUEST_BADGE_1                   0
#define QUEST_BADGE_2                   1
#define QUEST_BADGE_3                   2
#define QUEST_BADGE_4                   3
#define QUEST_BADGE_5                   4
#define QUEST_BADGE_6                   5
#define QUEST_BADGE_7                   6
#define QUEST_BADGE_8                   7
#define QUEST_SYMBOL_TOWER_SILVER       8
#define QUEST_SYMBOL_TOWER_GOLD         9
#define QUEST_SYMBOL_DOME_SILVER        10
#define QUEST_SYMBOL_DOME_GOLD          11
#define QUEST_SYMBOL_PALACE_SILVER      12
#define QUEST_SYMBOL_PALACE_GOLD        13
#define QUEST_SYMBOL_ARENA_SILVER       14
#define QUEST_SYMBOL_ARENA_GOLD         15
#define QUEST_SYMBOL_FACTORY_SILVER     16
#define QUEST_SYMBOL_FACTORY_GOLD       17
#define QUEST_SYMBOL_PIKE_SILVER        18
#define QUEST_SYMBOL_PIKE_GOLD          19
#define QUEST_SYMBOL_PYRAMID_SILVER     20
#define QUEST_SYMBOL_PYRAMID_GOLD       21
#define QUEST_SYMBOL_ALL_SILVER         22
#define QUEST_SYMBOL_ALL_GOLD           23
#define QUEST_DEX_30                    24
#define QUEST_DEX_50                    25
#define QUEST_DEX_100                   26
#define QUEST_DEX_200                   27
#define QUEST_DEX_300                   28
#define QUEST_DEX_500                   29
#define QUEST_DEX_750                   30
#define QUEST_DEX_1025                  31
#define QUEST_HOF_1                     32
#define QUEST_HOF_5                     33
#define QUEST_HOF_20                    34
#define QUEST_COUNT                     (QUEST_HOF_20 + 1)

// Achievements have no subquests. One dummy entry is kept so the
// subquest save array keeps a nonzero size.
#define SUB_QUEST_DUMMY                 0
#define SUB_QUEST_COUNT                 1

#define QUEST_ARRAY_COUNT (SUB_QUEST_COUNT > QUEST_COUNT ? SUB_QUEST_COUNT : QUEST_COUNT)
#endif // GUARD_CONSTANTS_QUESTS_H
