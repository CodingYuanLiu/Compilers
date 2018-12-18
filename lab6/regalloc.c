#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"

#define K 15
static struct Live_graph live;

static bool precolor(G_node n)
{
	return (*(int *)G_look(colorTab,n) != 0);
}



/*============================== Procedure Functions ===============================*/
struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here
	struct RA_result ret;
	bool done = FALSE;
	do{
		done = TRUE;
		/*Liveness Analysis*/
		G_graph fg = FG_AssemFlowGraph(il,f);
	    live = Live_liveness(fg);

		Build();
		
		MakeWorklist();

		while(simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist)
		{
			if(simplifyWorklist)
			{
				Simplify();
			}
			else if(worklistMoves)
			{
				Coalesce();
			}
			else if(freezeWorklist)
			{
				Freeze();
			}
			else if(spillWorklist)
			{
				SelectSpill();
			}
		}

		AssignColors();

		if(spilledNodes)
		{
			RewriteProgram(f,&il);
			done = FALSE;
		}

	}while(!done);
	ret.coloring = AssignRegisters(live);
	ret.il = il;
	return ret;
}

static void Build()
{
	simplifyWorklist = NULL;
	freezeWorklist = NULL;
	spillWorklist = NULL;

	spilledNodes = NULL;
	coalescedNodes = NULL;
	coloredNodes = NULL;
	selectStack = NULL;
	//precolored

	coalescedMoves = NULL;
	constrainedMoves = NULL;
	frozenMoves = NULL;
	worklistMoves = live.moves;
	activeMoves = NULL;

	degreeTab = G_empty();
	colorTab = G_empty();
	aliasTab = G_empty();
	moveListTab = G_empty();

	G_nodeList nodes = G_nodes(live.graph);
	for(;nodes; nodes = nodes->tail)
	{
		/* Initial degree */
		int *degree = checked_malloc(sizeof(int));
		*degree = 0;
		for(G_nodeList cur = G_succ(nodes->head);cur ; cur = cur->tail)
		{
			(*degree)++;
		}
		G_enter(degreeTab,nodes->head,degree);

		/* Initial color */
		int *color = checked_malloc(sizeof(int));
		Temp_temp temp = Live_gtemp(nodes->head);
		if(temp == F_RAX())				*color = 1;
		else if(temp == F_RBX())		*color = 2;
		else if (temp = F_RCX())  		*color = 3;
		else if (temp = F_RDX())  		*color = 4;
		else if (temp = F_RSI())  		*color = 5;
		else if (temp = F_RDI())  		*color = 6;
		else if (temp = F_RBP())		*color = 7;
		else if (temp = F_R8())   		*color = 8;
		else if (temp = F_R9())   		*color = 9;
		else if (temp = F_R10())  		*color = 10;
		else if (temp = F_R11())  		*color = 11;
		else if (temp = F_R12())  		*color = 12;
		else if (temp = F_R13())  		*color = 13;
		else if (temp = F_R14())  		*color = 14;
		else if (temp = F_R15()) 		*color = 15;
		else 							*color = 0; //Temp register
		G_enter(colorTab,nodes->head,color);
		
		/* Initial alias table */
		G_node *alias = checked_malloc(sizeof(G_node));
		*alias = nodes->head;
		G_enter(aliasTab,nodes->head,alias);

		/* Initial movelist table*/
		Live_moveList list = worklistMoves;
		Live_moveList *movelist = NULL;
		for(;list;list = list->tail)
		{
			if(list->src == nodes->head || list->dst == nodes->head)
			{
				*movelist = Live_MoveList(list->src,list->dst,*movelist);
			}
		}
		G_enter(moveListTab,nodes->head,movelist);

	}
}

static void MakeWorklist()
{
	G_nodeList nodes = G_nodes(live.graph);
	for(;nodes;nodes = nodes->tail)
	{
		G_node node = nodes->head;
		int *degree = G_look(degreeTab,node);
		int *color = G_look(colorTab,node);
		//Reject precolored register
		if(*color != 0){
			continue;
		}
		if(*degree >= K)
		{
			spillWorklist = G_NodeList(node,spillWorklist);
		}
		else if(MoveRelated(node))
		{
			freezeWorklist = G_NodeList(node,freezeWorklist);
		}
		else
		{
			simplifyWorklist = G_NodeList(node,simplifyWorklist);
		}
	}
}

static G_nodeList Adjacent(G_node n)
{
	/* adjList[n] \ (selectStack U coalescedNodes */
	G_nodeList adjList = G_succ(n);
	return G_SubNodeList(adjList,G_UnionNodeList(selectStack,coalescedNodes));
}

static Live_moveList NodeMoves(G_node n)
{
	Live_moveList *movelist = G_look(moveListTab,n);
	return Live_IntersectMoveList((*movelist),Live_UnionMoveList(activeMoves,worklistMoves));
}

static bool MoveRelated(G_node n)
{
	return NodeMoves(n) != NULL;
}

static void Simplify()
{
	G_node node = simplifyWorklist->head;
	simplifyWorklist = simplifyWorklist->tail;
	//push n to stack
	selectStack = G_NodeList(node,selectStack);
	G_nodeList adj = G_adj(node);
	for(;adj;adj = adj->tail)
	{
		DecrementDegree(adj->head);
	}
}

static void DecrementDegree(G_node n)
{
	int *degree = G_look(degreeTab,n);
	int d = *degree;// Optimization
	*degree = *degree - 1;
	int* color = G_look(colorTab,n);
	if(d == K && *color != 0)
	{
		EnableMoves(G_NodeList(n,Adjacent(n)));
		spillWorklist = G_SubNodeList(spillWorklist,G_NodeList(n,NULL));
		if(MoveRelated(n))
		{
			freezeWorklist = G_NodeList(n,freezeWorklist);
		}
		else{
			simplifyWorklist = G_NodeList(n,simplifyWorklist);
		}
	}
}

static void EnableMoves(G_nodeList nodes)
{
	for(;nodes;nodes = nodes->tail)
	{
		for(Live_moveList m = NodeMoves(nodes->head);m;m = m->tail)
		{
			if(Live_isinMoveList(activeMoves,m->src,m->dst))
			{
				activeMoves = Live_SubMoveList(activeMoves,Live_MoveList(m->src,m->dst,NULL));
				worklistMoves = Live_MoveList(m->src,m->dst,worklistMoves);	
			}
		}
	}
}

/* coalesce instructions in worklistMoves.*/
static void Coalesce()
{
	G_node x,y,u,v;
	x = worklistMoves->src;
	y = worklistMoves->dst;

	//if y is in precolored
	if(precolor(GetAlias(y)))
	{
		u = GetAlias(y);
		v = GetAlias(x);
	}
	else
	{
		u = GetAlias(x);
		v = GetAlias(y);
	}
	worklistMoves = worklistMoves->tail;
	
	if(u == v)
	{
		coalescedMoves = Live_MoveList(x,y,coalescedMoves);
		AddWorkList(u);
	}
	else if (precolor(v) && G_inNodeList(u,adj(v)))
	{
		
	}
}

