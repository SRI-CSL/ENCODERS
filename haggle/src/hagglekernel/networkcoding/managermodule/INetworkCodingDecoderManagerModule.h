/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef INETWORKCODINGDECODERMANAGERMODULE_H_
#define INETWORKCODINGDECODERMANAGERMODULE_H_

#include "networkcoding/concurrent/NetworkCodingDecodingTask.h"

class INetworkCodingDecoderManagerModule {
public:
    virtual bool addTask(NetworkCodingDecodingTaskRef networkCodingDecodingTask) = 0;

    //runnable in managermodule defines a start, so we delegate to start for asynchronous
    virtual bool startup() = 0;

    virtual bool shutdownAndClose() = 0;

    virtual ~INetworkCodingDecoderManagerModule() {};
};

#endif /* INETWORKCODINGDECODERMANAGERMODULE_H_ */
