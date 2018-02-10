#pragma once

#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tchar.h>  
#include <regex> 

#include "landlords_classes.h"
using namespace std;

Desk::Desk() {
	for (int i = 0; i < 54; i++) {
		this->cards[i] = cardDest[i];
	}

	this->state = 0;
	this->lastPlayIndex = -1;//当前谁出得牌
	this->currentPlayIndex = -1;//该谁出牌
	this->bossIndex = -1;//谁是地主
	this->isSecondCallForBoss = false;

	this->bossCount = 0;
	this->farmCount = 0;

	vector<wstring> lastCard;//上位玩家的牌
	this->lastCardType = L"";//上位玩家得牌类
	this->lastWeights = new vector<int>;//上位玩家的牌

	this->whoIsWinner = 0;
	this->multiple = 1;
	this->basic = CONFIG_INIT_SCORE; //防止三个0分的玩积分还是0
	this->turn = 0;
	this->lastTime = 0; //最后一次发牌时间，记录用
}

void Desk::at(int64_t playNum)
{
	this->msg << L"[CQ:at,qq=" << playNum << L"]";
}

void Desk::breakLine()
{
	this->msg << L"\r\n";
}

int Desk::getPlayer(int64_t number) {
	for (unsigned i = 0; i < players.size(); i++) {
		if (players[i]->number == number) {
			return i;
		}
	}
	return -1;
}

int Desk::getWatcher(int64_t number) {
	for (unsigned i = 0; i < watchers.size(); i++) {
		if (watchers[i]->number == number) {
			return i;
		}
	}
	return -1;
}

void Desk::listPlayers(int type)
{

	bool hasType = (type & 1) == 1;
	bool hasWin = ((type >> 1) & 1) == 1;

	int score = this->basic * this->multiple;
	int halfScore = score / 2;
	
	if (this->players.size() < 3) {
		this->msg << L"游戏尚未开始，玩家列表：";
		this->breakLine();
	}
	else {
		this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
		this->breakLine();
		this->msg << L"出牌次数：" << this->turn;
		this->breakLine();
	}

	for (unsigned i = 0; i < this->players.size(); i++) {
		this->msg << i + 1 << L"：";
		if (hasType) {
			this->msg << L"[" << (i == this->bossIndex && state == STATE_GAMEING ? L"地主" : L"农民") << L"]";
		}

		this->at(this->players[i]->number);
		if (hasWin) {
			if (this->whoIsWinner == 2) {//如果是农民赢了
				if (i == this->bossIndex) {
					this->msg << L"[" << L"失败-" << score << L"分]";
					Admin::addScore(this->players[i]->number, -score);
					Admin::addLose(this->players[i]->number);

					this->breakLine();
					this->msg << L"剩余手牌：";
					this->listCardsOnDesk(this->players[i]);
				}
				else if (this->players[i]->isSurrender) {
					this->msg << L"[" << L"弃牌-" << CONFIG_SURRENDER_PENALTY << L"分]";
					Admin::addScore(this->players[i]->number, -CONFIG_SURRENDER_PENALTY);
					Admin::addWin(this->players[i]->number);
				}
				else {
					this->msg << L"[" << L"胜利+" << halfScore << L"分]";
					Admin::addScore(this->players[i]->number, halfScore);
					Admin::addWin(this->players[i]->number); 
					
					//如果还有牌，就公开手牌
					if (this->players[i]->card.size() > 0) {
						this->breakLine();
						this->msg << L"剩余手牌：";
						this->listCardsOnDesk(this->players[i]);
					}
				}
			}
			else {
				if (i == this->bossIndex) {
					this->msg << L"[" << L"胜利+" << score << L"分]";
					Admin::addScore(this->players[i]->number, score);
					Admin::addWin(this->players[i]->number);
				}
				else if (this->players[i]->isSurrender) {
					this->msg << L"[" << L"弃牌-" << score+CONFIG_SURRENDER_PENALTY << L"分]";
					Admin::addScore(this->players[i]->number, - score - CONFIG_SURRENDER_PENALTY);
					Admin::addLose(this->players[i]->number);
				}
				else {
					this->msg << L"[" << L"失败-" << halfScore << L"分]";
					Admin::addScore(this->players[i]->number, -halfScore);
					Admin::addLose(this->players[i]->number);

					this->breakLine();
					this->msg << L"剩余手牌：";
					this->listCardsOnDesk(this->players[i]);
				}
			}
		}

		this->breakLine();
	}
}

bool Desk::isCanWin(int cardCount, vector<int> *weights, wstring type)
{

	if (type == L"" || this->lastCardType == L"王炸") {
		return false;
	}

	if (this->lastCardType == L"") {
		return true;
	}

	if (type == L"王炸") {
		return true;
	}
	if (type == L"炸弹" && type != this->lastCardType) {
		return true;
	}


	if (type == this->lastCardType && cardCount == this->lastCard.size()) {

		for (unsigned i = 0; i < weights->size(); i++) {
			if (weights[i] > this->lastWeights[i]) {
				return true;
			}
		}

	}


	return false;
}

