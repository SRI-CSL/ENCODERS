/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef INETWORKCODINGENCODERMANAGERMODULE_H_
#define INETWORKCODINGENCODERMANAGERMODULE_H_

#include "networkcoding/concurrent/encoder/NetworkCodingEncoderTask.h"

class INetworkCodingEncoderManagerModule {
public:
    int minEncoderDelayBase, minEncoderDelayLinear, minEncoderDelaySquare; // MOS

    virtual bool addTask(
            NetworkCodingEncoderTaskRef networkCodingEncoderTask) = 0;

    virtual bool startup() = 0;

    virtual bool shutdownAndClose() = 0;

    virtual ~INetworkCodingEncoderManagerModule() {};
};

#endif /* INETWORKCODINGENCODERMANAGERMODULE_H_ */
