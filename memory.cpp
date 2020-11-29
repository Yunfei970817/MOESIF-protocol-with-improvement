#include "memory.h"
#include "sim.h"
//#include "settings.h"

extern Simulator *Sim;
extern Sim_settings settings;

bool stall_direct = false;
int stall_time = 0;
int enter_mulS_time = 0;

deque<Entry *> entry;
Directory *mytable;
void Entry::Initial_Entry(Entry *entry, paddr_t addr, int node_num)
{

   entry->address = addr;
   entry->block_num = node_num;
   //printf("cycle: 0\n");
   entry->block = new Block[node_num];
   //printf("node_num: %d\n", node_num);
   for (int i = 0; i < node_num; i++)
   {
      // printf("cycle: %d\n", i);
      entry->block[i].state = "I";
      entry->block[i].nodeID = i;
      //entry->block[i].destID = -1;
   }
}
void Entry::State_change(int nodeID, string State)
{
   block[nodeID].state = State;
}

void Entry::Entry_pushback(Entry *Entry)
{
   entry.push_back(Entry);
}

Entry Entry::access(paddr_t addr)
{
   if (entry.size() != 0)
   {
      for (int i = 0; i < entry.size(); i++)
      {
         if (entry.at(i)->address == addr)
            return *entry.at(i);
      }
   }
}

bool Entry::addr_exist(paddr_t addr)
{
   bool exist = false;
   //printf("cycle: %d\n", int(Global_Clock));
   if (entry.size() != 0)
   {
      for (int i = 0; i < entry.size(); i++)
      {
         if (entry.at(i)->address == addr)
         {
            exist = true;
            break;
         }
      }
   }
   return exist;
}

Memory_controller::Memory_controller(ModuleID moduleID, int resp_time)
    : Module(moduleID, "MC_")
{
   this->mem_resp_time = resp_time;
}

Memory_controller::~Memory_controller()
{
}

int findnode(Directory *Mytable, paddr_t addr, string state)
{
   //printf("state: %s\n", state.c_str());
   int index = 100;
   for (int i = 0; i < Mytable->access_entry(addr).block_num; i++)
   {
      if (Mytable->access_entry(addr).access_block(i).state == state)
      {
         index = i;
         break;
      }
   }
   if (index == 100)
      printf("Couldn't find specific node!!\n");
   return index;
}

bool findnode_exist(Directory *Mytable, paddr_t addr, string state)
{
   //printf("state: %s\n", state.c_str());
   bool exist = false;
   for (int i = 0; i < Mytable->access_entry(addr).block_num; i++)
   {
      if (Mytable->access_entry(addr).access_block(i).state == state)
      {
         exist = true;
         break;
      }
   }

   return exist;
}

void printState(Directory *Mytable, paddr_t addr)
{
   for (int i = 0; i < Mytable->access_entry(addr).block_num; i++)
   {
      std::cout<<i<<" "<<Mytable->access_entry(addr).access_block(i).state<<std::endl;
   }
}

bool GETS_trans_state_deter(Directory *Mytable, paddr_t addr)
{
   bool trans = false;
   for (int i = 0; i < settings.num_nodes; i++)
   {
      if (Mytable->access_entry(addr).access_block(i).state == "IE" ||
          Mytable->access_entry(addr).access_block(i).state == "IM" ||
          Mytable->access_entry(addr).access_block(i).state == "SM" ||
          Mytable->access_entry(addr).access_block(i).state == "IP" ||
          Mytable->access_entry(addr).access_block(i).state == "IS")
      {
         trans = true;
         break;
      }
   }
   return trans;
}

int GETS_deter_state(Directory *Mytable, paddr_t addr, int nodeID)
{
   // if all the processors are invalid
   // if some processors are in the trans-state
   // if one is in E
   // if some are in S
   // if one is in M
   // all other situation are impossiable

   int state = 0; // all other situation are impossiable

   // if some processors are in the trans-state state = 100
   if (GETS_trans_state_deter(Mytable, addr))
   {
      state = 100;
      return state;
   }
   // disscuss on the target nodeID
   // no trans-state here
   int E_num = 0, M_num = 0, S_num = 0; // determine num of other state
   for (int i = 0; i < settings.num_nodes; i++)
   {
      if (Mytable->access_entry(addr).access_block(i).state == "E")
         E_num++;
      if (Mytable->access_entry(addr).access_block(i).state == "M")
         M_num++;
      if (Mytable->access_entry(addr).access_block(i).state == "S")
         S_num++;
   }
   if (Mytable->access_entry(addr).access_block(nodeID).state == "I")
   {

      // if have and only have one E
      if (E_num == 1 && M_num == 0 && S_num == 0)
         state = 1;
      // if have and only have one M
      if (E_num == 0 && M_num == 1 && S_num == 0)
         state = 2;
      // if have S
      if (E_num == 0 && M_num == 0 && S_num != 0)
      {
         if (S_num == 1)
            state = 3;
         else
            state = 30;
      }
      //else printf("State Error when ***GETS***! because there are more than two S!!!! current state is I\n");}
      // if all are I
      if (E_num == 0 && M_num == 0 && S_num == 0)
         state = 4;
   }
   if (Mytable->access_entry(addr).access_block(nodeID).state == "S")
   {
      if (E_num == 0 && M_num == 0 && S_num != 0)
         state = 5;
      else
         printf("State Error when ***GETS***! current state is S\n");
   }
   if (Mytable->access_entry(addr).access_block(nodeID).state == "E")
   {
      if (E_num == 1 && M_num == 0 && S_num == 0)
         state = 6;
      else
         printf("State Error when ***GETS***! current state is E\n");
   }
   if (Mytable->access_entry(addr).access_block(nodeID).state == "M")
   {
      if (E_num == 0 && M_num == 1 && S_num == 0)
         state = 7;
      else
         printf("State Error when ***GETS***! current state is M\n");
   }
   return state;
}

int MOFSI_GETS_Condition(Directory *Mytable, paddr_t addr, int nodeID)
{
   // 0: trans state exist
   // 1: pure I
   // 2: F exist
   // 3: O exist
   // 4: M exist
   // 100 is default
   int state = 100;                                                   // invalid condition
   if (mytable->access_entry(addr).access_block(nodeID).state == "I") // if current state is I
   {
      if (GETS_trans_state_deter(Mytable, addr))
         state = 0; // there is trans state
      else
      {
         int F_num = 0, O_num = 0, M_num = 0, S_num = 0; // determine num of other state
         for (int i = 0; i < settings.num_nodes; i++)
         {
            if (Mytable->access_entry(addr).access_block(i).state == "F")
               F_num++;
            if (Mytable->access_entry(addr).access_block(i).state == "M")
               M_num++;
            if (Mytable->access_entry(addr).access_block(i).state == "S")
               S_num++;
            if (Mytable->access_entry(addr).access_block(i).state == "O")
               O_num++;
         }

         if (F_num == 0 && O_num == 0 && M_num == 0 && S_num == 0)
            state = 1; // pure I
         else if (F_num == 1)
            state = 2; // F exist
         else if (O_num == 1)
            state = 3; // O exist
         else if (M_num == 1)
            state = 4; // M exist
         else
            printf("\nthe state to obtain GETS is wrong!!!!!!\n");
      }
   }
   else
      printf("\nthe state to obtain GETS is wrong!!!!!!\n");
   return state;
}

