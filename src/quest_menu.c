// PLEASE README
//
// QUEST SYSTEM MADE BY DISTURBO, JAIZU AND SAMURH FOR POKÉMON POLAR
// DO NOT USE WITHOUT PERMISSION
//
// Jaizu, 30 - August - 2021
#include "global.h"
#include "main.h"
#include "bg.h"
#include "decompress.h"
#include "event_object_movement.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "quest_menu.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trig.h"
#include "window.h"
#include "constants/event_objects.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "data/text/quest_menu.h"
#include "data/quest_table.h"

#define TAG_ARROW       53100
#define TAG_CUSTOM_ICON 53101

static void CB2_QuestMenu();
static void VBlankCallback_QuestMenu();
static void Task_QuestMenu_HandleInput(u8 taskId);
static void QuestMenu_InitBgs();
static void QuestMenu_LoadGraphics();
static void QuestMenu_HighlightSelectedQuest();
static void QuestMenu_InitDisplayedQuests();
static void QuestMenu_InitScrollAndCursorPosition();
static u8 QuestMenu_GetUnlockedFilterQuestCount();
static void QuestMenu_InitTextWindows();
static void QuestMenu_LoadHeaderText(bool8 isMainPage);
static void QuestMenu_LoadWindowText(u8 windowId, u8 font, u8 left, u8 top, const u8* color);
static void QuestMenu_LoadQuestText();
static void QuestMenu_PrintQuestString(u8 questId, u8 slotId);
static u8 QuestMenu_GetCurrentSelectedQuest();
static bool8 QuestMenu_TryCursorDown();
static bool8 QuestMenu_TryCursorUp();
static void QuestMenu_LoadOverworldIcon(u8 objectEventId, u8 slot);
static void QuestMenu_LoadCustomIcon(const struct CompressedSpriteSheet* spriteSheet, const struct SpritePalette* spritePalette, const struct SpriteTemplate* spriteTemplate, u8 slot);
static void QuestMenu_LoadIcons();
static void Task_FadeBackToOverworld(u8 taskId);
static void QuestMenu_FreeSprites();
static void SpriteCallback_ScrollingArrowDown(struct Sprite *sprite);
static void SpriteCallback_ScrollingArrowUp(struct Sprite *sprite);
static bool8 QuestMenu_IsMenuScrollableDown();
static bool8 QuestMenu_IsMenuScrollableUp();
static void QuestMenu_LoadScrollArrows();
static void QuestMenu_DestroyScrollArrows();
static void Task_QuestMenu_InitQuestWindow(u8 taskId);
static void QuestMenu_LoadQuestWindowBgData();
static void CB2_InitQuestWindow();
static void ResetWindows();
static void QuestMenu_LoadQuestPageText(u8 questId);
static void QuestMenu_LoadQuestPageTitleText(u8 questId);
static void QuestMenu_LoadQuestPageNpcAndMapText(u8 questId);
static void QuestMenu_LoadQuestPageProgress(u8 questId);
static void QuestMenu_AddQuestProgressText(u8 questId, u8 padding, u8* dest);
static void QuestMenu_LoadPageDescription(u8 questId);
static void QuestMenu_LoadPageIcon(u8 questId);
static void Task_HandlePageInput(u8 taskId);
static void CB2_ReturnToQuestMenuMainPage();

static void Task_QuestMenu_ChangeDisplayFilter(u8 taskId);
static const u8 QuestMenu_GetNextIndex(u8 currentValue, u8 maxValue);
static const bool8 QuestMenu_DiplayFilter_All(u8 completed);
static const bool8 QuestMenu_DiplayFilter_InProgress(u8 completed);
static const bool8 QuestMenu_DiplayFilter_Completed(u8 completed);

static const bool8 (*const sQuestDisplayFilter[QUEST_FILTER_COUNT])(u8) =
{
    [QUEST_FILTER_ALL] = QuestMenu_DiplayFilter_All,
    [QUEST_FILTER_IN_PROGRESS] = QuestMenu_DiplayFilter_InProgress,
    [QUEST_FILTER_COMPLETED] = QuestMenu_DiplayFilter_Completed,
};

static const u8 sFontColor_QuestMenuStd[3] = {0, 1, 2};
static const u8 sFontColor_QuestMenuPage[3] = {0, 2, 3};

