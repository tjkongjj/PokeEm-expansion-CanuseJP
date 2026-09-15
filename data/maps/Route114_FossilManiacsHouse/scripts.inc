Route114_FossilManiacsHouse_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route114_FossilManiacsHouse_OnTransition
	.byte 0

Route114_FossilManiacsHouse_OnTransition:
	setflag FLAG_LANDMARK_FOSSIL_MANIACS_HOUSE
	end

Route114_FossilManiacsHouse_EventScript_FossilManiacsBrother::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_TM_DIG, Route114_FossilManiacsHouse_EventScript_ReceivedDig
	msgbox Route114_FossilManiacsHouse_Text_HaveThisToDigLikeMyBrother, MSGBOX_DEFAULT
	giveitem ITEM_TM_DIG
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM_DIG
	release
	end

Route114_FossilManiacsHouse_EventScript_ReceivedDig::
	msgbox Route114_FossilManiacsHouse_Text_DigReturnsYouToEntrance, MSGBOX_DEFAULT
	release
	end

Route114_FossilManiacsHouse_EventScript_RockDisplay::
	msgbox Route114_FossilManiacsHouse_Text_RocksFillDisplayCase, MSGBOX_SIGN
	end

Route114_FossilManiacsHouse_EventScript_Bookshelf::
	msgbox Route114_FossilManiacsHouse_Text_CrammedWithBooks, MSGBOX_SIGN
	end

Route114_FossilManiacsHouse_Text_HaveThisToDigLikeMyBrother:
	.string "{JPN}ぼくの にいちゃん かせきマニア{0xB0}{0xB0}\n"
	.string "かせき だいすき ナイスガイ さ{0xB0}{0xB0}\p"
	.string "ちなみに あなを ほるのも だいすきで{0xB0}{0xB0}\n"
	.string "この あなも じぶんで ほったんだ{0xB0}{0xB0}\p"
	.string "これを あげるから{0xB0}{0xB0}\n"
	.string "きみも にいちゃん みたいに\l"
	.stirng "あなを ほってみなよ{0xB0}{0xB0}$"

Route114_FossilManiacsHouse_Text_DigReturnsYouToEntrance:
	.string "{JPN}ちなみに ポケモンが あなをほると\n"
	.string "どうくつの いりぐち まで\l"
	.string "いっしゅんで もどれるんだ{0xB0}{0xB0}$"

Route114_FossilManiacsHouse_Text_RocksFillDisplayCase:
	.string "{JPN}ケースのなかに かわった かたちの いしが\n"
	.string "いくつも かざってある{0xB0}{0xB0}$"

Route114_FossilManiacsHouse_Text_CrammedWithBooks:
	.string "{JPN}『ちそうが できるまで』\n"
	.string "『あめ ふって じめん かたまる』\l"
	.string "『いしと つちと がんせきと』\p"
	.string "たくさんの ほんが ならんでる！$"

