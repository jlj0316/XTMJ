#include "GH_HeNan_ZhengZhou.h"
#include "Config.h"
#include "Work.h"
#include "LLog.h"


bool GH_HeNan_ZhengZhou::startup(Desk *desk)
{
	if (desk)
	{
		shutdown();
		m_desk = desk;
	}
	return true;
}

void GH_HeNan_ZhengZhou::shutdown(void)
{
	for (Lint i = 0; i< DESK_USER_COUNT; ++i)
	{
		m_louHuCard[i] = NULL;
		m_startHuRet[i] = 0;
		m_thinkInfo[i].Reset();
		m_thinkRet[i].Clear();
		m_startHu[i].clear();
		m_handCard[i].clear();
		m_outCard[i].clear();
		m_pengCard[i].clear();
		m_gangCard[i].clear();
		m_abombCard[i].clear();
		m_eatCard[i].clear();
		m_pengInfo[i].clear();
		m_pengGangInfo[i].clear();
		m_angang[i] = 0;
		m_minggang[i] = 0;
		m_diangang[i] = 0;
		m_adiangang[i] = 0;
		m_gangPos[i] = 0;
		m_playerHuInfo[i].Clear();
		m_guoShouHu[i] = false;		// 过手胡限制
		m_gsCard[i].clear();
		m_isPiao[i] = 0;
		m_isFly[i] = false;
	}
	m_desk = NULL;
	m_curOutCard = NULL;
	m_curGetCard = NULL;
	m_beforePos = INVAILD_POS;
	m_beforeType = THINK_OPERATOR_NULL;
	m_zhuangpos = 0;	// 庄开始为0
	m_curPos = INVAILD_POS;
	m_endStartPos = 0;
	m_endStartCurPos = 0;
	m_needGetCard = false;
	m_deskCard.clear();
	m_qiangganghu = 0;
	m_ghostCardReal.clear();
	m_video.Clear();	
	m_gameInfo.Clear();
	m_ghostCardFlop.m_color = 0;
	m_ghostCardFlop.m_number = 0;
}

void GH_HeNan_ZhengZhou::SetDeskPlay()
{
	if (!m_desk || !m_desk->m_vip)
	{
		return;
	}

	for (Lint i = 0; i< DESK_USER_COUNT; ++i)
	{
		m_louHuCard[i] = NULL;
		m_startHuRet[i] = 0;
		m_thinkInfo[i].Reset();
		m_thinkRet[i].Clear();
		m_startHu[i].clear();
		m_handCard[i].clear();
		m_outCard[i].clear();
		m_pengCard[i].clear();
		m_gangCard[i].clear();
		m_abombCard[i].clear();
		m_eatCard[i].clear();
		m_pengInfo[i].clear();
		m_pengGangInfo[i].clear();
		m_angang[i] = 0;
		m_minggang[i] = 0;
		m_diangang[i] = 0;
		m_adiangang[i] = 0;
		m_gangPos[i] = 0;
		m_playerHuInfo[i].Clear();
		m_guoShouHu[i] = false;		// 过手胡限制
		m_gsCard[i].clear();
		m_isPiao[i] = 0;
		m_isFly[i] = false;
	}
	m_curOutCard = NULL;
	m_curGetCard = NULL;
	m_beforePos = INVAILD_POS;
	m_beforeType = THINK_OPERATOR_NULL;
	m_curPos = m_zhuangpos;
	m_endStartPos = 0;
	m_endStartCurPos = 0;
	m_needGetCard = false;
	m_deskCard.clear();
	m_video.Clear();
	m_qiangganghu = 0;
	m_ghostCardReal.clear();
	m_gameInfo.Clear();
	m_ghostCardFlop.m_color = 0;
	m_ghostCardFlop.m_number = 0;

	m_desk->m_vip->SendInfo();

	m_desk->setDeskState(DESK_PLAY);

	// 设置玩法
	m_playtype.setAllTypes( m_desk->getPlayType() );

	// 设置gameinfo
	m_gameInfo.m_playtype = m_playtype;

	if(m_playtype.hasPour())
	{
		BroadcastBottomPour();   //下跑通知
		m_desk->setDeskPlayState(DESK_PLAY_BOTTOM_POUR);
	}
	else
	{
		DeakCard();

		CheckStartPlayCard();
	}

}

void GH_HeNan_ZhengZhou::HanderUserChange( User* pUser, LMsgC2SBottomPour* msg )
{
	if (m_desk == NULL || pUser == NULL || msg == NULL)
	{
		LLOG_ERROR("HanderUserChange NULL ERROR ");
		return;
	}

	Lint pos = m_desk->GetUserPos(pUser);
	if(pos == INVAILD_POS || !m_playtype.hasPour())
	{
		LLOG_ERROR("HanderUserChange pos error %s", pUser->m_userData.m_nike.c_str());
		return;
	}

	if (m_isFly[pos])
	{
		LLOG_ERROR("HanderUserChange pos error %s", pUser->m_userData.m_nike.c_str());
		return;
	}

	m_isFly[pos] = true;
	m_isPiao[pos] = msg->m_nPour;
	LMsgS2CUserBottomPourInfo info;
	info.m_nPos = pos;
	info.m_nPour = msg->m_nPour;
	for (Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		if (i != pos)
		{
			m_desk->m_user[i]->Send(info);
		}
	}

	if (IsPouredOver())
	{
		DeakCard();

		CheckStartPlayCard();
	}
}

void GH_HeNan_ZhengZhou::ProcessRobot(Lint pos, User * pUser)
{
	if (pos < 0 || pos > 3)
	{
		return;
	}

	switch(m_desk->getDeskPlayState())
	{
	case DESK_PLAY_START_HU:
		{
			if (m_startHu[pos].size() > 0)
			{
				LMsgC2SUserStartHuSelect msg;
				msg.m_type = START_HU_NULL;
				m_desk->HanderUserStartHu(pUser, &msg);
				return;
			}
		}
		break;
	case DESK_PLAY_GET_CARD:
		{
			//打出牌去
			if (m_desk->getDeskPlayState() == DESK_PLAY_GET_CARD && m_curPos == pos)
			{
				LMsgC2SUserPlay msg;
				msg.m_thinkInfo.m_type = THINK_OPERATOR_OUT;
				CardValue card;
				card.m_color = m_handCard[pos][0]->m_color;
				card.m_number = m_handCard[pos][0]->m_number;
				msg.m_thinkInfo.m_card.push_back(card);
				m_desk->HanderUserPlayCard(pUser, &msg);
			}
		}
		break;
	case DESK_PLAY_THINK_CARD:
		{
			if (m_thinkInfo[pos].NeedThink())
			{
				for (Lsize i = 0; i < m_thinkInfo[pos].m_thinkData.size(); ++i)
				{
					if (m_thinkInfo[pos].m_thinkData[i].m_type == THINK_OPERATOR_BOMB)
					{
						LMsgC2SUserOper msg;
						msg.m_think.m_type = THINK_OPERATOR_NULL;		//
						CardVector& mCard = m_thinkInfo[pos].m_thinkData[i].m_card;
						for (Lsize j = 0; j <  mCard.size(); ++j)
						{
							CardValue card;
							card.m_color = mCard[j]->m_color;
							card.m_number =  mCard[j]->m_number;
							msg.m_think.m_card.push_back(card) ;
						}
						m_desk->HanderUserOperCard(pUser, &msg);
						return;
					}
					else
					{
						LMsgC2SUserOper msg;
						msg.m_think.m_type = m_thinkInfo[pos].m_thinkData[i].m_type;
						CardVector& mCard = m_thinkInfo[pos].m_thinkData[i].m_card;
						for (Lsize j = 0; j <  mCard.size(); ++j)
						{
							CardValue card;
							card.m_color = mCard[j]->m_color;
							card.m_number =  mCard[j]->m_number;
							msg.m_think.m_card.push_back(card) ;
						}
						m_desk->HanderUserOperCard(pUser, &msg);
					}
				}
			}
		}
		break;
	case DESK_PLAY_END_CARD:
		{
			if (m_desk->getDeskPlayState() == DESK_PLAY_END_CARD && m_curPos == pos)
			{
				LMsgC2SUserEndCardSelect msg;
				msg.m_flag = 0;
				m_desk->HanderUserEndSelect(pUser, &msg);
				return;
			}
		}
		break;
	case DESK_PLAY_BOTTOM_POUR:
		{
			// TODO: 下跑
			LMsgC2SBottomPour msg;
			msg.m_nPour = 2;	
			m_desk->HanderUserChange(pUser, &msg);
		}
		break;
	default:
		{

		} break;
	}
}