wstring Desk::getMycardType(vector<wstring> list, vector<int> *weights)
{
	int cardCount = list.size();
	sort(list.begin(), list.end(), Util::compareCard);

	if (cardCount == 2 && Util::findFlag(list[0]) + Util::findFlag(list[1]) == 27) {//王炸
		return L"王炸";
	}

	vector<wstring> cards;
	vector<int> counts;

	bool no2InCards = true;

	for (unsigned i = 0; i < list.size(); i++) {
		int index = Util::find(cards, list[i]);
		if (index == -1) {
			cards.push_back(list[i]);
			counts.push_back(1);
		}
		else {
			counts[index] = counts[index] + 1;
		}

		if (Util::findFlag(list[i]) == 12) {	//第12大的牌是2
			no2InCards = false;
		}
	}

	int max = counts[0];//存放大值
	int min = counts[0];//存放小值
	int cardGroupCout = cards.size();
	int tmp;
	for (unsigned i = 0; i < counts.size(); i++) {
		tmp = counts[i];
		if (tmp > max) {
			max = tmp;
		}
		if (tmp < min) {
			min = tmp;
		}
	}

	vector<int> tmpCount(counts);
	sort(tmpCount.begin(), tmpCount.end(), Util::desc);
	if (cardCount == 1) {//单牌
		weights->push_back(Util::findFlag(cards[0]));
		return L"单牌";
	}
	if (cardCount == 2 && max == 2) {//对子
		weights->push_back(Util::findFlag(cards[0]));
		return L"对子";
	}
	if (cardCount == 3 && max == 3) {//三张
		weights->push_back(Util::findFlag(cards[0]));
		return L"三张";
	}
	if (cardCount == 4 && max == 4) {//炸弹
		weights->push_back(Util::findFlag(cards[0]));
		return L"炸弹";
	}

	if (cardCount == 4 && max == 3) {//3带1

		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"3带1";
	}

	if (cardCount == 5 && max == 3 && min == 2) {//3带2
		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"3带2";
	}

	if (cardCount == 6 && max == 4) {//4带2
		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"4带2";
	}

	if (cardGroupCout > 2 && max == 2 && min == 2
		&& Util::findFlag(cards[0]) == Util::findFlag(cards[cardGroupCout - 1]) - cardGroupCout + 1
		&& Util::findFlag(cards[cardGroupCout - 1]) < 13
		&& no2InCards	//连对不能有2
		) {//连对
		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"连对";
	}

	//for (unsigned i = 0; i < cardGroupCout; i++) {
	//	if (cards[cardGroupCout] == L"2") {
	//		no2InCards = false;
	//	}
	//}

	if (cardGroupCout > 4 && max == 1 && min == 1
		&& Util::findFlag(cards[0]) == Util::findFlag(cards[cardGroupCout - 1]) - cardGroupCout + 1
		&& Util::findFlag(cards[cardGroupCout - 1]) < 13
		&& no2InCards	//三人斗地主顺子不能带2
		) {//顺子
		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"顺子";
	}

	//飞机
	int  planeCount = 0;
	for (unsigned i = 0; i < counts.size(); i++) {
		if (counts[i] >= 3) {
			planeCount++;
		}
	}
	if (planeCount > 1) {
		wstring tmp;
		if (cardCount == planeCount * 3) {
			tmp = L"飞机";
		}
		else if (cardCount == planeCount * 4) {
			tmp = L"飞机带翅膀";
		}
		else if (cardCount == planeCount * 5 && min == 2) {
			tmp = L"飞机带双翅膀";
		}

		for (int i = 0; i < planeCount; i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		sort(weights->begin(), weights->end(), Util::desc);

		int weightscount = weights->size();

		if (weights->at(0) - weightscount + 1 != weights->at(weightscount - 1)) {
			return L"";
		}

		return tmp;
	}
	return L"";
}

void Desk::sendMsg(bool subType)
{
	wstring tmp = this->msg.str();
	if (tmp.empty()) {
		return;
	}
	int length = tmp.length();
	if (tmp[length - 2] == '\r' && tmp[length - 1] == '\n') {
		tmp = tmp.substr(0, length - 2);
	}
	if (subType) {
		Util::sendGroupMsg(this->number, Util::wstring2string(tmp).data());
	}
	else {
		Util::sendDiscussMsg(this->number, Util::wstring2string(tmp).data());
	}

	this->msg.str(L"");
}

void Desk::sendPlayerMsg()
{
	for (unsigned i = 0; i < this->players.size(); i++) {
		players[i]->sendMsg();
	}
}

void Desk::sendWatcherMsg()
{
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watchers[i]->sendMsg();
	}
}

void Desk::shuffle() {
	srand((unsigned)time(NULL));
	for (unsigned i = 0; i < 54; i++) {
		swap(this->cards[i], this->cards[rand() % 54]);
	}
}

void Desk::creataBoss() {
	state = STATE_BOSSING;

	this->msg << L"下面进入抢地主环节。";
	this->breakLine();
	this->msg << L"---------------";
	this->breakLine();

	int index = rand() % 3;

	this->bossIndex = index;
	this->currentPlayIndex = index;
	this->at(this->players[index]->number);
	this->breakLine();
	this->msg << L"你是否要抢地主？";
	this->breakLine();
	this->msg << L"请用[抢]或[不(抢)]来回答。";
	this->breakLine();
	//战况播报
	this->sendWatchingMsg_Start();
}