int MOFSI_GETM_Condition(Directory *Mytable, paddr_t addr, int nodeID)
{
   // 0 : trans state
   // 1 : pure I
   // 2 : current I with F need deter S number
   // 3 : current I with O need deter S number
   // 4 : current I with M
   // 5 : current S with F need deter S number
   // 6 : current S with O need deter S number
   // 7 : current F need deter S number
   // 8 : current O need deter S number
   // 100: defaults
   int state = 100; // invalid condition
   if (GETS_trans_state_deter(Mytable, addr))
      state = 0; // there is trans state
   else
   {
      int F_num = 0, O_num = 0, M_num = 0, S_num = 0; // determine num of other state
      for (int i = 0; i < settings.num_nodes; i++)
      {
         if (Mytable->access_entry(addr).access_block(i).state == "F")
            F_num++;
         if (Mytable->access_entry(addr).access_block(i).state == "M")
            M_num++;
         if (Mytable->access_entry(addr).access_block(i).state == "S")
            S_num++;
         if (Mytable->access_entry(addr).access_block(i).state == "O")
            O_num++;
      }
      if (mytable->access_entry(addr).access_block(nodeID).state == "I") // if current state is I
      {
         if (F_num == 0 && O_num == 0 && M_num == 0 && S_num == 0)
            state = 1; // pure I
         else if (F_num == 1)
            state = 2; // F exist
         else if (O_num == 1)
            state = 3; // O exist
         else if (M_num == 1)
            state = 4; // M exist
      }
      else if (mytable->access_entry(addr).access_block(nodeID).state == "S") // if current state is S
      {
         if (F_num == 1)
            state = 5; // F exist
         else if (O_num == 1)
            state = 6; // O exist
      }
      else if (mytable->access_entry(addr).access_block(nodeID).state == "F")
         state = 7; // if current state is F
      else if (mytable->access_entry(addr).access_block(nodeID).state == "O")
         state = 8; // if vurrent state is O
      else
         printf("\nthe state to obtain GETS is wrong!!!!!!\n");
   }

   return state;
}

int MOEFSI_GETS_Condition(Directory *Mytable, paddr_t addr, int nodeID)
{
   // 0: trans state exist
   // 1: pure I
   // 2: F exist
   // 3: E exist
   // 4: O exist
   // 5: M exist
   //// 6: S exist(the only one O has written back)
   // 100 is default
   int state = 100;  // Default invalid condition
   if (mytable->access_entry(addr).access_block(nodeID).state == "I") // if current state is I
   {
      if (GETS_trans_state_deter(Mytable, addr))
         state = 0; // there is trans state
      else
      {
         int F_num = 0, O_num = 0, M_num = 0, S_num = 0, E_num = 0; // determine num of other state
         for (int i = 0; i < settings.num_nodes; i++)
         {
            if (Mytable->access_entry(addr).access_block(i).state == "E")
               E_num++;
            if (Mytable->access_entry(addr).access_block(i).state == "F")
               F_num++;
            if (Mytable->access_entry(addr).access_block(i).state == "M")
               M_num++;
            if (Mytable->access_entry(addr).access_block(i).state == "S")
               S_num++;
            if (Mytable->access_entry(addr).access_block(i).state == "O")
               O_num++;
         }

         if (E_num == 0 && F_num == 0 && O_num == 0 && M_num == 0 && S_num == 0)
            state = 1; // pure I
         else if (F_num == 1)
            state = 2; // F exist
         else if (E_num == 1)
            state = 3; // E exist
         else if (O_num == 1)
            state = 4; // O exist
         else if (M_num == 1)
            state = 5; // M exist
      }
   }
   else
   {
      printState(Mytable, addr);
      printf("\nThe state to obtain GETS is wrong!!!!!!\n");
   }
      
   return state;
}

int MOEFSI_GETM_Condition(Directory *Mytable, paddr_t addr, int nodeID)
{
   // 0 : trans state
   // 1 : pure I
   // 2 : current I with F need deter S number
   // 3 : current I with O need deter S number
   // 4 : current I with M
   // 5 : current I with E
   // 6 : current S with F need deter S number
   // 7 : current S with O need deter S number
   // 8 : current F need deter S number
   // 9 : current O need deter S number
   // 10 : current E
   // 100: defaults
   int state = 100; // invalid condition
   if (GETS_trans_state_deter(Mytable, addr))
      state = 0; // there is trans state
   else
   {
      int F_num = 0, O_num = 0, M_num = 0, S_num = 0,E_num=0; // determine num of other state
      for (int i = 0; i < settings.num_nodes; i++)
      {
         if (Mytable->access_entry(addr).access_block(i).state == "F")
            F_num++;
         if (Mytable->access_entry(addr).access_block(i).state == "M")
            M_num++;
         if (Mytable->access_entry(addr).access_block(i).state == "S")
            S_num++;
         if (Mytable->access_entry(addr).access_block(i).state == "O")
            O_num++;
         if (Mytable->access_entry(addr).access_block(i).state == "E")
             E_num++;
      }
      if (mytable->access_entry(addr).access_block(nodeID).state == "I") // if current state is I
      {
         if (F_num == 0 && O_num == 0 && M_num == 0 && S_num == 0 && E_num == 0)
            state = 1; // pure I
         else if (F_num == 1)
            state = 2; // F exist
         else if (O_num == 1)
            state = 3; // O exist
         else if (M_num == 1)
            state = 4; // M exist
         else if (E_num == 1)  
            state = 5; // E exist
      }
      else if (mytable->access_entry(addr).access_block(nodeID).state == "S") // if current state is S
      {
         if (F_num == 1)
            state = 6; // F exist
         else if (O_num == 1)
            state = 7; // O exist
      }
      else if (mytable->access_entry(addr).access_block(nodeID).state == "F")
         state = 8; // if current state is F
      else if (mytable->access_entry(addr).access_block(nodeID).state == "O")
         state = 9; // if current state is O
      else if (mytable->access_entry(addr).access_block(nodeID).state == "E")
         state = 10; // if current state is O
      else
         printf("\nThe state to obtain GETM is wrong!!!!!!\n");
   }

   return state;
}



