/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef CODETORRENTDECODER_H_
#define CODETORRENTDECODER_H_

#include <libcpphaggle/String.h>
#include "codetorrent.h"
#include "NetworkCodingConstants.h"

class codetorrentdecoder {
public:
    codetorrentdecoder(size_t fileSize,const char* outputFile,size_t blocksize);
	codetorrentdecoder(CodeTorrent* codetorrent);
	virtual ~codetorrentdecoder();

	/*
	 * returns if block was innovative
	 */
	bool store_block(int gen, CoeffsPtr coeffs, BlockPtr sums);

	bool decode();

	bool isDownloadCompleted();

	int getNumberOfBlocksPerGen();

    const string& getFilepath() const {
        return filepath;
    }

private:
	CodeTorrent* codetorrent;
	string filepath;
};

#endif /* CODETORRENTDECODER_H_ */