static const union AnimCmd sSpriteAnim_VerticalArrowUp[] = 
{
    ANIMCMD_FRAME(4, 0),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_VerticalArrowDown[] = 
{
    ANIMCMD_FRAME(4, 0, .vFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_ScrollArrowIndicatorUp[] =
{
    sSpriteAnim_VerticalArrowUp,
};

static const union AnimCmd *const sSpriteAnimTable_ScrollArrowIndicatorDown[] =
{
    sSpriteAnim_VerticalArrowDown,
};

const struct CompressedSpriteSheet sSpriteSheet_ArrowIcon = {
    .data = sTiles_ArrowRed,
    .size = 0x100,
    .tag = TAG_ARROW,
};

const struct CompressedSpriteSheet sSpriteSheet_CustomIcon = {
    .data = sTiles_CustomIcon,
    .size = 0x200,
    .tag = TAG_CUSTOM_ICON,
};

const struct SpritePalette sSpritePalette_ArrowIcon = 
{
    .data = sPalette_ArrowRed,
    .tag = TAG_ARROW,
};

const struct SpritePalette sSpritePalette_CustomIcon = {
    .data = sPalette_CustomIcon,
    .tag = TAG_CUSTOM_ICON,
};

const struct OamData sOam_ArrowIconDown = {
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = 0,
    .matrixNum = 0x10,
    .size = 1,
    .priority = 0,
};

const struct OamData sOam_ArrowIconUp = {
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = 0,
    .matrixNum = 0x8,
    .size = 1,
    .priority = 0,
};

const struct OamData sOam_Icon = {
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = 0,
    .size = 2,
    .priority = 0,
};

const struct SpriteTemplate sSpriteTemplate_ArrowRedDown = {
    .tileTag = TAG_ARROW,
    .paletteTag = TAG_ARROW,
    .oam = &sOam_ArrowIconDown,
    .anims = sSpriteAnimTable_ScrollArrowIndicatorDown,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallback_ScrollingArrowDown, 
};

const struct SpriteTemplate sSpriteTemplate_ArrowRedUp = {
    .tileTag = TAG_ARROW,
    .paletteTag = TAG_ARROW,
    .oam = &sOam_ArrowIconUp,
    .anims = sSpriteAnimTable_ScrollArrowIndicatorUp,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallback_ScrollingArrowUp, 
};

const struct SpriteTemplate sSpriteTemplate_CustomIcon = 
{
    .tileTag = TAG_CUSTOM_ICON,
    .paletteTag = TAG_CUSTOM_ICON,
    .oam = &sOam_Icon,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

enum{BG_0, BG_1, BG_2, BG_3};

static const struct BgTemplate sBgTemplates_QuestMenu[4] = {
    [BG_0] = {
        .bg = BG_0,
        .charBaseIndex = 0,
        .mapBaseIndex = 21,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    [BG_1] = {
        .bg = BG_1,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
    [BG_2] = {
        .bg = BG_2,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0,
    },
    [BG_3] = {
        .bg = BG_3,
        .charBaseIndex = 3,
        .mapBaseIndex = 8*3 + 2,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0,
    },
};

#define WINDOW_TEXT_BG                      0
#define WINDOW_TEXT_PALETTE                 15
#define WINDOW_TEXT_INFO_FILTER_PALETTE     14
#define WINDOW_TEXT_QUEST_PALETTE           13
#define HEADER_QUEST_WIDTH                  14
#define HEADER_HEIGHT                       3
#define HEADER_INFO_WIDTH                   11
#define QUEST_WIDTH                         20
#define QUEST_HEIGHT                        6
#define QQNAME_WIDHT                        20
#define QQNAME_HEIGHT                       3
#define QQNPCMAP_WIDTH                      24
#define QQNPCMAP_HEIGHT                     3
#define QQPROGRESS_WIDTH                    24
#define QQPROGRESS_HEIGHT                   3
#define QQDESC_WIDTH                        24
#define QQDESC_HEIGHT                       7

#define PALETTE_FADE_INFO_FILTER_FADE       1 << WINDOW_TEXT_INFO_FILTER_PALETTE
#define PALETTE_FADE_HEADER                 (1 << WINDOW_TEXT_INFO_FILTER_PALETTE) + (1 << WINDOW_TEXT_PALETTE)

#define BASEBLOCK_HEADER_QUEST  1
#define BASEBLOCK_HEADER_INFO   BASEBLOCK_HEADER_QUEST  + HEADER_QUEST_WIDTH * HEADER_HEIGHT
#define BASEBLOCK_QUEST_1       BASEBLOCK_HEADER_INFO   + HEADER_INFO_WIDTH * HEADER_HEIGHT
#define BASEBLOCK_QUEST_2       BASEBLOCK_QUEST_1       + QUEST_WIDTH * QUEST_HEIGHT
#define BASEBLOCK_QUEST_3       BASEBLOCK_QUEST_2       + QUEST_WIDTH * QUEST_HEIGHT
#define BASEBLOCK_QQNAME        BASEBLOCK_QUEST_3       + QUEST_WIDTH * QUEST_HEIGHT
#define BASEBLOCK_QQ_NPC_MAP    BASEBLOCK_QQNAME        + QQNAME_WIDHT * QQNAME_HEIGHT
#define BASEBLOCK_QQ_PROGRESS   BASEBLOCK_QQ_NPC_MAP    + QQNPCMAP_WIDTH * QQNPCMAP_HEIGHT
#define BASEBLOCK_QQ_DESC       BASEBLOCK_QQ_PROGRESS   + QQPROGRESS_WIDTH * QQPROGRESS_HEIGHT

enum{
    WINDOW_HEADER_QUEST,
    WINDOW_HEADER_INFO,
    WINDOW_QUEST_1,
    WINDOW_QUEST_2,
    WINDOW_QUEST_3,
    WINDOW_QQUEST_TITLE,
    WINDOW_QQ_NPC_MAP,
    WINDOW_QQ_PROGRESS,
    WINDOW_QQ_DESC,
    PSS_LABEL_WINDOW_END
};

static const struct WindowTemplate sWindowTemplate_QuestMenu[] = 
{
    [WINDOW_HEADER_QUEST] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 3,
        .tilemapTop = 0,
        .width = HEADER_QUEST_WIDTH,
        .height = HEADER_HEIGHT,
        .paletteNum = WINDOW_TEXT_PALETTE,
        .baseBlock = BASEBLOCK_HEADER_QUEST,
    },

    [WINDOW_HEADER_INFO] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 18, 
        .tilemapTop = 0,
        .width = HEADER_INFO_WIDTH,
        .height = HEADER_HEIGHT,
        .paletteNum = WINDOW_TEXT_INFO_FILTER_PALETTE,
        .baseBlock = BASEBLOCK_HEADER_INFO,
    },

    [WINDOW_QUEST_1] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 3, 
        .tilemapTop = 3,
        .width = QUEST_WIDTH,
        .height = QUEST_HEIGHT,
        .paletteNum = WINDOW_TEXT_QUEST_PALETTE,
        .baseBlock = BASEBLOCK_QUEST_1,
    },

    [WINDOW_QUEST_2] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 3, 
        .tilemapTop = 3 + 5,
        .width = QUEST_WIDTH,
        .height = QUEST_HEIGHT,
        .paletteNum = WINDOW_TEXT_QUEST_PALETTE,
        .baseBlock = BASEBLOCK_QUEST_2,
    },

    [WINDOW_QUEST_3] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 3, 
        .tilemapTop = 3 + 11,
        .width = QUEST_WIDTH,
        .height = QUEST_HEIGHT,
        .paletteNum = WINDOW_TEXT_QUEST_PALETTE,
        .baseBlock = BASEBLOCK_QUEST_3,
    },

    [WINDOW_QQUEST_TITLE] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 7,
        .tilemapTop = 4,
        .width = QQNAME_WIDHT,
        .height = QQNAME_HEIGHT,
        .paletteNum = WINDOW_TEXT_PALETTE,
        .baseBlock = BASEBLOCK_QQNAME,
    },

    [WINDOW_QQ_NPC_MAP] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 3,
        .tilemapTop = 7,
        .width = QQNPCMAP_WIDTH,
        .height = QQNPCMAP_HEIGHT,
        .paletteNum = WINDOW_TEXT_PALETTE,
        .baseBlock = BASEBLOCK_QQ_NPC_MAP,
    },

    [WINDOW_QQ_PROGRESS] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 3,
        .tilemapTop = 10,
        .width = QQPROGRESS_WIDTH,
        .height = QQPROGRESS_HEIGHT,
        .paletteNum = WINDOW_TEXT_PALETTE,
        .baseBlock = BASEBLOCK_QQ_PROGRESS,
    },

    [WINDOW_QQ_DESC] = {
        .bg = WINDOW_TEXT_BG,
        .tilemapLeft = 3,
        .tilemapTop = 13,
        .width = QQDESC_WIDTH,
        .height = QQDESC_HEIGHT,
        .paletteNum = WINDOW_TEXT_PALETTE,
        .baseBlock = BASEBLOCK_QQ_DESC,
    },

    [PSS_LABEL_WINDOW_END] = DUMMY_WIN_TEMPLATE
};