void GH_HeNan_ZhengZhou::HanderUserSpeak( User* pUser, LMsgC2SUserSpeak* msg )
{
	if(gConfig.GetDebugModel())
	{
		std::vector<Lstring> des;
		L_ParseString(msg->m_msg, des, " ");
		if (des.size() == 3)
		{
			if (des[0] == "#give")
			{
				for (auto itr=m_deskCard.begin(); itr!=m_deskCard.end(); ++itr)
				{
					if ((*itr)->m_number == atoi(des[1].c_str()) && (*itr)->m_color == atoi(des[2].c_str()))
					{
						std::swap((*itr),m_deskCard.back());
						break;
					}
				}
			}
		}
	}
}

void GH_HeNan_ZhengZhou::HanderUserPlayCard(User* pUser,LMsgC2SUserPlay* msg)
{
	if (m_desk == NULL || pUser == NULL || msg == NULL)
	{
		LLOG_DEBUG("HanderUserEndSelect NULL ERROR ");
		return;
	}
	LMsgS2CUserPlay sendMsg;
	sendMsg.m_errorCode = 0;
	sendMsg.m_pos = m_curPos;
	sendMsg.m_card = msg->m_thinkInfo;

	Lint pos = m_desk->GetUserPos(pUser);
	if(pos == INVAILD_POS)
	{
		//pUser->Send(sendMsg);
		LLOG_DEBUG("HanderUserPlayCard pos error %s", pUser->m_userData.m_nike.c_str());
		return;
	}

	if(pos != m_curPos)
	{
		pUser->Send(sendMsg);
		LLOG_ERROR("HanderUserPlayCard not my pos %d:%d", pos, m_curPos);
		return;
	}

	if(msg->m_thinkInfo.m_type == THINK_OPERATOR_OUT)
	{
		if(msg->m_thinkInfo.m_card.size())
		{
			for (Lsize i = 0; i < m_handCard[pos].size(); ++i)
			{
				if (m_handCard[pos][i]->m_color == msg->m_thinkInfo.m_card[0].m_color && m_handCard[pos][i]->m_number == msg->m_thinkInfo.m_card[0].m_number)
				{
					m_curOutCard = m_handCard[pos][i];
					gCB_HeNan_ZhengZhou->EraseCard(m_handCard[pos], m_curOutCard);
					m_desk->BoadCast(sendMsg);
					m_beforePos = pos;
					m_beforeType = THINK_OPERATOR_OUT;

					//录像
					std::vector<CardValue> cards;
					CardValue card;
					card.m_color = m_curOutCard->m_color;
					card.m_number = m_curOutCard->m_number;
					cards.push_back(card);
					m_video.AddOper(VIDEO_OPER_OUT_CARD, pos, cards);

					//这里玩家思考
					SetThinkIng();
					break;
				}
			}
		}
		return;
	}


	ThinkUnit* unit = NULL;
	for(Lsize i = 0 ; i < m_thinkInfo[pos].m_thinkData.size(); ++i)
	{
		if(msg->m_thinkInfo.m_type == m_thinkInfo[pos].m_thinkData[i].m_type)
		{
			if(msg->m_thinkInfo.m_card.size() == m_thinkInfo[pos].m_thinkData[i].m_card.size())
			{
				bool find = true;
				for(Lsize j = 0 ; j < msg->m_thinkInfo.m_card.size() ; ++j)
				{
					if(msg->m_thinkInfo.m_card[j].m_color != m_thinkInfo[pos].m_thinkData[i].m_card[j]->m_color ||
						msg->m_thinkInfo.m_card[j].m_number != m_thinkInfo[pos].m_thinkData[i].m_card[j]->m_number)
					{
						find = false;
						break;
					}
				}

				if(find)
				{
					unit = &m_thinkInfo[pos].m_thinkData[i];
					break;
				}
			}
		}
	}

	if(unit)
	{
		if(unit->m_type == THINK_OPERATOR_BOMB)
		{
			//录相;
			VideoDoing(unit->m_type,pos,0,0);

			if (m_curGetCard)
			{
				gCB_HeNan_ZhengZhou->EraseCard(m_handCard[pos], m_curGetCard);
				sendMsg.m_huCard.m_color = m_curGetCard->m_color;
				sendMsg.m_huCard.m_number = m_curGetCard->m_number;
			}
			sendMsg.m_hu = unit->m_hu;
			sendMsg.m_cardCount = m_handCard[pos].size();
			for (Lint i = 0; i < sendMsg.m_cardCount; ++i)
			{
				CardValue mCard;
				mCard.m_color = m_handCard[pos][i]->m_color;
				mCard.m_number = m_handCard[pos][i]->m_number;
				sendMsg.m_cardValue.push_back(mCard);
			}
			m_desk->BoadCast(sendMsg);

			m_thinkRet[m_curPos] = m_thinkInfo[m_curPos].m_thinkData[0];
			m_playerHuInfo[m_curPos].type = CheckIfBigHu(m_thinkRet[m_curPos].m_hu)? WIN_SUB_DZIMO : WIN_SUB_ZIMO;

			//录像
			if ( m_curGetCard )
			{
				CardValue curGetCard;
				curGetCard.m_color = m_curGetCard->m_color;
				curGetCard.m_number = m_curGetCard->m_number;
				m_playerHuInfo[m_curPos].wincards.push_back(curGetCard);
			}
			m_video.AddOper( VIDEO_OPER_ZIMO, m_curPos, m_playerHuInfo[m_curPos].wincards );

			OnGameOver(WIN_ZIMO, INVAILD_POS);
		}
		else if(unit->m_type == THINK_OPERATOR_AGANG)
		{
			//录相;
			VideoDoing(unit->m_type,pos,unit->m_card[0]->m_color,unit->m_card[0]->m_number);
			//
			gCB_HeNan_ZhengZhou->EraseCard(m_handCard[pos], unit->m_card[0],4);	
			m_desk->BoadCast(sendMsg);
			m_angang[pos] += 1;

			//录像
			std::vector<CardValue> cards;
			for (int i = 0; i < 4; ++i)
			{
				CardValue card;
				card.m_color = unit->m_card[0]->m_color;
				card.m_number = unit->m_card[0]->m_number;
				cards.push_back(card);
				m_abombCard[pos].push_back(unit->m_card[0]);
			}
			m_video.AddOper(VIDEO_OPER_AN_GANG, pos, cards);
			m_beforeType = THINK_OPERATOR_AGANG;
			SetPlayIng(pos, true, true, true, true);
		}
		else if(unit->m_type == THINK_OPERATOR_MGANG)
		{
			//录相;
			VideoDoing(unit->m_type,pos,unit->m_card[0]->m_color,unit->m_card[0]->m_number);
			//
			m_curOutCard = unit->m_card[0];
			gCB_HeNan_ZhengZhou->EraseCard(m_handCard[pos], unit->m_card[0],1);	

			//这里玩家思考
			m_beforePos = pos;
			m_beforeType = THINK_OPERATOR_MGANG;
			SetThinkIng();
		}
		else if(unit->m_type == THINK_OPERATOR_ABU)
		{
			//录相;
			VideoDoing(unit->m_type,pos,unit->m_card[0]->m_color,unit->m_card[0]->m_number);
			//
			gCardMgr->EraseCard(m_handCard[pos], unit->m_card[0],4);	
			m_desk->BoadCast(sendMsg);
			m_angang[pos] += 1;
			//录像
			std::vector<CardValue> cards;
			for (int i = 0; i < 4; ++i)
			{
				CardValue card;
				card.m_color = unit->m_card[0]->m_color;
				card.m_number = unit->m_card[0]->m_number;
				cards.push_back(card);
				m_abombCard[pos].push_back(unit->m_card[0]);
			}
			m_video.AddOper(VIDEO_OPER_AN_BU, pos, cards);
			//这里玩家思考
			m_beforePos = pos;
			m_beforeType = THINK_OPERATOR_ABU;
			SetPlayIng(pos,true, false, true, true);
		}
		else if(unit->m_type == THINK_OPERATOR_MBU)
		{
			//录相;
			VideoDoing(unit->m_type,pos,unit->m_card[0]->m_color,unit->m_card[0]->m_number);
			//
			m_curOutCard = unit->m_card[0];
			gCB_HeNan_ZhengZhou->EraseCard(m_handCard[pos], unit->m_card[0],1);
			//m_desk->BoadCast(sendMsg);	

			//这里玩家思考
			m_beforePos = pos;
			m_beforeType = THINK_OPERATOR_MBU;
			SetThinkIng();
		}
		//我们的thinkinfo中不会有THINK_OPERATOR_NULL，所以不会有unit，不会走到这一步
		// 			else if(unit->m_type == THINK_OPERATOR_NULL)
		// 			{
		// 				//录相;
		// 				VideoDoing(unit->m_type,pos,0,0);
		// 				//
		// 			}
	}
	else
	{
		if(msg->m_thinkInfo.m_type == THINK_OPERATOR_NULL)
		{
			// 检查玩家是否能胡 能胡添加上过手胡限制
			for(Lsize i = 0 ; i < m_thinkInfo[pos].m_thinkData.size(); ++i)
			{
				if(m_thinkInfo[pos].m_thinkData[i].m_type == THINK_OPERATOR_BOMB)
				{
					m_guoShouHu[pos] = true;
				}
			}
			//录相;
			VideoDoing(msg->m_thinkInfo.m_type,pos,0,0);
		}
		LLOG_DEBUG("Desk::HanderUserPlayCard %s,%d", pUser->m_userData.m_nike.c_str(), msg->m_thinkInfo.m_type);
	}
}

