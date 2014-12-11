/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "codetorrentdecoder.h"
#include <Trace.h>

codetorrentdecoder::codetorrentdecoder(size_t fileSize, const char* outputFile,size_t _blockSize) {

    const int fieldsize = NETWORKCODING_FIELDSIZE;
    const int blockSize = _blockSize;

    double doubleFileSize = static_cast<double>(fileSize);
    double doubleBlockSize = static_cast<double>(blockSize);
    double doubleNumBlocksPerGen = ceil(doubleFileSize / doubleBlockSize);
    int numBlocksPerGen = static_cast<int>(doubleNumBlocksPerGen);
    HAGGLE_DBG2("doubleFileSize=%f numblockspergen=%d\n", doubleFileSize, numBlocksPerGen);
    if (numBlocksPerGen == 0) {
        numBlocksPerGen = 1;
    }

    this->filepath = outputFile;
    this->codetorrent = new CodeTorrent(fieldsize, numBlocksPerGen, blockSize, outputFile, fileSize);
}

codetorrentdecoder::codetorrentdecoder(CodeTorrent* codetorrent) {
    HAGGLE_DBG2("calling copy constructor\n");
    this->codetorrent = codetorrent;
}

codetorrentdecoder::~codetorrentdecoder() {
    if(this->codetorrent) {
        delete this->codetorrent;
    }
}

bool codetorrentdecoder::store_block(int gen, CoeffsPtr coeffs, BlockPtr sums) {
    CodedBlock temp_cb;
    temp_cb.gen = gen;
    temp_cb.num_blocks_gen = this->codetorrent->GetNumBlocksGen(gen);
    temp_cb.block_size = this->codetorrent->GetBlockSize();
    temp_cb.coeffs = coeffs;
    temp_cb.sums = sums;

    //printf("[store_block] Trying to store block in gen %d...\n", gen);

    bool isHelpful = this->codetorrent->StoreBlock(&temp_cb);

    //printf("::Currently received::\n| ");

    for (int i = 0; i < this->codetorrent->GetNumGens(); i++) {
        //printf("%2d | ", this->codetorrent->GetRankVec()[i]);
    }

    //printf("\n");

    return isHelpful;
}

bool codetorrentdecoder::decode() {
    HAGGLE_DBG2("calling decode identity=%d\n",this->codetorrent->GetIdentity());
    return this->codetorrent->Decode();
}

bool codetorrentdecoder::isDownloadCompleted() {
    return this->codetorrent->DownloadCompleted();
}

int codetorrentdecoder::getNumberOfBlocksPerGen() {
    return this->codetorrent->GetNumBlocksGen(0);
}
