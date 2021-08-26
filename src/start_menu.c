// PLEASE README
//
// START MENU MADE BY SBIRD AND JAIZU FOR POKÉMON POLAR AND SOTS
// DO NOT USE WITHOUT PERMISSION
//
// Jaizu, 26 - August - 2021
#include "global.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_pyramid_bag.h"
#include "bg.h"
#include "day_night.h"
#include "decompress.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_object_lock.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "field_specials.h"
#include "field_weather.h"
#include "field_screen_effect.h"
#include "frontier_pass.h"
#include "frontier_util.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "item_menu.h"
#include "link.h"
#include "load_save.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "new_game.h"
#include "option_menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokenav.h"
#include "rtc.h"
#include "safari_zone.h"
#include "save.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "start_menu.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_card.h"
#include "window.h"
#include "constants/songs.h"
#include "union_room.h"
#include "constants/rgb.h"

#define TOP_BG_INITIAL_Y_SHIFT (56)
#define BOT_BG_INITIAL_Y_SHIFT  (112)

#define FSM_ANIMATION_SLOW (1)
#define FSM_ANIMATION_MEDIUM (2)
#define FSM_ANIMATION_FAST (4)

#define FSM_ANIMATION_SPEED FSM_ANIMATION_FAST

// Menu actions
enum
{
    MENU_ACTION_POKEDEX,
    MENU_ACTION_POKEMON,
    MENU_ACTION_BAG,
    MENU_ACTION_QUESTS,
    MENU_ACTION_PLAYER,
    MENU_ACTION_SAVE,
    MENU_ACTION_OPTION,
    MENU_ACTION_EXIT,
    MENU_ACTION_RETIRE_SAFARI,
    MENU_ACTION_PLAYER_LINK,
    MENU_ACTION_REST_FRONTIER,
    MENU_ACTION_RETIRE_FRONTIER,
    MENU_ACTION_PYRAMID_BAG,
};

// Save status
enum
{
    SAVE_IN_PROGRESS,
    SAVE_SUCCESS,
    SAVE_CANCELED,
    SAVE_ERROR
};

#define FSM_SHOE_NORMAL_PAL 0
#define FSM_SHOE_GRAY_PAL 1

//Start Menu globals
struct StartMenuData
{
    u16 *tilemapBuffers[4];
    u8 spriteIds[7];
    bool8 requestScrollIn;
    bool8 requestScrollOut;
    u8 shoeSprite;
    u8 shoePals[2];
    bool8 objectiveToggle;
    u8 selectorNormalSprite;
    u8 selectorShoeSprite;
};

struct MenuActionStartMenu
{
    const u8 *text;
    union {
        void (*void_u8)(u8);
        u8 (*u8_void)(void);
    } func;
    u8 iconAnimId;
};

// IWRAM common
bool8 (*gMenuCallback)(void);

// EWRAM

//FIXME: We probably don't need a lot of those globals anymore
EWRAM_DATA static u8 sSafariBallsWindowId = 0;
EWRAM_DATA static u8 sBattlePyramidFloorWindowId = 0;
EWRAM_DATA static u8 sStartMenuCursorPos = 0;
EWRAM_DATA static u8 sNumStartMenuActions = 0;
EWRAM_DATA static u8 sCurrentStartMenuActions[9] = {0};
EWRAM_DATA static u8 sInitStartMenuData[2] = {0};

EWRAM_DATA static u8 (*sSaveDialogCallback)(void) = NULL;
EWRAM_DATA static u8 sSaveDialogTimer = 0;
EWRAM_DATA static bool8 sSavingComplete = FALSE;
EWRAM_DATA static u8 sSaveInfoWindowId = 0;

EWRAM_DATA static struct StartMenuData *sStartMenuData = {0};

// Menu action callbacks
static bool8 StartMenuPokedexCallback(void);
static bool8 StartMenuPokemonCallback(void);
static bool8 StartMenuBagCallback(void);
static bool8 StartMenuPokeNavCallback(void);
static bool8 StartMenuPlayerNameCallback(void);
static bool8 StartMenuSaveCallback(void);
static bool8 StartMenuOptionCallback(void);
static bool8 StartMenuExitCallback(void);
static bool8 StartQuestMenuCallback(void);
static bool8 StartMenuSafariZoneRetireCallback(void);
static bool8 StartMenuLinkModePlayerNameCallback(void);
static bool8 StartMenuBattlePyramidRetireCallback(void);
static bool8 StartMenuBattlePyramidBagCallback(void);

// Menu callbacks
static bool8 SaveStartCallback(void);
static bool8 SaveCallback(void);
static bool8 BattlePyramidRetireStartCallback(void);
static bool8 BattlePyramidRetireReturnCallback(void);
static bool8 BattlePyramidRetireCallback(void);
static bool8 HandleStartMenuInput(void);

// Save dialog callbacks
static u8 SaveConfirmSaveCallback(void);
static u8 SaveYesNoCallback(void);
static u8 SaveConfirmInputCallback(void);
static u8 SaveFileExistsCallback(void);
static u8 SaveConfirmOverwriteDefaultNoCallback(void);
static u8 SaveConfirmOverwriteCallback(void);
static u8 SaveOverwriteInputCallback(void);
static u8 SaveSavingMessageCallback(void);
static u8 SaveDoSaveCallback(void);
static u8 SaveSuccessCallback(void);
static u8 SaveReturnSuccessCallback(void);
static u8 SaveErrorCallback(void);
static u8 SaveReturnErrorCallback(void);
static u8 BattlePyramidConfirmRetireCallback(void);
static u8 BattlePyramidRetireYesNoCallback(void);
static u8 BattlePyramidRetireInputCallback(void);

// Sprite callbacks

static void SpriteCB_InvisOffScreen(struct Sprite *sprite);
static void SpriteCB_InvisSelector(struct Sprite *sprite);
static void SpriteCB_InvisSmallSelector(struct Sprite *sprite);

// Task callbacks
static void StartMenuTask(u8 taskId);
static void SaveGameTask(u8 taskId);
static void Task_SaveAfterLinkBattle(u8 taskId);
static void Task_WaitForBattleTowerLinkSave(u8 taskId);
static bool8 FieldCB_ReturnToFieldStartMenu(void);

static void UpdateHeaderText(void);

static const struct WindowTemplate sSafariBallsWindowTemplate = {0, 1, 1, 9, 4, 0xF, 8};

static const u8* const sPyramidFloorNames[] =
{
    gText_Floor1,
    gText_Floor2,
    gText_Floor3,
    gText_Floor4,
    gText_Floor5,
    gText_Floor6,
    gText_Floor7,
    gText_Peak
};

static const struct WindowTemplate sPyramidFloorWindowTemplate_2 = {0, 1, 1, 0xA, 4, 0xF, 8};
static const struct WindowTemplate sPyramidFloorWindowTemplate_1 = {0, 1, 1, 0xC, 4, 0xF, 8};

#define FSM_ICON_DEX 0
#define FSM_ICON_MONS 1
#define FSM_ICON_PLAYER 2
#define FSM_ICON_SETTINGS 3
#define FSM_ICON_BAG 4
#define FSM_ICON_QUESTS 5
#define FSM_ICON_SAVE 6
#define FSM_ICON_FORFEIT 7

static const struct MenuActionStartMenu sStartMenuItems[] =
{
    {gText_MenuPokedex, {.u8_void = StartMenuPokedexCallback}, FSM_ICON_DEX},
    {gText_MenuPokemon, {.u8_void = StartMenuPokemonCallback}, FSM_ICON_MONS},
    {gText_MenuBag, {.u8_void = StartMenuBagCallback}, FSM_ICON_BAG},
    {gText_MenuOptionQuests, {.u8_void = StartQuestMenuCallback}, FSM_ICON_QUESTS},
    {gText_MenuPlayer, {.u8_void = StartMenuPlayerNameCallback}, FSM_ICON_PLAYER},
    {gText_MenuSave, {.u8_void = StartMenuSaveCallback}, FSM_ICON_SAVE},
    {gText_MenuOption, {.u8_void = StartMenuOptionCallback}, FSM_ICON_SETTINGS},
    {gText_MenuExit, {.u8_void = StartMenuExitCallback}, 0xFF},
    {gText_MenuRetire, {.u8_void = StartMenuSafariZoneRetireCallback}, FSM_ICON_FORFEIT},
    {gText_MenuPlayer, {.u8_void = StartMenuLinkModePlayerNameCallback}, FSM_ICON_PLAYER},
    {gText_MenuRest, {.u8_void = StartMenuSaveCallback}, FSM_ICON_SAVE},
    {gText_MenuRetire, {.u8_void = StartMenuBattlePyramidRetireCallback}, FSM_ICON_FORFEIT},
    {gText_MenuBag, {.u8_void = StartMenuBattlePyramidBagCallback}, FSM_ICON_BAG},
};

static const struct BgTemplate sBgTemplates_LinkBattleSave[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }
};

static const struct WindowTemplate sWindowTemplates_LinkBattleSave[] =
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 26,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x194
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sSaveInfoWindowTemplate = {
    .bg = 0, 
    .tilemapLeft = 2, 
    .tilemapTop = 2, 
    .width = 14, 
    .height = 10, 
    .paletteNum = 15, 
    .baseBlock = 72,
};

//bg gfx
static const u32 sFullScreenStartMenuBgTiles[] = INCBIN_U32("graphics/start_menu/start_menu_bg.4bpp.lz");
static const u32 sFullScreenStartMenuBgTopMap[] = INCBIN_U32("graphics/start_menu/start_menu_bg_top.bin.lz");
static const u32 sFullScreenStartMenuBgBottomMap[] = INCBIN_U32("graphics/start_menu/start_menu_bg_bottom.bin.lz");
static const u16 sFullScreenStartMenuBgPal[] = INCBIN_U16("graphics/start_menu/bg.gbapal");

//sprite gfx
static const u32 sFullScreenStartMenuIconSprites[] = INCBIN_U32("graphics/start_menu/menu_items.4bpp.lz");
static const u16 sFullScreenStartMenuPal[] = INCBIN_U16("graphics/start_menu/menu_items.gbapal");

static const u32 sFullScreenStartMenuShoeSprite[] = INCBIN_U32("graphics/start_menu/shoes.4bpp.lz");
static const u16 sFullScreenStartMenuShoePalNormal[] = INCBIN_U16("graphics/start_menu/shoes.gbapal");
static const u16 sFullScreenStartMenuShoePalGray[] = INCBIN_U16("graphics/start_menu/shoes_gray.gbapal");

static const u32 sFullScreenStartMenuCursorNormalGfx[] = INCBIN_U32("graphics/start_menu/selector.4bpp.lz");
static const u32 sFullScreenStartMenuCursorShoesGfx[] = INCBIN_U32("graphics/start_menu/selector_running_shoes.4bpp.lz");
static const u16 sFullScreenStartMenuCursorPal[] = INCBIN_U16("graphics/start_menu/selector.gbapal");

