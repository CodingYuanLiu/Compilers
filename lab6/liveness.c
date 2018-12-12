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
	//your code here.
	return NULL;
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
		G_enter(in,flownode,Temp_TempList(NULL,NULL)); //Don't forget the TempList may be empty
		G_enter(out,flownode,Temp_TempList(NULL,NULL));
	}

	bool fixed_point = FALSE;
	do{
		flownodes = G_nodes(flow);
		for(;flownodes;flownodes = flownodes->tail)
		{
			flownode = flownodes->head;
			Temp_tempList in_n_old = G_look(in,flownode);
			Temp_tempList out_n_old = G_look(out,flownode);
			Temp_tempList in_n = NULL,out_n = NULL;
			/* pseudocode:
			in[n] = use[n] and (out[n] – def[n])
 			out[n] = (for s in succ[n])
			            Union(in[s])
			*/
			G_nodeList succ = G_succ(flownode);
			for(;succ;succ = succ->tail)
			{
				out_n = UnionSets(out_n,G_look(in,succ->head));
			}
			in_n = UnionSets(FG_use(flownode),
						SubSets(out_n,FG_def(flownode)));

			if(tempequal(in_n_old,in_n) && tempequal(out_n_old,out_n))
			{
				fixed_point = TRUE;
				break;
			}
			/* Update in_n and out_n*/
			{
				*(Temp_tempList *)G_look(in,flownode) = in_n;
				*(Temp_tempList *)G_look(out,flownode) = out_n;
			}
		}
	}while(!fixed_point);

	return lg;
}

Temp_tempList UnionSets(Temp_tempList left,Temp_tempList right)
{
	for(;right;right = right->tail)
	{
		if(!right->head)//Right is empty
		{
			return left;
		}
		else
		{
			if(!intemp(left,right->head))
				left = Temp_TempList(right->head,left);
		}
	}
	return left;
}

bool intemp(Temp_tempList list,Temp_temp temp)
{
	for(;list;list = list->tail)
	{
		if(!list->head)
			return FALSE;
		else
		{
			if(list->head == temp)
			return TRUE;
		}
	}
	return FALSE;
}

Temp_tempList SubSets(Temp_tempList left, Temp_tempList right)
{
	Temp_tempList new = NULL;
	
}