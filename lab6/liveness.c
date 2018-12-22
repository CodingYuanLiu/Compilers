#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}


Temp_temp Live_gtemp(G_node n) {
	Temp_temp t = G_nodeInfo(n);
	return t;
}

bool Live_isinMoveList(G_node src,G_node dst,Live_moveList tail)
{
	for(;tail;tail = tail->tail)
	{
		if(src == tail->src && dst == tail->dst)
		{
			return TRUE;
		}
	}
	return FALSE;
}

struct Live_graph Live_liveness(G_graph flow) {
	//your code here.
	struct Live_graph lg;

	/* Firstly, construct the livemap.*/
	
	/* Record the sets of in[n] and out[n]*/
	/* Implement the pseudo codes of algorithm 10-1 on the textbook*/
	G_table in = G_empty();
	G_table out = G_empty();
	
	/* Init in,out sets with empty templist*/
	G_nodeList flownodes = G_nodes(flow);
	G_node flownode;
	for(;flownodes;flownodes = flownodes->tail)
	{
		flownode = flownodes->head;
		G_enter(in,flownode,checked_malloc(sizeof(Temp_tempList))); //Don't forget the TempList may be empty
		G_enter(out,flownode,checked_malloc(sizeof(Temp_tempList)));
	}
	
	bool fixed_point = FALSE;
	while(!fixed_point){
		fixed_point = TRUE;
		flownodes = G_nodes(flow);
		for(;flownodes;flownodes = flownodes->tail)
		{
			flownode = flownodes->head;
			Temp_tempList in_n_old = *(Temp_tempList *)G_look(in,flownode);
			Temp_tempList out_n_old = *(Temp_tempList *)G_look(out,flownode);
			Temp_tempList in_n = NULL,out_n = NULL;
			/* pseudocode:
			in[n] = use[n] and (out[n] – def[n])
 			out[n] = (for s in succ[n])
			            Union(in[s])
			*/
			G_nodeList succ = G_succ(flownode);
			for(;succ;succ = succ->tail)
			{
				Temp_tempList in_succ = *(Temp_tempList *)G_look(in,succ->head);
				out_n = UnionSets(out_n,in_succ);
			}
			in_n = UnionSets(FG_use(flownode),
						SubSets(out_n,FG_def(flownode)));

			if(!tempequal(in_n_old,in_n) || !tempequal(out_n_old,out_n))
			{
				fixed_point = FALSE;
			}
			/* Update in_n and out_n*/
			*(Temp_tempList *)G_look(in,flownode) = in_n;
			*(Temp_tempList *)G_look(out,flownode) = out_n;
			
		}
	}

	/* Then use the result of the livemap to construct interfere graph */
	/* Construct the graph */
	lg.graph = G_Graph();
	lg.moves = NULL;

	/* Add hardregister to the graph */
	TAB_table tempTab = TAB_empty();
	Temp_tempList hardreg = Temp_TempList(F_RAX(), Temp_TempList(F_RBX(),
							Temp_TempList(F_RCX(), Temp_TempList(F_RDX(),
							Temp_TempList(F_RSI(), Temp_TempList(F_RDI(),
							Temp_TempList(F_R8(), Temp_TempList(F_R9(),
							Temp_TempList(F_R10(), Temp_TempList(F_R11(),
							Temp_TempList(F_R12(), Temp_TempList(F_R13(),
							Temp_TempList(F_R14(), Temp_TempList(F_R15(), 
							Temp_TempList(F_RBP(),NULL)))))))))))))));
	/* Add nodes of hard register into temp table*/
	for(Temp_tempList temp = hardreg;temp;temp = temp->tail)
	{
		G_node Tempnode = G_Node(lg.graph,temp->head);
		TAB_enter(tempTab,temp->head,Tempnode);
	}
	/* Add edge between hard register node. */
	for(Temp_tempList A = hardreg; A; A = A->tail )
	{
		for(Temp_tempList B = A->tail; B; B = B->tail)
		{
			G_node nodeA = TAB_look(tempTab,A->head);
			G_node nodeB = TAB_look(tempTab,B->head);
			G_addEdge(nodeA,nodeB);
			G_addEdge(nodeB,nodeA);
		}
	}

	/* Add nodes of temp register into temp table*/
	flownodes = G_nodes(flow);

	for(;flownodes;flownodes = flownodes->tail)
	{
		flownode = flownodes -> head;
		Temp_tempList outn = *(Temp_tempList *)G_look(out,flownode),
		def = FG_def(flownode);
		if(!(def && def->head))
		{
			continue;
		}
		Temp_tempList temps = UnionSets(outn,def);
		for(;temps;temps = temps->tail)
		{
			if(temps->head == F_SP())
			{
				continue;
			}
			// If the temp is not in the map, add it.
			if(!TAB_look(tempTab,temps->head))
			{
				G_node tempNode = G_Node(lg.graph,temps->head);
				TAB_enter(tempTab,temps->head,tempNode);
			}
		}
	}

