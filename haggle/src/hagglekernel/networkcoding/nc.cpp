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

#include "nc.h"

#include <stdio.h>

long NC_GetTimeDifference(struct timeval tv_beg, struct timeval tv_end) {
	return ((tv_end.tv_usec - tv_beg.tv_usec) + (tv_end.tv_sec - tv_beg.tv_sec) * 1000000);
}

// initial condition: out must have all its header fields filled in (gen, num_blocks_gen, block_size)
void NC::EncodeBlock(std::vector<BlockPtr> &data, CodedBlockPtr out) {

	assert( out != NULL);

	struct timeval ct_encode_beg, ct_encode_end;
	int i, j;
	int gen = out->gen;
	int num_blocks_gen = out->num_blocks_gen;
	int block_size = out->block_size;

	memset(out->coeffs, 0, num_blocks_gen);
	memset(out->sums, 0, block_size);

	int field_limit = (int) 1 << (int) field_size;
	//int start_index = gen * num_blocks_gen;
	int start_index = 0; //gen * num_blocks_gen;

	//printf("field limit |%d|\n", field_limit);

	for (i = 0; i < num_blocks_gen; ++i) {

		// generate random coefficients 
		// coefficients are drawn uniformly from {1, ..., 2^fsize}
		while (!(out->coeffs[i] = (unsigned char) (rand() % field_limit)))
			;

		// generate coded symbols = coeff(0)*b_{i,1} + coeff(1)*b_{i,2} + ... + coeff(m_num_blocks-1)*b_{i,m_numblocks-1}
		for (j = 0; j < (is_sim ? 1 : block_size); ++j) {
		        unsigned char x = data[start_index + i][j];
			unsigned char y = out->coeffs[i];
			unsigned char m = Mul(x, y, field_size);
			unsigned char z = out->sums[j];
			unsigned char a = Add(m, z, field_size);
			out->sums[j] = a;
		}
	}
}


void NC::EncodeSingleBlock(std::vector<BlockPtr> &data, CodedBlockPtr out, int blockIdInGen) {

	assert( out != NULL);

//	struct timeval ct_encode_beg, ct_encode_end;
	int i, j;
	int gen = out->gen;
	int num_blocks_gen = out->num_blocks_gen;
	int block_size = out->block_size;

	memset(out->coeffs, 0, num_blocks_gen);
	memset(out->sums, 0, block_size);

	int field_limit = (int) 1 << (int) field_size;
	//int start_index = gen * num_blocks_gen;
	int start_index = 0; //gen * num_blocks_gen;

	//printf("field limit |%d|\n", field_limit);

#ifdef WIN32
	//srand((unsigned int)GetTickCount());
#else
	//srand(time(NULL));
#endif


	for (i = 0; i < num_blocks_gen; ++i) {

		// generate random coefficients 
		// coefficients are drawn uniformly from {1, ..., 2^fsize}
		//while (!(out->coeffs[i] = (unsigned char) (rand() % field_limit)))
		//	;
		if(i == blockIdInGen) {
			out->coeffs[i] = (unsigned char) (1);
		}
		else{
			out->coeffs[i] = (unsigned char) (0);
		}

		//printf("NC-EncodeSingleBlock-out->coeffs[%d]: %d\n", i, out->coeffs[i]);

		// generate coded symbols = coeff(0)*b_{i,1} + coeff(1)*b_{i,2} + ... + coeff(m_num_blocks-1)*b_{i,m_numblocks-1}
		for (j = 0; j < (is_sim ? 1 : block_size); ++j) {
			out->sums[j] = Add(Mul(data[start_index + i][j], out->coeffs[i], field_size), out->sums[j], field_size);
		}
	}
}