void Desk::getBoss(int64_t playerNum)
{
	int index = this->getPlayer(playerNum);
	if (this->state == STATE_BOSSING && this->currentPlayIndex == index) {

		this->bossIndex = index;
		this->currentPlayIndex = index;
		this->lastPlayIndex = index;
		sendBossCard();

		//进入加倍环节
		this->state = STATE_MULTIPLING;
		this->multipleChoice();
	}
}

void Desk::dontBoss(int64_t playerNum)
{
	int index = this->getPlayer(playerNum);
	if (this->state == STATE_BOSSING && this->currentPlayIndex == index) {

		this->setNextPlayerIndex();

		if (this->currentPlayIndex == this->bossIndex && this->isSecondCallForBoss) {
			this->msg << L"第2次抢地主失败，";
			this->sendBossCard();
			this->state = STATE_MULTIPLING;
			this->multipleChoice();
			return;
		}
		else if (this->currentPlayIndex == this->bossIndex) {
			this->msg << L"第1次抢地主失败，重新发牌。";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			this->isSecondCallForBoss = true;
			this->shuffle();
			this->deal();
			this->creataBoss();
			return;
		}
		else {
			this->at(this->players[index]->number);
			this->msg << L"不抢地主。";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			this->at(this->players[currentPlayIndex]->number);
			this->breakLine();
			this->msg << L"你是否要抢地主？";
			this->breakLine();
			this->msg << L"请用[抢]或[不(抢)]来回答。";
			this->breakLine();
		}
	}
}

void Desk::sendBossCard()
{
	Player *playerBoss = players[this->bossIndex];

	this->at(playerBoss->number);
	this->msg << L"是地主，底牌是：";
	this->msg << L"[" << this->cards[53] << L"]"
		<< L"[" << this->cards[52] << L"]"
		<< L"[" << this->cards[51] << L"]。";
	this->breakLine();
	this->msg << L"---------------";
	this->breakLine();

	for (int i = 0; i < 3; i++) {
		playerBoss->card.push_back(cards[53 - i]);
	}
	sort(playerBoss->card.begin(), playerBoss->card.end(), Util::compareCard);

	//playerBoss->msg << L"你是地主，收到底牌：";
	//playerBoss->breakLine();
	for (unsigned m = 0; m < playerBoss->card.size(); m++) {
		playerBoss->msg << L"[" << playerBoss->card.at(m) << L"]";
	}
	playerBoss->breakLine();

	//这里的状态声明移动到了加倍(dontMultiple)的最后一个人语句哪里。
}

void Desk::multipleChoice() {
	this->msg << L"抢地主环节结束，下面进入加倍环节。";
	this->breakLine();
	this->msg << L"---------------";
	this->breakLine();
	this->at(this->players[this->bossIndex]->number);
	this->breakLine();
	this->msg << L"你是否要加倍？";
	this->breakLine();
	this->msg << L"请用[加]或[不(加)]来回答。";
	this->breakLine();
}

void Desk::getMultiple(int64_t playerNum)
{
	int index = this->getPlayer(playerNum);
	if (this->state == STATE_MULTIPLING && this->currentPlayIndex == index) {
		this->multiple += 1;

		this->setNextPlayerIndex();

		if (this->currentPlayIndex == this->bossIndex && bossHasMultipled) {
			this->at(this->players[index]->number);
			this->msg << L"要加倍。";
			this->breakLine();
			this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();

			this->state = STATE_READYTOGO;

			this->msg << L"加倍环节结束，斗地主正式开始。";
			this->breakLine();
			this->msg << L"---------------";
			//this->breakLine();
			//this->msg << L"第" << this->turn + 1 << L"回合：剩余手牌数：";
			this->breakLine();
			for (unsigned i = 0; i < this->players.size(); i++) {
				this->msg << i + 1 << L"：";
				this->msg << L"[" << (i == this->bossIndex ? L"地主" : L"农民") << L"]"; //这里删除条件&& state == STATE_GAMEING 
				this->at(this->players[i]->number);
				this->msg << L"：" << static_cast<int>(this->players[i]->card.size());
				this->breakLine();
			}
			this->breakLine();
			this->msg << L"请地主";
			this->at(this->players[this->bossIndex]->number);
			this->msg << L"先出牌。";
			this->breakLine();
			//战况播报
			this->sendWatchingMsg_Start();
		}
		else {
			bossHasMultipled = true;

			this->at(this->players[index]->number);
			this->msg << L"要加倍。";
			this->breakLine();
			this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			this->at(this->players[currentPlayIndex]->number);
			this->msg << L"你是否要加倍？";
			this->breakLine();
			this->msg << L"请用[加]或[不(加)]来回答。";
			this->breakLine();
		}
	}
}