void GH_HeNan_ZhengZhou::HanderUserOperCard(User* pUser,LMsgC2SUserOper* msg)
{
	LMsgS2CUserOper sendMsg;
	sendMsg.m_pos = m_curPos;
	sendMsg.m_think = msg->m_think;

	Lint pos = m_desk->GetUserPos(pUser);
	if (pos == INVAILD_POS || !m_thinkInfo[pos].NeedThink())
	{
		sendMsg.m_errorCode = 1;
		pUser->Send(sendMsg);
		LLOG_DEBUG("HanderUserEndSelect pos error pos: %d     curPos:%d", pos, m_endStartCurPos);
		return;
	}

	bool find = false;
	for(Lsize i = 0 ; i < m_thinkInfo[pos].m_thinkData.size(); ++i)
	{
		if(m_thinkInfo[pos].m_thinkData[i].m_type == msg->m_think.m_type)
		{
			bool check = true;
			if(m_thinkInfo[pos].m_thinkData[i].m_card.size() == msg->m_think.m_card.size())
			{
				for(Lsize j = 0 ; j < msg->m_think.m_card.size(); ++j)
				{
					if(msg->m_think.m_card[j].m_color != m_thinkInfo[pos].m_thinkData[i].m_card[j]->m_color || 
						msg->m_think.m_card[j].m_number != m_thinkInfo[pos].m_thinkData[i].m_card[j]->m_number)
					{
						check = false;
						break;
					}
				}

				if(check)
				{
					m_thinkRet[pos] = m_thinkInfo[pos].m_thinkData[i];
					find = true;
					break;
				}
			}
		}
	}

	if (!find)
	{
		m_thinkRet[pos].m_type = THINK_OPERATOR_NULL;
		msg->m_think.m_type = THINK_OPERATOR_NULL;
	}

	//录相;
	VideoDoing(msg->m_think.m_type,pos,(msg->m_think.m_card.size()>0)?msg->m_think.m_card[0].m_color:0,(msg->m_think.m_card.size()>0)?msg->m_think.m_card[0].m_number:0);

	if (msg->m_think.m_type == THINK_OPERATOR_BOMB)
	{
		LMsgS2CUserOper send;
		send.m_pos = pos;
		send.m_errorCode = 0;
		send.m_think = msg->m_think;
		send.m_card.m_color = (m_curOutCard == NULL)? 1 : m_curOutCard->m_color;		//临时的 有错误
		send.m_card.m_number =(m_curOutCard == NULL)? 1 : m_curOutCard->m_number;		//临时的 有错误
		send.m_hu = m_thinkRet[pos].m_hu;
		send.m_cardCount = m_handCard[pos].size();
		for (Lint i = 0; i < send.m_cardCount; ++i)
		{
			CardValue mCard;
			mCard.m_color = m_handCard[pos][i]->m_color;
			mCard.m_number = m_handCard[pos][i]->m_number;
			send.m_cardValue.push_back(mCard);
		}
		m_desk->BoadCast(send);

		// 清除其他玩家的碰杠数据 其他玩家只能胡和过
		for(Lint i = 0; i < DESK_USER_COUNT; ++i)
		{
			if ( i != pos )
			{
				for(auto itThinkData = m_thinkInfo[i].m_thinkData.begin(); itThinkData != m_thinkInfo[i].m_thinkData.end();)
				{
					if(itThinkData->m_type != THINK_OPERATOR_BOMB)
					{
						itThinkData = m_thinkInfo[i].m_thinkData.erase(itThinkData);
					}
					else
					{
						itThinkData++;
					}
				}
			}
		}
	}

	// 过手胡
	if (msg->m_think.m_type == THINK_OPERATOR_NULL)
	{
		for(Lsize i = 0 ; i < m_thinkInfo[pos].m_thinkData.size(); ++i)
		{
			if(m_thinkInfo[pos].m_thinkData[i].m_type == THINK_OPERATOR_BOMB)
			{
				m_guoShouHu[pos] = true;
			}
		}
	}
	else if (msg->m_think.m_type == THINK_OPERATOR_PENG ||
		msg->m_think.m_type == THINK_OPERATOR_AGANG ||
		msg->m_think.m_type == THINK_OPERATOR_MGANG )
	{
		m_guoShouHu[pos] = false;
	}

	//设置以及思考过了
	m_thinkInfo[pos].Reset();

	CheckThink();
}