static EWRAM_DATA u8 sScrollPosition = 0;
static EWRAM_DATA u8 sCursorPosition = 0;
static EWRAM_DATA u8 sCurrentDisplayedQuestFilter = QUEST_FILTER_ALL;
static EWRAM_DATA u8 sUnlockedQuestCount = 0;                       // unlocked quests of the current type
static EWRAM_DATA u8 sDisplayedQuestIds[QUEST_COUNT] = {0};
static EWRAM_DATA u8 sIconIds[3] = {0};
static EWRAM_DATA u8 sScrollArrowIds[2] = {0};
static EWRAM_DATA bool8 sScrollArrowActive[2] = {0};
static EWRAM_DATA u8 sQuestWindowId = 0;

//Inits quest menu
void CB2_InitQuestMenu()
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ClearScheduledBgCopiesToVram();
        ScanlineEffect_Stop();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();

        gMain.state++;
        break;
    case 1: 
        sCurrentDisplayedQuestFilter = QUEST_FILTER_ALL;
        QuestMenu_InitDisplayedQuests();
        QuestMenu_InitScrollAndCursorPosition();
        QuestMenu_InitBgs();
        QuestMenu_LoadGraphics();
        QuestMenu_InitTextWindows();
        QuestMenu_LoadHeaderText(TRUE);
        QuestMenu_LoadQuestText();

        gMain.state++;
        break;
    case 2:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0x10, 0, RGB_BLACK);
        QuestMenu_LoadIcons();
        QuestMenu_LoadScrollArrows();
        gMain.state++;
        break;
    case 3:
    default:
        SetMainCallback2(CB2_QuestMenu);
        SetVBlankCallback(VBlankCallback_QuestMenu);
        
        CreateTask(Task_QuestMenu_HandleInput, 0);
        break;
    }
}

static void CB2_QuestMenu()
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCallback_QuestMenu()
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void Task_QuestMenu_HandleInput(u8 taskId)
{
    if (!gPaletteFade.active 
        || (gPaletteFade.multipurpose1 != 0xFFFFFFFF && gPaletteFade.multipurpose1 !=PALETTE_FADE_INFO_FILTER_FADE))
    {
        if (JOY_REPEAT(DPAD_DOWN))
        {
            QuestMenu_TryCursorDown();
        }
        else if (JOY_REPEAT(DPAD_UP))
        {
            QuestMenu_TryCursorUp();
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            if(sUnlockedQuestCount > 0)
                gTasks[taskId].func = Task_QuestMenu_InitQuestWindow;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            ResetPaletteFade();
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].func = Task_FadeBackToOverworld;
        }
        else if(JOY_NEW(R_BUTTON))
        {
            PlaySE(SE_SELECT);
            sCurrentDisplayedQuestFilter = QuestMenu_GetNextIndex(sCurrentDisplayedQuestFilter, QUEST_FILTER_COUNT);
            gTasks[taskId].func = Task_QuestMenu_ChangeDisplayFilter;
        }
    }
}

static void Task_FadeBackToOverworld(u8 taskId)
{
    if (!gPaletteFade.active 
        || (gPaletteFade.multipurpose1 != 0xFFFFFFFF && gPaletteFade.multipurpose1 !=PALETTE_FADE_INFO_FILTER_FADE))
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
}

static void QuestMenu_InitBgs()
{
    ResetAllBgsCoordinatesAndBgCntRegs();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates_QuestMenu, ARRAY_COUNT(sBgTemplates_QuestMenu));
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(3);
}

static void QuestMenu_LoadGraphics()
{
    LZ77UnCompVram(sBgTiles_QuestMenu, (void*) BG_CHAR_ADDR(sBgTemplates_QuestMenu[BG_3].charBaseIndex));
    QuestMenu_HighlightSelectedQuest();
}

static void QuestMenu_HighlightSelectedQuest()
{
    const void* tilemap;
    const u16* palette;
    
    if(gSaveBlock1Ptr->questData[sDisplayedQuestIds[QuestMenu_GetCurrentSelectedQuest()]].completed == TRUE)
        palette = sBgPalette_QuestMenuCompleted;
    
    palette = sBgPalette_QuestMenuGeneral;

    switch(sUnlockedQuestCount)
    {
        case 0:
            tilemap = sBgMap_QuestMenu_0;
            break;
        case 1:
            tilemap = sBgMap_QuestMenu_1;
            break;
        case 2:
            if (sCursorPosition == 0)
                tilemap = sBgMap_QuestMenu_2_10;
            else
                tilemap = sBgMap_QuestMenu_2_01;
            break;
        default:
        case 3:
            switch (sCursorPosition)
            {
            default:
            case 0:
                tilemap = sBgMap_QuestMenu_3_100;
                break;
            
            case 1:
                tilemap = sBgMap_QuestMenu_3_010;
                break;

            case 2:
                tilemap = sBgMap_QuestMenu_3_001;
                break;
            }
    }

    LoadPalette(palette, 0x0, 0x20);
    LZ77UnCompVram(tilemap, (u16*) BG_SCREEN_ADDR(sBgTemplates_QuestMenu[BG_3].mapBaseIndex));
}