void Desk::dontMultiple(int64_t playerNum)
{
	int index = this->getPlayer(playerNum);
	if (this->state == STATE_MULTIPLING && this->currentPlayIndex == index) {

		this->setNextPlayerIndex();

		if (this->currentPlayIndex == this->bossIndex && bossHasMultipled) {
			this->at(this->players[index]->number);
			this->msg << L"不要加倍。";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();

			this->state = STATE_READYTOGO;

			this->msg << L"加倍环节结束，斗地主正式开始。";
			this->breakLine();
			this->msg << L"---------------";
			//this->breakLine();
			//this->msg << L"第" << this->turn + 1 << L"回合：";
			this->breakLine();
			this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
			this->breakLine();
			this->msg << L"剩余手牌数：";
			this->breakLine();
			for (unsigned i = 0; i < this->players.size(); i++) {
				this->msg << i + 1 << L"：";
				this->msg << L"[" << (i == this->bossIndex ? L"地主" : L"农民") << L"]"; //这里删除条件&& state == STATE_GAMEING 
				this->at(this->players[i]->number);
				this->msg << L"：" << static_cast<int>(this->players[i]->card.size());
				this->breakLine();
			}
			this->breakLine();
			this->msg << L"请地主";
			this->at(this->players[this->bossIndex]->number);
			this->msg << L"先出牌。";
			this->breakLine();
			//战况播报
			this->sendWatchingMsg_Start();
		}
		else {
			bossHasMultipled = true;

			this->at(this->players[index]->number);
			this->msg << L"不要加倍。";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			this->at(this->players[currentPlayIndex]->number);
			this->breakLine();
			this->msg << L"你是否要加倍？";
			this->breakLine();
			this->msg << L"请用[加]或[不(加)]来回答。";
			this->breakLine();
		}
	}
}

void Desk::play(int64_t playNum, wstring msg)
{
	int playIndex = this->getPlayer(playNum);
	int length = msg.length();

	if (playIndex == -1 || playIndex != this->currentPlayIndex
		|| (!(this->state == STATE_GAMEING && this->turn > 0)
			&& !(this->state == STATE_READYTOGO && this->turn == 0))
		|| length < 2) {
		return;
	}

	vector<wstring> msglist;

	for (int i = 1; i < length; i++) {
		wstring tmp = msg.substr(i, 1);
		if (tmp == L"1") {
			tmp = msg.substr(i, 2);
			i++;
		}
		msglist.push_back(tmp);
	}

	this->play(msglist, playIndex);

}

void Desk::play(vector<wstring> list, int playIndex)
{

	Player *player = this->players[playIndex];
	vector<wstring> mycardTmp(player->card);

	int cardCount = list.size();

	for (int i = 0; i < cardCount; i++) {
		if (Util::findAndRemove(mycardTmp, list[i]) == -1) {
			this->at(this->players[currentPlayIndex]->number);
			this->breakLine();
			this->msg << L"真丢人，你就没有你要出的牌，会不会玩？";
			return;
		}
	}

	//处理出牌次数
	if (currentPlayIndex == this->bossIndex) {
		this->bossCount++;
	}
	else {
		this->farmCount++;
	}

	//记录出牌时间
	time_t rawtime;
	this->lastTime = time(&rawtime);

	vector<int> *weights = new vector<int>;

	wstring type = this->getMycardType(list, weights);

	bool isCanWin = this->isCanWin(cardCount, weights, type);

	if (isCanWin) {

		if (this->turn == 0) {
			this->state = STATE_GAMEING;
		}

		player->card = mycardTmp;
		this->lastWeights = weights;
		this->lastCard = list;
		this->lastCardType = type;
		this->lastPlayIndex = this->currentPlayIndex;
		this->turn++;


		//处理积分
		if (type == L"王炸") {
			this->multiple += 2;

			this->msg << L"打出王炸，积分倍数+2";
			this->breakLine();
			this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
		}
		else if (type == L"炸弹") {
			this->multiple += 1;

			this->msg << L"打出炸弹，积分倍数+1";
			this->breakLine();
			this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
		}

		if (mycardTmp.size() == 0) {//赢了。
			this->whoIsWinner = this->bossIndex == this->currentPlayIndex ? 1 : 2;

			this->sendWatchingMsg_Over();

			this->msg << L"斗地主游戏结束，";
			this->msg << (this->whoIsWinner == 1 ? L"地主赢了" : L"农民赢了");
			this->breakLine();


			if (this->farmCount == 0 && this->whoIsWinner == 1) {
				this->multiple *= 2;
				this->msg << L"---------------";
				this->breakLine();
				this->msg << L"本局出现春天，积分倍数x2";
				this->breakLine();
				this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
				this->breakLine();
			}
			else if (this->bossCount == 1 && this->whoIsWinner == 2) {
				this->multiple *= 2;
				this->msg << L"---------------";
				this->breakLine();
				this->msg << L"本局出现反春天，积分倍数x2";
				this->breakLine();
				this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
				this->breakLine();
			}


			this->msg << L"---------------";
			this->breakLine();
			this->msg << L"分数结算：";
			this->breakLine();
			this->listPlayers(3);

			casino.gameOver(this->number);
			return;
		}

		player->listCards();

		if (player->isOpenCard) {
			this->at(player->number);
			this->msg << L"明牌：";
			this->listCardsOnDesk(player);
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
		}

		if (player->card.size() < 3) {
			this->msg << L"红色警报！红色警报！";
			this->breakLine();
			this->at(player->number);
			this->msg << L"仅剩下" << player->card.size() << L"张牌！";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
		}

		//this->msg << L"上回合";
		this->at(this->players[currentPlayIndex]->number);
		this->msg << L"打出" << this->lastCardType;
		for (unsigned m = 0; m < this->lastCard.size(); m++) {
			this->msg << L"[" << this->lastCard.at(m) << L"]";
		}
		this->breakLine();

		//观战播报，必须先转发战况再设置下一位玩家，否则玩家信息错误
		this->sendWatchingMsg_Play();

		this->setNextPlayerIndex();

		this->msg << L"---------------";
		//this->breakLine();
		//this->msg << L"第" << this->turn + 1 << L"回合：";
		this->breakLine();
		this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
		this->breakLine();
		this->msg << L"剩余手牌数：";
		this->breakLine();
		for (unsigned i = 0; i < this->players.size(); i++) {
			this->msg << i + 1 << L"：";
			this->msg << L"[" << (i == this->bossIndex && state == STATE_GAMEING ? L"地主" : L"农民") << L"]";
			this->at(this->players[i]->number);
			this->msg << L"：" << static_cast<int>(this->players[i]->card.size());
			this->breakLine();
		}
		this->breakLine();
		this->msg << L"现在轮到";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"出牌。";
		this->breakLine();
	}
	else {
		this->at(this->players[this->currentPlayIndex]->number);
		this->breakLine();
		this->msg << L"傻逼网友，打的什么几把玩意！学会出牌再打！";
		this->breakLine();
	}
}

