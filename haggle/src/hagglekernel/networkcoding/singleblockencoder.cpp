/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Yu-Ting Yu (yty)
 */



#include <errno.h>
#include "singleblockencoder.h"
//for time-stamp
//#include <sys/timeb.h>
#include <sys/time.h>

#include <Trace.h>

BlockPtr SingleBlockEncoder::AllocBlock(int in_block_size) {

	return (BlockPtr) malloc(in_block_size);
}

void SingleBlockEncoder::FreeBlock(BlockPtr blk) {

	free(blk);
}

CoeffsPtr SingleBlockEncoder::AllocCoeffs(int numblks) {
	return (CoeffsPtr) malloc(numblks);
}

void SingleBlockEncoder::FreeCoeffs(CoeffsPtr blk) {
	free(blk);
}

CodedBlockPtr SingleBlockEncoder::AllocCodedBlock(int numblks, int blksize) {
	CodedBlock *blk = NULL;

	//printf("numblks %d blksize %d\n", numblks, blksize);
	blk = (CodedBlock *) malloc(sizeof(CodedBlock));
	assert(blk);

	blk->coeffs = AllocCoeffs(numblks);
	blk->sums = AllocBlock(blksize);
	blk->block_size = blksize;
	blk->num_blocks_gen = numblks;

	assert(blk->coeffs);
	assert(blk->sums);

	return blk;
}

void SingleBlockEncoder::FreeCodedBlock(CodedBlockPtr ptr) {
	FreeCoeffs(ptr->coeffs);
	FreeBlock(ptr->sums);
	free(ptr);
}

// encode a block from generation "gen"
CodedBlockPtr SingleBlockEncoder::EncodeSingleBlock(int gen, int blockIdInGen, BlockPtr fragBlockData, int fragDataSize, int num_blocks_gen) {
	int block_size = fragDataSize;
//	int buffer_size = 2 * block_size;

	if(fragDataSize != block_size){
		//TODO: Report error		
		return NULL;
	}

	// create a new copy
	CodedBlockPtr cb_to = AllocCodedBlock(num_blocks_gen, block_size);

	cb_to->gen = gen;
	cb_to->num_blocks_gen = num_blocks_gen; 
	cb_to->block_size = block_size;

	//Make a fake data to encode
	std::vector<BlockPtr> fakeData;
	BlockPtr pblk;
	for(int i=0;i<num_blocks_gen;i++){
		pblk = AllocBlock((block_size));
		memset(pblk, 0, (block_size));
		if(i==blockIdInGen){
			memcpy(pblk, fragBlockData, (block_size));
		}
		fakeData.push_back(pblk);
	}

	nc->EncodeSingleBlock(fakeData, cb_to, blockIdInGen);

	//release fake data
	for(int i=0;i<(int) fakeData.size();i++){
		FreeBlock(fakeData[i]);
	}
	fakeData.clear();

	return cb_to;
}

