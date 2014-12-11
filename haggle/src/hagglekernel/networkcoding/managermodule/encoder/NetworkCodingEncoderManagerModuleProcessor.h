/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGENCODERMANAGERMODULEPROCESSOR_H_
#define NETWORKCODINGENCODERMANAGERMODULEPROCESSOR_H_

#include "HaggleKernel.h"
#include "networkcoding/service/NetworkCodingEncoderService.h"
#include "networkcoding/concurrent/encoder/NetworkCodingEncoderTask.h"

class NetworkCodingEncoderManagerModuleProcessor {
public:
    NetworkCodingEncoderManagerModuleProcessor(NetworkCodingEncoderService* _networkCodingEncoderService,HaggleKernel* _haggleKernel);
    ~NetworkCodingEncoderManagerModuleProcessor();

    void encode(NetworkCodingEncoderTaskRef networkCodingEncoderTask);
private:
    NetworkCodingEncoderService* networkCodingEncoderService;
    HaggleKernel* haggleKernel;
};

#endif /* NETWORKCODINGENCODERMANAGERMODULEPROCESSOR_H_ */
