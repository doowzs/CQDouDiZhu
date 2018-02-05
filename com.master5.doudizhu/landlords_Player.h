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

void Player::listCards()
{
	for (unsigned m = 0; m < this->card.size(); m++) {
		this->msg << L"[" << this->card.at(m) << L"]";
	}

}

Player::Player() {
	this->isReady = false;
	this->isOpenCard = false;
	this->isSurrender = false;
}

void Player::sendMsg()
{
	wstring tmp = this->msg.str();
	if (tmp.empty()) {
		this->msg.str(L"");
		return;
	}
	int length = tmp.length();
	if (tmp[length - 2] == '\r' && tmp[length - 1] == '\n') {
		tmp = tmp.substr(0, length - 2);
	}

	Util::sendPrivateMsg(this->number, Util::wstring2string(tmp).data());
	this->msg.str(L"");
}

void Player::breakLine()
{
	this->msg << L"\r\n";
}