// initial condition: out must have all its header fields filled in (gen, num_blocks_gen, block_size)
bool NC::ReEncodeBlock(std::vector<CodedBlockPtr> &buffer, CodedBlock *out) {

	assert( out != NULL);

	int i, j;
	int gen = out->gen;
	int num_blocks_gen = out->num_blocks_gen;
	int block_size = out->block_size;

	/*
	 if( !buffer[gen].size() )
	 return;
	 */

	if (!GetRank(&buffer))
		return false;

	memset(out->coeffs, 0, num_blocks_gen);
	memset(out->sums, 0, (is_sim ? 1 : block_size));

	unsigned char *randCoeffs = new unsigned char[num_blocks_gen];

	// generate randomized coefficients 
	// coefficients are drawn uniform randomly from {1, ..., 2^fsize}

	int field_limit = (int) 1 << (int) field_size;

	for (i = 0; i < GetRank(&buffer); ++i)
		while (!(randCoeffs[i] = (unsigned char) (rand() % field_limit)))
			;

	// generate a coded piece from a set of coded pieces. { c_0, ... c_{GetRank()-1} }
	// c_i = {COEFFs, SUMs(i.e., coded symbols)} in the buffer (coded piece buffer in NC)
	// re-encoded piece = a_0*c_0 + a_1*c_1 + ... + a_{GetRank()-1}*c_{GetRank()-1}
	// where a_0, a_1, ..., a_{GetRank()} are random coefficients in randCoeffs[i]

	for (i = 0; i < GetRank(&buffer); ++i) {

		for (j = 0; j < num_blocks_gen; ++j) {

			out->coeffs[j] = gf->Add(gf->Mul(buffer[i]->coeffs[j], randCoeffs[i], field_size), out->coeffs[j],
					field_size);
		}
		for (j = 0; j < (is_sim ? 1 : block_size); ++j) {

			out->sums[j] = gf->Add(gf->Mul(buffer[i]->sums[j], randCoeffs[i], field_size), out->sums[j], field_size);
		}
	}

	delete[] randCoeffs;
	return true;
}

// check if the coded block is innovative
bool NC::IsHelpful(int *rank_vec, unsigned char ***m_helpful, CodedBlockPtr in) {

	int i, k;

	int gen = in->gen;
	int num_blocks_gen = in->num_blocks_gen;
	int block_size = in->block_size;

	bool isInnovative = false;
	int firstNonZeroIdx;


	//printf("rank_vec[gen] |%d|\n", rank_vec[gen]);
	if (rank_vec[gen] == num_blocks_gen)
		return false;

    unsigned char *new_vec = NULL; // incoming vector
	new_vec = new unsigned char[num_blocks_gen];
	memcpy(new_vec, in->coeffs, num_blocks_gen);

	for (i = 0; i < num_blocks_gen; i++) {
		//printf("new_vec[%d]=%d\n", i, new_vec[i]);

		// if there exists a row vector 
		//    and also there is a non-zero element of an incoming vector.
		//printf("m_helpful[gen][i][i] |%d| new_vec[i] |%d|\n", m_helpful[gen][i][i], new_vec[i]);
		if (m_helpful[gen][i][i] != 0 && new_vec[i] != 0) {
			// just the encoding vector
			for (k = num_blocks_gen - 1; k >= i; k--) {

				unsigned char mul = gf->Mul(new_vec[i], m_helpful[gen][i][k], field_size);
//				printf("mul=|%u|\n",mul);
//				unsigned char div = gf->Div(mul, m_helpful[gen][i][i], field_size);
//				printf("div=|%u|\n",div);
//				new_vec[k] = gf->Sub(new_vec[k], div, field_size);
//				printf("newvec=|%u|\n", new_vec[k]);

                new_vec[k] = gf->Sub(new_vec[k],
                                gf->Div(gf->Mul(new_vec[i], m_helpful[gen][i][k], field_size), m_helpful[gen][i][i],
                                                field_size), field_size);

			}

			continue;
		}

		// found a column!
		if (new_vec[i] != 0) {
			isInnovative = true;
			firstNonZeroIdx = i;
			break;
		}
	}

	if (!isInnovative) {
		delete[] new_vec;
		return false;
	}

	// put the innovative vector into the corresponding row.
	for (k = 0; k < num_blocks_gen; k++) {
		m_helpful[gen][firstNonZeroIdx][k] = new_vec[k];
	}

	delete[] new_vec;

	return true;
}

