#pragma once

#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tchar.h>  
#include <regex> 
#include <thread>

#include "landlords_classes.h"
using namespace std;

Desk::Desk() {
	for (int i = 0; i < 54; i++) {
		this->cards[i] = cardDest[i];
	}

	this->state = STATE_WAIT;
	this->lastPlayIndex = -1;//��ǰ˭������
	this->currentPlayIndex = -1;//��˭����
	this->bossIndex = -1;//˭�ǵ���
	this->isSecondCallForBoss = false;

	this->bossCount = 0;
	this->farmCount = 0;

	vector<wstring> lastCard;//��λ��ҵ���
	this->lastCardType = L"";//��λ��ҵ�����
	this->lastWeights = new vector<int>;//��λ��ҵ���

	this->whoIsWinner = 0;
	this->multiple = 1;
	this->basic = CONFIG_INIT_SCORE; //��ֹ����0�ֵ�����ֻ���0
	this->turn = 0;
	this->lastTime = 0; //���һ�η���ʱ�䣬��¼��
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

	int64_t score = this->basic * this->multiple;
	int64_t halfScore = score / 2;
	
	if (this->players.size() < 3) {
		this->msg << L"��Ϸ��δ��ʼ������б�";
		this->breakLine();
	}
	else {
		this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
		this->breakLine();
		this->msg << L"���ƴ�����" << this->turn;
		this->breakLine();
	}

	for (unsigned i = 0; i < this->players.size(); i++) {
		this->msg << i + 1 << L"��";
		if (hasType) {
			this->msg << L"[" << (i == this->bossIndex && state == STATE_GAMING ? L"����" : L"ũ��") << L"]";
		}

		this->at(this->players[i]->number);
		if (hasWin) {
			if (this->whoIsWinner == 2) {//�����ũ��Ӯ��
				if (i == this->bossIndex) {
					this->msg << L"[" << L"ʧ��-" << score << L"��]";
					Admin::addScore(this->players[i]->number, -score);
					Admin::addLose(this->players[i]->number);

					this->breakLine();
					this->msg << L"ʣ�����ƣ�";
					this->listCardsOnDesk(this->players[i]);
				}
				else if (this->players[i]->isSurrender) {
					this->msg << L"[" << L"����-" << CONFIG_SURRENDER_PENALTY << L"��]";
					Admin::addScore(this->players[i]->number, -CONFIG_SURRENDER_PENALTY);
					Admin::addWin(this->players[i]->number);
				}
				else {
					this->msg << L"[" << L"ʤ��+" << halfScore << L"��]";
					Admin::addScore(this->players[i]->number, halfScore);
					Admin::addWin(this->players[i]->number); 
					
					//��������ƣ��͹�������
					if (this->players[i]->card.size() > 0) {
						this->breakLine();
						this->msg << L"ʣ�����ƣ�";
						this->listCardsOnDesk(this->players[i]);
					}
				}
			}
			else {
				if (i == this->bossIndex) {
					this->msg << L"[" << L"ʤ��+" << score << L"��]";
					Admin::addScore(this->players[i]->number, score);
					Admin::addWin(this->players[i]->number);
				}
				else if (this->players[i]->isSurrender) {
					this->msg << L"[" << L"����-" << score+CONFIG_SURRENDER_PENALTY << L"��]";
					Admin::addScore(this->players[i]->number, - score - CONFIG_SURRENDER_PENALTY);
					Admin::addLose(this->players[i]->number);
				}
				else {
					this->msg << L"[" << L"ʧ��-" << halfScore << L"��]";
					Admin::addScore(this->players[i]->number, -halfScore);
					Admin::addLose(this->players[i]->number);

					this->breakLine();
					this->msg << L"ʣ�����ƣ�";
					this->listCardsOnDesk(this->players[i]);
				}
			}
		}

		this->breakLine();
	}
}

