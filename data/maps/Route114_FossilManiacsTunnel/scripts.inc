Route114_FossilManiacsTunnel_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route114_FossilManiacsTunnel_OnTransition
	map_script MAP_SCRIPT_ON_LOAD, Route114_FossilManiacsTunnel_OnLoad
	.byte 0

Route114_FossilManiacsTunnel_OnTransition:
	call_if_set FLAG_SYS_GAME_CLEAR, Route114_FossilManiacsTunnel_EventScript_MoveFossilManiac
	end

Route114_FossilManiacsTunnel_EventScript_MoveFossilManiac::
	setobjectxyperm LOCALID_FOSSIL_MANIAC, 6, 5
	setobjectmovementtype LOCALID_FOSSIL_MANIAC, MOVEMENT_TYPE_FACE_DOWN
	return

Route114_FossilManiacsTunnel_OnLoad:
	call_if_unset FLAG_SYS_GAME_CLEAR, Route114_FossilManiacsTunnel_EventScript_CloseDesertUnderpass
	end

Route114_FossilManiacsTunnel_EventScript_CloseDesertUnderpass::
	setmetatile 6, 1, METATILE_Fallarbor_RedRockWall, TRUE
	setmetatile 6, 2, METATILE_Fallarbor_RedRockWall, TRUE
	return

Route114_FossilManiacsTunnel_EventScript_ManiacMentionCaveIn::
	lockall
	applymovement LOCALID_FOSSIL_MANIAC, Common_Movement_WalkInPlaceFasterUp
	applymovement LOCALID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	msgbox Route114_FossilManiacsTunnel_Text_NotSafeThatWay, MSGBOX_DEFAULT
	setvar VAR_FOSSIL_MANIAC_STATE, 2
	releaseall
	end

Route114_FossilManiacsTunnel_EventScript_FossilManiac::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_REVIVED_FOSSIL_MON, Route114_FossilManiacsTunnel_EventScript_PlayerRevivedFossil
	checkitem ITEM_ROOT_FOSSIL
	goto_if_eq VAR_RESULT, TRUE, Route114_FossilManiacsTunnel_EventScript_PlayerHasFossil
	checkitem ITEM_CLAW_FOSSIL
	goto_if_eq VAR_RESULT, TRUE, Route114_FossilManiacsTunnel_EventScript_PlayerHasFossil
	msgbox Route114_FossilManiacsTunnel_Text_LookInDesertForFossils, MSGBOX_DEFAULT
	release
	end

Route114_FossilManiacsTunnel_EventScript_PlayerHasFossil::
	msgbox Route114_FossilManiacsTunnel_Text_DevonCorpRevivingFossils, MSGBOX_DEFAULT
	release
	end

Route114_FossilManiacsTunnel_EventScript_PlayerRevivedFossil::
	msgbox Route114_FossilManiacsTunnel_Text_FossilsAreWonderful, MSGBOX_DEFAULT
	release
	end

Route114_FossilManiacsTunnel_Text_LookInDesertForFossils:
	.string "{JPN}ぼく かせきマニア{0xB0}{0xB0}\n"
	.string "かせき だいすき ナイスガイ さ{0xB0}{0xB0}\p"
	.string "きみも カセキが ほしいの{0xB0}{0xB0}？\p"
	.string "ここで みつかる カセキは\n"
	.string "ぜんぶ ぼくの ものだから あげないよ{0xB0}{0xB0}\p"
	.string "どうしても ほしければ\n"
	.string "カセキの みつかりそうな\l"
	.string "いわや すなが おおい さばくでも\l"
	.string "さがして みなよ{0xB0}{0xB0}$"

Route114_FossilManiacsTunnel_Text_DevonCorpRevivingFossils:
	.string "{JPN}きみも カセキを みつけたんだね{0xB0}{0xB0}\n"
	.string "すてき{0xB0}{0xB0} うっとり しちゃうよ{0xB0}{0xB0}\p"
	.string "きみ その カセキ どうするの{0xB0}{0xB0}？\p"
	.string "きいた はなしでは デボンでは\n"
	.string "カセキを ポケモンに いきかえらせる\l"
	.string "けんきゅうを している そうだよ{0xB0}{0xB0}\p"
	.string "ぼくは カセキが すきだらか\n"
	.string "そんなこと しないけどね{0xB0}{0xB0}$"

Route114_FossilManiacsTunnel_Text_FossilsAreWonderful:
	.string "{JPN}カセキって{0xB0}{0xB0}\n"
	.string "すてき{0xB0}{0xB0} うっとり しちゃうよ{0xB0}{0xB0}$"

Route114_FossilManiacsTunnel_Text_NotSafeThatWay:
	.string "{JPN}あ{0xB0}{0xB0}\n"
	.string "そっちに いくと あぶないよ{0xB0}{0xB0}\p"
	.string "ほりまくって いたら\n"
	.string "かべが くずれて しまったんだ{0xB0}{0xB0}\p"
	.string "したに おおきな くうどうが\n"
	.string "あるみたい だけど{0xB0}{0xB0}\l"
	.string "カセキも なさそう だから\l"
	.string "そのままに してあるんだ{0xB0}{0xB0}$"

