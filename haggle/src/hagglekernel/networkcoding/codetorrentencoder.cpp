/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Yu-Ting Yu (yty)
 */

#include "codetorrentencoder.h"
#include "Trace.h"

codetorrentencoder::codetorrentencoder(const char* _file,size_t _blockSize, long fileSize) {
    double doubleFileSize = static_cast<double>(fileSize);
    double doubleBlockSize = static_cast<double>(_blockSize);

    double doublenumBlocksPerGen = ceil(doubleFileSize / doubleBlockSize);
    int numBlocksPerGen = static_cast<int>(doublenumBlocksPerGen);
    
    //printf("fieldsize=%d numblockspergen=%d blocksize=%d filepath=%s filesize=%d \n", NETWORKCODING_FIELDSIZE,
    //        numBlocksPerGen, _blockSize, _file, fileSize);

    CodeTorrent* codeTorrentServer = new CodeTorrent(NETWORKCODING_FIELDSIZE, numBlocksPerGen, _blockSize,
						     _file,fileSize,true);
    this->codetorrent = codeTorrentServer;

}

codetorrentencoder::codetorrentencoder(CodeTorrent* codetorrent) {
    this->codetorrent = codetorrent;
}

codetorrentencoder::~codetorrentencoder() {
    if(this->codetorrent) {
        delete this->codetorrent;
    }
}

/*BlockPtr codetorrentencoder::LoadSingleBlock(int blockIdInGen){
	return this->codetorrent->LoadSingleBlock(0, blockIdInGen);
}*/

CodedBlockPtr codetorrentencoder::encode() {
    return this->codetorrent->Encode(0);
}

/*CodedBlockPtr codetorrentencoder::encodeSingleBlock(int blockIdInGen, BlockPtr fragData, int fragBlockSize) {
	//printf("encoder init=%d",this->init);
	//
	printf("encodeSingleBlock: blockId-%d\n", blockIdInGen);
    return this->codetorrent->EncodeSingleBlock(0, blockIdInGen, fragData, fragBlockSize);
}*/

int codetorrentencoder::getNumberOfBlocksPerGen() {
    return this->codetorrent->GetNumBlocksGen(0);
}
