#pragma once 

#include "stdafx.h"
#include "string"

#ifdef _DEBUG 
#else
#include "cqp.h"
#endif 

#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tchar.h>  
#include <regex> 

#include "landlords_Util.h"
#include "landlords_Admin.h"
#include "landlords_Desks.h"
#include "landlords_Player.h"
#include "landlords_Watcher.h"

using namespace std;

int Util::AC = 0;

void Desk::commandList()
{
	this->msg
		<< L"斗地主命令列表（*号表示支持后带符号）：" << "\r\n" 
		<< L"0*. 斗地主版本：查看游戏版本号、GitHub链接与原作者信息\r\n"
		<< L"1*. 上桌|打牌：加入游戏\r\n"
		<< L"2*. 出|打：出牌 比如 出23456！\r\n"
		<< L"3*. 过(牌)|不要|pass：过牌\r\n"
		<< L"4*. 抢(地主)|不抢：是否抢地主\r\n"
		<< L"5*. 加(倍)|不加(倍)：是否加倍\r\n"
		<< L"6*. 开始|启动|GO：是否开始游戏\r\n"
		<< L"7*. 下桌|不玩了：退出游戏，只能在准备环节使用\r\n"
		<< L"8. 玩家列表：当前在游戏中得玩家信息\r\n"
		<< L"9*. 明牌：显示自己的牌给所有玩家，明牌会导致积分翻倍，只能在发完牌后以及出牌之前使用。\r\n"
		<< L"10*. 弃牌：放弃本局游戏，当地主或者两名农民弃牌游戏结束。农民玩家弃牌赢了不得分（有弃牌惩罚），输了双倍扣分" << "\r\n"
		//<< L"11. 获取积分（已废弃）：获取积分，每天可获取200积分。" << "\r\n"
		<< L"11. 我的信息：查看我的战绩与积分信息（群聊私聊皆可）" << "\r\n"
		<< L"12. 加入观战|观战：暗中观察" << "\r\n"
		<< L"13. 退出观战：光明正大的看打牌" << "\r\n"
		//<< L"14*. 举报|挂机|AFK：超过" << CONFIG_TIME_GAME 
		//	<< L"秒不出牌，可以举报，举报成功的奖励" << CONFIG_SURRENDER_PENALTY
		//	<< L"分，被举报的扣除" << CONFIG_SURRENDER_PENALTY << L"分。" << "\r\n"
		<< L"A1. " << L"我是管理：绑定游戏管理员为当前发送消息的qq，管理员可使用管理命令。管理设置后不能更改" << "\r\n"
		<< L"A2. " << L"重置斗地主：删除所有配置。重置后可重新设定管理员" << "\r\n"
		<< L"A3. " << L"结束游戏[群号]：结束指定群号的游戏，比如：结束游戏123456" << "\r\n"
		<< L"A4. " << L"设置积分[qq号]=[积分]：给指定qq分配积分，如：设置积分123456=-998" << "\r\n"
		<< L"A5. " << L"改变数据类型：切换“正式数据”与“测试数据”类型" << "\r\n"
		<< L"A6. " << L"备份数据：你懂的，防止服务器爆炸"
		;
	this->breakLine();
}

