/*
* Copyright (c) 2006-2013,  University of California, Los Angeles
* Coded by Joe Yeh/Uichin Lee
* Modified and extended by Seunghoon Lee/Sung Chul Choi/ Yu-Ting Yu
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

#ifndef __CODETORRENT_H__
#define __CODETORRENT_H__

#include "nc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <math.h>
#include <stdlib.h>

// WIN32: http://sourceware.org/pthreads-win32/
// Linux/Unix: run gcc with -lpthread command

#define CT_SERVER 0
#define CT_CLIENT 1

#define MAX_FILE_NAME_LEN 256
#define MAX_FILE_STORE 10 // # of files to share or download
#define PRINT_RANK	50
#define PRINT_BLK	15

typedef struct _CT_header {
	int gen; // generation
} CT_header;

// file info struct
typedef struct _CT_FileInfo {
	int filesize; // file size
	int blocksize; // block size
	unsigned long num_blocks_per_gen; // number
	char filename[MAX_FILE_NAME_LEN]; // file name

} CT_FileInfo;

class CodeTorrent {

public:

	////////////////////////////
	// constructors

	CodeTorrent() {
	}
	;

	// Server (Data source)
	CodeTorrent(int in_field_size, int in_num_blocks_per_gen, int in_block_size,
		    const char *in_filename, int in_file_size, bool dummy);

	// Client (MOS - better use different subclasses)
	CodeTorrent(int in_field_size, int in_num_blocks_per_gen, int in_block_size,
		    const char *in_filename, int in_file_size);

	~CodeTorrent();

	////////////////////////////
	// accessors

	inline char* GetFileName() {
		return filename;
	}
	inline int GetFileSize() {
		return file_size;
	}
	inline int GetFieldSize() {
		return field_size;
	}
	inline int GetBlockSize() {
		return block_size;
	}
	inline int GetNumBlocks() {
		return num_blocks;
	}
	inline int GetNumGens() {
		return num_gens;
	}

	inline int* GetNumBlocksGen() {
		return num_blocks_gen;
	}
	inline int GetNumBlocksGen(int in) {
		return num_blocks_gen[in];
	}
	inline int GetGenCompleted() {
		return gen_completed;
	}

	inline int* GetRankVec() {
		if (identity == CT_SERVER)
			return num_blocks_gen;

		return rank_vec;
	}
	inline int* GetRankBufferVec() {
		return rank_vec_in_buf;
	}

	inline bool DownloadCompleted() {
		if (identity == CT_SERVER)
			return true;
		return GetGenCompleted() == GetNumGens();
	}

	inline int GetIdentity() {
		return identity;
	}

	////////////////////////////
	// modifiers

	inline int IncrementGenCompleted() {
		return ++gen_completed;
	}

	////////////////////////////
	// block memory operations

	BlockPtr AllocBlock(int in_block_size);
	void FreeBlock(BlockPtr blk);

	CoeffsPtr AllocCoeffs(int numblks);
	void FreeCoeffs(CoeffsPtr blk);

	CodedBlockPtr AllocCodedBlock(int numblks, int blksize);
	CodedBlockPtr CopyCodedBlock(CodedBlockPtr ptr);
	void FreeCodedBlock(CodedBlockPtr ptr);

	CodedBlockPtr EmptyCodedBlock() {
		return AllocCodedBlock(num_blocks_gen[0], block_size);
	} // NOTE: Never call this before initializing it!

	////////////////////////////
	// data block operations
	//BlockPtr LoadSingleBlock(int gen, int blockIdInGen);
	//CodedBlockPtr EncodeSingleBlock(int gen, int blockIdInGen, BlockPtr fragBlockData, int fragDataSize); // encode a single block


	CodedBlockPtr Encode(int gen); // encode a block from generation "gen"			NOTE: server only
	CodedBlockPtr ReEncode(int gen); // re-encode a block from generation "gen"		NOTE: client only
									 // return NULL pointer if failed (e.g. no data in buffer)
	bool StoreBlock(CodedBlockPtr in); // store a new block into the buffer			NOTE: client only

	bool Decode();

	////////////////////////////
	// printing operations

	void PrintBlocks(int gen); // print things in data			NOTE: server only
	void PrintDecoded(); // print things in m_data		NOTE: client only
	void PrintFileInfo(); // print detailed information about the file
	void LoadFile(int gen); // load gen-th generation of the file to memory

	//////////////////////////////
	//
	inline int GetNumFiles() {
		return file_counter;
	}

private:

	////////////////////////////
	// private methods

	// Server
	void InitServer(int in_field_size, int in_num_blocks_per_gen, int in_block_size,
			const char *in_filename, int in_file_size,
			bool in_is_sim);

	// Client
	void InitClient(int in_field_size, int in_num_blocks_per_gen, int in_block_size,
			const char *in_filename, int in_file_size,
			bool in_is_sim);

	// File loading
	void LoadFileInfo(int in_num_blocks_per_gen); // load file info and determine parameters
	//void LoadFile(int gen);			// load gen-th generation of the file to memory :  moved to public area
	unsigned char NthSymbol(unsigned char *buf, int fsize, int at);

	// Decoding and writing
	bool DecodeGen(int gen); // decode data for each generation
	void WriteFile(); // NOTE: obsolete?
	bool WriteFile(int gen); // Write data into a file for each generation

	// File buffer related functions
	void PushGenBuf(CodedBlockPtr in); // Store a block into the corresponding file buffer
	CodedBlockPtr ReadGenBuf(int gen, int k); // Read k-th block in the gen-th file buffer
	void FlushBuf(); // Save some buffer space by pushing some blocks into files

	// clean up methods
	void CleanMHelpful();
	void CleanMGData();
	void CleanData();
	void CleanBuffer();

	void CleanTempFiles();

	////////////////////////////
	// member variables

	FILE *fp; // file (for reading purposes)
	FILE *fp_write; // file (for writing purposes)
	char filename[MAX_FILE_NAME_LEN]; // filename
	char out_filename[MAX_FILE_NAME_LEN]; // filename - decoded file <- only for client.

	int file_counter; // # of files
	int file_size; // file size in bytes
	int field_size; // field size
	int num_blocks; // number of blocks
	int block_size; // block size
	int num_gens; // number of generations
	int *num_blocks_gen; // number of blocks in a generation	(NOTE: it's vector!)
	bool is_sim; // true if in simulation phase

	int gen_completed; // for client; number of generations completed
	int identity; // CT_CLIENT or CT_SERVER

	int gen_in_memory; // which generation is loaded in m_gdata?

    // CBMEN, HL - Only use this when we need
	//CodedBlockPtr cb; // coded block buffer

	NC *nc; // network coding module

	// storages
	int buffer_size; // size of a buffer
	std::vector<CodedBlockPtr> buf; // memory buffer (I call this "memory" buffer to distinguish from "file buffers")
	std::vector<BlockPtr> data; // actual data													NOTE: only for server (or seed)

	// matrices
	unsigned char **m_gdata; // for actual data, only one generation (formerly m_d)			NOTE: allocate this when decoding
	unsigned char ***m_helpful; // for helpfulness checking	(formerly m_h)						NOTE: only for client

	// information
	int *rank_vec; // vector of ranks											NOTE: not really useful for server
	int *rank_vec_in_buf; // same thing, but only counting the ones in the buffer		NOTE: not really useful for server

};

#endif

