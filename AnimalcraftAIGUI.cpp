#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "ucci.h"
#include "resource.h"

#pragma comment(lib, "winmm.lib")

const int WINDOW_STYLES = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
const int MASK_COLOR = RGB(0, 255, 0);
const int SQUARE_SIZE = 80;
const int BOARD_EDGE = 8;
const int BOARD_WIDTH = BOARD_EDGE + SQUARE_SIZE * 9 + BOARD_EDGE;
const int BOARD_HEIGHT = BOARD_EDGE + SQUARE_SIZE * 7 + BOARD_EDGE;

const char name[24][5]={"\xa1\xa1","\xd1\xa8","\xda\xe5","\xa1\xf6","\xda\xe5","\xd1\xa8","\xa1\xa1","\xa1\xa1","\xcf\xf3","\xca\xa8","\xbb\xa2","\xb1\xaa","\xc0\xc7","\xb9\xb7","\xc3\xa8","\xca\xf3","\xcf\xf3","\xca\xa8","\xbb\xa2","\xb1\xaa","\xc0\xc7","\xb9\xb7","\xc3\xa8","\xca\xf3"};
const int RANK_TOP = 3;
const int RANK_BOTTOM = 9;
const int FILE_LEFT = 3;
const int FILE_RIGHT = 11;
const int PIECE_ELEPHANT = 0;
const int PIECE_LION = 1;
const int PIECE_TIGER = 2;
const int PIECE_LEOPARD = 3;
const int PIECE_WOLF = 4;
const int PIECE_DOG = 5;
const int PIECE_CAT = 6;
const int PIECE_MOUSE = 7;
const int MAX_GEN_MOVES = 128;
const int MAX_MOVES = 1000;

int t2=2000,depth=99999999,t,t4=4000;
bool fenxi=0,player[2],turn,ranghu=0;
bool training=0;

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

static const char ccDelta[4] = {-16, -1, 1, 16};
static const char ccJumpDelta[4] = {-48,-4,4,48};

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

inline bool IN_BOARD(int sq) { return ccInBoard[sq] != 0; }
inline int RANK_Y(int sq) { return sq >> 4; }
inline int FILE_X(int sq) { return sq & 15; }
inline int COORD_XY(int x, int y) { return x + (y << 4); }
inline int PIECE_NAME(int pc) { return (pc&7); }
inline int SQUARE_FLIP(int sq) { return 254 - sq; }
inline int FILE_FLIP(int x) { return 14 - x; }
inline int RANK_FLIP(int y) { return 15 - y; }
inline bool INRIVER(int sq) { return ccInFort[sq]==3; }
inline bool INSHOUXUE(int sq,int tag) { if(tag==8) return ccInFort[sq]==1; return ccInFort[sq]==5; }
inline bool INXIANJING(int sq,int tag) { if(tag==8) return ccInFort[sq]==2; return ccInFort[sq]==4; }
inline int SIDE_TAG(int sd) { return 8 + (sd << 3); }
inline int OPP_SIDE_TAG(int sd) { return 16 - (sd << 3); }
inline int SRC(int mv) { return mv & 255; }
inline int DST(int mv) { return mv >> 8; }
inline int MOVE(int sqSrc, int sqDst) { return sqSrc + sqDst * 256; }
inline int BLACK(int sq) {
int x=FILE_X(sq),y=RANK_Y(sq);
return COORD_XY(14-x,y);
}

struct MoveStruct {
  int wmv,ucpcCaptured,wpc;
  int dwKey;
  void Set(int mv, int pcCaptured,int pc, int dwKey_) {
    wmv = mv;
    ucpcCaptured = pcCaptured;
    wpc=pc;
    dwKey = dwKey_;
  }
};

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
  for (i = 0; i < 256; i ++) { s[i] = i; }
  for (i = 0; i < 256; i ++) {
    j = (j + s[i]) & 255;
    uc = s[i];
    s[i] = s[j];
    s[j] = uc;
  }
}

struct ZobristStruct {
  int dwKey, dwLock0, dwLock1;
  void InitZero(void) { dwKey = dwLock0 = dwLock1 = 0; }
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

static struct {
  ZobristStruct Player;
  ZobristStruct Table[16][256];
} Zobrist;

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

struct PositionStruct {
  bool sdPlayer;
  int ucpcSquares[256];
  int nDistance, nMoveNum, nMoveNum2;
  MoveStruct mvsList[MAX_MOVES];
  ZobristStruct zobr;
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
                if(ucpcSquares[j]==PIECE_MOUSE+24-(ucpcSquares[src]-PIECE_NAME(ucpcSquares[src]))||!INRIVER(j)) return 0;
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
  if(ucpcSquares[src]==0||ucpcSquares[dst]==0) return 0;
  int as=PIECE_NAME(ucpcSquares[src]),bs=PIECE_NAME(ucpcSquares[dst]);
  if(INXIANJING(dst,ucpcSquares[dst]-bs)) return 1;
    if(as==PIECE_MOUSE&&bs==PIECE_ELEPHANT)
    {
      if(INRIVER(src)&&!INRIVER(dst)) return 0;
      return 1;
}
    if(as==PIECE_ELEPHANT&&bs==PIECE_MOUSE) return 0;
    return as<=bs;
  }
  void ClearBoard(void) {
    sdPlayer=FALSE;
    nDistance = 0;
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
    sdPlayer = !sdPlayer;
    zobr.Xor(Zobrist.Player);
  }
  void AddPiece(int sq, int pc) {
    ucpcSquares[sq] = pc;
    if (pc < 16) { zobr.Xor(Zobrist.Table[pc - 8][sq]); }
    else { zobr.Xor(Zobrist.Table[pc - 8][sq]); }
  }
  void DelPiece(int sq, int pc) {
    ucpcSquares[sq] = 0;
    if (pc < 16) { zobr.Xor(Zobrist.Table[pc - 8][sq]); }
    else { zobr.Xor(Zobrist.Table[pc - 8][sq]); }
  }
  bool Captured(void) const { return mvsList[nMoveNum - 1].ucpcCaptured != 0; }
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
  void Mirror(PositionStruct &posMirror) const;
};

