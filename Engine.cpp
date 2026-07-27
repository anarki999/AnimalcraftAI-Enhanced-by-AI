#include<bits/stdc++.h>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <windows.h>
#include"ucci.h"
using namespace std;
//常量 
const char name[24][5]={"　","穴","阱","■","阱","穴","　","　","象","狮","虎","豹","狼","狗","猫","鼠","象","狮","虎","豹","狼","狗","猫","鼠"};//棋子名称
const int RANK_TOP = 3;//起始行
const int RANK_BOTTOM = 9;//终点行
const int FILE_LEFT = 3;//起始列
const int FILE_RIGHT = 11;//终点列 
//以下为棋子编号，0-6对应7种棋子
const int PIECE_ELEPHANT = 0;
const int PIECE_LION = 1;
const int PIECE_TIGER = 2;
const int PIECE_LEOPARD = 3;
const int PIECE_WOLF = 4;
const int PIECE_DOG = 5;
const int PIECE_CAT = 6;
const int PIECE_MOUSE = 7;
//其它 
const int MAX_GEN_MOVES = 128; // 最大的生成走法数
const int MAX_MOVES = 1000;     // 最大的历史走法数
const int MATE_VALUE = 10000;  // 最高分值，即将死的分值
const int WIN_VALUE = MATE_VALUE - 100; // 搜索出胜负的分值界限，超出此值就说明已经搜索出杀棋了
// v16a优化：HASH_SIZE 由固定 1024（仅1024个置换表项，深搜几层就严重碰撞，
// 令置换表几乎失效）改为可配置、动态分配、并保持2的幂以便用mask取余。
// 默认 128MB，可在 engine.ini 用 HashMB=<N> 覆盖（最小1MB，最大4096MB）。
const int DEFAULT_HASH_MB = 128;
const int MIN_HASH_MB = 1;
const int MAX_HASH_MB = 4096;
int nHashMB = DEFAULT_HASH_MB; // 置换表大小(MB)，从engine.ini读取
const int HASH_ALPHA = 1;      // ALPHA节点的置换表项
const int HASH_BETA = 2;       // BETA节点的置换表项
const int HASH_PV = 3;         // PV节点的置换表项
const int NULL_MARGIN = 600;   // 空步裁剪的子力边界
const int NULL_DEPTH = 2;      // 空步裁剪的裁剪深度
// v15新增：搜索迭代深度上限。原本硬编码100，会导致纯ponder/infinite模式下
// 一旦迭代深度自然跑到100层就会跳出循环并送出bestmove，
// 这在没收到ponderhit/stop的情况下是违反UCI/UCCI协议的(等于泄漏了bestmove)。
// 提高上限，使其在正常时间控制下几乎不可能触及，从根本上堵住这个漏洞。
const int MAX_ITER_DEPTH = 200;

int t2=1000,setdepth=99999999,t,t3;//时间控制和深度控制
// v15a修復：bInfinite原本是普通bool，但它是跨執行緒共享的旗標——
// 主執行緒在收到"go infinite"時寫入，搜索執行緒/看門狗執行緒在
// CheckTimeUp()等函式裡讀取它來判斷能不能因超時自動停止。
// 同一份程式碼裡功能相同的g_stopSearch、g_bPondering都已經是
// std::atomic<bool>，唯獨這裡被漏掉，屬於未定義行為的資料競爭：
// 編譯器優化時可能把讀取結果快取住看不到另一執行緒寫入的最新值，
// 實務後果是go infinite模式下可能誤判超時、提前送出bestmove，
// 違反UCI/UCCI協議"infinite模式必須等到收到stop才能吐出bestmove"的規定。
// 修復：改為std::atomic<bool>，並統一用.load()/.store()存取。
std::atomic<bool> bInfinite(false); // 是否为"go infinite"无限思考模式

int nThreads = 1; // 线程数，从engine.ini读取

void LoadConfig(const char *szIniFile) {
  FILE *fp = fopen(szIniFile, "r");
  if (fp == NULL) return; 
  char line[256];
  while (fgets(line, 256, fp)) {
    int val;
    if (sscanf(line, "Threads=%d", &val) == 1) {
      nThreads = val;
      if (nThreads < 1) nThreads = 1;
      if (nThreads > 64) nThreads = 64; 
    }
    // v16a新增：HashMB=<N> 设定置换表大小(单位MB)，未设定时维持DEFAULT_HASH_MB。
    if (sscanf(line, "HashMB=%d", &val) == 1) {
      nHashMB = val;
      if (nHashMB < MIN_HASH_MB) nHashMB = MIN_HASH_MB;
      if (nHashMB > MAX_HASH_MB) nHashMB = MAX_HASH_MB;
    }
  }
  fclose(fp);
}

// Lazy SMP 多线程支持：
std::atomic<bool> g_stopSearch(false);
std::atomic<long long> g_totalNodes(0);
thread_local bool g_isMainThread = true; 

// 正规Pondering支持
// g_bPondering：true表示当前搜索处于"后台思考"状态，时钟不计时，直到收到ponderhit才启用时钟
// g_searchThread：搜索现在跑在独立线程上，让main()主循环能持续读取stdin指令(stop/ponderhit)
std::atomic<bool> g_bPondering(false);
std::thread g_searchThread;


