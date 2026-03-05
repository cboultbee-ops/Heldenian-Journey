// MessageHandler.cpp: Network message handler implementations
// Extracted from Game.cpp (Fix 13)
//
//////////////////////////////////////////////////////////////////////

#include "Game.h"
#include "lan_eng.h"
#include "MessageHandler.h"

// Global variables defined in Wmain.cpp
extern char G_cSpriteAlphaDegree;
extern void MapChangeLog(const char* fmt, ...);
extern char G_cCmdLine[256], G_cCmdLineTokenA[120], G_cCmdLineTokenA_Lowercase[120], G_cCmdLineTokenB[120], G_cCmdLineTokenC[120], G_cCmdLineTokenD[120], G_cCmdLineTokenE[120];

void CMessageHandler::GameRecvMsgHandler(DWORD dwMsgSize, char * Data)
{ DWORD * dwpMsgID;
	dwpMsgID = (DWORD *)(Data + INDEX4_MSGID);
	switch (*dwpMsgID) {
	case MSGID_RESPONSE_CHARGED_TELEPORT:
		ResponseChargedTeleport(Data);
		break;

	case MSGID_RESPONSE_TELEPORT_LIST:
		ResponseTeleportList(Data);
		break;

	case MSGID_RESPONSE_NOTICEMENT:
		NoticementHandler(Data);
		break;

	case MSGID_DYNAMICOBJECT:
		DynamicObjectHandler(Data);
		break;

	case MSGID_RESPONSE_INITPLAYER:
		InitPlayerResponseHandler(Data);
		break;

	case MSGID_RESPONSE_INITDATA:
		InitDataResponseHandler(Data);
		break;

	case MSGID_RESPONSE_MOTION:
		MotionResponseHandler(Data);
		break;

	case MSGID_EVENT_COMMON:
		CommonEventHandler(Data);
		break;

	case MSGID_EVENT_MOTION:
		MotionEventHandler(Data);
		break;

	case MSGID_EVENT_LOG:
		LogEventHandler(Data);
		break;

	case MSGID_COMMAND_CHATMSG:
		ChatMsgHandler(Data);
		break;

	case MSGID_PLAYERITEMLISTCONTENTS:
		InitItemList(Data);
		m_pGame->LoadEquipSetsFromFile();
		break;

		case MSGID_PLAYERSKILLCONTENTS:
		InitSkillList(Data);
		break;
	case MSGID_NOTIFY:
		NotifyMsgHandler(Data);
		break;

	case MSGID_RESPONSE_CREATENEWGUILD:
		CreateNewGuildResponseHandler(Data);
		break;

	case MSGID_RESPONSE_DISBANDGUILD:
		DisbandGuildResponseHandler(Data);
		break;

	case MSGID_PLAYERCHARACTERCONTENTS:
		InitPlayerCharacteristics(Data);
		break;

	case MSGID_RESPONSE_CIVILRIGHT:
		CivilRightAdmissionHandler(Data);
		break;

	case MSGID_RESPONSE_RETRIEVEITEM:
		RetrieveItemHandler(Data);
		break;

	case MSGID_RESPONSE_PANNING:
		ResponsePanningHandler(Data);
		break;

	case MSGID_RESPONSE_FIGHTZONE_RESERVE:
		ReserveFightzoneResponseHandler(Data);
		break;
	}
}

