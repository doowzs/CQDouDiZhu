#pragma once

#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tchar.h>  
#include <regex> 
using namespace std;

static const wstring  cardDest[54] = {
	L"鬼",L"王",
	L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"10",L"J",L"Q",L"K",L"A",
	L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"10",L"J",L"Q",L"K",L"A",
	L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"10",L"J",L"Q",L"K",L"A",
	L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"10",L"J",L"Q",L"K",L"A"
};

static const wstring flag[15] = { L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"10",L"J",L"Q",L"K",L"A",L"2",L"鬼",L"王" };

static const int STATE_WAIT = 0;
static const int STATE_START = 1;
static const int STATE_BOSSING = 2;
static const int STATE_MULTIPLING = 4;
static const int STATE_READYTOGO = 4;
static const int STATE_GAMEING = 5;

static const wstring CONFIG_PATH = L".\\app\\com.auntspecial.doudizhu\\config.ini";
static const wstring CONFIG_DIR = L".\\app\\com.auntspecial.doudizhu\\";

static const int CONIFG_INIT_SCORE = 200;
static const int CONFIG_BOTTOM_SCORE = 5;

static const wregex allotReg(L"分配积分(\\d+)=(\\d+)");
static const wregex numberReg(L"\\d+");

class Util {
public:
	static int AC;
	static void testMsg(bool subType, int64_t playNum, int64_t desknum, const char * str);
	static void sendGroupMsg(int64_t groupid, const char *msg);
	static void sendDiscussMsg(int64_t groupid, const char *msg);
	static void sendPrivateMsg(int64_t groupid, const char *msg);
	static int  findAndRemove(vector<wstring> &dest, wstring str);
	static int  find(vector<wstring> &dest, wstring str);
	static int  findFlag(wstring str);
	static int  desc(int a, int b);
	static int  asc(int a, int b);
	static bool  compareCard(const wstring &carda, const wstring &cardb);
	static void  trim(wstring &s);
	static void  toUpper(wstring &str);
	static void setAC(int32_t ac);
	static string wstring2string(wstring wstr);
	static wstring string2wstring(string str);
	static void mkdir();
};

class Admin {
public:
	static wstring readString();
	static int64_t readAdmin();
	static int64_t readScore(int64_t playerNum);
	static bool isAdmin(int64_t playNum);
	static bool writeAdmin(int64_t playerNum);
	static bool getScore(int64_t playerNum);
	static bool addScore(int64_t playerNum, int score);
	static bool IAmAdmin(int64_t playerNum);
	static bool resetGame(int64_t playNum);
	static bool allotScoreTo(wstring msg, int64_t playNum);
	static bool gameOver(wstring msg, int64_t playNum);
private:
	static bool writeScore(int64_t playerNum, int64_t score);
};

//部分关于player的函数定义在Desks中
class Player
{
public:
	Player();
	wstringstream msg;
	int64_t number;
	vector<wstring> card;
	bool isReady;
	bool isOpenCard;//明牌状态
	bool isSurrender;//投降状态
	void sendMsg();
	void listCards();
	void breakLine();
};

//部分关于watcher的函数定义在Desks中
class Watcher
{
public:
	Watcher();
	wstringstream msg;
	int64_t number;
	void sendMsg();
	void breakLine();
	void at(int64_t playNum);
};

class Desk {
public:

	Desk();
	int multiple;
	int basic;
	int turn;
	wstring cards[54];
	int64_t number;
	vector<Player*> players;
	vector<Watcher*> watchers;//观察者队列

	int whoIsWinner;
	int state;
	int lastPlayIndex;//当前谁出得牌
	int currentPlayIndex;//该谁出牌
	int bossIndex;//谁是地主

	int bossCount;//记录出牌次数，检测春天
	int farmCount;

	vector<wstring> lastCard;//上位玩家的牌
	wstring lastCardType;//上位玩家得牌类
	vector<int> *lastWeights;//上位玩家的牌的权重

	wstringstream msg;

	void join(int64_t playNum);
	void startGame();
	void exit(int64_t playNum);
	void commandList();

	void shuffle();//洗牌
	void deal();//发牌
				//抢地主
	void creataBoss();
	void getBoss(int64_t playerNum);
	void dontBoss(int64_t playerNum);
	//抢加倍
	void multipleChoice();
	void getMultiple(int64_t playerNum);
	void dontMultiple(int64_t playerNum);
	bool bossHasMultipled = false;
	void sendBossCard();
	void play(int64_t playNum, wstring msg);
	void play(vector<wstring> list, int playIndex);//出牌
	void discard(int64_t playNum);
	void surrender(int64_t playNum);//投降
	void openCard(int64_t playNum);//明牌

	void getPlayerInfo(int64_t playNum);
	void getScore(int64_t playNum);

	void at(int64_t playNum);
	void breakLine();
	int getPlayer(int64_t number);//按qq号获得玩家得索引
	void setNextPlayerIndex();//设置下个出牌得玩家索引
	void listPlayers(int type);
	bool isCanWin(int cardCount, vector<int> *Weights, wstring type);
	wstring getMycardType(vector<wstring> list, vector<int> *Weights);
	void sendMsg(bool subType);
	void sendPlayerMsg();
	void sendWatcherMsg();
	void listCardsOnDesk(Player* player);

	void joinWatching(int64_t playNum);
	void exitWatching(int64_t playNum);
	void sendWatchingMsg_Join(int64_t joinNum);
	void sendWatchingMsg_Start();
	void sendWatchingMsg_Play();
	void sendWatchingMsg_Surrender(int64_t playNum);
	void sendWatchingMsg_Over();
	void sendWatchingMsg_Pass(int64_t playNum);
	int getWatcher(int64_t number);//按qq号获得玩家得索引
};

class Desks {
public:
	vector<Desk*> desks;
	Desk* getOrCreatDesk(int64_t deskNum);
	static bool game(bool subType, int64_t deskNum, int64_t playNum, const char *msg);
	static bool game(int64_t playNum, const char *msg);
	int getDesk(int64_t deskNum);
	void gameOver(int64_t deskNum);
	void listDesks();
};

static Desks casino;