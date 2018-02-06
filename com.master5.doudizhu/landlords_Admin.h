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

int64_t Admin::readAdmin()
{
	wstring model = L"admin";
	wstring key = L"admin";
	return GetPrivateProfileInt(model.c_str(), key.c_str(), 0, CONFIG_PATH.c_str());
}

bool Admin::isAdmin(int64_t playNum)
{
	return playNum == Admin::readAdmin();
}

wstring Admin::readString() {
	WCHAR tmp[15];
	GetPrivateProfileString(L"admin", L"admin", L"", tmp, 15, CONFIG_PATH.c_str());
	return wstring(tmp);
}

bool Admin::allotScoreTo(wstring msg, int64_t playNum)
{


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
	src_it = mr[0].second;
	regex_search(src_it, src_end, mr, numberReg);
	wstringstream scoress;
	scoress << mr[0].str();
	scoress >> score;
	scoress.str(L"");

	return Admin::isAdmin(playNum) && Admin::writeScore(playerNum, score < 0 ? 0 : score);
}

bool Admin::gameOver(wstring msg, int64_t playNum)
{
	if (Admin::isAdmin(playNum)) {
		wstringstream ss;
		ss << msg.substr(4, msg.size());
		int64_t destNum;
		ss >> destNum;
		ss.str(L"");
		casino.gameOver(destNum);
		return true;
	}
	return false;
}

bool Admin::writeAdmin(int64_t playerNum)
{
	wstring model = L"admin";
	wstring key = L"admin";
	wstringstream ss;
	ss << playerNum;
	wstring value = ss.str();
	ss.str(L"");
	return WritePrivateProfileString(model.c_str(), key.c_str(), value.c_str(), CONFIG_PATH.c_str());

}

int64_t Admin::readScore(int64_t playerNum)
{
	wstring model = L"score";
	wstringstream ss;
	ss << playerNum;
	wstring key = ss.str();
	ss.str(L"");

	//增加负分功能，最低值负5亿分
	return GetPrivateProfileInt(model.c_str(), key.c_str(), 0, CONFIG_PATH.c_str())-500000000;
}

bool Admin::getScore(int64_t playerNum)
{
	wstring model = L"time";
	wstringstream ss;
	ss << playerNum;
	wstring key = ss.str();
	ss.str(L"");
	int64_t lastGetScoreTime = GetPrivateProfileInt(model.c_str(), key.c_str(), 0, CONFIG_PATH.c_str());
	time_t rawtime;
	int64_t now = time(&rawtime);

	if (now / (24 * 60 * 60) > lastGetScoreTime / (24 * 60 * 60)) {
		Admin::addScore(playerNum, CONIFG_INIT_SCORE);
		ss << now;
		wstring value = ss.str();
		ss.str(L"");
		return WritePrivateProfileString(model.c_str(), key.c_str(), value.c_str(), CONFIG_PATH.c_str());
	}
	return false;
}

bool Admin::writeScore(int64_t playerNum, int64_t score)
{
	wstring model = L"score";
	wstringstream ss;
	ss << playerNum;
	wstring key = ss.str();
	ss.str(L"");
	ss << score+500000000; //对应read中减少5亿分
	wstring value = ss.str();
	ss.str(L"");
	return WritePrivateProfileString(model.c_str(), key.c_str(), value.c_str(), CONFIG_PATH.c_str());
}

bool Admin::addScore(int64_t playerNum, int score)
{
	int64_t hasScore = Admin::readScore(playerNum) + score;
	if (hasScore < 0) {
		hasScore = 0;
	}
	else if (hasScore > 1000000000) {
		hasScore = 1000000000;
	}
	return  Admin::writeScore(playerNum, hasScore);
}

int64_t Admin::readWin(int64_t playerNum)
{
	wstring model = L"win";
	wstringstream ss;
	ss << playerNum;
	wstring key = ss.str();
	ss.str(L"");
	return GetPrivateProfileInt(model.c_str(), key.c_str(), 0, CONFIG_PATH.c_str());
}

int64_t Admin::readLose(int64_t playerNum)
{
	wstring model = L"lose";
	wstringstream ss;
	ss << playerNum;
	wstring key = ss.str();
	ss.str(L"");
	return GetPrivateProfileInt(model.c_str(), key.c_str(), 0, CONFIG_PATH.c_str());
}

bool Admin::writeWin(int64_t playerNum, int64_t win)
{
	wstring model = L"win";
	wstringstream ss;
	ss << playerNum;
	wstring key = ss.str();
	ss.str(L"");
	ss << win;
	wstring value = ss.str();
	ss.str(L"");
	return WritePrivateProfileString(model.c_str(), key.c_str(), value.c_str(), CONFIG_PATH.c_str());
}

bool Admin::writeLose(int64_t playerNum, int64_t lose)
{
	wstring model = L"lose";
	wstringstream ss;
	ss << playerNum;
	wstring key = ss.str();
	ss.str(L"");
	ss << lose;
	wstring value = ss.str();
	ss.str(L"");
	return WritePrivateProfileString(model.c_str(), key.c_str(), value.c_str(), CONFIG_PATH.c_str());
}

bool Admin::addWin(int64_t playerNum)
{
	int64_t hasWin = Admin::readWin(playerNum) + 1;
	return  Admin::writeWin(playerNum, hasWin);
}

bool Admin::addLose(int64_t playerNum)
{
	int64_t hasLose = Admin::readLose(playerNum) + 1;
	return  Admin::writeLose(playerNum, hasLose);
}

bool Admin::IAmAdmin(int64_t playerNum)
{
	return  Admin::readAdmin() == 0 && Admin::writeAdmin(playerNum);
}

bool Admin::resetGame(int64_t playNum)
{
	return playNum == Admin::readAdmin() && DeleteFile(CONFIG_PATH.c_str());
}