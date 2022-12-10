//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Yuxiang Chen";
const char *studentID = "A59016369";
const char *email = "yuc129@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

uint32_t ghistoryReg = 0;
uint8_t *globalPredictor;
uint8_t *localPredictor;
uint32_t *lhistoryRegs;
uint8_t *choice;
uint8_t lpred;
uint8_t gpred;

#define W_LEN 251
#define W_H 32
#define MAX_WEIGHT 1 << 7

int8_t W[W_LEN][W_H];     // 8*251*32 = 62.75Kbits
int8_t ghistory[W_H - 1]; // 31*8 = 0.24Kbits

int threshold;
uint8_t _hot = -1;
uint8_t train = 0;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
int hash(uint32_t pc) { return (pc * 3) % W_LEN; }

void backward(int8_t *w, uint8_t same) { *w += same == 1 ? (*w < MAX_WEIGHT - 1 ? 1 : 0) : (*w > -MAX_WEIGHT ? -1 : 0); }
int forward(uint32_t pc)
{
  int index = hash(pc);
  int out = W[index][0];

  for (int i = 1; i < W_H; i++)
    out += ghistory[i - 1] * W[index][i];

  _hot = (out >= 0) ? TAKEN : NOTTAKEN;
  train = (out < threshold && out > -threshold) ? 1 : 0;

  return _hot;
}

void init_predictor()
{
  //
  // TODO: Initialize Branch Predictor Data Structures
  //
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    ghistoryReg = 0;
    globalPredictor = malloc((1 << ghistoryBits) * sizeof *globalPredictor);
    for (int i = 0; i < (1 << ghistoryBits); i++)
      globalPredictor[i] = 1;
    break;
  case TOURNAMENT:
    ghistoryReg = 0;
    globalPredictor = malloc((1 << ghistoryBits) * sizeof *globalPredictor);
    for (int i = 0; i < (1 << ghistoryBits); i++)
      globalPredictor[i] = 1;
    choice = malloc((1 << ghistoryBits) * sizeof *choice);
    for (int i = 0; i < (1 << ghistoryBits); i++)
      choice[i] = 1;

    lhistoryRegs = malloc((1 << pcIndexBits) * sizeof lhistoryRegs);
    for (int i = 0; i < (1 << pcIndexBits); i++)
      lhistoryRegs[i] = 0;
    localPredictor = malloc((1 << lhistoryBits) * sizeof localPredictor);
    for (int i = 0; i < (1 << lhistoryBits); i++)
      localPredictor[i] = 1;

    break;
  case CUSTOM:
    threshold = 1.25 * W_H + 14;
    for (int i = 0; i < W_LEN; i++)
      for (int j = 0; j < W_H; j++)
        W[i][j] = 0;
    for (int i = 0; i < W_H; i++)
      ghistory[i] = -1;
  default:
    break;
  }
}

uint32_t clip(uint32_t reg, int bits)
{
  return reg & ((1 << bits) - 1);
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  // TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  int ghis;
  uint32_t index = hash(pc);
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return globalPredictor[clip(pc ^ ghistoryReg, ghistoryBits)] >= 2 ? TAKEN : NOTTAKEN;
  case TOURNAMENT:
    ghis = clip(ghistoryReg, ghistoryBits);
    gpred = globalPredictor[ghis] >= 2 ? TAKEN : NOTTAKEN;
    lpred = localPredictor[lhistoryRegs[clip(pc, pcIndexBits)]] >= 2 ? TAKEN : NOTTAKEN;
    return choice[ghis] <= 1 ? gpred : lpred;
  case CUSTOM:
    return forward(pc);
  default:
    return NOTTAKEN;
  }
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  // TODO: Implement Predictor training
  //
  int index = 0;
  int ghis = 0;
  int8_t pout = outcome == TAKEN ? 1 : -1;
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    index = clip(pc ^ ghistoryReg, ghistoryBits);
    if (outcome == TAKEN && globalPredictor[index] < 3)
      globalPredictor[index] += 1;
    if (outcome == NOTTAKEN && globalPredictor[index] > 0)
      globalPredictor[index] -= 1;
    ghistoryReg = clip((ghistoryReg << 1) + outcome, ghistoryBits);
    break;
  case TOURNAMENT:
    index = clip(pc, pcIndexBits);
    ghis = clip(ghistoryReg, ghistoryBits);
    if (gpred != lpred)
    {
      if (outcome == lpred)
      {
        if (choice[ghis] < 3)
          choice[ghis] += 1;
      }
      else if (choice[ghis] > 0)
        choice[ghis] -= 1;
    }

    if (outcome == TAKEN)
    {
      if (globalPredictor[ghis] < 3)
        globalPredictor[ghis] += 1;
      if (localPredictor[lhistoryRegs[index]] < 3)
        localPredictor[lhistoryRegs[index]] += 1;
    }
    else
    {
      if (globalPredictor[ghis] > 0)
        globalPredictor[ghis] -= 1;
      if (localPredictor[lhistoryRegs[index]] > 0)
        localPredictor[lhistoryRegs[index]] -= 1;
    }

    ghistoryReg = clip((ghistoryReg << 1) + outcome, ghistoryBits);
    lhistoryRegs[index] = clip((lhistoryRegs[index] << 1) + outcome, lhistoryBits);
    break;
  case CUSTOM:
    index = hash(pc);

    if ((_hot != outcome) || train)
    {
      backward(&(W[index][0]), pout);
      for (int i = 1; i < W_H; i++)
        backward(&(W[index][i]), (pout == ghistory[i - 1]));
    }

    for (int i = W_H - 1; i > 0; i--)
      ghistory[i] = ghistory[i - 1];
    ghistory[0] = pout;
  default:
    break;
  }
}