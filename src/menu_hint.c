#include "global.h"
#include "menu_hint.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "event_data.h"
#include "field_specials.h"
#include "graphics.h"
#include "link.h"
#include "menu.h"
#include "overworld.h"
#include "pokedex.h"
#include "safari_zone.h"
#include "strings.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/battle_frontier.h"
#include "constants/map_groups.h"

// "Next objective" hint window for the start menu (bottom-left).
// The ladder below is evaluated top to bottom when the menu opens and
// the first row whose completion condition is not met yet is shown.
// Row order and conditions: see menu-hint-対応表.md / menu-hint-対応表-殿堂入り後編.md
// NOTE: rows 1-5 already carry their final texts; the remaining rows
// are placeholders (ヒントテストN) for a playthrough test of the
// switching order. The text master is menu-hint-ヒント文一覧.md.

#define MENU_HINT_TYPE_FLAG       0 // complete when the flag is set
#define MENU_HINT_TYPE_VAR_GE     1 // complete when the var reaches the threshold
#define MENU_HINT_TYPE_DEX_CAUGHT 2 // complete when the National Dex caught count reaches the threshold

// The bulb is icon 12 on the start menu icon sheet (64x64, 4 per row):
// tile offset ((12 / 4) * 16) + ((12 % 4) * 2) = 48
#define MENU_HINT_BULB_TILE_OFFSET 48

struct MenuHintRow
{
    u8 type;
    u16 flagOrVar;
    u16 threshold;
    const u8 *text;
};