#define PALTAG_FSM_ICONS 0x6000
#define TILETAG_FSM_ICONS 0x6000

#define PALTAG_FSM_SHOE 0x6001
#define TILETAG_FSM_SHOE 0x6001

#define PALTAG_FSM_SHOE_GRAY 0x6002

#define PALTAG_FSM_SELECTOR 0x6003
#define TILETAG_FSM_SELECTOR_NORMAL 0x6003
#define TILETAG_FSM_SELECTOR_SHOES 0x6004

static const struct SpritePalette sFullScreenStartMenuSpritePalette =
{
    .tag = PALTAG_FSM_ICONS,
    .data = sFullScreenStartMenuPal
};

static const struct SpritePalette sFullScreenStartMenuShoePalette = 
{
    .tag = PALTAG_FSM_SHOE,
    .data = sFullScreenStartMenuShoePalNormal,
};

static const struct SpritePalette sFullScreenStartMenuShoeGrayPalette = 
{
    .tag = PALTAG_FSM_SHOE_GRAY,
    .data = sFullScreenStartMenuShoePalGray,
};

static const struct SpritePalette sFullScreenStartMenuSelectorPalette = 
{
    .tag = PALTAG_FSM_SELECTOR,
    .data = sFullScreenStartMenuCursorPal,
};

static const struct CompressedSpriteSheet sFullScreenStartMenuSelectorNormalSpritesheet =
{
    .tag = TILETAG_FSM_SELECTOR_NORMAL,
    .size = 32*32,
    .data = sFullScreenStartMenuCursorNormalGfx
};

static const struct CompressedSpriteSheet sFullScreenStartMenuSelectorShoesSpritesheet =
{
    .tag = TILETAG_FSM_SELECTOR_SHOES,
    .size = 64*32,
    .data = sFullScreenStartMenuCursorShoesGfx
};

static const struct CompressedSpriteSheet sFullScreenStartMenuShoeSpritesheet =
{
    .tag = TILETAG_FSM_SHOE,
    .size = 32*16,
    .data = sFullScreenStartMenuShoeSprite
};

static const struct CompressedSpriteSheet sFullScreenStartMenuSpriteSheet =
{
    .tag = TILETAG_FSM_ICONS,
    .size = 8*32*16,
    .data = sFullScreenStartMenuIconSprites
};

static const struct OamData sFullScreenMenuOam32x32 = 
{
    .priority = 1,
    .size = SPRITE_SIZE(32x32),
    .shape = SPRITE_SHAPE(32x32),
    .affineMode = ST_OAM_AFFINE_NORMAL
};

static const struct OamData sFullScreenMenuOam64x32 =
{
    .priority = 1,
    .size = SPRITE_SIZE(64x32),
    .shape = SPRITE_SHAPE(64x32),
    .affineMode = ST_OAM_AFFINE_NORMAL
};

static const struct OamData sFullScreenMenuOam64x64 =
{
    .priority = 1,
    .size = SPRITE_SIZE(64x64),
    .shape = SPRITE_SHAPE(64x64),
    .affineMode = ST_OAM_AFFINE_NORMAL
};

static const union AnimCmd sAnimDex[] = 
{
    ANIMCMD_FRAME(0*16),
    ANIMCMD_END
};

static const union AnimCmd sAnimMons[] = 
{
    ANIMCMD_FRAME(1*16),
    ANIMCMD_END
};

static const union AnimCmd sAnimPlayer[] = 
{
    ANIMCMD_FRAME(2*16),
    ANIMCMD_END
};

static const union AnimCmd sAnimSettings[] = 
{
    ANIMCMD_FRAME(3*16),
    ANIMCMD_END
};

static const union AnimCmd sAnimBag[] = 
{
    ANIMCMD_FRAME(4*16),
    ANIMCMD_END
};

static const union AnimCmd sAnimQuests[] = 
{
    ANIMCMD_FRAME(5*16),
    ANIMCMD_END
};

static const union AnimCmd sAnimSave[] = 
{
    ANIMCMD_FRAME(6*16),
    ANIMCMD_END
};

static const union AnimCmd sAnimForfeit[] = 
{
    ANIMCMD_FRAME(7*16),
    ANIMCMD_END
};

static const union AnimCmd * const sFullScreenMenuSpriteFrameAnims[] = 
{
    sAnimDex,
    sAnimMons,
    sAnimPlayer,
    sAnimSettings,
    sAnimBag,
    sAnimQuests,
    sAnimSave,
    sAnimForfeit
};

static const struct SpriteTemplate sFullScreenStartMenuSpriteTemplate = 
{
    .tileTag = TILETAG_FSM_ICONS,
    .paletteTag = PALTAG_FSM_ICONS,
    .oam = &sFullScreenMenuOam32x32,
    .anims = sFullScreenMenuSpriteFrameAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_InvisOffScreen
};

static const struct SpriteTemplate sFullScreenStartMenuShoeTemplate =
{
    .tileTag = TILETAG_FSM_SHOE,
    .paletteTag = PALTAG_FSM_SHOE,
    .oam = &sFullScreenMenuOam32x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_InvisOffScreen
};

static const struct SpriteTemplate sFullScreenStartMenuSelectorTemplate =
{
    .tileTag = TILETAG_FSM_SELECTOR_NORMAL,
    .paletteTag = PALTAG_FSM_SELECTOR,
    .oam = &sFullScreenMenuOam64x32,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_InvisSmallSelector
};

static const struct SpriteTemplate sFullScreenStartMenuSelectorShoesTemplate =
{
    .tileTag = TILETAG_FSM_SELECTOR_SHOES,
    .paletteTag = PALTAG_FSM_SELECTOR,
    .oam = &sFullScreenMenuOam64x64,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_InvisSelector
};

static const struct BgTemplate sFullscreenStartMenuBgTemplates[] =
    {
        {.bg = 0,
         .charBaseIndex = 0,
         .mapBaseIndex = 31,
         .screenSize = 0,
         .paletteMode = 0,
         .priority = 0,
         .baseTile = 0},
        {.bg = 1,
         .charBaseIndex = 2,
         .mapBaseIndex = 20,
         .screenSize = 2,
         .paletteMode = 0,
         .priority = 1,
         .baseTile = 0},
        {.bg = 2,
         .charBaseIndex = 0,
         .mapBaseIndex = 24,
         .screenSize = 2,
         .paletteMode = 0,
         .priority = 2,
         .baseTile = 0},
        {.bg = 3,
         .charBaseIndex = 0,
         .mapBaseIndex = 28,
         .screenSize = 2,
         .paletteMode = 0,
         .priority = 0,
         .baseTile = 0},
};

#define WIN_FSM_DIALOGUE  0
#define WIN_FSM_ICONS_TOP 1
#define WIN_FSM_ICONS_BOT 2
#define WIN_FSM_QUEST_TOP 3

static const struct WindowTemplate sFullscreenStartMenuWindowTemplates[] =
{
    [WIN_FSM_DIALOGUE] = 
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x316,
    },
    [WIN_FSM_ICONS_TOP] = 
    {
        .bg = 1,
        .tilemapLeft = 1,
        .tilemapTop = 11,
        .width = 28,
        .height = 3,
        .paletteNum = 0,
        .baseBlock = 0x1
    },
    [WIN_FSM_ICONS_BOT] = 
    {
        .bg = 1,
        .tilemapLeft = 1,
        .tilemapTop = 17,
        .width = 21,
        .height = 3,
        .paletteNum = 0,
        .baseBlock = 0x55
    },
    [WIN_FSM_QUEST_TOP] = 
    {
        .bg = 3,
        .tilemapLeft = 2,
        .tilemapTop = 0,
        .width = 26,
        .height = 6,
        .paletteNum = 0,
        .baseBlock = 0x279
    },
    DUMMY_WIN_TEMPLATE
};

static const struct UCoords8 sStartMenuSelectorTilePositions[] =
{
    {2, 8},  {9,8},  {16,8}, {23,8},
    {2, 14}, {9,14}, {16,14}
};

static const u8 sStartMenuTextXOffsets[] =
{
    0, 56, 112, 168
};

static const struct Coords16 sStartMenuIconPositions[] = 
{
    {35, 80}, {92, 80}, {148, 80}, {204, 80},
    {35,128}, {92,128}, {148,128} 
};

static const u8 sIconTextColors[] =   {0, 1, 2};
static const u8 sHeaderTextColors[] = {7, 1, 3};

// Local functions
static void BuildStartMenuActions(void);
static void AddStartMenuAction(u8 action);
static void BuildNormalStartMenu(void);
static void BuildSafariZoneStartMenu(void);
static void BuildLinkModeStartMenu(void);
static void BuildUnionRoomStartMenu(void);
static void BuildBattlePikeStartMenu(void);
static void BuildBattlePyramidStartMenu(void);
static void BuildMultiPartnerRoomStartMenu(void);
static void ShowSafariBallsWindow(void);
static void ShowPyramidFloorWindow(void);
static void RemoveExtraStartMenuWindows(void);
static bool32 PrintStartMenuActions(s8 *pIndex, u32 count);
static bool32 InitStartMenuStep(void);
static void InitStartMenu(void);
static void CreateStartMenuTask(TaskFunc followupFunc);
static void InitSave(void);
static u8 RunSaveCallback(void);
static void ShowSaveMessage(const u8 *message, u8 (*saveCallback)(void));
static void HideSaveMessageWindow(void);
static void HideSaveInfoWindow(void);
static void SaveStartTimer(void);
static bool8 SaveSuccesTimer(void);
static bool8 SaveErrorTimer(void);
static void InitBattlePyramidRetire(void);
static void VBlankCB_LinkBattleSave(void);
static bool32 InitSaveWindowAfterLinkBattle(u8 *par1);
static void CB2_SaveAfterLinkBattle(void);
static void ShowSaveInfoWindow(void);
static void RemoveSaveInfoWindow(void);
static void HideStartMenuWindow(void);

static void BuildStartMenuActions(void)
{
    sNumStartMenuActions = 0;

    if (GetSafariZoneFlag())
        BuildSafariZoneStartMenu();
    else
        BuildNormalStartMenu();
}

static void AddStartMenuAction(u8 action)
{
    AppendToList(sCurrentStartMenuActions, &sNumStartMenuActions, action);
}

static void BuildNormalStartMenu(void)
{    
    if (FlagGet(FLAG_SYS_POKEDEX_GET))
        AddStartMenuAction(MENU_ACTION_POKEDEX);
    if (FlagGet(FLAG_SYS_POKEMON_GET))
        AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_BAG);
    if (FlagGet(FLAG_SYS_POKENAV_GET))
        AddStartMenuAction(MENU_ACTION_QUESTS);
    AddStartMenuAction(MENU_ACTION_SAVE);
}

static void BuildSafariZoneStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_RETIRE_SAFARI);
    AddStartMenuAction(MENU_ACTION_POKEDEX);
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_BAG);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
}

