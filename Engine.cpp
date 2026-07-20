#include<bits/stdc++.h>
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
const int HASH_SIZE = 1 << 20; // 置换表大小（优化：原1<<25过大，改为1<<20约100万项，节省内存）
const int HASH_ALPHA = 1;      // ALPHA节点的置换表项
const int HASH_BETA = 2;       // BETA节点的置换表项
const int HASH_PV = 3;         // PV节点的置换表项
const int NULL_MARGIN = 600;   // 空步裁剪的子力边界
const int NULL_DEPTH = 2;      // 空步裁剪的裁剪深度

int t2=1000,setdepth=99999999,t,t3;//时间控制和深度控制

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


// 翻转格子
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

  void InitZero(void);   // 用空密钥初始化密码流生成器
  int Nextint(void) {  // 生成密码流的下一个字节
    int uc;
    x = (x + 1) & 255;
    y = (y + s[x]) & 255;
    uc = s[x];
    s[x] = s[y];
    s[y] = uc;
    return s[(s[x] + s[y]) & 255];
  }
  int NextLong(void) { // 生成密码流的下四个字节
    int uc0, uc1, uc2, uc3;
    uc0 = Nextint();
    uc1 = Nextint();
    uc2 = Nextint();
    uc3 = Nextint();
    return uc0 + (uc1 << 8) + (uc2 << 16) + (uc3 << 24);
  }
};

// 用空密钥初始化密码流生成器
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

  void InitZero(void) {                 // 用零填充Zobrist
    dwKey = dwLock0 = dwLock1 = 0;
  }
  void InitRC4(RC4Struct &rc4) {        // 用密码流填充Zobrist
    dwKey = rc4.NextLong();
    dwLock0 = rc4.NextLong();
    dwLock1 = rc4.NextLong();
  }
  void Xor(const ZobristStruct &zobr) { // 执行XOR操作
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
// 局面结构
struct PositionStruct {
  bool sdPlayer;                   // 轮到谁走，0=红方，1=黑方
  int ucpcSquares[256];            // 棋盘上的棋子
  int vlWhite, vlBlack;            // 红、黑双方的子力价值
  int nDistance, nMoveNum, nMoveNum2;         // 距离根节点的步数，历史走法数
  MoveStruct mvsList[MAX_MOVES];   // 历史走法信息列表
  ZobristStruct zobr;              // Zobrist
  bool CanJump(int src,int dst)
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
  	  	  	  		        // 修正：不论敌我，鼠在河中即阻隔跳跃路线
  	  	  	  		        if(PIECE_NAME(ucpcSquares[j])==PIECE_MOUSE||!INRIVER(j)) return 0;
						}
						return 1;
				}
		  }
		  return 0;
	  }
  	  return 0;
  }
  bool CanMove(int src,int dst)
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
  bool CanEat(int src,int dst)
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
  	  if(as==PIECE_ELEPHANT&&bs==PIECE_MOUSE) return 0;
  	  return as<=bs;
  }
  void ClearBoard(void) {         // 清空棋盘
    sdPlayer = false;
	vlWhite = vlBlack = nDistance = 0;
    memset(ucpcSquares, 0, sizeof(ucpcSquares));
    zobr.InitZero();
  }
  void SetIrrev(void) {           // 清空(初始化)历史走法信息
    mvsList[0].Set(0, 0,0, zobr.dwKey);
    nMoveNum = 1;
  }
  void SetIrrev2(void) {           // 清空(初始化)历史走法信息
    nMoveNum2 = 1;
  }
  void Startup(void);             // 初始化棋盘
  void ChangeSide(void) {         // 交换走子方
    sdPlayer = 1 - sdPlayer;
    zobr.Xor(Zobrist.Player);
  }
  void AddPiece(int sq, int pc) { // 在棋盘上放一枚棋子
    ucpcSquares[sq] = pc;
    // 红方加分，黑方(注意"cucvlPiecePos"取值要颠倒)减分
    if (pc < 16) {
      vlWhite += cucvlPiecePos[pc - 8][sq];
      zobr.Xor(Zobrist.Table[pc - 8][sq]);
    } else {
      vlBlack += cucvlPiecePos[pc - 16][BLACK(sq)];
      zobr.Xor(Zobrist.Table[pc - 8][sq]);
    }
  }
  void DelPiece(int sq, int pc) { // 从棋盘上拿走一枚棋子
    ucpcSquares[sq] = 0;
    // 红方减分，黑方(注意"cucvlPiecePos"取值要颠倒)加分
    if (pc < 16) {
      vlWhite -= cucvlPiecePos[pc - 8][sq];
      zobr.Xor(Zobrist.Table[pc - 8][sq]);
    } else {
      vlBlack -= cucvlPiecePos[pc - 16][BLACK(sq)];
      zobr.Xor(Zobrist.Table[pc - 8][sq]);
    }
  }
  int Evaluate(void) const {      // 局面评价函数
    return (!sdPlayer ? vlWhite - vlBlack : vlBlack - vlWhite);
  }
  bool Captured(void) const {     // 上一步是否吃子
    return mvsList[nMoveNum - 1].ucpcCaptured != 0;
  }
  int MovePiece(int mv);                      // 搬一步棋的棋子
  void UndoMovePiece(int mv, int pcCaptured); // 撤消搬一步棋的棋子
  bool MakeMove(int mv);                      // 走一步棋
  void UndoMakeMove(void) {                   // 撤消走一步棋
    nDistance --;
    nMoveNum --;
    nMoveNum2 --;
    ChangeSide();
    UndoMovePiece(mvsList[nMoveNum].wmv, mvsList[nMoveNum].ucpcCaptured);
  }
  void NullMove(void) {                       // 走一步空步 
    int dwKey;
    dwKey = zobr.dwKey;
    ChangeSide();
    mvsList[nMoveNum].Set(0, 0,0, dwKey);
    nMoveNum ++;
    nMoveNum2 ++;
    nDistance ++;
  }
  void UndoNullMove(void) {                   // 撤消走一步空步
    nDistance --;
    nMoveNum --;
    nMoveNum2 --;
    ChangeSide();
  }
  // 生成所有走法，如果"bCapture"为"1"则只生成吃子走法
  int GenerateMoves(int *mvs, bool bCapture = 0);
  bool LegalMove(int mv);               // 判断走法是否合理
  bool IsMate(void);
  bool RepWuLai(void);
  bool RepWuSong(void);
  bool RepStatus(void);
  bool RepStatus2(void);
  void Mirror(PositionStruct &posMirror) const; // 对局面镜像
  bool NullOkay(void) const {                 // 判断是否允许空步裁剪
    return (sdPlayer == 0 ? vlWhite : vlBlack) > NULL_MARGIN;
  }
};