static const u8 sText_Hint001[] = _("2かいの へやで\nとけいを あわせよう");
static const u8 sText_Hint002[] = _("ママが よんでる\nしたに いこう！");
static const u8 sText_Hint003[] = _("となりの いえに\nあいさつに いこう");
static const u8 sText_Hint004[] = _("{RIVAL}は どこ？\nさがして みよう");
static const u8 sText_Hint005[] = _("はかせは どこだろう？\nさがして みよう！");
static const u8 sText_Hint006[] = _("ヒントテスト6 ヒントテスト6\nヒントテスト6 ヒントテスト6");
static const u8 sText_Hint007[] = _("ヒントテスト7 ヒントテスト7\nヒントテスト7 ヒントテスト7");
static const u8 sText_Hint008[] = _("ヒントテスト8 ヒントテスト8\nヒントテスト8 ヒントテスト8");
static const u8 sText_Hint009[] = _("ヒントテスト9 ヒントテスト9\nヒントテスト9 ヒントテスト9");
static const u8 sText_Hint010[] = _("ヒントテスト10 ヒントテスト10\nヒントテスト10 ヒントテスト10");
static const u8 sText_Hint011[] = _("ヒントテスト11 ヒントテスト11\nヒントテスト11 ヒントテスト11");
static const u8 sText_Hint012[] = _("ヒントテスト12 ヒントテスト12\nヒントテスト12 ヒントテスト12");
static const u8 sText_Hint013[] = _("ヒントテスト13 ヒントテスト13\nヒントテスト13 ヒントテスト13");
static const u8 sText_Hint014[] = _("ヒントテスト14 ヒントテスト14\nヒントテスト14 ヒントテスト14");
static const u8 sText_Hint015[] = _("ヒントテスト15 ヒントテスト15\nヒントテスト15 ヒントテスト15");
static const u8 sText_Hint016[] = _("ヒントテスト16 ヒントテスト16\nヒントテスト16 ヒントテスト16");
static const u8 sText_Hint017[] = _("ヒントテスト17 ヒントテスト17\nヒントテスト17 ヒントテスト17");
static const u8 sText_Hint018[] = _("ヒントテスト18 ヒントテスト18\nヒントテスト18 ヒントテスト18");
static const u8 sText_Hint019[] = _("ヒントテスト19 ヒントテスト19\nヒントテスト19 ヒントテスト19");
static const u8 sText_Hint020[] = _("ヒントテスト20 ヒントテスト20\nヒントテスト20 ヒントテスト20");
static const u8 sText_Hint021[] = _("ヒントテスト21 ヒントテスト21\nヒントテスト21 ヒントテスト21");
static const u8 sText_Hint022[] = _("ヒントテスト22 ヒントテスト22\nヒントテスト22 ヒントテスト22");
static const u8 sText_Hint023[] = _("ヒントテスト23 ヒントテスト23\nヒントテスト23 ヒントテスト23");
static const u8 sText_Hint024[] = _("ヒントテスト24 ヒントテスト24\nヒントテスト24 ヒントテスト24");
static const u8 sText_Hint025[] = _("ヒントテスト25 ヒントテスト25\nヒントテスト25 ヒントテスト25");
static const u8 sText_Hint026[] = _("ヒントテスト26 ヒントテスト26\nヒントテスト26 ヒントテスト26");
static const u8 sText_Hint027[] = _("ヒントテスト27 ヒントテスト27\nヒントテスト27 ヒントテスト27");
static const u8 sText_Hint028[] = _("ヒントテスト28 ヒントテスト28\nヒントテスト28 ヒントテスト28");
static const u8 sText_Hint029[] = _("ヒントテスト29 ヒントテスト29\nヒントテスト29 ヒントテスト29");
static const u8 sText_Hint030[] = _("ヒントテスト30 ヒントテスト30\nヒントテスト30 ヒントテスト30");
static const u8 sText_Hint031[] = _("ヒントテスト31 ヒントテスト31\nヒントテスト31 ヒントテスト31");
static const u8 sText_Hint032[] = _("ヒントテスト32 ヒントテスト32\nヒントテスト32 ヒントテスト32");
static const u8 sText_Hint033[] = _("ヒントテスト33 ヒントテスト33\nヒントテスト33 ヒントテスト33");
static const u8 sText_Hint034[] = _("ヒントテスト34 ヒントテスト34\nヒントテスト34 ヒントテスト34");
static const u8 sText_Hint035[] = _("ヒントテスト35 ヒントテスト35\nヒントテスト35 ヒントテスト35");
static const u8 sText_Hint036[] = _("ヒントテスト36 ヒントテスト36\nヒントテスト36 ヒントテスト36");
static const u8 sText_Hint037[] = _("ヒントテスト37 ヒントテスト37\nヒントテスト37 ヒントテスト37");
static const u8 sText_Hint038[] = _("ヒントテスト38 ヒントテスト38\nヒントテスト38 ヒントテスト38");
static const u8 sText_Hint039[] = _("ヒントテスト39 ヒントテスト39\nヒントテスト39 ヒントテスト39");
static const u8 sText_Hint040[] = _("ヒントテスト40 ヒントテスト40\nヒントテスト40 ヒントテスト40");
static const u8 sText_Hint041[] = _("ヒントテスト41 ヒントテスト41\nヒントテスト41 ヒントテスト41");
static const u8 sText_Hint042[] = _("ヒントテスト42 ヒントテスト42\nヒントテスト42 ヒントテスト42");
static const u8 sText_Hint043[] = _("ヒントテスト43 ヒントテスト43\nヒントテスト43 ヒントテスト43");
static const u8 sText_Hint044[] = _("ヒントテスト44 ヒントテスト44\nヒントテスト44 ヒントテスト44");
static const u8 sText_Hint045[] = _("ヒントテスト45 ヒントテスト45\nヒントテスト45 ヒントテスト45");
static const u8 sText_Hint046[] = _("ヒントテスト46 ヒントテスト46\nヒントテスト46 ヒントテスト46");
static const u8 sText_Hint047[] = _("ヒントテスト47 ヒントテスト47\nヒントテスト47 ヒントテスト47");
static const u8 sText_Hint048[] = _("ヒントテスト48 ヒントテスト48\nヒントテスト48 ヒントテスト48");
static const u8 sText_Hint049[] = _("ヒントテスト49 ヒントテスト49\nヒントテスト49 ヒントテスト49");
static const u8 sText_HintP01[] = _("ヒントテストP1 ヒントテストP1\nヒントテストP1 ヒントテストP1");
static const u8 sText_HintP02[] = _("ヒントテストP2 ヒントテストP2\nヒントテストP2 ヒントテストP2");
static const u8 sText_HintP03[] = _("ヒントテストP3 ヒントテストP3\nヒントテストP3 ヒントテストP3");
static const u8 sText_HintP04[] = _("ヒントテストP4 ヒントテストP4\nヒントテストP4 ヒントテストP4");
static const u8 sText_HintP05[] = _("ヒントテストP5 ヒントテストP5\nヒントテストP5 ヒントテストP5");
static const u8 sText_HintP06[] = _("ヒントテストP6 ヒントテストP6\nヒントテストP6 ヒントテストP6");
static const u8 sText_HintP07[] = _("ヒントテストP7 ヒントテストP7\nヒントテストP7 ヒントテストP7");
static const u8 sText_HintP08[] = _("ヒントテストP8 ヒントテストP8\nヒントテストP8 ヒントテストP8");
static const u8 sText_HintP09[] = _("ヒントテストP9 ヒントテストP9\nヒントテストP9 ヒントテストP9");
static const u8 sText_HintP10[] = _("ヒントテストP10 ヒントテストP10\nヒントテストP10 ヒントテストP10");
static const u8 sText_HintP11[] = _("ヒントテストP11 ヒントテストP11\nヒントテストP11 ヒントテストP11");
static const u8 sText_HintP12[] = _("ヒントテストP12 ヒントテストP12\nヒントテストP12 ヒントテストP12");
static const u8 sText_HintP13[] = _("ヒントテストP13 ヒントテストP13\nヒントテストP13 ヒントテストP13");
static const u8 sText_HintP14[] = _("ヒントテストP14 ヒントテストP14\nヒントテストP14 ヒントテストP14");
static const u8 sText_HintP15[] = _("ヒントテストP15 ヒントテストP15\nヒントテストP15 ヒントテストP15");
static const u8 sText_HintP16[] = _("ヒントテストP16 ヒントテストP16\nヒントテストP16 ヒントテストP16");
static const u8 sText_HintP17[] = _("ヒントテストP17 ヒントテストP17\nヒントテストP17 ヒントテストP17");
static const u8 sText_HintP18[] = _("ヒントテストP18 ヒントテストP18\nヒントテストP18 ヒントテストP18");
static const u8 sText_HintP19[] = _("ヒントテストP19 ヒントテストP19\nヒントテストP19 ヒントテストP19");
static const u8 sText_HintP20[] = _("ヒントテストP20 ヒントテストP20\nヒントテストP20 ヒントテストP20");
static const u8 sText_HintP21[] = _("ヒントテストP21 ヒントテストP21\nヒントテストP21 ヒントテストP21");
static const u8 sText_HintP22[] = _("ヒントテストP22 ヒントテストP22\nヒントテストP22 ヒントテストP22");
static const u8 sText_HintP23[] = _("ヒントテストP23 ヒントテストP23\nヒントテストP23 ヒントテストP23");
static const u8 sText_HintP24[] = _("ヒントテストP24 ヒントテストP24\nヒントテストP24 ヒントテストP24");
static const u8 sText_HintP25[] = _("ヒントテストP25 ヒントテストP25\nヒントテストP25 ヒントテストP25");
static const u8 sText_HintP26[] = _("ヒントテストP26 ヒントテストP26\nヒントテストP26 ヒントテストP26");
static const u8 sText_HintComplete[] = _("ヒントテストEND ヒントテストEND\nヒントテストEND ヒントテストEND");