bool Desk::isCanWin(int cardCount, vector<int> *weights, wstring type)
{

	if (type == L"" || this->lastCardType == L"��ը") {
		return false;
	}

	if (this->lastCardType == L"") {
		return true;
	}

	if (type == L"��ը") {
		return true;
	}
	if (type == L"ը��" && type != this->lastCardType) {
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

	if (cardCount == 2 && Util::findFlag(list[0]) + Util::findFlag(list[1]) == 27) {//��ը
		return L"��ը";
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

		if (Util::findFlag(list[i]) == 12) {	//��12�������2
			no2InCards = false;
		}
	}

	int max = counts[0];//��Ŵ�ֵ
	int min = counts[0];//���Сֵ
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
	if (cardCount == 1) {//����
		weights->push_back(Util::findFlag(cards[0]));
		return L"����";
	}
	if (cardCount == 2 && max == 2) {//����
		weights->push_back(Util::findFlag(cards[0]));
		return L"����";
	}
	if (cardCount == 3 && max == 3) {//����
		weights->push_back(Util::findFlag(cards[0]));
		return L"����";
	}
	if (cardCount == 4 && max == 4) {//ը��
		weights->push_back(Util::findFlag(cards[0]));
		return L"ը��";
	}

	if (cardCount == 4 && max == 3) {//3��1

		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"3��1";
	}

	if (cardCount == 5 && max == 3 && min == 2) {//3��2
		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"3��2";
	}

	if (cardCount == 6 && max == 4) {//4��2
		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"4��2";
	}

	if (cardGroupCout > 2 && max == 2 && min == 2
		&& Util::findFlag(cards[0]) == Util::findFlag(cards[cardGroupCout - 1]) - cardGroupCout + 1
		&& Util::findFlag(cards[cardGroupCout - 1]) < 13
		&& no2InCards	//���Բ�����2
		) {//����
		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"����";
	}

	//for (unsigned i = 0; i < cardGroupCout; i++) {
	//	if (cards[cardGroupCout] == L"2") {
	//		no2InCards = false;
	//	}
	//}

	if (cardGroupCout > 4 && max == 1 && min == 1
		&& Util::findFlag(cards[0]) == Util::findFlag(cards[cardGroupCout - 1]) - cardGroupCout + 1
		&& Util::findFlag(cards[cardGroupCout - 1]) < 13
		&& no2InCards	//���˶�����˳�Ӳ��ܴ�2
		) {//˳��
		for (unsigned i = 0; i < tmpCount.size(); i++) {
			int tmp = tmpCount[i];
			for (unsigned m = 0; m < counts.size(); m++) {
				if (counts[m] == tmp) {
					weights->push_back(Util::findFlag(cards[m]));
					counts[m] = -1;
				}
			}
		}

		return L"˳��";
	}

	//�ɻ�
	int  planeCount = 0;
	for (unsigned i = 0; i < counts.size(); i++) {
		if (counts[i] >= 3) {
			planeCount++;
		}
	}
	if (planeCount > 1) {
		wstring tmp;
		if (cardCount == planeCount * 3) {
			tmp = L"�ɻ�";
		}
		else if (cardCount == planeCount * 4) {
			tmp = L"�ɻ������";
		}
		else if (cardCount == planeCount * 5 && min == 2) {
			tmp = L"�ɻ���˫���";
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

	//��¼ʱ��
	time_t rawtime;
	this->lastTime = time(&rawtime);
	//��ֹbug
	this->warningSent = false;

	this->msg << L"����������������ڣ�����ʱ��ʼ��";
	this->breakLine();
	this->msg << L"---------------";
	this->breakLine();

	int index = rand() % 3;

	this->bossIndex = index;
	this->currentPlayIndex = index;
	this->at(this->players[index]->number);
	this->breakLine();
	this->msg << L"���Ƿ�Ҫ��������";
	this->breakLine();
	this->msg << L"����[��]��[��(��)]���ش�";
	this->breakLine();
	//ս������
	this->sendWatchingMsg_Start();
}

void Desk::getBoss(int64_t playerNum)
{
	int index = this->getPlayer(playerNum);
	if (this->state == STATE_BOSSING && this->currentPlayIndex == index) {

		//��¼ʱ��
		time_t rawtime;
		this->lastTime = time(&rawtime);
		//��ֹ��ʾ������������bug
		this->warningSent = false;

		this->bossIndex = index;
		this->currentPlayIndex = index;
		this->lastPlayIndex = index;
		sendBossCard();

		//����ӱ�����
		this->state = STATE_MULTIPLING;
		this->multipleChoice();
	}
}

void Desk::dontBoss(int64_t playerNum)
{
	int index = this->getPlayer(playerNum);
	if (this->state == STATE_BOSSING && this->currentPlayIndex == index) {

		this->setNextPlayerIndex();

		//��¼ʱ��
		time_t rawtime;
		this->lastTime = time(&rawtime);
		//��ֹ��ʾ��������bug
		this->warningSent = false;

		if (this->currentPlayIndex == this->bossIndex && this->isSecondCallForBoss) {
			this->msg << L"��2��������ʧ�ܣ�";
			this->sendBossCard();
			this->state = STATE_MULTIPLING;
			this->multipleChoice();
			return;
		}
		else if (this->currentPlayIndex == this->bossIndex) {
			this->msg << L"��1��������ʧ�ܣ����·��ơ�";
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
			this->msg << L"����������";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			this->at(this->players[currentPlayIndex]->number);
			this->breakLine();
			this->msg << L"���Ƿ�Ҫ��������";
			this->breakLine();
			this->msg << L"����[��]��[��(��)]���ش�";
			this->breakLine();
		}
	}
}

void Desk::sendBossCard()
{
	Player *playerBoss = players[this->bossIndex];

	this->at(playerBoss->number);
	this->msg << L"�ǵ����������ǣ�";
	this->msg << L"[" << this->cards[53] << L"]"
		<< L"[" << this->cards[52] << L"]"
		<< L"[" << this->cards[51] << L"]��";
	this->breakLine();
	this->msg << L"---------------";
	this->breakLine();

	for (int i = 0; i < 3; i++) {
		playerBoss->card.push_back(cards[53 - i]);
	}
	sort(playerBoss->card.begin(), playerBoss->card.end(), Util::compareCard);

	//playerBoss->msg << L"���ǵ������յ����ƣ�";
	//playerBoss->breakLine();
	for (unsigned m = 0; m < playerBoss->card.size(); m++) {
		playerBoss->msg << L"[" << playerBoss->card.at(m) << L"]";
	}
	playerBoss->breakLine();

	//�����״̬�����ƶ����˼ӱ�(dontMultiple)�����һ����������
}

void Desk::multipleChoice() {
	//��¼ʱ��
	time_t rawtime;
	this->lastTime = time(&rawtime);
	//��ֹbug
	this->warningSent = false;

	this->msg << L"���������ڽ������������ӱ����ڡ�";
	this->breakLine();
	this->msg << L"---------------";
	this->breakLine();
	this->at(this->players[this->bossIndex]->number);
	this->breakLine();
	this->msg << L"���Ƿ�Ҫ�ӱ���";
	this->breakLine();
	this->msg << L"����[��]��[��(��)]���ش�";
	this->breakLine();
}

void Desk::getMultiple(int64_t playerNum)
{
	int index = this->getPlayer(playerNum);
	if (this->state == STATE_MULTIPLING && this->currentPlayIndex == index) {
		this->multiple += 1;

		//��¼ʱ��
		time_t rawtime;
		this->lastTime = time(&rawtime);
		//��ֹ��ʾ��ӱ�����bug
		this->warningSent = false;

		this->setNextPlayerIndex();

		if (this->currentPlayIndex == this->bossIndex && bossHasMultipled) {
			this->at(this->players[index]->number);
			this->msg << L"Ҫ�ӱ���";
			this->breakLine();
			this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();

			this->state = STATE_READYTOGO;

			this->msg << L"�ӱ����ڽ�������������ʽ��ʼ��";
			this->breakLine();
			this->msg << L"---------------";
			//this->breakLine();
			//this->msg << L"��" << this->turn + 1 << L"�غϣ�ʣ����������";
			this->breakLine();
			for (unsigned i = 0; i < this->players.size(); i++) {
				this->msg << i + 1 << L"��";
				this->msg << L"[" << (i == this->bossIndex ? L"����" : L"ũ��") << L"]"; //����ɾ������&& state == STATE_GAMING 
				this->at(this->players[i]->number);
				this->msg << L"��" << static_cast<int>(this->players[i]->card.size());
				this->breakLine();
			}
			this->breakLine();
			this->msg << L"�����";
			this->at(this->players[this->bossIndex]->number);
			this->msg << L"�ȳ��ơ�";
			this->breakLine();
			//ս������
			this->sendWatchingMsg_Start();
		}
		else {
			bossHasMultipled = true;

			this->at(this->players[index]->number);
			this->msg << L"Ҫ�ӱ���";
			this->breakLine();
			this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			this->at(this->players[currentPlayIndex]->number);
			this->msg << L"���Ƿ�Ҫ�ӱ���";
			this->breakLine();
			this->msg << L"����[��]��[��(��)]���ش�";
			this->breakLine();
		}
	}
}

void Desk::dontMultiple(int64_t playerNum)
{
	int index = this->getPlayer(playerNum);
	if (this->state == STATE_MULTIPLING && this->currentPlayIndex == index) {

		this->setNextPlayerIndex();

		//��¼ʱ��
		time_t rawtime;
		this->lastTime = time(&rawtime);
		//��ֹ��ʾ�󲻼ӱ�����bug
		this->warningSent = false;

		if (this->currentPlayIndex == this->bossIndex && bossHasMultipled) {
			this->at(this->players[index]->number);
			this->msg << L"��Ҫ�ӱ���";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();

			this->state = STATE_READYTOGO;

			this->msg << L"�ӱ����ڽ�������������ʽ��ʼ��";
			this->breakLine();
			this->msg << L"---------------";
			//this->breakLine();
			//this->msg << L"��" << this->turn + 1 << L"�غϣ�";
			this->breakLine();
			this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
			this->breakLine();
			this->msg << L"ʣ����������";
			this->breakLine();
			for (unsigned i = 0; i < this->players.size(); i++) {
				this->msg << i + 1 << L"��";
				this->msg << L"[" << (i == this->bossIndex ? L"����" : L"ũ��") << L"]"; //����ɾ������&& state == STATE_GAMING 
				this->at(this->players[i]->number);
				this->msg << L"��" << static_cast<int>(this->players[i]->card.size());
				this->breakLine();
			}
			this->breakLine();
			this->msg << L"�����";
			this->at(this->players[this->bossIndex]->number);
			this->msg << L"�ȳ��ơ�";
			this->breakLine();
			//ս������
			this->sendWatchingMsg_Start();
		}
		else {
			bossHasMultipled = true;

			this->at(this->players[index]->number);
			this->msg << L"��Ҫ�ӱ���";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			this->at(this->players[currentPlayIndex]->number);
			this->breakLine();
			this->msg << L"���Ƿ�Ҫ�ӱ���";
			this->breakLine();
			this->msg << L"����[��]��[��(��)]���ش�";
			this->breakLine();
		}
	}
}

void Desk::play(int64_t playNum, wstring msg)
{
	int playIndex = this->getPlayer(playNum);
	int length = msg.length();

	if (playIndex == -1 || playIndex != this->currentPlayIndex
		|| (!(this->state == STATE_GAMING && this->turn > 0)
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
			this->msg << L"�涪�ˣ����û����Ҫ�����ƣ��᲻���棿";
			return;
		}
	}

	vector<int> *weights = new vector<int>;

	wstring type = this->getMycardType(list, weights);

	bool isCanWin = this->isCanWin(cardCount, weights, type);

	if (isCanWin) {
		if (this->turn == 0) {
			this->state = STATE_GAMING;
		}

		//ֻ�кϷ��ĳ��Ʋ��ܼ�¼
		//������ƴ���
		if (currentPlayIndex == this->bossIndex) {
			this->bossCount++;
		}
		else {
			this->farmCount++;
		}

		//��¼����ʱ��
		time_t rawtime;
		this->lastTime = time(&rawtime);
		//��ֹ��ʾ����Ƴ���bug
		this->warningSent = false;

		player->card = mycardTmp;
		this->lastWeights = weights;
		this->lastCard = list;
		this->lastCardType = type;
		this->lastPlayIndex = this->currentPlayIndex;
		this->turn++;


		//�������
		if (type == L"��ը") {
			this->multiple += 2;

			this->msg << L"�����ը�����ֱ���+2";
			this->breakLine();
			this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
		}
		else if (type == L"ը��") {
			this->multiple += 1;

			this->msg << L"���ը�������ֱ���+1";
			this->breakLine();
			this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
		}

		if (mycardTmp.size() == 0) {//Ӯ�ˡ�
			this->whoIsWinner = this->bossIndex == this->currentPlayIndex ? 1 : 2;

			this->sendWatchingMsg_Over();

			this->msg << L"��������Ϸ������";
			this->msg << (this->whoIsWinner == 1 ? L"����Ӯ��" : L"ũ��Ӯ��");
			this->breakLine();


			if (this->farmCount == 0 && this->whoIsWinner == 1) {
				this->multiple *= 2;
				this->msg << L"---------------";
				this->breakLine();
				this->msg << L"���ֳ��ִ��죬���ֱ���x2";
				this->breakLine();
				this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
				this->breakLine();
			}
			else if (this->bossCount == 1 && this->whoIsWinner == 2) {
				this->multiple *= 2;
				this->msg << L"---------------";
				this->breakLine();
				this->msg << L"���ֳ��ַ����죬���ֱ���x2";
				this->breakLine();
				this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
				this->breakLine();
			}


			this->msg << L"---------------";
			this->breakLine();
			this->msg << L"�������㣺";
			this->breakLine();
			this->listPlayers(3);

			casino.gameOver(this->number);
			return;
		}

		player->listCards();

		if (player->isOpenCard) {
			this->at(player->number);
			this->msg << L"���ƣ�";
			this->listCardsOnDesk(player);
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
		}

		if (player->card.size() < 3) {
			this->msg << L"��ɫ��������ɫ������";
			this->breakLine();
			this->at(player->number);
			this->msg << L"��ʣ��" << player->card.size() << L"���ƣ�";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
		}

		//this->msg << L"�ϻغ�";
		this->at(this->players[currentPlayIndex]->number);
		this->msg << L"���" << this->lastCardType;
		for (unsigned m = 0; m < this->lastCard.size(); m++) {
			this->msg << L"[" << this->lastCard.at(m) << L"]";
		}
		this->breakLine();

		//��ս������������ת��ս����������һλ��ң����������Ϣ����
		this->sendWatchingMsg_Play();

		this->setNextPlayerIndex();

		this->msg << L"---------------";
		//this->breakLine();
		//this->msg << L"��" << this->turn + 1 << L"�غϣ�";
		this->breakLine();
		this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;
		this->breakLine();
		this->msg << L"ʣ����������";
		this->breakLine();
		for (unsigned i = 0; i < this->players.size(); i++) {
			this->msg << i + 1 << L"��";
			this->msg << L"[" << (i == this->bossIndex && state == STATE_GAMING ? L"����" : L"ũ��") << L"]";
			this->at(this->players[i]->number);
			this->msg << L"��" << static_cast<int>(this->players[i]->card.size());
			this->breakLine();
		}
		this->breakLine();
		this->msg << L"�����ֵ�";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"���ơ�";
		this->breakLine();
	}
	else {
		this->at(this->players[this->currentPlayIndex]->number);
		this->breakLine();
		this->msg << L"ɵ�����ѣ����ʲô�������⣡ѧ������ٴ�";
		this->breakLine();
	}
}

void Desk::discard(int64_t playNum)
{
	if (this->currentPlayIndex != this->getPlayer(playNum) || this->state != STATE_GAMING) {
		return;
	}

	if (this->currentPlayIndex == this->lastPlayIndex) {
		this->msg << L"�����������ã��᲻���棬�㲻�ܹ����ˣ����ˣ�";
		return;
	}

	//��¼����ʱ��
	time_t rawtime;
	this->lastTime = time(&rawtime);
	//��ֹ��ʾ����Ƴ���bug
	this->warningSent = false;

	this->at(playNum);
	this->msg << L"���ƣ�";

	this->setNextPlayerIndex();

	this->msg << L"�����ֵ�";
	this->at(this->players[this->currentPlayIndex]->number);
	this->msg << L"���ơ�";
	this->breakLine();

	//��ս����
	this->sendWatchingMsg_Pass(playNum);
}

void Desk::surrender(int64_t playNum)
{
	int index = this->getPlayer(playNum);
	if (index == -1 || this->players[index]->isSurrender) {
		return;
	}
	if (this->state != STATE_GAMING) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"��ǰ��Ϸ״̬�޷����ƣ�Ͷ������������ƺ󷽿����ơ�";
		return;
	}

	//��¼����ʱ��
	time_t rawtime;
	this->lastTime = time(&rawtime);
	//��ֹ��ʾ�����Ƴ���bug
	this->warningSent = false;

	Player *player = this->players[index];

	player->isSurrender = true;

	if (index == this->bossIndex) {
		this->whoIsWinner = 2;//ũ��Ӯ
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

		this->msg << L"��������Ϸ������";
		this->msg << (this->whoIsWinner == 1 ? L"����Ӯ��" : L"ũ��Ӯ��");
		this->breakLine();

		//���Ʋ���ⴺ��

		this->msg << L"---------------";
		this->breakLine();
		this->msg << L"�������㣺";
		this->breakLine();
		this->listPlayers(3);

		casino.gameOver(this->number);
		return;
	}


	if (this->currentPlayIndex == index) {
		this->at(playNum);
		this->msg << L"���ƣ�";

		this->setNextPlayerIndex();

		this->msg << L"�����ֵ�";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"���ơ�";
		this->breakLine();
	}
	else {
		this->at(playNum);
		this->msg << L"���ơ�";
		this->breakLine();
	}

	//��ս����
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
		this->msg << L"����ֻ����׼���׶�ʹ�ã�";
		return;
	}
	Player *player = this->players[index];

	if (!player->isOpenCard) {
		player->isOpenCard = true;
		this->multiple += 2;
	}

	this->at(playNum);
	this->msg << L"���ƣ����ֱ���+2��";
	this->breakLine();

	this->listCardsOnDesk(player);
	this->breakLine();

	this->msg << L"---------------";
	this->breakLine();
	this->msg << L"���ֻ��֣�" <<  this->basic*this->multiple;


}

void Desk::getPlayerInfo(int64_t playNum)
{
	this->msg << Admin::readDataType() << L" " << Admin::readVersion();// << L" CST";
	this->breakLine();
	this->at(playNum);
	this->msg << L"��";
	//this->breakLine();
	this->msg << Admin::readWin(playNum) << L"ʤ"
		<< Admin::readLose(playNum) << L"����";
	this->msg << L"����";
	this->readSendScore(playNum);
	this->breakLine();
}

void Desk::getScore(int64_t playNum)
{
	this->at(playNum);
	this->breakLine();
	if (Admin::getScore(playNum)) {
		this->msg << L"���ǽ����200����֣�ף�����Ŀ��ģ�";
		this->breakLine();
		this->msg << L"�����ڵĻ����ܶ�Ϊ" << Admin::readScore(playNum) << L"��";
		this->msg << L"��ȡ��������������������Ǻ͹���Ա";
		this->at((int64_t)Admin::readAdmin());
		this->msg << L"����py���ף�";
		this->breakLine();
	}
	else {
		this->msg << L"������Ѿ��ù����֣�";
		this->breakLine();
		this->msg << L"��ȡ��������������������Ǻ͹���Ա";
		this->at((int64_t)Admin::readAdmin());
		this->msg << L"����py���ף�";
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

	//���ٹһ�������
	casino.desks[index]->state = STATE_OVER;
	casino.desks[index]->counter->join();

	vector<Desk*>::iterator it = casino.desks.begin() + index;
	casino.desks.erase(it); 
	//�������ݿ�汾
	//Admin::writeVersion();
	//Util::sendGroupMsg(number, "��Ϸ������");
}

void Desk::setNextPlayerIndex()
{
	this->currentPlayIndex = this->currentPlayIndex == 2 ? 0 : this->currentPlayIndex + 1;

	if (this->currentPlayIndex == this->lastPlayIndex) {
		this->lastCard.clear();
		this->lastWeights->clear();
		this->lastCardType = L"";
	}

	//�����һ���ó��Ƶ�������������� ������set��һλ���
	//���ڲ����ܴ���2�������� ������һ����һ��û������
	if (this->players[this->currentPlayIndex]->isSurrender) {
		this->setNextPlayerIndex();
	}


}

void Desk::deal() {
	unsigned i, k, j;
	for (i = k = 0; i < 3; i++) {
		Player *player = players[i];
		//�ڶ��η���ʱ��Ҫ���������Ѿ���������
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
		this->msg << L"���Ѿ�������Ϸ��";
		this->breakLine();
		return;
	}

	if (this->players.size() == 3) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"���ź�������������";
		this->breakLine();
		this->msg << L"�������[�����ս]��";
		this->breakLine();
		//this->joinWatching(playNum); �����Զ�����
		return;
	}

	//if (Admin::readScore(playNum) < 1) {
		//this->at(playNum);
		//this->breakLine();
		//this->msg << L"��Ļ����Ѿ�����ˣ�ף������Ϸ�����������";
		//this->breakLine();
		//this->msg << L"ϵͳ�Զ�Ϊ�����Ի�ȡÿ�ջ��֣�";
		//this->breakLine();
		//this->msg << L"---------------";
		//this->breakLine();
		//this->getScore(playNum);
		//��ȡ���ֵ�����л��У�����Ҫ this->breakLine();
		//this->msg << L"---------------";
		//this->breakLine();

		//�����ȡʧ�ܣ�����5��
		//if (Admin::readScore(playNum) < 1) {
			//this->msg << L"ϵͳ������5����֣�ף�����������";
			//Admin::addScore(playNum, 5);
			//return;
		//}
	//}

	Player *player = new Player;
	player->number = playNum;
	this->players.push_back(player);

	this->at(playNum);
	this->breakLine();
	this->msg << L"����ɹ����������" << this->players.size() << L"�����ֱ��ǣ�";
	this->breakLine();
	for (unsigned i = 0; i < this->players.size(); i++) {
		this->msg << i + 1 << L"��";
		this->at(this->players[i]->number);
		this->msg << L"��" << Admin::readWin(this->players[i]->number) << L"ʤ"
			<< Admin::readLose(this->players[i]->number) << L"��";
		this->msg << L"������";
		this->readSendScore(this->players[i]->number);
		this->breakLine();
	}

	if (Admin::readScore(playNum) <= 0) {
		this->msg << L"---------------";
		this->breakLine();
		this->at(playNum);
		this->breakLine();
		this->msg << L"��Ļ���Ϊ";
		this->readSendScore(playNum);
		this->msg << L"��"; //������van��Ϸ����̫�������ܵ����ۣ�";
		//this->breakLine();
		this->msg << L"��������ϰ���������Ƽ���ף�����������";
		this->breakLine();
	}


	if (this->players.size() == 3) {
		this->breakLine();
		this->msg << L"����������";
		this->msg << L"������[����]��[GO]��������Ϸ��";
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
			this->msg << L"�˳��ɹ���ʣ�����" << this->players.size() << L"��";
			if (this->players.size() > 0) {
				this->msg << L"���ֱ��ǣ�";
				this->breakLine();
				for (unsigned i = 0; i < this->players.size(); i++) {
					this->msg << i + 1 << L"��";
					this->at(this->players[i]->number);
					this->msg << L"��" << Admin::readWin(this->players[i]->number) << L"ʤ"
						<< Admin::readLose(this->players[i]->number) << L"��";
					this->msg << L"������";
					this->readSendScore(this->players[i]->number);
					this->breakLine();
				}
			}
			else {
				this->msg << L"��";
			}
		}

	}
	else {
		this->msg << L"��Ϸ�Ѿ���ʼ�����˳���";
		this->breakLine();
		this->msg << L"�������ʹ��[����]��������Ϸ��";
	}
}

