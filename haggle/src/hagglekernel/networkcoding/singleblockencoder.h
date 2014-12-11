/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Yu-Ting Yu (yty)
 */





#ifndef __SINGLEBLOCKENCODER_H__
#define __SINGLEBLOCKENCODER_H__

#include "nc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <math.h>
#include <stdlib.h>

class SingleBlockEncoder {

public:

	SingleBlockEncoder(int _field_size):field_size(_field_size) {
		nc = new NC(field_size, false);
	}
	;

	~SingleBlockEncoder(){
		delete nc;
	}

	BlockPtr AllocBlock(int in_block_size);
	void FreeBlock(BlockPtr blk);

	CoeffsPtr AllocCoeffs(int numblks);
	void FreeCoeffs(CoeffsPtr blk);

	CodedBlockPtr AllocCodedBlock(int numblks, int blksize);
	CodedBlockPtr CopyCodedBlock(CodedBlockPtr ptr);
	void FreeCodedBlock(CodedBlockPtr ptr);

	CodedBlockPtr EncodeSingleBlock(int gen, int blockIdInGen, BlockPtr fragBlockData, int fragDataSize, int numBlockGen); // encode a single block
	
private:

	int field_size; // field size
	//int block_size; // block size

	NC *nc; // network coding module

	// storages
	//std::vector<CodedBlockPtr> buf; // memory buffer (I call this "memory" buffer to distinguish from "file buffers")
};

#endif