static const struct MenuHintRow sMenuHintLadder[] =
{
    { MENU_HINT_TYPE_FLAG, FLAG_SET_WALL_CLOCK, 0, sText_Hint001 }, // 1
    { MENU_HINT_TYPE_VAR_GE, VAR_LITTLEROOT_INTRO_STATE, 7, sText_Hint002 }, // 2
    { MENU_HINT_TYPE_VAR_GE, VAR_LITTLEROOT_TOWN_STATE, 1, sText_Hint003 }, // 3
    { MENU_HINT_TYPE_VAR_GE, VAR_ROUTE101_STATE, 2, sText_Hint004 }, // 4
    { MENU_HINT_TYPE_FLAG, FLAG_RESCUED_BIRCH, 0, sText_Hint005 }, // 5
    { MENU_HINT_TYPE_VAR_GE, VAR_BIRCH_LAB_STATE, 3, sText_Hint006 }, // 6
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_RIVAL_ROUTE103, 0, sText_Hint007 }, // 7
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_POKEDEX_GET, 0, sText_Hint008 }, // 8
    { MENU_HINT_TYPE_VAR_GE, VAR_PETALBURG_GYM_STATE, 1, sText_Hint009 }, // 9
    { MENU_HINT_TYPE_VAR_GE, VAR_PETALBURG_GYM_STATE, 2, sText_Hint010 }, // 10
    { MENU_HINT_TYPE_VAR_GE, VAR_PETALBURG_WOODS_STATE, 1, sText_Hint011 }, // 11
    { MENU_HINT_TYPE_FLAG, FLAG_BADGE01_GET, 0, sText_Hint012 }, // 12
    { MENU_HINT_TYPE_FLAG, FLAG_DEVON_GOODS_STOLEN, 0, sText_Hint013 }, // 13
    { MENU_HINT_TYPE_FLAG, FLAG_RECOVERED_DEVON_GOODS, 0, sText_Hint014 }, // 14
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_POKENAV, 0, sText_Hint015 }, // 15
    { MENU_HINT_TYPE_FLAG, FLAG_MR_BRINEY_SAILING_INTRO, 0, sText_Hint016 }, // 16
    { MENU_HINT_TYPE_FLAG, FLAG_BADGE02_GET, 0, sText_Hint017 }, // 17
    { MENU_HINT_TYPE_FLAG, FLAG_DELIVERED_STEVEN_LETTER, 0, sText_Hint018 }, // 18
    { MENU_HINT_TYPE_FLAG, FLAG_DELIVERED_DEVON_GOODS, 0, sText_Hint019 }, // 19
    { MENU_HINT_TYPE_VAR_GE, VAR_ROUTE110_STATE, 1, sText_Hint020 }, // 20
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_WALLY_MAUVILLE, 0, sText_Hint021 }, // 21
    { MENU_HINT_TYPE_FLAG, FLAG_BADGE03_GET, 0, sText_Hint022 }, // 22
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_HM_ROCK_SMASH, 0, sText_Hint023 }, // 23
    { MENU_HINT_TYPE_VAR_GE, VAR_METEOR_FALLS_STATE, 1, sText_Hint024 }, // 24
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_METEORITE, 0, sText_Hint025 }, // 25
    { MENU_HINT_TYPE_FLAG, FLAG_BADGE04_GET, 0, sText_Hint026 }, // 26
    { MENU_HINT_TYPE_FLAG, FLAG_BADGE05_GET, 0, sText_Hint027 }, // 27
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_HM_SURF, 0, sText_Hint028 }, // 28
    { MENU_HINT_TYPE_VAR_GE, VAR_ROUTE118_STATE, 1, sText_Hint029 }, // 29
    { MENU_HINT_TYPE_VAR_GE, VAR_WEATHER_INSTITUTE_STATE, 1, sText_Hint030 }, // 30
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_HM_FLY, 0, sText_Hint031 }, // 31
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_DEVON_SCOPE, 0, sText_Hint032 }, // 32
    { MENU_HINT_TYPE_FLAG, FLAG_BADGE06_GET, 0, sText_Hint033 }, // 33
    { MENU_HINT_TYPE_VAR_GE, VAR_MT_PYRE_STATE, 1, sText_Hint034 }, // 34
    { MENU_HINT_TYPE_FLAG, FLAG_GROUDON_AWAKENED_MAGMA_HIDEOUT, 0, sText_Hint035 }, // 35
    { MENU_HINT_TYPE_FLAG, FLAG_MET_TEAM_AQUA_HARBOR, 0, sText_Hint036 }, // 36
    { MENU_HINT_TYPE_FLAG, FLAG_TEAM_AQUA_ESCAPED_IN_SUBMARINE, 0, sText_Hint037 }, // 37
    { MENU_HINT_TYPE_FLAG, FLAG_BADGE07_GET, 0, sText_Hint038 }, // 38
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_MAGMA_SPACE_CENTER, 0, sText_Hint039 }, // 39
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_HM_DIVE, 0, sText_Hint040 }, // 40
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_HM_STRENGTH, 0, sText_Hint041 }, // 41
    { MENU_HINT_TYPE_FLAG, FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN, 0, sText_Hint042 }, // 42
    { MENU_HINT_TYPE_VAR_GE, VAR_SKY_PILLAR_STATE, 1, sText_Hint043 }, // 43
    { MENU_HINT_TYPE_FLAG, FLAG_SOOTOPOLIS_ARCHIE_MAXIE_LEAVE, 0, sText_Hint044 }, // 44
    { MENU_HINT_TYPE_FLAG, FLAG_RECEIVED_HM_WATERFALL, 0, sText_Hint045 }, // 45
    { MENU_HINT_TYPE_FLAG, FLAG_BADGE08_GET, 0, sText_Hint046 }, // 46
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_WALLY_VICTORY_ROAD, 0, sText_Hint047 }, // 47
    { MENU_HINT_TYPE_VAR_GE, VAR_ELITE_4_STATE, 4, sText_Hint048 }, // 48
    { MENU_HINT_TYPE_FLAG, FLAG_IS_CHAMPION, 0, sText_Hint049 }, // 49
    { MENU_HINT_TYPE_VAR_GE, VAR_LITTLEROOT_HOUSES_STATE_BRENDAN, 4, sText_HintP01 }, // P1
    { MENU_HINT_TYPE_VAR_GE, VAR_DEX_UPGRADE_JOHTO_STARTER_STATE, 6, sText_HintP02 }, // P2
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_FRONTIER_PASS, 0, sText_HintP03 }, // P3
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_LATIAS_OR_LATIOS, 0, sText_HintP04 }, // P4
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_RAYQUAZA, 0, sText_HintP05 }, // P5
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_GROUDON, 0, sText_HintP06 }, // P6
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_KYOGRE, 0, sText_HintP07 }, // P7
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_REGIROCK, 0, sText_HintP08 }, // P8
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_REGICE, 0, sText_HintP09 }, // P9
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_REGISTEEL, 0, sText_HintP10 }, // P10
    { MENU_HINT_TYPE_FLAG, FLAG_DEFEATED_METEOR_FALLS_STEVEN, 0, sText_HintP11 }, // P11
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_TOWER_SILVER, 0, sText_HintP12 }, // P12
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_DOME_SILVER, 0, sText_HintP13 }, // P13
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_PALACE_SILVER, 0, sText_HintP14 }, // P14
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_ARENA_SILVER, 0, sText_HintP15 }, // P15
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_FACTORY_SILVER, 0, sText_HintP16 }, // P16
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_PIKE_SILVER, 0, sText_HintP17 }, // P17
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_PYRAMID_SILVER, 0, sText_HintP18 }, // P18
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_TOWER_GOLD, 0, sText_HintP19 }, // P19
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_DOME_GOLD, 0, sText_HintP20 }, // P20
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_PALACE_GOLD, 0, sText_HintP21 }, // P21
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_ARENA_GOLD, 0, sText_HintP22 }, // P22
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_FACTORY_GOLD, 0, sText_HintP23 }, // P23
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_PIKE_GOLD, 0, sText_HintP24 }, // P24
    { MENU_HINT_TYPE_FLAG, FLAG_SYS_PYRAMID_GOLD, 0, sText_HintP25 }, // P25
    { MENU_HINT_TYPE_DEX_CAUGHT, 0, 1025, sText_HintP26 }, // P26
};

