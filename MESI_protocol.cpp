#include "MESI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MESI_protocol::MESI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    //initial state
    this->state = MESI_CACHE_I;
}

MESI_protocol::~MESI_protocol ()
{    
}

void MESI_protocol::dump (void)
{
    const char *block_states[8] = {"X","I","S","E","M", "IE", "IM", "SM"};
    fprintf (stderr, "MESI_protocol - state: %s\n", block_states[state]);
}

void MESI_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
        case MESI_CACHE_I: do_cache_I (request); break;
        case MESI_CACHE_S: do_cache_S (request); break;
        case MESI_CACHE_E: do_cache_E (request); break;
        case MESI_CACHE_M: do_cache_M (request); break;
        case MESI_CACHE_IE: do_cache_IE (request); break;
        case MESI_CACHE_IM: do_cache_IM (request); break;
        case MESI_CACHE_SM: do_cache_SM (request); break;
    default:
        fatal_error ("Invalid Cache State for MESI Protocol\n");
    }
}

void MESI_protocol::process_fetch_request (Mreq *request)
{
	switch (state) {
        case MESI_CACHE_I: do_fetch_I (request); break;
        case MESI_CACHE_S: do_fetch_S (request); break;
        case MESI_CACHE_E: do_fetch_E (request); break;
        case MESI_CACHE_M: do_fetch_M (request); break;
        case MESI_CACHE_IE: do_fetch_IE (request); break;
        case MESI_CACHE_IM: do_fetch_IM (request); break;
        case MESI_CACHE_SM: do_fetch_SM (request); break;

    default:
    	fatal_error ("Invalid Cache State for MESI Protocol\n");
    }
}

inline void MESI_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        Sim->read_cache_miss++;
        send_GETS(request->addr);
        state = MESI_CACHE_IE;
        Sim->cache_misses ++;
        break;

    case STORE:
        Sim->write_cache_miss++;
        send_GETM(request->addr);
        state = MESI_CACHE_IM;
        Sim->cache_misses ++;
        break;
    
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
        break;
    }

}

inline void MESI_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
    // if hit
        Sim->read_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;

    case STORE:
        Sim->write_cache_miss++;
        send_GETM(request->addr);
        state = MESI_CACHE_SM;
        break;
    
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
        break;
    }

}

inline void MESI_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
    // if hit
        Sim->read_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;

    case STORE:
        Sim->write_cache_hit++;
        send_DATA_to_proc(request->addr); // need to set slient upgrade here
        state = MESI_CACHE_M;
        Sim->silent_upgrades ++;
        //
        send_silent_Up(request->addr);
        break;
    
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
        break;
    }

}

inline void MESI_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD: // ?
    // if hit
        Sim->read_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;

    case STORE:
    // if hit
        Sim->write_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;
    
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
        break;
    }

}

inline void MESI_protocol::do_cache_IE (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error("Should only have one outstanding request per processor!");
        break;

    case STORE:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error("Should only have one outstanding request per processor!");
        break;
    
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
        break;
    }

}

inline void MESI_protocol::do_cache_IM (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error("Should only have one outstanding request per processor!");
        break;
        
    case STORE:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error("Should only have one outstanding request per processor!");
        break;
    
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
        break;
    }

}

inline void MESI_protocol::do_cache_SM (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error("Should only have one outstanding request per processor!");
        break;
        
    case STORE:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error("Should only have one outstanding request per processor!");
        break;
    
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
        break;
    }

}

inline void MESI_protocol::do_fetch_I (Mreq *request)
{
    switch (request->msg) { // actually, directory will not send message to invalid block
    case GETS:
        break;

    case GETM:
        break;

    case DATA:
        break;
    
    case DATA_E:
        break;

    case MREQ_INVALID:
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_fetch_S (Mreq *request)
{
    switch (request->msg) {
    case GETS:
        // nothing changed
        break;

    case GETM:
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_I;
        break;

    case DATA:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: S state shouldn't see this message\n");
        break;
    
    case DATA_E:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: S state shouldn't see this message\n");
        break;
    
    case MREQ_INVALID:
        //send_DATA_on_ptpNetwork(request->addr, request->src_mid);// error
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_I;
    	break;

    case MREQ_INVALID_DS:
        // send a nop
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_I;
        break;

    case MREQ_INVALID_100:
        // send a DATA_100
        send_DATA100_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_I;
        break;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_fetch_E (Mreq *request)
{
    switch (request->msg) {
    case GETS:

        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_S;
        Sim->cache_to_cache_transfers ++;
        break;

    case GETM:

        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_I;
        Sim->cache_to_cache_transfers ++;
        break;

    case DATA:
        break;
    
    case DATA_E:
        break;

    case MREQ_INVALID:	
        send_ACK_on_ptpNetwork(request->msg, request->src_mid);
        state = MESI_CACHE_I;
    	break;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: E state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_fetch_M (Mreq *request)
{
    switch (request->msg) {
    case GETS:
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        //if(Global_Clock == 8170) printf("!!!!!!!src node ID: %d\n", request->src_mid.nodeID);
        state = MESI_CACHE_S;
        Sim->cache_to_cache_transfers ++;
        break;

    case GETM:
        //send_DATA_to_proc(request->addr);
        
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_I;
        Sim->cache_to_cache_transfers ++;
        break;

    case DATA:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
        break;
    
    case DATA_E:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
        break;
    	
    case MREQ_INVALID:
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_I;
    	break;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_fetch_IE (Mreq *request)
{
    switch (request->msg) {
    case GETS:
        this->my_table->pushback_input_port(request);
        break;

    case GETM:
        this->my_table->pushback_input_port(request);
        break;

    case DATA:
        send_DATA_to_proc(request->addr);
        state = MESI_CACHE_S;
        break;
    
    case DATA_E:
        send_DATA_to_proc(request->addr);
        state = MESI_CACHE_E;
        break;

    case MREQ_INVALID:
        //request->print_msg (my_table->moduleID, "ERROR");
        //fatal_error ("Client: IE state shouldn't see this message\n");
        this->my_table->pushback_input_port(request);
    	break;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: IE state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_fetch_IM (Mreq *request)
{
    switch (request->msg) {
    case GETS:
        this->my_table->pushback_input_port(request);
        break;

    case GETM:
        this->my_table->pushback_input_port(request);
        break;

    case DATA:
        
        send_DATA_to_proc(request->addr);
		state = MESI_CACHE_M;
        break;
    
    case DATA_E:
        break;
    	
    case MREQ_INVALID:
        this->my_table->pushback_input_port(request);
    	break;

    case ACK:
        send_DATA_to_proc(request->addr);
        state = MESI_CACHE_M;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_fetch_SM (Mreq *request)
{
    switch (request->msg) {
    case GETS:
        this->my_table->pushback_input_port(request);
        break;

    case GETM:
        this->my_table->pushback_input_port(request);
        break;

    case DATA:
        send_DATA_to_proc(request->addr);
		state = MESI_CACHE_M;
        break;
    
    case DATA_E:
        break;
    	
    case MREQ_INVALID:
        //this->my_table->pushback_input_port(request);
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_IM;
    	break;

    case ACK:
        send_DATA_to_proc(request->addr);
        state = MESI_CACHE_M;
        break;

    case MREQ_INVALID_DS:
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_IM;
        break;

    case MREQ_INVALID_100:
        // send a DATA_100
        send_DATA100_on_ptpNetwork(request->addr, request->src_mid);
        state = MESI_CACHE_IM;
        break;

    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: SM state shouldn't see this message\n");
    }
}

