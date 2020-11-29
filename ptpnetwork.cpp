#include "ptpnetwork.h"
#include "mreq.h"

ptpNetwork::ptpNetwork(int num_nd)
{
 num_nodes = num_nd; 
}

ptpNetwork::~ptpNetwork()
{
}

void ptpNetwork::tick()
{

	while (!pending_requests.empty())
	{
            Mreq *current_request;
	    current_request = pending_requests.front();
            // go to dest cache or directory            
               node_requests[current_request->dest_mid.nodeID].push_back(current_request);
            

	    pending_requests.pop_front(); // pending request is request in directory memory
	    
	}
}

bool ptpNetwork::ptpNetwork_request(Mreq *request) // used by write_output_port()
{
	
        pending_requests.push_back(request); // write request to pending
   

	return true;
}

bool ptpNetwork::ptpNetwork_pushback(Mreq *request, int nodeID) // used by pushback_input_port
{
        
        node_requests[nodeID].push_back(request);    // write request to specific node   
        return true;
}

Mreq* ptpNetwork::ptpNetwork_fetch(int nodeID) // node ID is where you wanna fetch, used by fetch_input_fetch()
{
    Mreq *request;

    if(!node_requests[nodeID].empty()){

    request = new Mreq ();
    *request = *node_requests[nodeID].front();
    node_requests[nodeID].pop_front();
    return request;
    }
    else
    {
        return NULL;
    }
}

/////////////////////////////////////////////
//    fetch = read + pop
/////////////////////////////////////////////


Mreq* ptpNetwork::ptpNetwork_read(int nodeID) // used
{
    Mreq *request;

    if(!node_requests[nodeID].empty()){

    request = new Mreq ();
    *request = *node_requests[nodeID].front();
    return request;
    }
    else
    {
        return NULL;
    }
}

void ptpNetwork::ptpNetwork_pop(int nodeID)
{

    if(!node_requests[nodeID].empty()){
    node_requests[nodeID].pop_front();
    }
}