	/* Add edge between temp register node. */
	flownodes = G_nodes(flow);
	for(;flownodes;flownodes = flownodes->tail)
	{
		flownode = flownodes->head;
		Temp_tempList outn = *(Temp_tempList *)G_look(out,flownode),
		def = FG_def(flownode);
		if(!(def && def->head))
		{
			continue;
		}
		/* Add edge to the interfere graph,firstly check if the instruction
		 of the flownode is MOVE. */
		/* If the instruction is not move, add edge directly.*/
		if(!FG_isMove(flownode))
		{
			for(;def;def = def->tail)
			{
				if(def->head == F_SP())
				{
					continue;
				}
				G_node defnode = TAB_look(tempTab,def->head);				
				for(;outn;outn = outn->tail)
				{
					if(outn->head == F_SP())
					{
						continue;
					}
					G_node outnode = TAB_look(tempTab,outn->head);
					/* If outnode and defnode is not linked yet.*/
					if(!G_inNodeList(defnode,G_adj(outnode)))
					{
						G_addEdge(defnode,outnode);
						G_addEdge(outnode,defnode);
					}
				}
			}
		}
		/* Else,the instruction is MOVE*/
		else
		{
			for(;def;def = def->tail)
			{
				if(def->head == F_SP())
				{
					continue;
				}
				G_node defnode = TAB_look(tempTab,def->head);
				for(;outn;outn = outn->tail)
				{
					if(outn->head == F_SP())
					{
						continue;
					}
					G_node outnode = TAB_look(tempTab,outn->head);
				
					if(!(G_inNodeList(defnode,G_adj(outnode))) && !(intemp(FG_use(flownode),outn->head)))
					{
						G_addEdge(outnode,defnode);
						G_addEdge(defnode,outnode);
					}			
				}
			
				/* Add it to the movelist. It's for coalescence in ra.*/
				for(Temp_tempList uses = FG_use(flownode);uses;uses = uses->tail)
				{
					if(uses->head == F_SP())
					{
						continue;
					}
					G_node usenode = TAB_look(tempTab,uses->head);
					if(!Live_isinMoveList(usenode,defnode,lg.moves))
					{
						lg.moves = Live_MoveList(usenode,defnode,lg.moves);
					}
				}
			}
		}
	}
	return lg;
}

Temp_tempList UnionSets(Temp_tempList left,Temp_tempList right)
{
	Temp_tempList unionn = left;
	for(;right;right = right->tail)
	{
		if(!intemp(unionn,right->head))
			unionn = Temp_TempList(right->head,unionn);
	}
	return unionn;
}

Temp_tempList SubSets(Temp_tempList left, Temp_tempList right)
{
	Temp_tempList sub = NULL;
	for(;left;left = left->tail)
	{
		if(!intemp(right,left->head))
		{
			sub = Temp_TempList(left->head,right);
		}
		
	}
	return sub;
}

bool intemp(Temp_tempList list,Temp_temp temp)
{
	for(;list;list = list->tail)
	{
		if(list->head == temp)
			return TRUE;
	}
	return FALSE;
}

bool tempequal(Temp_tempList old,Temp_tempList neww)
{
	Temp_tempList cur = old;
	
	for(;cur;cur = cur->tail)
	{
		if(!intemp(neww,cur->head))
		{
			return FALSE;
		}
	}
	cur = neww;
	for(;cur;cur = cur->tail)
	{
		if(!intemp(old,cur->head))
			return FALSE;
	}
	return TRUE;
}

Live_moveList Live_IntersectMoveList(Live_moveList left,Live_moveList right)
{
	Live_moveList cur = NULL;
	for(;left;left = left->tail)
	{
		if(Live_isinMoveList(left->src,left->dst,right))
		{
			cur = Live_MoveList(left->src,left->dst,cur);
		}
	}
	return cur;
}

Live_moveList Live_UnionMoveList(Live_moveList left,Live_moveList right)
{
	Live_moveList cur = left;
	for(;right;right = right->tail)
	{
		if(!Live_isinMoveList(right->src,right->dst,left))
		{
			cur = Live_MoveList(right->src,right->dst,cur);
		}
	}
	return cur;
}

Live_moveList Live_SubMoveList(Live_moveList left,Live_moveList right)
{
	Live_moveList cur = NULL;
	for(;left;left = left->tail)
	{
		if(!Live_isinMoveList(left->src,left->dst,right))
		{
			cur = Live_MoveList(left->src,left->dst,cur);
		}
	}
	return cur;
}