static const struct WindowTemplate sWindowTemplate_MenuHint = {
    .bg = 0,
    .tilemapLeft = 1,
    // The standard frame is drawn one tile outside the window on every
    // side, so the frame's bottom edge lands on the last screen row
    // (15 + 4 = 19). Two text lines of 16px fill the 4-tile height
    // exactly (y=0 and y=16)
    .tilemapTop = 15,
    .width = 21,
    .height = 4,
    .paletteNum = 15,
    // The start menu window is 9 wide but (4 rows * 2) + 2 = 10 tiles
    // tall, so it occupies 0x139..0x192; the free span below it is
    // 0x5C..0x124 (between the clock window at 0x50 and the Yes/No
    // window at 0x125), and 21 * 4 = 84 tiles fit there
    .baseBlock = 0x5C
};

EWRAM_DATA static u8 sMenuHintWindowId = 0;
EWRAM_DATA static bool8 sMenuHintWindowShown = FALSE;

static bool8 IsMenuHintRowComplete(const struct MenuHintRow *row)
{
    switch (row->type)
    {
    case MENU_HINT_TYPE_FLAG:
        return FlagGet(row->flagOrVar);
    case MENU_HINT_TYPE_VAR_GE:
        return VarGet(row->flagOrVar) >= row->threshold;
    case MENU_HINT_TYPE_DEX_CAUGHT:
        return GetNationalPokedexCount(FLAG_GET_CAUGHT) >= row->threshold;
    }
    return TRUE;
}

