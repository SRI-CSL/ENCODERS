/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#include "CodeTorrentUtility.h"


CodeTorrentUtility::CodeTorrentUtility() {

}

CodeTorrentUtility::~CodeTorrentUtility() {

}

void CodeTorrentUtility::freeCodedBlock(BlockyPacket** codedBlockPtrPtr) {
    if( codedBlockPtrPtr == NULL || *codedBlockPtrPtr == NULL ) {
        return;
    }

    BlockyPacket* block = *codedBlockPtrPtr;

    if( block->coeffs ) {
        free(block->coeffs);
        block->coeffs = NULL;
    }

    if( block->data ) {
        free(block->data);
        block->data = NULL;
    }

    if( block ) {
        free(block);
        block = NULL;
    }
}
