Std_QuestShowWindow::
	lockall
	playfanfare MUS_SLOTS_WIN
	show_quest_window_internal VAR_0x8000, VAR_0x8001
	switch VAR_0x8001
	case UPDATE_NEW, Std_QuestShowWindow_3
	case UPDATE_PROGRESS, Std_QuestShowWindow_4
	case UPDATE_COMPLETED, Std_QuestShowWindow_5
Std_QuestShowWindow_1:
	waitfanfare
	waitmessage
	waitbuttonpress
	closemessage
	releaseall
	hide_quest_window
	return

Std_QuestShowWindow_3:
	message Std_QuestShowWindow_Text_0
	goto Std_QuestShowWindow_1

Std_QuestShowWindow_4:
	message Std_QuestShowWindow_Text_1
	goto Std_QuestShowWindow_1

Std_QuestShowWindow_5:
	quest Q_CHECK_COMPLETED_NUM, VAR_0x8000, 1, VAR_0x8002
	buffernumberstring 0, VAR_0x8002
	message Std_QuestShowWindow_Text_2
	goto Std_QuestShowWindow_1


Std_QuestShowWindow_Text_0:
	.string "A new quest has been added to the\n"
	.string "quest menu.$"

Std_QuestShowWindow_Text_1:
	.string "The quest progress was updated in the\n"
	.string "quest menu.$"

Std_QuestShowWindow_Text_2:
	.string "You completed a quest!\n"
	.string "Quests completed: {COLOR RED}{STR_VAR_1}$"
