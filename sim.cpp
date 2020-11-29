#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>

#include "hash_table.h"
#include "processor.h"
#include "memory.h"
#include "module.h"
#include "mreq.h"
#include "settings.h"
#include "sim.h"
#include "types.h"

extern Sim_settings settings;

/** Fatal Error.  */
void fatal_error (const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
    
    /** Enable debugging by asserting zero.  */
    assert (0 && "Fatal Error");
    exit (-1);
}

Simulator::Simulator ()
{
    /** Seed random number generator.  */
    srandom (1023);
    
    /** Set global_clock to cycle zero.  */
    global_clock = 0;

    /** Allocate ptpNetwork.  */
    ptpnetwork = new ptpNetwork (settings.num_nodes);
    assert (ptpnetwork && "Sim error: Unable to alloc ptpnetwork.");

    Nd = new Node*[settings.num_nodes+1];

    /** Allocate processors.  */
    for (int node = 0; node < settings.num_nodes; node++)
    {
        char trace_file[100];
        sprintf (trace_file, "%s/p%d.trace", settings.trace_dir, node);

        Nd[node] = new Node (node);
        Nd[node]->build_processor (trace_file);
        //Add MC Module to each node
        //TODO
    }

    /** Allocate memory controllers.  */
    Nd[settings.num_nodes] = new Node (settings.num_nodes);
    Nd[settings.num_nodes]->build_memory_controller ();

    /*Set the memory controllers ModuleID */
     for (int node = 0; node < settings.num_nodes; node++)
    { 
       Nd[node]->mod[L1_M]->MC_moduleID = Nd[settings.num_nodes]->mod[MC_M]->moduleID;
       fprintf(stderr,"\n Set the memory controllers nodeID:  %d \n",Nd[node]->mod[L1_M]->MC_moduleID.nodeID);
    }


    cache_misses = 0;
    silent_upgrades = 0;
    cache_to_cache_transfers = 0;
    cache_accesses = 0;

    read_hit_clean = 0;
    read_hit_dirty = 0;
    read_miss = 0;
    write_hit_clean = 0;
    write_hit_dirty = 0;
    write_miss = 0;
    invalidation = 0;
    write_cache_hit = 0;
    read_cache_hit = 0;
    write_cache_miss = 0;
    read_cache_miss = 0;
}

Simulator::~Simulator ()
{
    for (int i = 0; i < settings.num_nodes; i++)
        delete Nd[i];

    delete [] Nd;    
}

void Simulator::dump_stats ()
{
    for (int i=0; i < settings.num_nodes; i++)
    {
    	get_L1(i)->dump_hash_table();
    }
    fprintf(stderr,"\nRun Time:         %8lld cycles\n",global_clock);
    fprintf(stderr,"Cache Miss:     %8ld misses\n",cache_misses);
    fprintf(stderr,"Cache Access:   %8ld accesses\n",cache_accesses);
    fprintf(stderr,"Silent Upgrade:  %8ld upgrades\n",silent_upgrades);
    fprintf(stderr,"$-to-$ Transfer: %8ld transfers\n",cache_to_cache_transfers);
    fprintf(stderr,"Read Hit Clean:   %8ld times\n",read_hit_clean);
    fprintf(stderr,"Read Hit Dirty:   %8ld times\n",read_hit_dirty);
    fprintf(stderr,"Read Miss:      %8ld times\n",read_miss);
    fprintf(stderr,"Write Hit Clean:  %8ld times\n",write_hit_clean);
    fprintf(stderr,"Write Hit Dirty:  %8ld times\n",write_hit_dirty);
    fprintf(stderr,"Write Miss:     %8ld times\n",write_miss);
    fprintf(stderr,"Invalidation:     %8ld times\n",invalidation);
    fprintf(stderr,"Cache Write Hit:     %8ld times\n",write_cache_hit);
    fprintf(stderr,"Cache Read Hit:     %8ld times\n",read_cache_hit);
    fprintf(stderr,"Cache Write Miss:     %8ld times\n",write_cache_miss);
    fprintf(stderr,"Cache Read Miss:     %8ld times\n",read_cache_miss);
}

void Simulator::run ()
{
    int sched;
    bool done;

    /** This must match what's in enums.h.  */
    const char *cp_str[11] = {"CACHE_PRO","MI_PRO","MSI_PRO","MESI_PRO",
							 "MOESI_PRO","MOSI_PRO","MOSIF_PRO","MOESIF_PRO","Improved_MOESIF_PRO","NULL_PRO","MEM_PRO"};

    fprintf (stderr, "CSX290 Sim - Begins  ");
    fprintf (stderr, " Cores: %d", settings.num_nodes);
    fprintf (stderr, " Protocol: %s\n", cp_str[settings.protocol]);

    /** Main run loop.  */
    sched = 0;
    done = false;
    while (!done)
    {
        ptpnetwork->tick ();

        for (int i = 0; i <= settings.num_nodes; i++)
            Nd[i]->tick_cache ();

        for (int i = 0; i <= settings.num_nodes; i++)
            Nd[i]->tick_pr ();

        for (int i = 0; i <= settings.num_nodes; i++)
            Nd[i]->tick_mc ();
        
        for (int i = 0; i <= settings.num_nodes; i++)
			Nd[i]->tock_pr ();

        global_clock++;

        done = true;
        for (int i = 0; i < settings.num_nodes; i++)
            if (!get_PR(i)->done ())
            {
                done = false;
                break;        
            }
    }

    fprintf(stderr,"\n\nSimulation Finished\n");
    dump_stats();
}

Processor* Simulator::get_PR (int node)
{
    return (Processor *)(Nd[node]->mod[PR_M]);
}
Hash_table* Simulator::get_L1 (int node)
{
    return (Hash_table *)(Nd[node]->mod[L1_M]);
}
Memory_controller* Simulator::get_MC (int node)
{
    return (Memory_controller *)(Nd[node]->mod[MC_M]);
}

/** Debug.  */
void Simulator::dump_processors (void)
{
    Processor *pr;
    for (int i = 0; i < settings.num_nodes; i++)
    {
        pr = get_PR (i);
    }
}

void Simulator::dump_outstanding_requests (int nodeID)
{
    assert (nodeID < settings.num_nodes);   
}

void Simulator::dump_cache_block (int nodeID, paddr_t addr)
{
    assert (get_L1(nodeID));
    get_L1 (nodeID)->dump_hash_entry (addr);
}