bool Desks::game(bool subType, int64_t deskNum, int64_t playNum, const char* msgArray) {

	string tmp = msgArray;

	wstring msg = Util::string2wstring(tmp);
	Util::trim(msg);
	Util::toUpper(msg);

	Desk *desk = casino.getOrCreatDesk(deskNum);

	desk->subType = subType;

	if (playNum == 80000000) {
		//desk->msg << L"匿名用户不能参加斗地主！";
		return false;
	}
	else if (msg.find(L"斗地主命令") == 0 || msg.find(L"斗地主指令") == 0 || msg.find(L"斗地主操作") == 0) {
		desk->commandList();
	}
	else if (msg.find(L"斗地主规则") == 0) {
		desk->msg << L"斗地主规则：" << "\r\n";
		desk->msg << L"打牌一次奖励" << CONFIG_PLAY_BONUS
			<< L"分，中途退出（弃牌）、挂机（抢地主、加倍" << CONFIG_TIME_BOSS << L"秒，出牌"
			<< CONFIG_TIME_GAME << L"秒）倒扣" << CONFIG_SURRENDER_PENALTY << L"分。";
		//desk->breakLine();
		desk->msg << L"每局游戏的标准分计算方法为：初始值" << CONFIG_INIT_SCORE
			<< L"分，最高最低分玩家的积分差额每有" << 50
			<< L"分，标准分加" << CONFIG_BOTTOM_SCORE << L"分，但标准分不会超过"
			<< CONFIG_TOP_SCORE << L"分。";
		//desk->breakLine();
		desk->msg << L"分数下限为负5亿，上限为正5亿。";
		//desk->breakLine();
	}
	else if (msg.find(L"斗地主") == 0) {
		desk->msg << L"斗地主 " << CONFIG_VERSION;
		desk->breakLine();
		desk->msg << Admin::readDataType() << L" " << Admin::readVersion(); // << L" CST";
		desk->breakLine();
		desk->msg << L"源代码与更新履历：https://github.com/doowzs/CQDouDiZhu";
		desk->breakLine();
		desk->msg << L"原作者与2.0.1源代码：https://github.com/lsjspl/CQDouDiZhu";
	}
	else if (msg.find(L"上桌") == 0 || msg.find(L"上座") == 0 || msg.find(L"上机") == 0
		|| msg.find(L"打牌") == 0) {
		desk->join(playNum);
	}
	else if ((desk->state >= STATE_READYTOGO) &&
		(msg.find(L"出") == 0 || msg.find(L"打") == 0)) {//出牌阶段
		desk->play(playNum, msg);
	}
	else if ((desk->state >= STATE_READYTOGO) &&
		(msg.find(L"过") == 0 || msg.find(L"过牌") == 0 || msg.find(L"不出") == 0
			|| msg.find(L"没有") == 0 || msg.find(L"打不出") == 0 || msg.find(L"要不起") == 0
			|| msg.find(L"不要") == 0 || msg.find(L"PASS") == 0)) {//跳过出牌阶段
		desk->discard(playNum);
	}
	else if (msg.find(L"退桌") == 0 || msg.find(L"下桌") == 0
		|| msg.find(L"不玩了") == 0) {//结束游戏
		desk->exit(playNum);
	}
	else if (msg == L"玩家列表") {
		desk->listPlayers(1);
	}
	else if (msg.find(L"GO") == 0 || msg.find(L"启动") == 0) {
		desk->startGame();
	}
	else if ((msg.find(L"抢") == 0 || msg.find(L"要") == 0) && desk->state == STATE_BOSSING) {
		desk->getBoss(playNum);
	}
	else if (msg.find(L"不") == 0 && desk->state == STATE_BOSSING) {
		desk->dontBoss(playNum);
	}
	else if (msg.find(L"加") == 0 && desk->state == STATE_MULTIPLING) {
		desk->getMultiple(playNum);
	}
	else if (msg.find(L"不") == 0 && desk->state == STATE_MULTIPLING) {
		desk->dontMultiple(playNum);
	}
	else if (msg.find(L"明牌") == 0) {
		desk->openCard(playNum);
	}
	else if ((msg.find(L"弃牌") == 0)
		&& desk->state >= STATE_BOSSING) {
		desk->surrender(playNum);
	}
	else if (msg == L"记牌器") {
		desk->msg << L"记牌器没做(好)呢！估计有生之年可以做好！";
	}
	else if (msg == L"我的信息") {
		desk->getPlayerInfo(playNum);
	}
	else if (msg.find(L"加入观战") == 0 || msg.find(L"观战") == 0) {
		desk->joinWatching(playNum);
	}
	else if (msg.find(L"退出观战") == 0) {
		desk->exitWatching(playNum);
	}
	//else if (msg.find(L"举报") == 0 || msg.find(L"挂机") == 0 || msg.find(L"AFK") == 0) {
	//	desk->AFKHandle(playNum);
	//}
	else if (msg.find(L"强制结束") == 0) {
		if (Admin::isAdmin(playNum)) {
			if (desk->state <= STATE_BOSSING) {
				return false;
			}
			else {
				desk->msg << L"管理员强制结束本桌游戏。";
				desk->breakLine();
				casino.gameOver(deskNum);
			}
		}
		else {
			desk->msg << L"你根本不是管理员！";
			desk->breakLine();
		}
	}
	else {
		return false;
	}

	desk->sendMsg(subType);
	desk->sendPlayerMsg();
	desk->sendWatcherMsg();
	return true;
}

bool Desks::game(int64_t playNum, const char * msgArray)
{
	string tmp = msgArray;

	wstring msg = Util::string2wstring(tmp);
	Util::trim(msg);
	Util::toUpper(msg);


	bool result;
	if (msg == L"我是管理") {
		result = Admin::IAmAdmin(playNum);
	}
	else if (msg == L"重置斗地主" || msg == L"初始化斗地主") {
		result = Admin::resetGame(playNum);
	}
	else if (msg.find(L"改变数据") == 0) {
		result = Admin::writeDataType();
		Admin::getPlayerInfo(Admin::readAdmin());
	}
	else if (regex_match(msg, allotReg)) {
		result = Admin::allotScoreTo(msg, playNum);
	}
	else if (regex_match(msg, allotReg2)) {
		result = Admin::allotScoreTo2(msg, playNum);
	}
	else if (msg.find(L"结束游戏") == 0 || msg.find(L"停止游戏") == 0) {//结束游戏
		result = Admin::gameOver(msg, playNum);
	}
	else if (msg == L"我的信息") {
		Admin::getPlayerInfo(playNum);
		return false;
	}
	else if (regex_match(msg, getInfoReg)) {
		//查询指定玩家积分
		int score;
		int64_t playerNum;

		wsmatch mr;
		wstring::const_iterator src_it = msg.begin(); // 获取起始位置
		wstring::const_iterator src_end = msg.end(); // 获取结束位置
		regex_search(src_it, src_end, mr, numberReg);
		wstringstream ss;
		ss << mr[0].str();
		ss >> playerNum;
		ss.str(L"");

		Admin::getPlayerInfo(playNum);
		return false;
	}
	else if (msg == L"备份数据") {
		result = Admin::backupData(playNum);
	}
	else {
		return false;
	}

	msg = result ? L"操作成功，尊贵的管理员" : L"非常抱歉，操作失败";
	Util::sendPrivateMsg(playNum, Util::wstring2string(msg).data());
	return true;
}