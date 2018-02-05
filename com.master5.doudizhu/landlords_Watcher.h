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

Watcher::Watcher() {
}

void Watcher::sendMsg()
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

void Watcher::breakLine()
{
	this->msg << L"\r\n";
}

void Watcher::at(int64_t playNum)
{
	this->msg << L"[CQ:at,qq=" << playNum << L"]";
}