void Desk::discard(int64_t playNum)
{
	if (this->currentPlayIndex != this->getPlayer(playNum) || this->state != STATE_GAMEING) {
		return;
	}

	if (this->currentPlayIndex == this->lastPlayIndex) {
		this->msg << L"过过过过你妹，会不会玩，你不能过牌了，丢人！";
		return;
	}

	//记录过牌时间
	time_t rawtime;
	this->lastTime = time(&rawtime);

	this->at(playNum);
	this->msg << L"过牌，";

	this->setNextPlayerIndex();

	this->msg << L"现在轮到";
	this->at(this->players[this->currentPlayIndex]->number);
	this->msg << L"出牌。";
	this->breakLine();

	//观战播报
	this->sendWatchingMsg_Pass(playNum);
}

void Desk::surrender(int64_t playNum)
{
	int index = this->getPlayer(playNum);
	if (index == -1 || this->players[index]->isSurrender) {
		return;
	}
	if (this->state != STATE_GAMEING) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"当前游戏状态无法弃牌（投降）！任意出牌后方可弃牌。";
		return;
	}

	//记录弃牌时间
	time_t rawtime;
	this->lastTime = time(&rawtime);

	Player *player = this->players[index];

	player->isSurrender = true;

	if (index == this->bossIndex) {
		this->whoIsWinner = 2;//农民赢
	}
	else {
		for (size_t i = 0; i < this->players.size(); i++) {
			if (players[i]->isSurrender && i != this->bossIndex && i != index) {
				this->whoIsWinner = 1;
				break;
			}
		}
	}


	if (this->whoIsWinner > 0) {
		this->sendWatchingMsg_Over();

		this->msg << L"斗地主游戏结束，";
		this->msg << (this->whoIsWinner == 1 ? L"地主赢了" : L"农民赢了");
		this->breakLine();

		//弃牌不检测春天

		this->msg << L"---------------";
		this->breakLine();
		this->msg << L"分数结算：";
		this->breakLine();
		this->listPlayers(3);

		casino.gameOver(this->number);
		return;
	}


	if (this->currentPlayIndex == index) {
		this->at(playNum);
		this->msg << L"弃牌，";

		this->setNextPlayerIndex();

		this->msg << L"现在轮到";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"出牌。";
		this->breakLine();
	}
	else {
		this->at(playNum);
		this->msg << L"弃牌。";
		this->breakLine();
	}

	//观战播报
	this->sendWatchingMsg_Surrender(playNum);
}

void Desk::openCard(int64_t playNum)
{
	int index = this->getPlayer(playNum);

	if (index == -1) {
		return;
	}
	if (this->state > STATE_READYTOGO || this->state < STATE_START) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"明牌只能在准备阶段使用！";
		return;
	}
	Player *player = this->players[index];

	if (!player->isOpenCard) {
		player->isOpenCard = true;
		this->multiple += 2;
	}

	this->at(playNum);
	this->msg << L"明牌，积分倍数+2。";
	this->breakLine();

	this->listCardsOnDesk(player);
	this->breakLine();

	this->msg << L"---------------";
	this->breakLine();
	this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;


}

void Desk::getPlayerInfo(int64_t playNum)
{
	this->msg << Admin::readDataType() << L" " << Admin::readVersion() << L" UTC";
	this->breakLine();
	this->at(playNum);
	this->msg << L"：";
	this->breakLine();
	this->msg << Admin::readWin(playNum) << L"胜"
		<< Admin::readLose(playNum) << L"负，";
	this->msg << L"积分";
	this->readSendScore(playNum);
	this->breakLine();
}

