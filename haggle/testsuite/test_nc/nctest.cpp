/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Yu-Ting Yu (yty)
 */

#include <stdio.h>

#include <math.h>

#include "../../src/hagglekernel/networkcoding/galois.h"
#include "../../src/hagglekernel/networkcoding/nc.h"
#include "../../src/hagglekernel/networkcoding/codetorrent.h"
#include "../../src/hagglekernel/networkcoding/codetorrentdecoder.h"
#include "../../src/hagglekernel/networkcoding/codetorrentencoder.h"
#include "../../src/hagglekernel/LossRateSlidingWindowElement.h"

#define TEST_BLOCKSIZE 2048

void printCodedBlock(CodedBlockPtr codedBlockPtr1) {
    printf("gen |%d|\n", codedBlockPtr1->gen);
    printf("num_blocks_gen |%d|\n", codedBlockPtr1->num_blocks_gen);
    printf("block_size |%d|\n", codedBlockPtr1->block_size);
    printf("CoeffsPtr |%u|\n", codedBlockPtr1->coeffs);
    printf("BlockPtr |%u|\n", codedBlockPtr1->sums);
}

void decode() {

    char filename[MAX_FILE_NAME_LEN] = "/media/data/research/sri/srihaggle/cbmen-encoders/haggle/testsuite/test_nc/josh";

    FILE *file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);

    char content[fileSize];
    memset(content, 0, sizeof(content));
    file = fopen(filename, "rb");
    //fseek(file, 0, SEEK_);
    fread(content, 1, fileSize, file);

    int filesize = sizeof(content);

    const int fieldsize = NETWORKCODING_FIELDSIZE;
    const int blockSize = TEST_BLOCKSIZE;

    double doubleFileSize = static_cast<double>(filesize);
    double doubleBlockSize = static_cast<double>(blockSize);
    double doubleNumBlocksPerGen = ceil(doubleFileSize / doubleBlockSize);
    int numBlocksPerGen = static_cast<int>(doubleNumBlocksPerGen);
    printf("doubleFileSize=|%f| numblockspergen=|%d|\n", doubleFileSize, numBlocksPerGen);
    if (numBlocksPerGen == 0) {
        numBlocksPerGen = 1;
    }

    CodeTorrent* codeTorrentServer = new CodeTorrent(fieldsize, numBlocksPerGen, blockSize, filename,true);
    codetorrentencoder* encoder = new codetorrentencoder(codeTorrentServer);

    CodeTorrent* codetorrenClient = new CodeTorrent(fieldsize, numBlocksPerGen, blockSize,
            "/media/data/research/sri/srihaggle/cbmen-encoders/haggle/testsuite/test_nc/josh.received", filesize);
    codetorrentdecoder* decoder = new codetorrentdecoder(codetorrenClient);

    int counter = 0;
    while (!codetorrenClient->DownloadCompleted()) {
        CodedBlockPtr codedBlockPtr = encoder->encode();
        //codeTorrentServer->PrintBlocks(0);
        //printCodedBlock (codedBlockPtr);
        decoder->store_block(codedBlockPtr->gen, codedBlockPtr->coeffs, codedBlockPtr->sums);
        counter++;
    }
    printf("completed |%d|\n", codetorrenClient->DownloadCompleted());
    bool decoded = codetorrenClient->Decode();
    printf("decoded successfully=|%d|\n", decoded);
    printf("took |%d| iterations\n", counter);

}

void decode2() {


    char filename[MAX_FILE_NAME_LEN] = "/media/data/research/sri/srihaggle/cbmen-encoders/haggle/testsuite/test_nc/josh";
    FILE *file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);


    codetorrentencoder* encoder = new codetorrentencoder(filename,2048,fileSize);

    double doubleFileSize = static_cast<double>(fileSize);
    double doubleBlockSize = static_cast<double>(TEST_BLOCKSIZE);

    double doubleNumBlocksPerGen = ceil(doubleFileSize / doubleBlockSize);
    int numBlocksPerGen = static_cast<int>(doubleNumBlocksPerGen);

    CodeTorrent* codetorrenClient = new CodeTorrent(NETWORKCODING_FIELDSIZE, numBlocksPerGen, TEST_BLOCKSIZE,
            "/media/data/research/sri/srihaggle/cbmen-encoders/haggle/testsuite/test_nc/josh.received", fileSize);
    codetorrentdecoder* decoder = new codetorrentdecoder(codetorrenClient);

    int counter = 0;
    while (!decoder->isDownloadCompleted()) {
        CodedBlockPtr codedBlockPtr = encoder->encode();
        //codeTorrentServer->PrintBlocks(0);
        //printCodedBlock (codedBlockPtr);
        decoder->store_block(codedBlockPtr->gen, codedBlockPtr->coeffs, codedBlockPtr->sums);
        counter++;
    }
    printf("completed |%d|\n", decoder->isDownloadCompleted());
    bool decoded = decoder->decode();
    printf("decoded successfully=|%d|\n", decoded);
    printf("took |%d| iterations\n", counter);

}

void testSlidingWindowElement(){
	printf("testslidingwindowelement\n");
}

int main(int argc, char *argv[]) {

	testSlidingWindowElement();

    char filename[MAX_FILE_NAME_LEN] = "josh";

    FILE *file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);


    codetorrentencoder* encoder = new codetorrentencoder(filename,TEST_BLOCKSIZE,fileSize);

    codetorrentdecoder* decoder = new codetorrentdecoder(fileSize,"josh.received",TEST_BLOCKSIZE);

    int counter = 0;
    while (!decoder->isDownloadCompleted()) {
        CodedBlockPtr codedBlockPtr = encoder->encode();
        //codeTorrentServer->PrintBlocks(0);
        //printCodedBlock (codedBlockPtr);
        decoder->store_block(codedBlockPtr->gen, codedBlockPtr->coeffs, codedBlockPtr->sums);
        counter++;
    }
    printf("completed |%d|\n", decoder->isDownloadCompleted());
    bool decoded = decoder->decode();
    printf("decoded successfully=|%d|\n", decoded);
    printf("took |%d| iterations\n", counter);

}