// 初始化棋盘
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

// 搬一步棋的棋子
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

// 撤消搬一步棋的棋子
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

// 走一步棋
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
// "GenerateMoves"参数
const bool GEN_CAPTURE = 1;

// 生成所有走法，如果"bCapture"为"1"则只生成吃子走法
int PositionStruct::GenerateMoves(int *mvs, bool bCapture){
  int i, j, nGenMoves, nDelta, sqSrc, sqDst;
  int pcSelfSide, pcOppSide, pcSrc, pcDst;
  // 生成所有走法，需要经过以下几个步骤：

  nGenMoves = 0;
  pcSelfSide = SIDE_TAG(sdPlayer);
  pcOppSide = OPP_SIDE_TAG(sdPlayer);
  for (sqSrc = 0; sqSrc < 256; sqSrc ++) {
  	if(!IN_BOARD(sqSrc)) continue;
    // 1. 找到一个本方棋子，再做以下判断：
    pcSrc = ucpcSquares[sqSrc];
    if ((pcSrc & pcSelfSide) == 0) {
      continue;
    }
    // 2. 根据棋子确定走法
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

// 判断走法是否合理
bool PositionStruct::LegalMove(int mv){
  int sqSrc, sqDst, sqPin;
  int pcSelfSide,pcOppSide, pcSrc, pcDst, nDelta;
  // 判断走法是否合法，需要经过以下的判断过程：

  // 1. 判断起始格是否有自己的棋子
  sqSrc = SRC(mv);
  pcSrc = ucpcSquares[sqSrc];
  pcSelfSide = SIDE_TAG(sdPlayer);
  pcOppSide = OPP_SIDE_TAG(sdPlayer);
  if ((pcSrc & pcSelfSide) == 0) {
    return 0;
  }

  // 2. 判断目标格是否有自己的棋子
  sqDst = DST(mv);
  pcDst = ucpcSquares[sqDst];
  if ((pcDst & pcSelfSide) != 0) {
    return 0;
  }
  
  // 2. 根据棋子确定走法
    for(int delta=0;delta<=3;delta++)
    {
    	int sqDst2=sqSrc+ccDelta[delta];
    	if(!IN_BOARD(sqDst)||INDEN(sqDst,pcSelfSide)||sqDst2!=sqDst) continue;
    	pcDst = ucpcSquares[sqDst];
    	if(!CanMove(sqSrc,sqDst))continue;
        if (ucpcSquares[sqDst] ? ((pcDst & pcOppSide) != 0  && CanEat(sqSrc,sqDst) ): (pcDst & pcSelfSide) == 0) {
        	return 1;
        }
	}
    for(int delta=0;delta<=3;delta++)
    {
    	int sqDst2=sqSrc+ccJumpDelta[delta];
    	if(!IN_BOARD(sqDst)||INDEN(sqDst,pcSelfSide)||sqDst2!=sqDst) continue;
    	pcDst = ucpcSquares[sqDst];
    	if(!CanJump(sqSrc,sqDst))continue;
        if (ucpcSquares[sqDst] ? ((pcDst & pcOppSide) != 0  && CanEat(sqSrc,sqDst) ): (pcDst & pcSelfSide) == 0) {
        	return 1;
        }
	}
	return 0;
}

// 判断是否被杀
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
	if(RepWuLai())
	{
		return 1;
	}
	if(RepWuSong())
	{
		return 1;
	}
	return 0;
}


// 检测重复局面
bool PositionStruct::RepStatus2(void){
  bool bSelfSide;
  const MoveStruct *lpmvs;
  int nRecur=1;
  bSelfSide = 0;

  lpmvs = mvsList + nMoveNum - 1;
  while (lpmvs->wmv != 0 && lpmvs->ucpcCaptured == 0) {
    if (bSelfSide) {
      if (lpmvs->dwKey == zobr.dwKey) {
        nRecur --;
        if (nRecur == 0) {
           return 1;
        }
      }
    }
    bSelfSide = !bSelfSide;
    lpmvs --;
  }
  return 0;
}
static PositionStruct pos; // 局面实例
/*
struct BookStruct {
  int dwZobristLock;
  int wmv, wvl;
};
inline int BOOK_POS_CMP(const BookStruct &bk, const PositionStruct &pos) {
  return bk.dwZobristLock < pos.zobr.dwLock1 ? -1 :
      bk.dwZobristLock > pos.zobr.dwLock1 ? 1 : 0;
}
struct BookFileStruct {
  FILE *fp;
  int nLen;
  bool Open(const char *szFileName, bool bEdit = false) {
    fp = fopen(szFileName, bEdit ? "r+b" : "rb");
    if (fp == NULL) {
      return false;
    } else {
      fseek(fp, 0, SEEK_END);
      nLen = ftell(fp) / sizeof(BookStruct);
      return true;
    }
  }
  void Close(void) const {
    fclose(fp);
  }
  void Read(BookStruct &bk, int nPtr) const {
    fseek(fp, nPtr * sizeof(BookStruct), SEEK_SET);
    fread(&bk, sizeof(BookStruct), 1, fp);
  }
  void Write(const BookStruct &bk, int nPtr) const {
    fseek(fp, nPtr * sizeof(BookStruct), SEEK_SET);
    fwrite(&bk, sizeof(BookStruct), 1, fp);
  }
};
int GetBookMoves(const char *szBookFile, BookStruct *lpbks) {
  BookFileStruct BookFile;
  BookStruct bk;
  int nScan, nLow, nHigh, nPtr;
  int i, j, nMoves;
  // 从开局库中搜索着法的例程，有以下几个步骤：

  // 1. 打开开局库，如果打开失败，则返回空值；
  if (!BookFile.Open(szBookFile)) {
    return 0;
  }

    nPtr = nLow = 0;
    nHigh = BookFile.nLen - 1;
    while (nLow <= nHigh) {
      nPtr = (nLow + nHigh) / 2;
      BookFile.Read(bk, nPtr);
      if (BOOK_POS_CMP(bk, pos) < 0) {
        nLow = nPtr + 1;          
      } else if (BOOK_POS_CMP(bk, pos) > 0) {
        nHigh = nPtr - 1;
      } else {
        break;
      }
    }

  // 4. 如果找到局面，则向前查找第一个着法；
  for (nPtr --; nPtr >= 0; nPtr --) {
    BookFile.Read(bk, nPtr);
    if (BOOK_POS_CMP(bk, pos) < 0) {
      break;
    }
  }

  // 5. 向后依次读入属于该局面的每个着法；
  nMoves = 0;
  for (nPtr ++; nPtr < BookFile.nLen; nPtr ++) {
    BookFile.Read(bk, nPtr);
    if (BOOK_POS_CMP(bk, pos) > 0) {
      break;
    }
    if (pos.LegalMove(bk.wmv)) {
      // 如果局面是第二趟搜索到的，则着法必须做镜像
      lpbks[nMoves].wmv = bk.wmv;
      lpbks[nMoves].wvl = bk.wvl;
      nMoves ++;
      if (nMoves == MAX_GEN_MOVES) {
        break;
      }
    }
  }
  BookFile.Close();

  // 6. 对着法按分值排序
  for (i = 0; i < nMoves - 1; i ++) {
    for (j = nMoves - 1; j > i; j --) {
      if (lpbks[j - 1].wvl < lpbks[j].wvl) {
        swap(lpbks[j - 1], lpbks[j]);
      }
    }
  }
  return nMoves;
}
*/

// 与图形界面有关的全局变量
static struct {
  int mvLast;                       // 选中的格子，上一步棋
  bool bGameOver;                // 是否翻转棋盘，是否游戏结束(不让继续玩下去)
  int sqSelected;
  int banmove[MAX_GEN_MOVES],banmoves;
} Xqwl;

// 初始化棋局
static void Startup(void) {
  pos.Startup();
  Xqwl.sqSelected = Xqwl.mvLast = 0;
  Xqwl.bGameOver = 0;
  memset(Xqwl.banmove,0,sizeof(Xqwl.banmove));
  Xqwl.banmoves=0;
}


/*
以下为核心计算功能!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

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
// 置换表项结构
struct HashItem {
  short svl;                  // 优化：缩小字段类型，减少结构体大小
  unsigned char ucDepth, ucFlag;
  int wmv;
  int dwLock0, dwLock1;
};
// 与搜索有关的全局变量
static struct {
  int mvResult;                  // 电脑走的棋
  char fen[50];
  int nHistoryTable[65536];      // 历史表
  int mvKillers[100][2]; // 杀手走法表
  HashItem HashTable[HASH_SIZE]; // 置换表
  //char szBookFile[1024];         // 开局库
} Search;
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
//读入局面fen代码
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

// 提取置换表项
static int ProbeHash(int vlAlpha, int vlBeta, int nDepth, int &mv) {
  bool bMate; // 杀棋标志：如果是杀棋，那么不需要满足深度条件
  HashItem hsh;

  hsh = Search.HashTable[pos.zobr.dwKey & (HASH_SIZE - 1)];
  if (hsh.dwLock0 != pos.zobr.dwLock0 || hsh.dwLock1 != pos.zobr.dwLock1) {
    mv = 0;
    return -MATE_VALUE;
  }
  mv = hsh.wmv;
  bMate = 0;
  if (hsh.svl > WIN_VALUE) {
    hsh.svl -= pos.nDistance;
    bMate = 1;
  } else if (hsh.svl < -WIN_VALUE) {
    hsh.svl += pos.nDistance;
    bMate = 1;
  }
  if (hsh.ucDepth >= nDepth || bMate) {
    if (hsh.ucFlag == HASH_BETA) {
      return (hsh.svl >= vlBeta ? hsh.svl : -MATE_VALUE);
    } else if (hsh.ucFlag == HASH_ALPHA) {
      return (hsh.svl <= vlAlpha ? hsh.svl : -MATE_VALUE);
    }
    return hsh.svl;
  }
  return -MATE_VALUE;
};

// 保存置换表项
static void RecordHash(int nFlag, int vl, int nDepth, int mv) {
  HashItem hsh;
  hsh = Search.HashTable[pos.zobr.dwKey & (HASH_SIZE - 1)];
  if (hsh.ucDepth > nDepth) {
    return;
  }
  hsh.ucFlag = nFlag;
  hsh.ucDepth = nDepth;
  if (vl > WIN_VALUE) {
    hsh.svl = vl + pos.nDistance;
  } else if (vl < -WIN_VALUE) {
    hsh.svl = vl - pos.nDistance;
  } else {
    hsh.svl = vl;
  }
  hsh.wmv = mv;
  hsh.dwLock0 = pos.zobr.dwLock0;
  hsh.dwLock1 = pos.zobr.dwLock1;
  Search.HashTable[pos.zobr.dwKey & (HASH_SIZE - 1)] = hsh;
};

// MVV/LVA每种子力的价值
static int cucMvvLva[24] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  7, 8, 6, 5, 3, 2, 1, 4,
  7, 8, 6, 5, 3, 2, 1, 4
};

// 求MVV/LVA值
inline int MvvLva(int mv) {
  return (cucMvvLva[pos.ucpcSquares[DST(mv)]] << 3) - cucMvvLva[pos.ucpcSquares[SRC(mv)]];
}
// "qsort"按MVV/LVA值排序的比较函数
static int CompareMvvLva(const void *lpmv1, const void *lpmv2) {
  return MvvLva(*(int *) lpmv2) - MvvLva(*(int *) lpmv1);
}

// "qsort"按历史表排序的比较函数
static int CompareHistory(const void *lpmv1, const void *lpmv2) {
  return Search.nHistoryTable[*(int *) lpmv2] - Search.nHistoryTable[*(int *) lpmv1];
}
// 走法排序阶段
const int PHASE_HASH = 0;
const int PHASE_KILLER_1 = 1;
const int PHASE_KILLER_2 = 2;
const int PHASE_GEN_MOVES = 3;
const int PHASE_REST = 4;

// 走法排序结构
struct SortStruct {
  int mvHash, mvKiller1, mvKiller2; // 置换表走法和两个杀手走法
  int nPhase, nIndex, nGenMoves;    // 当前阶段，当前采用第几个走法，总共有几个走法
  int mvs[MAX_GEN_MOVES];           // 所有的走法

  void Init(int mvHash_) { // 初始化，设定置换表走法和两个杀手走法
    mvHash = mvHash_;
    mvKiller1 = Search.mvKillers[pos.nDistance][0];
    mvKiller2 = Search.mvKillers[pos.nDistance][1];
    nPhase = PHASE_HASH;
  }
  int Next(void); // 得到下一个走法
};

// 得到下一个走法
inline int SortStruct::Next(void) {
  int mv;
  switch (nPhase) {
  // "nPhase"表示着法启发的若干阶段，依次为：

  // 0. 置换表着法启发，完成后立即进入下一阶段；
  case PHASE_HASH:
    nPhase = PHASE_KILLER_1;
    if (mvHash != 0) {
      return mvHash;
    }
    // 技巧：这里没有"break"，表示"switch"的上一个"case"执行完后紧接着做下一个"case"，下同

  // 1. 杀手着法启发(第一个杀手着法)，完成后立即进入下一阶段；
  case PHASE_KILLER_1:
    nPhase = PHASE_KILLER_2;
    if (mvKiller1 != mvHash && mvKiller1 != 0 && pos.LegalMove(mvKiller1)) {
      return mvKiller1;
    }

  // 2. 杀手着法启发(第二个杀手着法)，完成后立即进入下一阶段；
  case PHASE_KILLER_2:
    nPhase = PHASE_GEN_MOVES;
    if (mvKiller2 != mvHash && mvKiller2 != 0 && pos.LegalMove(mvKiller2)) {
      return mvKiller2;
    }

  // 3. 生成所有着法，完成后立即进入下一阶段；
  case PHASE_GEN_MOVES:
    nPhase = PHASE_REST;
    nGenMoves = pos.GenerateMoves(mvs);
    qsort(mvs, nGenMoves, sizeof(int), CompareHistory);
    nIndex = 0;

  // 4. 对剩余着法做历史表启发；
  case PHASE_REST:
    while (nIndex < nGenMoves) {
      mv = mvs[nIndex];
      nIndex ++;
      if (mv != mvHash && mv != mvKiller1 && mv != mvKiller2 && pos.LegalMove(mv)) {
        return mv;
      }
    }
  // 5. 没有着法了，返回零。
  default:
    return 0;
  }
}
// 对最佳走法的处理
inline void SetBestMove(int mv, int nDepth) {
  int *lpmvKillers;
  Search.nHistoryTable[mv] += nDepth * nDepth;
  lpmvKillers = Search.mvKillers[pos.nDistance];
  if (lpmvKillers[0] != mv) {
    lpmvKillers[1] = lpmvKillers[0];
    lpmvKillers[0] = mv;
  }
}

// 静态(Quiescence)搜索过程
static int SearchQuiesc(int vlAlpha, int vlBeta) {
  int i, nGenMoves;
  int vl, vlBest;
  int mvs[MAX_GEN_MOVES];
  // 一个静态搜索分为以下几个阶段

  // 0. 优化：补上IsMate判断，避免吃入兽穴后仍继续静态评估导致分数不准
  if (pos.IsMate()) {
    return pos.nDistance - MATE_VALUE;
  }

  // 3. 初始化最佳值
  vlBest = -MATE_VALUE; // 这样可以知道，是否一个走法都没走过(杀棋)

   {
    // 5. 如果不被将军，先做局面评价
    vl = pos.Evaluate();
    if (vl > vlBest) {
      vlBest = vl;
      if (vl >= vlBeta) {
        return vl;
      }
      if (vl > vlAlpha) {
        vlAlpha = vl;
      }
    }

    // 6. 如果局面评价没有截断，再生成吃子走法
    nGenMoves = pos.GenerateMoves(mvs, GEN_CAPTURE);
    qsort(mvs, nGenMoves, sizeof(int), CompareMvvLva);
  }

  // 7. 逐一走这些走法，并进行递归
  for (i = 0; i < nGenMoves; i ++) {
    if (pos.MakeMove(mvs[i])) {
      vl = -SearchQuiesc(-vlBeta, -vlAlpha);
      pos.UndoMakeMove();

      // 8. 进行Alpha-Beta大小判断和截断
      if (vl > vlBest) {    // 找到最佳值(但不能确定是Alpha、PV还是Beta走法)
        vlBest = vl;        // "vlBest"就是目前要返回的最佳值，可能超出Alpha-Beta边界
        if (vl >= vlBeta) { // 找到一个Beta走法
          return vl;        // Beta截断
        }
        if (vl > vlAlpha) { // 找到一个PV走法
          vlAlpha = vl;     // 缩小Alpha-Beta边界
        }
      }
    }
  }

  // 9. 所有走法都搜索完了，返回最佳值
  return vlBest == -MATE_VALUE ? pos.nDistance - MATE_VALUE : vlBest;
}
typedef struct tagLINE{
int cmove;
int argmove[100];
} LINE;
long long node=0;
const BOOL NO_NULL = true;
// 超出边界(Fail-Soft)的Alpha-Beta搜索过程
static int SearchFull(int vlAlpha, int vlBeta, int nDepth, LINE *pline, BOOL bNoNull = 0) {
	LINE line;
	node++;
  int nHashFlag, vl, vlBest;
  int mv, mvBest, mvHash, nNewDepth;
  SortStruct Sort;
  // 一个Alpha-Beta完全搜索分为以下几个阶段
	if(pos.IsMate())
	{  
  		pline->cmove = 0;
		return -MATE_VALUE;
	}
	if(pos.RepStatus())
	{
  		pline->cmove = 0;
		return MATE_VALUE;
	}/*
	if(pos.RepStatus2())
	{
		pline->cmove = 0;
		return 0;
	}*/
	
	if(nDepth<=0)
	{
		pline->cmove=0;
		return SearchQuiesc(vlAlpha,vlBeta);
	}
	
  // 1-3. 尝试置换表裁剪，并得到置换表走法
  vl = ProbeHash(vlAlpha, vlBeta, nDepth, mvHash);
  if (vl > -MATE_VALUE) {
  	pline->cmove = 0;
    return vl;
  }
  
  // 1-4. 尝试空步裁剪(根节点的Beta值是"MATE_VALUE"，所以不可能发生空步裁剪)
  if (!bNoNull && pos.NullOkay()) {
    pos.NullMove();
    vl = -SearchFull(-vlBeta, 1 - vlBeta, nDepth - NULL_DEPTH - 1,&line, NO_NULL);
    pos.UndoNullMove();
    if (vl >= vlBeta) {
    	pline->cmove=0;
      return vl;
    }
  }	
  
  // 2. 初始化最佳值和最佳走法
  nHashFlag = HASH_ALPHA;
  vlBest = -MATE_VALUE; // 这样可以知道，是否一个走法都没走过(杀棋)
  mvBest = 0;           // 这样可以知道，是否搜索到了Beta走法或PV走法，以便保存到历史表

  // 3. 初始化走法排序结构
  Sort.Init(mvHash);
  // 4. 逐一走这些走法，并进行递归
  while ((mv = Sort.Next()) != 0) {
  	
    if (pos.MakeMove(mv)) {
      // 将军延伸
      nNewDepth = nDepth - 1;
      // PVS
      if (vlBest == -MATE_VALUE) {
        vl = -SearchFull(-vlBeta, -vlAlpha, nNewDepth, &line);
      } else {
        vl = -SearchFull(-vlAlpha-1,-vlAlpha, nNewDepth, &line);
        if (vl > vlAlpha && vl < vlBeta) {
          vl = -SearchFull(-vlBeta, -vlAlpha, nNewDepth, &line);
        }
      }
      pos.UndoMakeMove();
      // 5. 进行Alpha-Beta大小判断和截断
      if (vl > vlBest) {    // 找到最佳值(但不能确定是Alpha、PV还是Beta走法)
        vlBest = vl;        // "vlBest"就是目前要返回的最佳值，可能超出Alpha-Beta边界
        if (vl >= vlBeta) { // 找到一个Beta走法
          nHashFlag = HASH_BETA;
          mvBest = mv;      // Beta走法要保存到历史表
          break;            // Beta截断
        }
        if (vl > vlAlpha) { // 找到一个PV走法
          nHashFlag = HASH_PV;
          mvBest = mv;      // PV走法要保存到历史表
          vlAlpha = vl;     // 缩小Alpha-Beta边界
		  pline->argmove[0] = mv;
		  memcpy(pline->argmove + 1, line.argmove, line.cmove * sizeof(int));
		  pline->cmove = line.cmove + 1;
        }
      }
    }
  }

  // 5. 所有走法都搜索完了，把最佳走法(不能是Alpha走法)保存到历史表，返回最佳值
  if (vlBest == -MATE_VALUE) {
    // 如果是杀棋，就根据杀棋步数给出评价
    return pos.nDistance - MATE_VALUE;
  }
  // 记录到置换表
  RecordHash(nHashFlag, vlBest, nDepth, mvBest);
  if (mvBest != 0) {
    // 如果不是Alpha走法，就将最佳走法保存到历史表
    SetBestMove(mvBest, nDepth);
  }
  return vlBest;
}

