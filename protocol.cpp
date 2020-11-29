#include "protocol.h"
#include "../sim/sharers.h"
#include "../sim/hash_table.h"
#include "../sim/sim.h"

extern Simulator * Sim;

Protocol::Protocol (Hash_table *my_table, Hash_entry *my_entry)
{
    this->my_table = my_table;
    this->my_entry = my_entry;
}

Protocol::~Protocol ()
{
}

void Protocol::send_GETM(paddr_t addr)
{
	/* Create a new message to send on the ptpNetwork */
	Mreq * new_request;
	/* The arguments to Mreq are -- msg, address, src_id (optional), dest_id (optional) */
	new_request = new Mreq(GETM,addr);
	/* This will but the message in the ptpNetwork' arbitration queue to sent */
	this->my_table->write_to_ptpNetwork(new_request);
}

void Protocol::send_GETS(paddr_t addr)
{
	/* Create a new message to send on the ptpNetwork */
	Mreq * new_request;
	/* The arguments to Mreq are -- msg, address, src_id (optional), dest_id (optional) */
	new_request = new Mreq(GETS,addr);
	/* This will but the message in the ptpNetwork' arbitration queue to sent */
	this->my_table->write_to_ptpNetwork(new_request);
}

void Protocol::send_DATA_on_ptpNetwork(paddr_t addr, ModuleID dest)
{
	/* Create a new message to send on the ptpNetwork */
	Mreq * new_request;
	/* The arguments to Mreq are -- msg, address, src_id (optional), dest_id (optional) */
	// When DATA is sent on the ptpNetwork it _MUST_ have a destination module
	new_request = new Mreq(DATA, addr, my_table->moduleID, dest);
	/* Debug Message -- DO NOT REMOVE or you won't match the validation runs */
	fprintf(stderr,"**** DATA_SEND Cache: %d  Received cache: %d-- Clock: %lld\n",my_table->moduleID.nodeID, dest.nodeID,Global_Clock);
	/* This will but the message in the ptpNetwork' arbitration queue to sent */
	this->my_table->write_to_ptpNetwork(new_request);

//	Sim->cache_to_cache_transfers++;
}

void Protocol::send_ACK_on_ptpNetwork(paddr_t addr, ModuleID dest)
{
	Mreq * new_request;

	new_request = new Mreq(ACK, addr, my_table->moduleID, dest);
	this->my_table->write_to_ptpNetwork(new_request);
	
}

void Protocol::send_nop_on_ptpNetwork(paddr_t addr, ModuleID dest)
{
	Mreq * new_request;

	new_request = new Mreq(NOP, addr, my_table->moduleID, dest);
	this->my_table->write_to_ptpNetwork(new_request);
}

void Protocol::send_DATA100_on_ptpNetwork(paddr_t addr, ModuleID dest)
{
	Mreq * new_request;

	new_request = new Mreq(DATA_100, addr, my_table->moduleID, dest);
	this->my_table->write_to_ptpNetwork(new_request);
}

void Protocol::send_DATA_to_proc(paddr_t addr)
{
	/* Create a new message to send on the ptpNetwork */
	Mreq * new_request;
	/* The arguments to Mreq are -- msg, address, src_id (optional), dest_id (optional) */
	// When data is sent from a cache to proc, there is no need to set the src and dest
	new_request = new Mreq(DATA,addr);
	/* This writes the message into the processor's input buffer.  The processor
	 * only expects to ever receive DATA messages
	 */
	this->my_table->write_to_proc(new_request);
}

void Protocol::send_silent_Up(paddr_t addr)
{
	Mreq *new_request;
	new_request = new Mreq(Silent_Up, addr);
	this->my_table->write_to_ptpNetwork(new_request);
}

void Protocol::set_shared_line ()
{
	
}

bool Protocol::get_shared_line ()
{

}
