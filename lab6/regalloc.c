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

/*============================== Tool Functions ===============================*/
static bool precolor(G_node n)
{
	return (*(int *)G_look(colorTab,n) != 0);
}

static int degree(G_node n)
{
	return *(int *)G_look(degreeTab,n);
}

static G_node alias(G_node n)
{
	return *(G_node *)G_look(aliasTab,n);
}

static Live_moveList movelist(G_node n)
{
	return *(Live_moveList *)G_look(moveListTab,n);
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
	/****************** WorkList ****************/
	//All the nodes are in those worklist	
	//degree < K and not move related nodes
	simplifyWorklist = NULL;
	//degree < K but move related nodes
	freezeWorklist = NULL;
	//degree >= K
	spillWorklist = NULL;

	spilledNodes = NULL;
	coalescedNodes = NULL;
	coloredNodes = NULL;
	
	selectStack = NULL;

	/****************** Move Sets ****************/
	//All move instructions is in one of the sets below.
	//Already coalesced moves
	coalescedMoves = NULL;
	//Moves whose src and dst interfere.
	constrainedMoves = NULL;
	//Moves don't need to coalesce.
	frozenMoves = NULL;
	//Moves likely to be coalesced later.
	worklistMoves = live.moves;
	//Moves not ready to be coalesced (Do not satisfy Briggs or George)
	activeMoves = NULL;

	/****************** Nodes' bindings ****************/
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

static void AddEdge(G_node u,G_node v)
{
	if(!G_inNodeList(u,G_adj(v)) && u != v )
	{
		if(!precolor(u))
		{
			G_addEdge(u,v);
			(*(int *)G_look(degreeTab,u))++;
		}
		if(!precolor(v))
		{
			G_addEdge(v,u);
			(*(int *)G_look(degreeTab,v))++;
		}
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

//Relative Moves with n.
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
	G_nodeList adj = Adjacent(node);
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
		//If n and its adjacent nodes are in activeMoves,
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

/* The degree of the nodes decreased so that it may be ready to be coalesced.
   Add them into workListMoves.*/
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

	//If only 1 precolored in x,y, then it must be u.
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
	//If u and v are both precolored or u and v is adjacent,Cannot combine them directly.
	else if(precolor(v) || G_inNodeList(u,G_adj(v)))
	{
		constrainedMoves = Live_MoveList(x,y,constrainedMoves);
		AddWorkList(u);
		AddWorkList(v);
	}
	//If u and v satisfy the demands of Briggs,then combine them.
	//Conservative uses Briggs
	//OK uses George
	else if((precolor(u) && OK(v,u))
	|| (!precolor(u) && Conservative(G_UnionNodeList(Adjacent(u),Adjacent(v)))))
	{
		coalescedMoves = Live_MoveList(x,y,coalescedMoves);
		Combine(u,v);
		AddWorkList(u);
	}
	//Can't coalesce now
	else{
		activeMoves = Live_MoveList(x,y,activeMoves);
	}
}

//Add a node into simplifyWorklist.
static void AddWorkList(G_node n)
{
	if(!precolor(n) && !MoveRelated(n) &&  degree(n) < K)
	{
		freezeWorklist = G_SubNodeList(freezeWorklist,G_NodeList(n,NULL));
		simplifyWorklist = G_NodeList(n,simplifyWorklist);
	}
}

//Use George to check if v can be combined by u
static bool OK(G_node v,G_node u)
{
	bool ok = TRUE;
	for(G_nodeList adj = Adjacent(v);adj; adj=adj->tail)
	{
		G_node t = adj->head;
		if(!(degree(t) < K || precolor(t) || G_inNodeList(t,G_adj(u))))
		{
			ok = FALSE;
			break;
		}
	}
	return ok;
}

/* Use Briggs. Check if the high degree nodes in the adjacent nodes 
of the combined node(uv) are less than K */
static bool Conservative(G_nodeList nodes)
{
	int count = 0;
	for(;nodes;nodes = nodes->tail)
	{
		if(degree(nodes->head) >= K)
		{
			count = count + 1;
		}
	}
	return count < K;
}

static G_node GetAlias(G_node t)
{
	if(G_inNodeList(t,coalescedNodes))
	{
		return GetAlias(Alias(t));
	}
	else
	{
		return t;
	}
}

static void Combine(G_node u,G_node v)
{
	if(G_inNodeList(v,freezeWorklist))
	{
		freezeWorklist = G_SubNodeList(freezeWorklist,G_NodeList(v,NULL));
	}
	else
	{
		spillWorklist = G_SubNodeList(spillWorklist,G_NodeList(v,NULL));
	}
	coalescedNodes = G_NodeList(v,coalescedNodes);
	*(G_node *)G_look(aliasTab,v) = u;
	
	//Combine v's movelist by u's
	*(Live_moveList *)G_look(moveListTab,u) = Live_UnionMoveList(movelist(u),movelist(v));

	//If it's necessary?
	EnableMoves(v);

	for(G_nodeList adj = Adjacent(v);adj;adj = adj->tail)
	{
		G_node t = adj->head;
		AddEdge(t,u);
		DecrementDegree(t);
	}
	if(degree(u) >= K && G_inNodeList(u,freezeWorklist)))
	{
		freezeWorklist = G_SubNodeList(freezeWorklist,G_NodeList(u,NULL));
		spillWorklist = G_NodeList(u,spillWorklist);
	}
}

static void Freeze()
{
	G_node u = freezeWorklist->head;
	freezeWorklist = freezeWorklist->tail;
	simplifyWorklist = G_NodeList(u,simplifyWorklist);
	FreezeMoves(u);
}

static void FreezeMoves(G_node u)
{
	for(Live_moveList m = NodeMoves(u); m; m = m->tail)
	{
		G_node x = m->src;
		G_node y = m->dst;
		
		//v is the Move neighbor of u.
		G_node v;
		if(GetAlias(y) == GetAlias(u))
		{
			v = GetAlias(x)
		}
		else
		{
			v = GetAlias(y);
		}
		activeMoves = Live_SubMoveList(activeMoves,Live_MoveList(x,y,NULL));
		frozenMoves = Live_MoveList(x,y,frozenMoves);
		if(NodeMoves(v) == NULL && degree(v) < K)
		{
			freezeWorklist = G_SubNodeList(freezeWorklist,G_NodeList(v,NULL));
			simplifyWorklist = G_NodeList(v,simplifyWorklist);
		}
	}
}