static void QuestMenu_InitDisplayedQuests()
{
    u8 i, j;
    
    j = 0;
    
    memset(sDisplayedQuestIds, 0, sizeof(sDisplayedQuestIds));

    for (i=0; i < ARRAY_COUNT(gSaveBlock1Ptr->questData); i++) {
        if (gSaveBlock1Ptr->questData[i].unlocked
            && sQuestDisplayFilter[sCurrentDisplayedQuestFilter](gSaveBlock1Ptr->questData[i].completed))
        {
            sDisplayedQuestIds[j] = i;
            j++;
        }
    }
}

static void QuestMenu_InitScrollAndCursorPosition()
{
    sCursorPosition = 0;
    sScrollPosition = 0;
    sUnlockedQuestCount = QuestMenu_GetUnlockedFilterQuestCount();
}

static u8 QuestMenu_GetUnlockedFilterQuestCount()
{
    u8 i, count;

    for (i=0, count=0; i< ARRAY_COUNT(gSaveBlock1Ptr->questData); i++)
        if (gSaveBlock1Ptr->questData[i].unlocked
            && sQuestDisplayFilter[sCurrentDisplayedQuestFilter](gSaveBlock1Ptr->questData[i].completed))
            count++;

    return count;
}      

static void QuestMenu_InitTextWindows()
{
    InitWindows(sWindowTemplate_QuestMenu);
    DeactivateAllTextPrinters();
    LoadPalette(GetOverworldTextboxPalettePtr(), WINDOW_TEXT_QUEST_PALETTE * 16, 0x20);
    LoadPalette(GetOverworldTextboxPalettePtr(), WINDOW_TEXT_INFO_FILTER_PALETTE * 16, 0x20);
    LoadPalette(GetOverworldTextboxPalettePtr(), WINDOW_TEXT_PALETTE * 16, 0x20);
}

static void QuestMenu_LoadHeaderText(bool8 isMainPage)
{
    if (isMainPage)
    {   
        StringExpandPlaceholders(gStringVar4, sText_Guide);
        QuestMenu_LoadWindowText(WINDOW_HEADER_INFO, 0, 2, 5, sFontColor_QuestMenuStd);
        StringExpandPlaceholders(gStringVar4, sQuestMenuShowingTexts[sCurrentDisplayedQuestFilter]);
        QuestMenu_LoadWindowText(WINDOW_HEADER_QUEST, 1, 1, 4, sFontColor_QuestMenuStd);
    }
    else
    {
        StringExpandPlaceholders(gStringVar4, sText_QuestInfoTitle);
        QuestMenu_LoadWindowText(WINDOW_HEADER_QUEST, 1, 1, 4, sFontColor_QuestMenuStd);
    }
}

static void QuestMenu_LoadQuestText()
{
    if (sUnlockedQuestCount > 0)
        QuestMenu_PrintQuestString(sDisplayedQuestIds[sScrollPosition], 0);

    if (sUnlockedQuestCount > 1)
        QuestMenu_PrintQuestString(sDisplayedQuestIds[sScrollPosition + 1], 1);

    if (sUnlockedQuestCount > 2)
        QuestMenu_PrintQuestString(sDisplayedQuestIds[sScrollPosition + 2], 2);
}

static void QuestMenu_PrintQuestString(u8 questId, u8 slotId)
{
    StringExpandPlaceholders(gStringVar4, gQuestList[questId].textTitle);
    StringAppend(gStringVar4, gText_NewLine2);
    StringAppend(gStringVar4, gQuestList[questId].textMap);
    StringAppend(gStringVar4, gText_NewLine2);

    switch (slotId)
    {
    case 0:
    default:
        QuestMenu_LoadWindowText(WINDOW_QUEST_1, 1, 0, 5, sFontColor_QuestMenuStd);
        break;
    case 1:
        QuestMenu_LoadWindowText(WINDOW_QUEST_2, 1, 0, 10, sFontColor_QuestMenuStd);
        break;
    case 2:
        QuestMenu_LoadWindowText(WINDOW_QUEST_3, 1, 0, 7, sFontColor_QuestMenuStd);
        break;
    }
}

static void QuestMenu_LoadWindowText(u8 windowId, u8 font, u8 left, u8 top, const u8* color)
{
    FillWindowPixelBuffer(windowId, 0);
    PutWindowTilemap(windowId);
    AddTextPrinterParameterized3(windowId, font, left, top, color, -1, gStringVar4);
    CopyWindowToVram(windowId, 3);
}

static u8 QuestMenu_GetCurrentSelectedQuest()
{
    return sScrollPosition + sCursorPosition;
}

static bool8 QuestMenu_TryCursorDown()
{
    if (QuestMenu_GetCurrentSelectedQuest() + 1 < sUnlockedQuestCount)
    {
        if (sCursorPosition < 2)
        {
            sCursorPosition++;
        }
        else
        {
            sScrollPosition++;
            QuestMenu_LoadQuestText();
            QuestMenu_FreeSprites();
            QuestMenu_DestroyScrollArrows();
            QuestMenu_LoadIcons();
            QuestMenu_LoadScrollArrows();
        }

        PlaySE(SE_RG_BAG_CURSOR);
        QuestMenu_HighlightSelectedQuest();

        return TRUE;
    }

    return FALSE;
}