void Desk::getScore(int64_t playNum)
{
	this->at(playNum);
	this->breakLine();
	if (Admin::getScore(playNum)) {
		this->msg << L"这是今天的200点积分，祝你玩♂的开心！";
		this->breakLine();
		this->msg << L"你现在的积分总额为" << Admin::readScore(playNum) << L"，";
		this->msg << L"获取更多积分请明天再来或是和管理员";
		this->at((int64_t)Admin::readAdmin());
		this->msg << L"进行py交易！";
		this->breakLine();
	}
	else {
		this->msg << L"你今日已经拿过积分！";
		this->breakLine();
		this->msg << L"获取更多积分请明天再来或是和管理员";
		this->at((int64_t)Admin::readAdmin());
		this->msg << L"进行py交易！";
		this->breakLine();
	}
}

int64_t Desk::readScore(int64_t playNum) {
	int64_t hasScore = Admin::readScore(playNum);
	hasScore -= 500000000;
	return hasScore;
}

void Desk::readSendScore(int64_t playNum) {
	int64_t hasScore = Admin::readScore(playNum);
	hasScore -= 500000000;
	if (hasScore >= 0) {
		this->msg << hasScore;
	}
	else {
		this->msg << L"-" << -hasScore;
	}
}

void Desks::gameOver(int64_t number)
{
	int index = casino.getDesk(number);
	if (index == -1) {
		return;
	}
	vector<Desk*>::iterator it = casino.desks.begin() + index;
	casino.desks.erase(it); 
	//更新数据库版本
	//Admin::writeVersion();
	//Util::sendGroupMsg(number, "游戏结束。");
}

void Desk::setNextPlayerIndex()
{
	this->currentPlayIndex = this->currentPlayIndex == 2 ? 0 : this->currentPlayIndex + 1;

	if (this->currentPlayIndex == this->lastPlayIndex) {
		this->lastCard.clear();
		this->lastWeights->clear();
		this->lastCardType = L"";
	}

	//如果下一个该出牌的玩家正好弃牌了 则重新set下一位玩家
	//由于不可能大于2个人弃牌 所以下一个人一定没有弃牌
	if (this->players[this->currentPlayIndex]->isSurrender) {
		this->setNextPlayerIndex();
	}


}

void Desk::deal() {
	unsigned i, k, j;
	for (i = k = 0; i < 3; i++) {
		Player *player = players[i];
		//第二次发牌时需要消除所有已经发到的牌
		players[i]->card.clear();

		for (j = 0; j < 17; j++) {
			player->card.push_back(cards[k++]);
		}

		sort(player->card.begin(), player->card.end(), Util::compareCard);

		for (unsigned m = 0; m < player->card.size(); m++) {
			player->msg << L"[" << player->card.at(m) << L"]";
		}

		player->breakLine();
	}

}

int Desks::getDesk(int64_t deskNum) {

	for (unsigned i = 0; i < this->desks.size(); i++) {
		if (this->desks[i]->number == deskNum) {
			return i;
		}
	}

	return -1;
}

void Desk::join(int64_t playNum)
{

	int playIndex = this->getPlayer(playNum);

	if (playIndex != -1) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"你已经加入游戏！";
		this->breakLine();
		return;
	}

	if (this->players.size() == 3) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"很遗憾，人数已满！";
		this->breakLine();
		this->msg << L"但你可以[加入观战]！";
		this->breakLine();
		//this->joinWatching(playNum); 不再自动加入
		return;
	}

	//if (Admin::readScore(playNum) < 1) {
		//this->at(playNum);
		//this->breakLine();
		//this->msg << L"你的积分已经输光了，祝你在游戏中弯道超车！";
		//this->breakLine();
		//this->msg << L"系统自动为您尝试获取每日积分：";
		//this->breakLine();
		//this->msg << L"---------------";
		//this->breakLine();
		//this->getScore(playNum);
		//获取积分的最后有换行，不需要 this->breakLine();
		//this->msg << L"---------------";
		//this->breakLine();

		//如果领取失败，赠送5分
		//if (Admin::readScore(playNum) < 1) {
			//this->msg << L"系统赠送你5点积分，祝你弯道超车！";
			//Admin::addScore(playNum, 5);
			//return;
		//}
	//}

	Player *player = new Player;
	player->number = playNum;
	this->players.push_back(player);

	this->at(playNum);
	this->breakLine();
	this->msg << L"加入成功，已有玩家" << this->players.size() << L"个，分别是：";
	this->breakLine();
	for (unsigned i = 0; i < this->players.size(); i++) {
		this->msg << i + 1 << L"：";
		this->at(this->players[i]->number);
		this->msg << L"，" << Admin::readWin(this->players[i]->number) << L"胜"
			<< Admin::readLose(this->players[i]->number) << L"负";
		this->msg << L"，积分";
		this->readSendScore(this->players[i]->number);
		this->breakLine();
	}

	if (Admin::readScore(playNum) <= 0) {
		this->msg << L"---------------";
		this->breakLine();
		this->at(playNum);
		this->breakLine();
		this->msg << L"你的积分为";
		this->readSendScore(playNum);
		this->msg << L"，"; //在这里van游戏可能太♂弱而受到挫折！";
		//this->breakLine();
		this->msg << L"请多多练♂习以提高你的牌技，祝你弯道超车！";
		this->breakLine();
	}


	if (this->players.size() == 3) {
		this->breakLine();
		this->msg << L"人数已满，";
		this->msg << L"请输入[启动]或[GO]来启动游戏。";
		this->breakLine();
	}
}