void Desk::joinWatching(int64_t playNum) {
	int playIndex = this->getPlayer(playNum);
	int watchIndex = this->getWatcher(playNum);

	//��������Ҳ��ܹ�ս
	if (playIndex != -1 && !players[playIndex]->isSurrender) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"���Ѿ�������Ϸ���벻Ҫ���ף�";
		this->breakLine();
		return;
	}
	if (watchIndex != -1) {
		//this->at(playNum);
		//this->breakLine();
		//this->msg << L"���Ѿ������սģʽ��";
		//this->breakLine();
		return;
	}
	if (this->players.size() < 3) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"��Ϸδ��ʼ���������㣬��ǰ�޷������սģʽ��";
		this->breakLine();
		return;
	}

	Watcher *watcher = new Watcher;
	watcher->number = playNum;
	this->watchers.push_back(watcher);

	//this->at(playNum);
	//this->breakLine();
	//this->msg << L"�����սģʽ�ɹ���";

	sendWatchingMsg_Join(playNum);
}

void Desk::exitWatching(int64_t playNum) {
	int index = this->getWatcher(playNum);
	if (index != -1) {
		vector<Watcher*>::iterator it = this->watchers.begin() + index;
		this->watchers.erase(it);
		//this->at(playNum);
		//this->breakLine();
		//this->msg << L"�˳���սģʽ�ɹ���";
	}
}

