#include "MOSIF_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOSIF_protocol::MOSIF_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    this->state = MOSIF_CACHE_I;
}

MOSIF_protocol::~MOSIF_protocol ()
{    
}

void MOSIF_protocol::dump (void)
{
    const char *block_states[8] = {"X","I","S","O","M","F", "IM", "IP"};
    fprintf (stderr, "MOSIF_protocol - state: %s\n", block_states[state]);
}

void MOSIF_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
    case MOSIF_CACHE_I: do_cache_I(request); break;
    case MOSIF_CACHE_S: do_cache_S(request); break;
    case MOSIF_CACHE_M: do_cache_M(request); break;
    case MOSIF_CACHE_O: do_cache_O(request); break;
    case MOSIF_CACHE_F: do_cache_F(request); break;
    case MOSIF_CACHE_IP: do_cache_IP(request); break;
    case MOSIF_CACHE_IM: do_cache_IM(request); break;
    default:
        fatal_error ("Invalid Cache State for MOSIF Protocol\n");
    }
}

void MOSIF_protocol::process_fetch_request (Mreq *request)
{
	switch (state) {
    case MOSIF_CACHE_I: do_fetch_I(request); break;
    case MOSIF_CACHE_S: do_fetch_S(request); break;
    case MOSIF_CACHE_M: do_fetch_M(request); break;
    case MOSIF_CACHE_O: do_fetch_O(request); break;
    case MOSIF_CACHE_F: do_fetch_F(request); break;
    case MOSIF_CACHE_IP: do_fetch_IP(request); break;
    case MOSIF_CACHE_IM: do_fetch_IM(request); break;
    default:
    	fatal_error ("Invalid Cache State for MOSIF Protocol\n");
    }
}

inline void MOSIF_protocol::do_cache_F (Mreq *request)
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
        state = MOSIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;
    }

}

inline void MOSIF_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        Sim->read_cache_miss++;
        Sim->cache_misses ++;
        send_GETS(request->addr);
        state = MOSIF_CACHE_IP;
        break;

    case STORE:
        Sim->write_cache_miss++;
        Sim->cache_misses ++;
        send_GETM(request->addr);
        state = MOSIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: I state shouldn't see this message\n");
        break;
    }

}

inline void MOSIF_protocol::do_cache_S (Mreq *request)
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
        state = MOSIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: S state shouldn't see this message\n");
        break;
    }

}

inline void MOSIF_protocol::do_cache_O (Mreq *request)
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
        state = MOSIF_CACHE_IM;
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: O state shouldn't see this message\n");
        break;
    }

}

inline void MOSIF_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
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
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: M state shouldn't see this message\n");
        break;
    }
}

inline void MOSIF_protocol::do_cache_IM (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case STORE:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;
    }

}

inline void MOSIF_protocol::do_cache_IP (Mreq *request)
{
    switch (request->msg)
    {
    case LOAD:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case STORE:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;
    
    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;
    }

}

inline void MOSIF_protocol::do_fetch_F (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // F has $ to $ trans
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        break;
    
    case GETM:
        // receive data from I
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case DATA_F:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        // receive data from S
        //Sim->cache_to_cache_transfers ++;
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    case MREQ_INVALID_DS:
        // receive data from S
        // Sim->cache_to_cache_transfers ++;
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    default:
        break;
    }
}

inline void MOSIF_protocol::do_fetch_I (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;
    
    case GETM:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case DATA_F:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    default:
        break;
    }

}

inline void MOSIF_protocol::do_fetch_S (Mreq *request)
{
    switch (request->msg)
    {
    case MREQ_INVALID_DS:
        // receive data from S
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    case MREQ_INVALID:
        // receive data from S
        // Sim->cache_to_cache_transfers ++;
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;
    }
}

inline void MOSIF_protocol::do_fetch_O (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // o has $ to $ trans
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        break;
    
    case GETM:
        // receive data from I
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case DATA_F:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        // receive data from S
        // Sim->cache_to_cache_transfers ++;
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;
    
    case MREQ_INVALID_DS:
        // receive data from S
        // Sim->cache_to_cache_transfers ++;
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    default:
        break;
    }

}

inline void MOSIF_protocol::do_fetch_M (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // M does not have $ to $ trans
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_O;
        break;
    
    case GETM:
        // receive data from I
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    case DATA:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case DATA_F:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: F state shouldn't see this message\n");
        break;

    case MREQ_INVALID:
        // receive data from S
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_I;
        break;

    default:
        break;
    }
}

inline void MOSIF_protocol::do_fetch_IM (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_IM;
        break;
    
    case GETM:
        this->my_table->pushback_input_port(request);
        break;

    case DATA:
        // obtain from cuerrent M
        send_DATA_to_proc(request->addr);
        state = MOSIF_CACHE_M;
        break;

    case MREQ_INVALID:
        // receive data from S
        // Sim->cache_to_cache_transfers ++;
        send_ACK_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_IM;
        break;

    case MREQ_INVALID_DS:
        // receive data from S
        send_nop_on_ptpNetwork(request->addr, request->src_mid);
        state = MOSIF_CACHE_IM;
        break;

    case ACK:
        send_DATA_to_proc(request->addr);
        state = MOSIF_CACHE_M;
        break;

    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IM state shouldn't see this message\n");
        break;
    }
}

inline void MOSIF_protocol::do_fetch_IP (Mreq *request)
{
    switch (request->msg)
    {
    case GETS:
        // Sim->cache_to_cache_transfers ++;
        send_DATA_on_ptpNetwork(request->addr, request->src_mid);
        break;
    
    case GETM:
        this->my_table->pushback_input_port(request);
        break;

    case DATA:
        // obtain from another F
        send_DATA_to_proc(request->addr);
        state = MOSIF_CACHE_S;
        break;

    case DATA_F:
        // obtain from current F
        send_DATA_to_proc(request->addr);
        state = MOSIF_CACHE_F;
        break;

    case MREQ_INVALID:
        this->my_table->pushback_input_port(request); // seems problem here
        break;

    default:
        request->print_msg(my_table->moduleID, "ERROR");
        fatal_error("Client: IP state shouldn't see this message\n");
        break;
    }
}