void GH_HeNan_ZhengZhou::OnUserReconnect(User* pUser)
{
	if (pUser == NULL || m_desk == NULL)
	{
		return;
	}
	Lint pos = m_desk->GetUserPos(pUser);
	if (pos == INVAILD_POS)
	{
		LLOG_ERROR("Desk::OnUserReconnect pos error %d", pUser->GetUserDataId());
		return;
	}
	Lint nCurPos = m_curPos;
	Lint nDeskPlayType = m_desk->getDeskPlayState();

	LMsgS2CAllUserBottomPourInfo BottomPourInfo;
	memcpy(&BottomPourInfo.m_bPoured, m_isFly, sizeof(m_isFly));
	memcpy(&BottomPourInfo.m_nPour, m_isPiao, sizeof(m_isPiao));
	for (Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		m_desk->m_user[i]->Send(BottomPourInfo);
	}

	LMsgS2CDeskState reconn;
	reconn.m_state = m_desk->getDeskState();
	reconn.m_pos = nCurPos;
	reconn.m_time = 15;
	reconn.m_zhuang = m_zhuangpos;
	reconn.m_myPos = pos;

	reconn.m_ghostCard.m_color = m_ghostCardFlop.m_color;
	reconn.m_ghostCard.m_number = m_ghostCardFlop.m_number;

	if(nCurPos != pos)
	{
		reconn.m_flag = 0;
	}
	else
	{
		reconn.m_flag = 1;			//不知道对不对
	}
	reconn.m_dCount = m_deskCard.size();
	for(Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		reconn.m_cardCount[i] = m_handCard[i].size();
		reconn.m_oCount[i] = m_outCard[i].size();
		reconn.m_aCount[i] = m_abombCard[i].size();
		reconn.m_mCount[i] = m_gangCard[i].size();
		reconn.m_pCount[i] = m_pengCard[i].size();
		reconn.m_eCount[i] = m_eatCard[i].size();

		for(Lsize j = 0 ; j < m_outCard[i].size(); ++j)
		{
			reconn.m_oCard[i][j].m_color = m_outCard[i][j]->m_color;
			reconn.m_oCard[i][j].m_number = m_outCard[i][j]->m_number;
		}

		for(Lsize j = 0 ; j < m_abombCard[i].size(); ++j)
		{
			reconn.m_aCard[i][j].m_color = m_abombCard[i][j]->m_color;
			reconn.m_aCard[i][j].m_number = m_abombCard[i][j]->m_number;
		}

		for(Lsize j = 0 ; j < m_gangCard[i].size(); ++j)
		{
			reconn.m_mCard[i][j].m_color = m_gangCard[i][j]->m_color;
			reconn.m_mCard[i][j].m_number = m_gangCard[i][j]->m_number;
		}

		for(Lsize j = 0 ; j < m_pengCard[i].size(); ++j)
		{
			reconn.m_pCard[i][j].m_color = m_pengCard[i][j]->m_color;
			reconn.m_pCard[i][j].m_number = m_pengCard[i][j]->m_number;
		}

		for(Lsize j = 0 ; j < m_eatCard[i].size(); ++j)
		{
			reconn.m_eCard[i][j].m_color = m_eatCard[i][j]->m_color;
			reconn.m_eCard[i][j].m_number = m_eatCard[i][j]->m_number;
		}
	}

	//我的牌,客户的重连，之前莫得牌的重新拿出来发给他
	if(nDeskPlayType == DESK_PLAY_GET_CARD && m_needGetCard && pos == nCurPos)
	{
		CardVector tmp = m_handCard[pos];
		if (m_curGetCard)
		{
			reconn.m_cardCount[pos] -= 1;
			gCB_HeNan_ZhengZhou->EraseCard(tmp,m_curGetCard);
		}
		for(Lsize j = 0 ;j < tmp.size(); ++j)
		{
			reconn.m_cardValue[j].m_color = tmp[j]->m_color;
			reconn.m_cardValue[j].m_number = tmp[j]->m_number;
		}
	}
	else
	{
		for(Lsize j = 0 ;j < m_handCard[pos].size(); ++j)
		{
			reconn.m_cardValue[j].m_color = m_handCard[pos][j]->m_color;
			reconn.m_cardValue[j].m_number = m_handCard[pos][j]->m_number;
		}
	}

	//该出牌的玩家，多发一张牌，用于打出去。
	if(m_needGetCard && nDeskPlayType == DESK_PLAY_THINK_CARD)
	{
		if (m_curOutCard && pos != nCurPos)
		{
			reconn.m_cardCount[nCurPos] ++;
		}
	}
	pUser->Send(reconn);

	//我思考
	if(nDeskPlayType == DESK_PLAY_THINK_CARD )
	{
		if (m_thinkInfo[pos].NeedThink())
		{
			LMsgS2CThink think;
			think.m_time = 15;
			think.m_flag = 1;
			think.m_card.m_color = (m_curOutCard == NULL)? 1 : m_curOutCard->m_color;		//临时的 有错误 m_curOutCard->m_color;
			think.m_card.m_number = (m_curOutCard == NULL)? 1 : m_curOutCard->m_number;		//临时的 有错误m_curOutCard->m_number;
			for (Lsize j = 0; j < m_thinkInfo[pos].m_thinkData.size(); ++j)
			{		
				ThinkData info;
				info.m_type = m_thinkInfo[pos].m_thinkData[j].m_type;
				for(Lsize n = 0 ; n < m_thinkInfo[pos].m_thinkData[j].m_card.size(); ++n)
				{
					CardValue v;
					v.m_color = m_thinkInfo[pos].m_thinkData[j].m_card[n]->m_color;
					v.m_number = m_thinkInfo[pos].m_thinkData[j].m_card[n]->m_number;
					info.m_card.push_back(v);
				}
				think.m_think.push_back(info);
			}
			pUser->Send(think);
		}
	}

	///海底牌
	if(nDeskPlayType == DESK_PLAY_END_CARD && pos == m_endStartCurPos)
	{
		LMsgS2CUserEndCardSelect msg;
		msg.m_pos = pos;
		msg.m_time = 15;
		pUser->Send(msg);
	}

	//我出牌
	if(nDeskPlayType == DESK_PLAY_GET_CARD && m_needGetCard && pos == nCurPos)
	{
		LMsgS2COutCard msg;
		msg.m_time = 15;
		msg.m_pos = pos;
		msg.m_deskCard = (Lint)m_deskCard.size();
		msg.m_flag = (m_curGetCard&&m_needGetCard)?0:1;
		msg.m_gang = m_gangPos[pos];
		msg.m_end = m_deskCard.size()==1?1:0;
		if(m_needGetCard && m_curGetCard)
		{
			msg.m_curCard.m_color = m_curGetCard->m_color;
			msg.m_curCard.m_number = m_curGetCard->m_number;
		}
		for (Lsize j = 0; j < m_thinkInfo[pos].m_thinkData.size(); ++j)
		{		
			ThinkData info;
			info.m_type = m_thinkInfo[pos].m_thinkData[j].m_type;
			for(Lsize n = 0 ; n < m_thinkInfo[pos].m_thinkData[j].m_card.size(); ++n)
			{
				CardValue v;
				v.m_color = m_thinkInfo[pos].m_thinkData[j].m_card[n]->m_color;
				v.m_number = m_thinkInfo[pos].m_thinkData[j].m_card[n]->m_number;
				info.m_card.push_back(v);
			}
			msg.m_think.push_back(info);
		}
		pUser->Send(msg);
	}

	//桌面上的牌重新发给玩家的桌牌
	if(m_needGetCard && nDeskPlayType == DESK_PLAY_THINK_CARD)
	{
		if (m_curOutCard)
		{
			LMsgS2CUserPlay sendMsg;
			sendMsg.m_errorCode = 0;
			sendMsg.m_pos = nCurPos;
			sendMsg.m_card.m_type = THINK_OPERATOR_REOUT;
			CardValue card;
			card.m_color = m_curOutCard->m_color;
			card.m_number = m_curOutCard->m_number;
			sendMsg.m_card.m_card.push_back(card);

			if(m_qiangganghu)
			{
				//sendMsg.m_qiangGangflag = 1;
			}

			pUser->Send(sendMsg);
		}
	}
}

void GH_HeNan_ZhengZhou::OnGameOver(Lint result,Lint bombpos)
{
	if (m_desk == NULL || m_desk->m_vip == NULL)
	{
		LLOG_DEBUG("OnGameOver NULL ERROR ");
		return;
	}
	//计算输赢结果
	Lint gold[4]={0};
	Lint bombCount = 0;
	Lint winPos[4] = {0};
	Lint mgang[4] = {0};
	//计算分数
	if (WIN_DISS != result && WIN_NONE != result)//自摸或者点炮
	{
		calcUserScore(result, gold, bombCount, winPos);
		for(Lint i = 0 ; i < DESK_USER_COUNT; ++i)
		{
			mgang[i] += m_minggang[i];
			mgang[i] += m_diangang[i];
		}
	}
	else if(WIN_NONE == result)//荒庄
	{
		if(m_playtype.hasHuangzhuang())//是否不慌杠计算杠分
		{
			GetGangSocore(gold);

			for(Lint i = 0 ; i < DESK_USER_COUNT; ++i)
			{
				mgang[i] += m_minggang[i];
				mgang[i] += m_diangang[i];
			}
		}
	}
	Lint zhuangPos = m_zhuangpos;
	//计算庄
	if (result == WIN_ZIMO)
	{
		m_zhuangpos = m_curPos;

		for(Lint i = 0 ; i < DESK_USER_COUNT; ++i)
		{
			if(winPos[i] == 1)
			{
				m_zhuangpos = i;	
				break;
			}
		}
	}
	else if (result == WIN_BOMB)
	{
		//winpos[m_beforePos] = WIN_SUB_ABOMB;		//置点炮位 带完善这里
		if (bombCount > 1)
			m_zhuangpos = bombpos;
		else
		{
			m_zhuangpos = m_curPos;

			for(Lint i = 0 ; i < DESK_USER_COUNT; ++i)
			{
				if(winPos[i] == 1)
				{
					m_zhuangpos = i;	
					break;
				}
			}
		}
	}
	else
	{
		m_zhuangpos = m_curPos;
		//录像
		std::vector<CardValue> cards;
		if ( result == WIN_NONE && m_video.m_time > 0 )
		{
			m_video.AddOper(VIDEO_OPER_HUANGZHUANG, m_curPos, cards);
		}
		else if (result == WIN_DISS && m_video.m_time > 0)
		{
			m_video.AddOper(VIDEO_OPER_DISS, m_curPos, cards);
		}
	}

	//保存录像
	LMsgL2LDBSaveVideo video;
	m_video.m_Id = gVipLogMgr.GetVideoId();
	if (m_video.m_time > 0)
	{
		m_video.m_playType = m_desk->getPlayType();

		video.m_userId = m_desk->m_user[0]->m_userData.m_id;
		video.m_type = 0;
		video.m_sql = m_video.GetInsertSql();
		gWork.SendMsgToDb(video);
	}
	
	//广播结果
	LMsgS2CGameOver over;
	over.m_result = result;
	for ( int i = 0; i < DESK_USER_COUNT; ++i )
	{
		over.m_win[i] = m_playerHuInfo[i].type;
		over.m_hucards[i] = m_playerHuInfo[i].wincards;	
	}
	// 点炮
	for ( int i = 0; i < DESK_USER_COUNT; ++i )
	{
		if ( m_playerHuInfo[i].type == WIN_SUB_BOMB && m_playerHuInfo[i].abombpos != INVAILD_POS )
		{
			over.m_win[m_playerHuInfo[i].abombpos] = WIN_SUB_ABOMB;
		}

		over.m_fanShu[i] = m_pengGangInfo[i].size();//碰杠

		std::vector<Lint> pengPosVect = m_pengGangInfo[i];
		for (size_t k = 0; k < pengPosVect.size(); ++k)		
		{
			Lint pengPos = pengPosVect[k];
			if ((pengPos >= 0) && (pengPos < DESK_USER_COUNT))
			{
				m_adiangang[pengPos] += 1;//加上被碰杠的数量
			}
		}

	}

	memcpy(over.m_score, gold, sizeof(over.m_score));
	memcpy(over.m_agang, m_angang, sizeof(over.m_agang));//暗杠
	memcpy(over.m_mgang, m_diangang, sizeof(over.m_mgang));//明杠
	memcpy(over.m_dgang, m_adiangang, sizeof(over.m_dgang));//点杠
	//memset(over.m_gsgang,0,sizeof(over.m_gsgang));
	//memcpy(over.m_mbgang, m_minggang, sizeof(over.m_mbgang));
	memcpy(over.m_startHu, m_startHuRet, sizeof(over.m_startHu));

	// 计算过手杠
	for(int i = 0; i < 4; ++i)
	{
		Lint count = 0;
		for(Lsize j = 0; j < m_gangCard[i].size(); j+=4)
		{
			if(isGuoHandGang(m_gangCard[i][j],i))
			{
				++count;
			}
		}
	//	over.m_gsgang[i] = count;
	//	over.m_mbgang[i] -= count;
	}

	/*for(Lsize i = 0; i < m_desk->m_vip->m_playtype.size(); ++i)
	{
	over.m_playType.push_back(m_desk->m_vip->m_playtype[i]);
	}*/

	for (Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		over.m_count[i] = m_handCard[i].size();

		//手牌
		for (Lint j = 0; j < over.m_count[i]; ++j)
		{
			over.m_card[i][j].m_color = m_handCard[i][j]->m_color;
			over.m_card[i][j].m_number = m_handCard[i][j]->m_number;
		}

		//胡牌类型
		if (m_thinkRet[i].m_type == THINK_OPERATOR_BOMB)
		{
			for (Lsize k = 0; k < m_thinkRet[i].m_hu.size(); ++k)
			{
				over.m_hu[i].push_back(m_thinkRet[i].m_hu[k]);
			}
		}
	}

	CardValue CardFlop;
	CardFlop.m_color = m_ghostCardFlop.m_color;
	CardFlop.m_number = m_ghostCardFlop.m_number;
	over.m_ghostCard.push_back(CardFlop);

	m_desk->SetDeskWait();

	//保存结果
	Lint m_checkTing[DESK_USER_COUNT] = {0};
	m_desk->m_vip->AddLog(m_desk->m_user, gold, m_playerHuInfo, zhuangPos, m_angang, mgang, m_video.m_Id, m_checkTing);

	//是否最后一局
	over.m_end = m_desk->m_vip->isEnd()?1:0;
	m_desk->BoadCast(over);

	m_desk->HanderGameOver(result);

	//////////////////////////////////////////////////////////////////////////
}

