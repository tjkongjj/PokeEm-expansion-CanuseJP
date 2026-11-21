PacifidlogTown_House5_MapScripts::
	.byte 0

PacifidlogTown_House5_EventScript_MirageIslandWatcher::
	lock
	faceplayer
	msgbox PacifidlogTown_House5_Text_MirageIslandComesAndGoes, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House5_EventScript_Gentleman::
	msgbox PacifidlogTown_House5_Text_MirageIslandAppearDependingOnWeather, MSGBOX_NPC
	end

PacifidlogTown_House5_Text_MirageIslandComesAndGoes:
	.string "マボロシじまは きえたり あらわれたり するのじゃ$"

PacifidlogTown_House5_Text_MirageIslandAppearDependingOnWeather:
	.string "マボロシじま……\nおそらく しんきろうの かんけいで\lみえたり みえなかったり するんだろう\pいや ひょっとしたら ほんとうに\nあらわれたり きえたり しているのか……?$"