void CMessageHandler::NotifyMsgHandler(char * Data)
{
	DWORD * dwp, dwTime, dwTemp;
 WORD  * wp, wEventType;
 char  * cp, cTemp[510], cTxt[120];
 short * sp, sX, sY, sV1, sV2, sV3, sV4, sV5, sV6, sV7, sV8, sV9;
 int   * ip, i, iV1, iV2, iV3, iV4;

	dwTime = timeGetTime();

	wp   = (WORD *)(Data + INDEX2_MSGTYPE);
	wEventType = *wp;

	switch (wEventType) {
	case NOTIFY_LGNPTS:
		dwp = (DWORD *)(Data + INDEX2_MSGTYPE + 2);
		m_pGame->m_cash = *dwp;
		break;

	case NOTIFY_SLATE_BERSERK:		// reversed by Snoopy: 0x0BED
		m_pGame->AddEventList( MSG_NOTIFY_SLATE_BERSERK, 10 );//"Berserk magic casted!"
		m_pGame->m_bUsingSlate = TRUE;
		break;
	
	case NOTIFY_LOTERY_LOST:		// reversed by Snoopy: 0x0BEE:	
		m_pGame->AddEventList( MSG_NOTIFY_LOTERY_LOST, 10 );//"You draw a blank. Please try again next time.."
		break;

	case NOTIFY_TELEPORT_REJECTED:
		sp = (short *)(Data + INDEX2_MSGTYPE + 2);
		sV1 = *sp; sp++;
		sV2 = *sp; sp++;
		m_pGame->m_pMapData->m_tile[sV1][sV2].m_bIsTeleport = FALSE;
		m_pGame->m_bIsTeleportRequested = false;
		m_pGame->ChangeGameMode(GAMEMODE_ONMAINGAME);
		m_pGame->AddEventList(NOTIFYMSG_TELEPORT_REJECTED, 10);
		break;

	case NOTIFY_SPEED_BUFF:
		sp = (short *)(Data + INDEX2_MSGTYPE + 2);
		sV1 = *sp; sp++; // 1=on, 0=off
		sV2 = *sp; sp++; // duration in seconds
		if (sV1 == 1) {
			m_pGame->m_bSpeedBuffActive = true;
			m_pGame->m_dwSpeedBuffEndTime = dwTime + (DWORD)(sV2 * 1000);
			m_pGame->ApplySpeedBuff(true);
			m_pGame->AddEventList("Speed buff activated!", 10);
		} else {
			m_pGame->m_bSpeedBuffActive = false;
			m_pGame->m_dwSpeedBuffEndTime = 0;
			m_pGame->ApplySpeedBuff(false);
			m_pGame->AddEventList("Speed buff expired.", 10);
		}
		break;

	case NOTIFY_FREEZE_STATE:
		sp = (short *)(Data + INDEX2_MSGTYPE + 2);
		sV1 = *sp; sp++; // 1=frozen, 0=unfrozen
		sV2 = *sp; sp++; // duration
		if (sV1 == 1) {
			m_pGame->m_iPlayerStatus |= STATUS_FROZEN;
			m_pGame->AddEventList("You have been frozen!", 10);
		} else {
			m_pGame->m_iPlayerStatus &= ~STATUS_FROZEN;
			m_pGame->AddEventList("The freeze has worn off.", 10);
		}
		break;

	case NOTIFY_SUPER_BERSERK:
		sp = (short *)(Data + INDEX2_MSGTYPE + 2);
		sV1 = *sp; sp++; // 1=activated
		sV2 = *sp; sp++; // duration
		if (sV1 == 1)
			m_pGame->AddEventList("Super Berserk activated! +20% magic damage.", 10);
		break;

	case NOTIFY_0BEF:				// 0x0BEF: // Snoopy: Crash or closes the client? (Calls SE entry !)
		// I'm noot sure at all of this function's result, so let's quit game...

		break;
	case NOTIFY_EVENTILLUSION:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sV1 = *cp;
		cp++;
		if(sV1 == 1) m_pGame->AddEventList(NOTIFYMSG_EVENTILLUSIONON, 10);
		else m_pGame->AddEventList(NOTIFYMSG_EVENTILLUSIONOFF, 10);
		break;

	case NOTIFY_EVENTTP:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sV1 = *cp;
		cp++;
		if(sV1 == 1) m_pGame->AddEventList(NOTIFYMSG_EVENTTPOPEN, 10);
		else m_pGame->AddEventList(NOTIFYMSG_EVENTTPCLOSE, 10);
		break;

	case NOTIFY_EVENTSPELL:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sV1 = *cp;
		cp++;
		sV2 = *cp;
		cp++;
		sV3 = *cp;
		cp++;
		if(sV2 < MAXMAGICTYPE && sV2 > 0){
			if(sV1 == 1){
				if(!sV3) m_pGame->m_magicDisabled[sV2] = true;
				wsprintf(cTemp, NOTIFYMSG_SPELLDISABLED, m_pGame->m_pMagicCfgList[sV2]->m_cName);
			}
			else{
				m_pGame->m_magicDisabled[sV2] = false;
				wsprintf(cTemp, NOTIFYMSG_SPELLENABLED, m_pGame->m_pMagicCfgList[sV2]->m_cName);
			}
			m_pGame->AddEventList(cTemp, 10 );
		}
		break;

	case NOTIFY_EVENTARMOR:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sV1 = *cp;
		cp++;
		sV2 = *cp;
		cp++;
		if(sV1 == 1){
			if(!sV2) m_pGame->m_armorDisabled = true;
			m_pGame->AddEventList(NOTIFYMSG_ARMORDISABLED, 10);
		}
		else {
			m_pGame->m_armorDisabled = false;
			m_pGame->AddEventList(NOTIFYMSG_ARMORENABLED, 10);
		}
		break;

	case NOTIFY_EVENTSHIELD:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sV1 = *cp;
		cp++;
		sV2 = *cp;
		cp++;
		if(sV1 == 1){
			m_pGame->AddEventList(NOTIFYMSG_SHIELDDISABLED, 10);
			if(!sV2) m_pGame->m_shieldDisabled = true;
		}
		else {
			m_pGame->AddEventList(NOTIFYMSG_SHIELDENABLED, 10);
			m_pGame->m_shieldDisabled = false;
		}
		break;

	case NOTIFY_EVENTCHAT:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sV1 = *cp;
		cp++;
		sV2 = *cp;
		cp++;
		if(sV1 == 1) m_pGame->AddEventList(NOTIFYMSG_CHATDISABLED, 10);
		else m_pGame->AddEventList(NOTIFYMSG_CHATENABLED, 10);
		break;

	case NOTIFY_EVENTPARTY:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sV1 = *cp;
		cp++;
		sV2 = *cp;
		cp++;
		if(sV1 == 1) m_pGame->AddEventList(NOTIFYMSG_PARTYDISABLED, 10);
		else m_pGame->AddEventList(NOTIFYMSG_PARTYENABLED, 10);
		break;

	case NOTIFY_EVENTRESET:
		m_pGame->m_shieldDisabled = false;
		m_pGame->m_armorDisabled = false;
		for(i = 0; i < MAXMAGICTYPE; i++)
			m_pGame->m_magicDisabled[i] = false;
		m_pGame->AddEventList(NOTIFYMSG_EVENTRESET, 10 );
		break;

	case NOTIFY_CRAFTING_SUCCESS:	//reversed by Snoopy: 0x0BF0:
		m_pGame->m_iContribution -= m_pGame->m_iContributionPrice;
		m_pGame->m_iContributionPrice = 0;
		m_pGame->DisableDialogBox(25);
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER42, 10);		// "Item manufacture success!"
		m_pGame->PlaySound('E', 23, 5);
		switch (m_pGame->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			m_pGame->PlaySound('C', 21, 0);
			break;
		case 4:
		case 5:
		case 6:
			m_pGame->PlaySound('C', 22, 0);
			break;
		}
		break;

	case NOTIFY_CRAFTING_FAIL:	
		m_pGame->AddEventList(MSG_NOTIFY_CRAFTING_FAILED, 10);		// "Crafting failed"	
		m_pGame->PlaySound('E', 24, 5);
		break;

	case NOTIFY_NOMATCHINGCRAFTING:
		m_pGame->AddEventList(MSG_NOTIFY_CRAFTING_NO_PART, 10);		// "There is not enough material"		
		m_pGame->PlaySound('E', 24, 5);
		break;

	case NOTIFY_NO_CRAFT_CONTRIB:
		m_pGame->AddEventList(MSG_NOTIFY_CRAFTING_NO_CONTRIB, 10);	// "There is not enough Contribution Point"	
		m_pGame->PlaySound('E', 24, 5);
		break;

	case NOTIFY_ANGELIC_STATS:		// reversed by Snoopy: 0x0BF2
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		ip = (int *)cp;
		m_pGame->m_angelStat[STAT_STR] = *ip;  // m_pGame->m_angelStat[STAT_STR]
		cp +=4;
		ip = (int *)cp;
		m_pGame->m_angelStat[STAT_INT] = *ip;  // m_pGame->m_angelStat[STAT_INT]
		cp +=4;
		ip = (int *)cp;
		m_pGame->m_angelStat[STAT_DEX] = *ip;  // m_pGame->m_angelStat[STAT_DEX]
		cp +=4;
		ip = (int *)cp;
		m_pGame->m_angelStat[STAT_MAG] = *ip;  // m_pGame->m_angelStat[STAT_MAG]
		break;			

	case NOTIFY_ITEM_CANT_RELEASE:	// reversed by Snoopy: 0x0BF3	
		m_pGame->AddEventList(MSG_NOTIFY_NOT_RELEASED , 10 );//"Item cannot be released"			
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		m_pGame->ItemEquipHandler(*cp);
		break;

	case NOTIFY_ANGEL_FAILED:		// reversed by Snoopy: 0x0BF4
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		ip = (int *)cp;
		iV1 = *ip; // Error reason
		switch (iV1) {
		case 1: // "BFB9BBF3C4A120BECAC0BA20B9F6B1D7C0D4B4CFB4D92E20A4D02E2EA4D0" (Stolen bytes ?)
			m_pGame->AddEventList(MSG_NOTIFY_ANGEL_FAILED , 10 ); //"Impossible to get a Tutelary Angel." // Invented by Snoopy.
			break;
		case 2: //
			m_pGame->AddEventList(MSG_NOTIFY_ANGEL_MAJESTIC , 10 );//"You need additional Majesty Points."
			break;
		case 3: //
			m_pGame->AddEventList(MSG_NOTIFY_ANGEL_LOW_LVL , 10 ); //"Only Majesty characters can receive Tutelary Angel"
			break;
		}
		break;

	case NOTIFY_ANGEL_RECEIVED:		// reversed by Snoopy: 0x0BF5:	
		m_pGame->AddEventList(MSG_NOTIFY_ANGEL_RECEIVED, 10 );// "You have received the Tutelary Angel."
		break;

	case NOTIFY_SPELL_SKILL:		// reversed by Snoopy: 0x0BF6
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		for (i = 0; i < MAXMAGICTYPE; i++)
		{	m_pGame->m_cMagicMastery[i] = *cp;
			cp++;
		}
		for (i = 0; i < MAXSKILLTYPE; i++)
		{	m_pGame->m_cSkillMastery[i] = (unsigned char)*cp;
			if (m_pGame->m_pSkillCfgList[i] != NULL)
				m_pGame->m_pSkillCfgList[i]->m_iLevel = (int)*cp;
			cp++;
		}
		m_pGame->InitSpecialAbilities();
		break;	

	case NOTIFY_NORECALL: // Snoopy 0x0BD1
		m_pGame->AddEventList( "You can not recall in this map.", 10 );
		break;

	case NOTIFY_APOCGATESTARTMSG: // Snoopy 0x0BD2
		m_pGame->SetTopMsg("The portal to the Apocalypse is opened.", 10);
		break;

	case NOTIFY_APOCGATEENDMSG: // Snoopy 0x0BD3
		m_pGame->SetTopMsg("The portal to the Apocalypse is closed.", 10);
		break;

	case NOTIFY_APOCGATEOPEN: // Snoopy ;  Case BD4 of switch 00454077
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		ip  = (int *)cp;
		m_pGame->m_iGatePositX = *ip;
		cp += 4;
		ip  = (int *)cp;
		m_pGame->m_iGatePositY = *ip;
		cp += 4;
		ZeroMemory(m_pGame->m_cGateMapName, sizeof(m_pGame->m_cGateMapName));
		memcpy(m_pGame->m_cGateMapName, cp, 10);
		cp += 10;
		break;

	case NOTIFY_QUESTCOUNTER: // Snoopy;  Case BE2 of switch 00454077
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		ip  = (int *)cp;
		m_pGame->m_stQuest.sCurrentCount = (short)*ip;
		cp += 4;
		break;

	case NOTIFY_MONSTERCOUNT: // Snoopy ;  Case BE3 of switch 00454077
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp+=2;
		wsprintf(cTxt,"Rest Monster :%d", sV1) ;
		m_pGame->AddEventList(cTxt, 10);
		break;

	case NOTIFY_APOCGATECLOSE: // Snoopy ;  Case BD5 of switch 00454077
		m_pGame->m_iGatePositX = m_pGame->m_iGatePositY = -1;
		ZeroMemory(m_pGame->m_cGateMapName, sizeof(m_pGame->m_cGateMapName));
		break;

	case NOTIFY_APOCFORCERECALLPLAYERS: // Snoopy ;  Case BD7 of switch 00454077
		m_pGame->AddEventList( "You are recalled by force, because the Apocalypse is started.", 10 );
		break;

	case NOTIFY_ABADDONKILLED: // Snoopy ;  Case BD6 of switch 00454077
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		ZeroMemory(cTxt, sizeof(cTxt));
		memcpy(cTxt, cp, 10);
		cp += 10;
		wsprintf(m_pGame->G_cTxt, "Abaddon is destroyed by %s", cTxt);
		m_pGame->AddEventList(m_pGame->G_cTxt, 10);
		break;

	case NOTIFY_RESURRECTPLAYER: // Case BE9 of switch 00454077
		m_pGame->EnableDialogBox(50, 0, NULL, NULL);
		break;

	case NOTIFY_HELDENIANTELEPORT: 
		m_pGame->m_bIsHeldenianMode = TRUE;	
		m_pGame->m_bIsHeldenianMap  = ((m_pGame->m_cMapIndex == 35) || (m_pGame->m_cMapIndex == 36) || (m_pGame->m_cMapIndex == 37));
		m_pGame->SetTopMsg("Teleport to Heldenian field is available from now. Magic casting is forbidden until real battle.", 10);
		break;

	case NOTIFY_HELDENIANEND: 
		m_pGame->m_bIsHeldenianMode = FALSE;
		m_pGame->SetTopMsg("Heldenian holy war has been closed.", 10);
		break;

	case NOTIFY_HELDENIANSTART: 
		m_pGame->m_bIsHeldenianMode = TRUE;	
		m_pGame->m_bIsHeldenianMap  = ((m_pGame->m_cMapIndex == 35) || (m_pGame->m_cMapIndex == 36) || (m_pGame->m_cMapIndex == 37));
		m_pGame->SetTopMsg("Heldenian real battle has been started from now on.", 10);
		break;

	case NOTIFY_HELDENIANVICTORY:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp+=2;
		m_pGame->ShowHeldenianVictory(sV1);
		m_pGame->m_iHeldenianAresdenLeftTower	= -1;
		m_pGame->m_iHeldenianElvineLeftTower		= -1;
		m_pGame->m_iHeldenianAresdenFlags		= -1;
		m_pGame->m_iHeldenianElvineFlags			= -1;
		m_pGame->m_iHeldenianAresdenDead			= -1;
		m_pGame->m_iHeldenianElvineDead			= -1;
		m_pGame->m_iHeldenianAresdenKill			= -1;
		break;

	case NOTIFY_HELDENIANCOUNT: 
		NotifyMsg_Heldenian(Data);
		break;


	// Slates - Diuuude
	case NOTIFY_SLATE_CREATESUCCESS:	// 0x0BC1
		m_pGame->AddEventList( MSG_NOTIFY_SLATE_CREATESUCCESS, 10 );
		break;
	case NOTIFY_SLATE_CREATEFAIL:		// 0x0BC2
		m_pGame->AddEventList( MSG_NOTIFY_SLATE_CREATEFAIL, 10 );
		break;
	case NOTIFY_SLATE_INVINCIBLE:		// 0x0BD8
		m_pGame->AddEventList( MSG_NOTIFY_SLATE_INVINCIBLE, 10 );
		m_pGame->m_bUsingSlate = TRUE;
		break;
	case NOTIFY_SLATE_MANA:				// 0x0BD9
		m_pGame->AddEventList( MSG_NOTIFY_SLATE_MANA, 10 );
		m_pGame->m_bUsingSlate = TRUE;
		break;
	case NOTIFY_SLATE_EXP:				// 0x0BE0
		m_pGame->AddEventList( MSG_NOTIFY_SLATE_EXP, 10 );
		m_pGame->m_bUsingSlate = TRUE;
		break;
	case NOTIFY_SLATE_STATUS:			// 0x0BE1
		m_pGame->AddEventList( MSG_NOTIFY_SLATECLEAR, 10 ); // "The effect of the prophecy-slate is disappeared."
		m_pGame->m_bUsingSlate = FALSE;
		break;

	// MJ Stats Change - Diuuude: Erreur, ici il s'agit de sorts et skills, le serveur comme la v351 sont aussi bugu�s !
	case NOTIFY_STATECHANGE_SUCCESS:	// 0x0BB5
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		for (i = 0; i < MAXMAGICTYPE; i++)
		{	m_pGame->m_cMagicMastery[i] = *cp;
			cp++;
		}
		for (i = 0; i < MAXSKILLTYPE; i++)
		{	m_pGame->m_cSkillMastery[i] = (unsigned char)*cp;
			if (m_pGame->m_pSkillCfgList[i] != NULL)
				m_pGame->m_pSkillCfgList[i]->m_iLevel = (int)*cp;
			//else m_pSkillCfgList[i]->m_iLevel = 0;
			cp++;
		}
		// MJ Stats Change - Diuuude
		m_pGame->m_stat[STAT_STR] += m_pGame->m_luStat[STAT_STR];
		m_pGame->m_stat[STAT_VIT] += m_pGame->m_luStat[STAT_VIT];
		m_pGame->m_stat[STAT_DEX] += m_pGame->m_luStat[STAT_DEX];
		m_pGame->m_stat[STAT_INT] += m_pGame->m_luStat[STAT_INT];
		m_pGame->m_stat[STAT_MAG] += m_pGame->m_luStat[STAT_MAG];
		m_pGame->m_stat[STAT_CHR] += m_pGame->m_luStat[STAT_CHR];
		m_pGame->m_iLU_Point = m_pGame->m_iLevel*3 - (
			(m_pGame->m_stat[STAT_STR] + m_pGame->m_stat[STAT_VIT] + m_pGame->m_stat[STAT_DEX] + m_pGame->m_stat[STAT_INT] + m_pGame->m_stat[STAT_MAG] + m_pGame->m_stat[STAT_CHR])
			- 70) 
			- 3 + m_pGame->m_angelStat[STAT_STR] + m_pGame->m_angelStat[STAT_DEX] + m_pGame->m_angelStat[STAT_INT] + m_pGame->m_angelStat[STAT_MAG];
		m_pGame->m_luStat[STAT_STR] = m_pGame->m_luStat[STAT_VIT] = m_pGame->m_luStat[STAT_DEX] = m_pGame->m_luStat[STAT_INT] = m_pGame->m_luStat[STAT_MAG] = m_pGame->m_luStat[STAT_CHR] = 0;
		m_pGame->AddEventList( "Your stat has been changed.", 10 ); // "Your stat has been changed."
		m_pGame->InitSpecialAbilities();
		break;

	case NOTIFY_LEVELUP: // 0x0B16
		NotifyMsg_LevelUp(Data);
		break;

	case NOTIFY_STATECHANGE_FAILED:		// 0x0BB6
		m_pGame->m_luStat[STAT_STR] = m_pGame->m_luStat[STAT_VIT] = m_pGame->m_luStat[STAT_DEX] = m_pGame->m_luStat[STAT_INT] = m_pGame->m_luStat[STAT_MAG] = m_pGame->m_luStat[STAT_CHR] = 0;
		m_pGame->m_iLU_Point = m_pGame->m_iLevel*3 - (
			(m_pGame->m_stat[STAT_STR] + m_pGame->m_stat[STAT_VIT] + m_pGame->m_stat[STAT_DEX] + m_pGame->m_stat[STAT_INT] + m_pGame->m_stat[STAT_MAG] + m_pGame->m_stat[STAT_CHR])
			- 70) 
			- 3 + m_pGame->m_angelStat[STAT_STR] + m_pGame->m_angelStat[STAT_DEX] + m_pGame->m_angelStat[STAT_INT] + m_pGame->m_angelStat[STAT_MAG];
		m_pGame->AddEventList( "Your stat has not been changed.", 10 );
		break;

	case NOTIFY_SETTING_FAILED: // 0x0BB4 -  Case BB4 of switch 00454077
		m_pGame->AddEventList( "Your stat has not been changed.", 10 );
		m_pGame->m_luStat[STAT_STR] = m_pGame->m_luStat[STAT_VIT] = m_pGame->m_luStat[STAT_DEX] = m_pGame->m_luStat[STAT_INT] = m_pGame->m_luStat[STAT_MAG] = m_pGame->m_luStat[STAT_CHR] = 0;
		m_pGame->m_iLU_Point = m_pGame->m_iLevel*3 - (
			(m_pGame->m_stat[STAT_STR] + m_pGame->m_stat[STAT_VIT] + m_pGame->m_stat[STAT_DEX] + m_pGame->m_stat[STAT_INT] + m_pGame->m_stat[STAT_MAG] + m_pGame->m_stat[STAT_CHR])
			- 70) 
			- 3 + m_pGame->m_angelStat[STAT_STR] + m_pGame->m_angelStat[STAT_DEX] + m_pGame->m_angelStat[STAT_INT] + m_pGame->m_angelStat[STAT_MAG];
		break;

	// CLEROTH - LU
	case NOTIFY_SETTING_SUCCESS: // 0x0BB3 - envoie le niv et les stats
		NotifyMsg_SettingSuccess(Data);
		break;

	case NOTIFY_AGRICULTURENOAREA:		// 0x0BB2
		m_pGame->AddEventList( MSG_NOTIFY_AGRICULTURENOAREA, 10 );
		break;
	case NOTIFY_AGRICULTURESKILLLIMIT:	// 0x0BB1
		m_pGame->AddEventList( MSG_NOTIFY_AGRICULTURESKILLLIMIT, 10 );
		break;

	case NOTIFY_NOMOREAGRICULTURE:		// 0x0BB0
		m_pGame->AddEventList( MSG_NOTIFY_NOMOREAGRICULTURE, 10 );
		break;
	case NOTIFY_MONSTEREVENT_POSITION:				// 0x0BAA
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		m_pGame->m_sMonsterID = (short)(*cp);
		cp++;
		sp  = (short *)cp;
		m_pGame->m_sEventX = *sp;
		cp+=2;
		sp  = (short *)cp;
		m_pGame->m_sEventY = *sp;
		cp+=2;
		m_pGame->m_dwMonsterEventTime = dwTime;
		break;

	case NOTIFY_RESPONSE_HUNTMODE:			// 0x0BA9
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		memcpy(m_pGame->m_cLocation, cp, 10);
		cp += 10;

		if (memcmp(m_pGame->m_cLocation, "are", 3) == 0)
			m_pGame->m_side = ARESDEN;
		else if (memcmp(m_pGame->m_cLocation, "elv", 3) == 0)
			m_pGame->m_side = ELVINE;
		else if (memcmp(m_pGame->m_cLocation, "ist", 3) == 0)
			m_pGame->m_side = ISTRIA;
		else
			m_pGame->m_side = NEUTRAL;

		m_pGame->AddEventList( MSG_GAMEMODE_CHANGED, 10 );
		break;

	case NOTIFY_REQGUILDNAMEANSWER:	 //   0x0BA6
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;
		sp  = (short *)cp;
		sV2 = *sp;
		cp += 2;
		ZeroMemory(cTemp, sizeof(cTemp));
		memcpy(cTemp, cp, 20);
		cp += 20;

		ZeroMemory( m_pGame->m_stGuildName[sV2].cGuildName, sizeof(m_pGame->m_stGuildName[sV2].cGuildName) );
		strcpy(m_pGame->m_stGuildName[sV2].cGuildName, cTemp);
		m_pGame->m_stGuildName[sV2].iGuildRank = sV1;
		for (i = 0; i < 20; i++) if (m_pGame->m_stGuildName[sV2].cGuildName[i] == '_') m_pGame->m_stGuildName[sV2].cGuildName[i] = ' ';
		break;

	case NOTIFY_FORCERECALLTIME: // 0x0BA7
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;
		if ( (int)(sV1/20) > 0)
			wsprintf(m_pGame->G_cTxt,NOTIFY_MSG_FORCERECALLTIME1,(int) (sV1/20)) ;
		else
			wsprintf(m_pGame->G_cTxt,NOTIFY_MSG_FORCERECALLTIME2) ;
		m_pGame->AddEventList(m_pGame->G_cTxt, 10);
		break;

	case NOTIFY_GIZONITEMUPGRADELEFT: // 0x0BA4// Item upgrade is possible.
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;
		m_pGame->m_iGizonItemUpgradeLeft = sV1;
		dwp = (DWORD *)cp;
		switch (*dwp) {
		case 1: //
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER_GIZONITEMUPGRADELEFT1, 10);
			break;
		}
		//wsprintf(G_cTxt,"majesty: %d", m_iGizonItemUpgradeLeft);
		//DebugLog(G_cTxt);
		cp += 4;
		break;

	case NOTIFY_GIZONEITEMCHANGE: // 0x0BA5
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;
		m_pGame->m_pItemList[sV1]->m_cItemType = *cp ;
		cp++ ;
		wp  = (WORD *)cp;
		m_pGame->m_pItemList[sV1]->m_wCurLifeSpan = *wp;
		cp += 2;
		sp  = (short *)cp;
		m_pGame->m_pItemList[sV1]->m_sSprite = *sp;
		cp += 2;
		sp  = (short *)cp;
		m_pGame->m_pItemList[sV1]->m_sSpriteFrame = *sp;
		cp += 2;
		m_pGame->m_pItemList[sV1]->m_cItemColor = *cp ;
		cp++ ;
		m_pGame->m_pItemList[sV1]->m_sItemSpecEffectValue2 = *cp ;
		cp++ ;
		dwp = (DWORD *) cp ;
		m_pGame->m_pItemList[sV1]->m_dwAttribute =  *dwp ;
		cp +=4 ;
		ZeroMemory( m_pGame->m_pItemList[sV1]->m_cName, sizeof(m_pGame->m_pItemList[sV1]->m_cName) );
		memcpy(m_pGame->m_pItemList[sV1]->m_cName,cp,20) ;
		cp += 20 ;
		if (m_pGame->m_bIsDialogEnabled[34] == TRUE)
		{	m_pGame->m_stDialogBoxInfo[34].cMode = 3; // succes
		}
		m_pGame->PlaySound('E', 23, 5);
		switch (m_pGame->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			m_pGame->PlaySound('C', 21, 0);
			break;

		case 4:
		case 5:
		case 6:
			m_pGame->PlaySound('C', 22, 0);
			break;
		}
		break;

	case NOTIFY_ITEMATTRIBUTECHANGE: // 0x0BA3
	case 0x0BC0: // 0x0BC0 Unknown msg, but real in client v3.51
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;
		dwTemp = m_pGame->m_pItemList[sV1]->m_dwAttribute;
		dwp  = (DWORD *)cp;
		m_pGame->m_pItemList[sV1]->m_dwAttribute = *dwp;
		cp += 4;
		dwp  = (DWORD *)cp;
		if (*dwp != 0) m_pGame->m_pItemList[sV1]->m_sItemSpecEffectValue1 = (short)*dwp;
		cp += 4;
		dwp  = (DWORD *)cp;
		if (*dwp != 0) m_pGame->m_pItemList[sV1]->m_sItemSpecEffectValue2 = (short)*dwp;
		cp += 4;
		if (dwTemp == m_pGame->m_pItemList[sV1]->m_dwAttribute)
		{	if (m_pGame->m_bIsDialogEnabled[34] == TRUE)
			{	m_pGame->m_stDialogBoxInfo[34].cMode = 4;// Failed
			}
			m_pGame->PlaySound('E', 24, 5);
		}else
		{	if (m_pGame->m_bIsDialogEnabled[34] == TRUE)
			{	m_pGame->m_stDialogBoxInfo[34].cMode = 3; // Success
			}
			m_pGame->PlaySound('E', 23, 5);
			switch (m_pGame->m_sPlayerType) {
			case 1:
			case 2:
			case 3:
				m_pGame->PlaySound('C', 21, 0);
				break;
			case 4:
			case 5:
			case 6:
				m_pGame->PlaySound('C', 22, 0);
				break;
		}	}
		break;

	case NOTIFY_ITEMUPGRADEFAIL:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;
		if (m_pGame->m_bIsDialogEnabled[34] == FALSE) return ;
		m_pGame->PlaySound('E', 24, 5);
		switch(sV1){
		case 1:
			m_pGame->m_stDialogBoxInfo[34].cMode = 8 ; // Failed
			break ;
		case 2:
			m_pGame->m_stDialogBoxInfo[34].cMode = 9 ; // Failed
			break ;
		case 3:
			m_pGame->m_stDialogBoxInfo[34].cMode = 10 ; // Failed
			break ;
		}
		break;

	case NOTIFY_PARTY:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;
		sp  = (short *)cp;
		sV2 = *sp;
		cp += 2;
		sp  = (short *)cp;
		sV3 = *sp;
		cp += 2;
		switch (sV1) {
		case 1: //Create Party
			switch (sV2) {
			case 0: //Create party failure
				m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
				m_pGame->m_stDialogBoxInfo[32].cMode = 9;
				break;

			case 1: //Create party success
				m_pGame->m_iPartyStatus = 1;
				m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
				m_pGame->m_stDialogBoxInfo[32].cMode = 8;
				m_pGame->m_stPartyMember.clear();
				m_pGame->bSendCommand(MSGID_COMMAND_COMMON, COMMONTYPE_REQUEST_JOINPARTY, NULL, 2, NULL, NULL, m_pGame->m_cMCName);
				break;
			}
			break;

		case 2: //Delete party
			m_pGame->m_iPartyStatus = 0;
			m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
			m_pGame->m_stDialogBoxInfo[32].cMode = 10;
			m_pGame->m_stPartyMember.clear();
			break;

		case 4: //Join party
			ZeroMemory(cTxt, sizeof(cTxt));
			memcpy(cTxt, cp, 10);
			cp += 10;

			switch (sV2) {
			case 0: //Join party failure
				m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
				m_pGame->m_stDialogBoxInfo[32].cMode = 9;
				break;

			case 1: //Join party success
				if (strcmp(cTxt, m_pGame->m_cPlayerName) == 0) {
					m_pGame->m_iPartyStatus = 2;
					m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
					m_pGame->m_stDialogBoxInfo[32].cMode = 8;
				}
				else {
					wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER1, cTxt);
					m_pGame->AddEventList(m_pGame->G_cTxt, 10);
				}

				m_pGame->m_stPartyMember.push_back(new CGame::partyMember(cTxt));
				break;
			}
			break;

		case 5: //Get list of names
			m_pGame->m_iTotalPartyMember = sV3;
			for (i = 0; i < sV3; i++) {
				ZeroMemory(cTxt, sizeof(cTxt));
				memcpy(cTxt, cp, 10);
				for (sV6 = 0, sV5 = false; sV6 < m_pGame->m_stPartyMember.size(); sV6++)
					if(m_pGame->m_stPartyMember[sV6]->cName.compare(cTxt) == 0){ sV5 = true; break;}

				if(sV5 == false){
					m_pGame->m_stPartyMember.push_back(new CGame::partyMember(cTxt));
				}
				cp+= 11;
			}
			break;

		default:
			sp  = (short *)cp;
			sV4 = *sp;
			cp += 2;
			break;

		case 6: //Dismissed party member
			ZeroMemory(cTxt, sizeof(cTxt));
			memcpy(cTxt, cp, 10);
			cp += 10;

			switch (sV2) {
			case 0: //
				m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
				m_pGame->m_stDialogBoxInfo[32].cMode = 7;
				break;

			case 1: //Party member successfully dismissed
				if (strcmp(cTxt, m_pGame->m_cPlayerName) == 0) {
					m_pGame->m_iPartyStatus = 0;
					m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
					m_pGame->m_stDialogBoxInfo[32].cMode = 6;
				}
				else {
					wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER2 , cTxt);
					m_pGame->AddEventList(m_pGame->G_cTxt, 10);
				}
				for (CGame::partyIterator it = m_pGame->m_stPartyMember.begin(); it < m_pGame->m_stPartyMember.end(); it++)
					if ((*it)->cName.compare(cTxt) == 0) {
						m_pGame->m_stPartyMember.erase(it);
						break;
					}
				break;
			}
			break;

		case 7: //Party join rejected
			m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
			m_pGame->m_stDialogBoxInfo[32].cMode = 9;
			break;

		case 8: //You left the party
			m_pGame->m_iPartyStatus = 0;
			m_pGame->m_iTotalPartyMember = NULL;
			m_pGame->m_stPartyMember.clear();
			break;
		}
		break;

	case NOTIFY_PARTY_COORDS:
		Notify_PartyInfo(Data);
		break;

	case NOTIFY_CANNOTCONSTRUCT:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);

		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;

		m_pGame->CannotConstruct(sV1);
		m_pGame->PlaySound('E', 25, 0, 0);
		break;

	case NOTIFY_TCLOC:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);

		sp  = (short *)cp;
		m_pGame->m_iTeleportLocX = *sp;
		cp += 2;

		sp  = (short *)cp;
		m_pGame->m_iTeleportLocY = *sp;
		cp += 2;

		ZeroMemory(m_pGame->m_cTeleportMapName, sizeof(m_pGame->m_cTeleportMapName));
		memcpy(m_pGame->m_cTeleportMapName, cp, 10);
		cp += 10;

		sp  = (short *)cp;
		m_pGame->m_iConstructLocX = *sp;
		cp += 2;

		sp  = (short *)cp;
		m_pGame->m_iConstructLocY = *sp;
		cp += 2;

		ZeroMemory(m_pGame->m_cConstructMapName, sizeof(m_pGame->m_cConstructMapName));
		memcpy(m_pGame->m_cConstructMapName, cp, 10);
		cp += 10;
		break;

	case NOTIFY_CONSTRUCTIONPOINT:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);

		sp  = (short *)cp;
		sV1 = *sp;
		cp += 2;

		sp  = (short *)cp;
		sV2 = *sp;
		cp += 2;

		sp  = (short *)cp;
		sV3 = *sp;
		cp += 2;

		if (sV3 == 0) {
			if ((sV1 > m_pGame->m_iConstructionPoint) && (sV2 > m_pGame->m_iWarContribution)) {
				wsprintf(m_pGame->G_cTxt, "%s +%d, %s +%d", CRUSADE_MESSAGE13, (sV1 - m_pGame->m_iConstructionPoint), CRUSADE_MESSAGE21, (sV2 - m_pGame->m_iWarContribution));
				m_pGame->SetTopMsg(m_pGame->G_cTxt, 5);
				m_pGame->PlaySound('E', 23, 0, 0);
			}

			if ((sV1 > m_pGame->m_iConstructionPoint) && (sV2 == m_pGame->m_iWarContribution)) {
				if (m_pGame->m_iCrusadeDuty == 3) {
					wsprintf(m_pGame->G_cTxt, "%s +%d", CRUSADE_MESSAGE13, sV1 - m_pGame->m_iConstructionPoint);
					m_pGame->SetTopMsg(m_pGame->G_cTxt, 5);
					m_pGame->PlaySound('E', 23, 0, 0);
				}
			}

			if ((sV1 == m_pGame->m_iConstructionPoint) && (sV2 > m_pGame->m_iWarContribution)) {
				wsprintf(m_pGame->G_cTxt, "%s +%d", CRUSADE_MESSAGE21, sV2 - m_pGame->m_iWarContribution);
				m_pGame->SetTopMsg(m_pGame->G_cTxt, 5);
				m_pGame->PlaySound('E', 23, 0, 0);
			}

			if (sV1 < m_pGame->m_iConstructionPoint) {
				if (m_pGame->m_iCrusadeDuty == 3) {
					wsprintf(m_pGame->G_cTxt, "%s -%d", CRUSADE_MESSAGE13, m_pGame->m_iConstructionPoint - sV1);
					m_pGame->SetTopMsg(m_pGame->G_cTxt, 5);
					m_pGame->PlaySound('E', 25, 0, 0);
				}
			}

			if (sV2 < m_pGame->m_iWarContribution) {
				wsprintf(m_pGame->G_cTxt, "%s -%d", CRUSADE_MESSAGE21, m_pGame->m_iWarContribution - sV2);
				m_pGame->SetTopMsg(m_pGame->G_cTxt, 5);
				m_pGame->PlaySound('E', 24, 0, 0);
			}
		}

		m_pGame->m_iConstructionPoint = sV1;
		m_pGame->m_iWarContribution   = sV2;
		break;

	case NOTIFY_NOMORECRUSADESTRUCTURE:
		m_pGame->SetTopMsg(CRUSADE_MESSAGE12, 5);
		m_pGame->PlaySound('E', 25, 0, 0);
		break;

	case NOTIFY_GRANDMAGICRESULT:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);

		wp  = (WORD *)cp;
		sV1 = *wp;
		cp += 2;

		wp  = (WORD *)cp;
		sV2 = *wp;
		cp += 2;

		wp  = (WORD *)cp;
		sV3 = *wp;
		cp += 2;

		ZeroMemory(cTxt, sizeof(cTxt));
		memcpy(cTxt, cp, 10);
		cp += 10;

		wp  = (WORD *)cp;
		sV4 = *wp;
		cp += 2;

		wp  = (WORD *)cp;
		sV5 = *wp;  //
		cp += 2;

		if (sV5  > 0 ) {
			wp  = (WORD *)cp;
			sV6 = *wp;
			cp += 2;
			sV5-- ;
		}
		else sV6 = 0 ;

		if ( sV5  > 0 ) {
			wp  = (WORD *)cp;
			sV7 = *wp;
			cp += 2;
			sV5-- ;
		}
		else sV7 = 0 ;

		if ( sV5  > 0 ) {
			wp  = (WORD *)cp;
			sV8 = *wp;
			cp += 2;
			sV5-- ;
		}
		else sV8 = 0 ;

		if ( sV5  > 0 ) {
			wp  = (WORD *)cp;
			sV9 = *wp;
			cp += 2;
			sV5-- ;
		}
		else sV9 = 0 ;

		m_pGame->GrandMagicResult(cTxt, sV1, sV2, sV3, sV4, sV6, sV7, sV8, sV9);
		break;

	case NOTIFY_METEORSTRIKECOMING:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		wp  = (WORD *)cp;
		sV1 = *wp;
		cp += 2;
		m_pGame->MeteorStrikeComing(sV1);
		m_pGame->PlaySound('E', 25, 0, 0);
		break;

	case NOTIFY_METEORSTRIKEHIT:
		m_pGame->SetTopMsg(CRUSADE_MESSAGE17, 5);
		//StartMeteorStrikeEffect
		for( i=0 ; i<36 ; i++ ) m_pGame->bAddNewEffect(60, m_pGame->m_sViewPointX +(rand() % 640), m_pGame->m_sViewPointY +(rand() % 480), NULL, NULL, -(rand() % 80));
		break;

	case NOTIFY_MAPSTATUSNEXT:
		m_pGame->AddMapStatusInfo(Data, FALSE);
		break;

	case NOTIFY_MAPSTATUSLAST:
		m_pGame->AddMapStatusInfo(Data, TRUE);
		break;

	case NOTIFY_LOCKEDMAP:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp = (short *)cp;
		sV1 = *sp;
		cp += 2;

		ZeroMemory(cTemp, sizeof(cTemp));
		ZeroMemory(cTxt, sizeof(cTxt));
		memcpy(cTxt, cp, 10);
		cp += 10;

		m_pGame->GetOfficialMapName(cTxt, cTemp);
		wsprintf( m_pGame->G_cTxt, NOTIFY_MSG_HANDLER3, sV1, cTemp );
		m_pGame->SetTopMsg(m_pGame->G_cTxt, 10);
		m_pGame->PlaySound('E', 25, 0, 0);
		break;

	case NOTIFY_CRUSADE: // Crusade msg
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		ip = (int *)cp;
		iV1 = *ip; // Crusademode
		cp += 4;
		ip = (int *)cp;
		iV2 = *ip; // crusade duty
		cp += 4;
		ip = (int *)cp;
		iV3 = *ip;
		cp += 4;
		ip = (int *)cp;
		iV4 = *ip;
		cp += 4;
		if (m_pGame->m_bIsCrusadeMode == FALSE)
		{	if (iV1 != 0) // begin crusade
			{	m_pGame->m_bIsCrusadeMode = TRUE;
				m_pGame->m_iCrusadeDuty = iV2;
				if( (m_pGame->m_iCrusadeDuty != 3) && m_pGame->m_side == NEUTRAL )
					m_pGame->_RequestMapStatus("middleland", 3);
				if (m_pGame->m_iCrusadeDuty != NULL)
					 m_pGame->EnableDialogBox(33, 2, iV2, NULL);
				else m_pGame->EnableDialogBox(33, 1, NULL, NULL);
				if( m_pGame->m_side == NEUTRAL ) m_pGame->EnableDialogBox(18, 800, NULL, NULL);
				else if( m_pGame->m_side == ARESDEN ) m_pGame->EnableDialogBox(18, 801, NULL, NULL);
				else if( m_pGame->m_side == ELVINE ) m_pGame->EnableDialogBox(18, 802, NULL, NULL);
				if (m_pGame->m_side == NEUTRAL) m_pGame->SetTopMsg(NOTIFY_MSG_CRUSADESTART_NONE, 10);
				else m_pGame->SetTopMsg(CRUSADE_MESSAGE9, 10);
				m_pGame->PlaySound('E', 25, 0, 0);
			}
			if (iV3 != 0) // Crusade finished, show XP result screen
			{	m_pGame->CrusadeContributionResult(iV3);
			}
			if (iV4 == -1) // The crusade you played in was finished.
			{	m_pGame->CrusadeContributionResult(0); // You connect in this crusade, but did not connect after previous one => no XP....
			}
		}else
		{	if (iV1 == 0) // crusade finished show result (1st result: winner)
			{	m_pGame->m_bIsCrusadeMode = FALSE;
				m_pGame->m_iCrusadeDuty   = NULL;
				m_pGame->CrusadeWarResult(iV4);
				m_pGame->SetTopMsg(CRUSADE_MESSAGE57, 8);
			}else
			{	if (m_pGame->m_iCrusadeDuty != iV2)
				{	m_pGame->m_iCrusadeDuty = iV2;
					m_pGame->EnableDialogBox(33, 2, iV2, NULL);
					m_pGame->PlaySound('E', 25, 0, 0);
			}	}
			if (iV4 == -1)
			{	m_pGame->CrusadeContributionResult(0); // You connect in this crusade, but did not connect after previous one => no XP....
		}	}
		break;

	case NOTIFY_SPECIALABILITYSTATUS:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp = (short *)cp;
		sV1 = *sp;
		cp += 2;
		sp = (short *)cp;
		sV2 = *sp;
		cp += 2;
		sp = (short *)cp;
		sV3 = *sp;
		cp += 2;
		if (sV1 == 1) // Use SA
		{	m_pGame->PlaySound('E', 35, 0);
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER4, 10); // "Use special ability!"
			switch (sV2) {
			case 1: wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER5,sV3); break;//"You are untouchable for %d seconds!"
			case 2: wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER6, sV3); break;//"
			case 3: wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER7, sV3); break;//"
			case 4: wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER8, sV3); break;//"
			case 5: wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER9, sV3); break;//"
			case 50:wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER10, sV3); break;//"
			case 51:wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER11, sV3); break;//"
			case 52:wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER12, sV3); break;//"
			case 55: // Spell effect
				if (sV3 >90)
					wsprintf(m_pGame->G_cTxt, "You cast a powerfull incantation, you can't use it again before %d minutes.", sV3/60);
				else
					wsprintf(m_pGame->G_cTxt, "You cast a powerfull incantation, you can't use it again before %d seconds.", sV3);
				break;
			case 100: // New Ability: Minor Berzerk → Regular Berserk
				m_pGame->m_stAbility[0].dwCooldownEnd = dwTime + (DWORD)sV3 * 1000;
				m_pGame->m_stAbility[0].bActive = true;
				m_pGame->m_stAbility[0].dwActiveEnd = dwTime + 60000;
				wsprintf(m_pGame->G_cTxt, "Berzerk ability activated! Cooldown: %d seconds.", sV3);
				break;
			case 101: // New Ability: Super Berzerk
				m_pGame->m_stAbility[1].dwCooldownEnd = dwTime + (DWORD)sV3 * 1000;
				m_pGame->m_stAbility[1].bActive = true;
				m_pGame->m_stAbility[1].dwActiveEnd = dwTime + 15000;
				wsprintf(m_pGame->G_cTxt, "Super Berzerk activated! Cooldown: %d seconds.", sV3);
				break;
			case 102: // New Ability: Speed Burst
				m_pGame->m_stAbility[2].dwCooldownEnd = dwTime + (DWORD)sV3 * 1000;
				m_pGame->m_stAbility[2].bActive = true;
				m_pGame->m_stAbility[2].dwActiveEnd = dwTime + 10000;
				wsprintf(m_pGame->G_cTxt, "Speed Burst activated! Cooldown: %d seconds.", sV3);
				break;
			case 103: // New Ability: Glacial Strike — 3s speed burst, 7s hit window
				m_pGame->m_stAbility[3].dwCooldownEnd = dwTime + (DWORD)sV3 * 1000;
				m_pGame->m_stAbility[3].bActive = true;
				m_pGame->m_stAbility[3].dwActiveEnd = dwTime + 7000;
				// Speed buff (3s) is applied via separate NOTIFY_SPEED_BUFF from server
				wsprintf(m_pGame->G_cTxt, "Glacial Strike! Hit a player within 7s to freeze them.");
				break;
			}
			m_pGame->AddEventList(m_pGame->G_cTxt, 10);
		}else if (sV1 == 2) // Finished using
		{	if (m_pGame->m_iSpecialAbilityType != (int)sV2)
			{	m_pGame->PlaySound('E', 34, 0);
				m_pGame->AddEventList(NOTIFY_MSG_HANDLER13, 10);//"Special ability has been set!"
				if (sV3 >= 60)
				{	switch (sV2) {
					case 1: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER14, sV3/60); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"Ability that decreases enemy's HP by 50%: Can use after %dMin"
					case 2: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER15, sV3/60); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 3: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER16, sV3/60); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 4: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER17, sV3/60); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 5: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER18, sV3/60); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 50:wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER19, sV3/60); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 51:wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER20, sV3/60); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 52:wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER21, sV3/60); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					}
				}else
				{	switch (sV2) {
					case 1: wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER22, sV3); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 2: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER23, sV3); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 3: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER24, sV3); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 4: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER25, sV3); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 5: wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER26, sV3); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 50:wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER27, sV3); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 51:wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER28, sV3); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//"
					case 52:wsprintf(m_pGame->G_cTxt,  NOTIFY_MSG_HANDLER29, sV3); m_pGame->AddEventList(m_pGame->G_cTxt, 10); break;//""Ability that makes character untouchable: Can use after %dSec"
			}	}	}
			m_pGame->m_iSpecialAbilityType = (int)sV2;
			m_pGame->m_dwSpecialAbilitySettingTime = dwTime;
			m_pGame->m_iSpecialAbilityTimeLeftSec  = (int)sV3;
		}else if (sV1 == 3)  // End of using time
		{	m_pGame->m_bIsSpecialAbilityEnabled = FALSE;
			m_pGame->m_dwSpecialAbilitySettingTime = dwTime;
			if (sV3 == 0)
			{	m_pGame->m_iSpecialAbilityTimeLeftSec  = 1200;
				m_pGame->AddEventList(NOTIFY_MSG_HANDLER30, 10);//"Special ability has run out! Will be available in 20 minutes."
			}else
			{	m_pGame->m_iSpecialAbilityTimeLeftSec  = (int)sV3;
				if (sV3 >90)
					 wsprintf(m_pGame->G_cTxt, "Special ability has run out! Will be available in %d minutes." , sV3/60);
				else wsprintf(m_pGame->G_cTxt, "Special ability has run out! Will be available in %d seconds." , sV3);
				m_pGame->AddEventList(m_pGame->G_cTxt, 10);
			}
		}else if (sV1 == 4) // Unequiped the SA item
		{	m_pGame->AddEventList(NOTIFY_MSG_HANDLER31, 10);//"Special ability has been released."
			m_pGame->m_iSpecialAbilityType = 0;
		}else if (sV1 == 5) // Angel
		{	m_pGame->PlaySound('E', 52, 0); // Angel
		}
		break;

	case NOTIFY_SPECIALABILITYENABLED:
		if (m_pGame->m_bIsSpecialAbilityEnabled == FALSE) {
			m_pGame->PlaySound('E', 30, 5);
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER32, 10);//"
		}
		m_pGame->m_bIsSpecialAbilityEnabled = TRUE;
		break;

	case NOTIFY_ENERGYSPHEREGOALIN:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp = (short *)cp;
		sV1 = *sp;
		cp += 2;
		sp = (short *)cp;
		sV2 = *sp;
		cp += 2;
		sp = (short *)cp;
		sV3 = *sp;
		cp += 2;
		ZeroMemory(cTxt, sizeof(cTxt));
		memcpy(cTxt, cp, 20);

		if (sV2 == sV3)
		{	m_pGame->PlaySound('E', 24, 0);
			if (strcmp(cTxt, m_pGame->m_cPlayerName) == 0)
			{	m_pGame->AddEventList(NOTIFY_MSG_HANDLER33, 10);//You pushed energy sphere to enemy's energy portal! Contribution point will be decreased by 10 points."
				m_pGame->m_iContribution += sV1; // fixed, server must match...
				m_pGame->m_iContributionPrice = 0;
				if (m_pGame->m_iContribution < 0) m_pGame->m_iContribution = 0;
			}
			else {
				ZeroMemory(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt));
				if( m_pGame->m_side == ARESDEN ) wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER34, cTxt);//"%s(Aresden) pushed energy sphere to enemy's portal!!..."
				else if (m_pGame->m_side == ELVINE) wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER34_ELV, cTxt);//"%s(Elvine) pushed energy sphere to enemy's portal!!..."
				m_pGame->AddEventList(m_pGame->G_cTxt, 10);
			}
		}else
		{	m_pGame->PlaySound('E', 23, 0);
			if (strcmp(cTxt, m_pGame->m_cPlayerName) == 0)
			{	switch (m_pGame->m_sPlayerType) {
				case 1:
				case 2:
				case 3:	m_pGame->PlaySound('C', 21, 0); break;
				case 4:
				case 5:
				case 6:	m_pGame->PlaySound('C', 22, 0); break;
				}
				m_pGame->AddEventList(NOTIFY_MSG_HANDLER35, 10);//"Congulaturations! You brought energy sphere to energy portal and earned experience and prize gold!"

				m_pGame->m_iContribution += 5;
				if (m_pGame->m_iContribution < 0) m_pGame->m_iContribution = 0;
			}else
			{	ZeroMemory(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt));
				if (sV3 == 1)
				{	wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER36, cTxt);//"Elvine %s : Goal in!"
					m_pGame->AddEventList(m_pGame->G_cTxt, 10);
				}else if (sV3 == 2)
				{	wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER37, cTxt);//"Aresden %s : Goal in!"
					m_pGame->AddEventList(m_pGame->G_cTxt, 10);
		}	}	}
		break;

	case NOTIFY_ENERGYSPHERECREATED:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp = (short *)cp;
		sV1 = *sp;
		cp += 2;
		sp = (short *)cp;
		sV2 = *sp;
		cp += 2;
		ZeroMemory(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt));
		wsprintf(m_pGame->G_cTxt, NOTIFY_MSG_HANDLER38, sV1, sV2);//"Energy sphere was dropped in (%d, %d) of middleland!"
		m_pGame->AddEventList(m_pGame->G_cTxt, 10);
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER39, 10);//"A player who pushed energy sphere to the energy portal of his city will earn many Exp and Contribution."
		break;

	case NOTIFY_QUERY_JOINPARTY:
		ZeroMemory(m_pGame->m_stDialogBoxInfo[32].cStr, sizeof(m_pGame->m_stDialogBoxInfo[32].cStr));
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		strcpy(m_pGame->m_stDialogBoxInfo[32].cStr, cp);

		if(m_pGame->m_partyAutoAccept) {
			m_pGame->bSendCommand(MSGID_COMMAND_COMMON, COMMONTYPE_REQUEST_ACCEPTJOINPARTY, NULL, 1, NULL, NULL, m_pGame->m_stDialogBoxInfo[32].cStr);
		} 
		else {
			m_pGame->EnableDialogBox(32, NULL, NULL, NULL);
			m_pGame->m_stDialogBoxInfo[32].cMode = 1;
		}
		break;

	case NOTIFY_RESPONSE_CREATENEWPARTY:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		sp = (short *)cp;

		if ((BOOL)*sp == TRUE)
		{	m_pGame->m_stDialogBoxInfo[32].cMode = 2;
		}else
		{	m_pGame->m_stDialogBoxInfo[32].cMode = 3;
		}
		break;

	case NOTIFY_DAMAGEMOVE:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		sp = (short *)cp;
		m_pGame->m_sDamageMove = *sp;
		cp += 2;
		sp = (short *)cp;
		m_pGame->m_sDamageMoveAmount = *sp;
		cp += 2;
		break;

	case NOTIFY_OBSERVERMODE:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		sp = (short *)cp;
		if (*sp == 1)
		{	m_pGame->AddEventList(NOTIFY_MSG_HANDLER40);//"Observer Mode On. Press 'SHIFT + ESC' to Log Out..."
			m_pGame->m_bIsObserverMode = TRUE;
			m_pGame->m_dwObserverCamTime = timeGetTime();
			char cName[12];
			ZeroMemory(cName, sizeof(cName));
			memcpy(cName, m_pGame->m_cPlayerName, 10);
			m_pGame->m_pMapData->bSetOwner(m_pGame->m_sPlayerObjectID, -1, -1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, cName, NULL, NULL, NULL, NULL);
		}else
		{	m_pGame->AddEventList(NOTIFY_MSG_HANDLER41);//"Observer Mode Off"
			m_pGame->m_bIsObserverMode = FALSE;
			m_pGame->m_pMapData->bSetOwner(m_pGame->m_sPlayerObjectID, m_pGame->m_sPlayerX, m_pGame->m_sPlayerY, m_pGame->m_sPlayerType, m_pGame->m_cPlayerDir, m_pGame->m_sPlayerAppr1, m_pGame->m_sPlayerAppr2, m_pGame->m_sPlayerAppr3, m_pGame->m_sPlayerAppr4, m_pGame->m_iPlayerApprColor, m_pGame->m_iPlayerStatus, m_pGame->m_cPlayerName, OBJECTSTOP, NULL, NULL, NULL);
		}
		break;

	case NOTIFY_BUILDITEMSUCCESS:
		m_pGame->DisableDialogBox(26);
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp = (short *)cp;
		sV1 = *sp;
		cp += 2;
		sp = (short *)cp;
		sV2 = *sp;
		cp += 2;
		if (sV1 < 10000)
		{	m_pGame->EnableDialogBox(26, 6, 1, sV1, NULL);
			m_pGame->m_stDialogBoxInfo[26].sV1 = sV2;
		}else
		{	m_pGame->EnableDialogBox(26, 6, 1, -1*(sV1 - 10000), NULL);
			m_pGame->m_stDialogBoxInfo[26].sV1 = sV2;
		}
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER42, 10);
		m_pGame->PlaySound('E', 23, 5);
		switch (m_pGame->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			m_pGame->PlaySound('C', 21, 0);
			break;

		case 4:
		case 5:
		case 6:
			m_pGame->PlaySound('C', 22, 0);
			break;
		}
		break;

	case NOTIFY_BUILDITEMFAIL:
		m_pGame->DisableDialogBox(26);
		m_pGame->EnableDialogBox(26, 6, 0, NULL);
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER43, 10);
		m_pGame->PlaySound('E', 24, 5);
		break;

	case NOTIFY_QUESTREWARD:
		NotifyMsg_QuestReward(Data);
		break;

	case NOTIFY_QUESTCOMPLETED:
		m_pGame->m_stQuest.bIsQuestCompleted = TRUE;
		m_pGame->DisableDialogBox(28);
		m_pGame->EnableDialogBox(28, 1, NULL, NULL);
		switch (m_pGame->m_sPlayerType) {
		case 1:
		case 2:
		case 3:	m_pGame->PlaySound('C', 21, 0); break;
		case 4:
		case 5:
		case 6:	m_pGame->PlaySound('C', 22, 0); break;
		}
		m_pGame->PlaySound('E', 23, 0);
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER44, 10);
		break;

	case NOTIFY_QUESTABORTED:
		m_pGame->m_stQuest.sQuestType = NULL;
		m_pGame->DisableDialogBox(28);
		m_pGame->EnableDialogBox(28, 2, NULL, NULL);
		break;

	case NOTIFY_QUESTCONTENTS:
		NotifyMsg_QuestContents(Data);
		break;

	case NOTIFY_ITEMCOLORCHANGE:
		NotifyMsg_ItemColorChange(Data);
		break;

	case NOTIFY_DROPITEMFIN_COUNTCHANGED:
		NotifyMsg_DropItemFin_CountChanged(Data);
		break;

	case NOTIFY_CANNOTGIVEITEM:
		NotifyMsg_CannotGiveItem(Data);
		break;

	case NOTIFY_GIVEITEMFIN_COUNTCHANGED:
		NotifyMsg_GiveItemFin_CountChanged(Data);
		break;

	case NOTIFY_EXCHANGEITEMCOMPLETE:
		m_pGame->AddEventList(NOTIFYMSG_EXCHANGEITEM_COMPLETE1, 10);
		m_pGame->DisableDialogBox(27);
		//Snoopy: MultiTrade
		m_pGame->DisableDialogBox(41);
		m_pGame->PlaySound('E', 23, 5);
		break;

	case NOTIFY_CANCELEXCHANGEITEM:
		m_pGame->PlaySound('E', 24, 5);
		m_pGame->AddEventList(NOTIFYMSG_CANCEL_EXCHANGEITEM1, 10);
		m_pGame->AddEventList(NOTIFYMSG_CANCEL_EXCHANGEITEM2, 10);
		//Snoopy: MultiTrade
		m_pGame->DisableDialogBox(41);
		m_pGame->DisableDialogBox(27);
		break;

	case NOTIFY_SETEXCHANGEITEM:
		NotifyMsg_SetExchangeItem(Data);
		break;

	case NOTIFY_OPENEXCHANGEWINDOW:
		NotifyMsg_OpenExchageWindow(Data);
		break;

	case NOTIFY_NOTFLAGSPOT:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER45, 10);
		break;

	case NOTIFY_ITEMPOSLIST:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		for (i = 0; i < MAXITEMS; i++) {
			sp = (short *)cp;
			sX = *sp;
			cp += 2;
			sp = (short *)cp;
			sY = *sp;
			cp += 2;
			if (m_pGame->m_pItemList[i] != NULL) {
				if (sY < -10) sY = -10;
				if (sX < 0)   sX = 0;
				if (sX > 170) sX = 170;
				if (sY > 95)  sY = 95;

				m_pGame->m_pItemList[i]->m_sX = sX;
				m_pGame->m_pItemList[i]->m_sY = sY;
			}
		}
		break;

	case NOTIFY_ENEMYKILLS:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		ip = (int *)cp;
		m_pGame->m_iEnemyKillCount = *ip;
		break;

	case NOTIFY_DOWNSKILLINDEXSET:
		NotifyMsg_DownSkillIndexSet(Data);
		break;

	case NOTIFY_ADMINIFO:
		NotifyMsg_AdminInfo(Data);
		break;

	case NOTIFY_NPCTALK:
		NpcTalkHandler(Data);
		break;

	case NOTIFY_POTIONSUCCESS:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER46, 10);
		break;

	case NOTIFY_POTIONFAIL:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER47, 10);
		break;

	case NOTIFY_LOWPOTIONSKILL:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER48, 10);
		break;

	case NOTIFY_NOMATCHINGPOTION:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER49, 10);
		break;

	case NOTIFY_SUPERATTACKLEFT:
		sp = (short *)(Data + INDEX2_MSGTYPE + 2);
		m_pGame->m_iSuperAttackLeft = (int)*sp;
		break;

	case NOTIFY_SAFEATTACKMODE:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		switch (*cp) {
		case 1:
			if(!m_pGame->m_bIsSafeAttackMode) m_pGame->AddEventList(NOTIFY_MSG_HANDLER50, 10);//"
			m_pGame->m_bIsSafeAttackMode = TRUE;
			break;
		case 0:
			if(m_pGame->m_bIsSafeAttackMode) m_pGame->AddEventList(NOTIFY_MSG_HANDLER51, 10);//"
			m_pGame->m_bIsSafeAttackMode = FALSE;
			break;
		}
		break;

	case NOTIFY_IPACCOUNTINFO:
		ZeroMemory(cTemp, sizeof(cTemp));
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		strcpy(cTemp, cp);
		m_pGame->AddEventList(cTemp);
		break;

	case NOTIFY_REWARDGOLD:
		dwp = (DWORD *)(Data + INDEX2_MSGTYPE + 2);
		m_pGame->m_iRewardGold = *dwp;
		break;

	case NOTIFY_SERVERSHUTDOWN:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		if (m_pGame->m_bIsDialogEnabled[25] == FALSE)
			 m_pGame->EnableDialogBox(25, *cp, NULL, NULL);
		else m_pGame->m_stDialogBoxInfo[25].cMode = *cp;
		m_pGame->PlaySound('E', 27, NULL);
		break;

	case NOTIFY_GLOBALATTACKMODE:
		NotifyMsg_GlobalAttackMode(Data);
		break;

	case NOTIFY_WHETHERCHANGE:
		NotifyMsg_WhetherChange(Data);
		break;

	case NOTIFY_FISHCANCELED:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		wp = (WORD *)cp;
		switch (*wp) {
		case NULL:
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER52, 10);
			m_pGame->DisableDialogBox(24);
			break;

		case 1:
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER53, 10);
			m_pGame->DisableDialogBox(24);
			break;

		case 2:
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER54, 10);
			m_pGame->DisableDialogBox(24);
			break;
		}
		break;

	case NOTIFY_FISHSUCCESS:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER55, 10);
		m_pGame->PlaySound('E', 23, 5);
		m_pGame->PlaySound('E', 17, 5);
		switch (m_pGame->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			m_pGame->PlaySound('C', 21, 0);
			break;

		case 4:
		case 5:
		case 6:
			m_pGame->PlaySound('C', 22, 0);
			break;
		}
		break;

	case NOTIFY_FISHFAIL:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER56, 10);
		m_pGame->PlaySound('E', 24, 5);
		break;

	case NOTIFY_FISHCHANCE:
		NotifyMsg_FishChance(Data);
		break;

	case NOTIFY_EVENTFISHMODE:
		NotifyMsg_EventFishMode(Data);
		break;

	case NOTIFY_NOTICEMSG:
		NotifyMsg_NoticeMsg(Data);
		break;

	case NOTIFY_RATINGPLAYER:
		NotifyMsg_RatingPlayer(Data);
		break;

	case NOTIFY_CANNOTRATING:
		NotifyMsg_CannotRating(Data);
		break;

	case NOTIFY_ADMINUSERLEVELLOW:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER58, 10);
		break;

	case NOTIFY_NOGUILDMASTERLEVEL:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER59, 10);
		break;
	case NOTIFY_SUCCESSBANGUILDMAN:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER60, 10);
		break;
	case NOTIFY_CANNOTBANGUILDMAN:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER61, 10);
		break;

	case NOTIFY_PLAYERSHUTUP:
		NotifyMsg_PlayerShutUp(Data);
		break;

	case NOTIFY_TIMECHANGE:
		NotifyMsg_TimeChange(Data);
		break;

	case NOTIFY_TOBERECALLED:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER62, 10);
		break;

	case NOTIFY_HUNGER:
		NotifyMsg_Hunger(Data);
		break;

	case NOTIFY_PLAYERPROFILE:
		NotifyMsg_PlayerProfile(Data);
		break;

	case NOTIFY_WHISPERMODEON:
		NotifyMsg_WhisperMode(TRUE, Data);
		break;

	case NOTIFY_WHISPERMODEOFF:
		NotifyMsg_WhisperMode(FALSE, Data);
		break;

	case NOTIFY_PLAYERONGAME:
		NotifyMsg_PlayerStatus(TRUE, Data);
		break;

	case NOTIFY_PLAYERNOTONGAME:
		NotifyMsg_PlayerStatus(FALSE, Data);
		break;

	case NOTIFY_RANGE:
		NotifyMsg_Range(Data);
		break;

	case NOTIFY_ITEMSOLD:
		m_pGame->DisableDialogBox(23);
		break;

	case NOTIFY_ITEMREPAIRED:
		m_pGame->DisableDialogBox(23);
		NotifyMsg_ItemRepaired(Data);
		break;

	case NOTIFY_CANNOTREPAIRITEM:
		NotifyMsg_CannotRepairItem(Data);
		break;

	case NOTIFY_CANNOTSELLITEM:
		NotifyMsg_CannotSellItem(Data);
		break;

	case NOTIFY_REPAIRITEMPRICE:
		NotifyMsg_RepairItemPrice(Data);
		break;

	case NOTIFY_SELLITEMPRICE:
		NotifyMsg_SellItemPrice(Data);
		break;

	case NOTIFY_SHOWMAP:
		NotifyMsg_ShowMap(Data);
		break;

	case NOTIFY_SKILLUSINGEND:
		NotifyMsg_SkillUsingEnd(Data);
		break;

	case NOTIFY_TOTALUSERS:
		NotifyMsg_TotalUsers(Data);
		break;

	case NOTIFY_EVENTSTART:
		NotifyMsg_EventStart(Data);
		break;

	case NOTIFY_EVENTSTARTING:
	case NOTIFY_EVENTSTARTING2:
	case NOTIFY_EVENTSTARTING3:
		NotifyMsg_EventStarting(Data);
		break;

	case NOTIFY_CASUALTIES:
		NotifyMsg_Casualties(Data);
		break;

	case NOTIFY_RELICINALTAR:
		NotifyMsg_RelicInAltar(Data);
		break;

	case NOTIFY_RELICGRABBED:
		NotifyMsg_RelicGrabbed(Data);
		break;

	case NOTIFY_CTRWINNER:
		NotifyMsg_CTRWinner(Data);
		break;

	case NOTIFY_RELICPOSITION:
		cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
		sp = (short *)cp;
		m_pGame->m_relicX = *sp;
		sp++;
		m_pGame->m_relicY = *sp;
		break;

	case NOTIFY_MAGICEFFECTOFF:
		NotifyMsg_MagicEffectOff(Data);
		break;

	case NOTIFY_MAGICEFFECTON:
		NotifyMsg_MagicEffectOn(Data);
		break;

	case NOTIFY_CANNOTITEMTOBANK:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER63, 10);
		break;

	case NOTIFY_SERVERCHANGE:
		NotifyMsg_ServerChange(Data);
		break;

	case NOTIFY_SKILL:
		NotifyMsg_Skill(Data);
		break;

	case NOTIFY_SETITEMCOUNT:
		NotifyMsg_SetItemCount(Data);
		break;

	case NOTIFY_ITEMDEPLETED_ERASEITEM:
		NotifyMsg_ItemDepleted_EraseItem(Data);
		break;

	case NOTIFY_DROPITEMFIN_ERASEITEM:
		NotifyMsg_DropItemFin_EraseItem(Data);
		break;

	case NOTIFY_GIVEITEMFIN_ERASEITEM:
		NotifyMsg_GiveItemFin_EraseItem(Data);
		break;

	case NOTIFY_ENEMYKILLREWARD:
		NotifyMsg_EnemyKillReward(Data);
		break;

	case NOTIFY_PKCAPTURED:
		NotifyMsg_PKcaptured(Data);
		break;

	case NOTIFY_PKPENALTY:
		NotifyMsg_PKpenalty(Data);
		break;

	case NOTIFY_ITEMTOBANK:
		NotifyMsg_ItemToBank(Data);
		break;

	case NOTIFY_TRAVELERLIMITEDLEVEL:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER64, 10);
		break;

	case NOTIFY_LIMITEDLEVEL:
		m_pGame->AddEventList(NOTIFYMSG_LIMITED_LEVEL1, 10);
		break;

	case NOTIFY_ITEMLIFESPANEND:
		NotifyMsg_ItemLifeSpanEnd(Data);
		break;

	case NOTIFY_ITEMRELEASED:
		NotifyMsg_ItemReleased(Data);
		break;

	case NOTIFY_ITEMOBTAINED:
		NotifyMsg_ItemObtained(Data);
		break;

	case NOTIFY_ITEMPURCHASED:
		NotifyMsg_ItemPurchased(Data);
		break;

	case NOTIFY_QUERY_JOINGUILDREQPERMISSION:
		NotifyMsg_QueryJoinGuildPermission(Data);
		break;

	case NOTIFY_QUERY_DISMISSGUILDREQPERMISSION:
		NotifyMsg_QueryDismissGuildPermission(Data);
		break;

	case COMMONTYPE_JOINGUILDAPPROVE:
		NotifyMsg_JoinGuildApprove(Data);
		break;

	case COMMONTYPE_JOINGUILDREJECT:
		NotifyMsg_JoinGuildReject(Data);
		break;

	case COMMONTYPE_DISMISSGUILDAPPROVE:
		NotifyMsg_DismissGuildApprove(Data);
		break;

	case COMMONTYPE_DISMISSGUILDREJECT:
		NotifyMsg_DismissGuildReject(Data);
		break;

	case NOTIFY_CANNOTCARRYMOREITEM:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER65, 10);//"
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER66, 10);//"
		// Bank dialog Box
		m_pGame->m_stDialogBoxInfo[14].cMode = 0;
		break;

	case NOTIFY_NOTENOUGHGOLD:
		m_pGame->DisableDialogBox(23);
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER67, 10);//"Gold
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		if (*cp >= 0) {
			m_pGame->m_bIsItemDisabled[*cp] = FALSE;
		}
		break;

	case NOTIFY_HP:
		NotifyMsg_HP(Data);
		break;
	case NOTIFY_MP:
		NotifyMsg_MP(Data);
		break;
	case NOTIFY_SP:
		NotifyMsg_SP(Data);
		break;
	case NOTIFY_KILLED:
		NotifyMsg_Killed(Data);
		break;
	case NOTIFY_EXP:
		NotifyMsg_Exp(Data);
		break;
	case NOTIFY_GUILDDISBANDED:
		NotifyMsg_GuildDisbanded(Data);
		break;
	case NOTIFY_CANNOTJOINMOREGUILDSMAN:
		NotifyMsg_CannotJoinMoreGuildsMan(Data);
		break;
	case NOTIFY_NEWGUILDSMAN:
		NotifyMsg_NewGuildsMan(Data);
		break;
	case NOTIFY_DISMISSGUILDSMAN:
		NotifyMsg_DismissGuildsMan(Data);
		break;
	case NOTIFY_MAGICSTUDYSUCCESS:
		NotifyMsg_MagicStudySuccess(Data);
		break;
	case NOTIFY_MAGICSTUDYFAIL:
		NotifyMsg_MagicStudyFail(Data);
		break;
	case NOTIFY_SKILLTRAINSUCCESS:
		NotifyMsg_SkillTrainSuccess(Data);
		break;
	case NOTIFY_SKILLTRAINFAIL:
		break;
	case NOTIFY_FORCEDISCONN:
		NotifyMsg_ForceDisconn(Data);
		break;
	case NOTIFY_FRIENDONGAME:
		NotifyMsg_FriendOnGame(Data);
		break;
	case NOTIFY_CANNOTRECALL:
		m_pGame->AddEventList(NOTIFY_MSG_HANDLER74, 10); //"You can't recall within 10 seconds of taking damage."
		break;
	case NOTIFY_FIGHTZONERESERVE:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		ip = (int *)cp;
		switch (*ip) {
		case -5:
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER68, 10);
			break;
		case -4:
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER69, 10);
			break;
		case -3:
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER70, 10);
			break;
		case -2:
			m_pGame->m_iFightzoneNumber = 0;
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER71, 10);
			break;
		case -1:
			m_pGame->m_iFightzoneNumber = m_pGame->m_iFightzoneNumber * -1 ;
			m_pGame->AddEventList(NOTIFY_MSG_HANDLER72, 10);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			wsprintf(cTxt, NOTIFY_MSG_HANDLER73, *ip);//"
			m_pGame->AddEventList(cTxt, 10);
			break;
		}
		break;
	}
}