static void BuildLinkModeStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_BAG);

    if (FlagGet(FLAG_SYS_POKENAV_GET) == TRUE)
    {
        AddStartMenuAction(MENU_ACTION_QUESTS);
    }

    AddStartMenuAction(MENU_ACTION_PLAYER_LINK);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildUnionRoomStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_BAG);

    if (FlagGet(FLAG_SYS_POKENAV_GET) == TRUE)
    {
        AddStartMenuAction(MENU_ACTION_QUESTS);
    }

    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildBattlePikeStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEDEX);
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildBattlePyramidStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_PYRAMID_BAG);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_REST_FRONTIER);
    AddStartMenuAction(MENU_ACTION_RETIRE_FRONTIER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildMultiPartnerRoomStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void ShowSafariBallsWindow(void)
{
    sSafariBallsWindowId = AddWindow(&sSafariBallsWindowTemplate);
    PutWindowTilemap(sSafariBallsWindowId);
    DrawStdWindowFrame(sSafariBallsWindowId, FALSE);
    ConvertIntToDecimalStringN(gStringVar1, gNumSafariBalls, STR_CONV_MODE_RIGHT_ALIGN, 2);
    StringExpandPlaceholders(gStringVar4, gText_SafariBallStock);
    AddTextPrinterParameterized(sSafariBallsWindowId, 1, gStringVar4, 0, 1, 0xFF, NULL);
    CopyWindowToVram(sSafariBallsWindowId, 2);
}

static void ShowPyramidFloorWindow(void)
{
    if (gSaveBlock2Ptr->frontier.curChallengeBattleNum == 7)
        sBattlePyramidFloorWindowId = AddWindow(&sPyramidFloorWindowTemplate_1);
    else
        sBattlePyramidFloorWindowId = AddWindow(&sPyramidFloorWindowTemplate_2);

    PutWindowTilemap(sBattlePyramidFloorWindowId);
    DrawStdWindowFrame(sBattlePyramidFloorWindowId, FALSE);
    StringCopy(gStringVar1, sPyramidFloorNames[gSaveBlock2Ptr->frontier.curChallengeBattleNum]);
    StringExpandPlaceholders(gStringVar4, gText_BattlePyramidFloor);
    AddTextPrinterParameterized(sBattlePyramidFloorWindowId, 1, gStringVar4, 0, 1, 0xFF, NULL);
    CopyWindowToVram(sBattlePyramidFloorWindowId, 2);
}

static void RemoveExtraStartMenuWindows(void)
{
    if (GetSafariZoneFlag())
    {
        ClearStdWindowAndFrameToTransparent(sSafariBallsWindowId, FALSE);
        CopyWindowToVram(sSafariBallsWindowId, 2);
        RemoveWindow(sSafariBallsWindowId);
    }
    if (InBattlePyramid())
    {
        ClearStdWindowAndFrameToTransparent(sBattlePyramidFloorWindowId, FALSE);
        RemoveWindow(sBattlePyramidFloorWindowId);
    }
}

static bool32 PrintStartMenuActions(s8 *pIndex, u32 count)
{
    s8 index = *pIndex;

    do
    {
        if (sStartMenuItems[sCurrentStartMenuActions[index]].func.u8_void == StartMenuPlayerNameCallback)
        {
            PrintPlayerNameOnWindow(GetStartMenuWindowId(), sStartMenuItems[sCurrentStartMenuActions[index]].text, 8, (index << 4) + 9);
        }
        else
        {
            StringExpandPlaceholders(gStringVar4, sStartMenuItems[sCurrentStartMenuActions[index]].text);
            AddTextPrinterParameterized(GetStartMenuWindowId(), 1, gStringVar4, 8, (index << 4) + 9, 0xFF, NULL);
        }

        index++;
        if (index >= sNumStartMenuActions)
        {
            *pIndex = index;
            return TRUE;
        }

        count--;
    }
    while (count != 0);

    *pIndex = index;
    return FALSE;
}

static bool32 InitStartMenuStep(void)
{
    s8 state = sInitStartMenuData[0];

    switch (state)
    {
    case 0:
        sInitStartMenuData[0]++;
        break;
    case 1:
        BuildStartMenuActions();
        sInitStartMenuData[0]++;
        break;
    case 2:
        LoadMessageBoxAndBorderGfx();
        DrawStdWindowFrame(sub_81979C4(sNumStartMenuActions), FALSE);
        sInitStartMenuData[1] = 0;
        sInitStartMenuData[0]++;
        break;
    case 3:
        if (GetSafariZoneFlag())
            ShowSafariBallsWindow();
        if (InBattlePyramid())
            ShowPyramidFloorWindow();
        sInitStartMenuData[0]++;
        break;
    case 4:
        if (PrintStartMenuActions(&sInitStartMenuData[1], 2))
            sInitStartMenuData[0]++;
        break;
    case 5:
        sStartMenuCursorPos = sub_81983AC(GetStartMenuWindowId(), 1, 0, 9, 16, sNumStartMenuActions, sStartMenuCursorPos);
        CopyWindowToVram(GetStartMenuWindowId(), TRUE);
        return TRUE;
    }

    return FALSE;
}

static void InitStartMenu(void)
{
    sInitStartMenuData[0] = 0;
    sInitStartMenuData[1] = 0;
    while (!InitStartMenuStep())
        ;
}

static void StartMenuTask(u8 taskId)
{
    if (InitStartMenuStep() == TRUE)
        SwitchTaskToFollowupFunc(taskId);
}

static void CreateStartMenuTask(TaskFunc followupFunc)
{
    u8 taskId;

    sInitStartMenuData[0] = 0;
    sInitStartMenuData[1] = 0;
    taskId = CreateTask(StartMenuTask, 0x50);
    SetTaskFuncWithFollowupFunc(taskId, StartMenuTask, followupFunc);
}

static bool8 FieldCB_ReturnToFieldStartMenu(void)
{
    if (InitStartMenuStep() == FALSE)
    {
        return FALSE;
    }

    ReturnToFieldOpenStartMenu();
    return TRUE;
}

void ShowReturnToFieldStartMenu(void)
{
    sInitStartMenuData[0] = 0;
    sInitStartMenuData[1] = 0;
    gFieldCallback2 = FieldCB_ReturnToFieldStartMenu;
}

void Task_ShowStartMenu(u8 taskId)
{
    struct Task* task = &gTasks[taskId];

    switch(task->data[0])
    {
    case 0:
        if (InUnionRoom() == TRUE)
            SetUsingUnionRoomStartMenu();

        gMenuCallback = HandleStartMenuInput;
        task->data[0]++;
        break;
    case 1:
        if (gMenuCallback() == TRUE)
            DestroyTask(taskId);
        break;
    }
}

static void CB2_FullscreenStartMenu(void)
{
    BuildOamBuffer();
    AnimateSprites();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
    RunTextPrinters();
    RunTasks();
}

static void SpriteCB_InvisOffScreen(struct Sprite *sprite)
{
    if(sprite->y > (160 + 32))
        sprite->invisible = TRUE;
    else
        sprite->invisible = FALSE;
}

static void SpriteCB_InvisSelector(struct Sprite *sprite)
{
    if(sprite->y > (160 + 32))
        sprite->invisible = TRUE;
    else if(sStartMenuCursorPos == 0xFF)
        sprite->invisible = FALSE;
}

static void SpriteCB_InvisSmallSelector(struct Sprite *sprite)
{
    if(sprite->y > (160 + 32))
        sprite->invisible = TRUE;
    else if(sStartMenuCursorPos != 0xFF)
        sprite->invisible = FALSE;
}

static void FullscreenStartmenu_ScrollSprites(bool8 down)
{
    u32 i;
    for(i = 0; i < 7; ++i)
    {
        if(sStartMenuData->spriteIds[i] != MAX_SPRITES)
        {
            if(!down)
                gSprites[sStartMenuData->spriteIds[i]].y -= FSM_ANIMATION_SPEED*2;
            else
                gSprites[sStartMenuData->spriteIds[i]].y += FSM_ANIMATION_SPEED*2;
        }
    }
    if(sStartMenuData->shoeSprite != MAX_SPRITES)
    {
        if(!down)
            gSprites[sStartMenuData->shoeSprite].y -= FSM_ANIMATION_SPEED*2;
        else
            gSprites[sStartMenuData->shoeSprite].y += FSM_ANIMATION_SPEED*2;
    }
    if(sStartMenuData->selectorNormalSprite != MAX_SPRITES)
    {
        if(!down)
            gSprites[sStartMenuData->selectorNormalSprite].y -= FSM_ANIMATION_SPEED*2;
        else
            gSprites[sStartMenuData->selectorNormalSprite].y += FSM_ANIMATION_SPEED*2;
    }
    if(sStartMenuData->selectorShoeSprite != MAX_SPRITES)
    {
        if(!down)
            gSprites[sStartMenuData->selectorShoeSprite].y -= FSM_ANIMATION_SPEED*2;
        else
            gSprites[sStartMenuData->selectorShoeSprite].y += FSM_ANIMATION_SPEED*2;
    }
}

static void VBlankCB_FullscreenStartMenu(void)
{
    if(sStartMenuData && sStartMenuData->requestScrollIn)
    {
        s16 currentBotY, currentTopY;
        currentBotY = (s16)GetGpuReg(REG_OFFSET_BG2VOFS);
        currentTopY = (s16)GetGpuReg(REG_OFFSET_BG3VOFS);
        if(currentBotY < 0)
        {
            currentBotY += FSM_ANIMATION_SPEED*2;
            SetGpuReg(REG_OFFSET_BG2VOFS, (u16)currentBotY);
            SetGpuReg(REG_OFFSET_BG1VOFS, (u16)currentBotY);
            FullscreenStartmenu_ScrollSprites(FALSE);
        }
        if(currentTopY > 0)
        {
            currentTopY -= FSM_ANIMATION_SPEED;
            SetGpuReg(REG_OFFSET_BG3VOFS, (u16)currentTopY);
        }
        if(currentTopY <= 0 && currentTopY >= 0)
        {
            SetGpuReg(REG_OFFSET_BG2VOFS, 0);
            SetGpuReg(REG_OFFSET_BG3VOFS, 0);
            SetGpuReg(REG_OFFSET_BG1VOFS, 0);
            FullscreenStartmenu_ScrollSprites(TRUE);
            sStartMenuData->requestScrollIn = FALSE;
        }
    }
    else if(sStartMenuData && sStartMenuData->requestScrollOut)
    {
        s16 currentBotY, currentTopY;
        currentBotY = (s16)GetGpuReg(REG_OFFSET_BG2VOFS);
        currentTopY = (s16)GetGpuReg(REG_OFFSET_BG3VOFS);
        if(currentBotY >= (0 - BOT_BG_INITIAL_Y_SHIFT))
        {
            currentBotY -= FSM_ANIMATION_SPEED*2;
            SetGpuReg(REG_OFFSET_BG2VOFS, (u16)currentBotY);
            SetGpuReg(REG_OFFSET_BG1VOFS, (u16)currentBotY);
            FullscreenStartmenu_ScrollSprites(TRUE);
        }
        if(currentTopY <= TOP_BG_INITIAL_Y_SHIFT)
        {
            currentTopY += FSM_ANIMATION_SPEED;
            SetGpuReg(REG_OFFSET_BG3VOFS, (u16)currentTopY);
        }
        if(currentTopY > TOP_BG_INITIAL_Y_SHIFT && currentBotY < (0 - BOT_BG_INITIAL_Y_SHIFT))
        {
            SetGpuReg(REG_OFFSET_BG2VOFS, 0 - BOT_BG_INITIAL_Y_SHIFT);
            SetGpuReg(REG_OFFSET_BG3VOFS, TOP_BG_INITIAL_Y_SHIFT);
            SetGpuReg(REG_OFFSET_BG1VOFS, 0 - BOT_BG_INITIAL_Y_SHIFT);
            FullscreenStartmenu_ScrollSprites(FALSE);
            sStartMenuData->requestScrollOut = FALSE;
        }
    }
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    //ScanlineEffect_InitHBlankDmaTransfer(); idk if we need scanline effects yet, maybe
}

#define POS_TO_SCR_ADDR(x,y) (32*(y) + (x))
#define SCR_MAP_ENTRY(tile, pal, hflip, vflip) (tile | (hflip ? (1<<10) : 0) | (vflip ? (1 << 11) : 0) | (pal << 12))

#define CORNER_TOP_LEFT           SCR_MAP_ENTRY(0x07, 0, FALSE, FALSE)
#define BORDER_TOP                SCR_MAP_ENTRY(0x08, 0, FALSE, FALSE)
#define CORNER_TOP_RIGHT          SCR_MAP_ENTRY(0x07, 0, TRUE,  FALSE)
#define FILL                      SCR_MAP_ENTRY(0x0A, 0, FALSE, FALSE)
#define CORNER_BOT_LEFT           SCR_MAP_ENTRY(0x0F, 0, FALSE, FALSE)
#define BORDER_BOT                SCR_MAP_ENTRY(0x10, 0, FALSE, FALSE)
#define CORNER_BOT_RIGHT          SCR_MAP_ENTRY(0x0F, 0, TRUE,  FALSE)
#define SHOE_CORNER_TOP_RIGHT     SCR_MAP_ENTRY(0x09, 0, FALSE, FALSE)
#define SHOE_CORNER_BOT_RIGHT     SCR_MAP_ENTRY(0x09, 0, FALSE,  TRUE)
#define SHOE_BORDER_RIGHT         SCR_MAP_ENTRY(0x0B, 0, FALSE, FALSE)
#define SHOE_BORDER_BOT           SCR_MAP_ENTRY(0x08, 0, FALSE,  TRUE)
#define SHOE_CORNER_BOT_LEFT      SCR_MAP_ENTRY(0x07, 0, FALSE,  TRUE)

#define SEL_CORNER_TOP_LEFT       SCR_MAP_ENTRY(0x0C, 0, FALSE, FALSE)
#define SEL_BORDER_TOP            SCR_MAP_ENTRY(0x0D, 0, FALSE, FALSE)
#define SEL_CORNER_TOP_RIGHT      SCR_MAP_ENTRY(0x0C, 0, TRUE,  FALSE)
#define SEL_FILL                  SCR_MAP_ENTRY(0x0E, 0, FALSE, FALSE)
#define SEL_CORNER_BOT_LEFT       SCR_MAP_ENTRY(0x11, 0, FALSE, FALSE)
#define SEL_BORDER_BOT            SCR_MAP_ENTRY(0x12, 0, FALSE, FALSE)
#define SEL_CORNER_BOT_RIGHT      SCR_MAP_ENTRY(0x11, 0, TRUE,  FALSE)
#define SEL_SHOE_CORNER_TOP_RIGHT SCR_MAP_ENTRY(0x13, 0, FALSE, FALSE)
#define SEL_SHOE_CORNER_BOT_RIGHT SCR_MAP_ENTRY(0x13, 0, FALSE,  TRUE)
#define SEL_SHOE_BORDER_RIGHT     SCR_MAP_ENTRY(0x14, 0, FALSE, FALSE)
#define SEL_SHOE_BORDER_BOT       SCR_MAP_ENTRY(0x0D, 0, FALSE,  TRUE)
#define SEL_SHOE_CORNER_BOT_LEFT  SCR_MAP_ENTRY(0x0C, 0, FALSE,  TRUE)


static void FullscreenStartMenu_DrawShoesBackground(bool8 selected, u8 x, u8 y, u8 bg)
{
    u16 *tilemapPtr = GetBgTilemapBuffer(bg);

    tilemapPtr[POS_TO_SCR_ADDR(x  ,y)]   = selected ? SEL_CORNER_TOP_LEFT : CORNER_TOP_LEFT;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y)]   = selected ? SEL_BORDER_TOP : BORDER_TOP;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y)]   = selected ? SEL_BORDER_TOP : BORDER_TOP;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y)]   = selected ? SEL_BORDER_TOP : BORDER_TOP;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y)]   = selected ? SEL_BORDER_TOP : BORDER_TOP;
    tilemapPtr[POS_TO_SCR_ADDR(x+5,y)]   = selected ? SEL_BORDER_TOP : BORDER_TOP;
    tilemapPtr[POS_TO_SCR_ADDR(x+6,y)]   = selected ? SEL_SHOE_CORNER_TOP_RIGHT : SHOE_CORNER_TOP_RIGHT;

    tilemapPtr[POS_TO_SCR_ADDR(x,  y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+5,y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+6,y+1)] = selected ? SEL_SHOE_BORDER_RIGHT : SHOE_BORDER_RIGHT;

    tilemapPtr[POS_TO_SCR_ADDR(x,  y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+5,y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+6,y+2)] = selected ? SEL_SHOE_BORDER_RIGHT : SHOE_BORDER_RIGHT;

    tilemapPtr[POS_TO_SCR_ADDR(x,  y+3)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y+3)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y+3)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y+3)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y+3)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+5,y+3)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+6,y+3)] = selected ? SEL_SHOE_BORDER_RIGHT : SHOE_BORDER_RIGHT;

    tilemapPtr[POS_TO_SCR_ADDR(x  ,y+4)] = selected ? SEL_SHOE_CORNER_BOT_LEFT : SHOE_CORNER_BOT_LEFT;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y+4)] = selected ? SEL_SHOE_BORDER_BOT : SHOE_BORDER_BOT;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y+4)] = selected ? SEL_SHOE_BORDER_BOT : SHOE_BORDER_BOT;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y+4)] = selected ? SEL_SHOE_BORDER_BOT : SHOE_BORDER_BOT;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y+4)] = selected ? SEL_SHOE_BORDER_BOT : SHOE_BORDER_BOT;
    tilemapPtr[POS_TO_SCR_ADDR(x+5,y+4)] = selected ? SEL_SHOE_BORDER_BOT : SHOE_BORDER_BOT;
    tilemapPtr[POS_TO_SCR_ADDR(x+6,y+4)] = selected ? SEL_SHOE_CORNER_BOT_RIGHT : SHOE_CORNER_BOT_RIGHT;
}

