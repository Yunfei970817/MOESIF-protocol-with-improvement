/*
 * messages.cpp
 *
 *  Created on: Apr 8, 2011
 *      Author: japoovey
 */
#include "messages.h"
#include "../sim/mreq.h"

/** This contains the message types as strings.  This must be of the same length
 * and the same order as the enum in messages.h.  This is used to output the message
 * trace.  This is needed to match the validation runs.
 */

const char * Mreq::message_t_str[MREQ_MESSAGE_NUM] = {

    "NOP",

    "LOAD",
    "STORE",

    "GETS",
    "GETM",

    "DATA",
    "DATA_O",
    "DATA_E",
    "DATA_F",
    "DATA_S",
    "DATA_100",

    "Silent_Up",
    "ACK",
    
    "MREQ_INVALID",
    "MREQ_INVALID_DS",
    "MREQ_INVALID_100"
};
