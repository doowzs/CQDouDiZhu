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
		<< L"------ ������ ------" << "\r\n" 
		<< L"------ �����б� ------" << "\r\n"
		<< L"------ *�ű�ʾ֧�ֺ������ ------" << "\r\n"
		<< L"0*. �������汾���鿴��Ϸ�汾�š�GitHub������ԭ������Ϣ\r\n"
		<< L"1*. ����|���ƣ�������Ϸ��ÿ�δ���ϵͳ����5���֣�\r\n"
		<< L"2*. ��|�򣺳��� ���� ��23456��\r\n"
		<< L"3*. ��(��)|��Ҫ|pass������\r\n"
		<< L"4*. ��(����)|�������Ƿ�������\r\n"
		<< L"5*. ��(��)|����(��)���Ƿ�ӱ�\r\n"
		<< L"6*. ��ʼ|����|GO���Ƿ�ʼ��Ϸ\r\n"
		<< L"7*. ����|�����ˣ��˳���Ϸ��ֻ����׼������ʹ��\r\n"
		<< L"8. ����б���ǰ����Ϸ�е������Ϣ\r\n"
		<< L"9*. ���ƣ���ʾ�Լ����Ƹ�������ң����ƻᵼ�»��ַ�����ֻ���ڷ����ƺ��Լ�����֮ǰʹ�á�\r\n"
		<< L"10*. ���ƣ�����������Ϸ����������������ũ��������Ϸ����������ũ�����Ӯ�˲��÷֣�����˫���۷�" << "\r\n"
		//<< L"11. ��ȡ���֣��ѷ���������ȡ���֣�ÿ��ɻ�ȡ200���֡�" << "\r\n"
		<< L"11. �ҵ���Ϣ���鿴�ҵ�ս���������Ϣ��Ⱥ��˽�ĽԿɣ�" << "\r\n"
		<< L"12. �����ս|��ս�����й۲�" << "\r\n"
		<< L"13. �˳���ս����������Ŀ�����" << "\r\n"
		<< L"14*. �ٱ�|�һ�|AFK������60�벻���ƣ����Ծٱ����ٱ��ɹ��Ľ���" << CONFIG_SURRENDER_PENALTY
		 << L"�֣����ٱ��Ŀ۳�" << CONFIG_SURRENDER_PENALTY << L"�֡�" << "\r\n"
		<< L"A1. " << L"���ǹ�������Ϸ����ԱΪ��ǰ������Ϣ��qq������Ա��ʹ�ù�������������ú��ܸ���" << "\r\n"
		<< L"A2. " << L"���ö�������ɾ���������á����ú�������趨����Ա" << "\r\n"
		<< L"A3. " << L"������Ϸ[Ⱥ��]������ָ��Ⱥ�ŵ���Ϸ�����磺������Ϸ123456" << "\r\n"
		<< L"A4. " << L"���û���[qq��]=[����]����ָ��qq������֣��磺���û���123456=-998" << "\r\n"
		<< L"A5. " << L"�ı��������ͣ��л�����ʽ���ݡ��롰�������ݡ�����" << "\r\n"
		<< L"A6. " << L"�������ݣ��㶮�ģ���ֹ��������ը"
		;
	this->breakLine();
}

