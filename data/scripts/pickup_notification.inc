EventScript_PickupNotification::
	lockall
EventScript_PickupNotification_Loop:
	callnative PickupNotification_PrepareNext
	goto_if_eq VAR_RESULT, FALSE, EventScript_PickupNotification_End
	msgbox EventScript_PickupNotification_Text, MSGBOX_DEFAULT
	goto EventScript_PickupNotification_Loop
EventScript_PickupNotification_End:
	releaseall
	end

EventScript_PickupNotification_Text:
	.string "{JPN}{STR_VAR_1}は {STR_VAR_2}を\n"
	.string "{JPN}ひろってきた!$"