static bool8 QuestMenu_TryCursorUp()
{
    if (QuestMenu_GetCurrentSelectedQuest() > 0)
    {
        if (sCursorPosition > 0)
        {
            sCursorPosition--;
        }
        else
        {
            sScrollPosition--;
            QuestMenu_LoadQuestText();
            QuestMenu_FreeSprites();
            QuestMenu_DestroyScrollArrows();
            QuestMenu_LoadIcons();
            QuestMenu_LoadScrollArrows();
        }

        PlaySE(SE_RG_BAG_CURSOR);
        QuestMenu_HighlightSelectedQuest();

        return TRUE;
    }
    
    return FALSE;
}

#define ICON_POS_X    200
#define ICON_POS_Y    41
#define ICON_Y_PADDING  45
#define PAGE_ICON_POS_X 36
#define PAGE_ICON_POS_Y 36

static void QuestMenu_LoadPageIcon(u8 questId)
{
    if (gQuestList[questId].hasCustomIcon)
    {
        LoadCompressedSpriteSheet(gQuestList[questId].spriteSheet);
        LoadSpritePalette(gQuestList[questId].spritePalette);
        sIconIds[0] = CreateSprite(gQuestList[questId].spriteTemplate, PAGE_ICON_POS_X, PAGE_ICON_POS_Y, 0);
    }
    else
    {
        sIconIds[0] = AddPseudoObjectEvent(gQuestList[questId].npcIconId, SpriteCallbackDummy, PAGE_ICON_POS_X, PAGE_ICON_POS_Y, 0);
    }
}

static void QuestMenu_LoadIcons()
{
    if (sUnlockedQuestCount > 0)
    {
        if (gQuestList[sDisplayedQuestIds[sScrollPosition]].hasCustomIcon)
            QuestMenu_LoadCustomIcon(gQuestList[sDisplayedQuestIds[sScrollPosition]].spriteSheet, 
                                    gQuestList[sDisplayedQuestIds[sScrollPosition]].spritePalette, 
                                    gQuestList[sDisplayedQuestIds[sScrollPosition]].spriteTemplate, 0);
        else
            QuestMenu_LoadOverworldIcon(gQuestList[sDisplayedQuestIds[sScrollPosition]].npcIconId, 0);
    }
    if (sUnlockedQuestCount > 1)
    {
        if (gQuestList[sDisplayedQuestIds[sScrollPosition]].hasCustomIcon)
            QuestMenu_LoadCustomIcon(gQuestList[sDisplayedQuestIds[sScrollPosition + 1]].spriteSheet, 
                                    gQuestList[sDisplayedQuestIds[sScrollPosition + 1]].spritePalette, 
                                    gQuestList[sDisplayedQuestIds[sScrollPosition + 1]].spriteTemplate, 1);
        else
            QuestMenu_LoadOverworldIcon(gQuestList[sDisplayedQuestIds[sScrollPosition + 1]].npcIconId, 1);
    }
    if (sUnlockedQuestCount > 2)
    {
        if (gQuestList[sDisplayedQuestIds[sScrollPosition]].hasCustomIcon)
            QuestMenu_LoadCustomIcon(gQuestList[sDisplayedQuestIds[sScrollPosition + 2]].spriteSheet, 
                                    gQuestList[sDisplayedQuestIds[sScrollPosition + 2]].spritePalette, 
                                    gQuestList[sDisplayedQuestIds[sScrollPosition + 2]].spriteTemplate, 2);
        else
            QuestMenu_LoadOverworldIcon(gQuestList[sDisplayedQuestIds[sScrollPosition + 2]].npcIconId, 2);
    }
}

static void QuestMenu_LoadOverworldIcon(u8 objectEventId, u8 slot)
{
    sIconIds[slot] = AddPseudoObjectEvent(objectEventId, SpriteCallbackDummy, ICON_POS_X, ICON_POS_Y + slot * ICON_Y_PADDING, 0);
}

static void QuestMenu_LoadCustomIcon(const struct CompressedSpriteSheet* spriteSheet, const struct SpritePalette* spritePalette, const struct SpriteTemplate* spriteTemplate, u8 slot)
{
    LoadCompressedSpriteSheet(spriteSheet);
    LoadSpritePalette(spritePalette);
    sIconIds[slot] = CreateSprite(spriteTemplate, ICON_POS_X, ICON_POS_Y + slot * ICON_Y_PADDING, 0);
}

static void QuestMenu_FreeSprites()
{
    u8 i;

    for (i=0; i<ARRAY_COUNT(sIconIds); i++)
    {
        DestroySprite(&gSprites[sIconIds[i]]);
    }
}

static bool8 QuestMenu_IsMenuScrollableDown()
{
    if (sScrollPosition < (sUnlockedQuestCount - 3))
        return TRUE;
    return FALSE;
}

static bool8 QuestMenu_IsMenuScrollableUp()
{
    if (sScrollPosition > 0)
        return TRUE;
    return FALSE;
}

#define SCROLL_ARROW_X      226
#define SCROLL_ARROW_UP_Y    35
#define SCROLL_ARROW_DOWN_Y  146

static void QuestMenu_LoadScrollArrows()
{
    bool8 scrollableDown = QuestMenu_IsMenuScrollableDown(),
          scrollableUp = QuestMenu_IsMenuScrollableUp();

    if(scrollableDown || scrollableUp)
    {
        LoadCompressedSpriteSheet(&sSpriteSheet_ArrowIcon);
        LoadSpritePalette(&sSpritePalette_ArrowIcon);
    }
    if (scrollableDown)
    {
        sScrollArrowActive[0] = TRUE;
        sScrollArrowIds[0] =  CreateSprite(&sSpriteTemplate_ArrowRedDown, SCROLL_ARROW_X, SCROLL_ARROW_DOWN_Y, 0);
    }
    if (scrollableUp)
    {
        sScrollArrowActive[1] = TRUE;
        sScrollArrowIds[1] =  CreateSprite(&sSpriteTemplate_ArrowRedUp, SCROLL_ARROW_X, SCROLL_ARROW_UP_Y, 0);
    }
}