static void FullscreenStartMenu_DrawSelectorBackground(bool8 selected, u8 x, u8 y, u8 bg)
{
    u16 *tilemapPtr = GetBgTilemapBuffer(bg);
    tilemapPtr[POS_TO_SCR_ADDR(x  ,y)]   = selected ? SEL_CORNER_TOP_LEFT : CORNER_TOP_LEFT;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y)]   = selected ? SEL_BORDER_TOP : BORDER_TOP;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y)]   = selected ? SEL_BORDER_TOP : BORDER_TOP;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y)]   = selected ? SEL_BORDER_TOP : BORDER_TOP;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y)]   = selected ? SEL_CORNER_TOP_RIGHT : CORNER_TOP_RIGHT;

    tilemapPtr[POS_TO_SCR_ADDR(x,  y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y+1)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y+1)] = selected ? SEL_FILL : FILL;

    tilemapPtr[POS_TO_SCR_ADDR(x,  y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y+2)] = selected ? SEL_FILL : FILL;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y+2)] = selected ? SEL_FILL : FILL;

    tilemapPtr[POS_TO_SCR_ADDR(x  ,y+3)] = selected ? SEL_CORNER_BOT_LEFT : CORNER_BOT_LEFT;
    tilemapPtr[POS_TO_SCR_ADDR(x+1,y+3)] = selected ? SEL_BORDER_BOT : BORDER_BOT;
    tilemapPtr[POS_TO_SCR_ADDR(x+2,y+3)] = selected ? SEL_BORDER_BOT : BORDER_BOT;
    tilemapPtr[POS_TO_SCR_ADDR(x+3,y+3)] = selected ? SEL_BORDER_BOT : BORDER_BOT;
    tilemapPtr[POS_TO_SCR_ADDR(x+4,y+3)] = selected ? SEL_CORNER_BOT_RIGHT : CORNER_BOT_RIGHT;
}

static const struct Coords16 sFullscreenStartMenuSelectorCoords[] =
{
    {48, 81}, {104, 81}, {160, 81}, {216, 81},
    {48, 129}, {104, 129}, {160, 129},
};

static void FullscreenStartMenu_FreeRessources(void)
{
    u32 i;
    for(i = 0; i < 4; ++i)
    {
        Free(sStartMenuData->tilemapBuffers[i]);
    }
    Free(sStartMenuData);
}

#define SELECTOR_SHOES_X 216
#define SELECTOR_SHOES_Y 145

static void FullscreenStartMenu_CreateSelectors(void)
{
    sStartMenuData->selectorNormalSprite = CreateSprite(&sFullScreenStartMenuSelectorTemplate, sFullscreenStartMenuSelectorCoords[sStartMenuCursorPos].x, sFullscreenStartMenuSelectorCoords[sStartMenuCursorPos].y + BOT_BG_INITIAL_Y_SHIFT, 0);
    sStartMenuData->selectorShoeSprite = CreateSprite(&sFullScreenStartMenuSelectorShoesTemplate, SELECTOR_SHOES_X, SELECTOR_SHOES_Y + BOT_BG_INITIAL_Y_SHIFT, 0);
    if(sStartMenuCursorPos == 0xFF)
        gSprites[sStartMenuData->selectorNormalSprite].invisible = TRUE;
    else
        gSprites[sStartMenuData->selectorShoeSprite].invisible = TRUE;
}

static const u8 gText_StartMenu_CurrentTimeAM[] = _("{STR_VAR_1}, {STR_VAR_2}:{STR_VAR_3} am");
static const u8 gText_StartMenu_CurrentTimeAMOff[] = _("{STR_VAR_1}, {STR_VAR_2}  {STR_VAR_3} am");
static const u8 gText_StartMenu_CurrentTimePM[] = _("{STR_VAR_1}, {STR_VAR_2}:{STR_VAR_3} pm");
static const u8 gText_StartMenu_CurrentTimePMOff[] = _("{STR_VAR_1}, {STR_VAR_2}  {STR_VAR_3} pm");
static const u8 gText_PlaceHolderText2[] = _("Something useful will be here!\nSomeday, maybe.");
static const u8 gText_StartMenu_TimeOfDay[] = _("{STR_VAR_1}");

