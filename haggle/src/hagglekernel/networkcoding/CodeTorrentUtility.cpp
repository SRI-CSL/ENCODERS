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

void CodeTorrentUtility::freeCodedBlock(CodedBlockPtr* codedBlockPtrPtr) {
    if( codedBlockPtrPtr == NULL || *codedBlockPtrPtr == NULL ) {
        return;
    }

    CodedBlockPtr block = *codedBlockPtrPtr;

    if( block->coeffs ) {
        free(block->coeffs);
        block->coeffs = NULL;
    }

    if( block->sums ) {
        free(block->sums);
        block->sums = NULL;
    }

    if( block ) {
        free(block);
        block = NULL;
    }
}