static void QuestMenu_DestroyScrollArrows()
{
    u8 i;

    for (i=0; i<ARRAY_COUNT(sScrollArrowActive); i++)
        if (sScrollArrowActive[i])
        {
            sScrollArrowActive[i] = FALSE;
            DestroySprite(&gSprites[sScrollArrowIds[i]]);
        }
}

#define sTimer data[0]
#define ARROW_FREQ  6
#define Y_MOVEMENT_OFFSET   4

static void SpriteCallback_ScrollingArrowDown(struct Sprite* sprite)
{
    if (sprite->sTimer % ARROW_FREQ == 0)
    {
        if (sprite->sTimer / ARROW_FREQ < Y_MOVEMENT_OFFSET)
            sprite->y++;
        else if (sprite->sTimer / ARROW_FREQ < Y_MOVEMENT_OFFSET * 2)
            sprite->y--;
        else
            sprite->sTimer = -1;
    }

    sprite->sTimer++;
}

static void SpriteCallback_ScrollingArrowUp(struct Sprite* sprite)
{
    if (sprite->sTimer % ARROW_FREQ == 0)
    {
        if (sprite->sTimer / ARROW_FREQ < Y_MOVEMENT_OFFSET)
            sprite->y--;
        else if (sprite->sTimer / ARROW_FREQ < Y_MOVEMENT_OFFSET * 2)
            sprite->y++;
        else
            sprite->sTimer = -1;
    }

    sprite->sTimer++;
}

#undef sTimer

static void Task_QuestMenu_InitQuestWindow(u8 taskId)
{
    ResetPaletteFade();
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
    gMain.state = 0;
    SetMainCallback2(CB2_InitQuestWindow);
    DestroyTask(taskId);
}

static void CB2_InitQuestWindow()
{
    if (!UpdatePaletteFade())
    {
        switch(gMain.state)
        {
            case 0:
                SetVBlankHBlankCallbacksToNull();
                ClearScheduledBgCopiesToVram();
                ScanlineEffect_Stop();
                FreeAllSpritePalettes();
                ResetPaletteFade();
                ResetSpriteData();
                ResetTasks();
                gMain.state++;
                break;
            case 1:
                ResetWindows();
                QuestMenu_LoadQuestWindowBgData();
                gMain.state++;
                break;
            case 2:
                QuestMenu_LoadQuestPageText(sDisplayedQuestIds[QuestMenu_GetCurrentSelectedQuest()]);
                gMain.state++;
                break;
            case 3:
                BeginNormalPaletteFade(0xFFFFFFFF, 0, 0x10, 0, RGB_BLACK);
                QuestMenu_LoadPageIcon(sDisplayedQuestIds[QuestMenu_GetCurrentSelectedQuest()]);
                SetMainCallback2(CB2_QuestMenu);
                SetVBlankCallback(VBlankCallback_QuestMenu);
                CreateTask(Task_HandlePageInput, 0);
                break;
        }
    }
}

static void QuestMenu_LoadQuestWindowBgData()
{
    LZ77UnCompVram(sBgTiles_QuestWindow, (void*) BG_CHAR_ADDR(sBgTemplates_QuestMenu[BG_3].charBaseIndex));
    LZ77UnCompVram(sBgMap_QuestWindow, (u16*) BG_SCREEN_ADDR(sBgTemplates_QuestMenu[BG_3].mapBaseIndex));
    LoadPalette(sBgPalette_QuestWindow, 0x0, 0x20);
}

static void ResetWindows()
{
    u8 i;
    //NOTE: DO NOT RESET WINDOW BUFFERS IT WILL GET BUGGED -> DON'T USE FreeAllWindowBuffers 
    DeactivateAllTextPrinters();
    for (i = 0; i < PSS_LABEL_WINDOW_END; i++)
    {
        ClearWindowTilemap(i);
    }
    ScheduleBgCopyTilemapToVram(0);
}

static void QuestMenu_LoadQuestPageText(u8 questId)
{
    QuestMenu_LoadHeaderText(FALSE);
    QuestMenu_LoadQuestPageTitleText(questId);
    QuestMenu_LoadQuestPageNpcAndMapText(questId);
    QuestMenu_LoadQuestPageProgress(questId);
    QuestMenu_LoadPageDescription(questId);
}

static void QuestMenu_LoadQuestPageTitleText(u8 questId)
{
    StringExpandPlaceholders(gStringVar4, gQuestList[questId].textTitle);
    QuestMenu_LoadWindowText(WINDOW_QQUEST_TITLE, 1, 1, 1, sFontColor_QuestMenuPage);
}

static void QuestMenu_LoadQuestPageNpcAndMapText(u8 questId)
{
    StringExpandPlaceholders(gStringVar4, gQuestList[questId].textNpc);
    StringAppend(gStringVar4, sText_SpacerBar);
    StringAppend(gStringVar4, gQuestList[questId].textMap);
    QuestMenu_LoadWindowText(WINDOW_QQ_NPC_MAP, 2, 5, 5, sFontColor_QuestMenuPage);
}

static void QuestMenu_LoadQuestPageProgress(u8 questId)
{
    u8* dest;

    if (gSaveBlock1Ptr->questData[questId].completed == TRUE)
    {
        StringExpandPlaceholders(gStringVar4, sText_QuestCompleted);
    }
    else
    {
        dest = StringExpandPlaceholders(gStringVar4, sText_QuestInProgress);

        if (gQuestList[questId].maxProgress > 1)
        {
            QuestMenu_AddQuestProgressText(questId, 2, dest);
        }
    }

    QuestMenu_LoadWindowText(WINDOW_QQ_PROGRESS, 2, 3, 5, sFontColor_QuestMenuPage);    
}

static void QuestMenu_AddQuestProgressText(u8 questId, u8 padding, u8* dest)
{
    u8 i;

    for (i=0; i<padding; i++)
    {
        *dest++ = CHAR_SPACE;
    }

    dest = ConvertIntToDecimalStringN(dest, gSaveBlock1Ptr->questData[questId].progress, STR_CONV_MODE_LEFT_ALIGN, 5);
    *dest++ = CHAR_SLASH;
    dest = ConvertIntToDecimalStringN(dest, gQuestList[questId].maxProgress, STR_CONV_MODE_LEFT_ALIGN, 5);
    *dest++ = EOS;
}