void Desk::exit(int64_t number)
{
	if (this->state == STATE_WAIT) {
		int index = this->getPlayer(number);
		if (index != -1) {
			vector<Player*>::iterator it = this->players.begin() + index;
			this->players.erase(it);
			this->msg << L"退出成功，剩余玩家" << this->players.size() << L"个";
			if (this->players.size() > 0) {
				this->msg << L"，分别是：";
				this->breakLine();
				for (unsigned i = 0; i < this->players.size(); i++) {
					this->msg << i + 1 << L"：";
					this->at(this->players[i]->number);
					this->msg << L"，" << Admin::readWin(this->players[i]->number) << L"胜"
						<< Admin::readLose(this->players[i]->number) << L"负";
					this->msg << L"，积分";
					this->readSendScore(this->players[i]->number);
					this->breakLine();
				}
			}
			else {
				this->msg << L"。";
			}
		}

	}
	else {
		this->msg << L"游戏已经开始不能退出！";
		this->breakLine();
		this->msg << L"但你可以使用[弃牌]来放弃游戏！";
	}
}

void Desk::joinWatching(int64_t playNum) {
	int playIndex = this->getPlayer(playNum);
	int watchIndex = this->getWatcher(playNum);

	//非弃牌玩家不能观战
	if (playIndex != -1 && !players[playIndex]->isSurrender) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"你已经加入游戏，请不要作弊！";
		this->breakLine();
		return;
	}
	if (watchIndex != -1) {
		//this->at(playNum);
		//this->breakLine();
		//this->msg << L"你已经加入观战模式。";
		//this->breakLine();
		return;
	}
	if (this->players.size() < 3) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"游戏未开始或人数不足，当前无法加入观战模式。";
		this->breakLine();
		return;
	}

	Watcher *watcher = new Watcher;
	watcher->number = playNum;
	this->watchers.push_back(watcher);

	//this->at(playNum);
	//this->breakLine();
	//this->msg << L"加入观战模式成功。";

	sendWatchingMsg_Join(playNum);
}

void Desk::exitWatching(int64_t playNum) {
	int index = this->getWatcher(playNum);
	if (index != -1) {
		vector<Watcher*>::iterator it = this->watchers.begin() + index;
		this->watchers.erase(it);
		//this->at(playNum);
		//this->breakLine();
		//this->msg << L"退出观战模式成功。";
	}
}

void Desk::sendWatchingMsg_Join(int64_t joinNum) {
	int index = getWatcher(joinNum);
	Watcher *watcher = watchers[index];

	watcher->msg << L"加入观战模式成功。";
	watcher->breakLine();

	watcher->msg << L"---------------";
	watcher->breakLine();
	watcher->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
	watcher->breakLine();
	watcher->msg << L"当前手牌信息：";
	watcher->breakLine();
	for (unsigned j = 0; j < this->players.size(); j++) {
		watcher->msg << j + 1 << L"：";
		watcher->msg << L"[" << (j == this->bossIndex ? L"地主" : L"农民") << L"]";
		watcher->at(this->players[j]->number);
		watcher->breakLine();

		for (unsigned m = 0; m < players[j]->card.size(); m++) {
			watcher->msg << L"[" << players[j]->card.at(m) << L"]";
		}

		watcher->breakLine();
	}
	//不需要换行 watcher->breakLine();
}

void Desk::sendWatchingMsg_Start() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		if (!this->isSecondCallForBoss) {
			watcher->msg << L"斗地主游戏开始";
		}
		else {
			watcher->msg << L"重新发牌";
		}
		watcher->breakLine();

		//这里不需要this->setNextPlayerIndex();
		watcher->msg << L"---------------";
		//watcher->breakLine();
		//watcher->msg << L"第" << this->turn + 1 << L"回合：";
		watcher->breakLine();
		watcher->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
		watcher->breakLine();
		watcher->msg << L"初始手牌：";
		watcher->breakLine();
		for (unsigned j = 0; j < this->players.size(); j++) {
			watcher->msg << j + 1 << L"：";
			watcher->msg << L"[" << (j == this->bossIndex ? L"地主" : L"农民") << L"]";
			watcher->at(this->players[j]->number);
			watcher->breakLine();

			for (unsigned m = 0; m < players[j]->card.size(); m++) {
				watcher->msg << L"[" << players[j]->card.at(m) << L"]";
			}

			watcher->breakLine();
		}
		watcher->breakLine();
	}
}

void Desk::sendWatchingMsg_Play() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		//watcher->msg << L"上回合";
		watcher->at(this->players[currentPlayIndex]->number);
		watcher->msg << L"打出" << this->lastCardType;
		for (unsigned m = 0; m < this->lastCard.size(); m++) {
			watcher->msg << L"[" << this->lastCard.at(m) << L"]";
		}
		watcher->breakLine();

		//这里不需要this->setNextPlayerIndex();
		watcher->msg << L"---------------";
		//watcher->breakLine();
		//watcher->msg << L"第" << this->turn + 1 << L"回合：";
		watcher->breakLine();
		watcher->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
		watcher->breakLine();
		watcher->msg << L"当前剩余牌：";
		watcher->breakLine();
		for (unsigned j = 0; j < this->players.size(); j++) {
			watcher->msg << j + 1 << L"：";
			watcher->msg << L"[" << (j == this->bossIndex ? L"地主" : L"农民") << L"]";
			watcher->at(this->players[j]->number);
			watcher->breakLine();

			for (unsigned m = 0; m < players[j]->card.size(); m++) {
				watcher->msg << L"[" << players[j]->card.at(m) << L"]";
			}

			watcher->breakLine();
		}
		watcher->breakLine();
		watcher->msg << L"现在轮到";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"出牌。";
		watcher->breakLine();
	}
}