static void FullscreenStartMenu_PrintHeaderText(void)
{
	UpdateHeaderText();
    PutWindowTilemap(WIN_FSM_QUEST_TOP);
}

static void UpdateHeaderText(void)
{
    //TODO: Print useful info
	u8 x;

    FillWindowPixelBuffer(WIN_FSM_QUEST_TOP, PIXEL_FILL(7));
	RtcCalcLocalTime();
    StringExpandPlaceholders(gStringVar1, gDayOfWeekTable[gLocalTime.dayOfWeek]);
	if (gLocalTime.hours >= 12)
	{
		if (gLocalTime.hours == 12)
			ConvertIntToDecimalStringN(gStringVar2, 12, STR_CONV_MODE_LEADING_ZEROS, 2);
		else
			ConvertIntToDecimalStringN(gStringVar2, gLocalTime.hours - 12, STR_CONV_MODE_RIGHT_ALIGN, 2);
		ConvertIntToDecimalStringN(gStringVar3, gLocalTime.minutes, STR_CONV_MODE_LEADING_ZEROS, 2);
		if (gLocalTime.seconds % 2)
			StringExpandPlaceholders(gStringVar4, gText_StartMenu_CurrentTimePM);
		else
			StringExpandPlaceholders(gStringVar4, gText_StartMenu_CurrentTimePMOff);
	}
	else
	{
		if (gLocalTime.hours == 0)
			ConvertIntToDecimalStringN(gStringVar2, 12, STR_CONV_MODE_LEADING_ZEROS, 2);
		else
			ConvertIntToDecimalStringN(gStringVar2, gLocalTime.hours, STR_CONV_MODE_RIGHT_ALIGN, 2);
		ConvertIntToDecimalStringN(gStringVar3, gLocalTime.minutes, STR_CONV_MODE_LEADING_ZEROS, 2);
		if (gLocalTime.seconds % 2)
			StringExpandPlaceholders(gStringVar4, gText_StartMenu_CurrentTimeAM);
		else
			StringExpandPlaceholders(gStringVar4, gText_StartMenu_CurrentTimeAMOff);
	}

    StringExpandPlaceholders(gStringVar1, gCurrentTimeOfDayList[GetCurrentTimeOfDay()]);
    StringExpandPlaceholders(gStringVar3, gText_StartMenu_TimeOfDay);

    x = GetStringRightAlignXOffset(8, gStringVar3, 26 * 8);

    AddTextPrinterParameterized4(WIN_FSM_QUEST_TOP, 8, 0, 4, 0, 4, sHeaderTextColors, 0xFF, gStringVar4);
    AddTextPrinterParameterized4(WIN_FSM_QUEST_TOP, 1, 0, 18, 0, -1, sHeaderTextColors, 0xFF, gText_PlaceHolderText2);
    AddTextPrinterParameterized4(WIN_FSM_QUEST_TOP, 8, x, 4, 0, 4, sHeaderTextColors, 0xFF, gStringVar3);

    CopyWindowToVram(WIN_FSM_QUEST_TOP, 2);
}

#define ICON_TO_WINDOW(id) (id <= 3 ? WIN_FSM_ICONS_TOP : WIN_FSM_ICONS_BOT)
#define ICON_TO_X_POS(id) (id <= 3 ? sStartMenuTextXOffsets[id] : sStartMenuTextXOffsets[id-4])

#define SHOES_POS_X 210
#define SHOES_POS_Y 134

static void FullscreenStartMenu_UpdateSelectorSpritePositions(void)
{
    if(sStartMenuCursorPos == 0xFF)
    {
        gSprites[sStartMenuData->selectorNormalSprite].invisible = TRUE;
        gSprites[sStartMenuData->selectorShoeSprite].invisible = FALSE;
    }
    else
    {
        gSprites[sStartMenuData->selectorNormalSprite].invisible = FALSE;
        gSprites[sStartMenuData->selectorShoeSprite].invisible = TRUE;
        gSprites[sStartMenuData->selectorNormalSprite].x = sFullscreenStartMenuSelectorCoords[sStartMenuCursorPos].x;
        gSprites[sStartMenuData->selectorNormalSprite].y = sFullscreenStartMenuSelectorCoords[sStartMenuCursorPos].y;
    }
}

static void FullscreenStartMenu_UpdateSelectorBackgrounds(void)
{
    u32 i;
    if(sStartMenuCursorPos == 0xFF)
    {
        FullscreenStartMenu_DrawShoesBackground(TRUE, 23, 14, 2);
    }
    else
    {
        FullscreenStartMenu_DrawShoesBackground(FALSE, 23, 14, 2);
    }
    for(i = 0; i < sNumStartMenuActions; ++i)
    {
        if(sStartMenuCursorPos == i)
            FullscreenStartMenu_DrawSelectorBackground(TRUE, sStartMenuSelectorTilePositions[i].x, sStartMenuSelectorTilePositions[i].y, 2);
        else
            FullscreenStartMenu_DrawSelectorBackground(FALSE, sStartMenuSelectorTilePositions[i].x, sStartMenuSelectorTilePositions[i].y, 2);
    }
}

static void FullscreenStartMenu_PrintActions(void)
{
    u32 i;
    u8 spriteId;
    FillWindowPixelBuffer(WIN_FSM_ICONS_TOP, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_FSM_ICONS_BOT, PIXEL_FILL(0));
    for(i = 0; i < sNumStartMenuActions; ++i)
    {
        u8 xCenter;
        StringExpandPlaceholders(gStringVar4, sStartMenuItems[sCurrentStartMenuActions[i]].text);
        xCenter = GetStringCenterAlignXOffset(1, gStringVar4, 56);
        AddTextPrinterParameterized3(ICON_TO_WINDOW(i), 1, ICON_TO_X_POS(i) + xCenter, 4, sIconTextColors, 0, gStringVar4);
    }

    CopyWindowToVram(WIN_FSM_ICONS_BOT, 2);
    PutWindowTilemap(WIN_FSM_ICONS_BOT);
    CopyWindowToVram(WIN_FSM_ICONS_TOP, 2);
    PutWindowTilemap(WIN_FSM_ICONS_TOP);

    //sprites
    for(i = 0; i < sNumStartMenuActions; ++i)
    {
        spriteId = CreateSprite(&sFullScreenStartMenuSpriteTemplate, sStartMenuIconPositions[i].x, sStartMenuIconPositions[i].y + BOT_BG_INITIAL_Y_SHIFT, 20+i);
        StartSpriteAnim(&gSprites[spriteId], sStartMenuItems[sCurrentStartMenuActions[i]].iconAnimId);
        sStartMenuData->spriteIds[i] = spriteId;
    }
    sStartMenuData->shoeSprite = CreateSprite(&sFullScreenStartMenuShoeTemplate, SHOES_POS_X, SHOES_POS_Y + BOT_BG_INITIAL_Y_SHIFT, 0);
    if(gSaveBlock1Ptr->autoRun)
        gSprites[sStartMenuData->shoeSprite].oam.paletteNum = sStartMenuData->shoePals[FSM_SHOE_NORMAL_PAL];
    else
        gSprites[sStartMenuData->shoeSprite].oam.paletteNum = sStartMenuData->shoePals[FSM_SHOE_GRAY_PAL];
}

#define tState gTasks[taskId].data[0]