void Memory_controller::tick()
{
   Mreq *request;
   request = read_input_port();
   if (request != NULL)
   {
      //if(request != NULL && int(Global_Clock) < 1000){
////////////////////////////////////////////////////////////////////////////
/////////////////////////***MESI***/////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
      if (settings.protocol == MESI_PRO)
      {

         if (mytable->addr_exist(request->addr)) // if the address of current request is in the table
         {
            // if request time cannot be solved
            if (Global_Clock == 8171)
               printf("here\n");
            if (int(Global_Clock) >= stall_time)
            {
               stall_direct = false;
            }
            if (stall_direct)
            {
               if (Global_Clock == 8171)
                  printf("here\n");
            }
            // if request time can be solved
            else
            {
               //if(int(Global_Clock) == 3) printf("Enter num: %d\n", request->src_mid.nodeID);
               // determine request message first
               switch (request->msg)
               {
               case GETS:
               {
                  if (int(Global_Clock) == 95931)
                     printf("GLobal_cyc: %d     GETS case: %d\n", int(Global_Clock), GETS_deter_state(mytable, request->addr, request->src_mid.nodeID));
                  switch (GETS_deter_state(mytable, request->addr, request->src_mid.nodeID))
                  {
                  case 100:
                  { // trans state
                     // pop the front request and push it in the end of the queue
                     Mreq *temp_request = Sim->ptpnetwork->ptpNetwork_fetch(settings.num_nodes);
                     Sim->ptpnetwork->ptpNetwork_pushback(temp_request, settings.num_nodes);
                     break;
                  }

                  case 0: // error state
                  {
                     printf("error when GETS!!!\n");
                     break;
                  }

                  case 1:
                  { 
                     Sim->read_hit_clean++;
                     // current nodeID is I but another is E
                     // change E to S directly
                     // mytable->access_entry(request->addr).access_block(findnode(mytable, request->addr, "E")).state = "S";
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "S");
                     // change current to IE
                     // mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state = "IE";
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IE"); // wait DATA to change IE as S
                     // send a GETS to cache which has E in this address back
                     Mreq *new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 2:
                  { 
                     Sim->read_hit_dirty++;
                     // current nodeID is I but another is M
                     // change M to S directly
                     // mytable->access_entry(request->addr).access_block(findnode(mytable, request->addr, "M")).state = "S";
                     if (Global_Clock == 95931)
                        printf("~~~~~~~nodeID with M: %d\n", findnode(mytable, request->addr, "M"));
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "M"), "S");
                     // change current to IE
                     // mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state = "IE";
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IE"); // wait for DATA back
                     // send a GETS to cache which has M in this address back
                     Mreq *new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 3: // current nodeID is I another is S
                          // change current state to S directly
                  {       //mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state = "S";
                     Sim->read_hit_clean++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "S");
                     // send DATA back to make IE change to S
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 30: // current state is I while more than 1 S exist
                  {
                     Sim->read_miss++;
                     // need to read from memory
                     // stall first
                     if (enter_mulS_time == 0)
                     {
                        stall_time = Global_Clock + 100;
                        stall_direct = true;
                        enter_mulS_time = 1;
                     }
                     else if (enter_mulS_time == 1)
                     { // if secound enter
                        mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "S");
                        // send DATA back to make IE change to S
                        Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        // pop the front request
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                        enter_mulS_time = 0;
                     }
                     /*
                     for(int j = 0; j < 2; j++)
                     {
                        // send MREQ_INVALID data to cache whose state is M // and wait signal back
                        Mreq* new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        // change S state to I
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                     }
                     */
                     break;
                  }

                  case 4:
                  {  // all the state are I
                     // change current to IE
                     Sim->read_miss++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "E");
                     // printf("P%d state:  %s\n", request->src_mid.nodeID, mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state.c_str());
                     // send a DATA_E to cache which has IE state in this address
                     Mreq *new_request = new Mreq(DATA_E, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 5: // this is already S
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;

                  case 6: // this is already E
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;

                  case 7: // this is already M
                          // back to S
                  {       // mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state = "S";
                     Sim->read_hit_dirty++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "S");
                     // send a DATA to cache which has IE state in this address
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  default:
                  {
                     printf("error when GETS!!!\n");
                     break;
                  }
                  }
                  break;
               }
               case GETM:
               {
                  // if there are temperate state in this address
                  if (GETS_trans_state_deter(mytable, request->addr))
                  {
                     // pop the front request and push it in the end of the queue
                     Mreq *temp_request = Sim->ptpnetwork->ptpNetwork_fetch(settings.num_nodes);
                     Sim->ptpnetwork->ptpNetwork_pushback(temp_request, settings.num_nodes);
                     break;
                  }
                  else
                  {
                     // if current state is I
                     if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "I")
                     {
                        int E_num = 0, M_num = 0, S_num = 0; // determine num of other state
                        for (int i = 0; i < settings.num_nodes; i++)
                        {
                           if (mytable->access_entry(request->addr).access_block(i).state == "E")
                              E_num++;
                           if (mytable->access_entry(request->addr).access_block(i).state == "M")
                              M_num++;
                           if (mytable->access_entry(request->addr).access_block(i).state == "S")
                              S_num++;
                        }
                        if (E_num > 1 || M_num > 1 || (E_num == 1 && M_num == 1))
                           printf("State Error when ***GETM***! current state is I\n");
                        else
                        {
                           if (E_num == 0 && M_num == 0 && S_num == 0) // if all state are I
                           {
                              Sim->write_miss++;
                              // write current node to M
                              // mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state = "M";
                              mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "M");
                              // send DATA back
                              Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              // pop the front request
                              Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                           }
                           else if (E_num == 1 && M_num == 0 && S_num == 0) // if there is a E
                           {
                              Sim->write_hit_clean++;
                              // write current node to M
                              // mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state = "IM";
                              mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM"); // wait ACK signal to change to M
                              // write E state to I
                              int tem_node = findnode(mytable, request->addr, "E");
                              // mytable->access_entry(request->addr).access_block(tem_node).state = "I";
                              mytable->access_entry(request->addr).State_change(tem_node, "I");
                              // send MREQ_INVALID data to cache whose state is E // and wait signal back
                              Mreq *new_request = new Mreq(GETM, request->addr, moduleID, Sim->Nd[tem_node]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              // pop the front request
                              Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                           }
                           else if (E_num == 0 && M_num == 1 && S_num == 0) // if there is a M
                           {
                              Sim->write_hit_dirty++;
                              // write current node to M
                              // mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state = "IM"; // wait ACK signal to change to M
                              mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM"); // wait ACK signal to change to M
                              // write M state to I
                              int tem_node = findnode(mytable, request->addr, "M");
                              mytable->access_entry(request->addr).State_change(tem_node, "I");
                              // send MREQ_INVALID data to cache whose state is M // and wait signal back
                              Mreq *new_request = new Mreq(GETM, request->addr, moduleID, Sim->Nd[tem_node]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              // pop the front request
                              Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                           }
                           else if (E_num == 0 && M_num == 0 && S_num != 0) // if there are some S
                           {
                              // write current node to M
                              if (S_num == 1) // if only one S
                              {
                                 Sim->write_hit_clean++;
                                 mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM"); // wait ACK signal to change to M
                                 // send MREQ_INVALID data to cache whose state is M // and wait signal back
                                 Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                                 this->write_output_port(new_request);
                                 // change S state to I
                                 mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                                 // pop the front request
                                 Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                              }
                              //else if(S_num > 2){printf("State Error when ***GETM***! because there are more than two S!!!! current state is I\n");}
                              else if (S_num >= 2)
                              { // if there are two S

                                 // stall

                                 //if(stall_time != Global_Clock + 100) stall_time = Global_Clock + 100;
                                 //stall_direct = true;
                                 Sim->write_miss++;
                                 mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM"); // wait ACK signal to change to M

                                 for (int j = 0; j < S_num; j++)
                                 {
                                    // send MREQ_INVALID data to cache whose state is M // and wait signal back
                                    Mreq *new_request;
                                    if (j == (S_num - 1))
                                    {
                                       printf("                                  nodeID:  %D\n", findnode(mytable, request->addr, "S"));
                                       new_request = new Mreq(MREQ_INVALID_100, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                                    }
                                    else
                                    {
                                       printf("                                  nodeID:  %D\n", findnode(mytable, request->addr, "S"));
                                       new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                                    }
                                    this->write_output_port(new_request);
                                    // change S state to I
                                    mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                                 }
                                 // pop the front request
                                 Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                              }
                           }
                        }
                     }
                     // if current state is E
                     else if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "E")
                        printf("E state cannot receive GETM in directory\n");
                     // if current state is M
                     else if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "M")
                     {
                        // pop the front request
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                     // if current state is S
                     else if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "S")
                     {
                        Sim->write_hit_clean++;
                        // write current node to M
                        // mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state = "SM"; // wait ACK signal to change to M
                        mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "SM"); // wait ACK signal to change to M

                        int E_num = 0, M_num = 0, S_num = 0; // determine num of other state
                        for (int i = 0; i < settings.num_nodes; i++)
                        {
                           if (mytable->access_entry(request->addr).access_block(i).state == "E")
                              E_num++;
                           if (mytable->access_entry(request->addr).access_block(i).state == "M")
                              M_num++;
                           if (mytable->access_entry(request->addr).access_block(i).state == "S")
                           {
                              printf("S node: %d\n", i);
                              S_num++;
                           }
                        }

                        if (Global_Clock == 51797)
                           printf("nodeID: %d, state: %s      node num in S :%d \n", request->src_mid.nodeID,
                                  mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state.c_str(), S_num);
                        if (E_num != 0)
                           printf("State Error when ***GETM***! current state is S\n");
                        else if (M_num != 0)
                           printf("State Error when ***GETM***! current state is S\n");

                        else if (E_num == 0 && M_num == 0 && S_num != 0) // if there are some S one is current state
                        {
                           // if(S_num > 2){printf("State Error when ***GETM***! because there are more than two S!!!! current state is S\n");}
                           // change current to SM

                           // send MREQ_INVALID to another S
                           if (S_num == 1)
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              // change another S to I
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           else
                           {
                              for (int j = 0; j < S_num; j++)
                              {
                                 printf("S node: %d\n", findnode(mytable, request->addr, "S"));
                                 if (j == (S_num - 1))
                                 {
                                    Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                                    this->write_output_port(new_request);
                                    // change another S to I
                                    mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                                 }
                                 else
                                 {
                                    Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                                    this->write_output_port(new_request);
                                    // change another S to I
                                    mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                                 }
                              }
                           }
                           // pop the front request
                           Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                        }
                     }
                  }
                  break;
               }
               case DATA:
               {
                  if (findnode_exist(mytable, request->addr, "IE"))
                  {
                     // send DATA to cache whiose state is IE
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IE")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IE state to S
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IE"), "S");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  else if (findnode_exist(mytable, request->addr, "IM"))
                  {
                     // send ACK to cache whiose state is IM
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IM")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IM state to M
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IM"), "M");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  break;
               }
               case ACK:
               {
                  Sim->invalidation++;
                  if (findnode_exist(mytable, request->addr, "IM"))
                  {
                     // send ACK to cache whiose state is IM
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IM")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IM state to M
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IM"), "M");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }

                  else if (findnode_exist(mytable, request->addr, "SM"))
                  {
                     // send ACK to cache whiose state is IM
                     Mreq *new_request = new Mreq(ACK, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "SM")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IM state to M
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "SM"), "M");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  break;
               }
               case Silent_Up:
               {
                  printf("cycle: %d\n", int(Global_Clock));
                  if (findnode_exist(mytable, request->addr, "E"))
                  {
                     // change E state to M
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "M");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  //else printf("State Error when ***Silentup***! current state is no E\n");
                  else if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "S")
                  {
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  else
                     printf("State Error when ***Silent***! \n");
                  break;
               }

               case NOP:
               {
                  Sim->invalidation++;
                  // pop the front request
                  Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  break;
               }

               case DATA_100:
               {
                  if (enter_mulS_time == 0) // first enter
                  {
                     if (stall_time != Global_Clock + 100)
                        stall_time = Global_Clock + 100;
                     stall_direct = true;
                     enter_mulS_time = 1;
                  }
                  else
                  {
                     enter_mulS_time = 0;
                     if (findnode_exist(mytable, request->addr, "IM"))
                     {
                        // send ACK to cache whiose state is IM
                        Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IM")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        // change IM state to M
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IM"), "M");
                        // pop the front request
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                  }

                  break;
               }

               default:
                  printf("     Invalid message!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
                  break;
               }
            }
         }
         else // if the address of current request is not in the table
         {
            // set the new request time as 100 cycles after.
            if (stall_time != Global_Clock + 100)
               stall_time = Global_Clock + 100;
            stall_direct = true;
            Sim->ptpnetwork->ptpNetwork_read(settings.num_nodes)->req_time = Global_Clock + 100;
            // build a new block for the request
            //printf("cycle: %d\n", int(Global_Clock));
            mytable->add_Entry(request->addr, settings.num_nodes);

            printf("request node: %d    req_time cycle: %llu\n", request->src_mid.nodeID, request->req_time);
         }
      }

      /////////////////////////////////////////////////////////
      ////////////////  ***MOFSI*** /////////////////////////////////
      /////////////////////////////////////////////////////////

      else if (settings.protocol == MOSIF_PRO)
      {
         if (mytable->addr_exist(request->addr)) // if the address of current request is in the table
         {

            if (int(Global_Clock) >= stall_time)
            {
               stall_direct = false;
            } // free stall
            //if(request->msg == GETM) printf("GETM case: %d\n", MOFSI_GETM_Condition(mytable, request->addr, request->src_mid.nodeID));
            if (stall_direct)
            {
               if (Global_Clock == 8171)
                  printf("here\n");
            }

            // after reading from memory
            else
            {
               switch (request->msg)
               {
               case GETS:
               {
                  //printf("message: %d\n", request->msg);
                  switch (MOFSI_GETS_Condition(mytable, request->addr, request->src_mid.nodeID))
                  {
                  // 0: trans state exist
                  // 1: pure I
                  // 2: F exist
                  // 3: E exist
                  // 4: O exist
                  // 100 is default
                  case 0:
                  {
                     // pop the front request and push it in the end of the queue
                     Mreq *temp_request = Sim->ptpnetwork->ptpNetwork_fetch(settings.num_nodes);
                     Sim->ptpnetwork->ptpNetwork_pushback(temp_request, settings.num_nodes);
                     break;
                  }
                  case 1: // 1: pure I
                  {
                     Sim->read_miss++;
                     // change current to F
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "F");
                     // printf("P%d state:  %s\n", request->src_mid.nodeID, mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state.c_str());
                     // send a DATA_F to cache which has IE state in this address
                     Mreq *new_request = new Mreq(DATA_F, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }
                  case 2: // 2: F exist
                  {
                     Sim->read_hit_clean++;
                     Sim->cache_to_cache_transfers++;
                     // change current to S
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IP");
                     // printf("P%d state:  %s\n", request->src_mid.nodeID, mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state.c_str());
                     // send a DATA to cache which has IE state in this address
                     Mreq *new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "F")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }
                  case 3: // 3: E exist
                  {
                     Sim->read_hit_clean++;
                     Sim->cache_to_cache_transfers++;
                     // change current to S
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IP");
                     // printf("P%d state:  %s\n", request->src_mid.nodeID, mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state.c_str());
                     // send a DATA to cache which has IE state in this address
                     Mreq *new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "O")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }
                  case 4:// 4: O exist
                  {
                     Sim->read_hit_dirty++;
                     Sim->cache_to_cache_transfers++;
                     // change current to IP
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IP");
                     // send a DATA to cache which has IE state in this address
                     Mreq *new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "M")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change M to O
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "M"), "O");
                     // printf("P%d state:  %s\n", request->src_mid.nodeID, mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state.c_str());

                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  default:
                     printf("Error case when **** GETS ****!\n");
                     break;
                  }

                  break;
               }

               case GETM:
               {
                  switch (MOFSI_GETM_Condition(mytable, request->addr, request->src_mid.nodeID))
                  {
                     printf("GETM case: %d\n", MOFSI_GETM_Condition(mytable, request->addr, request->src_mid.nodeID));
                  // 0 : trans state
                  // 1 : pure I
                  // 2 : current I with F need deter S number
                  // 3 : current I with O need deter S number
                  // 4 : current I with M
                  // 5 : current S with F need deter S number
                  // 6 : current S with O need deter S number
                  // 7 : current F need deter S number
                  // 8 : current O need deter S number
                  // 100: defaults
                  case 0: // 0 : trans state
                  {
                     // pop the front request and push it in the end of the queue
                     Mreq *temp_request = Sim->ptpnetwork->ptpNetwork_fetch(settings.num_nodes);
                     Sim->ptpnetwork->ptpNetwork_pushback(temp_request, settings.num_nodes);
                     break;
                     break;
                  }

                  case 1: // 1 : pure I
                  {
                     Sim->write_miss++;
                     // write current node to M
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "M");
                     // send DATA back
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 2: // 2 : current I with F need deter S number
                          //only here to have $ to $ transfer
                  {
                     Sim->write_hit_clean++;
                     // change current I to IM
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     // the last number send normal invalid and the other send DS invalid
                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 3: // 3 : current I with O need deter S number
                          //only here to have $ to $ transfer
                  {
                     Sim->write_hit_dirty++;
                     // change current I to IM
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");

                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 4: // 4 : current I with M
                  {
                     Sim->write_hit_dirty++;
                     // change current I to IM
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");
                     // then change M to I and get DATA back
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "M")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "M"), "I");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 5: // 5 : current S with F need deter S number
                  {
                     Sim->write_hit_clean++;
                     // change current S to IM
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 6: // 6 : current S with O need deter S number
                  {
                     Sim->write_hit_dirty++;
                     // change current S to IM
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");

                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 7: // 7 : current F need deter S number
                  {
                     Sim->write_hit_clean++;
                     // deter S num
                     // printf("\nhere???\n");
                     int S_num = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "S")
                           S_num++;
                     }

                     if (S_num == 0) // if no S
                     {
                        // send a ACK back and direct change F to M
                        Mreq *new_request = new Mreq(ACK, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "F")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "F"), "M");
                     }
                     else
                     {
                        // change All S to 0 and get nop back
                        while (S_num > 0)
                        {
                           if (S_num == 1)
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           else
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           S_num--;
                        }
                        // then change F to IM and get ACK back
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "F"), "IM");
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 8: // 8 : current O need deter S number
                  {
                     Sim->write_hit_dirty++;
                     // deter S num
                     // printf("\nhere???\n");
                     int S_num = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "S")
                           S_num++;
                     }

                     if (S_num == 0) // if no S
                     {
                        // send a ACK back and direct change O to M
                        Mreq *new_request = new Mreq(ACK, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "O")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "O"), "M");
                     }
                     else
                     {
                        // change All S to 0 and get nop back
                        while (S_num > 0)
                        {
                           if (S_num == 1)
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           else
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           S_num--;
                        }
                        // then change 0 to IM and get ACK back
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "O"), "IM");
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  default:
                     printf("Error case when **** GETM ****!\n");
                     break;
                  }
                  break;
               }

               case DATA:
               {
                  if (findnode_exist(mytable, request->addr, "IP"))
                  {
                     // send DATA to cache whiose state is IP
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IP")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IE state to S
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IP"), "S");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  else if (findnode_exist(mytable, request->addr, "IM"))
                  {
                     // send DATA to cache whiose state is IP
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IM")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IE state to S
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IM"), "M");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  break;
               }
               case ACK:
               {
                  if (findnode_exist(mytable, request->addr, "IM"))
                  {
                     Sim->invalidation++;
                     // send DATA to cache whiose state is IP
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IM")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IE state to S
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IM"), "M");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  break;
               }

               case NOP:
               {
                  Sim->invalidation++;
                  // pop the front request
                  Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  break;
               }

               default:
                  printf("wrong signal!!\n");
                  break;
               }
            }
         }
         else
         {
            // set the new request time as 100 cycles after.
            if (stall_time != Global_Clock + 100)
               stall_time = Global_Clock + 100;
            stall_direct = true;
            // add new entry for new address(read memory)
            mytable->add_Entry(request->addr, settings.num_nodes);
         }
      }
   ///////////////////////////////////////////////////////////
   ////////////////// *MOESIF* ///////////////////////////////
   ///////////////////////////////////////////////////////////
      else if (settings.protocol == MOESIF_PRO)
      {
         if (mytable->addr_exist(request->addr)) // if the address of current request is in the table
         {
            // if request time cannot be solved
            //if ((request->addr >> 6) == 0x9560c7)
            //   printState(mytable,request->addr);          
            if (int(Global_Clock) >= stall_time)
            {
               stall_direct = false;
            }
            if (stall_direct)
            {
               if (Global_Clock == 8171)
                  printf("here\n");
            }
            // if request time can be solved
            else
            {
               //if(int(Global_Clock) == 3) printf("Enter num: %d\n", request->src_mid.nodeID);
               // determine request message first
               switch (request->msg)
               {
               case GETS:
               {
                  switch (MOEFSI_GETS_Condition(mytable, request->addr, request->src_mid.nodeID))
                  {
                  // 0: trans state exist
                  // 1: pure I
                  // 2: F exist
                  // 3: E exist
                  // 4: O exist
                  // 5: M exist
                  // 100 is default
                  case 0:
                  { // 0: some states are trans state: 
                     // pop the front request and push it in the end of the queue
                     Mreq *temp_request = Sim->ptpnetwork->ptpNetwork_fetch(settings.num_nodes);
                     Sim->ptpnetwork->ptpNetwork_pushback(temp_request, settings.num_nodes);
                     break;
                  }

                  // case 100: // error state
                  // {
                  //    printf("error when GETS!!!\n");
                  //    break;
                  // }

                  case 1: // 1: pure I, current requester is in IS
                  {
                     Sim->read_miss++;
                     // change I (IS) to E directly, no need to stay in IS and wait for other data. 
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "E");
                     // send a DATA to requester to notify it to go to E
                     // no proc has the data, the memory will read for extra 100 cycles. 
                     Mreq* new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 2: // 2: F exist, current requester is in IS
                  { 
                     Sim->read_hit_clean++;
                     // change current state to IS
                     Sim->cache_to_cache_transfers ++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IS");
                     // send a GETS to F to request data forwarding. 
                     Mreq* new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "F")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 3: // 3: E exist: current requester is in IS
                  { 
                     Sim->read_hit_clean++;
                     // Change the current state to IS, change the E state to F
                     Sim->cache_to_cache_transfers ++;
                     // change current to IS
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IS");
                     // send a GETS to E to request data forwarding. 
                     Mreq* new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "E")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // Change the E state to F: 
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "F");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break; 
                  }

                  case 4: // 4: O exist: current requester is in IS
                  { 
                     Sim->read_hit_dirty++;
                     // Change the current requester state to IS
                     Sim->cache_to_cache_transfers ++;
                     // change current to IS
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IS");
                     // send a GETS to O to request data forwarding. 
                     Mreq* new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "O")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 5: // 5: M exist: current requester is in IS
                  { 
                     Sim->read_hit_dirty++;
                     // Change the current requester state to S, Change current M to O
                     Sim->cache_to_cache_transfers ++;
                     // change current to IS
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IS");
                     // send a GETS to M to request data forwarding. 
                     Mreq* new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "M")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change M to O
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "M"), "O");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  default:
                  {
                     printf("error when GETS!!!\n");
                     std::cout<<hex<<request->addr<<std::endl;
                     printState(mytable, request->addr);
                     break;
                  }
                  }
                  break;
               }

                case GETM:
               {
                  // if (request->addr == 0x647b0d40)
                  //    {
                  //       printf("\nIMPORTANT 0\n");
                  //       printState(mytable, request->addr);
                  //       printf("GETM case: %d\n", MOEFSI_GETM_Condition(mytable, request->addr, request->src_mid.nodeID));
                  //    }
                  switch (MOEFSI_GETM_Condition(mytable, request->addr, request->src_mid.nodeID))
                  {
                     
                  // 0 : trans state
                  // 1 : pure I
                  // 2 : current I with F need deter S number
                  // 3 : current I with O need deter S number
                  // 4 : current I with M
                  // 5 : current I with E
                  // 6 : current S with F need deter S number
                  // 7 : current S with O need deter S number
                  // 8 : current F need deter S number
                  // 9 : current O need deter S number
                  // 100: defaults
                  case 0: // 0 : trans state
                  {
                     // pop the front request and push it in the end of the queue
                     Mreq *temp_request = Sim->ptpnetwork->ptpNetwork_fetch(settings.num_nodes);
                     Sim->ptpnetwork->ptpNetwork_pushback(temp_request, settings.num_nodes);
                     break;

                  }

                  case 1: // 1 : pure I
                  {
                     Sim->write_miss++;
                     // write current node to M
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "M");
                     // send DATA back
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 2: // 2 : current I with F need deter S number
                          //only here to have $ to $ transfer
                  {
                     // change current I to IM
                     Sim->write_hit_clean++;
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     // the last number send normal invalid and the other send DS invalid
                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 3: // 3 : current I with O need deter S number
                          //only here to have $ to $ transfer
                  {
                     Sim->write_hit_dirty++;
                     // change current I to IM
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");

                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 4: // 4 : current I with M
                  {
                     // change current I to IM
                     Sim->write_hit_dirty++;
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");
                     // then change M to I and get DATA back
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "M")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "M"), "I");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 5: // 5 : current I with E
                  {
                     // change current I to IM
                     Sim->write_hit_clean++;
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");
                     // then change M to I and get DATA back
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "E")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "I");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 6: // 6 : current S with F need deter S number
                  {
                     Sim->write_hit_clean++;
                     // change current S to IM
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 7: // 7 : current S with O need deter S number
                  {
                     Sim->write_hit_dirty++;
                     // change current S to IM
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");

                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 8: // 8 : current F need deter S number
                  {
                     Sim->write_hit_clean++;
                     int S_num = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "S")
                           S_num++;
                     }

                     if (S_num == 0) // if no S
                     {
                        // send a DATA back and direct change IM to M
                        Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "F")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "F"), "M");
                     }
                     else
                     {
                        // change All S to I and wait nop/ack back
                        while (S_num > 0)
                        {
                           if (S_num == 1)
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           else
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           S_num--;
                        }
                        // then change F to IM and wait for ACK back
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "F"), "IM");
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 9: // 9 : current O need deter S number
                  {
                     Sim->write_hit_dirty++;
                     int S_num = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "S")
                           S_num++;
                     }

                     if (S_num == 0) // if no S
                     {
                        // send a DATA back and direct change O to M
                        Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "O")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "O"), "M");
                     }
                     else
                     {
                        // change All S to I and wait nop/ack back
                        while (S_num > 0)
                        {
                           if (S_num == 1)
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           else
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           S_num--;
                        }
                        // then change 0 to IM and get ACK back
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "O"), "IM");
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }
                  
                  case 10: // 10 : current E
                  {
                     if (findnode_exist(mytable, request->addr, "E"))
                     {
                        Sim->silent_upgrades++;
                        Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "E")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        // change E state to M
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "M");
                        // pop the front request
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                     else if (findnode_exist(mytable, request->addr, "M"))
                     {
                        // pop the front request
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                     // else if (findnode_exist(mytable, request->addr, "F"))
                     // {
                     //    // change current requester F to IM
                     //    printf("E has been stolen !");
                     //    mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");
                     //    int max_node = 0;
                     //    for (int i = 0; i < settings.num_nodes; i++)
                     //    {
                     //       if (mytable->access_entry(request->addr).access_block(i).state == "S")
                     //       {
                     //          if (i > max_node)
                     //             max_node = i;
                     //       }
                     //    }
                     //    Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     //    this->write_output_port(new_request);
                     //    mytable->access_entry(request->addr).State_change(max_node, "I");

                     //    for (int i = 0; i < settings.num_nodes; i++)
                     //    {
                     //       if (mytable->access_entry(request->addr).access_block(i).state == "S")
                     //       {
                     //          Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                     //          this->write_output_port(new_request);
                     //          mytable->access_entry(request->addr).State_change(i, "I");
                     //       }
                     //    }
                     //    // pop the front request
                     //    Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     //    break;
                     // }
                  }

                  default:
                  {
                     printf("Error case when **** GETM ****!\n");
                     printState(mytable,request->addr);
                     break;
                  }
                     
                  }
                  break;
               }

               case DATA:
               {
                  // if (findnode_exist(mytable, request->addr, "IM"))
                  // {
                  //    // send ACK to cache whiose state is IM
                  //    Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IM")]->mod[PR_M]->moduleID);
                  //    this->write_output_port(new_request);
                  //    // change IM state to M
                  //    mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IM"), "M");
                  //    // pop the front request
                  //    Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  // }
                  if (findnode_exist(mytable, request->addr, "IS"))
                  {
                     // send DATA to cache whiose state is IS
                     Mreq *new_request = new Mreq(DATA_S, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IS")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IE state to S
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IS"), "S");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  break;
               }

               case ACK: // The last processor receive INVALID and send ACK back
               {
                  if (findnode_exist(mytable, request->addr, "IM"))
                  {
                     Sim->invalidation++;
                     // send ACK to cache whiose state is IM
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IM")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IM state to M
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IM"), "M");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  break;
               }

               // case Silent_Up:
               // {
               //    printf("cycle: %d\n", int(Global_Clock));
               //    if (findnode_exist(mytable, request->addr, "E")) 
               //    {
               //       if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "E")
               //       {
               //          // change E state to M
               //          mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "M");
               //          // pop the front request
               //          Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
               //       }
               //    }
               //    else
               //       printf("State Error when ***Silent***! \n");
               //    break;
               // }

               case NOP: // The other processors receive INVALID_DS and send NOP back
               {
                  // pop the front request
                  Sim->invalidation++;
                  Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  break;
               }

               default:
                  printf("     Invalid message!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
                  break;
               }
            }
         }
         else // if the address of current request is not in the table (not in the cache system)
         {
            // set the new request time as 100 cycles after.
            if (stall_time != Global_Clock + 100)
               stall_time = Global_Clock + 100;
            stall_direct = true;
            Sim->ptpnetwork->ptpNetwork_read(settings.num_nodes)->req_time = Global_Clock + 100;
            // build a new block for the request
            //printf("cycle: %d\n", int(Global_Clock));
            mytable->add_Entry(request->addr, settings.num_nodes);

            printf("request node: %d    req_time cycle: %llu\n", request->src_mid.nodeID, request->req_time);
         }
      }
   ///////////////////////////////////////////////////////////
   ////////////////// *Improved MOESIF* //////////////////////
   ///////////////////////////////////////////////////////////
      else if (settings.protocol == Improved_MOESIF_PRO)
      {
         if (mytable->addr_exist(request->addr)) // if the address of current request is in the table
         {
            // if request time cannot be solved
            //if ((request->addr >> 6) == 0x9560c7)
            //   printState(mytable,request->addr);          
            if (int(Global_Clock) >= stall_time)
            {
               stall_direct = false;
            }
            if (stall_direct)
            {
               if (Global_Clock == 8171)
                  printf("here\n");
            }
            // if request time can be solved
            else
            {
               //if(int(Global_Clock) == 3) printf("Enter num: %d\n", request->src_mid.nodeID);
               // determine request message first
               switch (request->msg)
               {
               case GETS: // of Improved MOESIF
               {
                  switch (MOEFSI_GETS_Condition(mytable, request->addr, request->src_mid.nodeID))
                  {
                  // 0: trans state exist
                  // 1: pure I
                  // 2: F exist
                  // 3: E exist
                  // 4: O exist
                  // 5: M exist
                  // 100 is default
                  case 0:
                  { // 0: some states are trans state: 
                     // pop the front request and push it in the end of the queue
                     Mreq *temp_request = Sim->ptpnetwork->ptpNetwork_fetch(settings.num_nodes);
                     Sim->ptpnetwork->ptpNetwork_pushback(temp_request, settings.num_nodes);
                     break;
                  }

                  // case 100: // error state
                  // {
                  //    printf("error when GETS!!!\n");
                  //    break;
                  // }

                  case 1: // 1: pure I, current requester is in IS
                  {
                     Sim->read_miss++;
                     // change I (IS) to E directly, no need to stay in IS and wait for other data. 
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "E");
                     // send a DATA _E to requester to notify it to go to E
                     // no proc has the data, the memory will read for extra 100 cycles. 
                     Mreq* new_request = new Mreq(DATA_E, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 2: // 2: F exist, current requester is in IS
                  { 
                     Sim->read_hit_clean++;
                     // change current state to IS
                     Sim->cache_to_cache_transfers ++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IS");
                     // send a GETS to F to request data forwarding. 
                     Mreq* new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "F")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // Change the F state to S: 
                     //mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "F"), "S");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 3: // 3: E exist: current requester is in IS
                  { 
                     Sim->read_hit_clean++;
                     // Change the current state to IS, change the E state to F
                     Sim->cache_to_cache_transfers ++;
                     // change current to IS
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IS");
                     // send a GETS to E to request data forwarding. 
                     Mreq* new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "E")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // Change the E state to F: 
                     //mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "F");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break; 
                  }

                  case 4: // 4: O exist: current requester is in IS
                  { 
                     Sim->read_hit_dirty++;
                     // Change the current requester state to IS
                     Sim->cache_to_cache_transfers ++;
                     // change current to IS
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IS");
                     // send a GETS to O to request data forwarding. 
                     Mreq* new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "O")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // Change the O state to S: 
                     //mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "O"), "S");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 5: // 5: M exist: current requester is in IS
                  { 
                     Sim->read_hit_dirty++;
                     // Change the current requester state to S, Change current M to O
                     Sim->cache_to_cache_transfers ++;
                     // change current to IS
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IS");
                     // send a GETS to M to request data forwarding. 
                     Mreq* new_request = new Mreq(GETS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "M")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change M to O
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "M"), "O");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  default:
                  {
                     printf("error when GETS!!!\n");
                     std::cout<<hex<<request->addr<<std::endl;
                     printState(mytable, request->addr);
                     break;
                  }
                  }
                  break;
               }

                case GETM: // of Improved MOESIF
               {
                  // if (request->addr == 0x647b0d40)
                  //    {
                  //       printf("\nIMPORTANT 0\n");
                  //       printState(mytable, request->addr);
                  //       printf("GETM case: %d\n", MOEFSI_GETM_Condition(mytable, request->addr, request->src_mid.nodeID));
                  //    }
                  switch (MOEFSI_GETM_Condition(mytable, request->addr, request->src_mid.nodeID))
                  {
                     
                  // 0 : trans state
                  // 1 : pure I
                  // 2 : current I with F need deter S number
                  // 3 : current I with O need deter S number
                  // 4 : current I with M
                  // 5 : current I with E
                  // 6 : current S with F need deter S number
                  // 7 : current S with O need deter S number
                  // 8 : current F need deter S number
                  // 9 : current O need deter S number
                  // 100: defaults
                  case 0: // 0 : trans state
                  {
                     // pop the front request and push it in the end of the queue
                     Mreq *temp_request = Sim->ptpnetwork->ptpNetwork_fetch(settings.num_nodes);
                     Sim->ptpnetwork->ptpNetwork_pushback(temp_request, settings.num_nodes);
                     break;

                  }

                  case 1: // 1 : pure I
                  {
                     Sim->write_miss++;
                     // write current node to M
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "M");
                     // send DATA back
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[request->src_mid.nodeID]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 2: // 2 : current I with F need deter S number
                          //only here to have $ to $ transfer
                  {
                     // change current I to IM
                     Sim->write_hit_clean++;
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     // the last number send normal invalid and the other send DS invalid
                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 3: // 3 : current I with O need deter S number
                          //only here to have $ to $ transfer
                  {
                     Sim->write_hit_dirty++;
                     // change current I to IM
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");

                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 4: // 4 : current I with M
                  {
                     // change current I to IM
                     Sim->write_hit_dirty++;
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");
                     // then change M to I and get DATA back
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "M")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "M"), "I");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 5: // 5 : current I with E
                  {
                     // change current I to IM
                     Sim->write_hit_clean++;
                     Sim->cache_to_cache_transfers++;
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");
                     // then change M to I and get DATA back
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "E")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "I");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 6: // 6 : current S with F need deter S number
                  {
                     Sim->write_hit_clean++;
                     // change current S to IM
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "F" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 7: // 7 : current S with O need deter S number
                  {
                     Sim->write_hit_dirty++;
                     // change current S to IM
                     mytable->access_entry(request->addr).State_change(request->src_mid.nodeID, "IM");

                     int max_node = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           if (i > max_node)
                              max_node = i;
                        }
                     }
                     Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[max_node]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     mytable->access_entry(request->addr).State_change(max_node, "I");

                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "O" || mytable->access_entry(request->addr).access_block(i).state == "S")
                        {
                           Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[i]->mod[PR_M]->moduleID);
                           this->write_output_port(new_request);
                           mytable->access_entry(request->addr).State_change(i, "I");
                        }
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 8: // 8 : current F need deter S number
                  {
                     Sim->write_hit_clean++;
                     int S_num = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "S")
                           S_num++;
                     }

                     if (S_num == 0) // if no S
                     {
                        // send a DATA back and direct change IM to M
                        Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "F")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "F"), "M");
                     }
                     else
                     {
                        // change All S to I and wait nop/ack back
                        while (S_num > 0)
                        {
                           if (S_num == 1)
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           else
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           S_num--;
                        }
                        // then change F to IM and wait for ACK back
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "F"), "IM");
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }

                  case 9: // 9 : current O need deter S number
                  {
                     Sim->write_hit_dirty++;
                     int S_num = 0;
                     for (int i = 0; i < settings.num_nodes; i++)
                     {
                        if (mytable->access_entry(request->addr).access_block(i).state == "S")
                           S_num++;
                     }

                     if (S_num == 0) // if no S
                     {
                        // send a DATA back and direct change O to M
                        Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "O")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "O"), "M");
                     }
                     else
                     {
                        // change All S to I and wait nop/ack back
                        while (S_num > 0)
                        {
                           if (S_num == 1)
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           else
                           {
                              Mreq *new_request = new Mreq(MREQ_INVALID_DS, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "S")]->mod[PR_M]->moduleID);
                              this->write_output_port(new_request);
                              mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "S"), "I");
                           }
                           S_num--;
                        }
                        // then change 0 to IM and get ACK back
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "O"), "IM");
                     }
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     break;
                  }
                  
                  case 10: // 10 : current E
                  {
                     if (findnode_exist(mytable, request->addr, "E"))
                     {
                        Sim->silent_upgrades++;
                        Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "E")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        // change E state to M
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "M");
                        // pop the front request
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                     else if (findnode_exist(mytable, request->addr, "M"))
                     {
                        // pop the front request
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                  }

                  default:
                  {
                     printf("Error case when **** GETM ****!\n");
                     printState(mytable,request->addr);
                     break;
                  }
                     
                  }
                  break;
               }
               
               case DATA: // of Improved MOESIF
               {
                  if (findnode_exist(mytable, request->addr, "IS"))
                  {
                     if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "F") // if current responder state is F
                     {
                        // send DATA to cache whiose state is IS
                        Mreq *new_request = new Mreq(DATA_F, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IS")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        // Change current requester state to S: 
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "F"), "S");
                        // change IS state to S:
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IS"), "F");
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                     else if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "E") // if current responder state is E
                     {
                        // send DATA to cache whiose state is IS
                        Mreq *new_request = new Mreq(DATA_S, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IS")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        // Change the E state to F: 
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "E"), "F");
                        // change IS state to S:
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IS"), "S");
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                     else if (mytable->access_entry(request->addr).access_block(request->src_mid.nodeID).state == "O") // if current responder state is O
                     {
                        // send DATA to cache whiose state is IS
                        Mreq *new_request = new Mreq(DATA_O, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IS")]->mod[PR_M]->moduleID);
                        this->write_output_port(new_request);
                        // Change current requester state to S: 
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "O"), "S");
                        // change IS state to S:
                        mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IS"), "O");
                        Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                     }
                     
                  }
                  else
                  {
                     printf("Error when receiving ****DATA****");
                  }
                  
                  break;
               }

               case ACK: // The last processor receive INVALID and send ACK back
               {
                  if (findnode_exist(mytable, request->addr, "IM"))
                  {
                     Sim->invalidation++;
                     // send ACK to cache whiose state is IM
                     Mreq *new_request = new Mreq(DATA, request->addr, moduleID, Sim->Nd[findnode(mytable, request->addr, "IM")]->mod[PR_M]->moduleID);
                     this->write_output_port(new_request);
                     // change IM state to M
                     mytable->access_entry(request->addr).State_change(findnode(mytable, request->addr, "IM"), "M");
                     // pop the front request
                     Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  }
                  break;
               }

               case NOP: // The other processors receive INVALID_DS and send NOP back
               {
                  // pop the front request
                  Sim->invalidation++;
                  Sim->ptpnetwork->ptpNetwork_pop(settings.num_nodes);
                  break;
               }

               default:
                  printf("     Invalid message!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
                  break;
               }
            }
         }
         else // if the address of current request is not in the table (not in the cache system)
         {
            // set the new request time as 100 cycles after.
            if (stall_time != Global_Clock + 100)
               stall_time = Global_Clock + 100;
            stall_direct = true;
            Sim->ptpnetwork->ptpNetwork_read(settings.num_nodes)->req_time = Global_Clock + 100;
            // build a new block for the request
            //printf("cycle: %d\n", int(Global_Clock));
            mytable->add_Entry(request->addr, settings.num_nodes);

            printf("request node: %d    req_time cycle: %llu\n", request->src_mid.nodeID, request->req_time);
         }
      }
   }
}

void Memory_controller::tock()
{
   fatal_error("Memory controller tock should never be called!\n");
}