void Desk::sendWatchingMsg_Join(int64_t joinNum) {
	int index = getWatcher(joinNum);
	Watcher *watcher = watchers[index];

	watcher->msg << L"�����սģʽ�ɹ���";
	watcher->breakLine();

	watcher->msg << L"---------------";
	//watcher->breakLine();
	//watcher->msg << L"���ֻ��֣�" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
	watcher->breakLine();
	watcher->msg << L"��ǰ������Ϣ��";
	watcher->breakLine();
	for (unsigned j = 0; j < this->players.size(); j++) {
		watcher->msg << j + 1 << L"��";
		watcher->msg << L"[" << (j == this->bossIndex ? L"����" : L"ũ��") << L"]";
		watcher->at(this->players[j]->number);
		watcher->breakLine();

		for (unsigned m = 0; m < players[j]->card.size(); m++) {
			watcher->msg << L"[" << players[j]->card.at(m) << L"]";
		}

		watcher->breakLine();
	}
	//����Ҫ���� watcher->breakLine();
}

void Desk::sendWatchingMsg_Start() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		if (!this->isSecondCallForBoss) {
			watcher->msg << L"��������Ϸ��ʼ";
		}
		else {
			watcher->msg << L"���·���";
		}
		watcher->breakLine();

		//���ﲻ��Ҫthis->setNextPlayerIndex();
		watcher->msg << L"---------------";
		//watcher->breakLine();
		//watcher->msg << L"��" << this->turn + 1 << L"�غϣ�";
		//watcher->breakLine();
		//watcher->msg << L"���ֻ��֣�" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
		watcher->breakLine();
		watcher->msg << L"��ʼ���ƣ�";
		watcher->breakLine();
		for (unsigned j = 0; j < this->players.size(); j++) {
			watcher->msg << j + 1 << L"��";
			watcher->msg << L"[" << (j == this->bossIndex ? L"����" : L"ũ��") << L"]";
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

		//watcher->msg << L"�ϻغ�";
		watcher->at(this->players[currentPlayIndex]->number);
		watcher->msg << L"���" << this->lastCardType;
		for (unsigned m = 0; m < this->lastCard.size(); m++) {
			watcher->msg << L"[" << this->lastCard.at(m) << L"]";
		}
		watcher->breakLine();

		//���ﲻ��Ҫthis->setNextPlayerIndex();
		watcher->msg << L"---------------";
		//watcher->breakLine();
		//watcher->msg << L"��" << this->turn + 1 << L"�غϣ�";
		//watcher->breakLine();
		//watcher->msg << L"���ֻ��֣�" << this->multiple << L" x " << this->basic << L" = " << this->basic*this->multiple;
		watcher->breakLine();
		watcher->msg << L"��ǰʣ�����ƣ�";
		watcher->breakLine();
		for (unsigned j = 0; j < this->players.size(); j++) {
			watcher->msg << j + 1 << L"��";
			watcher->msg << L"[" << (j == this->bossIndex ? L"����" : L"ũ��") << L"]";
			watcher->at(this->players[j]->number);
			watcher->breakLine();

			for (unsigned m = 0; m < players[j]->card.size(); m++) {
				watcher->msg << L"[" << players[j]->card.at(m) << L"]";
			}

			watcher->breakLine();
		}
		watcher->breakLine();
		watcher->msg << L"�����ֵ�";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"���ơ�";
		watcher->breakLine();
	}
}