static void QuestMenu_LoadPageDescription(u8 questId)
{
    StringExpandPlaceholders(gStringVar4, gQuestList[questId].textDesc);
    QuestMenu_LoadWindowText(WINDOW_QQ_DESC, 0, 5, 2, sFontColor_QuestMenuPage);
}

static void Task_HandlePageInput(u8 taskId)
{
    if (JOY_NEW(B_BUTTON) && !gPaletteFade.active)
    {
        PlaySE(SE_SELECT);
        SetMainCallback2(CB2_ReturnToQuestMenuMainPage);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
        DestroyTask(taskId);
    }
}

static void CB2_ReturnToQuestMenuMainPage()
{
    if (!UpdatePaletteFade())
    {
        switch (gMain.state)
        {
        case 0:
            SetVBlankHBlankCallbacksToNull();
            FreeAllSpritePalettes();
            ResetPaletteFade();
            ResetSpriteData();
            ResetTasks();

            gMain.state++;
            break;
        case 1:
            ResetWindows();
            
            gMain.state++;
            break;
        case 2:
            QuestMenu_LoadGraphics();
            QuestMenu_LoadHeaderText(TRUE);
            QuestMenu_LoadQuestText();
            gMain.state++;
            break;
        default:
        case 3:
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0x10, 0, RGB_BLACK);
            QuestMenu_LoadIcons();
            QuestMenu_LoadScrollArrows();
            SetMainCallback2(CB2_QuestMenu);
            SetVBlankCallback(VBlankCallback_QuestMenu);
            CreateTask(Task_QuestMenu_HandleInput, 0);
            break;
        }
    }
}

#define RGB_BACKGROUND RGB(4, 4, 4)

void CB2_ChangeDisplayFilter()
{
    if (!UpdatePaletteFade())
    {
        switch (gMain.state)
        {
        case 0:
            FreeAllSpritePalettes();
            ResetPaletteFade();
            ResetSpriteData();
            ResetTasks();
            gMain.state++;
            break;
        case 1: 
            QuestMenu_InitDisplayedQuests();
            QuestMenu_InitScrollAndCursorPosition();
            ResetWindows();
            gMain.state++;
            break;
        case 2:
            QuestMenu_HighlightSelectedQuest();
            QuestMenu_LoadHeaderText(TRUE);
            QuestMenu_LoadQuestText();
            gMain.state++;
            break;
        case 3:
        default:
            BeginNormalPaletteFade(PALETTE_FADE_HEADER, 0, 0x10, 0, RGB_BACKGROUND);
            QuestMenu_LoadIcons();
            QuestMenu_LoadScrollArrows();
            SetMainCallback2(CB2_QuestMenu);
            CreateTask(Task_QuestMenu_HandleInput, 0);
            break;
        }
    }
}

static void Task_QuestMenu_ChangeDisplayFilter(u8 taskId)
{
    ResetPaletteFade();
    BeginNormalPaletteFade(PALETTE_FADE_HEADER, 0, 0, 0x10, RGB_BACKGROUND);
    SetMainCallback2(CB2_ChangeDisplayFilter);
    DestroyTask(taskId);
}

const u8 QuestMenu_GetNextIndex(u8 currentValue, u8 totalValues)
{
    if(currentValue + 1 < totalValues)
        return currentValue + 1;
    return 0;
}

const bool8 QuestMenu_DiplayFilter_All(u8 completed){
    return TRUE;
}

const bool8 QuestMenu_DiplayFilter_InProgress(u8 completed){
    return completed == FALSE;
}

const bool8 QuestMenu_DiplayFilter_Completed(u8 completed){
    return completed == TRUE;
}

static const struct WindowTemplate sWindowTemplate_QuestWindow = 
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 14,
    .height = 3,
    .paletteNum = 0xF,
    .baseBlock = 8,
};

static const u8 sFontColor_StartMenuWindowQuest[3] = {0, 2, 3};

static u8 GetDigitsNumber(u16 num)
{
    u8 count = 1;

    while (num >= 10)
    {
        num /= 10;
        count++;
    }

    return count;
}

void ShowQuestWindow(u16 questId, u8 type)
{
    sQuestWindowId = AddWindow(&sWindowTemplate_QuestWindow);
    PutWindowTilemap(sQuestWindowId);
    DrawStdWindowFrame(sQuestWindowId, FALSE);

    switch (type)
    {
    case QUEST_UPDATE_NEW:
        StringCopy(gStringVar1, gQuestList[questId].textTitle);
        StringExpandPlaceholders(gStringVar4, sText_Update_New);
        break;
    
    case QUEST_UPDATE_PROGRESS:
        StringCopy(gStringVar1,  gQuestList[questId].textTitle);
        ConvertIntToDecimalStringN(gStringVar2, gSaveBlock1Ptr->questData[questId].progress, STR_CONV_MODE_RIGHT_ALIGN, GetDigitsNumber(gSaveBlock1Ptr->questData[questId].progress));
        ConvertIntToDecimalStringN(gStringVar3, gQuestList[questId].maxProgress, STR_CONV_MODE_RIGHT_ALIGN, GetDigitsNumber(gQuestList[questId].maxProgress));
        StringExpandPlaceholders(gStringVar4, sText_Update_Progress);
        break;
    
    case QUEST_UPDATE_COMPLETED:
        StringCopy(gStringVar1, gQuestList[questId].textTitle);
        StringExpandPlaceholders(gStringVar4, sText_Update_Completed);
        break;
    }

    AddTextPrinterParameterized4(sQuestWindowId, 8, 0, 0, 0, 4, sFontColor_StartMenuWindowQuest, 0xFF, gStringVar4);
    CopyWindowToVram(sQuestWindowId, 2);
}