void CMessageHandler::InitPlayerResponseHandler(char * Data)
{
	WORD * wp;
	wp = (WORD *)(Data + INDEX2_MSGTYPE);
	switch (*wp) {
	case MSGTYPE_CONFIRM:
		//bSendCommand(MSGID_REQUEST_INITDATA, NULL, NULL, NULL, NULL, NULL, NULL);
		m_pGame->ChangeGameMode(GAMEMODE_ONWAITINGINITDATA);
		break;

	case MSGTYPE_REJECT:
		m_pGame->ChangeGameMode(GAMEMODE_ONLOGRESMSG);
		ZeroMemory(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg));
		strcpy(m_pGame->m_cMsg, "3J");
		break;
	}
}

void CMessageHandler::InitDataResponseHandler(char * Data)
{
 int * ip, i;
 short * sp, sX, sY;
 char  * cp, cMapFileName[32], cTxt[120], cPreCurLocation[12];
 BOOL  bIsObserverMode;
 HANDLE hFile;
 DWORD  dwFileSize;

	ZeroMemory( cPreCurLocation, sizeof(cPreCurLocation) );
	m_pGame->m_bParalyze = FALSE;

	m_pGame->m_pMapData->Init();

	m_pGame->m_sMonsterID = 0;
	m_pGame->m_dwMonsterEventTime = 0;


	m_pGame->DisableDialogBox(7);
	m_pGame->DisableDialogBox(11);
	m_pGame->DisableDialogBox(13);
	m_pGame->DisableDialogBox(14);
	m_pGame->DisableDialogBox(16);
	m_pGame->DisableDialogBox(22);
	m_pGame->DisableDialogBox(20);
	m_pGame->DisableDialogBox(21);
	m_pGame->DisableDialogBox(23);
	m_pGame->DisableDialogBox(51); // Gail's diag

	m_pGame->m_cCommand = OBJECTSTOP;
	//m_bCommandAvailable = TRUE;
	m_pGame->m_cCommandCount = 0;
	m_pGame->m_bIsGetPointingMode = FALSE;
	m_pGame->m_iPointCommandType  = -1;
	m_pGame->m_iIlusionOwnerH = NULL;
	m_pGame->m_cIlusionOwnerType = NULL;
	m_pGame->m_bIsTeleportRequested = FALSE;
	m_pGame->m_bIsConfusion = FALSE;
	m_pGame->m_bSkillUsingStatus = FALSE;

	m_pGame->m_bItemUsingStatus = FALSE ;

	m_pGame->m_cRestartCount = -1;
	m_pGame->m_dwRestartCountTime = NULL;

	m_pGame->m_relicX = m_pGame->m_relicY = 0;

	m_pGame->m_armorDisabled = false;
	m_pGame->m_shieldDisabled = false;

	for (i = 0; i < m_pGame->m_stPartyMember.size(); i++)
	{
		m_pGame->m_stPartyMember[i]->sX = m_pGame->m_stPartyMember[i]->sY = m_pGame->m_stPartyMember[i]->hp = m_pGame->m_stPartyMember[i]->mp = m_pGame->m_stPartyMember[i]->Maxhp = m_pGame->m_stPartyMember[i]->Maxmp = 0;
		m_pGame->m_stPartyMember[i]->bIsSelected = FALSE;
	}

	for (i = 0; i < MAXMAGICTYPE; i++)
		m_pGame->m_magicDisabled[i] = false;

	for (i = 0; i < (int)m_pGame->m_pEffectList.size(); i++)
		if (m_pGame->m_pEffectList[i] != NULL) delete m_pGame->m_pEffectList[i];
	m_pGame->m_pEffectList.clear();

	for (i = 0; i < MAXWHETHEROBJECTS; i++)
	{	m_pGame->m_stWhetherObject[i].sX    = 0;
		m_pGame->m_stWhetherObject[i].sBX   = 0;
		m_pGame->m_stWhetherObject[i].sY    = 0;
		m_pGame->m_stWhetherObject[i].cStep = 0;
	}

	for (i = 0; i < MAXGUILDNAMES; i++) 
	{	m_pGame->m_stGuildName[i].dwRefTime = 0;
		m_pGame->m_stGuildName[i].iGuildRank = -1;
		ZeroMemory(m_pGame->m_stGuildName[i].cCharName, sizeof(m_pGame->m_stGuildName[i].cCharName));
		ZeroMemory(m_pGame->m_stGuildName[i].cGuildName, sizeof(m_pGame->m_stGuildName[i].cGuildName));
	}

	for (i = 0; i < MAXCHATMSGS; i++) {
		if (m_pGame->m_pChatMsgList[i] != NULL) delete m_pGame->m_pChatMsgList[i];
		m_pGame->m_pChatMsgList[i] = NULL;
	}

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	// PlayerObjectID
	sp = (short *)cp;
	m_pGame->m_sPlayerObjectID = *sp;
	cp += 2;

	sp = (short *)cp;
	sX = *sp;
	cp += 2;

	sp = (short *)cp;
	sY = *sp;
	cp += 2;

	sp = (short *)cp;
	m_pGame->m_sPlayerType = *sp;
	cp += 2;

	sp = (short *)cp;
	m_pGame->m_sPlayerAppr1 = *sp;
	cp += 2;

	sp = (short *)cp;
	m_pGame->m_sPlayerAppr2 = *sp;
	cp += 2;

	sp = (short *)cp;
	m_pGame->m_sPlayerAppr3 = *sp;
	cp += 2;

	sp = (short *)cp;
	m_pGame->m_sPlayerAppr4 = *sp;
	cp += 2;

	ip = (int *)cp; 
	m_pGame->m_iPlayerApprColor = *ip;
	cp += 4;

	// CLEROTH - BLACK FIX
	ip = (int *)cp;
	m_pGame->m_iPlayerStatus = *ip;
	cp += 4;

	ZeroMemory(m_pGame->m_cMapName, sizeof(m_pGame->m_cMapName));
	ZeroMemory(m_pGame->m_cMapMessage, sizeof(m_pGame->m_cMapMessage));
	memcpy(m_pGame->m_cMapName, cp, 10);
	m_pGame->m_cMapIndex = m_pGame->GetOfficialMapName(m_pGame->m_cMapName, m_pGame->m_cMapMessage);
	if( m_pGame->m_cMapIndex < 0 )
	{	m_pGame->m_stDialogBoxInfo[9].sSizeX = -1;
		m_pGame->m_stDialogBoxInfo[9].sSizeY = -1;
	}else
	{	m_pGame->m_stDialogBoxInfo[9].sSizeX = 128;
		m_pGame->m_stDialogBoxInfo[9].sSizeY = 128;
	}
	cp += 10;

	strcpy( cPreCurLocation, m_pGame->m_cCurLocation );
	ZeroMemory(m_pGame->m_cCurLocation, sizeof(m_pGame->m_cCurLocation));
	memcpy(m_pGame->m_cCurLocation, cp, 10);
	cp += 10;

	G_cSpriteAlphaDegree = *cp;
	cp++;

	m_pGame->m_cWhetherStatus = *cp;
	cp++;
	switch (G_cSpriteAlphaDegree) { //Snoopy:  Xmas bulbs
	// Will be sent by server if DayTime is 3 (and a snowy weather)
	case 1:	m_pGame->m_bIsXmas = FALSE; break;
	case 2: m_pGame->m_bIsXmas = FALSE; break;
	case 3: // Snoopy Special night with chrismas bulbs
		if (m_pGame->m_cWhetherStatus >3) m_pGame->m_bIsXmas = TRUE;
		else m_pGame->m_bIsXmas = FALSE;
		G_cSpriteAlphaDegree = 2;
		break;
	}
	ip = (int *)cp;
	m_pGame->m_iContribution = *ip;
//	m_iContributionPrice = 0;
	cp += 4;
	bIsObserverMode = (BOOL)*cp;
	cp++;
	ip = (int *)cp;
//	m_iRating = *ip;
	cp += 4;
	ip = (int *)cp;
	m_pGame->m_iHP = *ip;
	cp += 4;
    m_pGame->m_cDiscount = (char )*cp;
    cp++;

	if (m_pGame->m_cWhetherStatus != NULL)
		 m_pGame->SetWhetherStatus(TRUE, m_pGame->m_cWhetherStatus);
	else m_pGame->SetWhetherStatus(FALSE, m_pGame->m_cWhetherStatus);

	ZeroMemory(cMapFileName, sizeof(cMapFileName));
	strcat(cMapFileName, "MapData\\");
	// CLEROTH - MW MAPS
	if(memcmp(m_pGame->m_cMapName,"defaultmw", 9)==0)
	{	strcat(cMapFileName, "mw\\defaultmw");
	}else
	{	strcat(cMapFileName, m_pGame->m_cMapName);
	}

	strcat(cMapFileName, ".amd");
	m_pGame->m_pMapData->OpenMapDataFile(cMapFileName);

	m_pGame->m_pMapData->m_sPivotX = sX;
	m_pGame->m_pMapData->m_sPivotY = sY;

	m_pGame->m_sPlayerX   = sX + 14 + 5;
	m_pGame->m_sPlayerY   = sY + 12 + 5;

	m_pGame->m_cPlayerDir = 5;

	if (bIsObserverMode == FALSE)
	{	m_pGame->m_pMapData->bSetOwner(m_pGame->m_sPlayerObjectID, m_pGame->m_sPlayerX, m_pGame->m_sPlayerY, m_pGame->m_sPlayerType, m_pGame->m_cPlayerDir,
							                  m_pGame->m_sPlayerAppr1, m_pGame->m_sPlayerAppr2, m_pGame->m_sPlayerAppr3, m_pGame->m_sPlayerAppr4, m_pGame->m_iPlayerApprColor, 
											  m_pGame->m_iPlayerStatus, m_pGame->m_cPlayerName,
											  OBJECTSTOP, NULL, NULL, NULL);
	}

	m_pGame->m_sViewDstX = m_pGame->m_sViewPointX = (sX+4+5)*32;
	m_pGame->m_sViewDstY = m_pGame->m_sViewPointY = (sY+5+5)*32;
	m_pGame->_ReadMapData(sX + 4 + 5, sY + 5 + 5, cp);
	m_pGame->m_bIsRedrawPDBGS = TRUE;
    // ------------------------------------------------------------------------+
	wsprintf(cTxt, INITDATA_RESPONSE_HANDLER1, m_pGame->m_cMapMessage);
	m_pGame->AddEventList(cTxt, 10);

	/*if (  ( memcmp( m_pGame->m_cCurLocation, "middleland"	,10 ) == 0 )
		|| ( memcmp( m_pGame->m_cCurLocation, "dglv2"		, 5 ) == 0 )
		|| ( memcmp( m_pGame->m_cCurLocation, "middled1n"	, 9 ) == 0 ))
    	m_pGame->EnableDialogBox(6, NULL,NULL, NULL);*/

// Snoopy: removed for v351 compatibility. Maybe usefull later...
/*	BOOL bPrevSafe, bNowSafe;
	if( memcmp( cPreCurLocation, m_pGame->m_cLocation, 3 ) == 0 )
		bPrevSafe = TRUE;
	else bPrevSafe = FALSE;

	if( memcmp( m_pGame->m_cCurLocation, m_pGame->m_cLocation, 3 ) == 0 )
		bNowSafe = TRUE;
	else bNowSafe = FALSE;

	if( memcmp( m_pGame->m_cCurLocation, "2nd", 3 ) == 0 ) bNowSafe = TRUE;
	if( m_pGame->m_iPKCount != 0 ) bNowSafe = FALSE;

	if( bPrevSafe )
	{	if( bNowSafe == FALSE ) m_pGame->SetTopMsg(MSG_DANGERZONE, 5);
	}else
	{	if( bNowSafe ) m_pGame->SetTopMsg(MSG_SAFEZONE, 5);
	}*/

    // ------------------------------------------------------------------------+

	m_pGame->ChangeGameMode(GAMEMODE_ONMAINGAME);
	m_pGame->m_DDraw.ClearBackB4();

	//v1.41
	if ((m_pGame->m_sPlayerAppr2 & 0xF000) != 0)
		 m_pGame->m_bIsCombatMode = TRUE;
	else m_pGame->m_bIsCombatMode = FALSE;

	//v1.42
	if (m_pGame->m_bIsFirstConn == TRUE) 
	{	m_pGame->m_bIsFirstConn = FALSE;
		hFile = CreateFile("contents\\contents1000.txt", GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			dwFileSize = 0;
		else 
		{	dwFileSize = GetFileSize(hFile, NULL);
			CloseHandle(hFile);
		}
		m_pGame->bSendCommand(MSGID_REQUEST_NOTICEMENT, NULL, NULL, (int)dwFileSize, NULL, NULL, NULL);
	}
	//cp += 2;

	m_pGame->LoadMuteList();
}

void CMessageHandler::MotionResponseHandler(char * Data)
{
 WORD  * wp, wResponse;
 short * sp, sX, sY;
 char  * cp, cDir;
 int   * ip, iPreHP;
	//						          0 3        4 5						 6 7		8 9		   10	    11
	// Confirm Code(4) | MsgSize(4) | MsgID(4) | OBJECTMOVE_CONFIRM(2) | Loc-X(2) | Loc-Y(2) | Dir(1) | MapData ...
	// Confirm Code(4) | MsgSize(4) | MsgID(4) | OBJECTMOVE_REJECT(2)  | Loc-X(2) | Loc-Y(2)
	wp = (WORD *)(Data + INDEX2_MSGTYPE);
	wResponse = *wp;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	switch (wResponse) {
	case OBJECTMOTION_CONFIRM:
		m_pGame->m_cCommandCount--;
		break;

	case OBJECTMOTION_ATTACK_CONFIRM:
		m_pGame->m_cCommandCount--;
		break;

	case OBJECTMOTION_REJECT:

		if (m_pGame->m_iHP <= 0) return;

		sp = (short *)cp;
		m_pGame->m_sPlayerX = *sp;
		cp += 2;

		sp = (short *)cp;
		m_pGame->m_sPlayerY = *sp;
		cp += 2;

		m_pGame->m_cCommand = OBJECTSTOP;
		m_pGame->m_sCommX = m_pGame->m_sPlayerX;
		m_pGame->m_sCommY = m_pGame->m_sPlayerY;

		m_pGame->m_pMapData->bSetOwner(m_pGame->m_sPlayerObjectID, m_pGame->m_sPlayerX, m_pGame->m_sPlayerY, m_pGame->m_sPlayerType, m_pGame->m_cPlayerDir,
						                  m_pGame->m_sPlayerAppr1, m_pGame->m_sPlayerAppr2, m_pGame->m_sPlayerAppr3, m_pGame->m_sPlayerAppr4, m_pGame->m_iPlayerApprColor,
										  m_pGame->m_iPlayerStatus, m_pGame->m_cPlayerName,
										  OBJECTSTOP, NULL, NULL, NULL, NULL, 5);
		m_pGame->m_cCommandCount = 0;
		m_pGame->m_bIsGetPointingMode = FALSE;
		m_pGame->m_sViewDstX = m_pGame->m_sViewPointX = (m_pGame->m_sPlayerX-10)*32;
		m_pGame->m_sViewDstY = m_pGame->m_sViewPointY = (m_pGame->m_sPlayerY-7)*32;

		m_pGame->m_bIsRedrawPDBGS = TRUE;
		break;

	case OBJECTMOVE_CONFIRM:
		sp = (short *)cp;
		sX = *sp;
		cp += 2;
		sp = (short *)cp;
		sY = *sp;
		cp += 2;
		cDir = *cp;
		cp++;
		m_pGame->m_iSP = m_pGame->m_iSP - *cp;
		cp++;
		if( m_pGame->m_iSP < 0 ) m_pGame->m_iSP = 0;

		iPreHP = m_pGame->m_iHP;
		ip = (int *)cp;
		m_pGame->m_iHP = *ip;
		cp += 4;

		if (m_pGame->m_iHP != iPreHP)
		{	if (m_pGame->m_iHP < iPreHP)
			{	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_HP_DOWN, iPreHP - m_pGame->m_iHP);
				m_pGame->AddEventList(m_pGame->G_cTxt, 10);
				m_pGame->m_dwDamagedTime = timeGetTime();
				if ((m_pGame->m_cLogOutCount>0) && (m_pGame->m_bForceDisconn==FALSE))
				{	m_pGame->m_cLogOutCount = -1;
					m_pGame->AddEventList(MOTION_RESPONSE_HANDLER2, 10);
				}
			}
		}
		m_pGame->m_pMapData->ShiftMapData(cDir);
		m_pGame->_ReadMapData(sX, sY, cp);
		m_pGame->m_bIsRedrawPDBGS = TRUE;
		m_pGame->m_cCommandCount--;
		break;

	case OBJECTMOVE_REJECT:
		if (m_pGame->m_iHP <= 0) return;
		wp = (WORD *)cp;
		if (m_pGame->m_sPlayerObjectID != *wp) return;
		cp += 2;
		sp  = (short *)cp;
		m_pGame->m_sPlayerX = *sp;
		cp += 2;
		sp  = (short *)cp;
		m_pGame->m_sPlayerY = *sp;
		cp += 2;
		sp  = (short *)cp;
		m_pGame->m_sPlayerType = *sp;
		cp += 2;
		m_pGame->m_cPlayerDir = *cp;
		cp++;
		//memcpy(cName, cp, 10);
		cp += 10;
		sp = (short *)cp;
		m_pGame->m_sPlayerAppr1 = *sp;
		cp += 2;
		sp = (short *)cp;
		m_pGame->m_sPlayerAppr2 = *sp;
		cp += 2;
		sp = (short *)cp;
		m_pGame->m_sPlayerAppr3 = *sp;
		cp += 2;
		sp = (short *)cp;
		m_pGame->m_sPlayerAppr4 = *sp;
		cp += 2;
		ip = (int *)cp;
		m_pGame->m_iPlayerApprColor = *ip;
		cp += 4;
		ip = (int *)cp;
		m_pGame->m_iPlayerStatus = *ip;
		cp += 4;
		m_pGame->m_cCommand = OBJECTSTOP;
		m_pGame->m_sCommX = m_pGame->m_sPlayerX;
		m_pGame->m_sCommY = m_pGame->m_sPlayerY;
		m_pGame->m_pMapData->bSetOwner(m_pGame->m_sPlayerObjectID, m_pGame->m_sPlayerX, m_pGame->m_sPlayerY, m_pGame->m_sPlayerType, m_pGame->m_cPlayerDir,
						                  m_pGame->m_sPlayerAppr1, m_pGame->m_sPlayerAppr2, m_pGame->m_sPlayerAppr3, m_pGame->m_sPlayerAppr4, m_pGame->m_iPlayerApprColor, 
										  m_pGame->m_iPlayerStatus, m_pGame->m_cPlayerName,
										  OBJECTSTOP, NULL, NULL, NULL,
										  0, 11);
		m_pGame->m_cCommandCount = 0;
		m_pGame->m_bIsGetPointingMode = FALSE;
		m_pGame->m_sViewDstX = m_pGame->m_sViewPointX = (m_pGame->m_sPlayerX-10)*32;
		m_pGame->m_sViewDstY = m_pGame->m_sViewPointY = (m_pGame->m_sPlayerY-7)*32;
		m_pGame->m_bIsPrevMoveBlocked = TRUE;
		switch (m_pGame->m_sPlayerType) {
		case 1:
		case 2:
		case 3:
			m_pGame->PlaySound('C', 12, 0);
			break;
		case 4:
		case 5:
		case 6:
			m_pGame->PlaySound('C', 13, 0);
			break;
		}
		//m_bCommandAvailable = TRUE;
		break;
	}
}

