/*
* Copyright (c) 2006-2013,  University of California, Los Angeles
* Coded by Joe Yeh/Uichin Lee
* Modified and extended by Seunghoon Lee/Sung Chul Choi / Yu-Ting Yu
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
* in the documentation and/or other materials provided with the distribution.
* Neither the name of the University of California, Los Angeles nor the names of its contributors 
* may be used to endorse or promote products derived from this software without specific prior written permission.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __NC_H__
#define __NC_H__

// libraries
#include "galois.h"
#include <vector>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
//#include <windows.h>	// for GetTickCount()


#define GF8  256
#define PP8 0435 // n.b. octal value.

typedef unsigned char *BlockPtr;
typedef unsigned char *CoeffsPtr;

// a coded block
typedef struct CodedBlock_ {

	int gen; // generation it belongs to
	int num_blocks_gen; // number of blocks in a block
	int block_size; // size of each block
	CoeffsPtr coeffs; // coefficients
	BlockPtr sums; // sums (the linear combination of data)

} CodedBlock;

typedef CodedBlock* CodedBlockPtr;

// NC module
class NC {

public:

	void Init() {
		int b, j;

		for (j = 0; j < GF; j++) {
			Log[j] = GF - 1;
			ALog[j] = 0;
		}

		b = 1;
		for (j = 0; j < GF - 1; j++) {
			Log[b] = j;
			ALog[j] = b;
			b = b * 2;
			if (b & GF)
				b = (b ^ PP) & (GF - 1);
		}

		for (; j <= (GF - 1) * 2 - 2; j++) {
			ALog[j] = ALog[j % (GF - 1)];
		}

		ALog[(GF - 1) * 2 - 1] = ALog[254];

	}


	////////////////////////////
	// constructor(s)

	// default constructor
	NC(int in_field_size, bool in_is_sim) {

		Init();
		field_size = in_field_size;
		gf = new Galois();
		is_sim = in_is_sim;

	}
	;

	// destructor
	~NC() {

		delete gf;
	}
	;

	////////////////////////////
	// accessors

	////////////////////////////
	// other publiic functions

	// given a buffer and index, find the rank (i.e. # of linearly indep. vectors in the buffer)
	int GetRank(std::vector<CodedBlockPtr> *buffer, int gen) {
		return buffer[gen].size();
	}
	int GetRank(std::vector<CodedBlockPtr> *buffer) {
		return buffer->size();
	} //for each gen

	// encode/re-encode
	void EncodeBlock(std::vector<BlockPtr> &data, CodedBlockPtr out);
	void EncodeSingleBlock(std::vector<BlockPtr> &data, CodedBlockPtr out, int blockIdInGen);
	
	bool ReEncodeBlock(std::vector<CodedBlockPtr> &buffer, CodedBlock* out);

	// check if the coded block is innovative
	//bool IsHelpful(std::vector<CodedBlockPtr> *buffer, unsigned char ***m_helpful, CodedBlockPtr in);	
	bool IsHelpful(int *rank_vec, unsigned char ***m_helpful, CodedBlockPtr in);

	// decode
	bool Decode(std::vector<CodedBlockPtr> *buffer, unsigned char ***m_data, int num_gens);
	bool Decode(std::vector<CodedBlockPtr> buffer, unsigned char **m_data, int gen); //for each gen

	unsigned char Add(unsigned char a, unsigned char b, int ff);
	unsigned char Mul(unsigned char a, unsigned char b, int ff);

private:

	////////////////////////////
	// private functions

	// decoding sub-methods
	bool IncrementalDecode(unsigned char ***m_upper, CodedBlockPtr in);
	bool BackSubstitution(std::vector<CodedBlockPtr> *buffer, unsigned char ***m_upper, unsigned char ***m_data,
			int gen);

	bool IncrementalDecode(unsigned char **m_upper, CodedBlockPtr in);
	bool BackSubstitution(std::vector<CodedBlockPtr> buffer, unsigned char **m_upper, unsigned char **m_data);

	////////////////////////////
	// member variables

	int field_size; // size of the finite field in which this network coding operates
	Galois *gf; // Galois field module
	bool is_sim; // Is this simulation?


	struct timezone nc_tz;
	unsigned char Log[GF8];
	unsigned char ALog[GF8 * 2];
	const static int GF = GF8;
	const static int PP = PP8;
};

#endif