void PositionStruct::Startup(void) {
  int sq, pc;
  ClearBoard();
  for (sq = 0; sq < 256; sq ++) {
  if(IN_BOARD(sq))
  {
    pc = cucpcStartup[sq];
    if (pc != 0) { AddPiece(sq, pc); }
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
  if (pcCaptured != 0) { DelPiece(sqDst, pcCaptured); }
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
  if (pcCaptured != 0) { AddPiece(sqDst, pcCaptured); }
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

int PositionStruct::GenerateMoves(int *mvs, bool bCapture)
{
  int  nGenMoves,  sqSrc, sqDst;
  int pcSelfSide, pcOppSide, pcSrc, pcDst;
  nGenMoves = 0;
  pcSelfSide = SIDE_TAG(sdPlayer);
  pcOppSide = OPP_SIDE_TAG(sdPlayer);
  for (sqSrc = 0; sqSrc < 256; sqSrc ++) {
  if(!IN_BOARD(sqSrc)) continue;
    pcSrc = ucpcSquares[sqSrc];
    if ((pcSrc & pcSelfSide) == 0) { continue; }
int delta;
    for(delta=0;delta<=3;delta++)
    {
    sqDst=sqSrc+ccDelta[delta];
    if(!IN_BOARD(sqDst)||INSHOUXUE(sqDst,pcSelfSide)) continue;
    pcDst = ucpcSquares[sqDst];
    if(!CanMove(sqSrc,sqDst))continue;
    if(!ucpcSquares[sqDst]&&bCapture) continue;
        if (ucpcSquares[sqDst] ? ((pcDst & pcOppSide) != 0  && CanEat(sqSrc,sqDst) ): (pcDst & pcSelfSide) == 0) {
          mvs[nGenMoves] = MOVE(sqSrc, sqDst);
          nGenMoves ++;
        }
}
for(delta=0;delta<=3;delta++)
{
    sqDst=sqSrc+ccJumpDelta[delta];
    if(!IN_BOARD(sqDst)||INSHOUXUE(sqDst,pcSelfSide)) continue;
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
  int sqSrc, sqDst;
  int pcSelfSide,pcOppSide, pcSrc, pcDst;
  sqSrc = SRC(mv);
  pcSrc = ucpcSquares[sqSrc];
  pcSelfSide = SIDE_TAG(sdPlayer);
  pcOppSide = OPP_SIDE_TAG(sdPlayer);
  if ((pcSrc & pcSelfSide) == 0) { return 0; }
  sqDst = DST(mv);
  pcDst = ucpcSquares[sqDst];
  if ((pcDst & pcSelfSide) != 0) { return 0; }
  int delta;
    for(delta=0;delta<=3;delta++)
    {
    int sqDst2=sqSrc+ccDelta[delta];
    if(!IN_BOARD(sqDst)||INSHOUXUE(sqDst,pcSelfSide)||sqDst2!=sqDst) continue;
    pcDst = ucpcSquares[sqDst];
    if(!CanMove(sqSrc,sqDst))continue;
        if (ucpcSquares[sqDst] ? ((pcDst & pcOppSide) != 0  && CanEat(sqSrc,sqDst) ): (pcDst & pcSelfSide) == 0) {
        return 1;
        }
}
    for(delta=0;delta<=3;delta++)
    {
    int sqDst2=sqSrc+ccJumpDelta[delta];
    if(!IN_BOARD(sqDst)||INSHOUXUE(sqDst,pcSelfSide)||sqDst2!=sqDst) continue;
    pcDst = ucpcSquares[sqDst];
    if(!CanJump(sqSrc,sqDst))continue;
        if (ucpcSquares[sqDst] ? ((pcDst & pcOppSide) != 0  && CanEat(sqSrc,sqDst) ): (pcDst & pcSelfSide) == 0) {
        return 1;
        }
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
if(INXIANJING(DST(mvsList[i].wmv),16)||INXIANJING(DST(mvsList[i].wmv),8))
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
if(INXIANJING(DST(mvsList[i].wmv),16)||INXIANJING(DST(mvsList[i].wmv),8))
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
bool PositionStruct::RepStatus(void) {
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

static PositionStruct pos;

static struct {
  HINSTANCE hInst;
  HWND hWnd;
  HDC hdc, hdcTmp;
  HBITMAP bmpBoard, bmpSelected, bmpTrap, bmpDen, bmpPieces[24];
  int sqSelected, mvLast;
  BOOL bGameOver;
} Xqwl;

void Startup(void) {
  pos.Startup();
  Xqwl.sqSelected = Xqwl.mvLast = 0;
  Xqwl.bGameOver = FALSE;
}

void fen_to_map(char fen[])
{
Startup();
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
}

static struct {
  int mvResult;
  char fen[1024];
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
void map_to_fen2(int undomoves)
{
int i;
for(i=0;i<undomoves;i++)
{
pos.UndoMakeMove();
}
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
strcat(Search.fen," moves");
for(i=0;i<undomoves;i++)
{
int mv=pos.mvsList[pos.nMoveNum].wmv;
pos.MakeMove(mv);
char tofour[15]="\0";
sprintf(tofour," %c%c%c%c",FILE_X(SRC(mv))+'a'-3,RANK_Y(SRC(mv))+'0'-3,FILE_X(DST(mv))+'a'-3,RANK_Y(DST(mv))+'0'-3);
strcat(Search.fen,tofour);
}
}

static void TransparentBlt2(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
    HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, UINT crTransparent, bool bFlip180 = false) {
  HDC hImageDC, hMaskDC;
  HBITMAP hOldImageBMP, hImageBMP, hOldMaskBMP, hMaskBMP;
  hImageBMP = CreateCompatibleBitmap(hdcDest, nWidthDest, nHeightDest);
  hMaskBMP = CreateBitmap(nWidthDest, nHeightDest, 1, 1, NULL);
  hImageDC = CreateCompatibleDC(hdcDest);
  hMaskDC = CreateCompatibleDC(hdcDest);
  hOldImageBMP = (HBITMAP) SelectObject(hImageDC, hImageBMP);
  hOldMaskBMP = (HBITMAP) SelectObject(hMaskDC, hMaskBMP);
  if (bFlip180) {
    StretchBlt(hImageDC, nWidthDest - 1, 0, -nWidthDest, nHeightDest,
        hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, SRCCOPY);
  } else if (nWidthDest == nWidthSrc && nHeightDest == nHeightSrc) {
    BitBlt(hImageDC, 0, 0, nWidthDest, nHeightDest,
        hdcSrc, nXOriginSrc, nYOriginSrc, SRCCOPY);
  } else {
    StretchBlt(hImageDC, 0, 0, nWidthDest, nHeightDest,
        hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, SRCCOPY);
  }
  SetBkColor(hImageDC, crTransparent);
  BitBlt(hMaskDC, 0, 0, nWidthDest, nHeightDest, hImageDC, 0, 0, SRCCOPY);
  SetBkColor(hImageDC, RGB(0,0,0));
  SetTextColor(hImageDC, RGB(255,255,255));
  BitBlt(hImageDC, 0, 0, nWidthDest, nHeightDest, hMaskDC, 0, 0, SRCAND);
  SetBkColor(hdcDest, RGB(255,255,255));
  SetTextColor(hdcDest, RGB(0,0,0));
  BitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
      hMaskDC, 0, 0, SRCAND);
  BitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
      hImageDC, 0, 0, SRCPAINT);
  SelectObject(hImageDC, hOldImageBMP);
  DeleteDC(hImageDC);
  SelectObject(hMaskDC, hOldMaskBMP);
  DeleteDC(hMaskDC);
  DeleteObject(hImageBMP);
  DeleteObject(hMaskBMP);
}

inline void DrawTransBmp(HDC hdc, HDC hdcTmp, int xx, int yy, HBITMAP bmp, bool bFlip180 = false) {
  SelectObject(hdcTmp, bmp);
  TransparentBlt2(hdc, xx, yy, SQUARE_SIZE, SQUARE_SIZE, hdcTmp, 0, 0, SQUARE_SIZE, SQUARE_SIZE, MASK_COLOR, bFlip180);
}

bool g_bBoardFlipped = false;
static volatile bool g_bPondering = false;

static volatile int g_ponderDepth = 0;
static volatile int g_ponderScore = 0;
static char g_ponderMoveStr[32] = "";
static HWND g_hPonderDlg = NULL;

static volatile bool g_bPonderHitSent = false;
static int g_ponderMove = 0;
static volatile int g_pendingApplyMove = 0;
static int g_lastPV[16];
static int g_lastPVCount = 0;
static volatile LONG g_ponderGeneration = 0;
static volatile bool g_bWaitingForClickResult = false;

struct ComputeThreadParam {
    volatile bool bRunning;
    volatile bool bCancelled;
    LONG generation;
    int mvResult;
};
static ComputeThreadParam g_computeParam = { false, false, 0, 0 };
static HANDLE g_hComputeThread = NULL;
static volatile bool g_bComputerThinking = false;
static const UINT WM_ENGINE_MOVE_READY = WM_APP + 6;
static const UINT WM_ENGINE_MOVE_DONE  = WM_APP + 7;

inline int DisplayXX(int x) {
    return g_bBoardFlipped
        ? BOARD_EDGE + (FILE_RIGHT - x) * SQUARE_SIZE
        : BOARD_EDGE + (x - FILE_LEFT) * SQUARE_SIZE;
}
inline int DisplayYY(int y) {
    return g_bBoardFlipped
        ? BOARD_EDGE + (RANK_BOTTOM - y) * SQUARE_SIZE
        : BOARD_EDGE + (y - RANK_TOP) * SQUARE_SIZE;
}

static void UpdateTitle(void) {
  char title[256];
  const char *side = pos.sdPlayer ? "\xc0\xb6\xb7\xbd" : "\xba\xec\xb7\xbd";
  char ponderTag[160] = "";
  if (g_bPondering || g_bComputerThinking) {
    if (g_ponderDepth > 0) {
      sprintf(ponderTag, "\xa3\xa8\xba\xf3\xcc\xa8\xcb\xbc\xbf\xbc\xd6\xd0 [\xc9\xee\xb6\xc8%d \xc6\xc0\xb7\xd6%+d \xd5\xd0\xb7\xa8%s]\xa3\xa9",
          g_ponderDepth, g_ponderScore, g_ponderMoveStr);
    } else {
      sprintf(ponderTag, "\xa3\xa8\xba\xf3\xcc\xa8\xcb\xbc\xbf\xbc\xd6\xd0\xa1\xad\xa3\xa9");
    }
  }
  if (Xqwl.bGameOver) {
    sprintf(title, "\xb6\xb7\xca\xde\xc6\xe5 AI - \xd3\xce\xcf\xb7\xbd\xe1\xca\xf8%s", ponderTag);
  } else {
    sprintf(title, "\xb6\xb7\xca\xde\xc6\xe5 AI - \xb5\xda%d\xbb\xd8\xba\xcf\xa3\xac\xc2\xd6\xb5\xbd%s\xd7\xdf%s", (pos.nMoveNum + 1) / 2, side, ponderTag);
  }
  SetWindowText(Xqwl.hWnd, title);
}

static void DrawBoard(HDC hdc) {
  int x, y, xx, yy, sq, pc;
  HDC hdcTmp;

  UpdateTitle();

  hdcTmp = CreateCompatibleDC(hdc);
  SelectObject(hdcTmp, Xqwl.bmpBoard);
  BitBlt(hdc, 0, 0, BOARD_WIDTH, BOARD_HEIGHT, hdcTmp, 0, 0, SRCCOPY);
  for (x = FILE_LEFT; x <= FILE_RIGHT; x ++) {
    for (y = RANK_TOP; y <= RANK_BOTTOM; y ++) {
        xx = DisplayXX(x);
        yy = DisplayYY(y);
      sq = COORD_XY(x, y);
      pc = pos.ucpcSquares[sq];
      switch(ccInFort[sq])
      {
      case 1:
      case 5:
      {
      DrawTransBmp(hdc, hdcTmp, xx, yy, Xqwl.bmpDen);
      break;
}
      case 2:
      case 4: DrawTransBmp(hdc, hdcTmp, xx, yy, Xqwl.bmpTrap);
}
      if (pc != 0) {
        DrawTransBmp(hdc, hdcTmp, xx, yy, Xqwl.bmpPieces[pc], g_bBoardFlipped);
      }
      if (sq == Xqwl.sqSelected || sq == SRC(Xqwl.mvLast) || sq == DST(Xqwl.mvLast)) {
        DrawTransBmp(hdc, hdcTmp, xx, yy, Xqwl.bmpSelected);
      }
    }
  }
  DeleteDC(hdcTmp);
}

inline void PlayResWav(int nResId) {
  PlaySound(MAKEINTRESOURCE(nResId), Xqwl.hInst, SND_ASYNC | SND_NOWAIT | SND_RESOURCE);
}

const BOOL DRAW_SELECTED = TRUE;

static void DrawSquare(int sq, BOOL bSelected = FALSE) {
  int sqFlipped, xx, yy, pc;

  sqFlipped = sq;
  xx = DisplayXX(FILE_X(sqFlipped));
  yy = DisplayYY(RANK_Y(sqFlipped));
  SelectObject(Xqwl.hdcTmp, Xqwl.bmpBoard);
  BitBlt(Xqwl.hdc, xx, yy, SQUARE_SIZE, SQUARE_SIZE, Xqwl.hdcTmp, xx, yy, SRCCOPY);

  pc = pos.ucpcSquares[sq];
      switch(ccInFort[sq])
      {
      case 1:
      case 5:
      {
      DrawTransBmp(Xqwl.hdc, Xqwl.hdcTmp, xx, yy, Xqwl.bmpDen);
      break;
}
      case 2:
      case 4: DrawTransBmp(Xqwl.hdc, Xqwl.hdcTmp, xx, yy, Xqwl.bmpTrap);
}
  if (pc != 0) {
    DrawTransBmp(Xqwl.hdc, Xqwl.hdcTmp, xx, yy, Xqwl.bmpPieces[pc], g_bBoardFlipped);
  }
  if (bSelected) {
    DrawTransBmp(Xqwl.hdc, Xqwl.hdcTmp, xx, yy, Xqwl.bmpSelected);
  }
}

static void ParseInfoPV(const char *input);
static void ParsePonderInfoLine(const char *input);
static void StopPondering(void);

static void ResetAnalysisState(void) {
  g_ponderDepth = 0;
  g_ponderScore = 0;
  g_ponderMoveStr[0] = '\0';
  g_ponderMove = 0;
  g_pendingApplyMove = 0;
  g_bPonderHitSent = false;
  g_bPondering = false;
  g_lastPVCount = 0;
}

static bool ParseMove4(const char *s, int &mv) {
  if (s == NULL) return false;
  if (!(s[0] >= 'a' && s[0] <= 'i' &&
        s[1] >= '0' && s[1] <= '9' &&
        s[2] >= 'a' && s[2] <= 'i' &&
        s[3] >= '0' && s[3] <= '9')) {
    return false;
  }
  int a=s[0]-'a'+3, b=s[1]-'0'+3, c=s[2]-'a'+3, d=s[3]-'0'+3;
  mv = MOVE(COORD_XY(a,b), COORD_XY(c,d));
  return true;
}

static void SendCurrentPositionToEngine(void) {
  char input[1200];
  map_to_fen2((pos.nMoveNum-1>=40)?40:(pos.nMoveNum-1));
  sprintf(input,"fen %s",Search.fen);
  pipeStdHandle.LineOutput(input);
}

static void SendPonderPositionToEngine(int ponderMove) {
  char input[1200];
  pos.MakeMove(ponderMove);
  map_to_fen2((pos.nMoveNum-1>=40)?40:(pos.nMoveNum-1));
  sprintf(input,"fen %s",Search.fen);
  pos.UndoMakeMove();
  pipeStdHandle.LineOutput(input);
}

static void BuildGoCommand(char *outBuf, bool bPonder) {
  if (bPonder) {
    sprintf(outBuf, "go ponder");
  } else if (depth != 99999999) {
    sprintf(outBuf, "go depth %d", depth);
  } else {
    sprintf(outBuf, "go time %d", t4);
  }
}

static bool WaitEngineBootReady(DWORD timeoutMs) {
  char input[1024] = "";
  DWORD start = GetTickCount();
  while (GetTickCount() - start < timeoutMs) {
    if (pipeStdHandle.LineInput(input)) {
      if (strcmp(input, "uaciok") == 0) {
        return true;
      }
    } else {
      Sleep(1);
    }
  }
  return false;
}

static void CleanupComputeHandle(void) {
    if (g_hComputeThread) {
        WaitForSingleObject(g_hComputeThread, 3000);
        CloseHandle(g_hComputeThread);
        g_hComputeThread = NULL;
    }
    g_computeParam.bRunning = false;
    g_bComputerThinking = false;
}

static DWORD WINAPI ComputeThread(LPVOID lpParam) {
    ComputeThreadParam *param = (ComputeThreadParam*)lpParam;
    char input[1024];
    LONG myGeneration = param->generation;
    while (param->bRunning) {
        if (pipeStdHandle.LineInput(input)) {
            if (!strncmp(input,"bestmove ",9)) {
                int mv = 0;
                if (ParseMove4(input + 9, mv)) {
                    param->mvResult = mv;
                    PostMessage(Xqwl.hWnd, WM_ENGINE_MOVE_READY, (WPARAM)myGeneration, (LPARAM)mv);
                } else {
                    param->mvResult = 0;
                    PostMessage(Xqwl.hWnd, WM_ENGINE_MOVE_DONE, (WPARAM)myGeneration, 0);
                }
                break;
            }
            if (!strncmp(input,"info",4)) ParsePonderInfoLine(input);
        } else {
            Sleep(1);
        }
    }
    param->bRunning = false;
    return 0;
}

static bool StartComputerMove(void) {
    char input[1024];
    if (g_bComputerThinking) return false;
    StopPondering();
    g_ponderDepth = 0;
    g_ponderScore = 0;
    g_ponderMoveStr[0] = '\0';
    SendCurrentPositionToEngine();
    Sleep(5);
    BuildGoCommand(input, false);
    pipeStdHandle.LineOutput(input);
    Sleep(5);
    g_lastPVCount = 0;
    g_computeParam.bCancelled = false;
    g_computeParam.mvResult = 0;
    g_computeParam.bRunning = true;
    g_computeParam.generation = InterlockedIncrement(&g_ponderGeneration);
    g_hComputeThread = CreateThread(NULL, 0, ComputeThread, &g_computeParam, 0, NULL);
    if (g_hComputeThread == NULL) {
        g_computeParam.bRunning = false;
        return false;
    }
    g_bComputerThinking = true;
    UpdateTitle();
    SetCursor((HCURSOR) LoadImage(NULL, IDC_WAIT, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    return true;
}

static INT_PTR CALLBACK PonderDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
  case WM_CLOSE:
    DestroyWindow(hDlg);
    g_hPonderDlg = NULL;
    return TRUE;
  case WM_APP+3: {
    char line[256];
    sprintf(line, "\xc9\xee\xb6\xc8\x3a\x20%d\r\n\xc6\xc0\xb7\xd6\x3a\x20%+d\r\n\xd5\xd0\xb7\xa8\x3a\x20%s",
        g_ponderDepth, g_ponderScore, g_ponderMoveStr);
    SetWindowText(GetDlgItem(hDlg, IDC_PONDER_EDIT), line);
    return TRUE;
  }
  }
  return FALSE;
}

struct PonderThreadParam {
    volatile bool bStop;
    LONG generation;
};
static PonderThreadParam g_ponderParam = { false, 0 };
static HANDLE g_hPonderThread = NULL;
static bool g_bPonderEnabled = false;
static volatile bool g_bPonderSuppressResult = false;

static void ParseInfoPV(const char *input) {
  const char *p = strstr(input, " pv ");
  if (!p) return;
  p += 4;
  int cnt = 0;
  while (cnt < 16 && p[0] >= 'a' && p[0] <= 'i' &&
         p[1] >= '0' && p[1] <= '9' &&
         p[2] >= 'a' && p[2] <= 'i' &&
         p[3] >= '0' && p[3] <= '9') {
    int a = p[0]-'a'+3, b = p[1]-'0'+3, c = p[2]-'a'+3, d = p[3]-'0'+3;
    g_lastPV[cnt++] = MOVE(COORD_XY(a,b), COORD_XY(c,d));
    p += 4;
    while (*p == ' ') p++;
  }
  if (cnt > 0) g_lastPVCount = cnt;
}

static void ParsePonderInfoLine(const char *input) {
    const char *p;
    int ldepth = 0, lscore = 0;
    bool hasScore = false;
    char rawpv[8] = "";

    ParseInfoPV(input);

    p = strstr(input, "depth");
    if (p) {
        ldepth = atoi(p + 6);
    }

    p = strstr(input, "score");
    if (p) {
        lscore = atoi(p + 6);
        hasScore = true;
    }

    p = strstr(input, " pv ");
    if (p) {
        p += 4;
        strncpy(rawpv, p, 4);
        rawpv[4] = '\0';
    }

    if (ldepth <= 0 || !hasScore) return;

    g_ponderDepth = ldepth;
    g_ponderScore = lscore;

    if (strlen(rawpv) >= 4 && rawpv[0] >= 'a' && rawpv[1] >= '0' && rawpv[1] <= '9') {
        int a = rawpv[0]-'a'+3, b = rawpv[1]-'0'+3;
        int sq = COORD_XY(a, b);
        int pc = pos.ucpcSquares[sq];
        const char *animalName = (pc != 0) ? name[PIECE_NAME(pc)+8] : "";
        sprintf(g_ponderMoveStr, "%s%c%c\xa1\xfa%c%c",
            animalName, rawpv[0], rawpv[1], rawpv[2], rawpv[3]);
    }

    if (Xqwl.hWnd) {
        PostMessage(Xqwl.hWnd, WM_APP+2, 0, 0);
    }
    if (g_hPonderDlg) {
        PostMessage(g_hPonderDlg, WM_APP+3, 0, 0);
    }
}

static DWORD WINAPI PonderThread(LPVOID lpParam) {
    PonderThreadParam *param = (PonderThreadParam*)lpParam;
    LONG myGeneration = param->generation;
    char input[1024];
    while (true) {
        if (pipeStdHandle.LineInput(input)) {
            if (strncmp(input, "bestmove", 8) == 0) {
                if (g_bPonderSuppressResult) {
                    PostMessage(Xqwl.hWnd, WM_APP+5, (WPARAM)myGeneration, 0);
                } else if (g_bPonderHitSent) {
                    char *point = input + 9;
                    int mv;
                    if (ParseMove4(point, mv)) {
                        g_pendingApplyMove = mv;
                        PostMessage(Xqwl.hWnd, WM_APP+4, (WPARAM)myGeneration, 0);
                    } else {
                        PostMessage(Xqwl.hWnd, WM_APP+5, (WPARAM)myGeneration, 0);
                    }
                } else {
                    PostMessage(Xqwl.hWnd, WM_APP+5, (WPARAM)myGeneration, 0);
                }
                break;
            }
            if (strncmp(input, "info", 4) == 0) {
                ParsePonderInfoLine(input);
            }
        } else {
            Sleep(5);
        }
    }
    g_bPondering = false;
    return 0;
}

static void StartPondering(void) {
    if (!g_bPonderEnabled || Xqwl.bGameOver || g_bPondering || training || g_bComputerThinking) return;

    char input[1024];
    g_ponderDepth = 0;
    g_ponderScore = 0;
    g_ponderMoveStr[0] = '\0';

    g_ponderMove = 0;
    if (g_lastPVCount >= 2 && pos.LegalMove(g_lastPV[1])) {
        g_ponderMove = g_lastPV[1];
    }

    if (g_ponderMove != 0) {
        SendPonderPositionToEngine(g_ponderMove);
    } else {
        SendCurrentPositionToEngine();
    }
    Sleep(5);
    BuildGoCommand(input, true);
    pipeStdHandle.LineOutput(input);

    g_bPonderHitSent = false;
    g_bPonderSuppressResult = false;
    g_ponderParam.bStop = false;
    g_ponderParam.generation = InterlockedIncrement(&g_ponderGeneration);
    g_bPondering = true;
    g_hPonderThread = CreateThread(NULL, 0, PonderThread, &g_ponderParam, 0, NULL);
    UpdateTitle();
    if (g_hPonderDlg) PostMessage(g_hPonderDlg, WM_APP+3, 0, 0);
}

static void StopPondering(void) {
    if (!g_bPondering && g_hPonderThread == NULL) return;
    g_ponderParam.bStop = true;
    g_bPonderSuppressResult = true;
    InterlockedIncrement(&g_ponderGeneration);
    pipeStdHandle.LineOutput("stop");
    if (g_hPonderThread) {
        if (WaitForSingleObject(g_hPonderThread, 3000) == WAIT_TIMEOUT) {
            TerminateThread(g_hPonderThread, 0);
        }
        CloseHandle(g_hPonderThread);
        g_hPonderThread = NULL;
    }
    g_bPondering = false;
    UpdateTitle();
}

static void CleanupPonderHandle(void) {
    if (g_hPonderThread) {
        WaitForSingleObject(g_hPonderThread, 3000);
        CloseHandle(g_hPonderThread);
        g_hPonderThread = NULL;
    }
    g_bPondering = false;
}

static void Computer(void)
{
    StartComputerMove();
}

static void ApplyEngineMove(int mv) {
  Search.mvResult = mv;
  if(!pos.LegalMove(Search.mvResult))
  {
    MessageBox(0,"\xd7\xa3\xba\xd8\xc4\xe3\xc8\xa1\xb5\xc3\xca\xa4\xc0\xfb\xa3\xa1","AnimalcraftAI",0);
    Xqwl.bGameOver = TRUE;
    UpdateTitle();
    return;
  }
  DrawSquare(SRC(Xqwl.mvLast));
  DrawSquare(DST(Xqwl.mvLast));
  pos.MakeMove(Search.mvResult);
  Xqwl.mvLast = Search.mvResult;
  DrawSquare(SRC(Xqwl.mvLast), DRAW_SELECTED);
  DrawSquare(DST(Xqwl.mvLast), DRAW_SELECTED);
 if (pos.IsMate()) {
    MessageBox(0,"\xc7\xeb\xd4\xd9\xbd\xd3\xd4\xd9\xc0\xf7\xa3\xa1","AnimalcraftAI",0);
    Xqwl.bGameOver = TRUE;
  }else {
  if(pos.nMoveNum2>600)
  {
    MessageBox(0,"\xb3\xac\xb9\xfd\xd7\xd4\xc8\xbb\xcf\xde\xd7\xc5\xd7\xf7\xba\xcd\xa3\xa1","AnimalcraftAI",0);
    Xqwl.bGameOver = TRUE;
  }
        else if(pos.RepStatus())
        {
    MessageBox(0,"\xd7\xa3\xba\xd8\xc4\xe3\xc8\xa1\xb5\xc3\xca\xa4\xc0\xfb\xa3\xa1","AnimalcraftAI",0);
    Xqwl.bGameOver = TRUE;
}
else
{
if(pos.Captured()) pos.SetIrrev2();
PlayResWav(IDR_ELEPHANT+PIECE_NAME(pos.mvsList[pos.nMoveNum-1].wpc));
}
  }
  UpdateTitle();
}

static void ResponseMove(void) {
  Computer();
}

static void ClickSquare(int sq) {
  int pc, mv;
  if (g_bWaitingForClickResult || g_bComputerThinking) return;
  Xqwl.hdc = GetDC(Xqwl.hWnd);
  Xqwl.hdcTmp = CreateCompatibleDC(Xqwl.hdc);
  pc = pos.ucpcSquares[sq];

  if ((pc & SIDE_TAG(pos.sdPlayer)) != 0) {
    if (Xqwl.sqSelected != 0) {
      DrawSquare(Xqwl.sqSelected);
    }
    Xqwl.sqSelected = sq;
    DrawSquare(sq, DRAW_SELECTED);
    if (Xqwl.mvLast != 0) {
      DrawSquare(SRC(Xqwl.mvLast));
      DrawSquare(DST(Xqwl.mvLast));
    }

  } else if (Xqwl.sqSelected != 0 && !Xqwl.bGameOver) {
    mv = MOVE(Xqwl.sqSelected, sq);

    if (pos.LegalMove(mv)) {
      if (pos.MakeMove(mv)) {

        Xqwl.mvLast = mv;
        DrawSquare(Xqwl.sqSelected, DRAW_SELECTED);
        DrawSquare(sq, DRAW_SELECTED);
        Xqwl.sqSelected = 0;

        if (pos.IsMate()) {
MessageBox(0,"\xd7\xa3\xba\xd8\xc4\xe3\xc8\xa1\xb5\xc3\xca\xa4\xc0\xfb\xa3\xa1","AnimalcraftAI",0);
          Xqwl.bGameOver = TRUE;
        } else if(pos.nMoveNum2>600)
  {
    MessageBox(0,"\xb3\xac\xb9\xfd\xd7\xd4\xc8\xbb\xcf\xde\xd7\xc5\xd7\xf7\xba\xcd\xa3\xa1","AnimalcraftAI",0);
          Xqwl.bGameOver = 1;
  }
else if(pos.RepStatus())
{
/*if(pos.RepWuLai()) MessageBox(0,"\xce\xa5\xc0\xfd\xa3\xba\xce\xde\xc0\xb5\xd1\xad\xbb\xb7","AnimalcraftAI",0);*/
if(pos.RepWuSong()) MessageBox(0,"\xce\xa5\xc0\xfd\xa3\xba\xb3\xa4\xd7\xbd","AnimalcraftAI",0);
pos.UndoMakeMove();
}
else {
if(pos.Captured()) pos.SetIrrev2();
PlayResWav(IDR_ELEPHANT+PIECE_NAME(pos.mvsList[pos.nMoveNum-1].wpc));
if(training==0) {
    if (g_bPondering) {
        g_bWaitingForClickResult = true;
        if (g_ponderMove != 0 && mv == g_ponderMove) {
            g_bPonderHitSent = true;
            pipeStdHandle.LineOutput("ponderhit");
        } else {
            g_bPonderHitSent = false;
            pipeStdHandle.LineOutput("stop");
        }
        SetCursor((HCURSOR) LoadImage(NULL, IDC_WAIT, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    } else {
        g_bWaitingForClickResult = true;
        StartComputerMove();
    }
}
}
      }
    }
  }
  UpdateTitle();
  DeleteDC(Xqwl.hdcTmp);
  ReleaseDC(Xqwl.hWnd, Xqwl.hdc);
}
char FEN[256];

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
char send[1024];
  int x, y;
  HDC hdc;
  RECT rect;
  PAINTSTRUCT ps;
  MSGBOXPARAMS mbp;

  switch (uMsg) {
  case WM_CREATE:
    GetWindowRect(hWnd, &rect);
    x = rect.left;
    y = rect.top;
    rect.right = rect.left + BOARD_WIDTH;
    rect.bottom = rect.top + BOARD_HEIGHT;
    AdjustWindowRect(&rect, WINDOW_STYLES, TRUE);
    MoveWindow(hWnd, x, y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
    break;
  case WM_DESTROY:
    if (g_hPonderDlg) {
      DestroyWindow(g_hPonderDlg);
      g_hPonderDlg = NULL;
    }
    PostQuitMessage(0);
    break;
  case WM_APP+2:
    UpdateTitle();
    break;
  case WM_APP+4: {
    g_bWaitingForClickResult = false;
    if ((LONG)wParam != g_ponderGeneration) {
        break;
    }
    CleanupPonderHandle();
    SetCursor((HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    if (!g_bPonderSuppressResult) {
        Xqwl.hdc = GetDC(Xqwl.hWnd);
        Xqwl.hdcTmp = CreateCompatibleDC(Xqwl.hdc);
        ApplyEngineMove(g_pendingApplyMove);
        DeleteDC(Xqwl.hdcTmp);
        ReleaseDC(Xqwl.hWnd, Xqwl.hdc);
        StartPondering();
    }
    g_bPonderSuppressResult = false;
    break;
  }
  case WM_APP+5: {
    g_bWaitingForClickResult = false;
    if ((LONG)wParam != g_ponderGeneration) {
        break;
    }
    CleanupPonderHandle();
    SetCursor((HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    if (!g_bPonderSuppressResult && training == 0 && !Xqwl.bGameOver) {
        g_bWaitingForClickResult = true;
        StartComputerMove();
    }
    g_bPonderSuppressResult = false;
    break;
  }
  case WM_ENGINE_MOVE_READY: {
    if ((LONG)wParam != g_computeParam.generation) {
        break;
    }
    CleanupComputeHandle();
    SetCursor((HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    g_bWaitingForClickResult = false;
    Xqwl.hdc = GetDC(Xqwl.hWnd);
    Xqwl.hdcTmp = CreateCompatibleDC(Xqwl.hdc);
    ApplyEngineMove((int)lParam);
    DeleteDC(Xqwl.hdcTmp);
    ReleaseDC(Xqwl.hWnd, Xqwl.hdc);
    StartPondering();
    break;
  }
  case WM_ENGINE_MOVE_DONE: {
    if ((LONG)wParam != g_computeParam.generation) {
        break;
    }
    CleanupComputeHandle();
    SetCursor((HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    g_bWaitingForClickResult = false;
    break;
  }
  case WM_COMMAND:
    if ((g_bWaitingForClickResult || g_bComputerThinking) && LOWORD(wParam) != 414) {
      break;
    }
    if (LOWORD(wParam) != 414) {
      StopPondering();
    }
    switch (LOWORD(wParam)) {
    case IDM_FILE_RED:
  PlayResWav(IDR_START);
  training=0;
      ResetAnalysisState();
      Startup();
      fen_to_map(FEN);
      map_to_fen();
      sprintf(send,"fen %s",Search.fen);
      pipeStdHandle.LineOutput(send);

      hdc = GetDC(Xqwl.hWnd);
      DrawBoard(hdc);
      ReleaseDC(Xqwl.hWnd, hdc);
      StartPondering();
      break;
    case IDM_FILE_BLACK:
      PlayResWav(IDR_START);
      if (!Xqwl.bGameOver) {
        StartComputerMove();
      }
      break;
    case IDM_FILE_TRAINING:
  PlayResWav(IDR_START);
  training=1;
      ResetAnalysisState();
      Startup();
      fen_to_map(FEN);
      hdc = GetDC(Xqwl.hWnd);
      DrawBoard(hdc);
      ReleaseDC(Xqwl.hWnd, hdc);
      break;
    case IDM_UNDO:
      ResetAnalysisState();
      if (pos.nMoveNum > 2 && !training) {
        pos.UndoMakeMove();
        pos.UndoMakeMove();
      } else if (pos.nMoveNum > 1 && training) {
        pos.UndoMakeMove();
      }
      Xqwl.sqSelected = 0;
      Xqwl.bGameOver = FALSE;
      hdc = GetDC(Xqwl.hWnd);
      DrawBoard(hdc);
      ReleaseDC(Xqwl.hWnd, hdc);
      StartPondering();
      break;
    case IDM_FILE_EXIT:
      DestroyWindow(Xqwl.hWnd);
      break;
    case IDM_STANDARD:
{
strcpy(FEN,"T1E3m1l/1C5d1/2W3p2/9/2P3w2/1D5c1/L1M3e1t r");
      ResetAnalysisState();
      fen_to_map(FEN);
      hdc = GetDC(Xqwl.hWnd);
      DrawBoard(hdc);
      ReleaseDC(Xqwl.hWnd, hdc);
break;
}
    case IDM_HANDICAP:
{
strcpy(FEN,"T1E3m1l/1C5d1/2W3p2/9/2P3w2/1D5c1/L1M3e2 r");
      ResetAnalysisState();
      fen_to_map(FEN);
      hdc = GetDC(Xqwl.hWnd);
      DrawBoard(hdc);
      ReleaseDC(Xqwl.hWnd, hdc);
break;
}
    case IDM_LOADFEN:
    {
if (!IsClipboardFormatAvailable(CF_TEXT))
{
break;
}
if (!OpenClipboard(NULL))
{
break;
}
HGLOBAL hMem = GetClipboardData(CF_TEXT);
if (hMem != NULL)
{
LPTSTR lpStr = (LPTSTR)GlobalLock(hMem);
if (lpStr != NULL)
{
strcpy(FEN,lpStr);
GlobalUnlock(hMem);
  ResetAnalysisState();
  Startup();
fen_to_map(FEN);
hdc = GetDC(Xqwl.hWnd);
    DrawBoard(hdc);
    ReleaseDC(Xqwl.hWnd, hdc);
}
}
CloseClipboard();
break;
}
case IDM_OUTPUTFEN:
{
map_to_fen();
if(OpenClipboard(NULL))
{
HANDLE hClip;
char* pBuf;
EmptyClipboard();

hClip=GlobalAlloc(GMEM_MOVEABLE,strlen(Search.fen)+1);
pBuf=(char*)GlobalLock(hClip);
strcpy(pBuf,Search.fen);
GlobalUnlock(hClip);
SetClipboardData(CF_TEXT,hClip);

CloseClipboard();
}
break;
}
case 412:
{
    g_bBoardFlipped = !g_bBoardFlipped;
    hdc = GetDC(Xqwl.hWnd);
    DrawBoard(hdc);
    ReleaseDC(Xqwl.hWnd, hdc);
    break;
}
case 413:
{
    g_bPonderEnabled = !g_bPonderEnabled;
    HMENU hMenu = GetMenu(Xqwl.hWnd);
    CheckMenuItem(hMenu, 413, MF_BYCOMMAND | (g_bPonderEnabled ? MF_CHECKED : MF_UNCHECKED));
    if (g_bPonderEnabled) {
        StartPondering();
    } else {
        StopPondering();
    }
    break;
}
case 411:
{
    if (MessageBox(Xqwl.hWnd, "\xd6\xb4\xd0\xd0\xa1\xb8\xd0\xe9\xd2\xbb\xca\xd6\xa1\xb9\xbd\xab\xbb\xe1\xc7\xe5\xbf\xd5\xd6\xae\xc7\xb0\xb5\xc4\xbb\xda\xc6\xe5\xbc\xcd\xc2\xbc\xa3\xac\xc8\xb7\xb6\xa8\xd2\xaa\xbc\xcc\xd0\xf8\xc2\xf0\xa3\xbf", "\xd0\xe9\xd2\xbb\xca\xd6\xc8\xb7\xc8\xcf", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        ResetAnalysisState();
        pos.ChangeSide();
        pos.SetIrrev();
        pos.SetIrrev2();

        Xqwl.sqSelected = 0;

        if (training == 0 && !Xqwl.bGameOver) {
            StartComputerMove();
        } else {
            hdc = GetDC(Xqwl.hWnd);
            DrawBoard(hdc);
            ReleaseDC(Xqwl.hWnd, hdc);
        }
    }
    break;
}
case 414:
{
    if (g_hPonderDlg == NULL) {
        g_hPonderDlg = CreateDialog(Xqwl.hInst, MAKEINTRESOURCE(IDD_PONDER_DIALOG), Xqwl.hWnd, PonderDialogProc);
        if (g_hPonderDlg) {
            ShowWindow(g_hPonderDlg, SW_SHOW);
            PostMessage(g_hPonderDlg, WM_APP+3, 0, 0);
        }
    } else {
        SetForegroundWindow(g_hPonderDlg);
    }
    break;
}
    break;
    }
    break;
  case WM_PAINT:
    hdc = BeginPaint(Xqwl.hWnd, &ps);
    DrawBoard(hdc);
    EndPaint(Xqwl.hWnd, &ps);
    break;
  case WM_LBUTTONDOWN:
    if (g_bBoardFlipped) {
      x = FILE_RIGHT - (LOWORD(lParam) - BOARD_EDGE) / SQUARE_SIZE;
      y = RANK_BOTTOM - (HIWORD(lParam) - BOARD_EDGE) / SQUARE_SIZE;
    } else {
      x = FILE_LEFT + (LOWORD(lParam) - BOARD_EDGE) / SQUARE_SIZE;
      y = RANK_TOP + (HIWORD(lParam) - BOARD_EDGE) / SQUARE_SIZE;
    }
    if (x >= FILE_LEFT && x <= FILE_RIGHT && y >= RANK_TOP && y <= RANK_BOTTOM) {
      ClickSquare(COORD_XY(x, y));
    }
    break;
  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  return FALSE;
}

inline HBITMAP LoadResBmp(int nResId) {
  return (HBITMAP) LoadImage(Xqwl.hInst, MAKEINTRESOURCE(nResId), IMAGE_BITMAP,0,0, LR_DEFAULTSIZE | LR_SHARED);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

pipeStdHandle.Open("engine.exe");
if (!WaitEngineBootReady(5000)) {
MessageBox(0,"\xd2\xfd\xc7\xe6\xce\xc4\xbc\xfe\xbc\xd7\xd4\xd8\xca\xa7\xb0\xdc","AnimalcraftAI",0);
pipeStdHandle.LineOutput("quit");
pipeStdHandle.Close();
return 0;
}

PlayResWav(IDR_START);

  int i;
  MSG msg;
  WNDCLASSEX wce;

  srand((DWORD) time(NULL));
  InitZobrist();
  Xqwl.hInst = hInstance;
  strcpy(FEN,"T1E3m1l/1C5d1/2W3p2/9/2P3w2/1D5c1/L1M3e1t r");
  fen_to_map(FEN);

  Xqwl.bmpBoard = LoadResBmp(IDB_BOARD);
  Xqwl.bmpSelected = LoadResBmp(IDB_SELECTED);
  for (i = PIECE_ELEPHANT; i <= PIECE_MOUSE; i ++) {
    Xqwl.bmpPieces[SIDE_TAG(0) + i] = LoadResBmp(IDB_RE + i);
    Xqwl.bmpPieces[SIDE_TAG(1) + i] = LoadResBmp(IDB_BE + i);
  }
    Xqwl.bmpTrap = LoadResBmp(IDB_TRAP);
    Xqwl.bmpDen = LoadResBmp(IDB_DEN);

  wce.cbSize = sizeof(WNDCLASSEX);
  wce.style = 0;
  wce.lpfnWndProc = (WNDPROC) WndProc;
  wce.cbClsExtra = wce.cbWndExtra = 0;
  wce.hInstance = hInstance;
  wce.hIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 32, 32, LR_SHARED);
  wce.hCursor = (HCURSOR) LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
  wce.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
  wce.lpszMenuName = MAKEINTRESOURCE(IDM_MAINMENU);
  wce.lpszClassName = "AnimalcraftAI";
  wce.hIconSm = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, LR_SHARED);
  RegisterClassEx(&wce);

  Xqwl.hWnd = CreateWindow("AnimalcraftAI", "AnimalcraftAI", WINDOW_STYLES,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
  if (Xqwl.hWnd == NULL) {
    return 0;
  }
  ShowWindow(Xqwl.hWnd, nCmdShow);
  UpdateWindow(Xqwl.hWnd);

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

StopPondering();
CleanupComputeHandle();
pipeStdHandle.LineOutput("quit");
pipeStdHandle.Close();
  return msg.wParam;
}