void CMessageHandler::CommonEventHandler(char * Data)
{
 WORD * wp, wEventType;
 short * sp, sX, sY, sV1, sV2, sV3, sV4;
 char * cp;

	wp   = (WORD *)(Data + INDEX2_MSGTYPE);
	wEventType = *wp;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	sp  = (short *)cp;
	sX  = *sp;
	cp += 2;

	sp  = (short *)cp;
	sY  = *sp;
	cp += 2;

	sp  = (short *)cp;
	sV1 = *sp;
	cp += 2;

	sp  = (short *)cp;
	sV2 = *sp;
	cp += 2;

	sp  = (short *)cp;
	sV3 = *sp;
	cp += 2;

	sp  = (short *)cp;
	sV4 = *sp;
	cp += 2;

	switch (wEventType) {
	case COMMONTYPE_ITEMDROP:
		if ((sV1 == 6) && (sV2 == 0)) {
			m_pGame->bAddNewEffect(4, sX, sY, NULL, NULL, 0);
		}
		m_pGame->m_pMapData->bSetItem(sX, sY, sV1, sV2, (char)sV3);
		break;

	case COMMONTYPE_SETITEM:
		m_pGame->m_pMapData->bSetItem(sX, sY, sV1, sV2, (char)sV3, FALSE); 
		break;

	case COMMONTYPE_MAGIC:
		m_pGame->bAddNewEffect(sV3, sX, sY, sV1, sV2, 0, sV4);
		break;

	case COMMONTYPE_CLEARGUILDNAME:
		m_pGame->ClearGuildNameList();
		break;
	}
}

void CMessageHandler::MotionEventHandler(char* Data) {
	WORD* wp, wEventType, wObjectID;
	short* sp, sX = 0, sY = 0, sType = 0, sV1 = 0, sV2 = 0, sV3 = 0, sPrevAppr2 = 0;
	short sAppr1 = 0, sAppr2 = 0, sAppr3 =0, sAppr4 = 0;
	int iStatus = 0;
	char* cp, cDir, cName[12];
	int* ip, iLoc = 0;
	int iApprColor = 0x00; 
	char cTxt[300];
	int i;
	ZeroMemory(cName, sizeof(cName));
	sV1 = sV2 = sV3 = NULL;
	wp   = (WORD *)(Data + INDEX2_MSGTYPE);
	wEventType = *wp;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	wp = (WORD *)cp;
	wObjectID = *wp;
	cp += 2;
	iLoc = 0;
	if (wObjectID < 30000)
	{	if (wObjectID < 10000) 	// Player
		{	sp  = (short *)cp;
			sX = *sp;
			cp += 2;
			sp  = (short *)cp;
			sY = *sp;
			cp += 2;
			sp  = (short *)cp;
			sType = *sp;
			cp += 2;
			cDir = *cp;
			cp++;
			memcpy(cName, cp, 10);
			cp += 10;
			sp  = (short *)cp;
			sAppr1 = *sp;
			cp += 2;
			sp  = (short *)cp;
			sAppr2 = *sp;
			cp += 2;
			sp  = (short *)cp;
			sAppr3 = *sp;
			cp += 2;
			sp  = (short *)cp;
			sAppr4 = *sp;
			cp += 2;
			ip = (int *)cp; 
			iApprColor = *ip;
			cp += 4;

			// CLEROTH - CRASH BUG ( STATUS )
			ip  = (int *)cp;
			iStatus = *ip;
			cp += 4;

			iLoc = *cp;
			cp++;
		}else 	// Npc or mob
		{	sp  = (short *)cp;
			sX = *sp;
			cp += 2;
			sp  = (short *)cp;
			sY = *sp;
			cp += 2;
			sp  = (short *)cp;
			sType = *sp;
			cp += 2;
			cDir = *cp;
			cp++;
			memcpy(cName, cp, 5);
			cp += 5;
			sAppr1 = sAppr3 = sAppr4 = 0;
			sp  = (short *)cp;
			sAppr2 = *sp;
			cp += 2;
			ip  = (int *)cp;
			iStatus = *ip;
			cp += 4;
			iLoc = *cp;
			cp++;
		}
	}else
	{	switch (wEventType) {
		case OBJECTMAGIC:
		case OBJECTDAMAGEMOVE:
		case OBJECTDAMAGE:
			cDir = *cp;
			cp++;
			sV1 = (short)*cp; //Damage
			cp++;
			sV2 = (short)*cp; //
			cp++;
  			break;

		case OBJECTDYING:
			cDir = *cp;
			cp++;
			sV1 = (short)*cp; //Damage
			cp++;
			sV2 = (short)*cp; //
			cp++;
			sp  = (short *)cp;
			sX = *sp;
			cp += 2;
			sp  = (short *)cp;
			sY = *sp;
			cp += 2;
			break;

		case OBJECTATTACK:
			cDir = *cp;
			cp++;
			sV1 = *cp;
			cp++;
			sV2 = *cp;
			cp++;
			sp = (short *)cp;
			sV3 = *sp;
			cp += 2;
			break;

		default:
			cDir = *cp;
			cp++;
			break;
	}	}
	
	if ((wEventType == OBJECTNULLACTION) && (memcmp(cName, m_pGame->m_cPlayerName, 10) == 0))
	
	{	
		m_pGame->m_sPlayerType   = sType;
		m_pGame->m_sPlayerAppr1  = sAppr1;
		sPrevAppr2      = m_pGame->m_sPlayerAppr2;
		m_pGame->m_sPlayerAppr2  = sAppr2;
		m_pGame->m_sPlayerAppr3  = sAppr3;
		m_pGame->m_sPlayerAppr4  = sAppr4;
		m_pGame->m_iPlayerApprColor = iApprColor;
		m_pGame->m_iPlayerStatus    = iStatus;
		if ((sPrevAppr2 & 0xF000) == 0)
		{	if ((sAppr2 & 0xF000) != 0)
			{	m_pGame->AddEventList(MOTION_EVENT_HANDLER1, 10);
				m_pGame->m_bIsCombatMode = TRUE;
			}
		}else
		{	if ((sAppr2 & 0xF000) == 0)
			{	m_pGame->AddEventList(MOTION_EVENT_HANDLER2, 10);
				m_pGame->m_bIsCombatMode = FALSE;
		}	}
		if (m_pGame->m_cCommand != OBJECTRUN && m_pGame->m_cCommand != OBJECTMAGIC) m_pGame->m_pMapData->bSetOwner(wObjectID, sX, sY, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, (char)wEventType, sV1, sV2, sV3, iLoc);
	}else if(!(wObjectID -30000 == m_pGame->m_sPlayerObjectID && wEventType == OBJECTDYING && (m_pGame->m_cCommand == OBJECTRUN || m_pGame->m_cCommand == OBJECTMOVE)))
		m_pGame->m_pMapData->bSetOwner(wObjectID, sX, sY, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, (char)wEventType, sV1, sV2, sV3, iLoc);

	switch (wEventType) {
	case OBJECTMAGIC: // Casting
		m_pGame->_RemoveChatMsgListByObjectID(wObjectID - 30000);

		for (i = 1; i < MAXCHATMSGS; i++)
		if (m_pGame->m_pChatMsgList[i] == NULL)
		{	ZeroMemory(cTxt, sizeof(cTxt));
			wsprintf(cTxt, "%s!", m_pGame->m_pMagicCfgList[sV1]->m_cName);
			m_pGame->m_pChatMsgList[i] = new class CMsg(41, cTxt, m_pGame->m_dwCurTime);
			m_pGame->m_pChatMsgList[i]->m_iObjectID = wObjectID - 30000;
			if (m_pGame->m_pMapData->bSetChatMsgOwner(wObjectID - 30000, -10, -10, i) == FALSE)
			{	delete m_pGame->m_pChatMsgList[i];
				m_pGame->m_pChatMsgList[i] = NULL;
			}
			return;
		}
		break;

	case OBJECTDYING:
		for (i = 1; i < MAXCHATMSGS; i++)
		{
			if (m_pGame->m_pChatMsgList[i] == NULL)
			{	
				int index = m_pGame->m_pMapData->getChatMsgIndex(wObjectID - 30000);
				if(m_pGame->m_showAllDmg && m_pGame->m_pChatMsgList[index] && strlen(m_pGame->m_pChatMsgList[index]->m_pMsg) < sizeof(cTxt)-30)
				{
					if(index != -1 && m_pGame->m_dwCurTime - m_pGame->m_pChatMsgList[index]->m_dwTime < 150 _ms && 
						m_pGame->m_pChatMsgList[index]->m_cType >= 21 && m_pGame->m_pChatMsgList[index]->m_cType <= 23)	
					{				
						if (sV1 > 0)
							wsprintf(cTxt, "%s-%d!", m_pGame->m_pChatMsgList[index]->m_pMsg, sV1);
						else
							wsprintf(cTxt, "%s-Crit!", m_pGame->m_pChatMsgList[index]->m_pMsg, sV1);
					}
					else 
					{
						if (sV1 > 0)
							wsprintf(cTxt, "-%d!", sV1);
						else strcpy(cTxt, "Crit!");
					}
				}else{
					if (sV1 > 0)
						wsprintf(cTxt, "-%d!", sV1);
					else strcpy(cTxt, "Critical!");
				}
				int iFontType;
				if ((sV1 >= 0) && (sV1 < 12))		iFontType = 21;
				else if ((sV1 >= 12) && (sV1 < 40)) iFontType = 22;
				else if ((sV1 >= 40) || (sV1 < 0))	iFontType = 23;
				m_pGame->_RemoveChatMsgListByObjectID(wObjectID - 30000);
				m_pGame->m_pChatMsgList[i] = new class CMsg(iFontType, cTxt, m_pGame->m_dwCurTime);
				m_pGame->m_pChatMsgList[i]->m_iObjectID = wObjectID - 30000;
				if (m_pGame->m_pMapData->bSetChatMsgOwner(wObjectID - 30000, -10, -10, i) == FALSE)
				{	delete m_pGame->m_pChatMsgList[i];
				m_pGame->m_pChatMsgList[i] = NULL;
				}
				return;
			}
		}
		break;

	case OBJECTDAMAGEMOVE:
	case OBJECTDAMAGE:
		if (memcmp(cName, m_pGame->m_cPlayerName, 10) == 0)
		{	m_pGame->m_bIsGetPointingMode = FALSE;
			m_pGame->m_iPointCommandType	 = -1;
			m_pGame->m_stMCursor.sCursorFrame = 0;
			m_pGame->ClearSkillUsingStatus();
		}

		for (i = 1; i < MAXCHATMSGS; i++)
		if (m_pGame->m_pChatMsgList[i] == NULL)
		{	
			ZeroMemory(cTxt, sizeof(cTxt));
			if (sV1 != 0)
			{	
				int index = m_pGame->m_pMapData->getChatMsgIndex(wObjectID - 30000);
				if(m_pGame->m_showAllDmg && m_pGame->m_pChatMsgList[index] && strlen(m_pGame->m_pChatMsgList[index]->m_pMsg) < sizeof(cTxt)-30)
				{
					if(index != -1 && m_pGame->m_dwCurTime - m_pGame->m_pChatMsgList[index]->m_dwTime < 150 _ms && 
						m_pGame->m_pChatMsgList[index]->m_cType >= 21 && m_pGame->m_pChatMsgList[index]->m_cType <= 23)	
					{				
						if (sV1 > 0)
							wsprintf(cTxt, "%s-%d", m_pGame->m_pChatMsgList[index]->m_pMsg, sV1);
						else
							wsprintf(cTxt, "%s-Crit", m_pGame->m_pChatMsgList[index]->m_pMsg, sV1);
					}
					else 
					{
						if (sV1 > 0)
							wsprintf(cTxt, "-%d", sV1);
						else strcpy(cTxt, "Crit");
					}
				}else{
					if (sV1 > 0)
						wsprintf(cTxt, "-%d", sV1);
					else strcpy(cTxt, "Critical");
				}

				int iFontType;
				if ((sV1 >= 0) && (sV1 < 12))		iFontType = 21;
				else if ((sV1 >= 12) && (sV1 < 40)) iFontType = 22;
				else if ((sV1 >= 40) || (sV1 < 0))	iFontType = 23;

				
				m_pGame->_RemoveChatMsgListByObjectID(wObjectID - 30000);
				m_pGame->m_pChatMsgList[i] = new class CMsg(iFontType, cTxt, m_pGame->m_dwCurTime);
			}else
			{	
				strcpy(cTxt, " * Failed! *");
				m_pGame->_RemoveChatMsgListByObjectID(wObjectID - 30000);
				m_pGame->m_pChatMsgList[i] = new class CMsg(22, cTxt, m_pGame->m_dwCurTime);
				m_pGame->PlaySound('C', 17, 0);
			}
			m_pGame->m_pChatMsgList[i]->m_iObjectID = wObjectID - 30000;
			if (m_pGame->m_pMapData->bSetChatMsgOwner(wObjectID - 30000, -10, -10, i) == FALSE)
			{	delete m_pGame->m_pChatMsgList[i];
				m_pGame->m_pChatMsgList[i] = NULL;
			}
			return;
		}
		break;
	}
}

void CMessageHandler::LogEventHandler(char * Data)
{WORD * wp, wEventType, wObjectID;
 short * sp, sX, sY, sType, sAppr1, sAppr2, sAppr3, sAppr4;
 int iStatus;
 char  * cp, cDir, cName[12];
 int   * ip;
 int iApprColor = 0x00;
	wp   = (WORD *)(Data + INDEX2_MSGTYPE);
	wEventType = *wp;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp  = (WORD *)cp;
	wObjectID  = *wp;
	cp += 2;
	sp  = (short *)cp;
	sX  = *sp;
	cp += 2;
	sp  = (short *)cp;
	sY  = *sp;
	cp += 2;
	sp  = (short *)cp;
	sType = *sp;
	cp += 2;
	cDir = *cp;
	cp++;
	ZeroMemory(cName, sizeof(cName));
	if (wObjectID < 10000)
	{	memcpy(cName, cp, 10);
		cp += 10;
		sp  = (short *)cp;
		sAppr1 = *sp;
		cp += 2;
		sp  = (short *)cp;
		sAppr2 = *sp;
		cp += 2;
		sp  = (short *)cp;
		sAppr3 = *sp;
		cp += 2;
		sp  = (short *)cp;
		sAppr4 = *sp;
		cp += 2;
		ip = (int *)cp;
		iApprColor = *ip;
		cp += 4;
		// CLEROTH - CRASH BUG ( STATUS )
		ip  = (int *)cp;
		iStatus = *ip;
		cp += 4;
	}else 	// NPC
	{	memcpy(cName, cp, 5);
		cp += 5;
		sAppr1 = sAppr3 = sAppr4 = 0;
		sp  = (short *)cp;
		sAppr2 = *sp;
		cp += 2;
		ip  = (int *)cp;
		iStatus = *ip;
		cp += 4;
	}

	switch (wEventType) {
	case MSGTYPE_CONFIRM:
		m_pGame->m_pMapData->bSetOwner(wObjectID, sX, sY, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, OBJECTSTOP, NULL, NULL, NULL);
		switch (sType) {
		case 43: // LWB
		case 44: // GHK
		case 45: // GHKABS
		case 46: // TK
		case 47: // BG
			m_pGame->bAddNewEffect(64, (sX)*32 ,(sY)*32, NULL, NULL, 0);
			break;
		}
		break;

	case MSGTYPE_REJECT:
		m_pGame->m_pMapData->bSetOwner(wObjectID, -1, -1, sType, cDir, sAppr1, sAppr2, sAppr3, sAppr4, iApprColor, iStatus, cName, OBJECTSTOP, NULL, NULL, NULL);
		break;
	}

	m_pGame->_RemoveChatMsgListByObjectID(wObjectID);
}

void CMessageHandler::ChatMsgHandler(char * Data)
{
	int i, iObjectID, iLoc;
	short * sp, sX, sY;
	char * cp, cMsgType, cName[21], cTemp[100], cMsg[100], cTxt1[100], cTxt2[100];
	DWORD dwTime;
	WORD * wp;
	BOOL bFlag;

	dwTime = m_pGame->m_dwCurTime;

	ZeroMemory(cTxt1, sizeof(cTxt1));
	ZeroMemory(cTxt2, sizeof(cTxt2));
	ZeroMemory(cMsg, sizeof(cMsg));

	wp = (WORD *)(Data + INDEX2_MSGTYPE);
	iObjectID = (int)*wp;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	sp = (short *)cp;
	sX = *sp;
	cp += 2;

	sp = (short *)cp;
	sY = *sp;
	cp += 2;
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, (char *)cp, 10);
	cp += 10;

	cMsgType = *cp;
	cp++;

	ZeroMemory(cTemp, sizeof(cTemp));
	strcpy(cTemp, cp);

	if((m_pGame->m_MuteList.find(string(cName)) != m_pGame->m_MuteList.end()) ||
		(!m_pGame->m_bWhisper && cMsgType == CHAT_WHISPER) ||
		(!m_pGame->m_bShout && (cMsgType == CHAT_SHOUT || cMsgType == CHAT_NATIONSHOUT) ) ||
		((cMsgType==0 || cMsgType==2 || cMsgType==3) && !m_pGame->m_Misc.bCheckIMEString(cTemp)) )
	{
		return;
	}

	char timeStamp[50];
	m_pGame->TimeStamp(timeStamp);

	ZeroMemory(cMsg, sizeof(cMsg));
	if(cMsgType == CHAT_WHISPER && (memcmp(m_pGame->m_cPlayerName, cName, 10) == 0))\
	{
		if (m_pGame->m_showTimeStamp) 
			wsprintf(cMsg, "[%s](%s) %s: %s", timeStamp, m_pGame->m_cWhisperName, cName, cTemp);
		else
			wsprintf(cMsg, "(%s) %s: %s", m_pGame->m_cWhisperName, cName, cTemp);
	}else{
		if (m_pGame->m_showTimeStamp) 
			wsprintf(cMsg, "[%s] %s: %s", timeStamp, cName, cTemp);
		else
			wsprintf(cMsg, "%s: %s", cName, cTemp);
	}

	m_pGame->m_DDraw._GetBackBufferDC();
	bFlag = FALSE;
	short sCheckByte = 0;
	while (bFlag == FALSE)
	{	iLoc = m_pGame->m_Misc.iGetTextLengthLoc(m_pGame->m_DDraw.m_hDC, cMsg, 305);
		for( int i=0 ; i<iLoc ; i++ ) if( cMsg[i] < 0 ) sCheckByte ++;
		if (iLoc == 0)
		{	m_pGame->PutChatScrollList(cMsg, cMsgType);
			bFlag = TRUE;
		}else
		{	if ((sCheckByte%2)==0)
			{	ZeroMemory(cTemp, sizeof(cTemp));
				memcpy(cTemp, cMsg, iLoc);
				m_pGame->PutChatScrollList(cTemp, cMsgType);
				ZeroMemory(cTemp, sizeof(cTemp));
				strcpy(cTemp, cMsg +iLoc );
				ZeroMemory(cMsg, sizeof(cMsg));
				strcpy(cMsg, " ");
				strcat(cMsg, cTemp);
			}else
			{	ZeroMemory(cTemp, sizeof(cTemp));
				memcpy(cTemp, cMsg, iLoc+1);
				m_pGame->PutChatScrollList(cTemp, cMsgType);
				ZeroMemory(cTemp, sizeof(cTemp));
				strcpy(cTemp, cMsg +iLoc+1);
				ZeroMemory(cMsg, sizeof(cMsg));
				strcpy(cMsg, " ");
				strcat(cMsg, cTemp);
	}	}	}

	m_pGame->m_DDraw._ReleaseBackBufferDC();

	m_pGame->_RemoveChatMsgListByObjectID(iObjectID);

	for (i = 1; i < MAXCHATMSGS; i++)
	if (m_pGame->m_pChatMsgList[i] == NULL) {
		m_pGame->m_pChatMsgList[i] = new class CMsg(1, (char *)(cp), dwTime);
		m_pGame->m_pChatMsgList[i]->m_iObjectID = iObjectID;

		if (m_pGame->m_pMapData->bSetChatMsgOwner(iObjectID, sX, sY, i) == FALSE) {
			delete m_pGame->m_pChatMsgList[i];
			m_pGame->m_pChatMsgList[i] = NULL;
		}

		char cHeadMsg[200];

		if ( (cMsgType != 0) && (m_pGame->m_bIsDialogEnabled[10] != TRUE) ) {
			ZeroMemory(cHeadMsg, sizeof(cHeadMsg));
			if(cMsgType == CHAT_WHISPER && (memcmp(m_pGame->m_cPlayerName, cName, 10) == 0))
			{
				if (m_pGame->m_showTimeStamp) 
					wsprintf(cHeadMsg, "[%s](%s) %s: %s", timeStamp, m_pGame->m_cWhisperName, cName, cp);
				else
					wsprintf(cHeadMsg, "(%s) %s: %s", m_pGame->m_cWhisperName, cName, cp);
			}else{
				if (m_pGame->m_showTimeStamp)
					wsprintf(cHeadMsg, "[%s] %s: %s", timeStamp, cName, cp);
				else
					wsprintf(cHeadMsg, "%s: %s", cName, cp);
			}
			m_pGame->AddEventList(cHeadMsg, cMsgType);
		}
		return;
	}
}

void CMessageHandler::InitItemList(char * Data)
{char    cTotalItems;
 int     i, iAngelValue;
 short * sp;
 DWORD * dwp;
 WORD  * wp;
 char  * cp;

	for (i = 0; i < MAXITEMS; i++)
		m_pGame->m_cItemOrder[i] = -1;

	for (i = 0; i < MAXITEMEQUIPPOS; i++)
		m_pGame->m_sItemEquipmentStatus[i] = -1;

	for (i = 0; i < MAXITEMS; i++)
		m_pGame->m_bIsItemDisabled[i] = FALSE;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	cTotalItems = *cp;
	cp++;

	for (i = 0; i < MAXITEMS; i++)
	if (m_pGame->m_pItemList[i] != NULL) 
	{	delete m_pGame->m_pItemList[i];
		m_pGame->m_pItemList[i] = NULL;
	}

	for (auto* p : m_pGame->m_pBankList) delete p;
	m_pGame->m_pBankList.clear();

	for (i = 0; i < cTotalItems; i++)
	{	m_pGame->m_pItemList[i] = new class CItem;
		memcpy(m_pGame->m_pItemList[i]->m_cName, cp, 20);
		cp += 20;
		dwp = (DWORD *)cp;
		m_pGame->m_pItemList[i]->m_dwCount = *dwp;
		m_pGame->m_pItemList[i]->m_sX      =	40;
		m_pGame->m_pItemList[i]->m_sY      =	30;
		cp += 4;
		m_pGame->m_pItemList[i]->m_cItemType = *cp;
		cp++;
		m_pGame->m_pItemList[i]->m_cEquipPos = *cp;
		cp++;
		if( *cp == 0 ) m_pGame->m_bIsItemEquipped[i] = FALSE;
		else m_pGame->m_bIsItemEquipped[i] = TRUE;
		cp++;
		if (m_pGame->m_bIsItemEquipped[i] == TRUE)
		{	m_pGame->m_sItemEquipmentStatus[m_pGame->m_pItemList[i]->m_cEquipPos] = i;
		}
		sp = (short *)cp;
		m_pGame->m_pItemList[i]->m_sLevelLimit = *sp;
		cp += 2;
		m_pGame->m_pItemList[i]->m_cGenderLimit = *cp;
		cp++;
		wp =(WORD *)cp;
		m_pGame->m_pItemList[i]->m_wCurLifeSpan = *wp;
		cp += 2;
		wp =(WORD *)cp;
		m_pGame->m_pItemList[i]->m_wWeight = *wp;
		cp += 2;
		sp = (short *)cp;
		m_pGame->m_pItemList[i]->m_sSprite = *sp;
		cp += 2;
		sp = (short *)cp;
		m_pGame->m_pItemList[i]->m_sSpriteFrame = *sp;
		cp += 2;
		m_pGame->m_pItemList[i]->m_cItemColor = *cp;
		cp++;
		m_pGame->m_pItemList[i]->m_sItemSpecEffectValue2 = (short)*cp; 
		cp++;
		dwp = (DWORD *)cp;
		m_pGame->m_pItemList[i]->m_dwAttribute = *dwp;
		cp += 4;
		/*
		m_pGame->m_pItemList[i]->m_bIsCustomMade = (BOOL)*cp;
		cp++;
		*/
		m_pGame->m_cItemOrder[i] = i;
		// Snoopy: Add Angelic Stats
		if (   (m_pGame->m_pItemList[i]->m_cItemType == 1)
			&& (m_pGame->m_bIsItemEquipped[i] == TRUE)
			&& (m_pGame->m_pItemList[i]->m_cEquipPos >= 11))
		{	if(memcmp(m_pGame->m_pItemList[i]->m_cName, "AngelicPendant(STR)", 19) == 0)
			{	iAngelValue = (m_pGame->m_pItemList[i]->m_dwAttribute & 0xF0000000) >> 28;
				m_pGame->m_angelStat[STAT_STR] = 1 + iAngelValue;
			}else if(memcmp(m_pGame->m_pItemList[i]->m_cName, "AngelicPendant(DEX)", 19) == 0)
			{	iAngelValue = (m_pGame->m_pItemList[i]->m_dwAttribute & 0xF0000000) >> 28;
				m_pGame->m_angelStat[STAT_DEX] = 1 + iAngelValue;
			}else if(memcmp(m_pGame->m_pItemList[i]->m_cName, "AngelicPendant(INT)", 19) == 0)
			{	iAngelValue = (m_pGame->m_pItemList[i]->m_dwAttribute & 0xF0000000) >> 28;
				m_pGame->m_angelStat[STAT_INT] = 1 + iAngelValue;
			}else if(memcmp(m_pGame->m_pItemList[i]->m_cName, "AngelicPendant(MAG)", 19) == 0)
			{	iAngelValue = (m_pGame->m_pItemList[i]->m_dwAttribute & 0xF0000000) >> 28;
				m_pGame->m_angelStat[STAT_MAG] = 1 + iAngelValue;
	}	}	}

	cTotalItems = *cp;
	cp++;

	for (auto* p : m_pGame->m_pBankList) delete p;
	m_pGame->m_pBankList.clear();

	for (i = 0; i < cTotalItems; i++)
	{	m_pGame->m_pBankList.push_back(new class CItem);
		memcpy(m_pGame->m_pBankList[i]->m_cName, cp, 20);
		cp += 20;

		dwp = (DWORD *)cp;
		m_pGame->m_pBankList[i]->m_dwCount = *dwp;
		cp += 4;

		m_pGame->m_pBankList[i]->m_sX = 40;
		m_pGame->m_pBankList[i]->m_sY = 30;

		m_pGame->m_pBankList[i]->m_cItemType = *cp;
		cp++;

		m_pGame->m_pBankList[i]->m_cEquipPos = *cp;
		cp++;

		sp = (short *)cp;
		m_pGame->m_pBankList[i]->m_sLevelLimit = *sp;
		cp += 2;

		m_pGame->m_pBankList[i]->m_cGenderLimit = *cp;
		cp++;

		wp =(WORD *)cp;
		m_pGame->m_pBankList[i]->m_wCurLifeSpan = *wp;
		cp += 2;

		wp =(WORD *)cp;
		m_pGame->m_pBankList[i]->m_wWeight = *wp;
		cp += 2;

		sp = (short *)cp;
		m_pGame->m_pBankList[i]->m_sSprite = *sp;
		cp += 2;

		sp = (short *)cp;
		m_pGame->m_pBankList[i]->m_sSpriteFrame = *sp;
		cp += 2;

		m_pGame->m_pBankList[i]->m_cItemColor = *cp;
		cp++;

		m_pGame->m_pBankList[i]->m_sItemSpecEffectValue2 = (short)*cp; 
		cp++;

		dwp = (DWORD *)cp;
		m_pGame->m_pBankList[i]->m_dwAttribute = *dwp;
		cp += 4;
		/*
		m_pGame->m_pBankList[i]->m_bIsCustomMade = (BOOL)*cp;
		cp++;
		*/
	}

	
}