int nGenMoves,mvs[MAX_GEN_MOVES];
// 根节点的Alpha-Beta搜索过程
static int SearchRoot(int nDepth,LINE *pline) {
  LINE line;
  int vl, vlBest, mv, nNewDepth;
  SortStruct Sort;
  vlBest = -MATE_VALUE;
  Sort.Init(Search.mvResult);
  int mnumber=0;
  while ((mv = Sort.Next()) != 0) {
    if (pos.MakeMove(mv)) {
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
		if(nDepth>=11)
		{
		    printf("info currmove %c%c%c%c currmovenumber %d\n",FILE_X(SRC(mv))-3+'a',RANK_Y(SRC(mv))-3+'0',FILE_X(DST(mv))-3+'a',RANK_Y(DST(mv))-3+'0',++mnumber);
		  	fflush(stdout);
		}
		nNewDepth=nDepth-1; 
      // PVS
      if (vlBest == -MATE_VALUE) {
        vl = -SearchFull(-MATE_VALUE, MATE_VALUE, nNewDepth,&line, NO_NULL);
      } else {
        vl = -SearchFull(-vlBest - 1, -vlBest, nNewDepth,&line);
        if (vl > vlBest) {
          vl = -SearchFull(-MATE_VALUE, -vlBest, nNewDepth,&line, NO_NULL);
        }
      }
      pos.UndoMakeMove();
      if (vl > vlBest) {
        vlBest = vl;
	    Search.mvResult = mv;
		pline->argmove[0] = Search.mvResult;
		memcpy(pline->argmove + 1, line.argmove, line.cmove * sizeof(int));
		pline->cmove = line.cmove + 1;
      }
    }
  }
  RecordHash(HASH_PV, vlBest, nDepth, Search.mvResult);
  SetBestMove(Search.mvResult, nDepth);
  return vlBest;
}
// 迭代加深搜索过程
static void SearchMain(void) {
  int i;
	/*
	BookStruct bks[MAX_GEN_MOVES];
	int nBookMoves = GetBookMoves(Search.szBookFile, bks);
	for(i=0;i<nBookMoves;i++)
	{
      int vl = bks[i].wvl;
      int mv = bks[i].wmv;
      if(!pos.LegalMove(mv)) continue;
      pos.MakeMove(mv);
      if(pos.RepStatus())
      {
      	  pos.UndoMakeMove();
      	  continue;
	  }
	  pos.UndoMakeMove();
	  Search.mvResult=mv;
	  printf("bestmove %c%c%c%c\n",FILE_X(SRC(Search.mvResult))-3+'a',RANK_Y(SRC(Search.mvResult))-3+'0',FILE_X(DST(Search.mvResult))-3+'a',RANK_Y(DST(Search.mvResult))-3+'0');
	  fflush(stdout);
	  return;
	}
	*/
	
	srand(time(0));
  // 初始化
  memset(Search.mvKillers, 0, 100 * 2 * sizeof(int)); // 清空杀手走法表
  memset(Search.nHistoryTable, 0, 65536 * sizeof(int));       // 清空历史表
  memset(Search.HashTable, 0, HASH_SIZE * sizeof(HashItem));  // 清空置换表
  Search.mvResult=0;
  
  t3 = t = clock();       // 初始化定时器
  pos.nDistance = node = 0; // 初始步数
  
  int vl;
  // 迭代加深过程
  for (i = 1; i <= min(setdepth,100); i ++) {
	LINE line;
	printf("info depth %d\n",i);
  	fflush(stdout);
	vl = SearchRoot(i,&line);
    printf("info depth %d score %d pv",i,vl);
  	for(int j=0;j<line.cmove;j++)
  	{
  		printf(" %c%c%c%c",FILE_X(SRC(line.argmove[j]))-3+'a',RANK_Y(SRC(line.argmove[j]))-3+'0',FILE_X(DST(line.argmove[j]))-3+'a',RANK_Y(DST(line.argmove[j]))-3+'0');
	}
	printf("\n");
	fflush(stdout);
	t=clock();
	printf("info time %d nodes %d\n",t-t3,node);
  	fflush(stdout); 
    // 超过n秒，就终止搜索
    if (t - t3 > t2) {
      break;
    }
    char input[1024];
    if(pipeStdHandle.LineInput(input))
    {
    	if(strcmp(input,"stop")==0) break;
	}
  }
  printf("bestmove %c%c%c%c\n",FILE_X(SRC(Search.mvResult))-3+'a',RANK_Y(SRC(Search.mvResult))-3+'0',FILE_X(DST(Search.mvResult))-3+'a',RANK_Y(DST(Search.mvResult))-3+'0');
  fflush(stdout);
}
/*
const int PATH_MAX_CHAR = 1024;
const int PATH_SEPARATOR = '\\';
inline bool AbsolutePath(const char *sz) {
  return sz[0] == '\\' || (((sz[0] >= 'A' && sz[0] <= 'Z') || (sz[0] >= 'a' && sz[0] <= 'z')) && sz[1] == ':');
}
inline void GetSelfExe(char *szDst) {
  GetModuleFileName(NULL, szDst, PATH_MAX_CHAR);
}
inline void LocatePath(char *szDst, const char *szSrc) {
  char *lpSeparator;
  if (AbsolutePath(szSrc)) {
    strcpy(szDst, szSrc);
  } else {
    GetSelfExe(szDst);
    lpSeparator = strrchr(szDst, PATH_SEPARATOR);
    if (lpSeparator == NULL) {
      strcpy(szDst, szSrc);
    } else {
      strcpy(lpSeparator + 1, szSrc);
    }
  }
}*/
int main()
{
	//LocatePath(Search.szBookFile,"BOOK.DAT");
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
			else if(strncmp(input,"go ",3)==0)
			{
				t2=setdepth=99999999;
				if(strncmp(input+3,"time ",5)==0)
				{
					t2=ReadDigit(input+8,100000);
				}
				else if(strncmp(input+3,"depth ",6)==0)
				{
					setdepth=ReadDigit(input+9,1000);
				}
				SearchMain();
			}
			else if(strcmp(input,"quit")==0)
			{
				printf("bye\n");
				fflush(stdout);
				return 0;
			}
			else if(strcmp(input,"uaci")==0||strcmp(input,"uci")==0||strcmp(input,"ucci")==0)
			{
				printf("id name AnimalcraftAI\n");
				fflush(stdout);
				printf("id version 1.7\n");
				fflush(stdout);
				printf(input);
				printf("ok\n");
				fflush(stdout);
			}
		}
		else Sleep(1); 
	}
}