void GH_HeNan_ZhengZhou::DeakCard()
{		
	if (!m_desk || !m_desk->m_vip)
	{
		return;
	}

	//发牌   
// 	if(gConfig.GetDebugModel() && (m_desk->m_specialCard[0].m_color > 0 && m_desk->m_specialCard[0].m_number > 0))   //玩家指定发牌 牌局
// 	{
// 		gCB_HeNan_ZhengZhou->DealCard2(m_handCard[0],m_handCard[1],m_handCard[2],m_handCard[3],m_deskCard,m_desk->m_specialCard, m_playtype.gameNeedWind());
// 	}
// 	else                //正常随机发牌 牌局
// 	{
// 		gCB_HeNan_ZhengZhou->DealCard(m_handCard[0],m_handCard[1],m_handCard[2],m_handCard[3],m_deskCard, m_playtype.gameNeedWind());
// 	}
	
	Card* special = gConfig.GetDebugModel() && m_desk->m_specialCard[0].isValid() ? m_desk->m_specialCard : nullptr;
	gCB_HeNan_ZhengZhou->DealCard(m_handCard[0],m_handCard[1],m_handCard[2],m_handCard[3],m_deskCard, special, m_playtype.gameNeedWind());

	//翻混牌
	InitHunCard();

	//庄家多发一张牌
	Card* newCard = m_deskCard.back();
	m_handCard[m_curPos].push_back(newCard);
	m_deskCard.pop_back();
	gCB_HeNan_ZhengZhou->SortCard(m_handCard[m_curPos]);

	//发送消息给客户端
	for(Lint i = 0 ; i < DESK_USER_COUNT; ++i)
	{
		if(m_desk->m_user[i] != NULL)
		{
			LMsgS2CPlayStart msg;
			msg.m_zhuang = m_curPos;
			msg.m_pos = i;

			for(Lsize j = 0 ; j < m_handCard[i].size(); ++j)
			{
				msg.m_cardValue[j].m_number = m_handCard[i][j]->m_number;
				msg.m_cardValue[j].m_color = m_handCard[i][j]->m_color;
			}

			for(Lint j = 0 ; j < DESK_USER_COUNT; ++j)
			{
				msg.m_cardCount[j] = m_handCard[j].size();
			}
			msg.m_dCount = (Lint)m_deskCard.size();
			m_desk->m_user[i]->Send(msg);
		}
	}

	//录像功能
	Lint id[4];
	Lint score[4];
	std::vector<CardValue> vec[4];
	for (Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		id[i] = m_desk->m_user[i]->GetUserDataId();
		score[i] = m_desk->m_vip->GetUserScore(m_desk->m_user[i]);
		for (Lsize j = 0; j < m_handCard[i].size(); ++j)
		{
			CardValue v;
			v.m_color = m_handCard[i][j]->m_color;
			v.m_number = m_handCard[i][j]->m_number;
			vec[i].push_back(v);
		}
	}
	m_video.DealCard(id, vec,gWork.GetCurTime().Secs(),m_zhuangpos,score,m_desk->GetDeskId(),m_desk->m_vip->m_curCircle,m_desk->m_vip->m_maxCircle,m_desk->getGameType(), m_desk->getPlayType());
}