void HideQuestWindow()
{
    ClearStdWindowAndFrameToTransparent(sQuestWindowId, FALSE);
    CopyWindowToVram(sQuestWindowId, 2);
    RemoveWindow(sQuestWindowId);
}

 /* * * * * * * * * * * * * * * * * *
  * * * QUEST SCRIPTING CODE! * * * *
  * * * * * * * * * * * * * * * * * */

// Unlocks given quest and returns TRUE.
// Returns false if quest is already unlocked or if it does not exist

bool8 QuestMenu_UnlockQuest(u8 questId)
{
    if (questId < ARRAY_COUNT(gSaveBlock1Ptr->questData) &&
        !gSaveBlock1Ptr->questData[questId].unlocked)
    {
        gSaveBlock1Ptr->questData[questId].unlocked = TRUE;
        return TRUE;
    }
    return FALSE;
}

// Completes given quest and returns TRUE.
// Returns false if quest is already completed or if it does not exist

bool8 QuestMenu_CompleteQuest(u8 questId)
{
    if (questId < ARRAY_COUNT(gSaveBlock1Ptr->questData) &&
        !gSaveBlock1Ptr->questData[questId].completed)
    {
        gSaveBlock1Ptr->questData[questId].completed = TRUE;
        return TRUE;
    }

    return FALSE;
}

// Checks if the given quest is unlocked

bool8 QuestMenu_IsQuestUnlocked(u8 questId)
{
    if (questId < ARRAY_COUNT(gSaveBlock1Ptr->questData) &&
        gSaveBlock1Ptr->questData[questId].unlocked)
    {
        return TRUE;
    }

    return FALSE;
}

// Checks if the given quest is completed

bool8 QuestMenu_IsQuestCompleted(u8 questId)
{
    if (questId < ARRAY_COUNT(gSaveBlock1Ptr->questData) &&
        gSaveBlock1Ptr->questData[questId].completed)
    {
        return TRUE;
    }

    return FALSE;
}

// Returns quest current progress

u16 QuestMenu_GetQuestCurrentProgress(u8 questId)
{
    if (questId < ARRAY_COUNT(gSaveBlock1Ptr->questData))
        return gSaveBlock1Ptr->questData[questId].progress;

    return 0;
}

// Returns quest max progress
u16 QuestMenu_GetMaxProgress(u8 questId)
{
    if (questId < ARRAY_COUNT(gQuestList))
        return gQuestList[questId].maxProgress;

    return 0;
}

// Returns a pointer to quest title text
const u8* QuestMenu_GetQuestTitleText(u8 questId)
{
    if (questId < ARRAY_COUNT(gQuestList))
        return gQuestList[questId].textTitle;

    return gQuestList[0].textTitle;
}

// Returns a pointer to quest npc name text
const u8* QuestMenu_GetQuestNpcText(u8 questId)
{
    if (questId < ARRAY_COUNT(gQuestList))
        return gQuestList[questId].textNpc;

    return gQuestList[0].textNpc;
}

// Returns a pointer to quest map text
const u8* QuestMenu_GetQuestMapText(u8 questId)
{
    if (questId < ARRAY_COUNT(gQuestList))
        return gQuestList[questId].textMap;

    return gQuestList[0].textMap;
}

// Returns number of completed quest
u16 QuestMenu_GetCompletedQuestCount()
{
    u32 i;
    u16 c;

    for (i = 0, c = 0; i < ARRAY_COUNT(gSaveBlock1Ptr->questData); i++)
        if (gSaveBlock1Ptr->questData[i].completed)
            c++;
    return c;
}

// Returns number of quest in progress
u16 QuestMenu_GetInProgressQuestCount()
{
    u32 i;
    u16 c;

    for (i = 0, c = 0; i < ARRAY_COUNT(gSaveBlock1Ptr->questData); i++)
        if (gSaveBlock1Ptr->questData[i].unlocked && !gSaveBlock1Ptr->questData[i].completed)
            c++;
    return c;
}

// Returns number of unlocked quests
u16 QuestMenu_GetUnlockedQuestCount()
{
    u32 i;
    u16 c;

    for (i = 0, c = 0; i < ARRAY_COUNT(gSaveBlock1Ptr->questData); i++)
        if (gSaveBlock1Ptr->questData[i].unlocked)
            c++;
    return c;
}

bool8 QuestMenu_AddProgress(u16 questId, u16 amount)
{
    if (gSaveBlock1Ptr->questData[questId].unlocked 
        && !gSaveBlock1Ptr->questData[questId].completed 
        && gQuestList[questId].maxProgress > 1)
    {
        if ((gSaveBlock1Ptr->questData[questId].progress + amount) < gQuestList[questId].maxProgress)
            gSaveBlock1Ptr->questData[questId].progress += amount;
        else
            gSaveBlock1Ptr->questData[questId].progress = gQuestList[questId].maxProgress;
        return TRUE;
    }
    return FALSE;
}

bool8 QuestMenu_SubProgress(u16 questId, u16 amount)
{
    if (gSaveBlock1Ptr->questData[questId].unlocked
        && !gSaveBlock1Ptr->questData[questId].completed
        && gQuestList[questId].maxProgress > 1)
    {
        if (gSaveBlock1Ptr->questData[questId].progress > amount)
            gSaveBlock1Ptr->questData[questId].progress -= amount;
        else
            gSaveBlock1Ptr->questData[questId].progress = 0;
        return TRUE;
    }
    return FALSE;
}

bool8 QuestMenu_SetProgress(u16 questId, u16 amount)
{
    if (gSaveBlock1Ptr->questData[questId].unlocked
        && !gSaveBlock1Ptr->questData[questId].completed
        && gQuestList[questId].maxProgress > 1)
    {
        if (amount <= gQuestList[questId].maxProgress)
            gSaveBlock1Ptr->questData[questId].progress = amount;
        else
            gSaveBlock1Ptr->questData[questId].progress = gQuestList[questId].maxProgress;
        return TRUE;
    }
    return FALSE;
}