void Desk::sendWatchingMsg_Pass(int64_t playNum) {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(playNum);
		watcher->msg << L"���ƣ�";

		watcher->msg << L"�����ֵ�";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"���ơ�";
		watcher->breakLine();
	}
}

void Desk::sendWatchingMsg_Surrender(int64_t playNum) {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->at(playNum);
		watcher->msg << L"���ƣ�";

		watcher->msg << L"�����ֵ�";
		watcher->at(this->players[this->currentPlayIndex]->number);
		watcher->msg << L"���ơ�";
		watcher->breakLine();
	}
}

void Desk::sendWatchingMsg_Over() {
	Watcher *watcher = watchers[0];
	for (unsigned i = 0; i < this->watchers.size(); i++) {
		watcher = watchers[i];

		watcher->msg << L"��������Ϸ������";
		watcher->msg << (this->whoIsWinner == 1 ? L"����Ӯ��" : L"ũ��Ӯ��");
		watcher->breakLine();
		watcher->msg << L"�˳���սģʽ��";
		watcher->breakLine();
	}
}

Desk* Desks::getOrCreatDesk(int64_t deskNum) {

	Desk *desk = NULL;
	int deskIndex = getDesk(deskNum);
	if (deskIndex == -1) {//û������
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

		//�����һ�������
		this->counter = new thread(&Desk::checkAFK, this);

		int64_t maxScore = -500000001;
		int64_t minScore = 500000001;
		int64_t tempScore = 0;

		//�ݶ��������ͷ�ÿ��50�ֱ�׼�ּ�3��
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

		this->basic += (int) CONFIG_BOTTOM_SCORE * (maxScore-minScore) / 50;
		if (this->basic > CONFIG_TOP_SCORE) {
			this->basic = CONFIG_TOP_SCORE; //��󵥾ֻ�����1000��
		}

		this->msg << L"��Ϸ��ʼ�����ţ�" << this->number << L"��";
		this->breakLine(); 
		this->msg << L"��Ϸ�һ����ʱ�䣺������" << CONFIG_TIME_BOSS << L"�룬�ӱ�" 
			<< CONFIG_TIME_MULTIPLE << L"�룬����" << CONFIG_TIME_GAME << L"�룬";
		this->breakLine();

		//this->msg << L"׼�����ڿ��Խ���[����]���������ƻ�ʹ���ֱ��� + 2�������������";
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
			//������Ϸ�������η��ͳ�����ʾ���˴���������Ч��ʾ
			return;
		}
		else {
			this->msg << L"û���㹻����ҡ�";
			this->breakLine();
			for (unsigned i = 0; i < this->players.size(); i++) {
				this->msg << i + 1 << L"��";
				this->at(this->players[i]->number);
				this->msg << L"��" << Admin::readWin(this->players[i]->number) << L"ʤ"
					<< Admin::readLose(this->players[i]->number) << L"��";
				this->msg << L"������";
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

	if (this->state < STATE_BOSSING) {
		return;
	}

	if (timeNow - this->lastTime > 50) {
		this->at(playNum);
		this->breakLine();
		this->msg << L"�ٱ��ɹ���";
		this->breakLine();

		Admin::addScore(this->players[this->currentPlayIndex]->number, -CONFIG_SURRENDER_PENALTY);
		Admin::addScore(playNum, CONFIG_SURRENDER_PENALTY);

		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"[" << L"�һ�-" << CONFIG_SURRENDER_PENALTY << L"��]";
		this->breakLine();
		this->at(playNum);
		this->msg << L"[" << L"�ٱ�+" << CONFIG_SURRENDER_PENALTY << L"��]";
		this->breakLine();


		this->setNextPlayerIndex();
		this->lastTime = timeNow;

		this->msg << L"�����ֵ�";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"���ơ�";
		this->breakLine();
	}
	else {
		this->at(playNum);
		this->breakLine();
		this->msg << L"�ٱ�ʧ�ܣ�";
		this->at(this->players[this->currentPlayIndex]->number);
		this->msg << L"��ʣ�����ʱ��Ϊ" << this->lastTime + CONFIG_TIME_GAME - timeNow << L"�롣";
	}
}

void Desk::checkAFK() {
	time_t rawtime;
	int64_t timeNow = time(&rawtime);

	while (this->state < STATE_BOSSING) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		;
	}

	this->warningSent = false;

	while (this->state == STATE_BOSSING) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		timeNow = time(&rawtime);

		//this->msg << L"������Ϣ��";
		//this->at(this->players[this->currentPlayIndex]->number);
		//this->msg << L"��ʣ�����ʱ��Ϊ" << this->lastTime + CONFIG_TIME_GAME - timeNow << L"�롣";

		if (timeNow - this->lastTime > CONFIG_TIME_BOSS) {
			this->warningSent = false;
			Admin::addScore(this->players[this->currentPlayIndex]->number, -CONFIG_SURRENDER_PENALTY);

			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"[" << L"�һ�-" << CONFIG_SURRENDER_PENALTY << L"��]";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			//this->setNextPlayerIndex();
			//this->lastTime = timeNow;

			this->dontBoss(this->players[this->currentPlayIndex]->number);
		}
		else if (!this->warningSent && timeNow - this->lastTime > CONFIG_TIME_BOSS - CONFIG_TIME_WARNING) {
			this->warningSent = true;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"������ʱ��ʣ��" << CONFIG_TIME_WARNING << L"�롣";
			this->breakLine();
		}

		this->sendMsg(this->subType);
	}

	//��ֹ��ʾ������������bug
	this->warningSent = false;

	while (this->state == STATE_MULTIPLING) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		timeNow = time(&rawtime);

		//this->msg << L"������Ϣ��";
		//this->at(this->players[this->currentPlayIndex]->number);
		//this->msg << L"��ʣ�����ʱ��Ϊ" << this->lastTime + CONFIG_TIME_GAME - timeNow << L"�롣";

		if (timeNow - this->lastTime > CONFIG_TIME_MULTIPLE) {
			this->warningSent = false;
			Admin::addScore(this->players[this->currentPlayIndex]->number, -CONFIG_SURRENDER_PENALTY);

			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"[" << L"�һ�-" << CONFIG_SURRENDER_PENALTY << L"��]";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			//this->setNextPlayerIndex();
			//this->lastTime = timeNow;

			this->dontMultiple(this->players[this->currentPlayIndex]->number);
		}
		else if (!this->warningSent && timeNow - this->lastTime > CONFIG_TIME_MULTIPLE - CONFIG_TIME_WARNING) {
			this->warningSent = true;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"�ӱ�ѡ��ʱ��ʣ��" << CONFIG_TIME_WARNING << L"�롣";
			this->breakLine();
		}

		this->sendMsg(this->subType);
	}

	//��ֹ��ʾ��ӱ�����bug
	this->warningSent = false;

	while (this->state == STATE_READYTOGO || this->state == STATE_GAMING) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		timeNow = time(&rawtime);

		//this->msg << L"������Ϣ��";
		//this->at(this->players[this->currentPlayIndex]->number);
		//this->msg << L"��ʣ�����ʱ��Ϊ" << this->lastTime + CONFIG_TIME_GAME - timeNow << L"�롣";

		if (timeNow - this->lastTime > CONFIG_TIME_GAME) {
			this->warningSent = false;
			Admin::addScore(this->players[this->currentPlayIndex]->number, -CONFIG_SURRENDER_PENALTY);

			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"[" << L"�һ�-" << CONFIG_SURRENDER_PENALTY << L"��]";
			this->breakLine();
			this->msg << L"---------------";
			this->breakLine();
			this->setNextPlayerIndex();
			this->lastTime = timeNow;

			this->msg << L"�����ֵ�";
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"���ơ�";
			this->breakLine();
		}
		else if (!this->warningSent && timeNow - this->lastTime > CONFIG_TIME_GAME - CONFIG_TIME_WARNING) {
			this->warningSent = true;
			this->at(this->players[this->currentPlayIndex]->number);
			this->msg << L"����ʱ��ʣ��" << CONFIG_TIME_WARNING << L"�롣";
			this->breakLine();
		}

		this->sendMsg(this->subType);
	}
}