void GH_HeNan_ZhengZhou::CheckStartPlayCard()
{
	m_desk->setDeskPlayState(DESK_PLAY_GET_CARD);
	SetPlayIng(m_curPos,false, false, true, true);
	m_curGetCard = m_handCard[m_curPos].back();
	m_needGetCard = true;
}
//摸牌
void GH_HeNan_ZhengZhou::SetPlayIng(Lint pos,bool needGetCard,bool gang, bool needThink, bool canhu)
{
	if (m_desk == NULL )
	{
		LLOG_ERROR("SetPlayIng NULL ERROR ");
		return;
	}
	if (pos < 0 || pos >= INVAILD_POS)
	{
		LLOG_ERROR("SetPlayIng Pos Error ! %d ", pos);
		return;
	}
	//穿庄
	if(needGetCard && m_deskCard.empty())
	{
		LLOG_INFO("Desk::SetPlayIng huangzhuang game over");
		OnGameOver(WIN_NONE, INVAILD_POS);
		return;
	}
	m_curPos = pos;

	// 取消过手胡限制D
	m_guoShouHu[pos] = false;

	//我摸牌思考信息
	m_thinkInfo[pos].m_thinkData.clear();
	m_desk->setDeskPlayState(DESK_PLAY_GET_CARD);
	m_needGetCard = false;
	if (needGetCard)
	{
		m_needGetCard = true;
		m_curGetCard = m_deskCard.back();
		m_deskCard.pop_back();
		//录像
		std::vector<CardValue> cards;
		CardValue card;
		card.m_color = m_curGetCard->m_color;
		card.m_number = m_curGetCard->m_number;
		cards.push_back(card);
		m_video.AddOper(VIDEO_OPER_GET_CARD, pos, cards);
	}
	if (needThink)
	{
		m_gameInfo.m_pGscard = NULL;
		m_gameInfo. b_canHu = canhu;		// 是否可以胡
		m_gameInfo. b_onlyHu = m_gangPos[pos];
		m_gameInfo. m_thinkGang = false;	// 单独处理是不是杠的牌
		m_gameInfo. m_deskState = m_desk->getDeskPlayState();	// 当前局牌状态
		m_gameInfo. m_playerPos = m_curPos;	// 当前一个出牌位置
		m_gameInfo. m_cardState = THINK_OPERATOR_NULL;	// 当前一个出牌状态
		m_gameInfo. m_endStartPos = m_endStartPos;	// 结束位置，即海底位置
		m_gameInfo. m_MePos = pos;		// 玩家的位置
		m_gameInfo.bNoCard = m_deskCard.empty();
		//m_gameInfo.m_playtype = m_playtype;
		m_thinkInfo[pos].m_thinkData = gCB_HeNan_ZhengZhou->CheckGetCardOperator(m_handCard[pos],m_pengCard[pos],m_abombCard[pos],m_gangCard[pos],m_eatCard[pos],m_curGetCard,m_gameInfo);
		// 记录过手杠
		if(m_gameInfo.m_pGscard)
		{
			m_gsCard[pos].push_back(m_gameInfo.m_pGscard);
		}
		VideoThink(pos);
	}
	if (m_needGetCard)
	{
		m_handCard[pos].push_back(m_curGetCard);
		gCB_HeNan_ZhengZhou->SortCard(m_handCard[pos]);
	}

	for(Lint i = 0 ; i < DESK_USER_COUNT; ++i)
	{
		if(m_desk->m_user[i] != NULL)
		{
			LMsgS2COutCard msg;
			msg.m_time = 15;
			msg.m_pos = pos;
			msg.m_deskCard = (Lint)m_deskCard.size();
			msg.m_gang = m_gangPos[pos];
			msg.m_end = m_desk->getDeskPlayState()==DESK_PLAY_END_CARD?1:0;
			msg.m_flag = 1;
			if(m_needGetCard)
			{
				msg.m_flag = 0;
			}

			if(pos == i)
			{
				if(m_needGetCard)
				{
					msg.m_curCard.m_number = m_curGetCard->m_number;
					msg.m_curCard.m_color = m_curGetCard->m_color;
				}

				for (Lsize j = 0; j < m_thinkInfo[pos].m_thinkData.size(); ++j)
				{		
					ThinkData info;
					info.m_type = m_thinkInfo[pos].m_thinkData[j].m_type;
					for(Lsize n = 0 ; n < m_thinkInfo[pos].m_thinkData[j].m_card.size(); ++n)
					{
						CardValue v;
						v.m_color = m_thinkInfo[pos].m_thinkData[j].m_card[n]->m_color;
						v.m_number = m_thinkInfo[pos].m_thinkData[j].m_card[n]->m_number;
						info.m_card.push_back(v);
					}
					msg.m_think.push_back(info);
				}
			}

			m_desk->m_user[i]->Send(msg);
		}
	}
}
// 计算玩家得分
void GH_HeNan_ZhengZhou::calcUserScore(Lint result, Lint gold[], Lint& bombCount, Lint winPos[])
{
	//计算杠牌得分
	GetGangSocore(gold);

	GetPaoSocore(result, gold, bombCount, winPos);
}

void GH_HeNan_ZhengZhou::SetThinkIng()
{
	if (!m_desk)
	{
		LLOG_ERROR("SetThinkIng m_desk == NULL");
		return;
	}

	for(Lint i = 0 ; i < DESK_USER_COUNT;++i)
	{
		m_thinkRet[i].Clear();
		m_thinkInfo[i].Reset();
		if(i != m_curPos)
		{
			m_gameInfo. b_canHu = ( m_playtype.gametypeDianPao()/* && !m_guoShouHu[i] */);		// 是否可以胡
			m_gameInfo. b_onlyHu = m_gangPos[i] || m_beforeType == THINK_OPERATOR_MGANG || m_beforeType == THINK_OPERATOR_MBU;
			m_gameInfo. m_thinkGang = false;	// 单独处理是不是杠的牌
			m_gameInfo. m_deskState = m_desk->getDeskPlayState();	// 当前局牌状态
			m_gameInfo. m_playerPos = m_curPos;	// 当前一个出牌位置
			m_gameInfo. m_cardState = m_beforeType;	// 当前一个出牌状态
			m_gameInfo. m_endStartPos = m_endStartPos;	// 结束位置，即海底位置
			m_gameInfo. m_MePos = i;		// 玩家的位置
			m_gameInfo.bNoCard = m_deskCard.empty();
			m_thinkInfo[i].m_thinkData = gCB_HeNan_ZhengZhou->CheckOutCardOperator(m_handCard[i],m_pengCard[i],m_abombCard[i],m_gangCard[i],m_eatCard[i],m_curOutCard,m_gameInfo);
		}
	}

	bool think = false;
	for(Lint i = 0 ; i < DESK_USER_COUNT;++i)
	{
		if(i == m_curPos)
		{
			continue;
		}
		if (m_thinkInfo[i].NeedThink())
		{
			think = true;
			VideoThink(i);
		}
	}

	if (think)
	{
		m_desk->setDeskPlayState(DESK_PLAY_THINK_CARD);
		for (Lint i = 0; i < DESK_USER_COUNT; ++i)
		{
			LMsgS2CThink think;
			think.m_time = 15;
			think.m_card.m_color = m_curOutCard->m_color;
			think.m_card.m_number = m_curOutCard->m_number;
			if (m_thinkInfo[i].NeedThink())
			{
				think.m_flag = 1;
				for (Lsize j = 0; j < m_thinkInfo[i].m_thinkData.size(); ++j)
				{		
					ThinkData info;
					info.m_type = m_thinkInfo[i].m_thinkData[j].m_type;
					for(Lsize n = 0 ; n < m_thinkInfo[i].m_thinkData[j].m_card.size(); ++n)
					{
						CardValue v;
						v.m_color = m_thinkInfo[i].m_thinkData[j].m_card[n]->m_color;
						v.m_number = m_thinkInfo[i].m_thinkData[j].m_card[n]->m_number;
						info.m_card.push_back(v);
					}
					think.m_think.push_back(info);
				}
				if(m_beforeType == THINK_OPERATOR_MBU)
				{
					m_qiangganghu = 1;
				}
			}
			else
			{
				think.m_flag = 0;
			}
			m_desk->m_user[i]->Send(think);
		}
	}
	else
	{
		ThinkEnd();
	}
}

void GH_HeNan_ZhengZhou::CheckThink()
{
	bool hu		= false;
	bool Peng	= false;
	bool Chi	= false;
	bool Gang	= false;
	bool Bu		= false;
	bool hu_New		= false;
	bool Peng_New	= false;
	bool Gang_New	= false;
	bool Bu_New		= false;
	for (Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		//
		if (m_thinkRet[i].m_type == THINK_OPERATOR_BOMB)			hu		= true;
		else if (m_thinkRet[i].m_type == THINK_OPERATOR_PENG)		Peng	= true;
		else if (m_thinkRet[i].m_type == THINK_OPERATOR_MGANG)		Gang	= true;
		else if (m_thinkRet[i].m_type == THINK_OPERATOR_MBU)		Bu		= true;

		//
		if(m_thinkInfo[i].NeedThink())
		{
			if (m_thinkInfo[i].HasHu())				hu_New		= true;
			else if (m_thinkInfo[i].HasPeng())		Peng_New	= true;
			else if (m_thinkInfo[i].HasMGang())		Gang_New	= true;
			else if (m_thinkInfo[i].HasMBu())		Bu_New		= true;
		}
	}

	bool think = false;

	if (hu_New)	
		think = true;
	else
	{
		if (!hu)
		{
			if (Peng_New || Gang_New || Bu_New )
				think = true;
		}
	}

	if (!think)
		ThinkEnd();
}

