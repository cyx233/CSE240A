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
int ghistoryReg = 0;
int lhistoryReg = 0;
int *globalPredictTable;
int **localPredictTable;

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
    globalPredictTable = malloc((1 << ghistoryBits) * sizeof *globalPredictTable);
    for (int i = 0; i < (1 << ghistoryBits); i++)
      globalPredictTable[i] = NOTTAKEN;
    break;
  case TOURNAMENT:
    globalPredictTable = malloc((1 << ghistoryBits) * sizeof *globalPredictTable);
    for (int i = 0; i < (1 << ghistoryBits); i++)
      globalPredictTable[i] = 0;

    localPredictTable = malloc((1 << lhistoryBits) * sizeof *localPredictTable);
    for (int i = 0; i < (1 << lhistoryBits); i++)
    {
      localPredictTable[i] = malloc((1 << pcIndexBits) * sizeof *localPredictTable[i]);
      for (int j = 0; j < (1 << pcIndexBits); j++)
        localPredictTable[i][j] = 2;
    }
    break;
  case CUSTOM:
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
  default:
    break;
  }
}
