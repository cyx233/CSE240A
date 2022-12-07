//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
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
int *lhistoryRegs;
uint8_t *choice;

#define W_HIDDEN 32
#define W_OUT 16
#define PCBIT 32
#define HIS_LEN 32
#define LR 0.05

float W_in[PCBIT + HIS_LEN][W_HIDDEN];
float W_hidden[W_HIDDEN][W_OUT];
float W_out[W_OUT];

float input[PCBIT + HIS_LEN];
float x1[W_HIDDEN];
float x2[W_OUT];

float delta0[PCBIT + HIS_LEN];
float delta1[W_HIDDEN];
float delta2[W_OUT];

float pred, _hot, delta, threshold;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
#define M_PI 3.1415926
int phase = 0;
float gaussrand(float mean, float stddev)
{
  float Z;
  float U, V;

  if (phase == 0)
  {
    U = (rand() + 1.) / (RAND_MAX + 2.);
    V = rand() / (RAND_MAX + 1.);
    Z = sqrt(-2 * log(U)) * sin(2 * M_PI * V);
  }
  else
  {
    Z = sqrt(-2 * log(U)) * cos(2 * M_PI * V);
  }

  phase = 1 - phase;

  return mean + stddev * Z;
}

float sigmoid(float x)
{
  return exp(x) / (1 + exp(x));
}

int forward(uint32_t pc)
{
  for (int i = 0; i < PCBIT; i++)
    input[i] = (pc >> i) & 1;

  for (int i = 0; i < HIS_LEN; i++)
    input[i + PCBIT] = (ghistoryReg >> i) & 1;

  for (int i = 0; i < W_HIDDEN; ++i)
  {
    x1[i] = 0;
    for (int j = 0; j < PCBIT + HIS_LEN; ++j)
      x1[i] += W_in[j][i] * input[j];
  }

  for (int i = 0; i < W_OUT; ++i)
  {
    x2[i] = 0;
    for (int j = 0; j < W_HIDDEN; ++j)
      x2[i] += sigmoid(x1[j]) * W_hidden[j][i];
  }

  for (int i = 0; i < W_OUT; ++i)
    _hot = sigmoid(x2[i]) * W_out[i];

  pred = sigmoid(_hot);
  if (isnan(pred))
  {
    exit(0);
  }
  return pred;
}

void backward(uint8_t outcome)
{
  float loss = 0;
  float temp = 0;

  if (outcome == TAKEN)
    loss = log(pred);
  else
    loss = log(1 - pred);

  delta = -(sigmoid(_hot) - outcome);

  for (int i = 0; i < W_OUT; i++)
  {
    delta2[i] = delta * W_out[i] * temp * (1 - temp);
  }

  for (int i = 0; i < W_HIDDEN; i++)
  {
    temp = sigmoid(x1[i]);
    delta1[i] = 0;
    for (int j = 0; j < W_OUT; j++)
      delta1[i] += delta2[j] * W_hidden[i][j] * temp * (1 - temp);
  }

  for (int i = 0; i < PCBIT + HIS_LEN; i++)
  {
    temp = sigmoid(input[i]);
    delta0[i] = 0;
    for (int j = 0; j < W_HIDDEN; j++)
      delta0[i] += delta1[j] * W_in[i][j] * temp * (1 - temp);
  }
}

void step()
{

  for (int i = 0; i < PCBIT + HIS_LEN; i++)
    for (int j = 0; j < W_HIDDEN; j++)
      W_in[i][j] += LR * delta0[j] * input[i];

  for (int i = 0; i < W_HIDDEN; i++)
    for (int j = 0; j < W_OUT; j++)
      W_hidden[i][j] += LR * delta1[j] * x1[i];

  for (int i = 0; i < W_OUT; i++)
    W_out[i] += LR * delta * x2[i];
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
    for (int i = 0; i < PCBIT + HIS_LEN; i++)
      for (int j = 0; j < W_HIDDEN; j++)
        W_in[i][j] = gaussrand(0, 0.5);

    for (int i = 0; i < W_HIDDEN; i++)
      for (int j = 0; j < W_OUT; j++)
        W_hidden[i][j] = gaussrand(0, 0.5);

    for (int i = 0; i < W_OUT; i++)
      W_out[i] = gaussrand(0, 0.5);

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
    return forward(pc) > threshold ? TAKEN : NOTTAKEN;
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
    backward(outcome);
    step();
    ghistoryReg = ((ghistoryReg << 1) + outcome) & ((1 << ghistoryBits) - 1);
  default:
    break;
  }
}