#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <vector>


#include "basics.h"
#include "bitrankw32int.h"

typedef struct sFTRep {
	  uint listLength;
	  byte nLevels;// byte es char
	  uint n_levels; // insertado nuevo porque no existe char en gpu
	  uint tamCode;
	  uint * levels;
	  uint * levelsIndex;
	  uint * iniLevel;
	  uint * rankLevels;
	  bitRankW32Int * bS;	
	  uint * base;
	  ushort * base_bits; //cambio tipo de dato por necesidades de la gpu
	  //uint * base_bits;
	  uint * tablebase;
	  uint tamtablebase;
	  uint * iteratorIndex;
  	
} FTRep;


// public:
FTRep* createFT(std::vector<int32_t>& list, uint listLength);
uint accessFT(FTRep* listRep, uint param);
//para gpu
uint accessFT_Dac(FTRep* listRep, uint param, bitRankDac* br_d, uint* data, uint* rs);
//--------------------------------------------------------------
//void saveFT(FTRep * listRep, char * filename);
uint* decompressFT(FTRep* listRep, uint n);
//FTRep* loadFT(char * filename);
void destroyFT(FTRep* listRep);
uint memoryUsage(FTRep* rep);
uint memoryUsageGPU(FTRep* rep);
uint nextFT(FTRep* listRep);

