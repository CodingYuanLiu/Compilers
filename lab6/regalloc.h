/* function prototype from regalloc.c */

#ifndef REGALLOC_H
#define REGALLOC_H

struct RA_result {Temp_map coloring; AS_instrList il;};
struct RA_result RA_regAlloc(F_frame f, AS_instrList il);
static void MakeWorklist();
static void Build();
static void Simplify();
static void Coalesce();
static void Freeze();
static void SelectSpill();
static void AssignColors();
static void RewriteProgram(F_frame f,AS_instrList ilist);
static bool MoveRelated(G_node node);
static G_nodeList Adjacent(G_node n);


static G_nodeList simplifyWorklist;
static G_nodeList freezeWorklist;
static G_nodeList spillWorklist;

static G_nodeList precolored;
static G_nodeList coloredNodes;
static G_nodeList spilledNodes;
static G_nodeList coalescedNodes;

//Movelist
static Live_moveList coalescedMoves;
static Live_moveList constrainedMoves;
static Live_moveList frozenMoves;
static Live_moveList worklistMoves;
static Live_moveList activeMoves;

//stack 
static G_nodeList selectStack;

//Nodes information
static G_table degreeTab;
static G_table colorTab;
static G_table aliasTab;
//static G_table moveListTab;

#endif