void CMessageHandler::InitSkillList(char * Data)
{
	char * cp;
	short * sp;
	int i, * ip;
	WORD * wp;

	cp = (char *)(Data + 6);

	for (i = 0; i < 10; i++) {
		ip = (int *)cp;
		m_pGame->m_cSkillMastery[i] = *ip;
		if (m_pGame->m_pSkillCfgList[i] != NULL)
			m_pGame->m_pSkillCfgList[i]->m_iLevel = (int)m_pGame->m_cSkillMastery[i];
		cp += 2;
	}
	// Magic, Skill Mastery
	for (i = 0; i < MAXMAGICTYPE; i++)
	{	
		m_pGame->m_cMagicMastery[i] = *cp;
		cp++;
	}

	//sp = (short *) cp ;
	//	sSkillIndex = (short) *sp ;
	//	cp += 2 ;
	//
	//	for ( i = 0; i < DEF_MAXSKILLTYPE; i++) {	
	//		m_pClientList[iClientH]->m_cSkillMastery[i] = 0 ;
	//		m_pClientList[iClientH]->m_iSkillSSN[i] = 0 ;
	//	}
	//
	//	for ( i = 0; i < sSkillIndex; i++ ) {
	//
	//		sp = (short *) cp ;
	//		sSkillID = (short) *sp ;
	//		cp += 2 ;
	//
	//		sp = (short *) cp ;
	//		m_pClientList[iClientH]->m_cSkillMastery[sSkillID] = (short) *sp ;
	//		cp += 2 ;
	//		 
	//		ip = (int *) cp ;
	//		m_pClientList[iClientH]->m_iSkillSSN[sSkillID] = (int) *ip ;
	//		cp += 4 ;
	//}
	//	
}

void CMessageHandler::CreateNewGuildResponseHandler(char * Data)
{
	WORD * wpResult;
 	wpResult = (WORD *)(Data + INDEX2_MSGTYPE);
	switch (*wpResult) {
	case MSGTYPE_CONFIRM:
		m_pGame->m_iGuildRank = 0;
		m_pGame->m_stDialogBoxInfo[7].cMode = 3;
		break;
	case MSGTYPE_REJECT:
		m_pGame->m_iGuildRank = -1;
		m_pGame->m_stDialogBoxInfo[7].cMode = 4;
		break;
	}
}

void CMessageHandler::DisbandGuildResponseHandler(char * Data)
{WORD * wpResult;
 	wpResult = (WORD *)(Data + INDEX2_MSGTYPE);
	switch (*wpResult) {
	case MSGTYPE_CONFIRM:
		ZeroMemory(m_pGame->m_cGuildName, sizeof(m_pGame->m_cGuildName));
		m_pGame->m_iGuildRank = -1;
		m_pGame->m_stDialogBoxInfo[7].cMode = 7;
		break;
	case MSGTYPE_REJECT:
		m_pGame->m_stDialogBoxInfo[7].cMode = 8;
		break;
	}
}

void CMessageHandler::InitPlayerCharacteristics(char * Data)
{
 int  * ip;
 char * cp;
 WORD * wp;

	for(int i=0;i < 6; i++)
		m_pGame->m_angelStat[i] = 0;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
 
	ip   = (int *)cp;
	m_pGame->m_iHP = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_iMP = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_iSP = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_iAC = *ip;		
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_iTHAC0 = *ip;    
	cp += 4;		 

	ip   = (int *)cp;
	m_pGame->m_iLevel = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_createStat[STAT_STR] = m_pGame->m_stat[STAT_STR] = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_createStat[STAT_INT] = m_pGame->m_stat[STAT_INT] = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_createStat[STAT_VIT] = m_pGame->m_stat[STAT_VIT] = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_createStat[STAT_DEX] = m_pGame->m_stat[STAT_DEX] = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_createStat[STAT_MAG] = m_pGame->m_stat[STAT_MAG] = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_createStat[STAT_CHR] = m_pGame->m_stat[STAT_CHR] = *ip;
	cp += 4;

	wp   = (WORD *)cp;
	m_pGame->m_iLU_Point = (*wp - 3);
	cp += 2;

	ip   = (int *)cp;
	m_pGame->m_iExp = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_iEnemyKillCount = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_iPKCount = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_iRewardGold = *ip;
	cp += 4;

	memcpy(m_pGame->m_cLocation, cp, 10);
	cp += 10;

	if (memcmp(m_pGame->m_cLocation, "are", 3) == 0)
		m_pGame->m_side = ARESDEN;
	else if (memcmp(m_pGame->m_cLocation, "elv", 3) == 0)
		m_pGame->m_side = ELVINE;
	else if (memcmp(m_pGame->m_cLocation, "ist", 3) == 0)
		m_pGame->m_side = ISTRIA;
	else
		m_pGame->m_side = NEUTRAL;

	cp = (char *)cp;
	memcpy(m_pGame->m_cGuildName, cp, 20);
	cp += 20;

	if (strcmp(m_pGame->m_cGuildName, "NONE") == 0)
		ZeroMemory(m_pGame->m_cGuildName, sizeof(m_pGame->m_cGuildName));

	m_pGame->m_Misc.ReplaceString(m_pGame->m_cGuildName, '_', ' ');

	ip   = (int *)cp;
	m_pGame->m_iGuildRank = *ip;
	cp += 4;

	m_pGame->m_iSuperAttackLeft = (int)*cp;
	cp++;

	ip   = (int *)cp;
	m_pGame->m_iFightzoneNumber = *ip;
	cp += 4;
	
}

void CMessageHandler::CivilRightAdmissionHandler(char *Data)
{WORD * wp, wResult;
 char * cp;

	wp   = (WORD *)(Data + INDEX2_MSGTYPE);
	wResult = *wp;

	switch (wResult) {
	case 0:
		m_pGame->m_stDialogBoxInfo[13].cMode = 4;
		break;

	case 1:
		m_pGame->m_stDialogBoxInfo[13].cMode = 3;
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		ZeroMemory(m_pGame->m_cLocation, sizeof(m_pGame->m_cLocation));
		memcpy(m_pGame->m_cLocation, cp, 10);

		if (memcmp(m_pGame->m_cLocation, "are", 3) == 0)
			m_pGame->m_side = ARESDEN;
		else if (memcmp(m_pGame->m_cLocation, "elv", 3) == 0)
			m_pGame->m_side = ELVINE;
		else if (memcmp(m_pGame->m_cLocation, "ist", 3) == 0)
			m_pGame->m_side = ISTRIA;
		else
			m_pGame->m_side = NEUTRAL;
		break;
	}
}

void CMessageHandler::RetrieveItemHandler(char *Data)
{char * cp, cBankItemIndex, cItemIndex, cTxt[120];
 WORD * wp;
 int j;
	wp = (WORD *)(Data + INDEX2_MSGTYPE);
	if (*wp != MSGTYPE_REJECT)
	{	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		cBankItemIndex = *cp;
		cp++;
		cItemIndex = *cp;
		cp++;

		if (cBankItemIndex >= 0 && cBankItemIndex < (int)m_pGame->m_pBankList.size()) {

			char cStr1[64], cStr2[64], cStr3[64];
			m_pGame->GetItemName(m_pGame->m_pBankList[cBankItemIndex], cStr1, cStr2, cStr3);

			ZeroMemory(cTxt, sizeof(cTxt));
			wsprintf(cTxt, RETIEVE_ITEM_HANDLER4, cStr1);//""You took out %s."
			m_pGame->AddEventList(cTxt, 10);

			if ( (m_pGame->m_pBankList[cBankItemIndex]->m_cItemType == ITEMTYPE_CONSUME) ||
				 (m_pGame->m_pBankList[cBankItemIndex]->m_cItemType == ITEMTYPE_ARROW) )
			{	if (m_pGame->m_pItemList[cItemIndex]	== NULL) goto RIH_STEP2;
				delete m_pGame->m_pBankList[cBankItemIndex];
				m_pGame->m_pBankList.erase(m_pGame->m_pBankList.begin() + cBankItemIndex);
			}else
			{
RIH_STEP2:;
				if (m_pGame->m_pItemList[cItemIndex] != NULL) return;
				short nX, nY;
				nX = 40;
				nY = 30;
				for (j = 0; j < MAXITEMS; j++)
				{	if ( ( m_pGame->m_pItemList[j] != NULL) && (memcmp(m_pGame->m_pItemList[j]->m_cName, cStr1, 20) == 0))
					{	nX = m_pGame->m_pItemList[j]->m_sX+1;
						nY = m_pGame->m_pItemList[j]->m_sY+1;
						break;
				}	}
				m_pGame->m_pItemList[cItemIndex] = m_pGame->m_pBankList[cBankItemIndex];
				m_pGame->m_pItemList[cItemIndex]->m_sX =	nX;
				m_pGame->m_pItemList[cItemIndex]->m_sY =	nY;
                m_pGame->bSendCommand(MSGID_REQUEST_SETITEMPOS, NULL, cItemIndex, nX, nY, NULL, NULL);

				for (j = 0; j < MAXITEMS; j++)
				if (m_pGame->m_cItemOrder[j] == -1)
				{	m_pGame->m_cItemOrder[j] = cItemIndex;
					break;
				}
				m_pGame->m_bIsItemEquipped[cItemIndex] = FALSE;
				m_pGame->m_bIsItemDisabled[cItemIndex] = FALSE;
				m_pGame->m_pBankList.erase(m_pGame->m_pBankList.begin() + cBankItemIndex);
	}	}	}
	m_pGame->m_stDialogBoxInfo[14].cMode = 0;
}

void CMessageHandler::ResponsePanningHandler(char *Data)
{
 char * cp, cDir;
 short * sp, sX, sY;

	cp = (char *)(Data + INDEX2_MSGTYPE +2);

	sp = (short *)cp;
	sX = *sp;
	cp += 2;

	sp = (short *)cp;
	sY = *sp;
	cp += 2;

	cDir = *cp;
	cp++;

	switch (cDir) {
	case 1: m_pGame->m_sViewDstY -= 32; m_pGame->m_sPlayerY--; break;
	case 2: m_pGame->m_sViewDstY -= 32; m_pGame->m_sPlayerY--; m_pGame->m_sViewDstX += 32; m_pGame->m_sPlayerX++; break;
	case 3: m_pGame->m_sViewDstX += 32; m_pGame->m_sPlayerX++; break;
	case 4: m_pGame->m_sViewDstY += 32; m_pGame->m_sPlayerY++; m_pGame->m_sViewDstX += 32; m_pGame->m_sPlayerX++; break;
	case 5: m_pGame->m_sViewDstY += 32; m_pGame->m_sPlayerY++;break;
	case 6: m_pGame->m_sViewDstY += 32; m_pGame->m_sPlayerY++; m_pGame->m_sViewDstX -= 32; m_pGame->m_sPlayerX--; break;
	case 7: m_pGame->m_sViewDstX -= 32; m_pGame->m_sPlayerX--; break;
	case 8: m_pGame->m_sViewDstY -= 32; m_pGame->m_sPlayerY--; m_pGame->m_sViewDstX -= 32; m_pGame->m_sPlayerX--; break;
	}

	m_pGame->m_pMapData->ShiftMapData(cDir);
	m_pGame->_ReadMapData(sX, sY, cp);

	m_pGame->m_bIsRedrawPDBGS = TRUE;

	m_pGame->m_bIsObserverCommanded = FALSE;
}

void CMessageHandler::ReserveFightzoneResponseHandler(char * Data)
{
 	WORD * wpResult;
	char * cp ;
	int * ip ;
 	wpResult = (WORD *)(Data + INDEX2_MSGTYPE);
	switch (*wpResult) {
	case MSGTYPE_CONFIRM:
		m_pGame->AddEventList(RESERVE_FIGHTZONE_RESPONSE_HANDLER1, 10);
		m_pGame->m_stDialogBoxInfo[7].cMode = 14;
		m_pGame->m_iFightzoneNumber = m_pGame->m_iFightzoneNumberTemp ;
		break;

	case MSGTYPE_REJECT:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		ip   = (int *)cp;
		cp += 4;
		m_pGame->AddEventList(RESERVE_FIGHTZONE_RESPONSE_HANDLER2, 10);
		m_pGame->m_iFightzoneNumberTemp = 0 ;

		if (*ip == 0) {
		 	m_pGame->m_stDialogBoxInfo[7].cMode = 15;
		}else if (*ip == -1){
			m_pGame->m_stDialogBoxInfo[7].cMode = 16;
		} else if (*ip == -2) {
			m_pGame->m_stDialogBoxInfo[7].cMode = 17;
		}else if (*ip == -3) {
			m_pGame->m_stDialogBoxInfo[7].cMode = 21;
		}else if (*ip == -4) {
			m_pGame->m_stDialogBoxInfo[7].cMode = 22;
		}
		break;
	}
}

void CMessageHandler::NoticementHandler(char * Data)
{
 char * cp;
 FILE * pFile;
 WORD * wp;
	wp = (WORD *)(Data + INDEX2_MSGTYPE);
	switch (*wp) {
	case MSGTYPE_CONFIRM:
		break;
	case MSGTYPE_REJECT:
		cp = (char *)(Data + INDEX2_MSGTYPE + 2);
		pFile = fopen("contents\\contents1000.txt", "wt");
		if (pFile == NULL) return;
		fwrite(cp, strlen(cp), 1, pFile);
		fclose(pFile);
		m_pGame->m_stDialogBoxInfo[18].sX  =  20;
		m_pGame->m_stDialogBoxInfo[18].sY  =  65;
		m_pGame->EnableDialogBox(18, 1000, NULL, NULL);
		break;
	}
	m_pGame->AddEventList(MSG_NOTIFY_HELP, 10);
	if (m_pGame->m_iLevel < 42) m_pGame->EnableDialogBox(35, NULL, NULL, NULL);

}

void CMessageHandler::DynamicObjectHandler(char * Data)
{
 WORD * wp;
 char * cp;
 short * sp, sX, sY, sV1, sV2, sV3;

	cp = (char *)(Data + INDEX2_MSGTYPE);
	wp = (WORD *)cp;
	cp += 2;

	sp = (short *)cp;
	sX = *sp;
	cp += 2;

	sp = (short *)cp;
	sY = *sp;
	cp += 2;

	sp = (short *)cp;
	sV1 = *sp;
	cp += 2;

	sp = (short *)cp;
	sV2 = *sp;		   // Dyamic Object Index
	cp += 2;

	sp = (short *)cp;
	sV3 = *sp;
	cp += 2;

	switch (*wp) {
	case MSGTYPE_CONFIRM:// Dynamic Object
		m_pGame->m_pMapData->bSetDynamicObject(sX, sY, sV2, sV1, TRUE);
		break;

	case MSGTYPE_REJECT:// Dynamic object
		m_pGame->m_pMapData->bSetDynamicObject(sX, sY, sV2, NULL, TRUE);
		break;
	}
}

void CMessageHandler::ResponseTeleportList(char *Data)
{	char *cp;
	int  *ip, i;
#ifdef _DEBUG
	m_pGame->AddEventList("Teleport ???", 10);
#endif
	cp = Data + 6;
	ip = (int*) cp;
	m_pGame->m_iTeleportMapCount = *ip;
	cp += 4;
	for ( i = 0 ; i < m_pGame->m_iTeleportMapCount ; i++)
	{	ip = (int*)cp;
		m_pGame->m_stTeleportList[i].iIndex = *ip;
		cp += 4;
		ZeroMemory(m_pGame->m_stTeleportList[i].mapname, sizeof(m_pGame->m_stTeleportList[i].mapname) );
		memcpy(m_pGame->m_stTeleportList[i].mapname, cp, 10);
		cp += 10;
		ip = (int*)cp;
		m_pGame->m_stTeleportList[i].iX = *ip;
		cp += 4;
		ip = (int*)cp;
		m_pGame->m_stTeleportList[i].iY = *ip;
		cp += 4;
		ip = (int*)cp;
		m_pGame->m_stTeleportList[i].iCost = *ip;
		cp += 4;
	}
}

void CMessageHandler::ResponseChargedTeleport(char *Data)
{	short *sp;
	char *cp;
	short sRejectReason = 0;
	cp = (char*)Data + INDEX2_MSGTYPE + 2;
	sp = (short*)cp;
	sRejectReason = *sp;

#ifdef _DEBUG
	m_pGame->AddEventList( "charged teleport ?", 10 );
#endif

	switch( sRejectReason )	{
	case 1:
		m_pGame->AddEventList( RESPONSE_CHARGED_TELEPORT1, 10 );
		break;
	case 2:
		m_pGame->AddEventList( RESPONSE_CHARGED_TELEPORT2, 10 );
		break;
	case 3:
		m_pGame->AddEventList( RESPONSE_CHARGED_TELEPORT3, 10 );
		break;
	case 4:
		m_pGame->AddEventList( RESPONSE_CHARGED_TELEPORT4, 10 );
		break;
	case 5:
		m_pGame->AddEventList( RESPONSE_CHARGED_TELEPORT5, 10 );
		break;
	case 6:
		m_pGame->AddEventList( RESPONSE_CHARGED_TELEPORT6, 10 );
		break;
	default:
		m_pGame->AddEventList( RESPONSE_CHARGED_TELEPORT7, 10 );
	}
}

void CMessageHandler::NpcTalkHandler(char *Data)
{
 char  * cp, cRewardName[21], cTargetName[21], cTemp[21], cTxt[250];
 short * sp, sType, sResponse;
 int     iAmount, iIndex, iContribution, iX, iY, iRange;
 int     iTargetType, iTargetCount, iQuestionType;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	sp = (short *)cp;
	sType = *sp;
	cp += 2;
	sp = (short *)cp;
	sResponse = *sp;
	cp += 2;
	sp = (short *)cp;
	iAmount = *sp;
	cp += 2;
	sp = (short *)cp;
	iContribution = *sp;
	cp += 2;
	sp = (short *)cp;
	iTargetType = *sp;
	cp += 2;
	sp = (short *)cp;
	iTargetCount = *sp;
	cp += 2;
	sp = (short *)cp;
	iX = *sp;
	cp += 2;
	sp = (short *)cp;
	iY = *sp;
	cp += 2;
	sp = (short *)cp;
	iRange = *sp;
	cp += 2;
	ZeroMemory(cRewardName, sizeof(cRewardName));
	memcpy(cRewardName, cp, 20);
	cp += 20;
	ZeroMemory(cTargetName, sizeof(cTargetName));
	memcpy(cTargetName, cp, 20);
	cp += 20;
	m_pGame->EnableDialogBox(21, sResponse, sType, 0);

	if ((sType >= 1) && (sType <= 100))
	{	iIndex = m_pGame->m_stDialogBoxInfo[21].sV1;
		m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, "  ", NULL);
		iIndex++;
		iQuestionType = NULL;
		switch (sType) {
		case 1: //Monster Hunt
			ZeroMemory(cTemp, sizeof(cTemp));
			m_pGame->GetNpcName(iTargetType, cTemp);
			ZeroMemory(cTxt, sizeof(cTxt));
			wsprintf(cTxt, NPC_TALK_HANDLER16, iTargetCount, cTemp);
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
			iIndex++;

			ZeroMemory(cTxt, sizeof(cTxt));
			if (memcmp(cTargetName, "NONE", 4) == 0) {
				strcpy(cTxt, NPC_TALK_HANDLER17);//"
				m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
				iIndex++;
			}
			else {
				ZeroMemory(cTemp, sizeof(cTemp));
				m_pGame->GetOfficialMapName(cTargetName, cTemp);
				wsprintf(cTxt, NPC_TALK_HANDLER18, cTemp);//"Map : %s"
				m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
				iIndex++;

				if (iX != 0) {
					ZeroMemory(cTxt, sizeof(cTxt));
					wsprintf(cTxt, NPC_TALK_HANDLER19, iX, iY, iRange);//"Position: %d,%d within %d blocks"
					m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
					iIndex++;
				}

				ZeroMemory(cTxt, sizeof(cTxt));
				wsprintf(cTxt, NPC_TALK_HANDLER20, iContribution);//"
				m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
				iIndex++;
			}
			iQuestionType = 1;
			break;

		case 7: //
			ZeroMemory(cTxt, sizeof(cTxt));
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, NPC_TALK_HANDLER21, NULL);
			iIndex++;

			ZeroMemory(cTxt, sizeof(cTxt));
			if (memcmp(cTargetName, "NONE", 4) == 0) {
				strcpy(cTxt, NPC_TALK_HANDLER22);
				m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
				iIndex++;
			}
			else {
				ZeroMemory(cTemp, sizeof(cTemp));
				m_pGame->GetOfficialMapName(cTargetName, cTemp);
				wsprintf(cTxt, NPC_TALK_HANDLER23, cTemp);
				m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
				iIndex++;

				if (iX != 0) {
					ZeroMemory(cTxt, sizeof(cTxt));
					wsprintf(cTxt, NPC_TALK_HANDLER24, iX, iY, iRange);
					m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
					iIndex++;
				}

				ZeroMemory(cTxt, sizeof(cTxt));
				wsprintf(cTxt, NPC_TALK_HANDLER25, iContribution);
				m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
				iIndex++;
			}
			iQuestionType = 1;
			break;

		case 10: // Crusade
			ZeroMemory(cTxt, sizeof(cTxt));
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, NPC_TALK_HANDLER26, NULL);
			iIndex++;

			ZeroMemory(cTxt, sizeof(cTxt));
            strcpy(cTxt, NPC_TALK_HANDLER27);//"
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
			iIndex++;

			ZeroMemory(cTxt, sizeof(cTxt));
            strcpy(cTxt, NPC_TALK_HANDLER28);//"
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
			iIndex++;

			ZeroMemory(cTxt, sizeof(cTxt));
            strcpy(cTxt, NPC_TALK_HANDLER29);//"
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
			iIndex++;

			ZeroMemory(cTxt, sizeof(cTxt));
            strcpy(cTxt, NPC_TALK_HANDLER30);//"
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
			iIndex++;

			ZeroMemory(cTxt, sizeof(cTxt));
			strcpy(cTxt, " ");
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
			iIndex++;

			ZeroMemory(cTxt, sizeof(cTxt));
			if (memcmp(cTargetName, "NONE", 4) == 0) {
				strcpy(cTxt, NPC_TALK_HANDLER31);//"
				m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
				iIndex++;
			}
			else {
				ZeroMemory(cTemp, sizeof(cTemp));
				m_pGame->GetOfficialMapName(cTargetName, cTemp);
				wsprintf(cTxt, NPC_TALK_HANDLER32, cTemp);//"
				m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
				iIndex++;
			}
			iQuestionType = 2;
			break;
		}

		switch (iQuestionType) {
		case 1:
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, "  ", NULL);
			iIndex++;
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, NPC_TALK_HANDLER33, NULL);//"
			iIndex++;
			m_pGame->m_pMsgTextList2[iIndex]  = new class CMsg(NULL, NPC_TALK_HANDLER34, NULL);//"
			iIndex++;
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, "  ", NULL);
			iIndex++;
			break;

		case 2:
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, "  ", NULL);
			iIndex++;
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, NPC_TALK_HANDLER35, NULL);//"
			iIndex++;
			m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, "  ", NULL);
			iIndex++;
			break;

		default: break;
		}
	}
}

void CMessageHandler::Notify_PartyInfo(char * Data)
{	
	char * cp;
	int  * ip;
	int  sV1, sV2, sV3, sV4, sV5, sV6;
	int i;
	char cTxt[120];

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	ZeroMemory(cTxt, sizeof(cTxt));
	ip   = (int *)cp;
	memcpy(cTxt, ip, 10);
	cp += 10;
	ip   = (int *)cp;
	sV1 = *ip;
	cp  += 4;
	ip   = (int *)cp;
	sV2 = *ip;
	cp  += 4;
	ip   = (int *)cp;
	sV3 = *ip;
	cp  += 4;
	ip   = (int *)cp;
	sV4 = *ip;
	cp  += 4;
	ip   = (int *)cp;
	sV5 = *ip;
	cp  += 4;
	ip   = (int *)cp;
	sV6 = *ip;
	cp  += 4;
	for (i = 0; i < m_pGame->m_stPartyMember.size(); i++){
		if(m_pGame->m_stPartyMember[i]->cName.compare(cTxt) ==0){
			m_pGame->m_stPartyMember[i]->sX = sV1;
			m_pGame->m_stPartyMember[i]->sY = sV2;
			m_pGame->m_stPartyMember[i]->hp = sV3;
			m_pGame->m_stPartyMember[i]->mp = sV4;
			m_pGame->m_stPartyMember[i]->Maxhp = sV5;
			m_pGame->m_stPartyMember[i]->Maxmp = sV6;
		}
	}
}

void CMessageHandler::NotifyMsg_Heldenian(char * Data)
{	
	WORD *wp;
	wp = (WORD *)(Data + INDEX2_MSGTYPE + 2);
	m_pGame->m_iHeldenianAresdenLeftTower = *wp;
	wp++;
	m_pGame->m_iHeldenianAresdenFlags = *wp;
	wp++;
	m_pGame->m_iHeldenianAresdenKill = *wp;
	wp++;
	m_pGame->m_iHeldenianAresdenDead = *wp;
	wp++;
	m_pGame->m_iHeldenianElvineLeftTower = *wp;
	wp++;
	m_pGame->m_iHeldenianElvineFlags = *wp;
	wp++;
	m_pGame->m_iHeldenianElvineKill = *wp;
	wp++;
	m_pGame->m_iHeldenianElvineDead = *wp;
}

void CMessageHandler::NotifyMsg_GlobalAttackMode(char *Data)
{
 char * cp;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	switch (*cp) {
	case 0:
		m_pGame->AddEventList(NOTIFYMSG_GLOBAL_ATTACK_MODE1, 10);
		m_pGame->AddEventList(NOTIFYMSG_GLOBAL_ATTACK_MODE2, 10);
		break;

	case 1:
		m_pGame->AddEventList(NOTIFYMSG_GLOBAL_ATTACK_MODE3, 10);
		break;
	}
	cp++;
}

void CMessageHandler::NotifyMsg_QuestReward(char *Data)
{short * sp, sWho, sFlag;
 char  * cp, cRewardName[21], cTxt[120];
 int   * ip, iAmount, iIndex, iPreCon;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	sp = (short *)cp;
	sWho = *sp;
	cp += 2;
	sp = (short *)cp;
	sFlag = *sp;
	cp += 2;
	ip = (int *)cp;
	iAmount = *ip;
	cp += 4;
	ZeroMemory(cRewardName, sizeof(cRewardName));
	memcpy(cRewardName, cp, 20);
	cp += 20;
	iPreCon = m_pGame->m_iContribution;
	ip = (int *)cp;
	m_pGame->m_iContribution = *ip;
//	m_iContributionPrice = 0;
	cp += 4;

	if (sFlag == 1)
	{	m_pGame->m_stQuest.sWho          = NULL;
		m_pGame->m_stQuest.sQuestType    = NULL;
		m_pGame->m_stQuest.sContribution = NULL;
		m_pGame->m_stQuest.sTargetType   = NULL;
		m_pGame->m_stQuest.sTargetCount  = NULL;
		m_pGame->m_stQuest.sX     = NULL;
		m_pGame->m_stQuest.sY     = NULL;
		m_pGame->m_stQuest.sRange = NULL;
		m_pGame->m_stQuest.sCurrentCount = NULL;
		m_pGame->m_stQuest.bIsQuestCompleted = FALSE;
		ZeroMemory(m_pGame->m_stQuest.cTargetName, sizeof(m_pGame->m_stQuest.cTargetName));
		m_pGame->EnableDialogBox(21, 0, sWho+110, 0);
		iIndex = m_pGame->m_stDialogBoxInfo[21].sV1;
		m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, "  ", NULL);
		iIndex++;
		ZeroMemory(cTxt, sizeof(cTxt));
		if (memcmp(cRewardName, "����ġ", 6) == 0)
		{	if (iAmount > 0) wsprintf(cTxt, NOTIFYMSG_QUEST_REWARD1, iAmount);
		}else
		{	wsprintf(cTxt, NOTIFYMSG_QUEST_REWARD2, iAmount, cRewardName);
		}
		m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, cTxt, NULL);
		iIndex++;
		m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, "  ", NULL);
		iIndex++;
		ZeroMemory(cTxt, sizeof(cTxt));
		if (iPreCon < m_pGame->m_iContribution)
			 wsprintf(cTxt, NOTIFYMSG_QUEST_REWARD3, m_pGame->m_iContribution - iPreCon);
		else wsprintf(cTxt, NOTIFYMSG_QUEST_REWARD4, iPreCon - m_pGame->m_iContribution);

		m_pGame->m_pMsgTextList2[iIndex] = new class CMsg(NULL, "  ", NULL);
		iIndex++;
	}
	else m_pGame->EnableDialogBox(21, 0, sWho+120, 0);
}

void CMessageHandler::NotifyMsg_QuestContents(char *Data)
{short * sp;
 char  * cp;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	sp = (short *)cp;
	m_pGame->m_stQuest.sWho = *sp;
	cp += 2;
	sp = (short *)cp;
	m_pGame->m_stQuest.sQuestType = *sp;
	cp += 2;
	sp = (short *)cp;
	m_pGame->m_stQuest.sContribution = *sp;
	cp += 2;
	sp = (short *)cp;
	m_pGame->m_stQuest.sTargetType = *sp;
	cp += 2;
	sp = (short *)cp;
	m_pGame->m_stQuest.sTargetCount = *sp;
	cp += 2;
	sp = (short *)cp;
	m_pGame->m_stQuest.sX = *sp;
	cp += 2;
	sp = (short *)cp;
	m_pGame->m_stQuest.sY = *sp;
	cp += 2;
	sp = (short *)cp;
	m_pGame->m_stQuest.sRange = *sp;
	cp += 2;
	sp = (short *)cp;
	m_pGame->m_stQuest.bIsQuestCompleted = (BOOL)*sp;
	cp += 2;
	ZeroMemory(m_pGame->m_stQuest.cTargetName, sizeof(m_pGame->m_stQuest.cTargetName));
	memcpy(m_pGame->m_stQuest.cTargetName, cp, 20);
	cp += 20;
	
	//AddEventList(NOTIFYMSG_QUEST_STARTED, 10);
}

