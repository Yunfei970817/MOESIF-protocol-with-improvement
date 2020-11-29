#include "MOESIF_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOESIF_protocol::MOESIF_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    this->state = MOESIF_CACHE_I;
}

MOESIF_protocol::~MOESIF_protocol ()
{    
}

void MOESIF_protocol::dump (void)
{
    const char *block_states[8] = {"I","S","F","E","O","M", "IM", "IS"};
    fprintf (stderr, "MOESIF_protocol - state: %s\n", block_states[state-1]);
}

void MOESIF_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
    case MOESIF_CACHE_I: do_cache_I(request); break;
    case MOESIF_CACHE_S: do_cache_S(request); break;
    case MOESIF_CACHE_M: do_cache_M(request); break;
    case MOESIF_CACHE_O: do_cache_O(request); break;
    case MOESIF_CACHE_F: do_cache_F(request); break;
    case MOESIF_CACHE_E: do_cache_E(request); break;
    case MOESIF_CACHE_IS: do_cache_IS(request); break;
    case MOESIF_CACHE_IM: do_cache_IM(request); break;
    default:
        fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

void MOESIF_protocol::process_fetch_request (Mreq *request)
{
	switch (state) {
    case MOESIF_CACHE_I: do_fetch_I(request); break;
    case MOESIF_CACHE_S: do_fetch_S(request); break;
    case MOESIF_CACHE_M: do_fetch_M(request); break;
    case MOESIF_CACHE_O: do_fetch_O(request); break;
    case MOESIF_CACHE_F: do_fetch_F(request); break;
    case MOESIF_CACHE_E: do_fetch_E(request); break;
    case MOESIF_CACHE_IS: do_fetch_IS(request); break;
    case MOESIF_CACHE_IM: do_fetch_IM(request); break;
    default:
    	fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

inline void MOESIF_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
    // Hit:
        Sim->read_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;
    // Silent upgrate: 
    case STORE:
        Sim->write_cache_miss++;
        //send_DATA_to_proc(request->addr);
        //send_silent_Up(request->addr);
        send_GETM(request->addr);
        state = MOESIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: E state shouldn't see this message\n");
        break;
    }

}



inline void MOESIF_protocol::do_cache_F (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
    // Hit: 
        Sim->read_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;

    case STORE:
        Sim->write_cache_miss++;
        send_GETM(request->addr);
        state = MOESIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;
    }

}

inline void MOESIF_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
    // Miss: 
        Sim->read_cache_miss++;
        Sim->cache_misses ++;
        send_GETS(request->addr);
        state = MOESIF_CACHE_IS;
        break;

    case STORE:
        Sim->write_cache_miss++;
        Sim->cache_misses ++;
        send_GETM(request->addr);
        state = MOESIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: I state shouldn't see this message\n");
        break;
    }
}

inline void MOESIF_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
    // Hit: 
        Sim->read_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;

    case STORE:
        Sim->write_cache_miss++;
        send_GETM(request->addr);
        state = MOESIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: S state shouldn't see this message\n");
        break;
    }

}

inline void MOESIF_protocol::do_cache_O (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
    // Hit: 
        Sim->read_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;

    case STORE:
        Sim->write_cache_miss++;
        send_GETM(request->addr);
        state = MOESIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: O state shouldn't see this message\n");
        break;
    }

}

inline void MOESIF_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
    // Hit: 
        Sim->read_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;

    case STORE:
    // Hit: 
        Sim->write_cache_hit++;
        send_DATA_to_proc(request->addr);
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: M state shouldn't see this message\n");
        break;
    }
}

inline void MOESIF_protocol::do_cache_IM (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IM state shouldn't see this message\n");
        break;

    case STORE:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IM state shouldn't see this message\n");
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IM state shouldn't see this message\n");
        break;
    }

}

inline void MOESIF_protocol::do_cache_IS (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IS state shouldn't see this message\n");
        break;

    case STORE:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IS state shouldn't see this message\n");
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IS state shouldn't see this message\n");
        break;
    }

}




inline void MOESIF_protocol::do_fetch_E (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // F has $ to $ trans
        //Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_F;
        break;
    
    case GETM:
        // F has $ to $ trans
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: E state shouldn't see this message\n");
        break;

    case DATA_S:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: E state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    case MREQ_INVALID_DS:
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;


	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: E state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_fetch_F (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // F has $ to $ trans
        //Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        break;
    
    case GETM:
        // F has $ to $ trans
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case DATA_S:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    case MREQ_INVALID_DS:
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: F state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_fetch_I (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: I state shouldn't see this message\n");
        break;
    
    case GETM:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: I state shouldn't see this message\n");
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: I state shouldn't see this message\n");
        break;

    case DATA_S:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: I state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: I state shouldn't see this message\n");
        break;

    case MREQ_INVALID_DS:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: I state shouldn't see this message\n");
        break;

    default:
        break;
    }

}

inline void MOESIF_protocol::do_fetch_S (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: S state shouldn't see this message\n");
        break;
    
    case GETM:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: S state shouldn't see this message\n");
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: S state shouldn't see this message\n");
        break;

    case DATA_S:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: S state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    case MREQ_INVALID_DS:
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: S state shouldn't see this message\n");
        break;
    }
}

inline void MOESIF_protocol::do_fetch_O (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // O has $ to $ trans
        //Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        break;
    
    case GETM:
        // O has $ to $ trans
        //Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: O state shouldn't see this message\n");
        break;

    case DATA_S:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: O state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    case MREQ_INVALID_DS:
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: O state shouldn't see this message\n");
        break;
    }

}

inline void MOESIF_protocol::do_fetch_M (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // M has $ to $ trans
        //Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_O;
        break;
    
    case GETM:
        // M has $ to $ trans
        //Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case DATA_S:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        //Sim->cache_to_cache_transfers ++;
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_I;
        break;

    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: M state shouldn't see this message\n");
        break;
    }
}

inline void MOESIF_protocol::do_fetch_IM (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_IM;
        break;
    
    case GETM:
        this->my_table->pushback_input_port(request);
        break;

    case DATA:
        // obtain data and upgrade to M
        send_DATA_to_proc(request->addr);
        state = MOESIF_CACHE_M;
        break;

    case DATA_S:
        fatal_error("Client: IM state shouldn't see DATA_S message\n");
        break;

    case MREQ_INVALID:
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_IM;
        break;

    case MREQ_INVALID_DS:
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOESIF_CACHE_IM;
        break;

    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IM state shouldn't see this message\n");
        break;
    }
}

inline void MOESIF_protocol::do_fetch_IS (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        break;
    
    case GETM:
        this->my_table->pushback_input_port(request);
        break;

    case DATA:
        send_DATA_to_proc(request->addr);
        state = MOESIF_CACHE_E;
        break;

    case DATA_S:
        send_DATA_to_proc(request->addr);
        state = MOESIF_CACHE_S;
        break;

    case MREQ_INVALID:
        this->my_table->pushback_input_port(request); 
        break;

    case MREQ_INVALID_DS:
        this->my_table->pushback_input_port(request); 
        break;

    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IS state shouldn't see this message\n");
        break;
    }
}



