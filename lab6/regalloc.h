/* function prototype from regalloc.c */

#ifndef REGALLOC_H
#define REGALLOC_H

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

struct RA_result {Temp_map coloring; AS_instrList il;};
struct RA_result RA_regAlloc(F_frame f, AS_instrList il);
static void MakeWorklist();
static void Build();
static void Simplify();
static void Coalesce();
static void Freeze();
static void SelectSpill();
static void AssignColors();
static void RewriteProgram(F_frame f,AS_instrList* ilist);
static bool MoveRelated(G_node node);
static G_nodeList Adjacent(G_node n);
static Live_moveList NodeMoves(G_node n);
//static void push(G_node n,G_nodeList stack);
static void DecrementDegree(G_node n);
static void EnableMoves(G_nodeList nodes);

/* Coalesce functions */
static G_node GetAlias(G_node t);
static void AddWorkList(G_node n);
static bool OK(G_node v,G_node u);
static bool Conservative(G_nodeList adj);
static void Combine(G_node u,G_node v);
static void AddEdge(G_node u,G_node v);

static void FreezeMoves(G_node n);

/* Tool Functions*/
static bool precolor(G_node n);
static int degree(G_node n);
static G_node alias(G_node n);
static Live_moveList movelist(G_node n);
static Temp_tempList replaceTempList(Temp_tempList,Temp_temp,Temp_temp);
static Temp_map AssignRegisters(struct Live_graph g);

/* Data Stucture*/
//Worklist
static G_nodeList simplifyWorklist;
static G_nodeList freezeWorklist;
static G_nodeList spillWorklist;

//Node sets
//static G_nodeList precolored;
static G_nodeList coloredNodes;
static G_nodeList spilledNodes;
static G_nodeList coalescedNodes;

static Temp_tempList notSpillTemps;

//Movelist
static Live_moveList coalescedMoves;
static Live_moveList constrainedMoves;
static Live_moveList frozenMoves;
static Live_moveList worklistMoves;//All move instructions.
static Live_moveList activeMoves;

//stack 
static G_nodeList selectStack;

//Nodes information
//The binding is a pointer.
static G_table degreeTab; // binding points to an int (degree).
static G_table colorTab; //binding points to an int (color).
static G_table aliasTab; // binding points to a G_node pointer.
static G_table moveListTab;// binding points to a Live_movelist pointer.
/* u in adjSet(u,v) can be replaced by G_inNodeList(u,G_adj(v)) */
/* adjList[u] can be replaced by G_adj(u) */

static string hard_reg[16] = {"none","%%rax","%%rbx","%%rcx","%%rdx","%%rsi","%%rdi","%%rbp","%%r8","%%r9","%%r10","%%r11","%%r12","%%r13","%%r14","%%r15"};
#endif