static void Task_ControlStartMenu(u8 taskId)
{
	UpdateHeaderText();

    switch(tState)
    {
        case 0:
            if(JOY_NEW(R_BUTTON))
            {
                PlaySE(SE_SELECT);
                sStartMenuData->objectiveToggle ^= 1;
                FullscreenStartMenu_PrintHeaderText();
            }
            else if(JOY_NEW(DPAD_LEFT))
            {
                PlaySE(SE_SELECT);
                if((sStartMenuCursorPos == 0) && (sNumStartMenuActions >= 4))
                    sStartMenuCursorPos = 3;
                else if(sStartMenuCursorPos == 4)
                    sStartMenuCursorPos = 0xFF;
                else if(sStartMenuCursorPos == 0)
                    sStartMenuCursorPos = 0xFF;
                else if(sStartMenuCursorPos == 0xFF)
                    sStartMenuCursorPos = sNumStartMenuActions -1;
                else
                    sStartMenuCursorPos--;
                FullscreenStartMenu_UpdateSelectorBackgrounds();
                FullscreenStartMenu_UpdateSelectorSpritePositions();
                ScheduleBgCopyTilemapToVram(2);
            }
            else if(JOY_NEW(DPAD_RIGHT))
            {
                PlaySE(SE_SELECT);
                if (sNumStartMenuActions == 3) {
                    if(sStartMenuCursorPos == 2)
                        sStartMenuCursorPos = 0xFF;
                    else if(sStartMenuCursorPos == 0xFF)
                        sStartMenuCursorPos = 0;
                    else
                        sStartMenuCursorPos++;
                }
                else if((sStartMenuCursorPos == 0xFF) && (sNumStartMenuActions == 4))
                    sStartMenuCursorPos = 0;
                else if((sStartMenuCursorPos == 4) && (sNumStartMenuActions == 5))
                    sStartMenuCursorPos = 0xFF;
                else if((sStartMenuCursorPos == 4) && (sNumStartMenuActions >= 5))
                    sStartMenuCursorPos = 5;
                else if((sStartMenuCursorPos == 5) && (sNumStartMenuActions == 6))
                    sStartMenuCursorPos = 0xFF;
                else if((sStartMenuCursorPos == 5) && (sNumStartMenuActions >= 6))
                    sStartMenuCursorPos = 6;
                else if((sStartMenuCursorPos == 6) && (sNumStartMenuActions == 7))
                    sStartMenuCursorPos = 0xFF;
                else if((sStartMenuCursorPos == 3) && (sNumStartMenuActions >= 4))
                    sStartMenuCursorPos = 0;
                else if((sStartMenuCursorPos == 0xFF) && (sNumStartMenuActions >= 4))
                    sStartMenuCursorPos = 4;
                else if(sStartMenuCursorPos == 0xFF)
                    sStartMenuCursorPos = 0;
                else
                    sStartMenuCursorPos++;
                FullscreenStartMenu_UpdateSelectorBackgrounds();
                FullscreenStartMenu_UpdateSelectorSpritePositions();
                ScheduleBgCopyTilemapToVram(2);
            }
            else if(JOY_NEW(DPAD_UP))
            {
                PlaySE(SE_SELECT);
                if((sNumStartMenuActions == 3) && (sStartMenuCursorPos == 0xFF))
                    sStartMenuCursorPos = 2;
                else if(sNumStartMenuActions == 4) {
                    if (sStartMenuCursorPos == 0xFF) 
                        sStartMenuCursorPos = 3;
                    else if (sStartMenuCursorPos == 3) 
                        sStartMenuCursorPos = 0xFF;
                }
                else if(sNumStartMenuActions > 4)
                {
                    if (sStartMenuCursorPos == 0xFF)                         
                        sStartMenuCursorPos = 3;
                    else if (sStartMenuCursorPos == 0)
                        sStartMenuCursorPos = 4;
                    else if ((sStartMenuCursorPos == 1) && (sNumStartMenuActions > 5))
                        sStartMenuCursorPos = 5;
                    else if ((sStartMenuCursorPos == 2) && (sNumStartMenuActions > 6))
                        sStartMenuCursorPos = 6;
                    else if (sStartMenuCursorPos == 3)
                        sStartMenuCursorPos = 0xFF;
                    else if (sStartMenuCursorPos == 4)
                        sStartMenuCursorPos = 0;
                    else if (sStartMenuCursorPos == 5)
                        sStartMenuCursorPos = 1;
                    else if (sStartMenuCursorPos == 6)
                        sStartMenuCursorPos = 2;
                }
                else
                {
                    sStartMenuCursorPos = 0xFF;
                }
                FullscreenStartMenu_UpdateSelectorBackgrounds();
                FullscreenStartMenu_UpdateSelectorSpritePositions();
                ScheduleBgCopyTilemapToVram(2);
            }
            else if(JOY_NEW(DPAD_DOWN))
            {
                PlaySE(SE_SELECT);
                if((sStartMenuCursorPos == 0xFF) && (sNumStartMenuActions >= 4))
                    sStartMenuCursorPos = 3;
                else if(sStartMenuCursorPos == 0xFF)
                    sStartMenuCursorPos = 0;
                else if(sStartMenuCursorPos == 4)
                    sStartMenuCursorPos = 0;
                else if(sStartMenuCursorPos == 5)
                    sStartMenuCursorPos = 1;
                else if(sStartMenuCursorPos == 6)
                    sStartMenuCursorPos = 2;
                else if((sStartMenuCursorPos + 4) >= sNumStartMenuActions)
                    sStartMenuCursorPos = 0xFF;
                else
                    sStartMenuCursorPos += 4;
                FullscreenStartMenu_UpdateSelectorBackgrounds();
                FullscreenStartMenu_UpdateSelectorSpritePositions();
                ScheduleBgCopyTilemapToVram(2);
            }
            else if(JOY_NEW(START_BUTTON | B_BUTTON))
            {
                PlaySE(SE_SELECT);
                tState = 10;
            }
            else if(JOY_NEW(A_BUTTON))
            {
                if(sStartMenuCursorPos == 0xFF)
                {
                    PlaySE(SE_SELECT);
                    gSaveBlock1Ptr->autoRun ^= 1;
                    if(gSaveBlock1Ptr->autoRun)
                        gSprites[sStartMenuData->shoeSprite].oam.paletteNum = sStartMenuData->shoePals[FSM_SHOE_NORMAL_PAL];
                    else
                        gSprites[sStartMenuData->shoeSprite].oam.paletteNum = sStartMenuData->shoePals[FSM_SHOE_GRAY_PAL];
                }
                else if(sStartMenuItems[sCurrentStartMenuActions[sStartMenuCursorPos]].iconAnimId == FSM_ICON_SAVE)
                {
                    PlaySE(SE_SELECT);
                    tState = 20;
                }
                else
                {
                    PlaySE(SE_SELECT);
                    tState = 15;
                }
            }
        break;
        case 10:
            //ready to leave to overworld
            sStartMenuData->requestScrollOut = TRUE;
            FullscreenStartmenu_ScrollSprites(TRUE);
            tState++;
            break;
            
        case 11:
            if(!sStartMenuData->requestScrollOut)
            {
                memset(gPlttBufferFaded, 0, 1024);
                SetMainCallback2(CB2_ReturnToField);
                FullscreenStartMenu_FreeRessources();
                DestroyTask(taskId);
            }
            break;
        case 15:
            //execute a start menu function
            sStartMenuData->requestScrollOut = TRUE;
            FullscreenStartmenu_ScrollSprites(TRUE);
            tState++;
            break;
        case 16:
            if(!sStartMenuData->requestScrollOut)
            {
                memset(gPlttBufferFaded, 0, 1024);
                FullscreenStartMenu_FreeRessources();
                DestroyTask(taskId);
                sStartMenuItems[sCurrentStartMenuActions[sStartMenuCursorPos]].func.u8_void();
            }
            break;
        case 20:
            {
                s16 currentY = (s16)GetGpuReg(REG_OFFSET_BG3VOFS);
                if(currentY > -160)
                {
                    currentY -= FSM_ANIMATION_SPEED * 2;
                    SetGpuReg(REG_OFFSET_BG3VOFS, currentY);
                }
                else
                {
                    InitSave();
                    tState++;
                }
            }
            break;
        case 21:
            switch (RunSaveCallback())
            {
            case SAVE_IN_PROGRESS:
                break;
            case SAVE_CANCELED: // Back to start menu
                ClearDialogWindowAndFrameToTransparent(0, TRUE);
                tState = 22;
                break;
            case SAVE_SUCCESS:
            case SAVE_ERROR:    // Close start menu
                ClearDialogWindowAndFrameToTransparent(0, TRUE);
                tState = 22;
                //ScriptUnfreezeObjectEvents();
                //ScriptContext2_Disable();
                SoftResetInBattlePyramid();
            }
            break;
        case 22:
            {
                s16 currentY = (s16)GetGpuReg(REG_OFFSET_BG3VOFS);
                if(currentY < 0)
                {
                    currentY += FSM_ANIMATION_SPEED * 2;
                    SetGpuReg(REG_OFFSET_BG3VOFS, currentY);
                }
                else
                {
                    tState = 0;
                }
            }
    }
}

static void Task_LoadStartMenuScene(u8 taskId)
{
    s16 currentTopY, currentBotY;
    u32 i;
    switch(tState)
    {
    case 0:
        SetVBlankCallback(VBlankCB_FullscreenStartMenu);
        ResetBgsAndClearDma3BusyFlags(0);
        FreeAllWindowBuffers();
        ResetSpriteData();
        ScanlineEffect_Stop();
        ResetPaletteFade();
        DeactivateAllTextPrinters();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = 0;
        InitBgsFromTemplates(0, sFullscreenStartMenuBgTemplates, 4);
        ResetBgPositions();
        tState++;
        break;
    case 1:
        SetBgTilemapBuffer(0, sStartMenuData->tilemapBuffers[0]);
        SetBgTilemapBuffer(1, sStartMenuData->tilemapBuffers[1]);
        SetBgTilemapBuffer(2, sStartMenuData->tilemapBuffers[2]);
        SetBgTilemapBuffer(3, sStartMenuData->tilemapBuffers[3]);
        FreeTempTileDataBuffersIfPossible();
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(3, sFullScreenStartMenuBgTiles, 0, 0, 0);
        CopyToBgTilemapBuffer(3, sFullScreenStartMenuBgTopMap, 0, 0);
        CopyToBgTilemapBuffer(2, sFullScreenStartMenuBgBottomMap, 0, 0);
        LoadPalette(sFullScreenStartMenuBgPal, 0, 0x20);
        LoadSpritePalette(&sFullScreenStartMenuSpritePalette);
        LoadCompressedSpriteSheet(&sFullScreenStartMenuSpriteSheet);
        sStartMenuData->shoePals[FSM_SHOE_NORMAL_PAL] = LoadSpritePalette(&sFullScreenStartMenuShoePalette);
        sStartMenuData->shoePals[FSM_SHOE_GRAY_PAL] = LoadSpritePalette(&sFullScreenStartMenuShoeGrayPalette);
        LoadCompressedSpriteSheet(&sFullScreenStartMenuShoeSpritesheet);
        LoadSpritePalette(&sFullScreenStartMenuSelectorPalette);
        LoadCompressedSpriteSheet(&sFullScreenStartMenuSelectorNormalSpritesheet);
        LoadCompressedSpriteSheet(&sFullScreenStartMenuSelectorShoesSpritesheet);
        SetGpuReg(REG_OFFSET_BG3VOFS, TOP_BG_INITIAL_Y_SHIFT);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0 - (BOT_BG_INITIAL_Y_SHIFT));
        SetGpuReg(REG_OFFSET_BG1VOFS, 0 - (BOT_BG_INITIAL_Y_SHIFT));
        InitWindows(&sFullscreenStartMenuWindowTemplates[0]);
        FullscreenStartMenu_PrintActions();
        FullscreenStartMenu_PrintHeaderText();
        FullscreenStartMenu_CreateSelectors();
        FullscreenStartMenu_UpdateSelectorBackgrounds();
        ScheduleBgCopyTilemapToVram(1);
        ScheduleBgCopyTilemapToVram(2);
        ScheduleBgCopyTilemapToVram(3);
        tState++;
        break;
    case 2:
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        sStartMenuData->requestScrollIn = TRUE;
        FullscreenStartmenu_ScrollSprites(FALSE);
        tState++;
        break;
    case 3:
        if(!sStartMenuData->requestScrollIn)
        {
            //note for future developers: loading the gfx here fixes a bug when coming from the option window
            //I don't really know why :( - sbird
            LoadMessageBoxAndBorderGfx();
            gTasks[taskId].func = Task_ControlStartMenu;
            tState = 0;
        }
        break;
    default:
        break;
    }
}

