// MessageHandler.h: Network message handler class (extracted from CGame)
//
//////////////////////////////////////////////////////////////////////

#if !defined(MESSAGEHANDLER_H_INCLUDED_)
#define MESSAGEHANDLER_H_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif

class CGame;

class CMessageHandler
{
public:
	CMessageHandler() : m_pGame(NULL) {}
	void Init(CGame* pGame) { m_pGame = pGame; }

	// Top-level dispatchers
	void GameRecvMsgHandler(DWORD dwMsgSize, char * Data);
	void NotifyMsgHandler(char * Data);

	// NotifyMsg handlers
	void NotifyMsg_Heldenian(char * Data);
	void NotifyMsg_GlobalAttackMode(char * Data);
	void NotifyMsg_QuestReward(char * Data);
	void NotifyMsg_QuestContents(char * Data);
	void NotifyMsg_ItemColorChange(char * Data);
	void NotifyMsg_DropItemFin_CountChanged(char * Data);
	void NotifyMsg_CannotGiveItem(char * Data);
	void NotifyMsg_GiveItemFin_CountChanged(char * Data);
	void NotifyMsg_SetExchangeItem(char * Data);
	void NotifyMsg_OpenExchageWindow(char * Data);
	void NotifyMsg_DownSkillIndexSet(char * Data);
	void NotifyMsg_AdminInfo(char * Data);
	void NotifyMsg_WhetherChange(char * Data);
	void NotifyMsg_FishChance(char * Data);
	void NotifyMsg_EventFishMode(char * Data);
	void NotifyMsg_NoticeMsg(char * Data);
	void NotifyMsg_RatingPlayer(char * Data);
	void NotifyMsg_CannotRating(char * Data);
	void NotifyMsg_PlayerShutUp(char * Data);
	void NotifyMsg_TimeChange(char * Data);
	void NotifyMsg_Hunger(char * Data);
	void NotifyMsg_PlayerProfile(char * Data);
	void NotifyMsg_WhisperMode(BOOL bActive, char * Data);
	void NotifyMsg_PlayerStatus(BOOL bOnGame, char * Data);
	void NotifyMsg_Range(char * Data);
	void NotifyMsg_ItemRepaired(char * Data);
	void NotifyMsg_RepairItemPrice(char * Data);
	void NotifyMsg_CannotRepairItem(char * Data);
	void NotifyMsg_CannotSellItem(char * Data);
	void NotifyMsg_SellItemPrice(char * Data);
	void NotifyMsg_ShowMap(char * Data);
	void NotifyMsg_SkillUsingEnd(char * Data);
	void NotifyMsg_TotalUsers(char * Data);
	void NotifyMsg_EventStart(char * Data);
	void NotifyMsg_EventStarting(char * Data);
	void NotifyMsg_Casualties(char * Data);
	void NotifyMsg_RelicInAltar(char * Data);
	void NotifyMsg_RelicGrabbed(char * Data);
	void NotifyMsg_CTRWinner(char * Data);
	void NotifyMsg_MagicEffectOff(char * Data);
	void NotifyMsg_MagicEffectOn(char * Data);
	void NotifyMsg_SetItemCount(char * Data);
	void NotifyMsg_ItemDepleted_EraseItem(char * Data);
	void NotifyMsg_ServerChange(char * Data);
	void NotifyMsg_Skill(char * Data);
	void NotifyMsg_DropItemFin_EraseItem(char * Data);
	void NotifyMsg_GiveItemFin_EraseItem(char * Data);
	void NotifyMsg_EnemyKillReward(char * Data);
	void NotifyMsg_PKcaptured(char * Data);
	void NotifyMsg_PKpenalty(char * Data);
	void NotifyMsg_ItemToBank(char * Data);
	void NotifyMsg_ItemLifeSpanEnd(char * Data);
	void NotifyMsg_ItemReleased(char * Data);
	void NotifyMsg_LevelUp(char * Data);
	void NotifyMsg_SettingSuccess(char * Data);
	void NotifyMsg_MP(char * Data);
	void NotifyMsg_SP(char * Data);
	void NotifyMsg_SkillTrainSuccess(char * Data);
	void NotifyMsg_MagicStudyFail(char * Data);
	void NotifyMsg_MagicStudySuccess(char * Data);
	void NotifyMsg_DismissGuildsMan(char * Data);
	void NotifyMsg_NewGuildsMan(char * Data);
	void NotifyMsg_CannotJoinMoreGuildsMan(char * Data);
	void NotifyMsg_GuildDisbanded(char * Data);
	void NotifyMsg_Exp(char * Data);
	void NotifyMsg_Killed(char * Data);
	void NotifyMsg_HP(char * Data);
	void NotifyMsg_ItemPurchased(char * Data);
	void NotifyMsg_DismissGuildReject(char * Data);
	void NotifyMsg_DismissGuildApprove(char * Data);
	void NotifyMsg_JoinGuildReject(char * Data);
	void NotifyMsg_JoinGuildApprove(char * Data);
	void NotifyMsg_QueryDismissGuildPermission(char * Data);
	void NotifyMsg_QueryJoinGuildPermission(char * Data);
	void NotifyMsg_ItemObtained(char * Data);
	void NotifyMsg_ForceDisconn(char * Data);
	void NotifyMsg_BanGuildMan(char * Data);
	void NotifyMsg_FriendOnGame(char * Data);

	// Other handlers dispatched from GameRecvMsgHandler
	void InitPlayerResponseHandler(char * Data);
	void InitDataResponseHandler(char * Data);
	void MotionResponseHandler(char * Data);
	void CommonEventHandler(char * Data);
	void MotionEventHandler(char * Data);
	void LogEventHandler(char * Data);
	void ChatMsgHandler(char * Data);
	void InitItemList(char * Data);
	void InitSkillList(char * Data);
	void CreateNewGuildResponseHandler(char * Data);
	void DisbandGuildResponseHandler(char * Data);
	void InitPlayerCharacteristics(char * Data);
	void CivilRightAdmissionHandler(char * Data);
	void RetrieveItemHandler(char * Data);
	void ResponsePanningHandler(char * Data);
	void ReserveFightzoneResponseHandler(char * Data);
	void NoticementHandler(char * Data);
	void DynamicObjectHandler(char * Data);
	void ResponseTeleportList(char * Data);
	void ResponseChargedTeleport(char * Data);
	void NpcTalkHandler(char * Data);
	void Notify_PartyInfo(char * Data);

private:
	CGame* m_pGame;
};

#endif // !defined(MESSAGEHANDLER_H_INCLUDED_)
