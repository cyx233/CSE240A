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

//
// TODO: Add your own Branch Predictor data structures here
//
uint8_t ghistoryReg = 0;
uint8_t lhistoryReg = 0;
uint8_t globalPredictTable[1 << 13];
uint8_t localPredictTable[1 << 10][1 << 10];

uint8_t mGPred[1 << 13];
uint8_t mLPred[1024][256];

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
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
    for (int i = 0; i < (1 << ghistoryBits); i++)
      globalPredictTable[i] = NOTTAKEN;
    break;
  case TOURNAMENT:
    for (int i = 0; i < (1 << ghistoryBits); i++)
      globalPredictTable[i] = 0;

    for (int i = 0; i < (1 << lhistoryBits); i++)
    {
      for (int j = 0; j < (1 << pcIndexBits); j++)
        localPredictTable[i][j] = 2;
    }
    break;
  case CUSTOM:
    ghistoryReg = 0;
    for (int i = 0; i < (1 << ghistoryBits); i++)
      globalPredictTable[i] = 0;
    for (int i = 0; i < (1 << lhistoryBits); i++)
    {
      for (int j = 0; j < (1 << pcIndexBits); j++)
        localPredictTable[i][j] = 2;
    }
  default:
    break;
  }
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
  int index = 0;
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return globalPredictTable[(pc & ((1 << ghistoryBits) - 1)) ^ ghistoryReg];
  case TOURNAMENT:
    index = (pc & ((1 << pcIndexBits) - 1)) ^ lhistoryReg;
    return (localPredictTable[index][ghistoryReg] >= 2)
               ? TAKEN
               : (globalPredictTable[ghistoryReg] >= 2 ? TAKEN : NOTTAKEN);
  case CUSTOM:
    // Hybrid GSHARE + TOURNAMENT predictor
    // Use GSHARE predictor for branch instructions with high degree of correlation
    if (globalPredictTable[ghistoryReg] >= 2)
      return globalPredictTable[(pc & ((1 << ghistoryBits) - 1)) ^ ghistoryReg];
    // Otherwise, use TOURNAMENT predictor for branch instructions with low degree of correlation
    else
    {
      index = (pc & ((1 << pcIndexBits) - 1)) ^ lhistoryReg;
      return (localPredictTable[index][ghistoryReg] >= 2)
                 ? TAKEN
                 : (globalPredictTable[ghistoryReg] >= 2 ? TAKEN : NOTTAKEN);
    }
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
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    globalPredictTable[(pc & ((1 << ghistoryBits) - 1)) ^ ghistoryReg] = outcome;
    ghistoryReg = ((ghistoryReg << 1) + outcome) & ((1 << ghistoryBits) - 1);
    break;
  case TOURNAMENT:
    index = (pc & ((1 << pcIndexBits) - 1)) ^ lhistoryReg;
    if (localPredictTable[index][ghistoryReg] >= 2)
      localPredictTable[index][ghistoryReg] += (outcome == TAKEN) ? 1 : -1;
    else
      globalPredictTable[ghistoryReg] += (outcome == TAKEN) ? 1 : -1;
    ghistoryReg = ((ghistoryReg << 1) + outcome) & ((1 << ghistoryBits) - 1);
    lhistoryReg = ((lhistoryReg << 1) + outcome) & ((1 << lhistoryBits) - 1);
    break;
  case CUSTOM:
    // Train GSHARE predictor for branch instructions with high degree of correlation
    if (globalPredictTable[ghistoryReg] >= 2)
    {
      globalPredictTable[(pc & ((1 << ghistoryBits) - 1)) ^ ghistoryReg] = outcome;
      ghistoryReg = ((ghistoryReg << 1) + outcome) & ((1 << ghistoryBits) - 1);
    }
    // Otherwise, train TOURNAMENT predictor for branch instructions with low degree of correlation
    else
    {
      index = (pc & ((1 << pcIndexBits) - 1)) ^ lhistoryReg;
      if (localPredictTable[index][ghistoryReg] >= 2)
        localPredictTable[index][ghistoryReg] -= outcome ? 0 : 1;
      else if (globalPredictTable[ghistoryReg] >= 2)
        globalPredictTable[ghistoryReg] -= outcome ? 0 : 1;
      lhistoryReg = ((lhistoryReg << 1) + outcome) & ((1 << lhistoryBits) - 1);
      ghistoryReg = ((ghistoryReg << 1) + outcome) & ((1 << ghistoryBits) - 1);
    }
    break;
  default:
    break;
  }
}