void ShowMenuHintWindow(void)
{
    const u8 *text;
    bool8 complete = FALSE;
    u32 i;

    // Normal field menu only: the Safari Zone, Battle Pyramid, link,
    // Union Room, Battle Pike and multi partner rooms have their own
    // start menus (same split as BuildStartMenuActions)
    if (IsOverworldLinkActive() == TRUE
        || InUnionRoom() == TRUE
        || GetSafariZoneFlag() == TRUE
        || InBattlePike() == TRUE
        || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE
        || InMultiPartnerRoom() == TRUE)
        return;

    // No hint window inside the moving truck at the start of the game
    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_INSIDE_OF_TRUCK)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_INSIDE_OF_TRUCK))
        return;

    for (i = 0; i < ARRAY_COUNT(sMenuHintLadder); i++)
    {
        if (!IsMenuHintRowComplete(&sMenuHintLadder[i]))
            break;
    }

    if (i == ARRAY_COUNT(sMenuHintLadder))
    {
        text = sText_HintComplete;
        complete = TRUE;
    }
    else
    {
        text = sMenuHintLadder[i].text;
    }

    sMenuHintWindowId = AddWindow(&sWindowTemplate_MenuHint);
    PutWindowTilemap(sMenuHintWindowId);
    DrawStdWindowFrame(sMenuHintWindowId, FALSE);
    if (complete)
    {
        // 24x24 hanamaru stamp instead of the bulb
        BlitBitmapRectToWindow(sMenuHintWindowId, gMenuHintComplete_Gfx, 0, 0, 24, 24, 2, 4, 24, 24);
        AddTextPrinterParameterized(sMenuHintWindowId, FONT_NORMAL, text, 28, 0, TEXT_SKIP_DRAW, NULL);
    }
    else
    {
        BlitBitmapRectToWindow(sMenuHintWindowId, &gStartMenuIcons_Gfx[MENU_HINT_BULB_TILE_OFFSET * 32], 0, 0, 64, 64, 2, 8, 16, 16);
        AddTextPrinterParameterized(sMenuHintWindowId, FONT_NORMAL, text, 20, 0, TEXT_SKIP_DRAW, NULL);
    }
    CopyWindowToVram(sMenuHintWindowId, COPYWIN_GFX);
    sMenuHintWindowShown = TRUE;
}

void HideMenuHintWindow(void)
{
    if (!sMenuHintWindowShown)
        return;

    ClearStdWindowAndFrameToTransparent(sMenuHintWindowId, FALSE);
    CopyWindowToVram(sMenuHintWindowId, COPYWIN_GFX);
    RemoveWindow(sMenuHintWindowId);
    sMenuHintWindowShown = FALSE;
}
