/*
 * // Copyright 2009-2020 NTESS. Under the terms
 * // of Contract DE-NA0003525 with NTESS, the U.S.
 * // Government retains certain rights in this software.
 * //
 * // Copyright (c) 2009-2020, NTESS
 * // All rights reserved.
 * //
 * // Portions are copyright of other developers:
 * // See the file CONTRIBUTORS.TXT in the top level directory
 * // the distribution for more information.
 * //
 * // This file is part of the SST software package. For license
 * // information, see the LICENSE file in the top level directory of the
 * // distribution.
 */

#ifndef _LOGIC_PE_H
#define _LOGIC_PE_H

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class LogicProcessingElement : public ProcessingElement
{
public:
    LogicProcessingElement(opType op_binding, uint32_t processor_id, uint32_t queue_depth,
                    LSQueue* lsqueue, SimpleMem*  mem_interface)  :
                    ProcessingElement(op_binding, processor_id, queue_depth,
                    lsqueue, mem_interface)
    {
        pending_op_ = 0;
        //setup up i/o for messages
        char prefix[256];
        sprintf(prefix, "[t=@t][ProcessingElement-%u]: ", processor_id_);
        output_ = new SST::Output(prefix, 0, 0, Output::STDOUT);

        input_queues_= new std::vector< std::queue< LlyrData >* >;
        output_queues_ = new std::vector< std::queue< LlyrData >* >;
    }

    virtual bool doSend()
    {
        uint32_t queueId;
        LlyrData sendVal;
        ProcessingElement* dstPe;

        for(auto it = output_queue_map_.begin() ; it != output_queue_map_.end(); ++it ) {
            queueId = it->first;
            dstPe = it->second;

            if( output_queues_->at(queueId)->size() > 0 ) {
                output_->verbose(CALL_INFO, 0, 0, ">> Sending...%" PRIu32 "-%" PRIu32 " to %" PRIu32 "\n",
                                processor_id_, queueId, dstPe->getProcessorId());

                sendVal = output_queues_->at(queueId)->front();
                output_queues_->at(queueId)->pop();

                dstPe->pushInputQueue(dstPe->getInputQueueId(this->getProcessorId()), sendVal);
            }
        }

        printInputQueue();
        printOutputQueue();

        return true;
    }

    virtual bool doReceive(LlyrData data) {};

    virtual bool doCompute()
    {
        output_->verbose(CALL_INFO, 0, 0, ">> Compute 0x%" PRIx32 " on PE %" PRIu32 "\n", op_binding_, processor_id_ );

        uint64_t intResult = 0x0F;

        std::vector< LlyrData > argList;
        LlyrData retVal;

        printInputQueue();
        printOutputQueue();

        uint32_t num_ready = 0;
        uint32_t num_inputs  = input_queues_->size();

        //check to see if all of the input queues have data
        for( uint32_t i = 0; i < num_inputs; ++i) {
            if( input_queues_->at(i)->size() > 0 ) {
                num_ready = num_ready + 1;
            }
        }

        //if there are values waiting on any of the inputs, this PE could still fire
        if( num_ready < num_inputs && num_ready > 0) {
            pending_op_ = 1;
        } else {
            pending_op_ = 0;
        }

        //if all inputs are available pull from queue and add to arg list
        if( num_ready < num_inputs ) {
            std::cout << "-Inputs " << num_inputs << " Ready " << num_ready <<std::endl;
            return false;
        } else {
            std::cout << "+Inputs " << num_inputs << " Ready " << num_ready <<std::endl;
            for( uint32_t i = 0; i < num_inputs; ++i) {
                argList.push_back(input_queues_->at(i)->front());
                input_queues_->at(i)->pop();
            }
        }

        switch( op_binding_ ) {
            case SLL :
                intResult = argList[0].to_ullong() + argList[1].to_ullong();
                break;
            case SLR :
                intResult = argList[0].to_ullong() - argList[1].to_ullong();
                break;
            case ROL :
                intResult = argList[0].to_ullong() * argList[1].to_ullong();
                break;
            case ROR :
                intResult = argList[0].to_ullong() / argList[1].to_ullong();
                break;
            default :
                output_->verbose(CALL_INFO, 0, 0, "Error: could not find corresponding op.\n");
                exit(-1);
        }

        retVal = LlyrData(intResult);

        std::cout << "intResult = " << intResult << std::endl;
        std::cout << "retVal = " << retVal << std::endl;

        //for now push the result to all output queues
        for( uint32_t i = 0; i < output_queues_->size(); ++i) {
            output_queues_->at(i)->push(retVal);
        }

        printInputQueue();
        printOutputQueue();

        return true;
    }

    //TODO for testing only
    virtual void fakeInit() {};

};

}//SST
}//Llyr

#endif // _LOGIC_PE_H