// 判断棋子是否在棋盘中的数组
static const char ccInBoard[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// 判断棋子是否在九宫的数组
static const char ccInFort[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 4, 0, 0, 3, 3, 3, 0, 0, 2, 0, 0, 0, 0,
  0, 0, 0, 1, 4, 0, 0, 0, 0, 0, 2, 5, 0, 0, 0, 0,
  0, 0, 0, 4, 0, 0, 3, 3, 3, 0, 0, 2, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const bool ccCanJump[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// 步长
static const char ccDelta[4] = {-16, -1, 1, 16};
// 跳河步长
static const char ccJumpDelta[4] = {-48,-4,4,48};

// 棋盘初始设置
static int cucpcStartup[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0,10, 0, 8, 0, 0, 0,23, 0,17, 0, 0, 0, 0,
  0, 0, 0, 0,14, 0, 0, 0, 0, 0,21, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,12, 0, 0, 0,19, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,11, 0, 0, 0,20, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0,13, 0, 0, 0, 0, 0,22, 0, 0, 0, 0, 0,
  0, 0, 0, 9, 0,15, 0, 0, 0,16, 0,18, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
/// 子力位置价值表
static int cucvlPiecePos[8][256] = {
  { // Elephant
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0,275,277,280,282,283,285, 291, 299, 281, 0, 0, 0, 0,
  0, 0, 0,280,282,283,  0,  0,  0, 301, 301, 304, 0, 0, 0, 0,
  0, 0, 0,281,284,285,  0,  0,  0, 304, 318, 320, 0, 0, 0, 0,
  0, 0, 0,  0,284,287,292,296,299, 315, 321,   0, 0, 0, 0, 0,
  0, 0, 0,281,284,285,  0,  0,  0, 304, 318, 320, 0, 0, 0, 0,
  0, 0, 0,280,282,283,  0,  0,  0, 301, 301, 304, 0, 0, 0, 0,
  0, 0, 0,275,277,280,282,283,285, 313, 299, 281, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
   { // Lion
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0,304,309,310, 312, 313, 315, 312, 301, 299, 0, 0, 0, 0,
  0, 0, 0,305,309,312,   0,   0,   0, 316, 309, 301, 0, 0, 0, 0,
  0, 0, 0,304,308,312,   0,   0,   0, 318, 322, 333, 0, 0, 0, 0,
  0, 0, 0,  0,307,312, 313, 314, 315, 321, 331,   0, 0, 0, 0, 0,
  0, 0, 0,304,308,312,   0,   0,   0, 318, 322, 333, 0, 0, 0, 0,
  0, 0, 0,305,309,312,   0,   0,   0, 316, 309, 301, 0, 0, 0, 0,
  0, 0, 0,304,309,310, 312, 313, 315, 312, 301, 299, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
  { // Tiger
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0,242,248,252,253,255,257,259,250,248, 0, 0, 0, 0,
  0, 0, 0,243,250,252,  0,  0,  0,261,265,251, 0, 0, 0, 0,
  0, 0, 0,243,251,252,  0,  0,  0,261,270,273, 0, 0, 0, 0,
  0, 0, 0,  0,244,253,255,256,258,260,273,  0, 0, 0, 0, 0,
  0, 0, 0,243,251,252,  0,  0,  0,261,270,273, 0, 0, 0, 0,
  0, 0, 0,243,250,252,  0,  0,  0,261,265,251, 0, 0, 0, 0,
  0, 0, 0,242,248,252,253,255,257,259,250,248, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
  { //Panther
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 87, 88, 90, 93, 97, 99,102,100,100, 0, 0, 0, 0,
  0, 0, 0, 87, 91, 90, 95, 95, 99,102,102,105, 0, 0, 0, 0,
  0, 0, 0, 87, 91, 90, 95, 95, 99,103,105,108, 0, 0, 0, 0,
  0, 0, 0, 87, 91, 90, 92, 96, 99,104,108, 95, 0, 0, 0, 0,
  0, 0, 0, 87, 91, 90, 95, 95, 99,103,105,108, 0, 0, 0, 0,
  0, 0, 0, 87, 91, 90, 95, 95, 99,102,102,105, 0, 0, 0, 0,
  0, 0, 0, 87, 88, 90, 93, 97, 99,102,100,100, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
  { //Wolf
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 83, 83, 83, 83, 83, 83, 83, 83, 83, 0, 0, 0, 0,
  0, 0, 0, 83, 83, 83, 83, 83, 83, 83, 83, 83, 0, 0, 0, 0,
  0, 0, 0, 83, 85, 83, 83, 83, 83, 83, 83, 83, 0, 0, 0, 0,
  0, 0, 0, 83, 83, 85, 83, 83, 83, 83, 83, 83, 0, 0, 0, 0,
  0, 0, 0, 83, 85, 83, 83, 83, 83, 83, 83, 83, 0, 0, 0, 0,
  0, 0, 0, 83, 83, 83, 83, 83, 83, 83, 83, 83, 0, 0, 0, 0,
  0, 0, 0, 83, 83, 83, 83, 83, 83, 83, 83, 83, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
  { //Dog
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 61, 61, 61, 61, 61, 61, 61, 61, 61, 0, 0, 0, 0,
  0, 0, 0, 61, 61, 61, 61, 61, 61, 61, 61, 61, 0, 0, 0, 0,
  0, 0, 0, 61, 64, 61, 61, 61, 61, 61, 61, 61, 0, 0, 0, 0,
  0, 0, 0, 61, 61, 64, 61, 61, 61, 61, 61, 61, 0, 0, 0, 0,
  0, 0, 0, 61, 64, 61, 61, 61, 61, 61, 61, 61, 0, 0, 0, 0,
  0, 0, 0, 61, 61, 61, 61, 61, 61, 61, 61, 61, 0, 0, 0, 0,
  0, 0, 0, 61, 61, 61, 61, 61, 61, 61, 61, 61, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
  { //Cat
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0,
  0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0,
  0, 0, 0, 50, 53, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0,
  0, 0, 0, 50, 50, 53, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0,
  0, 0, 0, 50, 53, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0,
  0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0,
  0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
  {//Mouse
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 87, 90, 91, 92, 92, 92, 92, 92, 92, 0, 0, 0, 0,
  0, 0, 0, 85, 86, 88, 94, 97, 96, 92, 92, 95, 0, 0, 0, 0,
  0, 0, 0, 82, 84, 89, 94, 95, 97, 93, 95, 99, 0, 0, 0, 0,
  0, 0, 0,  0, 82, 89, 92, 92, 92, 95, 99,  0, 0, 0, 0, 0,
  0, 0, 0, 82, 84, 89, 94, 95, 97, 93, 95, 99, 0, 0, 0, 0,
  0, 0, 0, 85, 86, 88, 94, 97, 96, 92, 92, 95, 0, 0, 0, 0,
  0, 0, 0, 87, 90, 91, 92, 92, 92, 92, 92, 92, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
};
// 判断棋子是否在棋盘中
inline bool IN_BOARD(int sq) {
  return ccInBoard[sq] != 0;
}

// 获得格子的横坐标
inline int RANK_Y(int sq) {
  return sq >> 4;
}

// 获得格子的纵坐标
inline int FILE_X(int sq) {
  return sq & 15;
}

// 根据纵坐标和横坐标获得格子
inline int COORD_XY(int x, int y) {
  return x + (y << 4);
}

//
inline int PIECE_NAME(int pc) {
  return (pc&7);
}


// 翻转棋子
inline int SQUARE_FLIP(int sq) {
  return 254 - sq;
}

// 纵坐标水平镜像
inline int FILE_FLIP(int x) {
  return 14 - x;
}

// 横坐标垂直镜像
inline int RANK_FLIP(int y) {
  return 15 - y;
}

// 是否在河中
inline bool INRIVER(int sq) {
  return ccInFort[sq]==3;
}
// 是否在兽穴中 
inline bool INDEN(int sq,int tag) {
  if(tag==8) return ccInFort[sq]==1;
  return ccInFort[sq]==5;
}
// 是否在陷阱中 
inline bool INTRAP(int sq,int tag) {
  if(tag==8) return ccInFort[sq]==2;
  return ccInFort[sq]==4;
}

// 获得红黑标记(红子是8，黑子是16)
inline int SIDE_TAG(int sd) {
  return 8 + (sd << 3);
}

// 获得对方红黑标记
inline int OPP_SIDE_TAG(int sd) {
  return 16 - (sd << 3);
}

// 获得走法的起点
inline int SRC(int mv) {
  return mv & 255;
}

// 获得走法的终点
inline int DST(int mv) {
  return mv >> 8;
}

// 根据起点和终点获得走法
inline int MOVE(int sqSrc, int sqDst) {
  return sqSrc + sqDst * 256;
}

// 走法水平镜像
inline int BLACK(int sq) {
int x=FILE_X(sq),y=RANK_Y(sq);
return COORD_XY(14-x,y);
  } 

// 历史走法信息(占4字节)
struct MoveStruct {
  int wmv,ucpcCaptured,wpc;
  int dwKey;

  void Set(int mv, int pcCaptured,int pc, int dwKey_) {
    wmv = mv;
    ucpcCaptured = pcCaptured;
    wpc=pc;
    dwKey = dwKey_;
  }
}; // mvs

// RC4密码流生成器
struct RC4Struct {
  int s[256];
  int x, y;

  void InitZero(void);   
  int Nextint(void) {  
    int uc;
    x = (x + 1) & 255;
    y = (y + s[x]) & 255;
    uc = s[x];
    s[x] = s[y];
    s[y] = uc;
    return s[(s[x] + s[y]) & 255];
  }
  int NextLong(void) { 
    int uc0, uc1, uc2, uc3;
    uc0 = Nextint();
    uc1 = Nextint();
    uc2 = Nextint();
    uc3 = Nextint();
    return uc0 + (uc1 << 8) + (uc2 << 16) + (uc3 << 24);
  }
};

void RC4Struct::InitZero(void) {
  int i, j;
  int uc;

  x = y = j = 0;
  for (i = 0; i < 256; i ++) {
    s[i] = i;
  }
  for (i = 0; i < 256; i ++) {
    j = (j + s[i]) & 255;
    uc = s[i];
    s[i] = s[j];
    s[j] = uc;
  }
}

// Zobrist结构
struct ZobristStruct {
  int dwKey, dwLock0, dwLock1;

  void InitZero(void) {                 
    dwKey = dwLock0 = dwLock1 = 0;
  }
  void InitRC4(RC4Struct &rc4) {        
    dwKey = rc4.NextLong();
    dwLock0 = rc4.NextLong();
    dwLock1 = rc4.NextLong();
  }
  void Xor(const ZobristStruct &zobr) { 
    dwKey ^= zobr.dwKey;
    dwLock0 ^= zobr.dwLock0;
    dwLock1 ^= zobr.dwLock1;
  }
  void Xor(const ZobristStruct &zobr1, const ZobristStruct &zobr2) {
    dwKey ^= zobr1.dwKey ^ zobr2.dwKey;
    dwLock0 ^= zobr1.dwLock0 ^ zobr2.dwLock0;
    dwLock1 ^= zobr1.dwLock1 ^ zobr2.dwLock1;
  }
};

// Zobrist表
static struct {
  ZobristStruct Player;
  ZobristStruct Table[16][256];
} Zobrist;

// 初始化Zobrist表
static void InitZobrist(void) {
  int i, j;
  RC4Struct rc4;

  rc4.InitZero();
  Zobrist.Player.InitRC4(rc4);
  for (i = 0; i < 16; i ++) {
    for (j = 0; j < 256; j ++) {
      Zobrist.Table[i][j].InitRC4(rc4);
    }
  }
}

static const int cnThreatPenalty[8] = {
  60,  
  55,  
  45,  
  30,  
  20,  
  15,  
  10,  
  25   
};

static const int ATTACK_PIECE_BONUS_NEAR = 25;
static const int ATTACK_PIECE_BONUS_THREAT = 70;

const int CORR_HIST_LIMIT = 400;   
const int CORR_HIST_SCALE = 256;   
static thread_local int t_nCorrHist[8]; 
#define SearchCorrHist t_nCorrHist

struct PositionStruct {
  bool sdPlayer;                   
  int ucpcSquares[256];            
  int vlWhite, vlBlack;            
  int nDistance, nMoveNum, nMoveNum2;         
  MoveStruct mvsList[MAX_MOVES];   
  ZobristStruct zobr;              
  bool CanJump(int src,int dst) const
  { 
    if(PIECE_NAME(ucpcSquares[src])==PIECE_LION||PIECE_NAME(ucpcSquares[src])==PIECE_TIGER)
    { 
      if(!ccCanJump[src]||!ccCanJump[dst]) return 0; 
      for(int i=0;i<=3;i++)
      { 
        if(dst-src==ccJumpDelta[i])
        { 
          for(int j=src+ccDelta[i];j!=dst&&IN_BOARD(j);j+=ccDelta[i])
          { 
             if(PIECE_NAME(ucpcSquares[j])==PIECE_MOUSE||!INRIVER(j)) return 0;      
          } 
          return 1;  
        }
      }
      return 0; 
    }
    return 0; 
  }
  bool CanMove(int src,int dst) const
  { 
    if(PIECE_NAME(ucpcSquares[src])==PIECE_MOUSE)
    { 
      for(int i=0;i<=3;i++)
      { 
        if(dst-src==ccDelta[i])
        { 
          return 1;  
        }
      } 
      return 0;  
    }
    if(INRIVER(dst)) return 0; 
    for(int i=0;i<=3;i++)
    {
      if(dst-src==ccDelta[i])
      {
        return 1;
      }
    }
    return 0; 
  }
  bool CanEat(int src,int dst) const
  {              
    if(ucpcSquares[dst]==0) return 0;   
    int as=PIECE_NAME(ucpcSquares[src]),bs=PIECE_NAME(ucpcSquares[dst]);  
    if(ucpcSquares[src]-as==ucpcSquares[dst]-bs) return 0;    
    if(INTRAP(dst,ucpcSquares[dst]-bs)) return 1;   
    if(as==PIECE_MOUSE&&bs==PIECE_ELEPHANT)
    {   
      if(INRIVER(src)&&!INRIVER(dst)) return 0;  
      return 1;
    }
    if(as==PIECE_MOUSE&&bs==PIECE_MOUSE)
    { 
        if(INRIVER(src)&&!INRIVER(dst)) return 0;
        if(INRIVER(dst)&&!INRIVER(src)) return 0;
        return 1;
    }
    if(as==PIECE_ELEPHANT&&bs==PIECE_MOUSE) return 0;  
    return as<=bs;    
  }
  void ClearBoard(void) {         
    sdPlayer = false;
    vlWhite = vlBlack = nDistance = 0;
    memset(ucpcSquares, 0, sizeof(ucpcSquares));
    zobr.InitZero();
  }
  void SetIrrev(void) {           
    mvsList[0].Set(0, 0,0, zobr.dwKey);
    nMoveNum = 1;
  }
  void SetIrrev2(void) {           
    nMoveNum2 = 1;
  }
  void Startup(void);             
  void ChangeSide(void) {         
    sdPlayer = 1 - sdPlayer;
    zobr.Xor(Zobrist.Player);
  }
  void AddPiece(int sq, int pc) { 
    ucpcSquares[sq] = pc;
    if (pc < 16) {
      vlWhite += cucvlPiecePos[pc - 8][sq];
      zobr.Xor(Zobrist.Table[pc - 8][sq]);
    } else {
      vlBlack += cucvlPiecePos[pc - 16][BLACK(sq)];
      zobr.Xor(Zobrist.Table[pc - 8][sq]);
    }
  }
  void DelPiece(int sq, int pc) { 
    ucpcSquares[sq] = 0;
    if (pc < 16) {
      vlWhite -= cucvlPiecePos[pc - 8][sq];
      zobr.Xor(Zobrist.Table[pc - 8][sq]);
    } else {
      vlBlack -= cucvlPiecePos[pc - 16][BLACK(sq)];
      zobr.Xor(Zobrist.Table[pc - 8][sq]);
    }
  }
  int EvalThreat(bool sd) const {
    int pcSelfSide = SIDE_TAG(sd);
    int pcOppSide = OPP_SIDE_TAG(sd);
    int vlThreatPenalty = 0;

    for (int sqSrc = 0; sqSrc < 256; sqSrc++) {
      if (!IN_BOARD(sqSrc)) continue;
      int pcSrc = ucpcSquares[sqSrc];
      if ((pcSrc & pcSelfSide) == 0) continue; 
      int pieceName = PIECE_NAME(pcSrc);

      bool bThreatened = false;
      for (int delta = 0; delta <= 3 && !bThreatened; delta++) {
        int sqAtk = sqSrc + ccDelta[delta];
        if (!IN_BOARD(sqAtk)) continue;
        int pcAtk = ucpcSquares[sqAtk];
        if (pcAtk == 0 || (pcAtk & pcOppSide) == 0) continue;
        if (CanMove(sqAtk, sqSrc) && CanEat(sqAtk, sqSrc)) {
          bThreatened = true;
        }
      }
      for (int delta = 0; delta <= 3 && !bThreatened; delta++) {
        int sqAtk = sqSrc + ccJumpDelta[delta];
        if (!IN_BOARD(sqAtk)) continue;
        int pcAtk = ucpcSquares[sqAtk];
        if (pcAtk == 0 || (pcAtk & pcOppSide) == 0) continue;
        if (CanJump(sqAtk, sqSrc) && CanEat(sqAtk, sqSrc)) {
          bThreatened = true;
        }
      }
      if (bThreatened) {
        vlThreatPenalty += cnThreatPenalty[pieceName];
      }
    }
    return vlThreatPenalty;
  }

  int EvalAttackPotential(bool sd) const {
    int pcSelfSide = SIDE_TAG(sd);
    int sqEnemyDen = sd ? 99 : 107; 
    int vlBonus = 0;
    for (int sq = 0; sq < 256; sq++) {
      if (!IN_BOARD(sq)) continue;
      int pc = ucpcSquares[sq];
      if ((pc & pcSelfSide) == 0) continue;
      int pieceName = PIECE_NAME(pc);
      if (pieceName != PIECE_ELEPHANT && pieceName != PIECE_LION && pieceName != PIECE_TIGER) continue;
      int nDist = abs(FILE_X(sq) - FILE_X(sqEnemyDen)) + abs(RANK_Y(sq) - RANK_Y(sqEnemyDen));
      if (nDist <= 1) {
        vlBonus += ATTACK_PIECE_BONUS_THREAT; 
      } else if (nDist <= 3) {
        vlBonus += ATTACK_PIECE_BONUS_NEAR;   
      }
    }
    return vlBonus;
  }

  int Evaluate(void) const {      
    int vlBase = (!sdPlayer ? vlWhite - vlBlack : vlBlack - vlWhite);
    int vlThreatSelf = EvalThreat(sdPlayer);
    int vlThreatOpp  = EvalThreat(!sdPlayer);
    int vlAttackSelf = EvalAttackPotential(sdPlayer);   
    int vlAttackOpp  = EvalAttackPotential(!sdPlayer);  
    int vlEval = vlBase - vlThreatSelf + vlThreatOpp + vlAttackSelf - vlAttackOpp;
    if (nMoveNum > 1) {
      int pieceLastMoved = PIECE_NAME(mvsList[nMoveNum - 1].wpc);
      vlEval -= SearchCorrHist[pieceLastMoved] / CORR_HIST_SCALE;
    }
    return vlEval;
  }
  bool Captured(void) const {     
    return mvsList[nMoveNum - 1].ucpcCaptured != 0;
  }
  int MovePiece(int mv);                      
  void UndoMovePiece(int mv, int pcCaptured); 
  bool MakeMove(int mv);                      
  void UndoMakeMove(void) {                   
    nDistance --;
    nMoveNum --;
    nMoveNum2 --;
    ChangeSide();
    UndoMovePiece(mvsList[nMoveNum].wmv, mvsList[nMoveNum].ucpcCaptured);
  }
  void NullMove(void) {                       
    int dwKey;
    dwKey = zobr.dwKey;
    ChangeSide();
    mvsList[nMoveNum].Set(0, 0,0, dwKey);
    nMoveNum ++;
    nMoveNum2 ++;
    nDistance ++;
  }
  void UndoNullMove(void) {                   
    nDistance --;
    nMoveNum --;
    nMoveNum2 --;
    ChangeSide();
  }
  int GenerateMoves(int *mvs, bool bCapture = 0);
  bool LegalMove(int mv);               
  bool IsMate(void);
  bool RepWuLai(void);
  bool RepWuSong(void);
  bool RepStatus(void);
  bool RepStatus2(void);
  void Mirror(PositionStruct &posMirror) const; 
  bool NullOkay(void) const {                 
    return (sdPlayer == 0 ? vlWhite : vlBlack) > NULL_MARGIN;
  }
};

void PositionStruct::Startup(void) {
  int sq, pc;
  ClearBoard();
  for (sq = 0; sq < 256; sq ++) {
    if(IN_BOARD(sq))
    {
      pc = cucpcStartup[sq];
      if (pc != 0) {
        AddPiece(sq, pc);
      }
    }
  }
  SetIrrev();
  SetIrrev2();
}

int PositionStruct::MovePiece(int mv) {
  int sqSrc, sqDst, pc, pcCaptured;
  sqSrc = SRC(mv);
  sqDst = DST(mv);
  pcCaptured = ucpcSquares[sqDst];
  if (pcCaptured != 0) {
    DelPiece(sqDst, pcCaptured);
  }
  pc = ucpcSquares[sqSrc];
  DelPiece(sqSrc, pc);
  AddPiece(sqDst, pc);
  return pcCaptured;
}

void PositionStruct::UndoMovePiece(int mv, int pcCaptured) {
  int sqSrc, sqDst, pc;
  sqSrc = SRC(mv);
  sqDst = DST(mv);
  pc = ucpcSquares[sqDst];
  DelPiece(sqDst, pc);
  AddPiece(sqSrc, pc);
  if (pcCaptured != 0) {
    AddPiece(sqDst, pcCaptured);
  }
}

bool PositionStruct::MakeMove(int mv) {
  int pcCaptured;
  int dwKey;

  dwKey = zobr.dwKey;

  pcCaptured = MovePiece(mv);
  ChangeSide();
  mvsList[nMoveNum].Set(mv, pcCaptured,ucpcSquares[DST(mv)], dwKey);
  nMoveNum ++;
  nMoveNum2 ++;
  nDistance ++;
  return 1;
}

const bool GEN_CAPTURE = 1;

int PositionStruct::GenerateMoves(int *mvs, bool bCapture){
  int i, j, nGenMoves, nDelta, sqSrc, sqDst;
  int pcSelfSide, pcOppSide, pcSrc, pcDst;

  nGenMoves = 0;
  pcSelfSide = SIDE_TAG(sdPlayer);
  pcOppSide = OPP_SIDE_TAG(sdPlayer);
  for (sqSrc = 0; sqSrc < 256; sqSrc ++) {
    if(!IN_BOARD(sqSrc)) continue;
    pcSrc = ucpcSquares[sqSrc];
    if ((pcSrc & pcSelfSide) == 0) {
      continue;
    }
    for(int delta=0;delta<=3;delta++)
    {
      sqDst=sqSrc+ccDelta[delta];
      if(!IN_BOARD(sqDst)||INDEN(sqDst,pcSelfSide)) continue;
      pcDst = ucpcSquares[sqDst];
      if(!CanMove(sqSrc,sqDst))continue;
      if(!ucpcSquares[sqDst]&&bCapture) continue;
      if (ucpcSquares[sqDst] ? ((pcDst & pcOppSide) != 0  && CanEat(sqSrc,sqDst) ): (pcDst & pcSelfSide) == 0) {
        mvs[nGenMoves] = MOVE(sqSrc, sqDst);
        nGenMoves ++;
      }
    }
    for(int delta=0;delta<=3;delta++)
    { 
      sqDst=sqSrc+ccJumpDelta[delta];
      if(!IN_BOARD(sqDst)||INDEN(sqDst,pcSelfSide)) continue;
      pcDst = ucpcSquares[sqDst];
      if(!CanJump(sqSrc,sqDst))continue;
      if(!ucpcSquares[sqDst]&&bCapture) continue;
      if (ucpcSquares[sqDst] ?  ((pcDst & pcOppSide) != 0  && CanEat(sqSrc,sqDst) ): (pcDst & pcSelfSide) == 0) {
        mvs[nGenMoves] = MOVE(sqSrc, sqDst);
        nGenMoves ++;
      }
    }
  }
  return nGenMoves;
}

bool PositionStruct::LegalMove(int mv){ 
    int sqSrc = SRC(mv), sqDst = DST(mv); 
    if (!IN_BOARD(sqDst)) return 0;   
    int pcSrc = ucpcSquares[sqSrc]; 
    int pcSelfSide = SIDE_TAG(sdPlayer); 
    int pcOppSide = OPP_SIDE_TAG(sdPlayer); 
    if ((pcSrc & pcSelfSide) == 0) return 0; 
    int pcDst = ucpcSquares[sqDst]; 
    if ((pcDst & pcSelfSide) != 0) return 0; 
    if (INDEN(sqDst, pcSelfSide)) return 0; 

    int delta = sqDst - sqSrc; 
    bool isNormal = (delta==-16||delta==-1||delta==1||delta==16); 
    bool isJump   = (delta==-48||delta==-4||delta==4||delta==48); 
    if (!isNormal && !isJump) return 0; 

    if (isNormal && CanMove(sqSrc,sqDst)) { 
        return pcDst ? ((pcDst & pcOppSide)!=0 && CanEat(sqSrc,sqDst)) : true;
    } 
    if (isJump && CanJump(sqSrc,sqDst)) { 
        return pcDst ? ((pcDst & pcOppSide)!=0 && CanEat(sqSrc,sqDst)) : true;
    } 
    return 0;
 } 

bool PositionStruct::IsMate(void) {
  if(((ucpcSquares[99]>=8&&ucpcSquares[99]<=23)&&!sdPlayer)||((ucpcSquares[107]>=8&&ucpcSquares[107]<=23)&&sdPlayer)) return 1;
  return 0;
}

bool PositionStruct::RepWuLai(void){
  if(nMoveNum<=15) return 0;
  if(LegalMove(MOVE(DST(mvsList[nMoveNum-2].wmv),SRC(mvsList[nMoveNum-1].wmv)))) return 0;
  int count[24][256];
  memset(count,0,sizeof(count));
  for(int i=nMoveNum-3;i>=nMoveNum-16&&i>=1;i-=2)
  {
    if(INTRAP(DST(mvsList[i].wmv),16)||INTRAP(DST(mvsList[i].wmv),8))
    {
      return 0;
    }
    count[mvsList[i].wpc][DST(mvsList[i].wmv)]++;
    if(count[mvsList[i].wpc][DST(mvsList[i].wmv)]>=3&&DST(mvsList[i].wmv)==DST(mvsList[nMoveNum-1].wmv))
    {
      return 1;
    }
  }
  return 0;
}
bool PositionStruct::RepWuSong(void){
  if(nMoveNum<=35) return 0;
  int animal=mvsList[nMoveNum-1].wpc,dst=DST(mvsList[nMoveNum-1].wmv),count[6],qigenum=0;
  memset(count,0,sizeof(count));
  for(int i=nMoveNum-3;i>=nMoveNum-36&&i>=0;i-=2)
  {
    if(INTRAP(DST(mvsList[i].wmv),16)||INTRAP(DST(mvsList[i].wmv),8))
    {
      return 0;
    }
    if(mvsList[i].wpc!=animal) return 0;
    bool rep=0;
    for(int j=0;j<qigenum;j++)
    {
      if(DST(mvsList[i].wmv)==count[j])
      {
        rep=1;
        break;
      }
    }
    if(rep==0) count[qigenum++]=DST(mvsList[i].wmv);
    if(qigenum>5) return 0;
  }
  for(int j=0;j<qigenum;j++)
  {
    if(dst==count[j])
    {
      return 1;
    }
  }
  return 0;
}
bool PositionStruct::RepStatus(void){
  if(RepWuLai()) return 1;
  if(RepWuSong()) return 1;
  return 0;
}

bool PositionStruct::RepStatus2(void) {
  bool bSelfSide = 0;
  const MoveStruct *lpmvs;
  int nRecur = 1;
  lpmvs = mvsList + nMoveNum - 1;
  while (lpmvs->wmv != 0 && lpmvs->ucpcCaptured == 0) {
    if (bSelfSide) {
      if (lpmvs->dwKey == zobr.dwKey) {
        nRecur --;
        if (nRecur == 0) return 1;
      }
    }
    bSelfSide = !bSelfSide;
    lpmvs --;
  }
  return 0;
}
static thread_local PositionStruct pos; 

static struct {
  int mvLast;                       
  bool bGameOver;                
  int sqSelected;
  int banmove[MAX_GEN_MOVES],banmoves;
} Xqwl;

static void Startup(void) {
  pos.Startup();
  Xqwl.sqSelected = Xqwl.mvLast = 0;
  Xqwl.bGameOver = 0;
  memset(Xqwl.banmove,0,sizeof(Xqwl.banmove));
  Xqwl.banmoves=0;
}

inline void DrawBoard(void) {
  int x, y, xx, yy, sq, pc;
  for (y = RANK_TOP; y <= RANK_BOTTOM; y ++)
  {
    printf("%2d",y-2);
    for (x = FILE_LEFT; x <= FILE_RIGHT;x ++)
    {
      sq=COORD_XY(x, y);
      if(pos.ucpcSquares[sq]) printf("%s",name[pos.ucpcSquares[sq]]);
      else printf("%s",name[ccInFort[sq]]);
    }
    printf("\n");
  }
  printf("   1 2 3 4 5 6 7 8 9\n");
  if(!pos.sdPlayer) printf("第%d回合（无吃子%d回合），红方走\n",(pos.nMoveNum+1)/2,(pos.nMoveNum2-1)/2);
  else printf("第%d回合（无吃子%d回合），蓝方走\n",(pos.nMoveNum+1)/2,(pos.nMoveNum2-1)/2);
}


struct HashItem {
  short svl;                  
  unsigned char ucDepth, ucFlag;
  int wmv;
  int dwLock0, dwLock1;
};

// v16a优化：置换表由固定1024项改为动态分配、可设定大小的表。
// 同时改用「双层替换」策略取代原本单一slot的做法：
// 每个bucket包含两个HashItem——
//   depthPreferred：优先保留深度较大的搜索结果，只有在新结果深度>=旧结果
//                   深度、或是同一局面(lock相同)时才覆盖，避免深层的宝贵
//                   结果被浅层但常见的局面挤掉；
//   alwaysReplace ：永远覆盖成最新结果，确保浅层、高频重复出现的局面
//                   也能保有一个「最新」的置换表命中，不会被深度优先slot
//                   永久卡位。
// ProbeHash查询时两个slot都会检查，RecordHash写入时按上述规则挑slot。
struct HashBucket {
  HashItem depthPreferred;
  HashItem alwaysReplace;
};

static thread_local int t_mvResult;              
static thread_local int t_nHistoryTable[65536];  
static thread_local int t_mvKillers[100][2];     
static thread_local int t_nCaptureHistory[8][256][8];
static thread_local int t_nCounterMove[8][256];
static thread_local int t_nContHistory[8][256][8][256];

static HashBucket *g_HashTable = NULL; // 动态分配的置换表，大小为g_nHashEntries个bucket
static size_t g_nHashEntries = 0;      // bucket数量，保证为2的幂
static size_t g_nHashMask = 0;         // g_nHashEntries - 1，用于快速取余

// 将期望的MB数换算成不超过该内存上限、且为2的幂的bucket数量。
static size_t ComputeHashEntries(int nMB) {
  size_t nBytes = (size_t)nMB * 1024ULL * 1024ULL;
  size_t nEntries = nBytes / sizeof(HashBucket);
  if (nEntries < 1024) nEntries = 1024; // 至少保留1024个bucket，避免设定过小时表几乎失效
  // 向下取到最接近的2的幂，方便用mask (& (n-1)) 取余，比取模(%)快很多
  size_t nPow2 = 1;
  while ((nPow2 << 1) <= nEntries) nPow2 <<= 1;
  return nPow2;
}

// 依据nHashMB(可由engine.ini的HashMB=设定)动态分配置换表。
// 支持重复调用：若表已存在会先释放，重新按新大小分配。
static void AllocateHashTable(int nMB) {
  if (g_HashTable != NULL) {
    free(g_HashTable);
    g_HashTable = NULL;
  }
  g_nHashEntries = ComputeHashEntries(nMB);
  g_nHashMask = g_nHashEntries - 1;
  g_HashTable = (HashBucket *)calloc(g_nHashEntries, sizeof(HashBucket));
  if (g_HashTable == NULL) {
    // 分配失败时退回最小可用大小，确保引擎仍可运行而不是直接崩溃
    g_nHashEntries = 1024;
    g_nHashMask = g_nHashEntries - 1;
    g_HashTable = (HashBucket *)calloc(g_nHashEntries, sizeof(HashBucket));
  }
}

static void ClearHashTable(void) {
  if (g_HashTable != NULL) {
    memset(g_HashTable, 0, g_nHashEntries * sizeof(HashBucket));
  }
}

static struct {
  char fen[50];
} Search;

#define SearchMvResult t_mvResult
#define SearchNHistoryTable t_nHistoryTable
#define SearchMvKillers t_mvKillers
#define SearchNCaptureHistory t_nCaptureHistory
#define SearchNCounterMove t_nCounterMove
#define SearchNContHistory t_nContHistory

void map_to_fen(void)
{
  strcpy(Search.fen,"\0");
  int x,y,empty=0;
  for(y=RANK_TOP;y<=RANK_BOTTOM;y++)
  {
    for(x=FILE_LEFT;x<=FILE_RIGHT;x++)
    {
      int sq=COORD_XY(x,y);
      if(pos.ucpcSquares[sq]==0)
      {
        empty++;
      }
      else
      {
        if(empty>=1)
        {
          char aa[3];
          aa[0]=empty+'0';
          aa[1]='\0';
          strcat(Search.fen,aa);
          empty=0;
        }
        switch(pos.ucpcSquares[sq])
        {
          case 8: strcat(Search.fen,"E"); break;
          case 9: strcat(Search.fen,"L"); break;
          case 10: strcat(Search.fen,"T"); break;
          case 11: strcat(Search.fen,"P"); break;
          case 12: strcat(Search.fen,"W"); break;
          case 13: strcat(Search.fen,"D"); break;
          case 14: strcat(Search.fen,"C"); break;
          case 15: strcat(Search.fen,"M"); break;
          case 16: strcat(Search.fen,"e"); break;
          case 17: strcat(Search.fen,"l"); break;
          case 18: strcat(Search.fen,"t"); break;
          case 19: strcat(Search.fen,"p"); break;
          case 20: strcat(Search.fen,"w"); break;
          case 21: strcat(Search.fen,"d"); break;
          case 22: strcat(Search.fen,"c"); break;
          case 23: strcat(Search.fen,"m"); break;
        }
      }
    }
    if(empty>=1)
    {
      char aa[3];
      aa[0]=empty+'0';
      aa[1]='\0';
      strcat(Search.fen,aa);
      empty=0;
    }
    if(y<RANK_BOTTOM) strcat(Search.fen,"/");
  }
  if(pos.sdPlayer) strcat(Search.fen," b");
  else strcat(Search.fen," r");
}

void fen_to_map(char fen[])
{
  pos.ClearBoard();
  int x,y,i;
  x=FILE_LEFT;
  y=RANK_TOP;
  for(i=0;y<=RANK_BOTTOM;i++)
  {
    if(fen[i]=='/'||fen[i]==' ')
    {
      x=FILE_LEFT;
      y++;
    }
    else if(fen[i]>='0'&&fen[i]<='9')
    {
      for(int j=1;j<=fen[i]-'0';j++)
      {
        x++;
      }
    }
    else if(fen[i]!=' ')
    {
      switch(fen[i])
      {
        case 'w': pos.AddPiece(COORD_XY(x,y),20); break;
        case 'p': pos.AddPiece(COORD_XY(x,y),19); break;
        case 't': pos.AddPiece(COORD_XY(x,y),18); break;
        case 'l': pos.AddPiece(COORD_XY(x,y),17); break;
        case 'e': pos.AddPiece(COORD_XY(x,y),16); break;
        case 'd': pos.AddPiece(COORD_XY(x,y),21); break;
        case 'c': pos.AddPiece(COORD_XY(x,y),22); break;
        case 'm': pos.AddPiece(COORD_XY(x,y),23); break;
        case 'W': pos.AddPiece(COORD_XY(x,y),12); break;
        case 'P': pos.AddPiece(COORD_XY(x,y),11); break;
        case 'T': pos.AddPiece(COORD_XY(x,y),10); break;
        case 'L': pos.AddPiece(COORD_XY(x,y),9); break;
        case 'E': pos.AddPiece(COORD_XY(x,y),8); break;
        case 'D': pos.AddPiece(COORD_XY(x,y),13); break;
        case 'C': pos.AddPiece(COORD_XY(x,y),14); break;
        case 'M': pos.AddPiece(COORD_XY(x,y),15); break;
      }
      x++;
    }
  }
  if(fen[i]!='r') pos.ChangeSide();
  pos.SetIrrev();
  pos.SetIrrev2();
  Xqwl.sqSelected = Xqwl.mvLast = 0;
  Xqwl.bGameOver = 0;
  Xqwl.banmoves = 0;
  if(strncmp(fen+i+1," moves ",7)==0)
  {
    i+=8;
    for(;fen[i-1]!='\0';i+=5)
    {
      int mv=MOVE(COORD_XY(fen[i]-'a'+3,fen[i+1]-'0'+3),COORD_XY(fen[i+2]-'a'+3,fen[i+3]-'0'+3));
      pos.MakeMove(mv);
    }
  }
}

// v16a优化：ProbeHash现在会同时检查同一bucket里的depthPreferred与
// alwaysReplace两个slot，哪个命中(lock相符)且满足深度/边界条件就用哪个，
// 双层设计让浅层高频局面与深层关键局面都有机会留在表内而不互相挤占。
static int ProbeHash(int vlAlpha, int vlBeta, int nDepth, int &mv) {
  bool bMate;
  HashItem hsh;
  HashBucket &bucket = g_HashTable[pos.zobr.dwKey & g_nHashMask];

  bool bHitDeep = (bucket.depthPreferred.dwLock0 == pos.zobr.dwLock0 &&
                    bucket.depthPreferred.dwLock1 == pos.zobr.dwLock1);
  bool bHitAlways = (bucket.alwaysReplace.dwLock0 == pos.zobr.dwLock0 &&
                      bucket.alwaysReplace.dwLock1 == pos.zobr.dwLock1);

  // 优先用深度优先slot；若其未命中或深度不足，再看看alwaysReplace slot
  // 是否有更新鲜、更适用的资料。
  if (bHitDeep) {
    hsh = bucket.depthPreferred;
  } else if (bHitAlways) {
    hsh = bucket.alwaysReplace;
  } else {
    mv = 0;
    return -MATE_VALUE;
  }

  mv = hsh.wmv;
  bMate = false;
  if (hsh.svl > WIN_VALUE) {
    hsh.svl -= pos.nDistance;
    bMate = true;
  } else if (hsh.svl < -WIN_VALUE) {
    hsh.svl += pos.nDistance;
    bMate = true;
  }

  if (hsh.ucDepth >= nDepth) {
    if (hsh.ucFlag == HASH_BETA) {
      return (hsh.svl >= vlBeta ? hsh.svl : -MATE_VALUE);
    } else if (hsh.ucFlag == HASH_ALPHA) {
      return (hsh.svl <= vlAlpha ? hsh.svl : -MATE_VALUE);
    }
    return hsh.svl;
  }

  // depthPreferred深度不足时，若alwaysReplace命中且深度够，也可以用
  if (bHitDeep && bHitAlways) {
    HashItem hsh2 = bucket.alwaysReplace;
    if (hsh2.ucDepth >= nDepth) {
      mv = hsh2.wmv;
      if (hsh2.svl > WIN_VALUE) hsh2.svl -= pos.nDistance;
      else if (hsh2.svl < -WIN_VALUE) hsh2.svl += pos.nDistance;
      if (hsh2.ucFlag == HASH_BETA) {
        return (hsh2.svl >= vlBeta ? hsh2.svl : -MATE_VALUE);
      } else if (hsh2.ucFlag == HASH_ALPHA) {
        return (hsh2.svl <= vlAlpha ? hsh2.svl : -MATE_VALUE);
      }
      return hsh2.svl;
    }
  }

  (void)bMate;
  return -MATE_VALUE;
};

// v16a优化：RecordHash写入时采用双层替换策略——
// 1) depthPreferred slot：只有在同局面(lock相符)、或新结果深度>=旧结果
//    深度时才覆盖，让深搜得到的宝贵结果尽量留久一点；
// 2) alwaysReplace slot：永远直接覆盖成最新结果，确保浅层但高频出现的
//    局面（例如开局阶段重复走位）也总有一个「最新」命中可用，
//    不会被深度优先slot长期占住位置而挤不进表。
static void RecordHash(int nFlag, int vl, int nDepth, int mv) {
  HashBucket &bucket = g_HashTable[pos.zobr.dwKey & g_nHashMask];

  short svlToStore;
  if (vl > WIN_VALUE) {
    svlToStore = vl + pos.nDistance;
  } else if (vl < -WIN_VALUE) {
    svlToStore = vl - pos.nDistance;
  } else {
    svlToStore = vl;
  }

  // --- depthPreferred slot ---
  HashItem &deep = bucket.depthPreferred;
  bool bSameLockDeep = (deep.dwLock0 == pos.zobr.dwLock0 && deep.dwLock1 == pos.zobr.dwLock1);
  if (!bSameLockDeep || deep.ucDepth <= nDepth) {
    deep.ucFlag = nFlag;
    deep.ucDepth = nDepth;
    deep.svl = svlToStore;
    deep.wmv = mv;
    deep.dwLock0 = pos.zobr.dwLock0;
    deep.dwLock1 = pos.zobr.dwLock1;
  }

  // --- alwaysReplace slot：永远覆盖 ---
  HashItem &always = bucket.alwaysReplace;
  always.ucFlag = nFlag;
  always.ucDepth = nDepth;
  always.svl = svlToStore;
  always.wmv = mv;
  always.dwLock0 = pos.zobr.dwLock0;
  always.dwLock1 = pos.zobr.dwLock1;
};

static int cucMvvLva[24] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  7, 8, 6, 5, 3, 2, 1, 4,
  7, 8, 6, 5, 3, 2, 1, 4
};

inline int MvvLva(int mv) {
  return (cucMvvLva[pos.ucpcSquares[DST(mv)]] << 3) - cucMvvLva[pos.ucpcSquares[SRC(mv)]];
}
static int CompareMvvLva(const void *lpmv1, const void *lpmv2) {
  return MvvLva(*(int *) lpmv2) - MvvLva(*(int *) lpmv1);
}

inline int PrevMoveForContHistory(void);
inline int GetCounterMove(int prevMv);
inline int GetContinuationHistoryScore(int prevMv, int mv);

inline int QuietOrderScore(int mv) {
  int score = SearchNHistoryTable[mv];
  int prevMv = PrevMoveForContHistory();
  score += GetContinuationHistoryScore(prevMv, mv) * 2;
  if (GetCounterMove(prevMv) == mv) score += 16000;
  return score;
}

static int CompareHistory(const void *lpmv1, const void *lpmv2) {
  return QuietOrderScore(*(int *) lpmv2) - QuietOrderScore(*(int *) lpmv1);
}
struct ScoredMove {
  int mv;
  int score;
};

static int CompareScoredMoveDesc(const void *a, const void *b);
inline int CaptureOrderScore(int mv);
const int PHASE_HASH = 0;
const int PHASE_KILLER_1 = 1;
const int PHASE_KILLER_2 = 2;
const int PHASE_GEN_MOVES = 3;
const int PHASE_REST = 4;

struct SortStruct {
  int mvHash, mvKiller1, mvKiller2;
  int nPhase, nIndex, nGenMoves;
  int mvs[MAX_GEN_MOVES];
  int captureCount;
  int captures[MAX_GEN_MOVES];
  int quietCount;
  int quiets[MAX_GEN_MOVES];
  ScoredMove scoredCaptures[MAX_GEN_MOVES];
  ScoredMove scoredQuiets[MAX_GEN_MOVES];

  void Init(int mvHash_) {
    mvHash = mvHash_;
    mvKiller1 = SearchMvKillers[pos.nDistance][0];
    mvKiller2 = SearchMvKillers[pos.nDistance][1];
    nPhase = PHASE_HASH;
    nIndex = 0;
    nGenMoves = 0;
    captureCount = 0;
    quietCount = 0;
  }
  int Next(void);
};

inline int SortStruct::Next(void) {
  int mv;
  switch (nPhase) {
  case PHASE_HASH:
    nPhase = PHASE_KILLER_1;
    if (mvHash != 0) {
      return mvHash;
    }

  case PHASE_KILLER_1:
    nPhase = PHASE_KILLER_2;
    if (mvKiller1 != mvHash && mvKiller1 != 0 && pos.LegalMove(mvKiller1)) {
      return mvKiller1;
    }

  case PHASE_KILLER_2:
    nPhase = PHASE_GEN_MOVES;
    if (mvKiller2 != mvHash && mvKiller2 != 0 && pos.LegalMove(mvKiller2)) {
      return mvKiller2;
    }

  case PHASE_GEN_MOVES:
    nPhase = PHASE_REST;
    nGenMoves = pos.GenerateMoves(mvs);
    captureCount = 0;
    quietCount = 0;
    for (int i = 0; i < nGenMoves; i++) {
      mv = mvs[i];
      if (pos.ucpcSquares[DST(mv)] != 0) {
        scoredCaptures[captureCount].mv = mv;
        scoredCaptures[captureCount].score = CaptureOrderScore(mv);
        captureCount++;
      } else {
        scoredQuiets[quietCount].mv = mv;
        scoredQuiets[quietCount].score = QuietOrderScore(mv);
        quietCount++;
      }
    }
    if (captureCount > 1) {
      qsort(scoredCaptures, captureCount, sizeof(ScoredMove), CompareScoredMoveDesc);
    }
    if (quietCount > 1) {
      qsort(scoredQuiets, quietCount, sizeof(ScoredMove), CompareScoredMoveDesc);
    }
    for (int i = 0; i < captureCount; i++) captures[i] = scoredCaptures[i].mv;
    for (int i = 0; i < quietCount; i++) quiets[i] = scoredQuiets[i].mv;
    nIndex = 0;

  case PHASE_REST:
    while (nIndex < captureCount) {
      mv = captures[nIndex++];
      if (mv != mvHash && mv != mvKiller1 && mv != mvKiller2 && pos.LegalMove(mv)) {
        return mv;
      }
    }
    while (nIndex - captureCount < quietCount) {
      mv = quiets[nIndex - captureCount];
      nIndex ++;
      if (mv != mvHash && mv != mvKiller1 && mv != mvKiller2 && pos.LegalMove(mv)) {
        return mv;
      }
    }
  default:
    return 0;
  }
}

static const int MAX_HISTORY = 16384;

inline bool IsCaptureMove(int mv) {
  return pos.ucpcSquares[DST(mv)] != 0;
}

inline void UpdateHistoryScore(int mv, int bonus) {
  if (mv == 0) return;
  if (bonus > MAX_HISTORY) bonus = MAX_HISTORY;
  if (bonus < -MAX_HISTORY) bonus = -MAX_HISTORY;
  int &h = SearchNHistoryTable[mv];
  h += bonus - h * abs(bonus) / MAX_HISTORY;
}

inline int HistoryBonusFromDepth(int nDepth) {
  int bonus = nDepth * nDepth;
  if (bonus < 1) bonus = 1;
  if (bonus > 1200) bonus = 1200;
  return bonus;
}

inline int HistoryReductionAdjust(int histScore) {
  if (histScore >= 12000) return -2;
  if (histScore >= 8000) return -1;
  if (histScore >= 4000) return -1;
  if (histScore <= -8000) return 2;
  if (histScore <= -4000) return 1;
  return 0;
}

inline void UpdateKillerMove(int mv) {
  int *lpmvKillers = SearchMvKillers[pos.nDistance];
  if (lpmvKillers[0] != mv) {
    lpmvKillers[1] = lpmvKillers[0];
    lpmvKillers[0] = mv;
  }
}

inline int PrevMoveForContHistory(void) {
  if (pos.nMoveNum <= 1) return 0;
  return pos.mvsList[pos.nMoveNum - 1].wmv;
}

inline int MoveHistPiece(int mv) {
  int pc = pos.ucpcSquares[SRC(mv)];
  return (pc == 0 ? 0 : PIECE_NAME(pc));
}

inline int MoveHistTo(int mv) {
  return DST(mv);
}

inline int GetCounterMove(int prevMv) {
  if (prevMv == 0) return 0;
  return SearchNCounterMove[MoveHistPiece(prevMv)][MoveHistTo(prevMv)];
}

inline int GetContinuationHistoryScore(int prevMv, int mv) {
  if (prevMv == 0 || mv == 0) return 0;
  return SearchNContHistory[MoveHistPiece(prevMv)][MoveHistTo(prevMv)][MoveHistPiece(mv)][MoveHistTo(mv)];
}

inline void UpdateCounterMove(int prevMv, int mv) {
  if (prevMv == 0 || mv == 0) return;
  SearchNCounterMove[MoveHistPiece(prevMv)][MoveHistTo(prevMv)] = mv;
}

inline void UpdateContinuationHistoryScore(int prevMv, int mv, int bonus) {
  if (prevMv == 0 || mv == 0) return;
  if (bonus > MAX_HISTORY) bonus = MAX_HISTORY;
  if (bonus < -MAX_HISTORY) bonus = -MAX_HISTORY;
  int &h = SearchNContHistory[MoveHistPiece(prevMv)][MoveHistTo(prevMv)][MoveHistPiece(mv)][MoveHistTo(mv)];
  h += bonus - h * abs(bonus) / MAX_HISTORY;
}

inline void UpdateQuietHistoryBest(int mv, int nDepth) {
  int prevMv = PrevMoveForContHistory();
  UpdateHistoryScore(mv, HistoryBonusFromDepth(nDepth));
  UpdateContinuationHistoryScore(prevMv, mv, HistoryBonusFromDepth(nDepth));
  UpdateCounterMove(prevMv, mv);
  UpdateKillerMove(mv);
}

inline void UpdateQuietHistoryMalus(const int *quiets, int nQuiets, int mvBest, int nDepth) {
  int prevMv = PrevMoveForContHistory();
  int malus = HistoryBonusFromDepth(nDepth);
  if (malus > 800) malus = 800;
  for (int i = 0; i < nQuiets; i++) {
    int mv = quiets[i];
    if (mv != 0 && mv != mvBest) {
      UpdateHistoryScore(mv, -malus);
      UpdateContinuationHistoryScore(prevMv, mv, -malus);
    }
  }
}

inline int &CaptureHistoryEntry(int mv, int capturedPc) {
  return SearchNCaptureHistory[PIECE_NAME(pos.ucpcSquares[SRC(mv)])][DST(mv)][PIECE_NAME(capturedPc)];
}

inline void UpdateCaptureHistoryScore(int mv, int capturedPc, int bonus) {
  if (mv == 0 || capturedPc == 0) return;
  if (bonus > MAX_HISTORY) bonus = MAX_HISTORY;
  if (bonus < -MAX_HISTORY) bonus = -MAX_HISTORY;
  int &h = CaptureHistoryEntry(mv, capturedPc);
  h += bonus - h * abs(bonus) / MAX_HISTORY;
}

inline void UpdateCaptureHistoryBest(int mv, int capturedPc, int nDepth) {
  UpdateCaptureHistoryScore(mv, capturedPc, HistoryBonusFromDepth(nDepth));
}

inline void UpdateCaptureHistoryMalus(const int *caps, const int *capPieces, int nCaps, int mvBest, int nDepth) {
  int malus = HistoryBonusFromDepth(nDepth);
  if (malus > 800) malus = 800;
  for (int i = 0; i < nCaps; i++) {
    int mv = caps[i];
    int pcCaptured = capPieces[i];
    if (mv != 0 && mv != mvBest && pcCaptured != 0) {
      UpdateCaptureHistoryScore(mv, pcCaptured, -malus);
    }
  }
}

inline void ClearCaptureHist(void) {
  memset(SearchNCaptureHistory, 0, sizeof(SearchNCaptureHistory));
}

inline void ClearContinuationHist(void) {
  memset(SearchNCounterMove, 0, sizeof(SearchNCounterMove));
  memset(SearchNContHistory, 0, sizeof(SearchNContHistory));
}

inline void SetBestMove(int mv, bool bIsCapture, int capturedPc, int nDepth) {
  if (mv == 0) return;
  if (bIsCapture) {
    if (capturedPc != 0) {
      UpdateCaptureHistoryBest(mv, capturedPc, nDepth);
    }
  } else {
    UpdateQuietHistoryBest(mv, nDepth);
  }
}

inline void ClearCorrHist(void) {
  memset(SearchCorrHist, 0, sizeof(int) * 8);
}

inline void UpdateCorrHist(int mvBest, int vlThisDepth, int vlLastDepth) {
  if (mvBest == 0 || vlLastDepth <= -MATE_VALUE) return;
  if (abs(vlThisDepth) >= WIN_VALUE || abs(vlLastDepth) >= WIN_VALUE) return;

  // Correction history 只在 quiet best move 時更新，避免 tactical capture
  // 把高噪音的搜尋分數差直接寫進修正表。
  if (pos.ucpcSquares[DST(mvBest)] != 0) return;

  // 合理條件：若本層分數沒有真正偏離上一層基準，就不更新。
  int nDiff = vlThisDepth - vlLastDepth;
  if (nDiff == 0) return;

  int pcMoved = pos.ucpcSquares[SRC(mvBest)];
  if (pcMoved == 0) return;
  int pieceMoved = PIECE_NAME(pcMoved);
  SearchCorrHist[pieceMoved] += (nDiff - SearchCorrHist[pieceMoved]) / 4;
  if (SearchCorrHist[pieceMoved] > CORR_HIST_LIMIT) SearchCorrHist[pieceMoved] = CORR_HIST_LIMIT;
  if (SearchCorrHist[pieceMoved] < -CORR_HIST_LIMIT) SearchCorrHist[pieceMoved] = -CORR_HIST_LIMIT;
}

static int cucPieceValue[8] = {320,360,260,100,85,62,50,95};

inline int PieceValueByPc(int pc) {
  return (pc == 0 ? 0 : cucPieceValue[PIECE_NAME(pc)]);
}

inline int FindLeastValuableAttackerSquare(int sqTarget, bool sdAttacker) {
  int pcSelfSide = SIDE_TAG(sdAttacker);
  int bestSq = 0;
  int bestVal = 1 << 30;
  for (int sqSrc = 0; sqSrc < 256; sqSrc++) {
    if (!IN_BOARD(sqSrc)) continue;
    int pcSrc = pos.ucpcSquares[sqSrc];
    if (pcSrc == 0 || (pcSrc & pcSelfSide) == 0) continue;
    if ((abs(sqTarget - sqSrc) == 16 || abs(sqTarget - sqSrc) == 1) && pos.CanMove(sqSrc, sqTarget) && pos.CanEat(sqSrc, sqTarget)) {
      int v = PieceValueByPc(pcSrc);
      if (v < bestVal) {
        bestVal = v;
        bestSq = sqSrc;
      }
    }
    if ((abs(sqTarget - sqSrc) == 48 || abs(sqTarget - sqSrc) == 4) && pos.CanJump(sqSrc, sqTarget) && pos.CanEat(sqSrc, sqTarget)) {
      int v = PieceValueByPc(pcSrc);
      if (v < bestVal) {
        bestVal = v;
        bestSq = sqSrc;
      }
    }
  }
  return bestSq;
}

static int StaticExchangeEval(int mv) {
  int sqSrc = SRC(mv), sqDst = DST(mv);
  int pcSrc = pos.ucpcSquares[sqSrc];
  int pcDst = pos.ucpcSquares[sqDst];
  if (pcSrc == 0 || pcDst == 0) return 0;
  if (!pos.CanEat(sqSrc, sqDst)) return -MATE_VALUE;

  int gain[32];
  int depth = 0;
  gain[0] = PieceValueByPc(pcDst);

  PositionStruct bak = pos;
  if (!pos.MakeMove(mv)) {
    pos = bak;
    return -MATE_VALUE;
  }

  bool sdSide = pos.sdPlayer;
  while (depth < 30) {
    int sqAtk = FindLeastValuableAttackerSquare(sqDst, sdSide);
    if (sqAtk == 0) break;
    int pcAtk = pos.ucpcSquares[sqAtk];
    if (pcAtk == 0) break;
    depth++;
    gain[depth] = PieceValueByPc(pcAtk) - gain[depth - 1];
    int reply = MOVE(sqAtk, sqDst);
    if (!pos.MakeMove(reply)) {
      depth--;
      break;
    }
    sdSide = pos.sdPlayer;
  }

  while (depth > 0) {
    depth--;
    if (-gain[depth + 1] < gain[depth]) gain[depth] = -gain[depth + 1];
  }

  pos = bak;
  return gain[0];
}

inline int CaptureOrderScore(int mv) {
  int see = StaticExchangeEval(mv);
  if (see <= -MATE_VALUE) return see;
  int pcCaptured = pos.ucpcSquares[DST(mv)];
  int hist = (pcCaptured == 0 ? 0 : SearchNCaptureHistory[PIECE_NAME(pos.ucpcSquares[SRC(mv)])][DST(mv)][PIECE_NAME(pcCaptured)]);
  return see * 4096 + hist * 4 + MvvLva(mv);
}

static int CompareCaptureSEE(const void *lpmv1, const void *lpmv2) {
  return CaptureOrderScore(*(int *) lpmv2) - CaptureOrderScore(*(int *) lpmv1);
}

static int CompareScoredMoveDesc(const void *a, const void *b) {
  return ((const ScoredMove *)b)->score - ((const ScoredMove *)a)->score;
}

// =====================================================================
// 新增：先做好前置宣告與定義順序，避免 PollTimeAndMaybeStop 用到的
// node / CheckTimeUp 因為原本定義在後面而找不到符號。
// =====================================================================

// node 提前宣告在此（原本這行在 typedef LINE 之後，現在移到這裡，
// 下面 typedef LINE 區塊那邊的重複宣告已移除，避免 thread_local 重複定義）
thread_local long long node = 0;

// CheckTimeUp 提前宣告（原本定義在 SearchRootWindow 之前，現在改用
// forward declaration，實際定義維持在原本位置，內容不變）
inline bool CheckTimeUp(void);

// =====================================================================
// 共用的節點計數超時檢查。SearchFull / SearchQuiesc 每隔固定節點數
// 就主動檢查一次是否超時，超時立刻設 g_stopSearch 並讓遞迴逐層快速返回，
// 不再只靠外層迭代加深迴圈在每一整層結束後才檢查一次時間。
// 只有主執行緒(g_isMainThread)才會真正因超時觸發 stop，worker 執行緒
// 只被動檢查 g_stopSearch 標記，避免 Lazy SMP 背景執行緒各自誤判時間。
// =====================================================================
static const long long TIME_CHECK_NODE_INTERVAL = 2048;

inline bool PollTimeAndMaybeStop(void) {
  if (g_stopSearch.load(std::memory_order_relaxed)) {
    return true;
  }
  if ((node & (TIME_CHECK_NODE_INTERVAL - 1)) == 0) {
    if (g_isMainThread && CheckTimeUp()) {
      g_stopSearch.store(true, std::memory_order_relaxed);
      return true;
    }
  }
  return false;
}

static int SearchQuiesc(int vlAlpha, int vlBeta) {
  int i, nGenMoves;
  int vl, vlBest, vlStandPat;
  int mvs[MAX_GEN_MOVES];
  int seeScores[MAX_GEN_MOVES];
  ScoredMove scoredCaps[MAX_GEN_MOVES];

  node++;
  g_totalNodes.fetch_add(1, std::memory_order_relaxed);

  // 修正2：入口就檢查超時/停止，不再只檢查 g_stopSearch
  if (PollTimeAndMaybeStop()) {
    return pos.nDistance - MATE_VALUE;
  }

  if (pos.IsMate()) {
    return pos.nDistance - MATE_VALUE;
  }

  vlBest = -MATE_VALUE;

   {
    vlStandPat = pos.Evaluate();
    vl = vlStandPat;
    if (vl > vlBest) {
      vlBest = vl;
      if (vl >= vlBeta) {
        return vl;
      }
      if (vl > vlAlpha) {
        vlAlpha = vl;
      }
    }

    nGenMoves = pos.GenerateMoves(mvs, GEN_CAPTURE);
    for (i = 0; i < nGenMoves; i++) {
      int see = StaticExchangeEval(mvs[i]);
      seeScores[i] = see;
      scoredCaps[i].mv = mvs[i];
      if (see <= -MATE_VALUE) {
        scoredCaps[i].score = see;
      } else {
        int pcCaptured = pos.ucpcSquares[DST(mvs[i])];
        int hist = (pcCaptured == 0 ? 0 : SearchNCaptureHistory[PIECE_NAME(pos.ucpcSquares[SRC(mvs[i])])][DST(mvs[i])][PIECE_NAME(pcCaptured)]);
        scoredCaps[i].score = see * 4096 + hist * 4 + MvvLva(mvs[i]);
      }
    }
    if (nGenMoves > 1) {
      qsort(scoredCaps, nGenMoves, sizeof(ScoredMove), CompareScoredMoveDesc);
    }
    for (i = 0; i < nGenMoves; i++) {
      mvs[i] = scoredCaps[i].mv;
    }
  }

  const int DELTA_MARGIN = 50;

  for (i = 0; i < nGenMoves; i ++) {
    // 修正2：吃子迴圈內部同樣定期檢查超時，近殺局面吃子交換多時尤其重要
    if (PollTimeAndMaybeStop()) {
      break;
    }
    int mv = scoredCaps[i].mv;
    int see = seeScores[i];
    int pcCaptured = pos.ucpcSquares[DST(mv)];
    if (see <= -MATE_VALUE) {
      continue;
    }
    if (see < 0 && vlStandPat + see <= vlAlpha) {
      continue;
    }
    if (pcCaptured != 0) {
      int capVal = cucPieceValue[PIECE_NAME(pcCaptured)];
      if (vlStandPat + capVal + DELTA_MARGIN < vlAlpha) {
        continue;
      }
    }
    if (pos.MakeMove(mv)) {
      vl = -SearchQuiesc(-vlBeta, -vlAlpha);
      pos.UndoMakeMove();

      if (vl > vlBest) {
        vlBest = vl;
        if (vl >= vlBeta) {
          return vl;
        }
        if (vl > vlAlpha) {
          vlAlpha = vl;
        }
      }
    }
  }

  return vlBest == -MATE_VALUE ? pos.nDistance - MATE_VALUE : vlBest;
}

typedef struct tagLINE{
int cmove;
int argmove[100];
} LINE;
const BOOL NO_NULL = true;

static const int FUTILITY_MARGIN[4] = {0, 100, 180, 260};

static const int SINGULAR_MARGIN = 30;
static const int SINGULAR_EXT_DEPTH = 2;
static const int MAX_SINGULAR_EXT_TOTAL = 6;
static thread_local int t_nSingularExtCount = 0;

static const int REVERSE_MULTICUT_RANK_LIMIT = 3;

static int SearchFull(int vlAlpha, int vlBeta, int nDepth, LINE *pline, BOOL bNoNull = 0) {
  LINE line;
  memset(&line, 0, sizeof(LINE));
  pline->cmove = 0;
  node++;
  g_totalNodes.fetch_add(1, std::memory_order_relaxed);

  // 修正1：入口先做節點計數超時檢查，取代原本只查 g_stopSearch
  if (PollTimeAndMaybeStop()) {
    pline->cmove = 0;
    return pos.nDistance - MATE_VALUE;
  }

  int nHashFlag, vl, vlBest;
  int mv, mvBest, mvHash, nNewDepth;
  SortStruct Sort;
  bool bInCheck;
  int nMoveCount;
  int quietsSearched[MAX_GEN_MOVES];
  int nQuietsSearched;
  int capturesSearched[MAX_GEN_MOVES];
  int capturePieces[MAX_GEN_MOVES];
  int nCapturesSearched;

  if(pos.IsMate())
  {
    pline->cmove = 0;
    return -MATE_VALUE;
  }
  if(pos.RepStatus())
  {
    pline->cmove = 0;
    return MATE_VALUE;
  }
  if(pos.RepStatus2())
  {
    SearchNHistoryTable[pos.mvsList[pos.nMoveNum-1].wmv] -= 10;
    pline->cmove = 0;
    return 0;
  }

  if(nDepth<=0)
  {
    pline->cmove=0;
    return SearchQuiesc(vlAlpha,vlBeta);
  }

  vl = ProbeHash(vlAlpha, vlBeta, nDepth, mvHash);
  if (vl > -MATE_VALUE) {
    pline->cmove = 0;
    return vl;
  }

  if (!bNoNull && pos.NullOkay()) {
    pos.NullMove();
    vl = -SearchFull(-vlBeta, 1 - vlBeta, nDepth - NULL_DEPTH - 1,&line, NO_NULL);
    pos.UndoNullMove();
    if (vl >= vlBeta) {
      pline->cmove=0;
      return vl;
    }
  }

  // 修正1：空步裁剪之後、正式展開走法之前也補一次檢查，
  // 避免空步搜索本身花了很久卻沒被上層及時發現
  if (PollTimeAndMaybeStop()) {
    pline->cmove = 0;
    return pos.nDistance - MATE_VALUE;
  }

  bInCheck = pos.IsMate();
  if (!bNoNull && !bInCheck && nDepth >= 1 && nDepth <= 3 && vlBeta < WIN_VALUE && vlAlpha > -WIN_VALUE) {
    int vlEval = pos.Evaluate();
    if (vlEval + FUTILITY_MARGIN[nDepth] <= vlAlpha) {
      vl = SearchQuiesc(vlAlpha, vlBeta);
      if (vl <= vlAlpha) {
        pline->cmove = 0;
        return vl;
      }
    }
  }

  nHashFlag = HASH_ALPHA;
  vlBest = -MATE_VALUE;
  mvBest = 0;
  nMoveCount = 0;
  nQuietsSearched = 0;
  nCapturesSearched = 0;

  Sort.Init(mvHash);
  while ((mv = Sort.Next()) != 0) {
    // 修正1：走法迴圈內部逐一檢查，避免某個節點分支特別龐大時拖住整體
    if (PollTimeAndMaybeStop()) {
      break;
    }

    if (pos.MakeMove(mv)) {
      nMoveCount ++;
      int capturedPc = pos.mvsList[pos.nMoveNum-1].ucpcCaptured;
      bool bIsCapture = (capturedPc != 0);
      if (!bIsCapture && nQuietsSearched < MAX_GEN_MOVES) {
        quietsSearched[nQuietsSearched++] = mv;
      }
      if (bIsCapture && nCapturesSearched < MAX_GEN_MOVES) {
        capturesSearched[nCapturesSearched] = mv;
        capturePieces[nCapturesSearched] = pos.mvsList[pos.nMoveNum-1].ucpcCaptured;
        nCapturesSearched ++;
      }
      nNewDepth = nDepth - 1;
      if(pos.IsMate()) nNewDepth = nDepth;
      bool bGivesCheck = (nNewDepth == nDepth);

      int nReduction = 0;
      int nHistScore = SearchNHistoryTable[mv];
      bool bCanReduce = (!bIsCapture && !bInCheck && !bGivesCheck && nNewDepth == nDepth - 1 && nDepth >= 3 && vlBest != -MATE_VALUE && nMoveCount >= 3);
      if (bCanReduce) {
        // c. LMR 條件細化：除了 quiet history 分數，額外考慮
        // killer move、countermove、continuation history 這幾個
        // 「這步在當前上下文有多可疑/多有希望」的訊號，讓 reduction
        // 更貼近走法真實品質，而不是只看第幾手與 depth。
        int prevMvForLmr = PrevMoveForContHistory();
        bool bIsKillerMove = (mv == Sort.mvKiller1 || mv == Sort.mvKiller2);
        bool bIsCounterMove = (GetCounterMove(prevMvForLmr) == mv);
        int nContHistScore = GetContinuationHistoryScore(prevMvForLmr, mv);

        double dReduction = log((double)(nDepth + 1)) * log((double)(nMoveCount + 1)) / 2.25;
        if (nMoveCount >= 8) dReduction += 0.35;
        if (nDepth >= 8) dReduction += 0.25;
        dReduction += (double)HistoryReductionAdjust(nHistScore);

        // 額外訊號：killer/countermove 命中，或 continuation history
        // 分數偏高，代表這步在類似上下文曾經很有用，減少 reduction；
        // continuation history 分數偏低則代表這類接續走法一向表現差，
        // 可以再減多一點深度。
        if (bIsKillerMove) dReduction -= 0.5;
        if (bIsCounterMove) dReduction -= 0.5;
        if (nContHistScore >= 8000) dReduction -= 1.0;
        else if (nContHistScore >= 4000) dReduction -= 0.5;
        else if (nContHistScore <= -8000) dReduction += 1.0;
        else if (nContHistScore <= -4000) dReduction += 0.5;

        if (dReduction < 0.0) dReduction = 0.0;
        nReduction = (int)(dReduction + 0.5);
        if (nMoveCount <= 4 && nReduction > 1) nReduction = 1;
        if (nHistScore >= 8000) nReduction = 0;
        else if (nHistScore >= 4000 && nReduction > 0) nReduction --;
        if (bIsKillerMove || bIsCounterMove) {
          if (nReduction > 0) nReduction --;
        }
        if (nReduction > nNewDepth - 1) nReduction = nNewDepth - 1;
        if (nReduction < 0) nReduction = 0;
      }

      if (vlBest == -MATE_VALUE) {
        vl = -SearchFull(-vlBeta, -vlAlpha, nNewDepth, &line);
      } else {
        vl = -SearchFull(-vlAlpha-1,-vlAlpha, nNewDepth - nReduction, &line);
        if (nReduction && vl > vlAlpha) {
          vl = -SearchFull(-vlAlpha-1,-vlAlpha, nNewDepth, &line);
        }
        if (vl <= vlAlpha && vl > vlAlpha - SINGULAR_MARGIN &&
            nMoveCount <= REVERSE_MULTICUT_RANK_LIMIT && nNewDepth >= 1 &&
            t_nSingularExtCount < MAX_SINGULAR_EXT_TOTAL) {
          t_nSingularExtCount ++;
          int nVerifyDepth = nNewDepth + SINGULAR_EXT_DEPTH;
          int vlVerify = -SearchFull(-vlAlpha-1,-vlAlpha, nVerifyDepth, &line);
          if (vlVerify > vlAlpha) {
            vl = -SearchFull(-vlBeta, -vlAlpha, nVerifyDepth, &line);
          }
          t_nSingularExtCount --;
        }
        if (vl > vlAlpha && vl < vlBeta) {
          vl = -SearchFull(-vlBeta, -vlAlpha, nNewDepth, &line);
        }
      }
      pos.UndoMakeMove();

      if (vl > vlBest) {
        vlBest = vl;
        if (vl >= vlBeta) {
          nHashFlag = HASH_BETA;
          mvBest = mv;
          if (!bIsCapture) {
            int nHistoryDepth = nDepth;
            if (nReduction > 0) nHistoryDepth += nReduction;
            UpdateQuietHistoryBest(mvBest, nHistoryDepth);
            UpdateQuietHistoryMalus(quietsSearched, nQuietsSearched, mvBest, nDepth);
          } else {
            UpdateCaptureHistoryBest(mvBest, capturedPc, nDepth);
            UpdateCaptureHistoryMalus(capturesSearched, capturePieces, nCapturesSearched, mvBest, nDepth);
          }
          break;
        }
        if (vl > vlAlpha) {
          nHashFlag = HASH_PV;
          mvBest = mv;
          vlAlpha = vl;
          pline->argmove[0] = mv;
          memcpy(pline->argmove + 1, line.argmove, line.cmove * sizeof(int));
          pline->cmove = line.cmove + 1;
        }
      }
    }
  }

  if (vlBest == -MATE_VALUE) {
    return pos.nDistance - MATE_VALUE;
  }
  RecordHash(nHashFlag, vlBest, nDepth, mvBest);
  return vlBest;
}

int nGenMoves,mvs[MAX_GEN_MOVES];

// v15a修復：bInfinite在上半部已改為std::atomic<bool>，這裡統一改用
// .load()讀取，與同函式內的g_bPondering.load()寫法保持一致，
// 避免混用「atomic變數卻直接當成普通bool讀取」的不一致寫法。
inline bool CheckTimeUp(void) {
    if (bInfinite.load(std::memory_order_relaxed) || g_bPondering.load(std::memory_order_relaxed)) return false;
    return (GetTickCount() - t3 > (DWORD)t2);
}

static int SearchRootWindow(int nDepth, LINE *pline, int vlAspAlpha, int vlAspBeta) {
    LINE line;
    memset(&line, 0, sizeof(LINE));
    pline->cmove = 0;
    int vl, vlBest, mv, nNewDepth;
    SortStruct Sort;
    vlBest = -MATE_VALUE;
    Sort.Init(SearchMvResult);
    int mnumber=0;
    int quietsSearched[MAX_GEN_MOVES];
    int nQuietsSearched = 0;
    int capturesSearched[MAX_GEN_MOVES];
    int capturePieces[MAX_GEN_MOVES];
    int nCapturesSearched = 0;

    while ((mv = Sort.Next()) != 0) {
        if (g_stopSearch.load(std::memory_order_relaxed)) break;
        if (g_isMainThread && CheckTimeUp()) {
            g_stopSearch.store(true, std::memory_order_relaxed);
            break;
        }
        if (pos.MakeMove(mv)) {
            int capturedPc = pos.mvsList[pos.nMoveNum-1].ucpcCaptured;
            bool bIsCapture = (capturedPc != 0);
            bool ban=0;
            for(int j=0;j<Xqwl.banmoves;j++)
            {
                if(mv==Xqwl.banmove[j])
                {
                    ban=1;
                    break;
                }
            }
            if(ban||pos.RepStatus())
            {
                pos.UndoMakeMove();
                continue;
            }
            if (!bIsCapture && nQuietsSearched < MAX_GEN_MOVES) {
                quietsSearched[nQuietsSearched++] = mv;
            }
            if (bIsCapture && nCapturesSearched < MAX_GEN_MOVES) {
                capturesSearched[nCapturesSearched] = mv;
                capturePieces[nCapturesSearched] = pos.mvsList[pos.nMoveNum-1].ucpcCaptured;
                nCapturesSearched ++;
            }
            if(nDepth>=11 && g_isMainThread)
            {
                printf("info currmove %c%c%c%c currmovenumber %d\n",FILE_X(SRC(mv))-3+'a',RANK_Y(SRC(mv))-3+'0',FILE_X(DST(mv))-3+'a',RANK_Y(DST(mv))-3+'0',++mnumber);
                fflush(stdout);
            }
            nNewDepth=nDepth-1;

            if (vlBest == -MATE_VALUE) {
                vl = -SearchFull(-vlAspBeta, -vlAspAlpha, nNewDepth, &line, NO_NULL);
            } else {
                vl = -SearchFull(-vlBest - 1, -vlBest, nNewDepth, &line);
                if (vl > vlBest) {
                    vl = -SearchFull(-vlAspBeta, -vlBest, nNewDepth, &line, NO_NULL);
                }
            }
            pos.UndoMakeMove();

            if (vl > vlBest) {
                vlBest = vl;
                SearchMvResult = mv;
                pline->argmove[0] = SearchMvResult;
                memcpy(pline->argmove + 1, line.argmove, line.cmove * sizeof(int));
                pline->cmove = line.cmove + 1;
            }
            if (vl >= vlAspBeta) {
                if (!bIsCapture) {
                    UpdateQuietHistoryBest(mv, nDepth + 1);
                    UpdateQuietHistoryMalus(quietsSearched, nQuietsSearched, mv, nDepth);
                } else {
                    UpdateCaptureHistoryBest(mv, capturedPc, nDepth + 1);
                    UpdateCaptureHistoryMalus(capturesSearched, capturePieces, nCapturesSearched, mv, nDepth);
                }
            }
        }
    }

    if (vlBest != -MATE_VALUE && SearchMvResult != 0) {
      RecordHash(HASH_PV, vlBest, nDepth, SearchMvResult);
    }
    return vlBest;
}

// 修正3：偵測到 mate 分數(abs(vlBest) >= WIN_VALUE)導致的 fail-low/fail-high
// 時，aspiration window 直接整個打開到 [-MATE_VALUE, MATE_VALUE]，
// 不再用 200 分步長逐輪擴大，避免絕殺局面下反覆重搜整個根節點。
static int SearchRoot(int nDepth, LINE *pline, int vlLast) {
    int vlAspAlpha, vlAspBeta, vlBest;
    if (vlLast > -MATE_VALUE) {
        vlAspAlpha = vlLast - 50;
        vlAspBeta  = vlLast + 50;
    } else {
        vlAspAlpha = -MATE_VALUE;
        vlAspBeta  = MATE_VALUE;
    }

    for (;;) {
        if (g_stopSearch.load(std::memory_order_relaxed)) break;

        vlBest = SearchRootWindow(nDepth, pline, vlAspAlpha, vlAspBeta);
        if (g_stopSearch.load(std::memory_order_relaxed)) break;

        if (vlBest <= vlAspAlpha && vlAspAlpha > -MATE_VALUE) {
            if (abs(vlBest) >= WIN_VALUE) {
                vlAspAlpha = -MATE_VALUE;
            } else {
                vlAspAlpha = (vlAspAlpha <= -MATE_VALUE + 200) ? -MATE_VALUE : vlAspAlpha - 200;
            }
            continue;
        }
        if (vlBest >= vlAspBeta && vlAspBeta < MATE_VALUE) {
            if (abs(vlBest) >= WIN_VALUE) {
                vlAspBeta = MATE_VALUE;
            } else {
                vlAspBeta = (vlAspBeta >= MATE_VALUE - 200) ? MATE_VALUE : vlAspBeta + 200;
            }
            continue;
        }
        break;
    }
    UpdateCorrHist(SearchMvResult, vlBest, vlLast);
    return vlBest;
}

// =====================================================================
// 修正：貼近 Stockfish / Pikafish 標準 Lazy SMP 做法。
// 移除原本「執行緒依 PV 候選分組、各自獨立搜自己那條線」的 GroupSchedule
// 設計（該設計不是 Stockfish 的做法）。改為：
//   1. 所有執行緒（含 worker）共用同一個置換表（g_HashTable 本來就是
//      全域共享，未改動；v16a已改為動態分配並支援雙層替換策略），各自獨立跑「完整」的 MultiPV 迭代加深迴圈。
//   2. 每個執行緒的搜索起始深度做輕微錯開（Stockfish 的 "thread id
//      offset" 手法），增加搜索多樣性、加速置換表覆蓋範圍。
//   3. 只有主執行緒負責印 info 輸出，其餘執行緒安靜跑，純粹貢獻置換表
//      內容，這點與 Stockfish 一致。
//   4. 所有執行緒結束後，用「最佳執行緒選擇」機制（best thread voting）
//      從所有執行緒各自求得的結果中選出最終 bestmove，而不是永遠只採用
//      主執行緒的結果——這是 Stockfish 實際會做、但很多簡化版引擎會漏掉
//      的一步。
// =====================================================================

struct ThreadResultStruct {
  int mv;
  int vl;
  int nDepthReached;
  LINE line;
};
static ThreadResultStruct g_threadResults[64];

// 每個執行緒（含主執行緒）共用的核心搜索邏輯：完整 MultiPV 迭代加深，
// 執行緒結束或被 stop 時，把自己搜到的最新完整結果寫進 g_threadResults[threadId]。
static void RunIterativeDeepening(int threadId, int nMaxDepth) {
  int vlLastScore = -MATE_VALUE;

  int startDepth = 1 + (threadId % 2);

  int nLastCompleteMv = 0;
  int nLastCompleteVl = -MATE_VALUE;
  LINE lineLastComplete;
  lineLastComplete.cmove = 0;
  bool bHaveCompleteResult = false;

  for (int i = startDepth; i <= nMaxDepth; i++) {
    if (g_stopSearch.load(std::memory_order_relaxed)) break;

    if (g_isMainThread) {
      printf("info depth %d\n", i);
      fflush(stdout);
    }

    LINE line;
    line.cmove = 0;
    int vl = SearchRoot(i, &line, vlLastScore);

    if (g_stopSearch.load(std::memory_order_relaxed)) {
      break;
    }

    if (vl <= -MATE_VALUE || SearchMvResult == 0) {
      break;
    }

    vlLastScore = vl;
    bHaveCompleteResult = true;
    nLastCompleteMv = SearchMvResult;
    nLastCompleteVl = vl;
    lineLastComplete.argmove[0] = SearchMvResult;
    memcpy(lineLastComplete.argmove + 1, line.argmove, line.cmove * sizeof(int));
    lineLastComplete.cmove = line.cmove + 1;

    g_threadResults[threadId].mv = nLastCompleteMv;
    g_threadResults[threadId].vl = nLastCompleteVl;
    g_threadResults[threadId].nDepthReached = i;
    g_threadResults[threadId].line = lineLastComplete;

    if (g_isMainThread) {
      printf("info depth %d score %d pv", i, vl);
      for (int j = 0; j < lineLastComplete.cmove; j++) {
        int mvShow = lineLastComplete.argmove[j];
        printf(" %c%c%c%c",
               FILE_X(SRC(mvShow))-3+'a', RANK_Y(SRC(mvShow))-3+'0',
               FILE_X(DST(mvShow))-3+'a', RANK_Y(DST(mvShow))-3+'0');
      }
      printf("\n");
      fflush(stdout);

      t = GetTickCount();
      printf("info time %d nodes %lld threads %d\n", t-t3, (long long)g_totalNodes.load(std::memory_order_relaxed), nThreads);
      fflush(stdout);
    }

    if (g_isMainThread) {
      if (!bInfinite.load(std::memory_order_relaxed) && !g_bPondering.load(std::memory_order_relaxed) && t - t3 > t2) break;
    }
    if (g_stopSearch.load(std::memory_order_relaxed)) break;
  }

  if (!bHaveCompleteResult) {
    g_threadResults[threadId].mv = 0;
    g_threadResults[threadId].vl = -MATE_VALUE;
    g_threadResults[threadId].nDepthReached = 0;
    g_threadResults[threadId].line.cmove = 0;
  }
}

static void WorkerSearchThread(PositionStruct initialPos, int threadId) {
  g_isMainThread = false;
  pos = initialPos;
  memset(SearchMvKillers, 0, 100 * 2 * sizeof(int));
  memset(SearchNHistoryTable, 0, 65536 * sizeof(int));
  ClearCaptureHist();
  ClearContinuationHist();
  ClearCorrHist();
  t_nSingularExtCount = 0;
  SearchMvResult = 0;
  pos.nDistance = 0;
  node = 0;

  RunIterativeDeepening(threadId, MAX_ITER_DEPTH);
}

static void SearchMain_ClassicSchedule(void) {
  std::vector<std::thread> workers;
  PositionStruct rootPosCopy = pos;

  for (int idx = 0; idx < 64; idx++) {
    g_threadResults[idx].mv = 0;
    g_threadResults[idx].vl = -MATE_VALUE;
    g_threadResults[idx].nDepthReached = 0;
    g_threadResults[idx].line.cmove = 0;
  }

  for (int w = 1; w < nThreads; w++) {
    workers.emplace_back(WorkerSearchThread, rootPosCopy, w);
  }

  g_isMainThread = true;
  RunIterativeDeepening(0, min(setdepth, MAX_ITER_DEPTH));

  g_stopSearch.store(true, std::memory_order_relaxed);
  for (auto &wth : workers) {
    wth.join();
  }

  int nBestThread = 0;
  for (int idx = 1; idx < nThreads; idx++) {
    if (g_threadResults[idx].mv == 0) continue;
    if (g_threadResults[nBestThread].mv == 0) { nBestThread = idx; continue; }

    bool bCurMate = abs(g_threadResults[idx].vl) >= WIN_VALUE;
    bool bBestMate = abs(g_threadResults[nBestThread].vl) >= WIN_VALUE;

    if (bCurMate && bBestMate) {
      if (g_threadResults[idx].vl > g_threadResults[nBestThread].vl) nBestThread = idx;
    } else if (bCurMate && !bBestMate) {
      if (g_threadResults[idx].vl > 0) nBestThread = idx;
    } else if (!bCurMate && !bBestMate) {
      if (g_threadResults[idx].nDepthReached > g_threadResults[nBestThread].nDepthReached) {
        nBestThread = idx;
      } else if (g_threadResults[idx].nDepthReached == g_threadResults[nBestThread].nDepthReached &&
                 g_threadResults[idx].vl > g_threadResults[nBestThread].vl) {
        nBestThread = idx;
      }
    }
  }

  if (g_threadResults[nBestThread].mv != 0) {
    SearchMvResult = g_threadResults[nBestThread].mv;
  }
}

static void SearchMain(PositionStruct initialPos) {
    pos = initialPos;
    srand(time(0));
    memset(SearchMvKillers, 0, 100 * 2 * sizeof(int));
    memset(SearchNHistoryTable, 0, 65536 * sizeof(int));
    ClearHashTable(); // v16a: 改用动态置换表清空函式，取代固定大小的memset
    ClearCorrHist();
    t_nSingularExtCount = 0;
    SearchMvResult = 0;
    t3 = t = GetTickCount();
    pos.nDistance = 0;
    node = 0;
    g_stopSearch.store(false, std::memory_order_relaxed);
    g_totalNodes.store(0, std::memory_order_relaxed);

    SearchMain_ClassicSchedule();

    if (SearchMvResult != 0) {
        printf("bestmove %c%c%c%c\n",FILE_X(SRC(SearchMvResult))-3+'a',RANK_Y(SRC(SearchMvResult))-3+'0',FILE_X(DST(SearchMvResult))-3+'a',RANK_Y(DST(SearchMvResult))-3+'0');
    } else {
        printf("bestmove null\n");
    }
    fflush(stdout);
    g_bPondering.store(false, std::memory_order_relaxed);
}

static void SearchMainAsync(void) {
    if (g_searchThread.joinable()) {
        g_stopSearch.store(true, std::memory_order_relaxed);
        g_searchThread.join();
    }
    PositionStruct snapshot = pos;
    g_searchThread = std::thread(SearchMain, snapshot);
}

int main()
{
LoadConfig("engine.ini");
printf("info string threads config loaded: %d\n", nThreads);
fflush(stdout);
// v16a优化：依engine.ini的HashMB=设定(默认128MB)动态分配置换表，
// 取代原本硬编码1024项、几乎必然大量碰撞的置换表。
AllocateHashTable(nHashMB);
printf("info string hash table allocated: %dMB (%zu entries)\n", nHashMB, g_nHashEntries);
fflush(stdout);
BootLine();
cout<<"uaciok\n";
fflush(stdout);
InitZobrist();
Startup();
char input[1024];
while(1)
{
if(pipeStdHandle.LineInput(input))
{
if(strncmp(input,"fen ",4)==0)
{
fen_to_map(input+4);
}
else if(strncmp(input,"banmoves ",9)==0)
{
Xqwl.banmoves=0;
for(int i=8;input[i]!='\0';i+=5)
{
Xqwl.banmove[Xqwl.banmoves++]=MOVE(COORD_XY(input[i+1]-'a'+3,input[i+2]-'0'+3),COORD_XY(input[i+3]-'a'+3,input[i+4]-'0'+3));
}
}
// v16a新增：支援UCI風格的 "setoption name Hash value <N>" 指令，
// 讓外部GUI/腳本可以在runtime動態調整置換表大小(單位MB)，
// 而不需要每次改engine.ini再重啟。調整後會清空並重新分配置換表。
else if(strncmp(input,"setoption name Hash value ",27)==0)
{
int nNewMB = atoi(input+27);
if (nNewMB < MIN_HASH_MB) nNewMB = MIN_HASH_MB;
if (nNewMB > MAX_HASH_MB) nNewMB = MAX_HASH_MB;
nHashMB = nNewMB;
AllocateHashTable(nHashMB);
printf("info string hash table resized: %dMB (%zu entries)\n", nHashMB, g_nHashEntries);
fflush(stdout);
}
else if(strncmp(input,"go ",3)==0)
{
t2=setdepth=99999999;
// v15a修復：bInfinite已改為atomic，統一用.store()寫入
bInfinite.store(false, std::memory_order_relaxed);
bool bPonder = false;

if(strstr(input+3,"ponder")!=NULL)
{
    bPonder = true;
}

if(strstr(input+3,"time ")!=NULL)
{
    char *p = strstr(input+3,"time ");
    t2=ReadDigit(p+5,100000);
}
else if(strstr(input+3,"depth ")!=NULL)
{
    char *p = strstr(input+3,"depth ");
    setdepth=ReadDigit(p+6,1000);
}
else if(strstr(input+3,"infinite")!=NULL)
{
    bInfinite.store(true, std::memory_order_relaxed);
}

g_bPondering.store(bPonder, std::memory_order_relaxed);

SearchMainAsync();
}
else if(strcmp(input,"ponderhit")==0)
{
    t3 = GetTickCount();
    g_bPondering.store(false, std::memory_order_relaxed);
}
else if(strcmp(input,"stop")==0)
{
g_stopSearch.store(true, std::memory_order_relaxed);
if (g_searchThread.joinable()) {
    g_searchThread.join();
}
}
else if(strcmp(input,"quit")==0)
{
g_stopSearch.store(true, std::memory_order_relaxed);
if (g_searchThread.joinable()) {
    g_searchThread.join();
}
printf("bye\n");
fflush(stdout);
return 0;
}
else if(strcmp(input,"uaci")==0||strcmp(input,"uci")==0||strcmp(input,"ucci")==0)
{
printf("id name AnimalcraftAI\n");
fflush(stdout);
printf("v16b-fix11-incheckext-corrquiet-corrsrc-qseeprune-lmrfinetune-nodoublebest-seecache-declfix-splitcapsquiets-setbestmove-capturecache-see-history-lmr-caphist-compactconthist\n");
fflush(stdout);
printf("%s\n", input);
printf("ok\n");
fflush(stdout);
}
}
else Sleep(1);
}
return 0;
}