bool Desks::game(bool subType, int64_t deskNum, int64_t playNum, const char* msgArray) {

	string tmp = msgArray;

	wstring msg = Util::string2wstring(tmp);
	Util::trim(msg);
	Util::toUpper(msg);

	Desk *desk = casino.getOrCreatDesk(deskNum);

	if (playNum == 80000000) {
		//desk->msg << L"�����û����ܲμӶ�������";
		return false;
	}
	else if (msg.find(L"����������") == 0 || msg.find(L"������ָ��") == 0 || msg.find(L"����������") == 0) {
		desk->commandList();
	}
	else if (msg.find(L"������") == 0) {
		desk->msg << L"������ " << CONFIG_VERSION;
		desk->breakLine();
		desk->msg << Admin::readDataType() << L" " << Admin::readVersion(); // << L" CST";
		desk->breakLine();
		desk->msg << L"Դ���������������https://github.com/doowzs/CQDouDiZhu";
		desk->breakLine();
		desk->msg << L"ԭ������2.0.1Դ���룺https://github.com/lsjspl/CQDouDiZhu";
	}
	else if (msg.find(L"����") == 0 || msg.find(L"����") == 0
		|| msg.find(L"����") == 0) {
		desk->join(playNum);
	}
	else if ((desk->state >= STATE_READYTOGO) &&
		(msg.find(L"��") == 0 || msg.find(L"��") == 0)) {//���ƽ׶�
		desk->play(playNum, msg);
	}
	else if ((desk->state >= STATE_READYTOGO) &&
		(msg.find(L"��") == 0 || msg.find(L"����") == 0 || msg.find(L"����") == 0
			|| msg.find(L"û��") == 0 || msg.find(L"�򲻳�") == 0 || msg.find(L"Ҫ����") == 0
			|| msg.find(L"��Ҫ") == 0 || msg.find(L"PASS") == 0)) {//�������ƽ׶�
		desk->discard(playNum);
	}
	else if (msg.find(L"����") == 0 || msg.find(L"����") == 0
		|| msg.find(L"������") == 0) {//������Ϸ
		desk->exit(playNum);
	}
	else if (msg == L"����б�") {
		desk->listPlayers(1);
	}
	else if (msg.find(L"GO") == 0 || msg.find(L"����") == 0) {
		desk->startGame();
	}
	else if ((msg.find(L"��") == 0 || msg.find(L"Ҫ") == 0) && desk->state == STATE_BOSSING) {
		desk->getBoss(playNum);
	}
	else if (msg.find(L"��") == 0 && desk->state == STATE_BOSSING) {
		desk->dontBoss(playNum);
	}
	else if (msg.find(L"��") == 0 && desk->state == STATE_MULTIPLING) {
		desk->getMultiple(playNum);
	}
	else if (msg.find(L"��") == 0 && desk->state == STATE_MULTIPLING) {
		desk->dontMultiple(playNum);
	}
	else if (msg.find(L"����") == 0) {
		desk->openCard(playNum);
	}
	else if ((msg.find(L"����") == 0)
		&& desk->state >= STATE_BOSSING) {
		desk->surrender(playNum);
	}
	else if (msg == L"������") {
		desk->msg << L"������û��(��)�أ���������֮��������ã�";
	}
	else if (msg == L"�ҵ���Ϣ") {
		desk->getPlayerInfo(playNum);
	}
	else if (msg == L"��ȡ����" || msg == L"�������" || msg == L"���ֹ���") {
		desk->msg << L"Ϊ�����ƻ���ͨ�����ͣ�3.2.0�汾��ȡ����ÿ�ջ��ֹ��ܣ���Ϊ�������ͻ��֣�";
		desk->breakLine();
		desk->msg << L"����һ�λ��" << CONFIG_PLAY_BONUS
			<< L"�֣���;�˳������ƣ�����" << CONFIG_SURRENDER_PENALTY << L"�֡�";
		desk->breakLine();
		desk->msg << L"ÿ����Ϸ�ı�׼�ּ��㷽��Ϊ����ʼֵ" << CONFIG_INIT_SCORE
			<< L"�֣������ͷ���ҵĻ��ֲ��ÿ��" << 50
			<< L"�֣���׼�ּ�" << CONFIG_BOTTOM_SCORE << L"�֣�����׼�ֲ��ᳬ��"
			<< CONFIG_TOP_SCORE << L"�֡�";
		desk->breakLine();
		desk->msg << L"��������Ϊ��5�ڣ�����Ϊ��5�ڡ�";
		desk->breakLine();
	}
	else if (msg.find(L"�����ս") == 0 || msg.find(L"��ս") == 0) {
		desk->joinWatching(playNum);
	}
	else if (msg.find(L"�˳���ս") == 0) {
		desk->exitWatching(playNum);
	}
	else if (msg.find(L"�ٱ�") == 0 || msg.find(L"�һ�") == 0 || msg.find(L"AFK") == 0) {
		desk->AFKHandle(playNum);
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
	if (msg == L"���ǹ���") {
		result = Admin::IAmAdmin(playNum);
	}
	else if (msg == L"���ö�����" || msg == L"��ʼ��������") {
		result = Admin::resetGame(playNum);
	}
	else if (msg.find(L"�ı�����") == 0) {
		result = Admin::writeDataType();
		Admin::getPlayerInfo(Admin::readAdmin());
	}
	else if (regex_match(msg, allotReg)) {
		result = Admin::allotScoreTo(msg, playNum);
	}
	else if (regex_match(msg, allotReg2)) {
		result = Admin::allotScoreTo2(msg, playNum);
	}
	else if (msg.find(L"������Ϸ") == 0) {//������Ϸ
		result = Admin::gameOver(msg, playNum);
	}
	else if (msg == L"�ҵ���Ϣ") {
		Admin::getPlayerInfo(playNum);
		return false;
	}
	else if (msg == L"��������") {
		result = Admin::backupData(playNum);
	}
	else {
		return false;
	}

	msg = result ? L"�����ɹ������Ĺ���Ա" : L"�ǳ���Ǹ������ʧ��";
	Util::sendPrivateMsg(playNum, Util::wstring2string(msg).data());
	return true;
}