void Desk::sendWatchingMsg_Pass(int64_t playNum) {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(playNum);
		watcher->msg << L"过牌，";

		watcher->msg << L"现在轮到";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"出牌。";
		watcher->breakLine();
	}
}

void Desk::sendWatchingMsg_Surrender(int64_t playNum) {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(playNum);
		watcher->msg << L"弃牌，";

		watcher->msg << L"现在轮到";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"出牌。";
		watcher->breakLine();
	}
}

void Desk::sendWatchingMsg_Over() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->msg << L"斗地主游戏结束，";
		watcher->msg << (this->whoIsWinner == 1 ? L"地主赢了" : L"农民赢了");
		watcher->breakLine();
		watcher->msg << L"退出观战模式。";
		watcher->breakLine();
	}
}

Desk* Desks::getOrCreatDesk(int64_t deskNum) {

	Desk *desk = NULL;
	int deskIndex = getDesk(deskNum);
	if (deskIndex == -1) {//没有桌子
		desk = new Desk;
		desk->number = deskNum;
		desks.push_back(desk);
	}
	else {
		desk = casino.desks[deskIndex];
	}

	return desk;
}

void Desk::startGame() {
	if (this->players.size() == 3 && this->state == STATE_WAIT) {
		this->state = STATE_START;

		int64_t maxScore = -500000001;
		int64_t minScore = 500000001;
		int64_t tempScore = 0;

		//暂定：最大最低分每差50分标准分加3。
		for (unsigned i = 0; i < this->players.size(); i++) {
			Admin::addScore(this->players[i]->number, CONFIG_PLAY_BONUS);

			tempScore = Admin::readScore(this->players[i]->number);
			if (tempScore > maxScore) {
				maxScore = tempScore;
			}
			if (tempScore < minScore) {
				minScore = tempScore;
			}
		}

		this->basic += CONFIG_BOTTOM_SCORE * (maxScore-minScore) / 50;
		if (this->basic > CONFIG_TOP_SCORE) {
			this->basic = CONFIG_TOP_SCORE; //最大单局基本分1000分
		}

		this->msg << L"游戏开始，桌号：" << this->number << L"。";
		this->breakLine(); 
		//重复提示，已删除
		//this->msg << L"本局积分：" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
		//this->breakLine(); 

		//this->msg << L"准备环节可以进行[明牌]操作，明牌会使积分倍数 + 2，请谨慎操作！";
		//this->breakLine();
		this->msg << L"---------------";
		this->breakLine();

		this->listPlayers(1);

		this->shuffle();

		this->deal();

		this->creataBoss();
	}
	else {
		if (this->state >= STATE_BOSSING) {
			//启动游戏往往会多次发送出发提示，减少多次无效提示
			//this->msg << L"已经开始游戏。";
			//this->breakLine();
			//this->listPlayers(1);
			return;
		}
		else {
			this->msg << L"没有足够的玩家。";
			this->breakLine();
			for (unsigned i = 0; i < this->players.size(); i++) {
				this->msg << i + 1 << L"：";
				this->at(this->players[i]->number);
				this->msg << L"，" << Admin::readWin(this->players[i]->number) << L"胜"
					<< Admin::readLose(this->players[i]->number) << L"负";
				this->msg << L"，积分";
				this->readSendScore(this->players[i]->number);
				this->breakLine();
			}
		}
	}
}

void Desk::listCardsOnDesk(Player* player)
{
	for (unsigned m = 0; m < player->card.size(); m++) {
		this->msg << L"[" << player->card.at(m) << L"]";
	}
}

void Desk::AFKHandle(int64_t playNum) {
	time_t rawtime;
	int64_t timeNow = time(&rawtime);

	if (this->state < STATE_GAMEING) {
		return;
	}

	if (timeNow - this->lastTime > 60) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"举报成功。";
		this->breakLine();

		Admin::addScore(this->players[this->currentPlayIndex]->number, -CONFIG_SURRENDER_PENALTY);
		Admin::addScore(playNum, CONFIG_SURRENDER_PENALTY);

		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"[" << L"挂机-" << CONFIG_SURRENDER_PENALTY << L"分]";
		this->breakLine();
		this->at(playNum);
		this->msg << L"[" << L"举报+" << CONFIG_SURRENDER_PENALTY << L"分]";
		this->breakLine();


		this->setNextPlayerIndex();
		this->lastTime = timeNow;

		this->msg << L"现在轮到";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"出牌。";
		this->breakLine();
	}
	else {
		this->at(playNum);
		this->breakLine();
		this->msg << L"举报失败，";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"的剩余出牌时间为" << this->lastTime + 60 - timeNow << L"秒。";
	}
}