// decode
bool NC::Decode(std::vector<CodedBlockPtr> *buffer, unsigned char ***m_data, int num_gens) {

	int i, j, num_blocks_gen, block_size;

	// NOTE: create m_upper here
	unsigned char ***m_upper;

	m_upper = new unsigned char **[num_gens];

	for (i = 0; i < num_gens; ++i) {

		// WARNING: we require there's at least one code block in each buffer!
		// NOTE: we obtain num_blocks_gen for each generation in case it's variable

		num_blocks_gen = buffer[i][0]->num_blocks_gen;
		block_size = buffer[i][0]->block_size;

		m_upper[i] = new unsigned char*[num_blocks_gen];

		for (j = 0; j < num_blocks_gen; ++j) {

			m_upper[i][j] = new unsigned char[num_blocks_gen + (is_sim ? 1 : block_size)];
			memset(m_upper[i][j], 0, num_blocks_gen + (is_sim ? 1 : block_size));
		}
	}

	// actual decoding's done here
	for (i = 0; i < num_gens; i++) {

		num_blocks_gen = buffer[i][0]->num_blocks_gen;
		block_size = buffer[i][0]->block_size;

		for (j = 0; j < num_blocks_gen; j++) {

			IncrementalDecode(m_upper, buffer[i][j]);
		}
		BackSubstitution(buffer, m_upper, m_data, i);
	}

	// NOTE: delete m_upper
	for (i = 0; i < num_gens; ++i) {

		num_blocks_gen = buffer[i][0]->num_blocks_gen;
		block_size = buffer[i][0]->block_size;

		for (j = 0; j < num_blocks_gen; ++j) {

			delete[] (m_upper[i][j]);
		}

	}

	return true;
}

bool NC::Decode(std::vector<CodedBlockPtr> buffer, unsigned char **m_data, int gen) {

	int j, num_blocks_gen, block_size;

	// NOTE: create m_upper here
	unsigned char **m_upper;

	//m_upper = new unsigned char *[num_gens];

	// WARNING: we require there's at least one code block in each buffer!
	// NOTE: we obtain num_blocks_gen for each generation in case it's variable

	num_blocks_gen = buffer[0]->num_blocks_gen;
	block_size = buffer[0]->block_size;

	m_upper = new unsigned char*[num_blocks_gen];

	for (j = 0; j < num_blocks_gen; ++j) {
		m_upper[j] = new unsigned char[num_blocks_gen + (is_sim ? 1 : block_size)];
		memset(m_upper[j], 0, num_blocks_gen + (is_sim ? 1 : block_size));
	}

	// actual decoding's done here

	for (j = 0; j < num_blocks_gen; j++) {

		bool isInnovative = IncrementalDecode(m_upper, buffer[j]);
		if(!isInnovative) {
			return false;
		}
	}
	bool isBackSubstitute = BackSubstitution(buffer, m_upper, m_data);
	if(!isBackSubstitute) {
		return false;
	}

	// NOTE: delete m_upper here

	for (j = 0; j < num_blocks_gen; ++j) {

		delete[] (m_upper[j]);
	}

	delete[] m_upper;

	return true;
}

////////////////////////////
// private functions

// Perform an incremental decoding. This takes O(n^2)
// Note: Given AX=B, check whether an incoming vector increases its rank
// of matrix A (a triangular "singular" matrix)
bool NC::IncrementalDecode(unsigned char ***m_upper, CodedBlockPtr in) {

	int i, k;

	bool isInnovative = false;
	int firstNonZeroIdx;

	int gen = in->gen;
	int num_blocks_gen = in->num_blocks_gen;
	int block_size = in->block_size;

	unsigned char *new_vec = NULL; // incoming vector
	new_vec = new unsigned char[num_blocks_gen + (is_sim ? 1 : block_size)];

	// get the new vector.
	memcpy(new_vec, in->coeffs, num_blocks_gen);
	memcpy(new_vec + num_blocks_gen, in->sums, (is_sim ? 1 : block_size));

	for (i = 0; i < num_blocks_gen; i++) {

		// if there exists a row vector 
		//    and also there is a non-zero element of an incoming vector.
		if (m_upper[gen][i][i] != 0 && new_vec[i] != 0) {

			// not only encoding vector but also coded symbols.
			for (k = num_blocks_gen + (is_sim ? 1 : block_size) - 1; k >= i; k--) {

				new_vec[k] = gf->Sub(new_vec[k],
						gf->Div(gf->Mul(new_vec[i], m_upper[gen][i][k], field_size), m_upper[gen][i][i], field_size),
						field_size);
			}
			continue;
		}

		// found a column!
		if (new_vec[i] != 0) {
			isInnovative = true;
			firstNonZeroIdx = i;
			break;
		}
	}

	if (!isInnovative) {
		delete[] new_vec;
		return false;
	}

	// put the innovative vector into the corresponding row.
	for (k = 0; k < num_blocks_gen + (is_sim ? 1 : block_size); k++) {
		m_upper[gen][firstNonZeroIdx][k] = new_vec[k];
	}

	delete[] new_vec;

	return true;
}