void CMessageHandler::NotifyMsg_ItemColorChange(char *Data)
{
 short * sp, sItemIndex, sItemColor;
 char * cp;
 char cTxt[120];

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	sp = (short *)cp;
	sItemIndex = *sp;
	cp += 2;

	sp = (short *)cp;
	sItemColor = (short)*sp;
	cp += 2;

	if (m_pGame->m_pItemList[sItemIndex] != NULL) {
		char cStr1[64], cStr2[64], cStr3[64];
		m_pGame->GetItemName( m_pGame->m_pItemList[sItemIndex], cStr1, cStr2, cStr3 );
		if (sItemColor != -1) {
			m_pGame->m_pItemList[sItemIndex]->m_cItemColor = (char)sItemColor;
			wsprintf(cTxt, NOTIFYMSG_ITEMCOLOR_CHANGE1, cStr1);
			m_pGame->AddEventList(cTxt, 10);
		}
		else {
			wsprintf(cTxt, NOTIFYMSG_ITEMCOLOR_CHANGE2, cStr1);
			m_pGame->AddEventList(cTxt, 10);
		}
	}
}

void CMessageHandler::NotifyMsg_DropItemFin_CountChanged(char *Data)
{
 char * cp, cTxt[256];
 WORD * wp, wItemIndex;
 int  * ip, iAmount;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	wItemIndex = *wp;
	cp += 2;

	ip = (int *)cp;
	iAmount = *ip;
	cp += 4;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(m_pGame->m_pItemList[wItemIndex]->m_cName, m_pGame->m_pItemList[wItemIndex]->m_dwAttribute, cStr1, cStr2, cStr3);
	wsprintf(cTxt, NOTIFYMSG_THROW_ITEM1, iAmount, cStr1);

	m_pGame->AddEventList(cTxt, 10);
}

void CMessageHandler::NotifyMsg_CannotGiveItem(char *Data)
{
 char * cp, cName[21], cTxt[256];
 WORD * wp, wItemIndex;
 int  * ip, iAmount;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	wItemIndex = *wp;
	cp += 2;

	ip = (int *)cp;
	iAmount = *ip;
	cp += 4;

	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(m_pGame->m_pItemList[wItemIndex], cStr1, cStr2, cStr3);
	if( iAmount == 1 ) wsprintf(cTxt, NOTIFYMSG_CANNOT_GIVE_ITEM2, cStr1, cName);
	else wsprintf( cTxt, NOTIFYMSG_CANNOT_GIVE_ITEM1, iAmount, cStr1, cName);


	m_pGame->AddEventList(cTxt, 10);
}

void CMessageHandler::NotifyMsg_GiveItemFin_CountChanged(char *Data)
{
 char * cp, cName[21], cTxt[256];
 WORD * wp, wItemIndex;
 int  * ip, iAmount;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	wItemIndex = *wp;
	cp += 2;

	ip = (int *)cp;
	iAmount = *ip;
	cp += 4;

	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(m_pGame->m_pItemList[wItemIndex]->m_cName, m_pGame->m_pItemList[wItemIndex]->m_dwAttribute, cStr1, cStr2, cStr3);
	if( iAmount == 1 ) wsprintf(cTxt, NOTIFYMSG_GIVEITEMFIN_COUNTCHANGED1, cStr1, cName);
	wsprintf(cTxt, NOTIFYMSG_GIVEITEMFIN_COUNTCHANGED2, iAmount, cStr1, cName);
	m_pGame->AddEventList(cTxt, 10);
}

void CMessageHandler::NotifyMsg_SetExchangeItem(char *Data)
{short * sp, sDir, sSprite, sSpriteFrame, sCurLife, sMaxLife, sPerformance;
 int * ip, iAmount, i;
 char * cp, cColor, cItemName[24], cCharName[12];
 DWORD * dwp, dwAttribute;
	ZeroMemory(cItemName, sizeof(cItemName));
	ZeroMemory(cCharName, sizeof(cCharName));

	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	sp = (short *)cp;
	sDir = *sp;
	cp += 2;
	sp = (short *)cp;
	sSprite = *sp;
	cp += 2;
	sp = (short *)cp;
	sSpriteFrame = *sp;
	cp += 2;
	ip = (int *)cp;
	iAmount = *ip;
	cp += 4;
	cColor = *cp;
	cp++;
	sp = (short *)cp;
	sCurLife = *sp;
	cp += 2;
	sp = (short *)cp;
	sMaxLife = *sp;
	cp += 2;
	sp = (short *)cp;
	sPerformance = *sp;
	cp += 2;
	memcpy(cItemName, cp, 20);
	cp += 20;
	memcpy(cCharName, cp, 10);
	cp += 10;
	dwp = (DWORD *)cp;
	dwAttribute = *dwp;
	cp += 4;

	if (sDir >= 1000)  // Set the item I want to exchange
	{	i = 0;
		while (m_pGame->m_stDialogBoxExchangeInfo[i].sV1 !=-1)
		{	i++;
			if (i>=4) return; // Error situation
		}
	}else // Set the item he proposes me.
	{	i = 4;
		while (m_pGame->m_stDialogBoxExchangeInfo[i].sV1 !=-1)
		{	i++;
			if (i>=8) return; // Error situation
	}	}
	m_pGame->m_stDialogBoxExchangeInfo[i].sV1 = sSprite;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV2 = sSpriteFrame;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV3 = iAmount;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV4 = cColor;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV5 = (int)sCurLife;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV6 = (int)sMaxLife;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV7 = (int)sPerformance;
	memcpy(m_pGame->m_stDialogBoxExchangeInfo[i].cStr1, cItemName, 20);
	memcpy(m_pGame->m_stDialogBoxExchangeInfo[i].cStr2, cCharName, 10);
	m_pGame->m_stDialogBoxExchangeInfo[i].dwV1 = dwAttribute;
	//if (i<4) m_stDialogBoxExchangeInfo[i].sItemID = sDir -1000;
}

void CMessageHandler::NotifyMsg_OpenExchageWindow(char *Data)
{short * sp, sDir, sSprite, sSpriteFrame, sCurLife, sMaxLife, sPerformance;
 int * ip, iAmount;
 char * cp, cColor, cItemName[24], cCharName[12];
 DWORD * dwp, dwAttribute;
	ZeroMemory(cItemName, sizeof(cItemName));
	ZeroMemory(cCharName, sizeof(cCharName));

	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	sp = (short *)cp;
	sDir = *sp;
	cp += 2;
	sp = (short *)cp;
	sSprite = *sp;
	cp += 2;
	sp = (short *)cp;
	sSpriteFrame = *sp;
	cp += 2;
	ip = (int *)cp;
	iAmount = *ip;
	cp += 4;
	cColor = *cp;
	cp++;
	sp = (short *)cp;
	sCurLife = *sp;
	cp += 2;
	sp = (short *)cp;
	sMaxLife = *sp;
	cp += 2;
	sp = (short *)cp;
	sPerformance = *sp;
	cp += 2;
	memcpy(cItemName, cp, 20);
	cp += 20;
	memcpy(cCharName, cp, 10);
	cp += 10;
	dwp = (DWORD *)cp;
	dwAttribute = *dwp;
	cp += 4;

	m_pGame->EnableDialogBox(27, 1, 0, 0, NULL);
	int i;
	if (sDir >= 1000)  // Set the item I want to exchange
	{	i = 0;
		while (m_pGame->m_stDialogBoxExchangeInfo[i].sV1 !=-1)
		{	i++;
			if (i>=4) return; // Error situation
		}
		if ((sDir >1000) && (i == 0))
		{	m_pGame->m_bIsItemDisabled[sDir -1000] = TRUE;
			m_pGame->m_stDialogBoxExchangeInfo[0].sItemID = sDir -1000;
		}
	}else // Set the item he proposes me.
	{	i = 4;
		while (m_pGame->m_stDialogBoxExchangeInfo[i].sV1 !=-1)
		{	i++;
			if (i>=8) return; // Error situation
	}	}
	m_pGame->m_stDialogBoxExchangeInfo[i].sV1 = sSprite;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV2 = sSpriteFrame;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV3 = iAmount;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV4 = cColor;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV5 = (int)sCurLife;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV6 = (int)sMaxLife;
	m_pGame->m_stDialogBoxExchangeInfo[i].sV7 = (int)sPerformance;
	memcpy(m_pGame->m_stDialogBoxExchangeInfo[i].cStr1, cItemName, 20);
	memcpy(m_pGame->m_stDialogBoxExchangeInfo[i].cStr2, cCharName, 10);
	m_pGame->m_stDialogBoxExchangeInfo[i].dwV1 = dwAttribute;
}

void CMessageHandler::NotifyMsg_DownSkillIndexSet(char *Data)
{
 WORD * wp;
 short sSkillIndex;
 char * cp;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	sSkillIndex = (short)*wp;
	cp += 2;
	m_pGame->m_iDownSkillIndex = sSkillIndex;
	m_pGame->m_stDialogBoxInfo[15].bFlag = FALSE;
}

void CMessageHandler::NotifyMsg_AdminInfo(char *Data)
{
	char * cp, cStr[256];
	int  * ip, iV1, iV2, iV3, iV4, iV5;

	cp = (char *)(Data + 6);

	ip = (int *)cp;
	iV1 = *ip;
	cp += 4;

	ip = (int *)cp;
	iV2 = *ip;
	cp += 4;

	ip = (int *)cp;
	iV3 = *ip;
	cp += 4;

	ip = (int *)cp;
	iV4 = *ip;
	cp += 4;

	ip = (int *)cp;
	iV5 = *ip;
	cp += 4;

	ZeroMemory(cStr, sizeof(cStr));
	wsprintf(cStr, "%d %d %d %d %d", iV1, iV2, iV3, iV4, iV5);
	m_pGame->AddEventList(cStr);
}

void CMessageHandler::NotifyMsg_WhetherChange(char * Data)
{
 char * cp;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	m_pGame->m_cWhetherStatus = *cp;
	cp++;

	if (m_pGame->m_cWhetherStatus != NULL)
		 m_pGame->SetWhetherStatus(TRUE,  m_pGame->m_cWhetherStatus);
	else m_pGame->SetWhetherStatus(FALSE, NULL);
}

void CMessageHandler::NotifyMsg_FishChance(char * Data)
{
 int iFishChance;
 char * cp;
 WORD * wp;
	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	iFishChance = (int)*wp;
	cp += 2;
	m_pGame->m_stDialogBoxInfo[24].sV1 = iFishChance;
}

void CMessageHandler::NotifyMsg_EventFishMode(char * Data)
{
	short sSprite, sSpriteFrame;
	char * cp, cName[21];
	WORD * wp, wPrice;
	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);

	wp = (WORD *)cp;
	wPrice = *wp;
	cp += 2;

	wp = (WORD *)cp;
	sSprite = (short)*wp;
	cp += 2;

	wp = (WORD *)cp;
	sSpriteFrame = (short)*wp;
	cp += 2;

	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;

	m_pGame->EnableDialogBox(24, 0, NULL, wPrice, cName);
	m_pGame->m_stDialogBoxInfo[24].sV3 = sSprite;
	m_pGame->m_stDialogBoxInfo[24].sV4 = sSpriteFrame;

	m_pGame->AddEventList(NOTIFYMSG_EVENTFISHMODE1, 10);
}

void CMessageHandler::NotifyMsg_NoticeMsg(char * Data)
{char * cp, cMsg[1000];
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	strcpy(cMsg, cp);
	m_pGame->AddEventList(cMsg, 10);
}

void CMessageHandler::NotifyMsg_RatingPlayer(char * Data)
{//int * ip;
 char * cp, cName[12];
 WORD  cValue;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	cValue = *cp;
	cp++;
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 10);
	cp += 10;
//	ip = (int *)cp;
//	m_iRating = *ip;
	cp += 4;
	ZeroMemory(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt));
	if (memcmp(m_pGame->m_cPlayerName, cName, 10) == 0)
	{	if (cValue == 1)
		{	 strcpy(m_pGame->G_cTxt, NOTIFYMSG_RATING_PLAYER1);
			 m_pGame->PlaySound('E', 23, 0);
 		}
	}else
	{	if (cValue == 1)
			 wsprintf(m_pGame->G_cTxt, NOTIFYMSG_RATING_PLAYER2, cName);
		else wsprintf(m_pGame->G_cTxt, NOTIFYMSG_RATING_PLAYER3, cName);
	}
	m_pGame->AddEventList(m_pGame->G_cTxt, 10);
}

void CMessageHandler::NotifyMsg_CannotRating(char * Data)
{
 char * cp, cTxt[120];
 WORD * wp, wTime;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	wTime = *wp;
	cp += 2;

	if (wTime == 0) wsprintf(cTxt, NOTIFYMSG_CANNOT_RATING1, wTime*3);
	else wsprintf(cTxt, NOTIFYMSG_CANNOT_RATING2, wTime*3);
	m_pGame->AddEventList(cTxt, 10);
}

void CMessageHandler::NotifyMsg_PlayerShutUp(char * Data)
{char * cp, cName[12];
 WORD * wp, wTime;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	wTime = *wp;
	cp += 2;
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 10);
	cp += 10;
	if (memcmp(m_pGame->m_cPlayerName, cName, 10) == 0)
		 wsprintf(m_pGame->G_cTxt, NOTIFYMSG_PLAYER_SHUTUP1, wTime);
	else wsprintf(m_pGame->G_cTxt, NOTIFYMSG_PLAYER_SHUTUP2, cName, wTime);

	m_pGame->AddEventList(m_pGame->G_cTxt, 10);
}

void CMessageHandler::NotifyMsg_TimeChange(char * Data)
{ char * cp;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	G_cSpriteAlphaDegree = *cp;
	switch (G_cSpriteAlphaDegree) {
	case 1:	m_pGame->m_bIsXmas = FALSE; m_pGame->PlaySound('E', 32, 0); break;
	case 2: m_pGame->m_bIsXmas = FALSE; m_pGame->PlaySound('E', 31, 0); break;
	case 3: // Snoopy Special night with chrismas bulbs
		if (m_pGame->m_cWhetherEffectType >3) m_pGame->m_bIsXmas = TRUE;
		else m_pGame->m_bIsXmas = FALSE;
		m_pGame->PlaySound('E', 31, 0);
		G_cSpriteAlphaDegree = 2;break;
	}
	m_pGame->m_cGameModeCount = 1;
	m_pGame->m_bIsRedrawPDBGS = TRUE;
}

void CMessageHandler::NotifyMsg_Hunger(char * Data)
{
 char * cp, cHLv;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	cHLv = *cp;

	if ((cHLv <= 40) && (cHLv > 30)) m_pGame->AddEventList(NOTIFYMSG_HUNGER1, 10);//"
	if ((cHLv <= 25) && (cHLv > 20)) m_pGame->AddEventList(NOTIFYMSG_HUNGER2, 10);//"
	if ((cHLv <= 20) && (cHLv > 15)) m_pGame->AddEventList(NOTIFYMSG_HUNGER3, 10);//"
	if ((cHLv <= 15) && (cHLv > 10)) m_pGame->AddEventList(NOTIFYMSG_HUNGER4, 10);//"
	if ((cHLv <= 10) && (cHLv >= 0)) m_pGame->AddEventList(NOTIFYMSG_HUNGER5, 10);//"
}

void CMessageHandler::NotifyMsg_PlayerProfile(char * Data)
{char * cp;
 char cTemp[500];
 int i;
	ZeroMemory(cTemp, sizeof(cTemp));
	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	strcpy(cTemp, cp);
	for (i = 0; i < 500; i++)
	if (cTemp[i] == '_') cTemp[i] = ' ';
	m_pGame->AddEventList(cTemp, 10);
}

void CMessageHandler::NotifyMsg_WhisperMode(BOOL bActive, char * Data)
{
	char * cp;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	ZeroMemory(m_pGame->m_cWhisperName, sizeof(m_pGame->m_cWhisperName));
	memcpy(m_pGame->m_cWhisperName, cp, 10);
	if (bActive == TRUE)
	{
		wsprintf(m_pGame->G_cTxt, NOTIFYMSG_WHISPERMODE1, m_pGame->m_cWhisperName);
		if (m_pGame->m_pWhisperMsg[MAXWHISPERMSG - 1] != NULL) {
			delete m_pGame->m_pWhisperMsg[MAXWHISPERMSG - 1];
			m_pGame->m_pWhisperMsg[MAXWHISPERMSG - 1] = NULL;
		}
		for (int i = MAXWHISPERMSG - 2; i >= 0; i--) {
			m_pGame->m_pWhisperMsg[i+1] = m_pGame->m_pWhisperMsg[i];
			m_pGame->m_pWhisperMsg[i] = NULL;
		}
		m_pGame->m_pWhisperMsg[0] = new class CMsg(NULL, m_pGame->m_cWhisperName, NULL);
		m_pGame->m_cWhisperIndex = 0;
	}
	else wsprintf(m_pGame->G_cTxt, NOTIFYMSG_WHISPERMODE2, m_pGame->m_cWhisperName);

	m_pGame->AddEventList(m_pGame->G_cTxt, 10);
}

void CMessageHandler::NotifyMsg_PlayerStatus(BOOL bOnGame, char * Data)
{char cName[12], cMapName[12], * cp;
 WORD * wp ;
 WORD  dx= 1 ,dy = 1;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 10);
	cp += 10;
	ZeroMemory(cMapName, sizeof(cMapName));
	memcpy(cMapName, cp, 10);
	cp += 10;
	wp = (WORD * ) cp ;
	dx = (WORD ) *wp ;
	cp += 2 ;
	wp = (WORD * ) cp ;
	dy = (WORD ) *wp ;
	cp += 2 ;
	ZeroMemory(m_pGame->G_cTxt, sizeof(m_pGame->G_cTxt));
	if (bOnGame == TRUE) {
		if (strlen(cMapName) == 0)
			 wsprintf(m_pGame->G_cTxt, NOTIFYMSG_PLAYER_STATUS1, cName);
		else wsprintf(m_pGame->G_cTxt, NOTIFYMSG_PLAYER_STATUS2, cName, cMapName, dx, dy);
	}
	else wsprintf(m_pGame->G_cTxt, NOTIFYMSG_PLAYER_STATUS3, cName);
	m_pGame->AddEventList(m_pGame->G_cTxt, 10);
}

void CMessageHandler::NotifyMsg_Range(char * Data)
{
 DWORD * dwp;
 int  iPrevChar;
 char cTxt[120];

	iPrevChar = m_pGame->m_stat[STAT_CHR];
	dwp = (DWORD *)(Data + INDEX2_MSGTYPE + 2);
	m_pGame->m_stat[STAT_CHR] = (int)*dwp;

	if (m_pGame->m_stat[STAT_CHR] > iPrevChar)
	{	wsprintf(cTxt, NOTIFYMSG_RANGE_UP, m_pGame->m_stat[STAT_CHR] - iPrevChar);//"
		m_pGame->AddEventList(cTxt, 10);
		m_pGame->PlaySound('E', 21, 0);
	}else
	{	wsprintf(cTxt, NOTIFYMSG_RANGE_DOWN, iPrevChar - m_pGame->m_stat[STAT_CHR]);//"
		m_pGame->AddEventList(cTxt, 10);
	}
}

void CMessageHandler::NotifyMsg_ItemRepaired(char * Data)
{
 char * cp, cTxt[120];
 DWORD * dwp, dwItemID, dwLife;

	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);

	dwp = (DWORD *)cp;
	dwItemID = *dwp;
	cp += 4;

	dwp = (DWORD *)cp;
	dwLife = *dwp;
	cp += 4;

	m_pGame->m_pItemList[dwItemID]->m_wCurLifeSpan = (WORD)dwLife;
	m_pGame->m_bIsItemDisabled[dwItemID] = FALSE;
	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName( m_pGame->m_pItemList[dwItemID], cStr1, cStr2, cStr3 );

	wsprintf(cTxt, NOTIFYMSG_ITEMREPAIRED1, cStr1);

	m_pGame->AddEventList(cTxt, 10);
}

void CMessageHandler::NotifyMsg_RepairItemPrice(char * Data)
{char * cp, cName[21];
 DWORD * dwp, wV1, wV2, wV3, wV4;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	dwp = (DWORD *)cp;
  	wV1 = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
  	wV2 = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
  	wV3 = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
  	wV4 = *dwp;
	cp += 4;
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;
	m_pGame->EnableDialogBox(23, 2, wV1, wV2);
	m_pGame->m_stDialogBoxInfo[23].sV3 = wV3;
}

void CMessageHandler::NotifyMsg_CannotRepairItem(char * Data)
{
 char * cp, cTxt[120], cStr1[64], cStr2[64], cStr3[64];
 WORD * wp, wV1, wV2;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
  	wV1 = *wp;
	cp += 2;
	wp = (WORD *)cp;
  	wV2 = *wp;
	cp += 2;
	ZeroMemory( cStr1, sizeof(cStr1) );
	ZeroMemory( cStr2, sizeof(cStr2) );
	ZeroMemory( cStr3, sizeof(cStr3) );
	m_pGame->GetItemName( m_pGame->m_pItemList[wV1], cStr1, cStr2, cStr3 );

	switch (wV2) {
	case 1:
		wsprintf(cTxt, NOTIFYMSG_CANNOT_REPAIR_ITEM1, cStr1 );
		m_pGame->AddEventList(cTxt, 10);
 		break;
	case 2:
		wsprintf(cTxt, NOTIFYMSG_CANNOT_REPAIR_ITEM2, cStr1 );
		m_pGame->AddEventList(cTxt, 10);
 		break;
	}
	m_pGame->m_bIsItemDisabled[wV1] = FALSE;
}

void CMessageHandler::NotifyMsg_CannotSellItem(char * Data)
{
 char * cp, cTxt[120], cStr1[64], cStr2[64], cStr3[64];
 WORD * wp, wV1, wV2;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	wp = (WORD *)cp;
  	wV1 = *wp;
	cp += 2;

	wp = (WORD *)cp;
  	wV2 = *wp;
	cp += 2;

	ZeroMemory( cStr1, sizeof(cStr1) );
	ZeroMemory( cStr2, sizeof(cStr2) );
	ZeroMemory( cStr3, sizeof(cStr3) );
	m_pGame->GetItemName( m_pGame->m_pItemList[wV1], cStr1, cStr2, cStr3 );

	switch (wV2) {
	case 1:
		wsprintf(cTxt, NOTIFYMSG_CANNOT_SELL_ITEM1, cStr1);//"
		m_pGame->AddEventList(cTxt, 10);
		break;

	case 2:
		wsprintf(cTxt, NOTIFYMSG_CANNOT_SELL_ITEM2, cStr1);//"
		m_pGame->AddEventList(cTxt, 10);
		break;

	case 3:
		wsprintf(cTxt, NOTIFYMSG_CANNOT_SELL_ITEM3, cStr1);//"
		m_pGame->AddEventList(cTxt, 10);
		m_pGame->AddEventList(NOTIFYMSG_CANNOT_SELL_ITEM4, 10);//"
		break;

	case 4:
		m_pGame->AddEventList(NOTIFYMSG_CANNOT_SELL_ITEM5, 10); // "
		m_pGame->AddEventList(NOTIFYMSG_CANNOT_SELL_ITEM6, 10); // "
		break;
	}
	m_pGame->m_bIsItemDisabled[wV1] = FALSE;
}

void CMessageHandler::NotifyMsg_SellItemPrice(char * Data)
{char * cp, cName[21];
 DWORD * dwp, wV1, wV2, wV3, wV4;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	dwp = (DWORD *)cp;
  	wV1 = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
  	wV2 = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
  	wV3 = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
  	wV4 = *dwp;
	cp += 4;
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;
	m_pGame->EnableDialogBox(23, 1, wV1, wV2);
	m_pGame->m_stDialogBoxInfo[23].sV3 = wV3;
	m_pGame->m_stDialogBoxInfo[23].sV4 = wV4;
}

void CMessageHandler::NotifyMsg_ShowMap(char * Data)
{char * cp;
 WORD * wp, w1, w2;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	w1 = *wp;
	cp += 2;
	wp = (WORD *)cp;
	w2 = *wp;
	cp += 2;
	if (w2 == 0) m_pGame->AddEventList(NOTIFYMSG_SHOW_MAP1, 10);
	else m_pGame->EnableDialogBox(22, NULL, w1, w2 -1);
}

void CMessageHandler::NotifyMsg_SkillUsingEnd(char * Data)
{char * cp;
 WORD * wp, wResult;
	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	wResult = * wp;
	switch (wResult) {
	case NULL:
		m_pGame->AddEventList(NOTIFYMSG_SKILL_USINGEND1, 10);
		break;
	case 1:
		m_pGame->AddEventList(NOTIFYMSG_SKILL_USINGEND2, 10);
		break;
	}
	m_pGame->m_bSkillUsingStatus = FALSE;
}

void CMessageHandler::NotifyMsg_TotalUsers(char * Data)
{	
	WORD *wp;
	int iTotal;
	wp = (WORD *)(Data + INDEX2_MSGTYPE + 2);
	iTotal = (int)*wp;
	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_TOTAL_USER1, iTotal);
	m_pGame->AddEventList(m_pGame->G_cTxt, 10);
}

void CMessageHandler::NotifyMsg_EventStart(char * Data)
{	
	char *cp;
	EventType eType;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	eType = (EventType)*cp;

	for(int i=0; i < MAXSIDES; i++)
	{
		m_pGame->m_astoriaStats[i].deaths = 0;
		m_pGame->m_astoriaStats[i].kills = 0;
	}
	m_pGame->m_bIsAstoriaMode = TRUE;
	m_pGame->m_relicOwnedTime = 0;

	switch(eType)
	{
	case ET_CAPTURE:
		wsprintf(m_pGame->G_cTxt, NOTIFYMSG_EVENTSTART_CTR);
		break;
	default:
		return;
	}

	strcat(m_pGame->G_cTxt, NOTIFYMSG_EVENTSTART);
	m_pGame->SetTopMsg(m_pGame->G_cTxt, 10);
}

void CMessageHandler::NotifyMsg_EventStarting(char * Data)
{	
	char *cp;
	EventType eType;

	WORD * wp = (WORD *)(Data + INDEX2_MSGTYPE);
	DWORD msgID = *wp;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	eType = (EventType)*cp;

	sstream msg;
	msg.str().c_str();
	msg.flush();
	msg.str().c_str();

	msg << "A " << eventName[eType] << " event will be starting in ";
	switch(msgID)
	{
	case NOTIFY_EVENTSTARTING:
		msg << "2 hours.";
		break;
	case NOTIFY_EVENTSTARTING2:
		msg << "30 minutes.";
		break;
	case NOTIFY_EVENTSTARTING3:
		msg << "10 minutes.";
		break;
	}

	m_pGame->SetTopMsg(msg.str().c_str(), 10);
}

void CMessageHandler::NotifyMsg_Casualties(char * Data)
{	
	WORD *wp;
	wp = (WORD *)(Data + INDEX2_MSGTYPE + 2);

	m_pGame->m_astoriaStats[ARESDEN].deaths = *wp;
	wp++;
	m_pGame->m_astoriaStats[ELVINE].deaths = *wp;
	wp++;
	m_pGame->m_astoriaStats[ISTRIA].deaths = *wp;
	wp++;
	m_pGame->m_astoriaStats[ARESDEN].kills = *wp;
	wp++;
	m_pGame->m_astoriaStats[ELVINE].kills = *wp;
	wp++;
	m_pGame->m_astoriaStats[ISTRIA].kills = *wp;

	m_pGame->m_bIsAstoriaMode = TRUE;
}

void CMessageHandler::NotifyMsg_RelicInAltar(char * Data)
{	
	char *cp;
	Side altarSide;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	altarSide = (Side)*cp;
	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_RELICINALTAR, sideName[altarSide]);
	m_pGame->SetTopMsg(m_pGame->G_cTxt, 10);

	m_pGame->m_relicOwnedSide = altarSide;
	m_pGame->m_relicOwnedTime = timeGetTime() + 1 _s;
}

void CMessageHandler::NotifyMsg_RelicGrabbed(char * Data)
{	
	char *cp;	
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	char playerName[12];
	ZeroMemory(playerName, sizeof(playerName));
	memcpy(playerName, cp, 10);

	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_RELICGRABBED, playerName);
	m_pGame->AddEventList(m_pGame->G_cTxt, 10);

	m_pGame->m_relicOwnedTime = 0;
}

void CMessageHandler::NotifyMsg_CTRWinner(char * Data)
{	
	char *cp;
	Side winnerSide;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	winnerSide = (Side)*cp;
	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_CTRWINNER, sideName[winnerSide]);
	m_pGame->SetTopMsg(m_pGame->G_cTxt, 10);

	m_pGame->m_bIsAstoriaMode = FALSE;
}

void CMessageHandler::NotifyMsg_MagicEffectOff(char * Data)
{char * cp;
 WORD * wp;
 short  sMagicType, sMagicEffect;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	sMagicType = (short)*wp;
	cp += 2;
	wp = (WORD *)cp;
	sMagicEffect = (short)*wp;
	cp += 2;
	switch (sMagicType) {
	case MAGICTYPE_PROTECT:
		switch (sMagicEffect) {
		case 1: // "Protection from arrows has vanished."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF1, 10);
			break;
		case 2:	// "Protection from magic has vanished."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF2, 10);
			break;
		case 3:	// "Defense shield effect has vanished."
		case 4:	// "Defense shield effect has vanished."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF3, 10);
			break;
		case 5:	// "Absolute Magic Protection has been vanished."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF14, 10);
			break;
		}
		break;

	case MAGICTYPE_HOLDOBJECT:
		switch (sMagicEffect) {
		case 1:	// "Hold person magic effect has vanished."
			m_pGame->m_bParalyze = FALSE;
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF4, 10);
			break;

		case 2:	// "Paralysis magic effect has vanished."
			m_pGame->m_bParalyze = FALSE;
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF5, 10);
			break;
		}
		break;

	case MAGICTYPE_INVISIBILITY:
		switch (sMagicEffect) {
		case 1:	// "Invisibility magic effect has vanished."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF6, 10);
			break;
		}
		break;

	case MAGICTYPE_CONFUSE:
		switch (sMagicEffect) {
		case 1:	// "Language confuse magic effect has vanished."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF7, 10);
			break;
		case 2:	// "Confusion magic has vanished."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF8, 10);
			m_pGame->m_bIsConfusion = FALSE;
			break;
		case 3:	// "Illusion magic has vanished."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF9, 10);
			m_pGame->m_iIlusionOwnerH = NULL;
			break;
		case 4:	// "At last, you gather your senses." // snoopy
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF15, 10);
			m_pGame->m_bIllusionMVT = FALSE;
			break;
		}
		break;

	case MAGICTYPE_POISON:
		if (m_pGame->m_bIsPoisoned) m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF10, 10);
		m_pGame->m_bIsPoisoned = FALSE;
		break;

	case MAGICTYPE_BERSERK:
		switch (sMagicEffect) {
		case 1:
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF11, 10);
			break;
		}
		break;

	case MAGICTYPE_POLYMORPH:
		switch (sMagicEffect) {
		case 1:
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF12, 10);
			break;
		}
		break;

	case MAGICTYPE_ICE:
		m_pGame->m_iPlayerStatus &= ~STATUS_FROZEN;
		m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_OFF13, 10);
		break;

	case MAGICTYPE_SPEED:
		m_pGame->m_bSpeedBuffActive = false;
		m_pGame->m_dwSpeedBuffEndTime = 0;
		m_pGame->ApplySpeedBuff(false);
		m_pGame->AddEventList("Speed buff has worn off.", 10);
		break;
	}
}

