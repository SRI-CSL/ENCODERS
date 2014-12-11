/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef NETWORKCODINGDECODERMANAGERMODULEPROCESSOR_H_
#define NETWORKCODINGDECODERMANAGERMODULEPROCESSOR_H_

#include "HaggleKernel.h"
#include "networkcoding/service/NetworkCodingDecoderService.h"
#include "networkcoding/concurrent/NetworkCodingDecodingTask.h"

class NetworkCodingDecoderManagerModuleProcessor {
public:
    NetworkCodingDecoderManagerModuleProcessor(
            NetworkCodingDecoderService* _networkCodingDecoderService,
	    NetworkCodingConfiguration* _networkCodingConfiguration,
	    HaggleKernel* _haggleKernel);
    ~NetworkCodingDecoderManagerModuleProcessor();

    void decode(NetworkCodingDecodingTaskRef networkCodingDecodingTask);
private:
    NetworkCodingDecoderService* networkCodingDecoderService;
    NetworkCodingConfiguration* networkCodingConfiguration;
    HaggleKernel* haggleKernel;
};

#endif /* NETWORKCODINGDECODERMANAGERMODULEPROCESSOR_H_ */