void GH_HeNan_ZhengZhou::ThinkEnd()
{
	for(int i = 0; i < DESK_USER_COUNT; ++i)
	{
		m_thinkInfo[i].Reset();
	}
	Lint huCount = 0;

	Lint pengPos = INVAILD_POS;
	Lint gangPos = INVAILD_POS;
	Lint buPos = INVAILD_POS;

	for (Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		if (m_thinkRet[i].m_type == THINK_OPERATOR_BOMB)
		{
			m_playerHuInfo[i].type = CheckIfBigHu(m_thinkRet[i].m_hu)? WIN_SUB_DBOMB : WIN_SUB_BOMB;
			m_playerHuInfo[i].abombpos = m_beforePos;
			huCount++;
		}

		if (m_thinkRet[i].m_type == THINK_OPERATOR_MGANG)
			gangPos = i;

		if (m_thinkRet[i].m_type == THINK_OPERATOR_MBU)
			buPos = i;

		if (m_thinkRet[i].m_type == THINK_OPERATOR_PENG)
			pengPos = i;
	}

	m_qiangganghu = 0;

	if (huCount != 0)
	{
		//这里有人胡牌
		if (m_beforeType == THINK_OPERATOR_AGANG || m_beforeType == THINK_OPERATOR_MGANG || m_beforeType == THINK_OPERATOR_MBU)
		{
			//录像
			for (int i = 0; i < DESK_USER_COUNT; ++i)
			{
				if (m_thinkRet[i].m_type == THINK_OPERATOR_BOMB)
				{
					for(Lsize n=0;n<m_thinkRet[i].m_card.size();++n)
					{
						CardValue curOutCard;
						curOutCard.m_color = m_thinkRet[i].m_card[n]->m_color;
						curOutCard.m_number = m_thinkRet[i].m_card[n]->m_number;
						m_playerHuInfo[i].wincards.push_back(curOutCard);
					}
					m_video.AddOper(VIDEO_OPER_SHOUPAO, i, m_playerHuInfo[i].wincards);
				}
			}
			OnGameOver(WIN_BOMB, m_beforePos);
		}
		else
		{
			for (int i = 0; i < DESK_USER_COUNT; ++i)
			{
				if (m_thinkRet[i].m_type == THINK_OPERATOR_BOMB)
				{
					std::vector<CardValue> cards;
					CardValue curOutCard;
					curOutCard.m_color = m_curOutCard->m_color;
					curOutCard.m_number = m_curOutCard->m_number;
					m_playerHuInfo[i].wincards.push_back(curOutCard);
					m_video.AddOper(VIDEO_OPER_SHOUPAO, i, m_playerHuInfo[i].wincards);
				}
			}
			OnGameOver(WIN_BOMB, m_beforePos);
		}
		return;
	}

	//杠
	if (gangPos != INVAILD_POS)
	{
		LMsgS2CUserOper send;
		send.m_pos = gangPos;
		send.m_errorCode = 0;
		send.m_think.m_type = m_thinkRet[gangPos].m_type;
		for(Lsize i = 0 ; i < m_thinkRet[gangPos].m_card.size(); ++i)
		{
			CardValue v;
			v.m_color = m_thinkRet[gangPos].m_card[i]->m_color;
			v.m_number = m_thinkRet[gangPos].m_card[i]->m_number;
			send.m_think.m_card.push_back(v);
		}
		send.m_card.m_color = m_curOutCard->m_color;
		send.m_card.m_number = m_curOutCard->m_number;
		m_desk->BoadCast(send);
		gCB_HeNan_ZhengZhou->EraseCard(m_handCard[gangPos], m_thinkRet[gangPos].m_card[0],3);

		//录像
		std::vector<CardValue> cards;
		for (int i = 0; i < 4; ++i)
		{
			CardValue card;
			card.m_color = m_curOutCard->m_color;
			card.m_number = m_curOutCard->m_number;
			cards.push_back(card);
			m_gangCard[gangPos].push_back(m_curOutCard);
		}
		m_video.AddOper(VIDEO_OPER_GANG, gangPos, cards);

		m_diangang[gangPos] += 1;
		m_adiangang[m_beforePos] += 1;

		for (int i = 0; i < DESK_USER_COUNT; ++i)
		{
			m_thinkRet[i].Clear();
		}
		//给玩家摸一张牌
		SetPlayIng(gangPos, true, true, true, true);
		return;
	}

	//补
	if (buPos != INVAILD_POS)
	{
		LMsgS2CUserOper send;
		send.m_pos = buPos;
		send.m_errorCode = 0;
		send.m_think.m_type = m_thinkRet[buPos].m_type;
		for(Lsize i = 0 ; i < m_thinkRet[buPos].m_card.size(); ++i)
		{
			CardValue v;
			v.m_color = m_thinkRet[buPos].m_card[i]->m_color;
			v.m_number = m_thinkRet[buPos].m_card[i]->m_number;
			send.m_think.m_card.push_back(v);
		}
		send.m_card.m_color = m_curOutCard->m_color;
		send.m_card.m_number = m_curOutCard->m_number;
		m_desk->BoadCast(send);
		gCB_HeNan_ZhengZhou->EraseCard(m_handCard[buPos], m_thinkRet[buPos].m_card[0],3);

		//录像
		std::vector<CardValue> cards;
		for (int i = 0; i < 4; ++i)
		{
			CardValue card;
			card.m_color = m_curOutCard->m_color;
			card.m_number = m_curOutCard->m_number;
			cards.push_back(card);
			m_gangCard[buPos].push_back(m_curOutCard);
		}
		m_video.AddOper(VIDEO_OPER_OTHER_BU, buPos, cards);

		m_diangang[buPos] += 1;
		m_adiangang[m_beforePos] += 1;

		for (int i = 0; i < DESK_USER_COUNT; ++i)
		{
			m_thinkRet[i].Clear();
		}
		//给玩家摸一张牌
		SetPlayIng(buPos, true, false, true, true);
		return;
	}


	if (pengPos != INVAILD_POS)
	{
		LMsgS2CUserOper send;
		send.m_pos = pengPos;
		send.m_errorCode = 0;
		send.m_think.m_type = m_thinkRet[pengPos].m_type;
		for(Lsize i = 0 ; i < m_thinkRet[pengPos].m_card.size(); ++i)
		{
			CardValue v;
			v.m_color = m_thinkRet[pengPos].m_card[i]->m_color;
			v.m_number = m_thinkRet[pengPos].m_card[i]->m_number;
			send.m_think.m_card.push_back(v);
		}
		send.m_card.m_color = m_curOutCard->m_color;
		send.m_card.m_number = m_curOutCard->m_number;
		m_desk->BoadCast(send);
		gCB_HeNan_ZhengZhou->EraseCard(m_handCard[pengPos], m_curOutCard,2);

		{
			PengInfo pengInfo;
			pengInfo.card.m_color = m_curOutCard->m_color;
			pengInfo.card.m_number = m_curOutCard->m_number;
			pengInfo.pos = m_beforePos;
			m_pengInfo[pengPos].push_back(pengInfo);
		}
		

		//录像
		std::vector<CardValue> cards;
		for (int i = 0; i < 3; ++i)
		{
			CardValue card;
			card.m_color = m_curOutCard->m_color;
			card.m_number = m_curOutCard->m_number;
			cards.push_back(card);
			m_pengCard[pengPos].push_back(m_curOutCard);
		}
		m_video.AddOper(VIDEO_OPER_PENG_CARD, pengPos, cards);

		for (int i = 0; i < DESK_USER_COUNT; ++i)
		{
			m_thinkRet[i].Clear();
		}
		//碰完打一张牌
		m_curGetCard = NULL;
		SetPlayIng(pengPos, false, false, false, false);
		m_needGetCard = true;
		return;
	}

	//这里没有人操作
	if (m_beforeType == THINK_OPERATOR_MGANG || m_beforeType == THINK_OPERATOR_MBU)
	{
		//录像
		std::vector<CardValue> cards;
		for (int i = 0; i < 4; ++i)
		{
			CardValue card;
			card.m_color = m_curOutCard->m_color;
			card.m_number = m_curOutCard->m_number;
			cards.push_back(card);
		}
		if (m_beforeType == THINK_OPERATOR_MGANG)
		{
			m_video.AddOper(VIDEO_OPER_MING_GANG, m_beforePos, cards);
		}
		else
		{
			m_video.AddOper(VIDEO_OPER_SELF_BU, m_beforePos, cards);
		}

		m_gangCard[m_beforePos].push_back(m_curOutCard);
		CardVector::iterator it = m_pengCard[m_beforePos].begin();
		for(; it != m_pengCard[m_beforePos].end();it+=3)
		{
			if(gCB_HeNan_ZhengZhou->IsSame(m_curOutCard,*it))
			{
				m_gangCard[m_beforePos].insert(m_gangCard[m_beforePos].end(),it,it+3);
				m_pengCard[m_beforePos].erase(it,it+3);
				break;
			}
		}
		//这里处理 明杠 明补发消息
		LMsgS2CUserPlay sendMsg;
		sendMsg.m_errorCode = 0;
		sendMsg.m_pos = m_beforePos;
		sendMsg.m_card.m_type = m_beforeType;
		CardValue card;
		card.m_color = m_curOutCard->m_color;
		card.m_number = m_curOutCard->m_number;
		sendMsg.m_card.m_card.push_back(card);
		m_desk->BoadCast(sendMsg);

		//m_minggang[m_beforePos] += 1;
		Lint pengGangPos = FindPengGangPos(m_beforePos, card);
		if ((pengGangPos >= 0) && (pengGangPos < DESK_USER_COUNT))
		{
			m_pengGangInfo[m_beforePos].push_back(pengGangPos);
		}

		SetPlayIng(m_beforePos,true,m_beforeType == THINK_OPERATOR_MGANG, true, true);
	}
	else
	{
		if(m_curOutCard!=NULL)
			m_outCard[m_beforePos].push_back(m_curOutCard);
		SetPlayIng(m_desk->GetNextPos(m_beforePos), true, false, true, true);
	}
}