void CMessageHandler::NotifyMsg_MagicEffectOn(char * Data)
{char * cp;
 DWORD * dwp;
 WORD * wp;
 short  sMagicType, sMagicEffect, sOwnerH;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	sMagicType = (short)*wp;
	cp += 2;
	dwp = (DWORD *)cp;
	sMagicEffect = (short)*dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	sOwnerH = (short)*dwp;
	cp += 4;
	switch (sMagicType) {
	case MAGICTYPE_PROTECT:
		switch (sMagicEffect) {
		case 1: // "You are completely protected from arrows!"
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON1, 10);
			break;
		case 2: // "You are protected from magic!"
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON2, 10);
			break;
		case 3: // "Defense ratio increased by a magic shield!"
		case 4: // "Defense ratio increased by a magic shield!"
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON3, 10);
			break;
		case 5: // "You are completely protected from magic!"
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON14, 10);
			break;
		}
		break;

	case MAGICTYPE_HOLDOBJECT:
		switch (sMagicEffect) {
		case 1: // "You were bounded by a Hold Person spell! Unable to move!"
			m_pGame->m_bParalyze = TRUE;
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON4, 10);
			break;
		case 2: // "You were bounded by a Paralysis spell! Unable to move!"
			m_pGame->m_bParalyze = TRUE;
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON5, 10);
			break;
		}
		break;

	case MAGICTYPE_INVISIBILITY:
		switch (sMagicEffect) {
		case 1: // "You are now invisible, no one can see you!"
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON6, 10);
			break;
		}
		break;

	case MAGICTYPE_CONFUSE:
		switch (sMagicEffect) {
		case 1:	// Confuse Language "No one understands you because of language confusion magic!"
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON7, 10);
			break;

		case 2: // Confusion "Confusion magic casted, impossible to determine player allegience."
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON8, 10);
			m_pGame->m_bIsConfusion = TRUE;
			break;

		case 3:	// Illusion "Illusion magic casted, impossible to tell who is who!"
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON9, 10);
			m_pGame->_SetIlusionEffect(sOwnerH);
			break;

		case 4:	// IllusionMouvement "You are thrown into confusion, and you are flustered yourself." // snoopy
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON15, 10);
			m_pGame->m_bIllusionMVT = TRUE;
			break;
		}
		break;

	case MAGICTYPE_POISON:
		m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON10, 10);
		m_pGame->m_bIsPoisoned = TRUE;
		break;

	case MAGICTYPE_BERSERK:
		switch (sMagicEffect) {
		case 1:
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON11, 10);
			break;
		case 2:
			m_pGame->AddEventList("Minor Berserk Magic Casted!", 10);
			break;
		}
		break;

	case MAGICTYPE_POLYMORPH:
		switch (sMagicEffect) {
		case 1:
			m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON12, 10);
			break;
		}
		break;

	case MAGICTYPE_ICE:
		m_pGame->m_iPlayerStatus |= STATUS_FROZEN;
		m_pGame->AddEventList(NOTIFYMSG_MAGICEFFECT_ON13, 10);
		break;
	}
}

void CMessageHandler::NotifyMsg_SetItemCount(char * Data)
{char  * cp;
 WORD  * wp;
 DWORD * dwp;
 short  sItemIndex;
 DWORD  dwCount;
 BOOL   bIsItemUseResponse;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	sItemIndex = *wp;
	cp += 2;
	dwp = (DWORD *)cp;
	dwCount = *dwp;
	cp += 4;
	bIsItemUseResponse = (BOOL)*cp;
	cp++;
	if (m_pGame->m_pItemList[sItemIndex] != NULL)
	{	m_pGame->m_pItemList[sItemIndex]->m_dwCount = dwCount;
		if (bIsItemUseResponse == TRUE) m_pGame->m_bIsItemDisabled[sItemIndex] = FALSE;
	}
}

void CMessageHandler::NotifyMsg_ItemDepleted_EraseItem(char * Data)
{
 char * cp;
 WORD * wp;
 short  sItemIndex;
 BOOL   bIsUseItemResult;
 char   cTxt[120];

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	wp = (WORD *)cp;
	sItemIndex = *wp;
	cp += 2;

	bIsUseItemResult = (BOOL)*cp;
	cp += 2;

	ZeroMemory(cTxt, sizeof(cTxt));

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(m_pGame->m_pItemList[sItemIndex], cStr1, cStr2, cStr3);

	if (m_pGame->m_bIsItemEquipped[sItemIndex] == TRUE) {
		wsprintf(cTxt, ITEM_EQUIPMENT_RELEASED, cStr1);
		m_pGame->AddEventList(cTxt, 10);
		
		m_pGame->m_sItemEquipmentStatus[	m_pGame->m_pItemList[sItemIndex]->m_cEquipPos ] = -1;
		m_pGame->m_bIsItemEquipped[sItemIndex] = FALSE;
	}

	ZeroMemory(cTxt, sizeof(cTxt));

	switch(m_pGame->m_pItemList[sItemIndex]->m_cItemType)
	{
	case ITEMTYPE_CONSUME:
	case ITEMTYPE_ARROW:
		wsprintf(cTxt, NOTIFYMSG_ITEMDEPlETED_ERASEITEM2, cStr1);
		break;
	case ITEMTYPE_USE_DEPLETE:
		if (bIsUseItemResult) 
			wsprintf(cTxt, NOTIFYMSG_ITEMDEPlETED_ERASEITEM3, cStr1);
		break;
	case ITEMTYPE_EAT:
		if (bIsUseItemResult) 
		{
			wsprintf(cTxt, NOTIFYMSG_ITEMDEPlETED_ERASEITEM4, cStr1);
			if ( (m_pGame->m_sPlayerType >= 1) && (m_pGame->m_sPlayerType <= 3) )
				m_pGame->PlaySound('C', 19, 0);
			if ( (m_pGame->m_sPlayerType >= 4) && (m_pGame->m_sPlayerType <= 6) )
				m_pGame->PlaySound('C', 20, 0);
		}
		break;
	case ITEMTYPE_USE_DEPLETE_DEST:
		if (bIsUseItemResult)
			wsprintf(cTxt, NOTIFYMSG_ITEMDEPlETED_ERASEITEM3, cStr1);
		break;
	default:
		if (bIsUseItemResult) 
		{
			wsprintf(cTxt, NOTIFYMSG_ITEMDEPlETED_ERASEITEM6, cStr1);
			m_pGame->PlaySound('E', 10, 0);
		}
		break;
	}

	m_pGame->AddEventList(cTxt, 10);

	if (bIsUseItemResult == TRUE) 	m_pGame->m_bItemUsingStatus = FALSE;
	m_pGame->EraseItem((char)sItemIndex);
	m_pGame->_bCheckBuildItemStatus();
}

void CMessageHandler::NotifyMsg_ServerChange(char * Data)
{
 char * cp, cWorldServerAddr[16];	//Snoopy: change names for better readability
 int * ip, iWorldServerPort;		//Snoopy: change names for better readability

	MapChangeLog("SERVERCHANGE_START");

	ZeroMemory(m_pGame->m_cMapName, sizeof(m_pGame->m_cMapName));
	ZeroMemory(m_pGame->m_cMapMessage, sizeof(m_pGame->m_cMapMessage));
	ZeroMemory(cWorldServerAddr, sizeof(cWorldServerAddr));


	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
    memcpy(m_pGame->m_cMapName, cp, 10);

//	m_cMapIndex = GetOfficialMapName(m_cMapName, m_cMapMessage);
	cp += 10;

	memcpy(cWorldServerAddr, cp, 15);
	cp += 15;
	ip = (int *)cp;
	iWorldServerPort = *ip;
	cp += 4;

	MapChangeLog("SERVERCHANGE_PARSED map=%s addr=%s port=%d LAN=%d",
		m_pGame->m_cMapName, cWorldServerAddr, iWorldServerPort, m_pGame->m_iGameServerMode);

	if (m_pGame->m_pGSock != NULL)
	{	delete m_pGame->m_pGSock;
		m_pGame->m_pGSock = NULL;
	}
	MapChangeLog("SERVERCHANGE_GSOCK_DELETED");

	if (m_pGame->m_pLSock != NULL)
	{	delete m_pGame->m_pLSock;
		m_pGame->m_pLSock = NULL;
	}
	MapChangeLog("SERVERCHANGE_LSOCK_DELETED");

	// Drain stale socket events from both GSOCK and LSOCK after deleting old sockets.
	// WSAAsyncSelect events already queued before closesocket() can persist in the
	// Windows message queue. If the OS reuses the old socket handle for a new socket,
	// the stale events (especially FD_CLOSE) would be mistakenly delivered to the new
	// socket, killing the connection during cross-server teleport.
	{	MSG msg;
		while (PeekMessage(&msg, m_pGame->m_hWnd, WM_USER_GAMESOCKETEVENT, WM_USER_GAMESOCKETEVENT, PM_REMOVE));
		while (PeekMessage(&msg, m_pGame->m_hWnd, WM_USER_LOGSOCKETEVENT, WM_USER_LOGSOCKETEVENT, PM_REMOVE));
	}
	MapChangeLog("SERVERCHANGE_EVENTS_DRAINED");

	m_pGame->m_pLSock = new class XSocket(m_pGame->m_hWnd, SOCKETBLOCKLIMIT);
	if (m_pGame->m_iGameServerMode == 1) // LAN
	{	MapChangeLog("SERVERCHANGE_CONNECTING LAN addr=%s port=%d", m_pGame->m_cLogServerAddr, iWorldServerPort);
		m_pGame->m_pLSock->bConnect(m_pGame->m_cLogServerAddr, iWorldServerPort, WM_USER_LOGSOCKETEVENT);
	}else
	{	MapChangeLog("SERVERCHANGE_CONNECTING WAN addr=%s port=%d", cWorldServerAddr, iWorldServerPort);
		m_pGame->m_pLSock->bConnect(cWorldServerAddr, iWorldServerPort, WM_USER_LOGSOCKETEVENT);
	}
	m_pGame->m_pLSock->bInitBufferSize(30000);

	m_pGame->m_bIsPoisoned = FALSE;

	m_pGame->ChangeGameMode(GAMEMODE_ONCONNECTING);
	m_pGame->m_dwConnectMode  = MSGID_REQUEST_ENTERGAME;
	//m_wEnterGameType = ENTERGAMEMSGTYPE_NEW; //Gateway
	m_pGame->m_wEnterGameType = ENTERGAMEMSGTYPE_CHANGINGSERVER;
		ZeroMemory(m_pGame->m_cMsg, sizeof(m_pGame->m_cMsg));
	strcpy(m_pGame->m_cMsg,"55");
	MapChangeLog("SERVERCHANGE_COMPLETE mode=%d enterType=%d", (int)m_pGame->m_dwConnectMode, (int)m_pGame->m_wEnterGameType);
}

void CMessageHandler::NotifyMsg_Skill(char *Data)
{ WORD * wp;
 short sSkillIndex, sValue;
 char * cp;
 char cTxt[120];
 int i;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	sSkillIndex = (short)*wp;
	cp += 2;
	wp = (WORD *)cp;
	sValue = (short)*wp;
	cp += 2;
	m_pGame->_RemoveChatMsgListByObjectID(m_pGame->m_sPlayerObjectID);
	if(!m_pGame->m_pSkillCfgList[sSkillIndex]) return;
	if (m_pGame->m_pSkillCfgList[sSkillIndex]->m_iLevel < sValue)
	{	wsprintf(cTxt, NOTIFYMSG_SKILL1, m_pGame->m_pSkillCfgList[sSkillIndex]->m_cName, sValue - m_pGame->m_pSkillCfgList[sSkillIndex]->m_iLevel);
		m_pGame->AddEventList(cTxt, 10);
		m_pGame->PlaySound('E', 23, 0);
		for (i = 1; i < MAXCHATMSGS; i++)
		if (m_pGame->m_pChatMsgList[i] == NULL)
		{	ZeroMemory(cTxt, sizeof(cTxt));
			wsprintf(cTxt, "%s +%d%%", m_pGame->m_pSkillCfgList[sSkillIndex]->m_cName, sValue - m_pGame->m_pSkillCfgList[sSkillIndex]->m_iLevel);
			m_pGame->m_pChatMsgList[i] = new class CMsg(20, cTxt, m_pGame->m_dwCurTime);
			m_pGame->m_pChatMsgList[i]->m_iObjectID = m_pGame->m_sPlayerObjectID;
			if (m_pGame->m_pMapData->bSetChatMsgOwner(m_pGame->m_sPlayerObjectID, -10, -10, i) == FALSE)
			{	delete m_pGame->m_pChatMsgList[i];
				m_pGame->m_pChatMsgList[i] = NULL;
			}
			break;
		}
	}else if (m_pGame->m_pSkillCfgList[sSkillIndex]->m_iLevel > sValue) {
		wsprintf(cTxt, NOTIFYMSG_SKILL2, m_pGame->m_pSkillCfgList[sSkillIndex]->m_cName, m_pGame->m_pSkillCfgList[sSkillIndex]->m_iLevel - sValue);
		m_pGame->AddEventList(cTxt, 10);
		m_pGame->PlaySound('E', 24, 0);
		for (i = 1; i < MAXCHATMSGS; i++)
		if (m_pGame->m_pChatMsgList[i] == NULL)
		{	ZeroMemory(cTxt, sizeof(cTxt));
			wsprintf(cTxt, "%s -%d%%", m_pGame->m_pSkillCfgList[sSkillIndex]->m_cName, sValue - m_pGame->m_pSkillCfgList[sSkillIndex]->m_iLevel);
			m_pGame->m_pChatMsgList[i] = new class CMsg(20, cTxt, m_pGame->m_dwCurTime);
			m_pGame->m_pChatMsgList[i]->m_iObjectID = m_pGame->m_sPlayerObjectID;
			if (m_pGame->m_pMapData->bSetChatMsgOwner(m_pGame->m_sPlayerObjectID, -10, -10, i) == FALSE)
			{	delete m_pGame->m_pChatMsgList[i];
				m_pGame->m_pChatMsgList[i] = NULL;
			}
			break;
	}	}
	m_pGame->m_pSkillCfgList[sSkillIndex]->m_iLevel = sValue;
	m_pGame->m_cSkillMastery[sSkillIndex] = (unsigned char)sValue;
}

void CMessageHandler::NotifyMsg_DropItemFin_EraseItem(char *Data)
{
 char * cp;
 WORD * wp;
 int * ip, iAmount;
 short  sItemIndex;
 char   cTxt[120];

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	wp = (WORD *)cp;
	sItemIndex = *wp;
	cp += 2;

	ip = (int *)cp;
	iAmount = *ip;
	cp += 4;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(m_pGame->m_pItemList[sItemIndex], cStr1, cStr2, cStr3);

	ZeroMemory(cTxt, sizeof(cTxt));
	if (m_pGame->m_bIsItemEquipped[sItemIndex] == TRUE)
	{	wsprintf(cTxt, ITEM_EQUIPMENT_RELEASED, cStr1);
		m_pGame->AddEventList(cTxt, 10);
		m_pGame->m_sItemEquipmentStatus[	m_pGame->m_pItemList[sItemIndex]->m_cEquipPos ] = -1;
		m_pGame->m_bIsItemEquipped[sItemIndex] = FALSE;
	}
	if (m_pGame->m_iHP > 0)
	{	wsprintf(cTxt, NOTIFYMSG_THROW_ITEM2, cStr1);
	}else
	{	if (iAmount < 2)
			wsprintf(cTxt, NOTIFYMSG_DROPITEMFIN_ERASEITEM3, cStr1); // "You dropped a %s."
		else // Snoopy fix
		{	wsprintf(cTxt, NOTIFYMSG_DROPITEMFIN_ERASEITEM5, cStr1); // "You dropped %s."
	}	}
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->EraseItem((char)sItemIndex);
	m_pGame->_bCheckBuildItemStatus();
}

void CMessageHandler::NotifyMsg_GiveItemFin_EraseItem(char *Data)
{
 char * cp;
 WORD * wp;
 int  * ip, iAmount;
 short  sItemIndex;
 char cName[21], cTxt[250];


	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	wp = (WORD *)cp;
	sItemIndex = *wp;
	cp += 2;

	ip = (int *)cp;
	iAmount = *ip;
	cp += 4;

	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(m_pGame->m_pItemList[sItemIndex]->m_cName, m_pGame->m_pItemList[sItemIndex]->m_dwAttribute, cStr1, cStr2, cStr3);

	if (m_pGame->m_bIsItemEquipped[sItemIndex] == TRUE) {
		wsprintf(cTxt, ITEM_EQUIPMENT_RELEASED, cStr1);
		m_pGame->AddEventList(cTxt, 10);
		
		m_pGame->m_sItemEquipmentStatus[	m_pGame->m_pItemList[sItemIndex]->m_cEquipPos ] = -1;
		m_pGame->m_bIsItemEquipped[sItemIndex] = FALSE;
	}
	if (strlen(cName) == 0) wsprintf(cTxt, NOTIFYMSG_GIVEITEMFIN_ERASEITEM2, iAmount, cStr1);
	else {
		if (strcmp(cName, "Howard") == 0)
			 wsprintf(cTxt, NOTIFYMSG_GIVEITEMFIN_ERASEITEM3, iAmount, cStr1);
		else if (strcmp(cName, "William") == 0)
			 wsprintf(cTxt, NOTIFYMSG_GIVEITEMFIN_ERASEITEM4, iAmount, cStr1);
		else if (strcmp(cName, "Kennedy") == 0)
			wsprintf(cTxt, NOTIFYMSG_GIVEITEMFIN_ERASEITEM5, iAmount, cStr1);
		else if (strcmp(cName, "Tom") == 0)
			wsprintf(cTxt, NOTIFYMSG_GIVEITEMFIN_ERASEITEM7, iAmount, cStr1);
		else wsprintf(cTxt, NOTIFYMSG_GIVEITEMFIN_ERASEITEM8, iAmount, cStr1, cName);
	}
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->EraseItem((char)sItemIndex);
	m_pGame->_bCheckBuildItemStatus();
}

void CMessageHandler::NotifyMsg_EnemyKillReward(char *Data)
{
 DWORD * dwp;
 short * sp, sGuildRank;
 char  * cp, cName[12], cGuildName[24], cTxt[120];
 int   iExp, iEnemyKillCount, iWarContribution;

	ZeroMemory(cName, sizeof(cName));
	ZeroMemory(cGuildName, sizeof(cGuildName));

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	dwp  = (DWORD *)cp;
	iExp = *dwp;
	cp += 4;
	dwp  = (DWORD *)cp;
	iEnemyKillCount = *dwp;
	cp += 4;
	memcpy(cName, cp, 10);
	cp += 10;
	memcpy(cGuildName, cp, 20);
	cp += 20;
	sp  = (short *)cp;
	sGuildRank = *sp;
	cp += 2;
	sp  = (short *)cp;
	iWarContribution = *sp;
	cp += 2;

	if (iWarContribution > m_pGame->m_iWarContribution)
	{	wsprintf(m_pGame->G_cTxt, "%s +%d!", CRUSADE_MESSAGE21, iWarContribution - m_pGame->m_iWarContribution);
		m_pGame->SetTopMsg(m_pGame->G_cTxt, 5);
	}else if (iWarContribution < m_pGame->m_iWarContribution)
	{}
	m_pGame->m_iWarContribution = iWarContribution;

	if (sGuildRank == -1)
	{	wsprintf(cTxt, NOTIFYMSG_ENEMYKILL_REWARD1, cName);
		m_pGame->AddEventList(cTxt, 10);
	}else
	{	wsprintf(cTxt, NOTIFYMSG_ENEMYKILL_REWARD2, cName, cGuildName); // Fixed by Snoopy
		m_pGame->AddEventList(cTxt, 10);
	}

/*	if( m_pGame->m_iExp != iExp ) // removed by snoopy because too much msg hide victim's name
	{	if (m_pGame->m_iExp > iExp) wsprintf(cTxt, EXP_DECREASED,m_pGame->m_iExp - iExp);
		else wsprintf(cTxt, EXP_INCREASED,iExp - m_pGame->m_iExp);
		m_pGame->AddEventList(cTxt, 10);
	}*/

	if (m_pGame->m_iEnemyKillCount != iEnemyKillCount)
	{	if (m_pGame->m_iEnemyKillCount > iEnemyKillCount)
		{	wsprintf(cTxt, NOTIFYMSG_ENEMYKILL_REWARD5,m_pGame->m_iEnemyKillCount - iEnemyKillCount);
			m_pGame->AddEventList(cTxt, 10);
		}else
		{	wsprintf(cTxt, NOTIFYMSG_ENEMYKILL_REWARD6, iEnemyKillCount - m_pGame->m_iEnemyKillCount);
			m_pGame->AddEventList(cTxt, 10);
		}
	}

	if( iExp >= 0 ) m_pGame->m_iExp = iExp;
	if( iEnemyKillCount >= 0 ) m_pGame->m_iEnemyKillCount = iEnemyKillCount;
	m_pGame->PlaySound('E', 23, 0);

	if(m_pGame->m_ekScreenshot)
		m_pGame->m_ekSSTime = timeGetTime() + 650;
}

void CMessageHandler::NotifyMsg_PKcaptured(char *Data)
{char  * cp;
 DWORD * dwp;
 WORD  * wp;
 int     iPKcount, iLevel, iExp, iRewardGold;
 char cTxt[120], cName[12];
	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	wp = (WORD *)cp;
	iPKcount = *wp;
	cp += 2;
	wp = (WORD *)cp;
	iLevel = *wp;
	cp += 2;
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 10);
	cp += 10;
	dwp = (DWORD *)cp;
	iRewardGold = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	iExp = *dwp;
	cp += 4;
	wsprintf(cTxt, NOTIFYMSG_PK_CAPTURED1, iLevel, cName, iPKcount);
	m_pGame->AddEventList(cTxt, 10);
	wsprintf(cTxt, EXP_INCREASED, iExp - m_pGame->m_iExp);
	m_pGame->AddEventList(cTxt, 10);
	wsprintf(cTxt, NOTIFYMSG_PK_CAPTURED3, iExp - m_pGame->m_iExp);
	m_pGame->AddEventList(cTxt, 10);
}

void CMessageHandler::NotifyMsg_PKpenalty(char *Data)
{char  * cp;
 DWORD * dwp;
 int     iPKcount, iExp, iStr, iVit, iDex, iInt, iMag, iChr;
	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	dwp = (DWORD *)cp;
	iExp = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	iStr = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	iVit = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	iDex = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	iInt = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	iMag = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	iChr = *dwp;
	cp += 4;
	dwp = (DWORD *)cp;
	iPKcount = *dwp;
	cp += 4;
	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_PK_PENALTY1, iPKcount);
	m_pGame->AddEventList(m_pGame->G_cTxt, 10);
	if (m_pGame->m_iExp > iExp)
	{	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_PK_PENALTY2, m_pGame->m_iExp - iExp);
		m_pGame->AddEventList(m_pGame->G_cTxt, 10);
	}
	m_pGame->m_iExp = iExp;
	m_pGame->m_stat[STAT_STR] = iStr;
	m_pGame->m_stat[STAT_VIT] = iVit;
	m_pGame->m_stat[STAT_DEX] = iDex;
	m_pGame->m_stat[STAT_INT] = iInt;
	m_pGame->m_stat[STAT_MAG] = iMag;
	m_pGame->m_stat[STAT_CHR] = iChr;
	m_pGame->m_iPKCount = iPKcount;
}

void CMessageHandler::NotifyMsg_ItemToBank(char *Data)
{
 char * cp, cIndex;
 DWORD * dwp, dwCount, dwAttribute;
 char  cName[21], cItemType, cEquipPos, cGenderLimit, cItemColor;
 BOOL  bIsEquipped;
 short * sp, sSprite, sSpriteFrame, sLevelLimit, sItemEffectValue2, sItemSpecEffectValue2;
 WORD  * wp, wWeight, wCurLifeSpan;
 char  cTxt[120];

	cp = (Data + INDEX2_MSGTYPE + 2);

	cIndex = *cp;
	cp++;

	cp++;

	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;

	dwp = (DWORD *)cp;
	dwCount = *dwp;
	cp += 4;

	cItemType = *cp;
	cp++;

	cEquipPos = *cp;
	cp++;

	bIsEquipped = (BOOL)*cp;
	cp++;

	sp = (short *)cp;
	sLevelLimit = *sp;
	cp += 2;

	cGenderLimit = *cp;
	cp++;

	wp = (WORD *)cp;
	wCurLifeSpan = *wp;
	cp += 2;

	wp = (WORD *)cp;
	wWeight = *wp;
	cp += 2;

	sp = (short *)cp;
	sSprite = *sp;
	cp += 2;

	sp = (short *)cp;
	sSpriteFrame = *sp;
	cp += 2;

	cItemColor = *cp;
	cp++;

	
	sp = (short *)cp;
	sItemEffectValue2 = *sp;
	cp += 2;

	dwp = (DWORD *)cp;
	dwAttribute = *dwp;
	cp += 4;
	sItemSpecEffectValue2 = (short) *cp ;
	cp ++ ;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(cName, dwAttribute, cStr1, cStr2, cStr3);


	if (cIndex >= 0 && cIndex == (int)m_pGame->m_pBankList.size() && (int)m_pGame->m_pBankList.size() < MAXBANKITEMS) {
		m_pGame->m_pBankList.push_back(new class CItem);

		memcpy(m_pGame->m_pBankList[cIndex]->m_cName, cName, 20);
		m_pGame->m_pBankList[cIndex]->m_dwCount = dwCount;

		m_pGame->m_pBankList[cIndex]->m_cItemType = cItemType;
		m_pGame->m_pBankList[cIndex]->m_cEquipPos = cEquipPos;

		m_pGame->m_pBankList[cIndex]->m_sLevelLimit  = sLevelLimit;
		m_pGame->m_pBankList[cIndex]->m_cGenderLimit = cGenderLimit;
		m_pGame->m_pBankList[cIndex]->m_wCurLifeSpan = wCurLifeSpan;
		m_pGame->m_pBankList[cIndex]->m_wWeight      = wWeight;
		m_pGame->m_pBankList[cIndex]->m_sSprite      = sSprite;
		m_pGame->m_pBankList[cIndex]->m_sSpriteFrame = sSpriteFrame;
		m_pGame->m_pBankList[cIndex]->m_cItemColor   = cItemColor;
		m_pGame->m_pBankList[cIndex]->m_sItemEffectValue2  = sItemEffectValue2;
		m_pGame->m_pBankList[cIndex]->m_dwAttribute        = dwAttribute;
		m_pGame->m_pBankList[cIndex]->m_sItemSpecEffectValue2 = sItemSpecEffectValue2 ;

		ZeroMemory(cTxt, sizeof(cTxt));
		if( dwCount == 1 ) wsprintf(cTxt, NOTIFYMSG_ITEMTOBANK3, cStr1);
		else wsprintf(cTxt, NOTIFYMSG_ITEMTOBANK2, dwCount, cStr1);

		int scrollPos = (int)m_pGame->m_pBankList.size() - 12;
		if( m_pGame->m_bIsDialogEnabled[14] == TRUE ) m_pGame->m_stDialogBoxInfo[14].sView = (scrollPos > 0) ? scrollPos : 0;
		m_pGame->AddEventList(cTxt, 10);
	}
}

void CMessageHandler::NotifyMsg_ItemLifeSpanEnd(char * Data)
{
 char * cp;
 short * sp, sEquipPos, sItemIndex;
 char cTxt[120];

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	sp = (short *)cp;
	sEquipPos = *sp;
	cp += 2;
	sp = (short *)cp;
	sItemIndex = *sp;
	cp += 2;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName( m_pGame->m_pItemList[sItemIndex], cStr1, cStr2, cStr3 );
	wsprintf(cTxt, NOTIFYMSG_ITEMLIFE_SPANEND1, cStr1);
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->m_sItemEquipmentStatus[	m_pGame->m_pItemList[sItemIndex]->m_cEquipPos ] = -1;
	m_pGame->m_bIsItemEquipped[sItemIndex] = FALSE;
	m_pGame->m_pItemList[sItemIndex]->m_wCurLifeSpan = 0;

	m_pGame->PlaySound('E', 10, 0);
}

void CMessageHandler::NotifyMsg_ItemReleased(char * Data)
{
 char * cp;
 short * sp, sEquipPos, sItemIndex;
 char cTxt[120];

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	sp = (short *)cp;
	sEquipPos = *sp;
	cp += 2;
	sp = (short *)cp;
	sItemIndex = *sp;
	cp += 2;

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(m_pGame->m_pItemList[sItemIndex], cStr1, cStr2, cStr3);
	wsprintf(cTxt, ITEM_EQUIPMENT_RELEASED, cStr1);
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->m_bIsItemEquipped[sItemIndex] = FALSE;
	m_pGame->m_sItemEquipmentStatus[	m_pGame->m_pItemList[sItemIndex]->m_cEquipPos ] = -1;

	if(memcmp(m_pGame->m_pItemList[sItemIndex]->m_cName, "AngelicPendant", 14) == 0) m_pGame->PlaySound('E', 53, 0);
	else m_pGame->PlaySound('E', 29, 0);
}

