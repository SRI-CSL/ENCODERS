/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Yu-Ting Yu (yty)
 */

#ifndef CODETORRENTENCODER_H_
#define CODETORRENTENCODER_H_

#include "codetorrent.h"
#include "NetworkCodingConstants.h"

class codetorrentencoder {
private:
	CodeTorrent* codetorrent;

public:
	codetorrentencoder(const char* _file,size_t _blockSize, long fileSize);
	codetorrentencoder(CodeTorrent* codetorrent);
	~codetorrentencoder();
	CodedBlockPtr encode();

	//CodedBlockPtr encodeSingleBlock(int blockIdInGen, BlockPtr fragData, int fragBlockSize);
	//BlockPtr LoadSingleBlock(int blockIdInGen);

	int getNumberOfBlocksPerGen();
};

#endif /* CODETORRENTENCODER_H_ */