bool NC::IncrementalDecode(unsigned char **m_upper, CodedBlockPtr in) {

	int i, k;

	bool isInnovative = false;
	int firstNonZeroIdx;

	//int gen = in->gen;
	int num_blocks_gen = in->num_blocks_gen;
	int block_size = in->block_size;

	unsigned char *new_vec = NULL; // incoming vector
	new_vec = new unsigned char[num_blocks_gen + (is_sim ? 1 : block_size)];

	// get the new vector.
	memcpy(new_vec, in->coeffs, num_blocks_gen);
	memcpy(new_vec + num_blocks_gen, in->sums, (is_sim ? 1 : block_size));

	for (i = 0; i < num_blocks_gen; i++) {
		// if there exists a row vector 
		//    and also there is a non-zero element of an incoming vector.
		if (m_upper[i][i] != 0 && new_vec[i] != 0) {

			// not only encoding vector but also coded symbols.
			for (k = num_blocks_gen + (is_sim ? 1 : block_size) - 1; k >= i; k--) {

				new_vec[k] = gf->Sub(new_vec[k],
						gf->Div(gf->Mul(new_vec[i], m_upper[i][k], field_size), m_upper[i][i], field_size), field_size);
			}
			continue;
		}

		// found a column!
		if (new_vec[i] != 0) {
			isInnovative = true;
			firstNonZeroIdx = i;
			break;
		}
	}

	if (!isInnovative) {
		delete[] new_vec;
		return false;
	}

	// put the innovative vector into the corresponding row.
	for (k = 0; k < num_blocks_gen + (is_sim ? 1 : block_size); k++) {
		m_upper[firstNonZeroIdx][k] = new_vec[k];
	}

	delete[] new_vec;

	return true;
}

bool NC::BackSubstitution(std::vector<CodedBlockPtr> *buffer, unsigned char ***m_upper, unsigned char ***m_data,
		int gen) {

	unsigned char tmp;

	int i, j, k;

	int num_blocks_gen = buffer[gen][0]->num_blocks_gen;
	int block_size = buffer[gen][0]->block_size;

	if (GetRank(buffer, gen) != num_blocks_gen)
		return false;

	for (i = num_blocks_gen - 1; i >= 0; i--) {

		for (j = 0; j < (is_sim ? 1 : block_size); j++) {

			tmp = (unsigned char) 0x0;

			for (k = i + 1; k < num_blocks_gen; k++) {
				tmp = gf->Add(tmp, gf->Mul(m_upper[gen][i][k], m_data[gen][k][j], field_size), field_size);
			}

			m_data[gen][i][j] = gf->Div(gf->Sub(m_upper[gen][i][num_blocks_gen + j], tmp, field_size),
					m_upper[gen][i][i], field_size);
		}
	}

	return true;
}

bool NC::BackSubstitution(std::vector<CodedBlockPtr> buffer, unsigned char **m_upper, unsigned char **m_data) {

	unsigned char tmp;

	int i, j, k;

	int num_blocks_gen = buffer[0]->num_blocks_gen;
	int block_size = buffer[0]->block_size;

	if (GetRank(&buffer) != num_blocks_gen) //check the GetRank() func parameter
		return false;

	for (i = num_blocks_gen - 1; i >= 0; i--) {

		for (j = 0; j < (is_sim ? 1 : block_size); j++) {

			tmp = (unsigned char) 0x0;

			for (k = i + 1; k < num_blocks_gen; k++) {
				tmp = gf->Add(tmp, gf->Mul(m_upper[i][k], m_data[k][j], field_size), field_size);
			}

			m_data[i][j] = gf->Div(gf->Sub(m_upper[i][num_blocks_gen + j], tmp, field_size), m_upper[i][i], field_size);
		}
	}

	return true;
}

unsigned char NC::Add(unsigned char a, unsigned char b, int ff) {
	return a ^ b;
}

// a+b
unsigned char NC::Mul(unsigned char a, unsigned char b, int ff) {
	if (a == 0 || b == 0)
		return 0;
	else
		return ALog[Log[a] + Log[b]];
}