void CMessageHandler::NotifyMsg_LevelUp(char * Data)
{char * cp;
 int  * ip;
 int i, iPrevLevel;
 char cTxt[120];

	iPrevLevel = m_pGame->m_iLevel;

	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);

	ip  = (int *)cp;
	m_pGame->m_iLevel = *ip;
	cp += 4;

	ip   = (int *)cp;
	m_pGame->m_stat[STAT_STR] = *ip;
	cp  += 4;

	ip   = (int *)cp;
	m_pGame->m_stat[STAT_VIT] = *ip;
	cp  += 4;

	ip   = (int *)cp;
	m_pGame->m_stat[STAT_DEX] = *ip;
	cp  += 4;

	ip   = (int *)cp;
	m_pGame->m_stat[STAT_INT] = *ip;
	cp  += 4;

	ip   = (int *)cp;
	m_pGame->m_stat[STAT_MAG] = *ip;
	cp  += 4;

	ip   = (int *)cp;
	m_pGame->m_stat[STAT_CHR] = *ip;
	cp  += 4;

	// CLEROTH - LU
	m_pGame->m_iLU_Point = m_pGame->m_iLevel*3 - (
			(m_pGame->m_stat[STAT_STR] + m_pGame->m_stat[STAT_VIT] + m_pGame->m_stat[STAT_DEX] + m_pGame->m_stat[STAT_INT] + m_pGame->m_stat[STAT_MAG] + m_pGame->m_stat[STAT_CHR])
			- 70) 
			- 3 + m_pGame->m_angelStat[STAT_STR] + m_pGame->m_angelStat[STAT_DEX] + m_pGame->m_angelStat[STAT_INT] + m_pGame->m_angelStat[STAT_MAG];
	m_pGame->m_luStat[STAT_STR] = m_pGame->m_luStat[STAT_VIT] = m_pGame->m_luStat[STAT_DEX] = m_pGame->m_luStat[STAT_INT] = m_pGame->m_luStat[STAT_MAG] = m_pGame->m_luStat[STAT_CHR] = 0;

	wsprintf(cTxt, NOTIFYMSG_LEVELUP1, m_pGame->m_iLevel);// "Level up!!! Level %d!"
	m_pGame->AddEventList(cTxt, 10);

	switch (m_pGame->m_sPlayerType) {
	case 1:
	case 2:
	case 3:
		m_pGame->PlaySound('C', 21, 0);
		break;

	case 4:
	case 5:
	case 6:
		m_pGame->PlaySound('C', 22, 0);
		break;
	}

	m_pGame->_RemoveChatMsgListByObjectID(m_pGame->m_sPlayerObjectID);

	for (i = 1; i < MAXCHATMSGS; i++)
	if (m_pGame->m_pChatMsgList[i] == NULL) {
		ZeroMemory(cTxt, sizeof(cTxt));
		strcpy(cTxt, "Level up!");
		m_pGame->m_pChatMsgList[i] = new class CMsg(23, cTxt, m_pGame->m_dwCurTime);
		m_pGame->m_pChatMsgList[i]->m_iObjectID = m_pGame->m_sPlayerObjectID;

		if (m_pGame->m_pMapData->bSetChatMsgOwner(m_pGame->m_sPlayerObjectID, -10, -10, i) == FALSE) {
			delete m_pGame->m_pChatMsgList[i];
			m_pGame->m_pChatMsgList[i] = NULL;
		}
		return;
	}
}

void CMessageHandler::NotifyMsg_SettingSuccess(char * Data)
{char * cp;
 int  * ip;
 int iPrevLevel;
 char cTxt[120];
	iPrevLevel = m_pGame->m_iLevel;
	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	ip  = (int *)cp;
	m_pGame->m_iLevel = *ip;
	cp += 4;
	ip   = (int *)cp;
	m_pGame->m_stat[STAT_STR] = *ip;
	cp  += 4;
	ip   = (int *)cp;
	m_pGame->m_stat[STAT_VIT] = *ip;
	cp  += 4;
	ip   = (int *)cp;
	m_pGame->m_stat[STAT_DEX] = *ip;
	cp  += 4;
	ip   = (int *)cp;
	m_pGame->m_stat[STAT_INT] = *ip;
	cp  += 4;
	ip   = (int *)cp;
	m_pGame->m_stat[STAT_MAG] = *ip;
	cp  += 4;
	ip   = (int *)cp;
	m_pGame->m_stat[STAT_CHR] = *ip;
	cp  += 4;
	wsprintf(cTxt, "Your stat has been changed.");
	m_pGame->AddEventList(cTxt, 10);
	// CLEROTH - LU
	m_pGame->m_iLU_Point = m_pGame->m_iLevel*3 - (
			(m_pGame->m_stat[STAT_STR] + m_pGame->m_stat[STAT_VIT] + m_pGame->m_stat[STAT_DEX] + m_pGame->m_stat[STAT_INT] + m_pGame->m_stat[STAT_MAG] + m_pGame->m_stat[STAT_CHR])
			- 70) 
			- 3 + m_pGame->m_angelStat[STAT_STR] + m_pGame->m_angelStat[STAT_DEX] + m_pGame->m_angelStat[STAT_INT] + m_pGame->m_angelStat[STAT_MAG];
	m_pGame->m_luStat[STAT_STR] = m_pGame->m_luStat[STAT_VIT] = m_pGame->m_luStat[STAT_DEX] = m_pGame->m_luStat[STAT_INT] = m_pGame->m_luStat[STAT_MAG] = m_pGame->m_luStat[STAT_CHR] = 0;
}

void CMessageHandler::NotifyMsg_MP(char * Data)
{DWORD * dwp;
 int iPrevMP;
 char cTxt[120];
	iPrevMP = m_pGame->m_iMP;
	dwp = (DWORD *)(Data + INDEX2_MSGTYPE + 2);
	m_pGame->m_iMP = (int)*dwp;
	if (abs(m_pGame->m_iMP - iPrevMP) < 10) return;
	if (m_pGame->m_iMP > iPrevMP)
	{	wsprintf(cTxt, NOTIFYMSG_MP_UP, m_pGame->m_iMP - iPrevMP);
		m_pGame->AddEventList(cTxt, 10);
		m_pGame->PlaySound('E', 21, 0);
	}else
	{	wsprintf(cTxt, NOTIFYMSG_MP_DOWN, iPrevMP - m_pGame->m_iMP);
		m_pGame->AddEventList(cTxt, 10);
	}
}

void CMessageHandler::NotifyMsg_SP(char * Data)
{DWORD * dwp;
 int iPrevSP;
	iPrevSP = m_pGame->m_iSP;
	dwp = (DWORD *)(Data + INDEX2_MSGTYPE + 2);
	m_pGame->m_iSP = (int)*dwp;
	if (abs(m_pGame->m_iSP - iPrevSP) < 10) return;
	if (m_pGame->m_iSP > iPrevSP)
	{	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_SP_UP, m_pGame->m_iSP - iPrevSP);
		m_pGame->AddEventList(m_pGame->G_cTxt, 10);
		m_pGame->PlaySound('E', 21, 0);
	}else
	{	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_SP_DOWN, iPrevSP - m_pGame->m_iSP);
		m_pGame->AddEventList(m_pGame->G_cTxt, 10);
	}
}

void CMessageHandler::NotifyMsg_SkillTrainSuccess(char * Data)
{char * cp, cSkillNum, cSkillLevel;
 char cTemp[120];
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	cSkillNum = *cp;
	cp++;
	cSkillLevel = *cp;
	cp++;
	if(!m_pGame->m_pSkillCfgList[cSkillNum]) return;
	ZeroMemory(cTemp, sizeof(cTemp));
	wsprintf(cTemp, NOTIFYMSG_SKILL_TRAIN_SUCCESS1, m_pGame->m_pSkillCfgList[cSkillNum]->m_cName, cSkillLevel);
	m_pGame->AddEventList(cTemp, 10);
	m_pGame->m_pSkillCfgList[cSkillNum]->m_iLevel = cSkillLevel;
	m_pGame->m_cSkillMastery[cSkillNum] = (unsigned char)cSkillLevel;
	m_pGame->PlaySound('E', 23, 0);
}

void CMessageHandler::NotifyMsg_MagicStudyFail(char * Data)
{
 char * cp, cMagicNum, cName[31], cFailCode;
 char cTxt[120];
 int  * ip, iCost, iReqInt;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	cFailCode = *cp;
	cp++;
	cMagicNum = *cp;
	cp++;
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 30);
	cp += 30;
	ip = (int *)cp;
	iCost = *ip;
	cp += 4;
	ip = (int *)cp;
	iReqInt = *ip;
	cp += 4;
/*	// Snoopy: remove special CLEROTH's feature
	ip = (int *)cp;
	iReqStr = *ip;
	cp += 4;
	// CLEROTH
	wsprintf(cTxt, NOTIFYMSG_MAGICSTUDY_FAIL4, cName, iCost, iReqInt, iReqStr);
	m_pGame->AddEventList(cTxt, 10);*/

	if (iCost > 0)
	{	wsprintf(cTxt, NOTIFYMSG_MAGICSTUDY_FAIL1, cName);
		m_pGame->AddEventList(cTxt, 10);
	}else
	{	wsprintf(cTxt, NOTIFYMSG_MAGICSTUDY_FAIL2,  cName);
		m_pGame->AddEventList(cTxt, 10);
		wsprintf(cTxt, NOTIFYMSG_MAGICSTUDY_FAIL3, iReqInt);
		m_pGame->AddEventList(cTxt, 10);
	}
}

void CMessageHandler::NotifyMsg_MagicStudySuccess(char * Data)
{char * cp, cMagicNum, cName[31];
 char cTxt[120];
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	cMagicNum = *cp;
	cp++;
	m_pGame->m_cMagicMastery[cMagicNum] = 1;
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 30);
	wsprintf(cTxt, NOTIFYMSG_MAGICSTUDY_SUCCESS1, cName);
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->PlaySound('E', 23, 0);
	m_pGame->InitSpecialAbilities();
}

void CMessageHandler::NotifyMsg_DismissGuildsMan(char * Data)
{
 char * cp, cName[12], cTxt[120];
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 10);

	if( memcmp( m_pGame->m_cPlayerName, cName, 10 ) != 0 ) {
		wsprintf(cTxt, NOTIFYMSG_DISMISS_GUILDMAN1, cName);
		m_pGame->AddEventList(cTxt, 10);
	}
	m_pGame->ClearGuildNameList();
}

void CMessageHandler::NotifyMsg_NewGuildsMan(char * Data)
{char * cp, cName[12], cTxt[120];
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 10);
	wsprintf(cTxt, NOTIFYMSG_NEW_GUILDMAN1, cName);
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->ClearGuildNameList();
}

void CMessageHandler::NotifyMsg_CannotJoinMoreGuildsMan(char * Data)
{
 char * cp, cName[12], cTxt[120];

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 10);

	wsprintf(cTxt, NOTIFYMSG_CANNOT_JOIN_MOREGUILDMAN1, cName);
	m_pGame->AddEventList(cTxt, 10);
	m_pGame->AddEventList(NOTIFYMSG_CANNOT_JOIN_MOREGUILDMAN2, 10);
}

void CMessageHandler::NotifyMsg_GuildDisbanded(char * Data)
{char * cp, cName[24], cLocation[12];
	ZeroMemory(cName, sizeof(cName));
	ZeroMemory(cLocation, sizeof(cLocation));
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	memcpy(cName, cp, 20);
	cp += 20;
	memcpy(cLocation, cp, 10);
	cp += 10;
	m_pGame->m_Misc.ReplaceString(cName, '_', ' ');
	m_pGame->EnableDialogBox(8, NULL, NULL, NULL);
	m_pGame->_PutGuildOperationList(cName, 7);
	ZeroMemory(m_pGame->m_cGuildName, sizeof(m_pGame->m_cGuildName));
	m_pGame->m_iGuildRank = -1;
	ZeroMemory(m_pGame->m_cLocation, sizeof(m_pGame->m_cLocation));
	memcpy(m_pGame->m_cLocation, cLocation, 10);

	if (memcmp(m_pGame->m_cLocation, "are", 3) == 0)
		m_pGame->m_side = ARESDEN;
	else if (memcmp(m_pGame->m_cLocation, "elv", 3) == 0)
		m_pGame->m_side = ELVINE;
	else if (memcmp(m_pGame->m_cLocation, "ist", 3) == 0)
		m_pGame->m_side = ISTRIA;
	else
		m_pGame->m_side = NEUTRAL;
}

void CMessageHandler::NotifyMsg_Exp(char * Data)
{
 DWORD * dwp;
 int iPrevExp, * ip;
 char * cp, cTxt[120];

	iPrevExp = m_pGame->m_iExp;
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	dwp = (DWORD *)cp;
	m_pGame->m_iExp = (int)*dwp;
	cp += 4;

	ip = (int *)cp;
//	m_iRating = *ip;
	cp += 4;

	if (m_pGame->m_iExp > iPrevExp)
	{	wsprintf(cTxt, EXP_INCREASED, m_pGame->m_iExp - iPrevExp);
		m_pGame->AddEventList(cTxt, 10);
	}else if (m_pGame->m_iExp < iPrevExp)
	{	wsprintf(cTxt, EXP_DECREASED, iPrevExp - m_pGame->m_iExp);
		m_pGame->AddEventList(cTxt, 10);
	}
}

void CMessageHandler::NotifyMsg_Killed(char * Data)
{ char * cp, cAttackerName[21];
	m_pGame->m_bCommandAvailable = FALSE;
	m_pGame->m_cCommand = OBJECTSTOP;
	m_pGame->m_iHP = 0;
	m_pGame->m_cCommand = -1;
	// Restart
	m_pGame->m_bItemUsingStatus = FALSE;
	m_pGame->ClearSkillUsingStatus();
	ZeroMemory(cAttackerName, sizeof(cAttackerName));
	cp = (char *)(Data	+ INDEX2_MSGTYPE + 2);
	memcpy(cAttackerName, cp, 20);
	cp += 20;
/*	if (strlen(cAttackerName) == 0) // removed in v2.20 (bug?) Many servers send the info themselves.
		m_pGame->AddEventList(NOTIFYMSG_KILLED1, 10);
	else
	{	wsprintf(m_pGame->G_cTxt, NOTIFYMSG_KILLED2, cAttackerName);
		m_pGame->AddEventList(m_pGame->G_cTxt, 10);
	}*/
	// Snoopy: reduced 3 lines -> 2 lines
	m_pGame->AddEventList(NOTIFYMSG_KILLED1, 10);
	m_pGame->AddEventList(NOTIFYMSG_KILLED3, 10);
	//AddEventList(NOTIFYMSG_KILLED4, 10);//"Log Out
}

void CMessageHandler::NotifyMsg_HP(char * Data)
{
 DWORD * dwp;
 int iPrevHP;
 char cTxt[120];
 int iPrevMP;

	iPrevHP = m_pGame->m_iHP;
	dwp = (DWORD *)(Data + INDEX2_MSGTYPE + 2);
	m_pGame->m_iHP = (int)*dwp;

	iPrevMP = m_pGame->m_iMP;
	dwp = (DWORD *)(Data + INDEX2_MSGTYPE + 6);
	m_pGame->m_iMP = (int)*dwp;

	if (m_pGame->m_iHP > iPrevHP)
	{	if ((m_pGame->m_iHP - iPrevHP) < 10) return;
		m_pGame->PlaySound('E', 21, 0);
	}else
	{	if ( (m_pGame->m_cLogOutCount > 0) && (m_pGame->m_bForceDisconn==FALSE) )
		{	m_pGame->m_cLogOutCount = -1;
			m_pGame->AddEventList(NOTIFYMSG_HP2, 10);
		}
		m_pGame->m_dwDamagedTime = timeGetTime();
		if (m_pGame->m_iHP < 20) m_pGame->AddEventList(NOTIFYMSG_HP3, 10);
		if ((iPrevHP - m_pGame->m_iHP) < 10) return;
		wsprintf(cTxt, NOTIFYMSG_HP_DOWN, iPrevHP - m_pGame->m_iHP);
		m_pGame->AddEventList(cTxt, 10);
	}
}

void CMessageHandler::NotifyMsg_ItemPurchased(char * Data)
{
 char  * cp;
 short * sp;
 DWORD * dwp;
 WORD  * wp;
 int i, j;

 DWORD dwCount;
 char  cName[21], cItemType, cEquipPos, cGenderLimit;
 BOOL  bIsEquipped;
 short sSprite, sSpriteFrame, sLevelLimit;
 WORD  wCost, wWeight, wCurLifeSpan;
 char  cTxt[120], cItemColor;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	cp++;

	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;

	dwp = (DWORD *)cp;
	dwCount = *dwp;
	cp += 4;

	cItemType = *cp;
	cp++;

	cEquipPos = *cp;
	cp++;

	bIsEquipped = (BOOL)*cp;
	cp++;

	sp = (short *)cp;
	sLevelLimit = *sp;
	cp += 2;

	cGenderLimit = *cp;
	cp++;

 	wp = (WORD *)cp;
	wCurLifeSpan = *wp;
	cp += 2;

	wp = (WORD *)cp;
	wWeight = *wp;
	cp += 2;

	sp = (short *)cp;
	sSprite = *sp;
	cp += 2;

	sp = (short *)cp;
	sSpriteFrame = *sp;
	cp += 2;

	cItemColor = *cp; 
	cp++;

	wp = (WORD *)cp;
	wCost = *wp;
	ZeroMemory(cTxt, sizeof(cTxt));
	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName( cName, NULL, cStr1, cStr2, cStr3 );
	wsprintf(cTxt, NOTIFYMSG_ITEMPURCHASED, cStr1, wCost);
	m_pGame->AddEventList(cTxt, 10);

	if ( (cItemType == ITEMTYPE_CONSUME) || (cItemType == ITEMTYPE_ARROW))
	{	for (i = 0; i < MAXITEMS; i++)
		if ((m_pGame->m_pItemList[i] != NULL) && (memcmp(m_pGame->m_pItemList[i]->m_cName, cName, 20) == 0))
		{	m_pGame->m_pItemList[i]->m_dwCount += dwCount;
			return;
	}	}

 short nX, nY;
 for (i = 0; i < MAXITEMS; i++)
  {	  if ( ( m_pGame->m_pItemList[i] != NULL) && (memcmp(m_pGame->m_pItemList[i]->m_cName, cName, 20) == 0))
	  {	  nX = m_pGame->m_pItemList[i]->m_sX;
		  nY = m_pGame->m_pItemList[i]->m_sY;
		  break;
	  }else
	  {  nX = 40;
		  nY = 30;
  }  }

	for (i = 0; i < MAXITEMS; i++)
	if (m_pGame->m_pItemList[i] == NULL)
	{	m_pGame->m_pItemList[i] = new class CItem;
		memcpy(m_pGame->m_pItemList[i]->m_cName, cName, 20);
		m_pGame->m_pItemList[i]->m_dwCount      = dwCount;
		//m_pItemList[i]->m_sX           = 40;
		//m_pItemList[i]->m_sY           = 30;
		m_pGame->m_pItemList[i]->m_sX           = nX;
		m_pGame->m_pItemList[i]->m_sY           = nY;
		m_pGame->bSendCommand(MSGID_REQUEST_SETITEMPOS, NULL, i, nX, nY, NULL, NULL);
		m_pGame->m_pItemList[i]->m_cItemType    = cItemType;
		m_pGame->m_pItemList[i]->m_cEquipPos    = cEquipPos;
		m_pGame->m_bIsItemDisabled[i]           = FALSE;
		m_pGame->m_bIsItemEquipped[i]           = FALSE;
		m_pGame->m_pItemList[i]->m_sLevelLimit  = sLevelLimit;
		m_pGame->m_pItemList[i]->m_cGenderLimit = cGenderLimit;
		m_pGame->m_pItemList[i]->m_wCurLifeSpan = wCurLifeSpan;
		m_pGame->m_pItemList[i]->m_wWeight      = wWeight;
		m_pGame->m_pItemList[i]->m_sSprite      = sSprite;
		m_pGame->m_pItemList[i]->m_sSpriteFrame = sSpriteFrame;
		m_pGame->m_pItemList[i]->m_cItemColor   = cItemColor;    

		// fixed v1.11
		for (j = 0; j < MAXITEMS; j++)
		if (m_pGame->m_cItemOrder[j] == -1) {
			m_pGame->m_cItemOrder[j] = i;
			return;
		}

		return;
	}
}

void CMessageHandler::NotifyMsg_DismissGuildReject(char * Data)
{char * cp, cName[21];
	ZeroMemory(cName, sizeof(cName));
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	memcpy(cName, cp, 20);
	cp += 20;
	m_pGame->EnableDialogBox(8, NULL, NULL, NULL);
	m_pGame->_PutGuildOperationList(cName, 6);
}

void CMessageHandler::NotifyMsg_DismissGuildApprove(char * Data)
{
 char * cp, cName[24], cLocation[12];
	ZeroMemory(cName, sizeof(cName));
	ZeroMemory(cLocation, sizeof(cLocation));
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	memcpy(cName, cp, 20);
	cp += 20;
	cp += 2;
	memcpy(cLocation, cp, 10);
	cp += 10;
	ZeroMemory(m_pGame->m_cGuildName, sizeof(m_pGame->m_cGuildName));
	m_pGame->m_iGuildRank = -1;
	ZeroMemory(m_pGame->m_cLocation, sizeof(m_pGame->m_cLocation));
	memcpy(m_pGame->m_cLocation, cLocation, 10);

	if (memcmp(m_pGame->m_cLocation, "are", 3) == 0)
		m_pGame->m_side = ARESDEN;
	else if (memcmp(m_pGame->m_cLocation, "elv", 3) == 0)
		m_pGame->m_side = ELVINE;
	else if (memcmp(m_pGame->m_cLocation, "ist", 3) == 0)
		m_pGame->m_side = ISTRIA;
	else
		m_pGame->m_side = NEUTRAL;

	m_pGame->EnableDialogBox(8, NULL, NULL, NULL);
	m_pGame->_PutGuildOperationList(cName, 5);
}

void CMessageHandler::NotifyMsg_JoinGuildReject(char * Data)
{char * cp, cName[21];
	ZeroMemory(cName, sizeof(cName));
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	memcpy(cName, cp, 20);
	cp += 20;
	m_pGame->EnableDialogBox(8, NULL, NULL, NULL);
	m_pGame->_PutGuildOperationList(cName, 4);
}

void CMessageHandler::NotifyMsg_JoinGuildApprove(char * Data)
{char * cp, cName[21];
 short * sp;
	ZeroMemory(cName, sizeof(cName));
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	memcpy(cName, cp, 20);
	cp += 20;
	sp = (short *)cp;
	cp += 2;
	ZeroMemory(m_pGame->m_cGuildName, sizeof(m_pGame->m_cGuildName));
	strcpy(m_pGame->m_cGuildName, cName);
	m_pGame->m_iGuildRank = *sp;
	m_pGame->EnableDialogBox(8, NULL, NULL, NULL);
	m_pGame->_PutGuildOperationList(cName, 3);
}

void CMessageHandler::NotifyMsg_QueryDismissGuildPermission(char * Data)
{char * cp, cName[12];
	ZeroMemory(cName, sizeof(cName));
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	memcpy(cName, cp, 10);
	cp += 10;
	m_pGame->EnableDialogBox(8, NULL, NULL, NULL);
	m_pGame->_PutGuildOperationList(cName, 2);
}

void CMessageHandler::NotifyMsg_QueryJoinGuildPermission(char * Data)
{char * cp, cName[12];
	ZeroMemory(cName, sizeof(cName));
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	memcpy(cName, cp, 10);
	cp += 10;
	m_pGame->EnableDialogBox(8, NULL, NULL, NULL);
	m_pGame->_PutGuildOperationList(cName, 1);
}

void CMessageHandler::NotifyMsg_ItemObtained(char * Data)
{
 char * cp;
 short * sp;
 DWORD * dwp;
 int i, j;

 DWORD dwCount, dwAttribute;
 char  cName[21], cItemType, cEquipPos;
 BOOL  bIsEquipped;
 short sSprite, sSpriteFrame, sLevelLimit, sSpecialEV2;
 char  cTxt[120], cGenderLimit, cItemColor;
 WORD  * wp, wWeight, wCurLifeSpan;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);

	cp++;

	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 20);
	cp += 20;

	dwp = (DWORD *)cp;
	dwCount = *dwp;
	cp += 4;

	cItemType = *cp;
	cp++;

	cEquipPos = *cp;
	cp++;

	bIsEquipped = (BOOL)*cp;
	cp++;

	sp = (short *)cp;
	sLevelLimit = *sp;
	cp += 2;

	cGenderLimit = *cp;
	cp++;

	wp = (WORD *)cp;
	wCurLifeSpan = *wp;
	cp += 2;

	wp = (WORD *)cp;
	wWeight = *wp;
	cp += 2;

	sp = (short *)cp;
	sSprite = *sp;
	cp += 2;

	sp = (short *)cp;
	sSpriteFrame = *sp;
	cp += 2;

	cItemColor = *cp;
	cp++;

	sSpecialEV2 = (short)*cp; 
	cp++;

	dwp = (DWORD *)cp;
	dwAttribute = *dwp;
	cp += 4;
	/*
	bIsCustomMade = (BOOL)*cp;
	cp++;
	*/

	char cStr1[64], cStr2[64], cStr3[64];
	m_pGame->GetItemName(cName, dwAttribute, cStr1, cStr2, cStr3);

	ZeroMemory(cTxt, sizeof(cTxt));
	if( dwCount == 1 ) wsprintf(cTxt, NOTIFYMSG_ITEMOBTAINED2, cStr1);
	else wsprintf(cTxt, NOTIFYMSG_ITEMOBTAINED1, dwCount, cStr1);

	m_pGame->AddEventList(cTxt, 10);

	m_pGame->PlaySound('E', 20, 0);

	if ((cItemType == ITEMTYPE_CONSUME) || (cItemType == ITEMTYPE_ARROW))
	{	for (i = 0; i < MAXITEMS; i++)
		if ((m_pGame->m_pItemList[i] != NULL) && (memcmp(m_pGame->m_pItemList[i]->m_cName, cName, 20) == 0))
		{	m_pGame->m_pItemList[i]->m_dwCount += dwCount;
			m_pGame->m_bIsItemDisabled[i] = FALSE;
			return;
	}	}

  	short nX, nY;
  	for (i = 0; i < MAXITEMS; i++)
  	{	if ( ( m_pGame->m_pItemList[i] != NULL) && (memcmp(m_pGame->m_pItemList[i]->m_cName, cName, 20) == 0)) 
		{	nX = m_pGame->m_pItemList[i]->m_sX;
			nY = m_pGame->m_pItemList[i]->m_sY;
			break;
		}else
		{	nX = 40;
			nY = 30;
	}	}
 

	for (i = 0; i < MAXITEMS; i++)
	if (m_pGame->m_pItemList[i] == NULL)
	{	m_pGame->m_pItemList[i] = new class CItem;
		memcpy(m_pGame->m_pItemList[i]->m_cName, cName, 20);
		m_pGame->m_pItemList[i]->m_dwCount = dwCount;
		//m_pItemList[i]->m_sX      =	40;
		//m_pItemList[i]->m_sY      =	30;
		m_pGame->m_pItemList[i]->m_sX      =	nX;
		m_pGame->m_pItemList[i]->m_sY      =	nY;
		m_pGame->bSendCommand(MSGID_REQUEST_SETITEMPOS, NULL, i, nX, nY, NULL, NULL);
		m_pGame->m_pItemList[i]->m_cItemType = cItemType;
		m_pGame->m_pItemList[i]->m_cEquipPos = cEquipPos;
		m_pGame->m_bIsItemDisabled[i]        = FALSE;

		m_pGame->m_bIsItemEquipped[i] = FALSE;
		m_pGame->m_pItemList[i]->m_sLevelLimit  = sLevelLimit;
		m_pGame->m_pItemList[i]->m_cGenderLimit = cGenderLimit;
		m_pGame->m_pItemList[i]->m_wCurLifeSpan = wCurLifeSpan;
		m_pGame->m_pItemList[i]->m_wWeight      = wWeight;
		m_pGame->m_pItemList[i]->m_sSprite      = sSprite;
		m_pGame->m_pItemList[i]->m_sSpriteFrame = sSpriteFrame;
		m_pGame->m_pItemList[i]->m_cItemColor   = cItemColor;
		m_pGame->m_pItemList[i]->m_sItemSpecEffectValue2 = sSpecialEV2; 
		m_pGame->m_pItemList[i]->m_dwAttribute = dwAttribute;
		//m_pItemList[i]->m_bIsCustomMade = bIsCustomMade;

		m_pGame->_bCheckBuildItemStatus();

		for (j = 0; j < MAXITEMS; j++)
		if (m_pGame->m_cItemOrder[j] == -1) {
			m_pGame->m_cItemOrder[j] = i;
			return;
		}
		return;
	}
}

void CMessageHandler::NotifyMsg_ForceDisconn(char *Data)
{
	WORD * wpCount;
	wpCount = (WORD *)(Data + 6);
	m_pGame->m_bForceDisconn = TRUE;
	//m_cLogOutCount = (char)*wpCount;
	if( m_pGame->m_bIsProgramActive )
	{	if( m_pGame->m_cLogOutCount < 0 || m_pGame->m_cLogOutCount > 5 ) m_pGame->m_cLogOutCount = 5;
		m_pGame->AddEventList(NOTIFYMSG_FORCE_DISCONN1, 10);
	}else
	{	delete m_pGame->m_pGSock;
		m_pGame->m_pGSock = NULL;
		m_pGame->m_bEscPressed = FALSE;
		if (m_pGame->m_SoundMgr.m_bSoundFlag ) m_pGame->m_SoundMgr.StopWeatherSound();
		if ((m_pGame->m_SoundMgr.m_bSoundFlag) && (m_pGame->m_bMusicStat == TRUE))
		{	
			m_pGame->m_SoundMgr.StopBGM();
		}
		if (strlen(G_cCmdLineTokenA) != 0)
			 m_pGame->ChangeGameMode(GAMEMODE_ONQUIT);
		else m_pGame->ChangeGameMode(GAMEMODE_ONMAINMENU);
	}
}

void CMessageHandler::NotifyMsg_BanGuildMan(char * Data)
{ char * cp, cName[24], cLocation[12];
	ZeroMemory(cName, sizeof(cName));
	ZeroMemory(cLocation, sizeof(cLocation));
	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	memcpy(cName, cp, 20);
	cp += 20;
	cp += 2;
	memcpy(cLocation, cp, 10);
	cp += 10;
	ZeroMemory(m_pGame->m_cGuildName, sizeof(m_pGame->m_cGuildName));
	m_pGame->m_iGuildRank = -1;
	ZeroMemory(m_pGame->m_cLocation, sizeof(m_pGame->m_cLocation));
	memcpy(m_pGame->m_cLocation, cLocation, 10);

	if (memcmp(m_pGame->m_cLocation, "are", 3) == 0)
		m_pGame->m_side = ARESDEN;
	else if (memcmp(m_pGame->m_cLocation, "elv", 3) == 0)
		m_pGame->m_side = ELVINE;
	else if (memcmp(m_pGame->m_cLocation, "ist", 3) == 0)
		m_pGame->m_side = ISTRIA;
	else
		m_pGame->m_side = NEUTRAL;

	m_pGame->EnableDialogBox(8, NULL, NULL, NULL);
	m_pGame->_PutGuildOperationList(cName, 8);
}

void CMessageHandler::NotifyMsg_FriendOnGame(char * Data)
{
	char cName[12], * cp;

	cp = (char *)(Data + INDEX2_MSGTYPE + 2);
	ZeroMemory(cName, sizeof(cName));
	memcpy(cName, cp, 10);

	for(int i = 0; i < m_pGame->m_iTotalFriends; i++)
		if(strcmp(cName,m_pGame->friendsList[i].friendName) == 0){
			m_pGame->friendsList[i].online = true;
			m_pGame->friendsList[i].updated = true;
		}
}

