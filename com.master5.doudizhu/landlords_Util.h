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

void Util::testMsg(bool subType, int64_t desknum, int64_t playNum, const char * str) {
	int index = casino.desks[0]->currentPlayIndex;
	casino.game(subType, desknum, playNum + index, str);
}


void Util::sendGroupMsg(int64_t groupid, const char *msg) {
#ifdef _DEBUG  
	string aa = msg;
	cout << "群发：" << aa << endl;
#else
	CQ_sendGroupMsg(Util::AC, groupid, msg);
#endif
}


void Util::sendDiscussMsg(int64_t groupid, const char *msg) {
#ifdef _DEBUG  
	string aa = msg;
	cout << "群发：" << aa << endl;
#else
	CQ_sendDiscussMsg(Util::AC, groupid, msg);
#endif
}

void Util::sendPrivateMsg(int64_t number, const char* msg) {
#ifdef _DEBUG  
	string aa = msg;
	cout << "私聊" << number << "：" << aa << endl;
#else
	CQ_sendPrivateMsg(Util::AC, number, msg);
#endif
}


//将string转换成wstring  
wstring Util::string2wstring(string str)
{
	wstring result;
	//获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
	TCHAR* buffer = new TCHAR[len + 1];
	//多字节编码转换成宽字节编码  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
	buffer[len] = '\0';             //添加字符串结尾  
									//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

void Util::mkdir()
{
	CreateDirectory(CONFIG_DIR.c_str(), NULL);
}

//将wstring转换成string  
string Util::wstring2string(wstring wstr)
{
	string result;
	//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	//宽字节编码转换成多字节编码  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

int Util::findAndRemove(vector<wstring> &dest, wstring str) {

	for (unsigned i = 0; i < dest.size(); i++) {
		if (dest.at(i) == str) {
			vector<wstring>::iterator it = dest.begin() + i;
			dest.erase(it);
			return i;
		}
	}
	return -1;
}

int Util::find(vector<wstring> &dest, wstring str) {

	for (unsigned i = 0; i < dest.size(); i++) {
		if (dest.at(i) == str) {
			return i;
		}
	}
	return -1;
}

int Util::findFlag(wstring str)
{
	for (int i = 0; i < 15; i++) {
		if (flag[i] == str) {
			return i;
		}
	}
	return -1;

}

int Util::desc(int a, int b)
{
	return a > b;
}

int Util::asc(int a, int b)
{
	return a < b;
}

bool Util::compareCard(const wstring &carda, const wstring &cardb)
{
	return findFlag(carda) < findFlag(cardb);
}

void Util::trim(wstring &s)
{

	int index = 0;
	if (!s.empty())
	{
		while ((index = s.find(' ', index)) != wstring::npos)
		{
			s.erase(index, 1);
		}
	}

}

void Util::toUpper(wstring &str) {
	transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void Util::setAC(int32_t ac)
{
	Util::AC = ac;
}

