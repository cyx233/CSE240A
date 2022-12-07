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

int ghistoryReg = 0;
uint8_t *globalPredictor;
uint8_t *localPredictor;
int *lhistoryRegs;
uint8_t *choice;

#define INPUT_LEN 427
#define INPUT_H 19
#define MAX_WEIGHT 1 << 7

int16_t W[INPUT_LEN][INPUT_H + 1];
uint8_t ghistory[INPUT_H];
int threshold;
uint8_t _hot = NOTTAKEN;
uint8_t train = 0;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
int hash(uint32_t pc) { return (pc * 19) % INPUT_LEN; }

void backward(int16_t *w, uint8_t same) { *w += same ? (*w < MAX_WEIGHT - 1 ? 1 : 0) : (*w > -MAX_WEIGHT ? -1 : 0); }
int forward(uint32_t pc)
{
  int index = hash(pc);
  int out = W[index][0];

  for (int i = 1; i <= INPUT_H; i++)
  {
    out += ghistory[i - 1] == TAKEN ? W[index][i] : -W[index][i];
  }

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
    globalPredictor = malloc((1 << ghistoryBits) * sizeof *globalPredictor);
    for (int i = 0; i < (1 << ghistoryBits); i++)
      globalPredictor[i] = NOTTAKEN;
    break;
  case TOURNAMENT:
    globalPredictor = malloc((1 << ghistoryBits) * sizeof *globalPredictor);
    for (int i = 0; i < (1 << ghistoryBits); ++i)
      globalPredictor[i] = 2;
    localPredictor = malloc((1 << pcIndexBits) * sizeof localPredictor);
    for (int i = 0; i < (1 << pcIndexBits); ++i)
    {
      localPredictor[i] = 2;
    }
    lhistoryRegs = malloc((1 << lhistoryBits) * sizeof lhistoryRegs);
    for (int i = 0; i < (1 << lhistoryBits); ++i)
      lhistoryRegs[i] = 0;
    choice = malloc((1 << ghistoryBits) * sizeof *choice);
    for (int i = 0; i < (1 << ghistoryBits); ++i)
      choice[i] = 2;
    break;
  case CUSTOM:
    threshold = 1.93 * INPUT_H + 14;
    for (int i = 0; i < INPUT_LEN; i++)
      for (int j = 0; j < INPUT_H; j++)
        W[i][j] = 0;
    for (int i = 0; i < INPUT_H; i++)
      ghistory[i] = 0;
  default:
    break;
  }
}

int clip(int reg, int bits)
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
    return globalPredictor[(pc & ((1 << ghistoryBits) - 1)) ^ ghistoryReg];
  case TOURNAMENT:
    ghis = clip(ghistoryReg, ghistoryBits);
    return choice[ghis] >= 2
               ? globalPredictor[ghis] >= 2 ? TAKEN : NOTTAKEN
           : localPredictor[lhistoryRegs[clip(pc, pcIndexBits)]] >= 2 ? TAKEN
                                                                      : NOTTAKEN;
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
  int pout = outcome == TAKEN ? 1 : -1;
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    globalPredictor[(pc & ((1 << ghistoryBits) - 1)) ^ ghistoryReg] = outcome;
    ghistoryReg = ((ghistoryReg << 1) + outcome) & ((1 << ghistoryBits) - 1);
    break;
  case TOURNAMENT:
    index = lhistoryRegs[clip(pc, pcIndexBits)];
    ghis = clip(ghistoryReg, ghistoryBits);
    if (choice[ghis] >= 2)
    {
      if (outcome == TAKEN)
      {
        if (globalPredictor[ghis] >= 2)
          choice[ghis] = 3;
        else
          choice[ghis] -= 1;
        if (globalPredictor[ghis] < 3)
          globalPredictor[ghis] += 1;
      }
      else
      {
        if (globalPredictor[ghis] >= 2)
          choice[ghis] -= 1;
        else
          choice[ghis] = 3;
        if (globalPredictor[ghis] > 0)
          globalPredictor[ghis] -= 1;
      }
    }
    else
    {
      if (outcome == TAKEN)
      {
        if (localPredictor[index] >= 2)
          choice[ghis] = 0;
        else
          choice[ghis] += 1;
        if (localPredictor[index] < 3)
          localPredictor[index] += 1;
      }
      else
      {
        if (localPredictor[index] >= 2)
          choice[ghis] += 1;
        else
          choice[ghis] = 0;
        if (localPredictor[index] > 0)
          localPredictor[index] -= 1;
      }
    }

    ghistoryReg = ((ghistoryReg << 1) + outcome) & ((1 << ghistoryBits) - 1);
    lhistoryRegs[clip(pc, pcIndexBits)] = ((lhistoryRegs[clip(pc, pcIndexBits)] << 1) + outcome) & ((1 << lhistoryBits) - 1);
    break;
  case CUSTOM:
    index = hash(pc);

    if ((_hot != outcome) || train)
    {
      backward(&(W[index][0]), outcome);
      for (int i = 1; i <= INPUT_H; i++)
      {
        backward(&(W[index][i]), (outcome == ghistory[i - 1]));
      }
    }

    for (int i = INPUT_H - 1; i > 0; i--)
      ghistory[i] = ghistory[i - 1];
    ghistory[0] = outcome;
  default:
    break;
  }
}