static void CB2_InitFullscreenStartMenu(void)
{
    if(!gPaletteFade.active)
    {
        u8 taskId;
        u32 i;
        CleanupOverworldWindowsAndTilemaps();
        ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        sStartMenuData = AllocZeroed(sizeof(*sStartMenuData));
        if(sStartMenuData == NULL)
        {
            SetMainCallback2(CB2_ReturnToField);
            return;
        }
        sStartMenuData->tilemapBuffers[0] = AllocZeroed(0x800);
        sStartMenuData->tilemapBuffers[1] = AllocZeroed(0x1000);
        sStartMenuData->tilemapBuffers[2] = AllocZeroed(0x1000);
        sStartMenuData->tilemapBuffers[3] = AllocZeroed(0x1000);
        for(i = 0; i < 7; ++i)
        {
            sStartMenuData->spriteIds[i] = MAX_SPRITES;
        }
        sStartMenuData->shoeSprite = MAX_SPRITES;
        sStartMenuData->objectiveToggle = FALSE;
        ResetTasks();
        SetVBlankCallback(NULL);
        DmaFillLarge16(3, 0, (u8 *)VRAM, VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetMainCallback2(CB2_FullscreenStartMenu);
        taskId = CreateTask(Task_LoadStartMenuScene, 1);
        tState = 0;
    }
    else
    {
        UpdatePaletteFade();
    }
}

#undef tState

void ReturnToFullscreenStartMenu(void)
{
    BuildStartMenuActions();
    if(sStartMenuCursorPos >= sNumStartMenuActions)
        sStartMenuCursorPos = 0xFF;
    SetMainCallback2(CB2_InitFullscreenStartMenu);
}

void ShowFullscreenStartMenu(void)
{
    if(!IsUpdateLinkStateCBActive())
    {
        FreezeObjectEvents();
        PlayerFreeze();
        sub_808BCF4();
    }
    BuildStartMenuActions();
    if(sStartMenuCursorPos >= sNumStartMenuActions)
        sStartMenuCursorPos = 0xFF;
    BeginNormalPaletteFade(0xFFFFFFFF, -16, 0, 16, RGB_BLACK);
    PlayRainStoppingSoundEffect();
    SetMainCallback2(CB2_InitFullscreenStartMenu);
}

//FIXME: This is the old initialization handler for the start menu, we want to clean up some code here after we're done
void ShowStartMenu(void)
{
    if (!IsUpdateLinkStateCBActive())
    {
        FreezeObjectEvents();
        PlayerFreeze();
        sub_808BCF4();
    }
    CreateStartMenuTask(Task_ShowStartMenu);
    ScriptContext2_Enable();
}

static bool8 HandleStartMenuInput(void)
{
    if (JOY_NEW(DPAD_UP))
    {
        PlaySE(SE_SELECT);
        sStartMenuCursorPos = Menu_MoveCursor(-1);
    }

    if (JOY_NEW(DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        sStartMenuCursorPos = Menu_MoveCursor(1);
    }

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (sStartMenuItems[sCurrentStartMenuActions[sStartMenuCursorPos]].func.u8_void == StartMenuPokedexCallback)
        {
            if (GetNationalPokedexCount(FLAG_GET_SEEN) == 0)
                return FALSE;
        }
        gMenuCallback = sStartMenuItems[sCurrentStartMenuActions[sStartMenuCursorPos]].func.u8_void;

        if (gMenuCallback != StartMenuSaveCallback
            && gMenuCallback != StartMenuExitCallback
            && gMenuCallback != StartMenuSafariZoneRetireCallback
            && gMenuCallback != StartMenuBattlePyramidRetireCallback)
        {
           FadeScreen(FADE_TO_BLACK, 0);
        }

        return FALSE;
    }

    if (JOY_NEW(START_BUTTON | B_BUTTON))
    {
        RemoveExtraStartMenuWindows();
        HideStartMenu();
        return TRUE;
    }

    return FALSE;
}

bool8 StartMenuPokedexCallback(void)
{
    if (!gPaletteFade.active)
    {
        IncrementGameStat(GAME_STAT_CHECKED_POKEDEX);
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_OpenPokedex);

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuPokemonCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_PartyMenuFromStartMenu); // Display party menu

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuBagCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_BagMenuFromStartMenu); // Display bag menu

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuPokeNavCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_InitPokeNav);  // Display PokeNav

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuPlayerNameCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();

        if (IsUpdateLinkStateCBActive() || InUnionRoom())
            ShowPlayerTrainerCard(CB2_ReturnToFieldWithOpenMenu); // Display trainer card
        else if (FlagGet(FLAG_SYS_FRONTIER_PASS))
            ShowFrontierPass(CB2_ReturnToFieldWithOpenMenu); // Display frontier pass
        else
            ShowPlayerTrainerCard(CB2_ReturnToFieldWithOpenMenu); // Display trainer card

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuSaveCallback(void)
{
    if (InBattlePyramid())
        RemoveExtraStartMenuWindows();

    gMenuCallback = SaveStartCallback; // Display save menu

    return FALSE;
}

static bool8 StartMenuOptionCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_InitOptionMenu); // Display option menu
        gMain.savedCallback = CB2_ReturnToFieldWithOpenMenu;

        return TRUE;
    }

    return FALSE;
}

static bool8 StartQuestMenuCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        // TODO: IMPLEMENT QUEST SYSTEM
        //SetMainCallback2(CB2_InitQuestMenu); 
        
        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuExitCallback(void)
{
    RemoveExtraStartMenuWindows();
    HideStartMenu(); // Hide start menu

    return TRUE;
}

static bool8 StartMenuSafariZoneRetireCallback(void)
{
    RemoveExtraStartMenuWindows();
    HideStartMenu();
    SafariZoneRetirePrompt();

    return TRUE;
}

static bool8 StartMenuLinkModePlayerNameCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        CleanupOverworldWindowsAndTilemaps();
        ShowTrainerCardInLink(gLocalLinkPlayerId, CB2_ReturnToFieldWithOpenMenu);

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuBattlePyramidRetireCallback(void)
{
    gMenuCallback = BattlePyramidRetireStartCallback; // Confirm retire

    return FALSE;
}

// Functionally unused
void ShowBattlePyramidStartMenu(void)
{
    ClearDialogWindowAndFrameToTransparent(0, FALSE);
    ScriptUnfreezeObjectEvents();
    CreateStartMenuTask(Task_ShowStartMenu);
    ScriptContext2_Enable();
}

static bool8 StartMenuBattlePyramidBagCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_PyramidBagMenuFromStartMenu);

        return TRUE;
    }

    return FALSE;
}

static bool8 SaveStartCallback(void)
{
    InitSave();
    gMenuCallback = SaveCallback;

    return FALSE;
}

static bool8 SaveCallback(void)
{
    switch (RunSaveCallback())
    {
    case SAVE_IN_PROGRESS:
        return FALSE;
    case SAVE_CANCELED: // Back to start menu
        ClearDialogWindowAndFrameToTransparent(0, FALSE);
        InitStartMenu();
        gMenuCallback = HandleStartMenuInput;
        return FALSE;
    case SAVE_SUCCESS:
    case SAVE_ERROR:    // Close start menu
        ClearDialogWindowAndFrameToTransparent(0, TRUE);
        ScriptUnfreezeObjectEvents();
        ScriptContext2_Disable();
        SoftResetInBattlePyramid();
        return TRUE;
    }

    return FALSE;
}

static bool8 BattlePyramidRetireStartCallback(void)
{
    InitBattlePyramidRetire();
    gMenuCallback = BattlePyramidRetireCallback;

    return FALSE;
}

static bool8 BattlePyramidRetireReturnCallback(void)
{
    InitStartMenu();
    gMenuCallback = HandleStartMenuInput;

    return FALSE;
}

static bool8 BattlePyramidRetireCallback(void)
{
    switch (RunSaveCallback())
    {
    case SAVE_SUCCESS: // No (Stay in battle pyramid)
        RemoveExtraStartMenuWindows();
        gMenuCallback = BattlePyramidRetireReturnCallback;
        return FALSE;
    case SAVE_IN_PROGRESS:
        return FALSE;
    case SAVE_CANCELED: // Yes (Retire from battle pyramid)
        ClearDialogWindowAndFrameToTransparent(0, TRUE);
        ScriptUnfreezeObjectEvents();
        ScriptContext2_Disable();
        ScriptContext1_SetupScript(BattlePyramid_Retire);
        return TRUE;
    }

    return FALSE;
}

static void InitSave(void)
{
    SaveMapView();
    sSaveDialogCallback = SaveConfirmSaveCallback;
    sSavingComplete = FALSE;
}

static u8 RunSaveCallback(void)
{
    // True if text is still printing
    if (RunTextPrintersAndIsPrinter0Active() == TRUE)
    {
        return SAVE_IN_PROGRESS;
    }

    sSavingComplete = FALSE;
    return sSaveDialogCallback();
}

void SaveGame(void)
{
    InitSave();
    CreateTask(SaveGameTask, 0x50);
}

static void ShowSaveMessage(const u8 *message, u8 (*saveCallback)(void))
{
    StringExpandPlaceholders(gStringVar4, message);
    sub_819786C(0, TRUE);
    AddTextPrinterForMessage_2(TRUE);
    sSavingComplete = TRUE;
    sSaveDialogCallback = saveCallback;
}

static void SaveGameTask(u8 taskId)
{
    u8 status = RunSaveCallback();

    switch (status)
    {
    case SAVE_CANCELED:
    case SAVE_ERROR:
        gSpecialVar_Result = 0;
        break;
    case SAVE_SUCCESS:
        gSpecialVar_Result = status;
        break;
    case SAVE_IN_PROGRESS:
        return;
    }

    DestroyTask(taskId);
    EnableBothScriptContexts();
}

static void HideSaveMessageWindow(void)
{
    ClearDialogWindowAndFrame(0, TRUE);
}

static void HideSaveInfoWindow(void)
{
    RemoveSaveInfoWindow();
}

static void SaveStartTimer(void)
{
    sSaveDialogTimer = 60;
}

