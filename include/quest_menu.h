#ifndef GUARD_QUEST_MENU_H
#define GUARD_QUEST_MENU_H

#define QUEST_FILTER_ALL            0
#define QUEST_FILTER_IN_PROGRESS    1
#define QUEST_FILTER_COMPLETED      2
#define QUEST_FILTER_COUNT          3

extern const struct CompressedSpriteSheet sSpriteSheet_ArrowIcon;
extern const struct CompressedSpriteSheet sSpriteSheet_CustomIcon;
extern const struct SpritePalette sSpritePalette_ArrowIcon;
extern const struct SpritePalette sSpritePalette_CustomIcon;
extern const struct OamData sOam_ArrowIconDown;
extern const struct OamData sOam_ArrowIconUp;
extern const struct OamData sOam_Icon;
extern const struct SpriteTemplate sSpriteTemplate_ArrowRedDown;
extern const struct SpriteTemplate sSpriteTemplate_ArrowRedUp;
extern const struct SpriteTemplate sSpriteTemplate_CustomIcon;

enum {QUEST_UPDATE_NEW, QUEST_UPDATE_PROGRESS, QUEST_UPDATE_COMPLETED};

struct Quest {
    u16 maxProgress;
    u8 hasCustomIcon:1;
    u8 filler:7;
    u8 npcIconId;
    const struct CompressedSpriteSheet* spriteSheet;
    const struct SpritePalette* spritePalette;
    const struct SpriteTemplate* spriteTemplate;
    const u8* textTitle;
    const u8* textNpc;
    const u8* textMap;
    const u8* textDesc;
};

void CB2_InitQuestMenu();
bool8 QuestMenu_UnlockQuest(u8 questId);
bool8 QuestMenu_CompleteQuest(u8 questId);
bool8 QuestMenu_IsQuestUnlocked(u8 questId);
bool8 QuestMenu_IsQuestCompleted(u8 questId);
u16 QuestMenu_GetQuestCurrentProgress(u8 questId);
const u8* QuestMenu_GetQuestTitleText(u8 questId);
const u8* QuestMenu_GetQuestNpcText(u8 questId);
const u8* QuestMenu_GetQuestMapText(u8 questId);
u16 QuestMenu_GetMaxProgress(u8 questId);
u16 QuestMenu_GetCompletedQuestCount();
u16 QuestMenu_GetInProgressQuestCount();
u16 QuestMenu_GetUnlockedQuestCount();
bool8 QuestMenu_AddProgress(u16 questId, u16 amount);
bool8 QuestMenu_SubProgress(u16 questId, u16 amount);
bool8 QuestMenu_SetProgress(u16 questId, u16 amount);
void ShowQuestWindow(u16 questId, u8 type);
void HideQuestWindow();

#endif /* GUARD_QUEST_MENU_H */
