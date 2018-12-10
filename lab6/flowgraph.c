#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

Temp_tempList FG_def(G_node n) {
	AS_instr instr = G_nodeInfo(n);
	switch(instr->kind)
	{
		case I_OPER:
			return instr->u.OPER.dst;
		case I_LABEL:
			return NULL;
		case I_MOVE:
			return instr->u.MOVE.dst;
	}	
}

Temp_tempList FG_use(G_node n) {
	AS_instr instr = G_nodeInfo(n);
	switch(instr->kind)
	{
		case I_OPER:
			return instr->u.OPER.src;
		case I_LABEL:
			return NULL;
		case I_MOVE:
			return instr->u.MOVE.src;
	}	
}

bool FG_isMove(G_node n) {
	AS_instr instr = G_nodeInfo(n);
	return instr->kind == I_MOVE;
}
G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) 
{
	G_graph flowgragh = G_Graph();
	TAB_table labelmap = TAB_empty();
	AS_instrList instrl;
	AS_instr instr;
	G_node prev = NULL, cur = NULL;

	/* Add Nodes and edges between sequential instructions to the flowgraph first */
	for(instrl = il;instrl;instrl = instrl->tail)
	{
		instr = instrl->head;
		cur = G_Node(flowgragh,instr);
		if(!prev)
		{
			G_addEdge(prev,cur);
		}

		/* Add labels to the TABtable */
		if(instr->kind == I_LABEL)
		{
			TAB_enter(labelmap,instr->u.LABEL.label,cur);
		}

		/* If the current instruction is jmp xxx, then do not addedge between current instruction
		and the next instruction, because after jmp xxx, the next instruction will not be executed. */
		if(instr->kind == I_OPER && strncmp("jmp",instr->u.OPER.assem,3))
		{
			prev = NULL;
			continue;
		}
		
		prev = cur;
	}

	/* Add edges between jump instructions first*/
	G_nodeList nodes = G_nodes(flowgragh);

	G_node target;
	for(;nodes;nodes = nodes->tail)
	{
		cur = nodes->head;
		instr = G_nodeInfo(cur);
		if(instr->kind == I_OPER && instr->u.OPER.jumps->labels)
		{
			Temp_tempList labels = instr->u.OPER.jumps->labels;
			for(;labels;labels = labels->tail)
			{
				target = TAB_look(labelmap,labels->head);
				G_addEdge(cur,target);
			}
		}
	}
	return flowgragh;
}