void GH_HeNan_ZhengZhou::VideoThink(Lint pos)
{
	if(m_thinkInfo[pos].m_thinkData.size() >0)
	{
		std::vector<CardValue> cards;
		for (auto itr=m_thinkInfo[pos].m_thinkData.begin(); itr!=m_thinkInfo[pos].m_thinkData.end(); ++itr)
		{
			CardValue card;
			card.m_number = itr->m_type;
			if (itr->m_card.size()>0)
				card.m_color  = itr->m_card[0]->m_color*10+itr->m_card[0]->m_number;
			if (itr->m_card.size()>1)
				card.m_color  = card.m_color*1000+itr->m_card[1]->m_color*10+itr->m_card[1]->m_number;
			cards.push_back(card);
		}
		m_video.AddOper(VIDEO_OPEN_THINK, pos, cards);
	}
}

void GH_HeNan_ZhengZhou::VideoDoing(Lint op, Lint pos, Lint card_color, Lint card_number)
{
	std::vector<CardValue> cards;
	CardValue card;
	card.m_number = op;
	card.m_color  = card_color*10+card_number;
	cards.push_back(card);
	m_video.AddOper(VIDEO_OPEN_DOING, pos, cards);

}

bool GH_HeNan_ZhengZhou::isGuoHandGang(Card* card,Lint pos)
{
	if(!card || pos < 0 || pos >= 4)
	{
		//printf("error...\n");
		return false;
	}

	for(size_t i = 0; i < m_gsCard[pos].size(); ++i)
	{
		if(m_gsCard[pos].at(i)->m_color == card->m_color && m_gsCard[pos].at(i)->m_number == card->m_number)
		{
			return true;
		}
	}
	return false;
}

void GH_HeNan_ZhengZhou::InitHunCard()
{
	if(m_playtype.GametypeGetGhost())
	{
		Card* card = m_deskCard.back();
		m_deskCard.pop_back();
		if (card)
		{
			m_ghostCardFlop = *card;
			CardValue ghost1;
			Lint number1 = card->m_number + 1;
			if (card->m_color != CARD_COLOR_ZI)		//万筒条
			{
				ghost1.m_number = number1 % 9 == 0 ? 9: number1 % 9;
				ghost1.m_color = card->m_color;
			}
			else if (card->m_color == CARD_COLOR_ZI)		//风
			{
				ghost1.m_number = number1 % 7 == 0 ? 7: number1 % 7;
				ghost1.m_color = card->m_color;

			}
			m_ghostCardReal.push_back(ghost1);

			Card ghost;
			ghost.m_color = ghost1.m_color;
			ghost.m_number = ghost1.m_number;

			m_gameInfo.m_hCard.clear();
			m_gameInfo.m_hCard.push_back(ghost);

			//录像
			std::vector<CardValue> cards;
			CardValue cardValue;
			cardValue.m_color = m_ghostCardFlop.m_color;
			cardValue.m_number = m_ghostCardFlop.m_number;
			cards.push_back(cardValue);
			m_video.AddOper(VIDEO_OPER_FANJIN, 0, cards);//需要修改
		}
	}

	LMsgS2CGhostCard ghoshCard;
	ghoshCard.m_ghostCard.m_color = m_ghostCardFlop.m_color;
	ghoshCard.m_ghostCard.m_number = m_ghostCardFlop.m_number;
	m_desk->BoadCast(ghoshCard);
}

void GH_HeNan_ZhengZhou::BroadcastBottomPour()
{
	LMsgS2CStartBottomPour bottompour;
	bottompour.m_need_bottom = m_playtype.hasPour();
	m_desk->BoadCast(bottompour);
}

bool GH_HeNan_ZhengZhou::IsPouredOver()
{
	int iDingQueCount = 0;
	for(int i = 0; i < DESK_USER_COUNT; ++i)
	{
		if(m_isFly[i] == 0)
			break;
		iDingQueCount++;
	}

	return iDingQueCount == DESK_USER_COUNT;
}

bool GH_HeNan_ZhengZhou::CheckIfBigHu(std::vector<Lint>& vec)
{
	bool isBigHu = false;
	for (Lint i = 0; i < vec.size(); ++i)
	{
		if (vec[i] == HU_QIDUI || vec[i] == HU_FOURGHOST)
		{
			isBigHu = true;
			break;
		}
	}
	return isBigHu;
}

Lint GH_HeNan_ZhengZhou::GetHuBaseSocore(HuInfo* pHuInfo)
{
	Lint base = 0;
	if(pHuInfo == NULL)
		return base;

	for(int i = 0; i < DESK_USER_COUNT; ++i)
	{
		if((pHuInfo+i)->type == WIN_SUB_ZIMO)
		{
			base = 2;
			break;
		}
		else if((pHuInfo+i)->type == WIN_SUB_BOMB)
		{
			base = 1;
			break;
		}
		else if((pHuInfo+i)->type == WIN_SUB_DZIMO)
		{
			base = 6;
			break;
		}
		else if((pHuInfo+i)->type == WIN_SUB_DBOMB)
		{
			base = 4;
			break;
		}
	}

	return base;
}

void GH_HeNan_ZhengZhou::GetGangSocore(Lint gold[])
{
	for (Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		// 过手杠
		/*int gscount = 0;
		for(Lsize j = 0; j < m_gangCard[i].size(); j+=4)
		{
			if(isGuoHandGang(m_gangCard[i][j],i))
			{
				++gscount;
			}
		}*/

		gold[i] += 3 * 2 * m_angang[i];  // 暗杠 向每人收取2
		gold[i] += 1 * m_pengGangInfo[i].size();  //碰杠数量
		gold[i] += 1 * m_diangang[i];// 点杠 向点杠的人收取X
		for (Lint j = 0; j < DESK_USER_COUNT; ++j)
		{
			if (j != i)
			{
				gold[j] -= 2 * m_angang[i];
				//gold[j] -= (m_minggang[i]-gscount);
			}
		}

		//碰杠
		std::vector<Lint> pengPosVect = m_pengGangInfo[i];
		for (size_t k = 0; k < pengPosVect.size(); ++k)		//扣除碰杠的数量
		{
			Lint pengPos = pengPosVect[k];
			if ((pengPos >= 0) && (pengPos < DESK_USER_COUNT))
			{
				gold[pengPos] -= 1;
			}
		}

		//这里是给别人点杠
		gold[i] -=  m_adiangang[i];
	}

}

void GH_HeNan_ZhengZhou::GetPaoSocore(Lint result,Lint gold[], Lint& bombCount, Lint winPos[])
{
	int x = GetHuBaseSocore(m_playerHuInfo);

	for (Lint i = 0; i < DESK_USER_COUNT; ++i)
	{
		if (result == WIN_ZIMO)
		{
			// 向每人收取Y
			if (m_playerHuInfo[i].type == WIN_SUB_ZIMO || m_playerHuInfo[i].type == WIN_SUB_DZIMO)
			{
				winPos[i] = 1;
				for (Lint j = 0; j < DESK_USER_COUNT; ++j)
				{
					if (j != i)
					{
						Lint score = (m_isPiao[j] + m_isPiao[i]) + x ;//x为胡牌分数，跑数加上胡牌分数
						gold[j] -= score;
						gold[i] += score;
					}
				}
			}
		}
		else if (result == WIN_BOMB)
		{
			if (m_playerHuInfo[i].type == WIN_SUB_BOMB || m_playerHuInfo[i].type == WIN_SUB_DBOMB)
			{
				winPos[i] = 1;
				bombCount++;
				for (Lint j = 0; j < DESK_USER_COUNT; ++j)
				{
					if (j == m_playerHuInfo[i].abombpos)
					{
						Lint score = (m_isPiao[j] + m_isPiao[i]) + x ;//x为胡牌分数，跑数加上胡牌分数
						gold[j] -= score;
						gold[i] += score;
					}
				}
			}
		}
	}
}

Lint GH_HeNan_ZhengZhou::FindPengGangPos(Lint curPos, CardValue card) 
{
	if ((curPos < 0) || (curPos >= DESK_USER_COUNT))
	{
		return INVAILD_POS;
	}

	auto it = m_pengInfo[curPos].begin();
	for ( ; it != m_pengInfo[curPos].end(); ++it)
	{
		if ((it->card.m_color == card.m_color) && (it->card.m_number == card.m_number))
		{
			return it->pos;
		}
	}
	return INVAILD_POS;
}