/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef CODETORRENTUTILITY_H_
#define CODETORRENTUTILITY_H_

#include "codetorrent.h"
#include "networkcoding/blocky/src/blockypacket.h"
class CodeTorrentUtility {
public:
    CodeTorrentUtility();
    virtual ~CodeTorrentUtility();
    void freeCodedBlock(BlockyPacket** codedBlockPtr);
};

#endif /* CODETORRENTUTILITY_H_ */