static bool8 SaveSuccesTimer(void)
{
    sSaveDialogTimer--;

    if (JOY_HELD(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        return TRUE;
    }
    if (sSaveDialogTimer == 0)
    {
        return TRUE;
    }

    return FALSE;
}

static bool8 SaveErrorTimer(void)
{
    if (sSaveDialogTimer != 0)
    {
        sSaveDialogTimer--;
    }
    else if (JOY_HELD(A_BUTTON))
    {
        return TRUE;
    }

    return FALSE;
}

static u8 SaveConfirmSaveCallback(void)
{
    ClearStdWindowAndFrame(GetStartMenuWindowId(), FALSE);
    ShowSaveInfoWindow();

    if (InBattlePyramid())
    {
        ShowSaveMessage(gText_BattlePyramidConfirmRest, SaveYesNoCallback);
    }
    else
    {
        ShowSaveMessage(gText_ConfirmSave, SaveYesNoCallback);
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveYesNoCallback(void)
{
    DisplaySaveOffsetYesNoMenuDefaultYes(); // Show Yes/No menu
    sSaveDialogCallback = SaveConfirmInputCallback;
    return SAVE_IN_PROGRESS;
}

static u8 SaveConfirmInputCallback(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes
        switch (gSaveFileStatus)
        {
        case SAVE_STATUS_EMPTY:
        case SAVE_STATUS_CORRUPT:
            if (gDifferentSaveFile == FALSE)
            {
                sSaveDialogCallback = SaveFileExistsCallback;
                return SAVE_IN_PROGRESS;
            }

            sSaveDialogCallback = SaveSavingMessageCallback;
            return SAVE_IN_PROGRESS;
        default:
            sSaveDialogCallback = SaveFileExistsCallback;
            return SAVE_IN_PROGRESS;
        }
    case -1: // B Button
    case 1: // No
        HideSaveInfoWindow();
        HideSaveMessageWindow();
        return SAVE_CANCELED;
    }

    return SAVE_IN_PROGRESS;
}

// A different save file exists
static u8 SaveFileExistsCallback(void)
{
    if (gDifferentSaveFile == TRUE)
    {
        sSaveDialogCallback = SaveSavingMessageCallback; //ShowSaveMessage(gText_DifferentSaveFile, SaveConfirmOverwriteDefaultNoCallback);
    }
    else
    {
        sSaveDialogCallback = SaveSavingMessageCallback;
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveConfirmOverwriteDefaultNoCallback(void)
{
    DisplayYesNoMenuWithDefault(1); // Show Yes/No menu (No selected as default)
    sSaveDialogCallback = SaveOverwriteInputCallback;
    return SAVE_IN_PROGRESS;
}

static u8 SaveConfirmOverwriteCallback(void)
{
    DisplaySaveOffsetYesNoMenuDefaultYes(); // Show Yes/No menu
    sSaveDialogCallback = SaveOverwriteInputCallback;
    return SAVE_IN_PROGRESS;
}

static u8 SaveOverwriteInputCallback(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes
        sSaveDialogCallback = SaveSavingMessageCallback;
        return SAVE_IN_PROGRESS;
    case -1: // B Button
    case 1: // No
        HideSaveInfoWindow();
        HideSaveMessageWindow();
        return SAVE_CANCELED;
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveSavingMessageCallback(void)
{
    ShowSaveMessage(gText_SavingDontTurnOff, SaveDoSaveCallback);
    return SAVE_IN_PROGRESS;
}

static u8 SaveDoSaveCallback(void)
{
    u8 saveStatus;

    IncrementGameStat(GAME_STAT_SAVED_GAME);
    PausePyramidChallenge();

    if (gDifferentSaveFile == TRUE)
    {
        saveStatus = TrySavingData(SAVE_OVERWRITE_DIFFERENT_FILE);
        gDifferentSaveFile = FALSE;
    }
    else
    {
        saveStatus = TrySavingData(SAVE_NORMAL);
    }

    if (saveStatus == SAVE_STATUS_OK)
        ShowSaveMessage(gText_PlayerSavedGame, SaveSuccessCallback);
    else
        ShowSaveMessage(gText_SaveError, SaveErrorCallback);

    SaveStartTimer();
    return SAVE_IN_PROGRESS;
}

static u8 SaveSuccessCallback(void)
{
    if (!IsTextPrinterActive(0))
    {
        PlaySE(SE_SAVE);
        sSaveDialogCallback = SaveReturnSuccessCallback;
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveReturnSuccessCallback(void)
{
    if (!IsSEPlaying() && SaveSuccesTimer())
    {
        HideSaveInfoWindow();
        return SAVE_SUCCESS;
    }
    else
    {
        return SAVE_IN_PROGRESS;
    }
}

static u8 SaveErrorCallback(void)
{
    if (!IsTextPrinterActive(0))
    {
        PlaySE(SE_BOO);
        sSaveDialogCallback = SaveReturnErrorCallback;
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveReturnErrorCallback(void)
{
    if (!SaveErrorTimer())
    {
        return SAVE_IN_PROGRESS;
    }
    else
    {
        HideSaveInfoWindow();
        return SAVE_ERROR;
    }
}

static void InitBattlePyramidRetire(void)
{
    sSaveDialogCallback = BattlePyramidConfirmRetireCallback;
    sSavingComplete = FALSE;
}

static u8 BattlePyramidConfirmRetireCallback(void)
{
    ClearStdWindowAndFrame(GetStartMenuWindowId(), FALSE);
    RemoveStartMenuWindow();
    ShowSaveMessage(gText_BattlePyramidConfirmRetire, BattlePyramidRetireYesNoCallback);

    return SAVE_IN_PROGRESS;
}

static u8 BattlePyramidRetireYesNoCallback(void)
{
    DisplayYesNoMenuWithDefault(1); // Show Yes/No menu (No selected as default)
    sSaveDialogCallback = BattlePyramidRetireInputCallback;

    return SAVE_IN_PROGRESS;
}

static u8 BattlePyramidRetireInputCallback(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes
        return SAVE_CANCELED;
    case -1: // B Button
    case 1: // No
        HideSaveMessageWindow();
        return SAVE_SUCCESS;
    }

    return SAVE_IN_PROGRESS;
}

static void VBlankCB_LinkBattleSave(void)
{
    TransferPlttBuffer();
}

static bool32 InitSaveWindowAfterLinkBattle(u8 *state)
{
    switch (*state)
    {
    case 0:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
        SetVBlankCallback(NULL);
        ScanlineEffect_Stop();
        DmaClear16(3, PLTT, PLTT_SIZE);
        DmaFillLarge16(3, 0, (void *)VRAM, VRAM_SIZE, 0x1000);
        break;
    case 1:
        ResetSpriteData();
        ResetTasks();
        ResetPaletteFade();
        ScanlineEffect_Clear();
        break;
    case 2:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates_LinkBattleSave, ARRAY_COUNT(sBgTemplates_LinkBattleSave));
        InitWindows(sWindowTemplates_LinkBattleSave);
        LoadUserWindowBorderGfx_(0, 8, 224);
        Menu_LoadStdPalAt(240);
        break;
    case 3:
        ShowBg(0);
        BlendPalettes(-1, 16, 0);
        SetVBlankCallback(VBlankCB_LinkBattleSave);
        EnableInterrupts(1);
        break;
    case 4:
        return TRUE;
    }

    (*state)++;
    return FALSE;
}

void CB2_SetUpSaveAfterLinkBattle(void)
{
    if (InitSaveWindowAfterLinkBattle(&gMain.state))
    {
        CreateTask(Task_SaveAfterLinkBattle, 0x50);
        SetMainCallback2(CB2_SaveAfterLinkBattle);
    }
}

static void CB2_SaveAfterLinkBattle(void)
{
    RunTasks();
    UpdatePaletteFade();
}

static void Task_SaveAfterLinkBattle(u8 taskId)
{
    s16 *state = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
        switch (*state)
        {
        case 0:
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            AddTextPrinterParameterized2(0,
                                        1,
                                        gText_SavingDontTurnOffPower,
                                        TEXT_SPEED_FF,
                                        NULL,
                                        TEXT_COLOR_DARK_GRAY,
                                        TEXT_COLOR_WHITE,
                                        TEXT_COLOR_LIGHT_GRAY);
            DrawTextBorderOuter(0, 8, 14);
            PutWindowTilemap(0);
            CopyWindowToVram(0, 3);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);

            if (gWirelessCommType != 0 && InUnionRoom())
            {
                if (Link_AnyPartnersPlayingFRLG_JP())
                {
                    *state = 1;
                }
                else
                {
                    *state = 5;
                }
            }
            else
            {
                gSoftResetDisabled = 1;
                *state = 1;
            }
            break;
        case 1:
            SetContinueGameWarpStatusToDynamicWarp();
            FullSaveGame();
            *state = 2;
            break;
        case 2:
            if (CheckSaveFile())
            {
                ClearContinueGameWarpStatus2();
                *state = 3;
                gSoftResetDisabled = 0;
            }
            break;
        case 3:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            *state = 4;
            break;
        case 4:
            FreeAllWindowBuffers();
            SetMainCallback2(gMain.savedCallback);
            DestroyTask(taskId);
            break;
        case 5:
            CreateTask(Task_LinkSave, 5);
            *state = 6;
            break;
        case 6:
            if (!FuncIsActiveTask(Task_LinkSave))
            {
                *state = 3;
            }
            break;
        }
    }
}

static void ShowSaveInfoWindow(void)
{
    struct WindowTemplate saveInfoWindow = sSaveInfoWindowTemplate;
    u8 gender;
    u8 color;
    u32 xOffset;
    u32 yOffset;

    if (!FlagGet(FLAG_SYS_POKEDEX_GET))
    {
        saveInfoWindow.height -= 2;
    }

    sSaveInfoWindowId = AddWindow(&saveInfoWindow);
    DrawStdWindowFrame(sSaveInfoWindowId, FALSE);

    gender = gSaveBlock2Ptr->playerGender;
    color = TEXT_COLOR_RED;  // Red when female, blue when male.

    if (gender == MALE)
    {
        color = TEXT_COLOR_BLUE;
    }

    // Print region name
    yOffset = 1;
    BufferSaveMenuText(SAVE_MENU_LOCATION, gStringVar4, TEXT_COLOR_GREEN);
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gStringVar4, 0, yOffset, 0xFF, NULL);

    // Print player name
    yOffset += 16;
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gText_SavingPlayer, 0, yOffset, 0xFF, NULL);
    BufferSaveMenuText(SAVE_MENU_NAME, gStringVar4, color);
    xOffset = GetStringRightAlignXOffset(1, gStringVar4, 0x70);
    PrintPlayerNameOnWindow(sSaveInfoWindowId, gStringVar4, xOffset, yOffset);

    // Print badge count
    yOffset += 16;
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gText_SavingBadges, 0, yOffset, 0xFF, NULL);
    BufferSaveMenuText(SAVE_MENU_BADGES, gStringVar4, color);
    xOffset = GetStringRightAlignXOffset(1, gStringVar4, 0x70);
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gStringVar4, xOffset, yOffset, 0xFF, NULL);

    if (FlagGet(FLAG_SYS_POKEDEX_GET) == TRUE)
    {
        // Print pokedex count
        yOffset += 16;
        AddTextPrinterParameterized(sSaveInfoWindowId, 1, gText_SavingPokedex, 0, yOffset, 0xFF, NULL);
        BufferSaveMenuText(SAVE_MENU_CAUGHT, gStringVar4, color);
        xOffset = GetStringRightAlignXOffset(1, gStringVar4, 0x70);
        AddTextPrinterParameterized(sSaveInfoWindowId, 1, gStringVar4, xOffset, yOffset, 0xFF, NULL);
    }

    // Print play time
    yOffset += 16;
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gText_SavingTime, 0, yOffset, 0xFF, NULL);
    BufferSaveMenuText(SAVE_MENU_PLAY_TIME, gStringVar4, color);
    xOffset = GetStringRightAlignXOffset(1, gStringVar4, 0x70);
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gStringVar4, xOffset, yOffset, 0xFF, NULL);

    CopyWindowToVram(sSaveInfoWindowId, 2);
}

static void RemoveSaveInfoWindow(void)
{
    ClearStdWindowAndFrame(sSaveInfoWindowId, FALSE);
    RemoveWindow(sSaveInfoWindowId);
}

static void Task_WaitForBattleTowerLinkSave(u8 taskId)
{
    if (!FuncIsActiveTask(Task_LinkSave))
    {
        DestroyTask(taskId);
        EnableBothScriptContexts();
    }
}

#define tPartialSave data[2]

void SaveForBattleTowerLink(void)
{
    u8 taskId = CreateTask(Task_LinkSave, 5);
    gTasks[taskId].tPartialSave = TRUE;
    gTasks[CreateTask(Task_WaitForBattleTowerLinkSave, 6)].data[1] = taskId;
}

#undef tPartialSave

static void HideStartMenuWindow(void)
{
    ClearStdWindowAndFrame(GetStartMenuWindowId(), TRUE);
    RemoveStartMenuWindow();
    ScriptUnfreezeObjectEvents();
    ScriptContext2_Disable();
}

void HideStartMenu(void)
{
    PlaySE(SE_SELECT);
    HideStartMenuWindow();
}

void AppendToList(u8 *list, u8 *pos, u8 newEntry)
{
    list[*pos] = newEntry;
    (*pos)++;
}
