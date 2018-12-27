/*
 * graph.c - Functions to manipulate and create control flow and
 *           interference graphs.
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "errormsg.h"
#include "table.h"
#include "flowgraph.h"

struct G_graph_ {int nodecount;
		 G_nodeList mynodes, mylast;
	       };

struct G_node_ {
  G_graph mygraph;
  int mykey;
  G_nodeList succs;
  G_nodeList preds;
  void *info;
};

G_graph G_Graph(void)
{G_graph g = (G_graph) checked_malloc(sizeof *g);
 g->nodecount = 0;
 g->mynodes = NULL;
 g->mylast = NULL;
 return g;
}

G_nodeList G_NodeList(G_node head, G_nodeList tail)
{G_nodeList n = (G_nodeList) checked_malloc(sizeof *n);
 n->head=head;
 n->tail=tail;
 return n;
}

/* generic creation of G_node */
G_node G_Node(G_graph g, void *info)
{G_node n = (G_node)checked_malloc(sizeof *n);
 G_nodeList p = G_NodeList(n, NULL);
 assert(g);
 n->mygraph=g;
 n->mykey=g->nodecount++;

 if (g->mylast==NULL)
   g->mynodes=g->mylast=p;
 else g->mylast = g->mylast->tail = p;

 n->succs=NULL;
 n->preds=NULL;
 n->info=info;
 return n;
}

G_nodeList G_nodes(G_graph g)
{
  assert(g);
  return g->mynodes;
} 

/* return true if a is in l list */
bool G_inNodeList(G_node a, G_nodeList l) {
  G_nodeList p;
  for(p=l; p!=NULL; p=p->tail)
    if (p->head==a) return TRUE;
  return FALSE;
}

void G_addEdge(G_node from, G_node to) {
  assert(from);  assert(to);
  assert(from->mygraph == to->mygraph);
  if (G_goesTo(from, to)) return;
  to->preds=G_NodeList(from, to->preds);
  from->succs=G_NodeList(to, from->succs);
}

static G_nodeList delete(G_node a, G_nodeList l) {
  assert(a && l);
  if (a==l->head) return l->tail;
  else return G_NodeList(l->head, delete(a, l->tail));
}

void G_rmEdge(G_node from, G_node to) {
  assert(from && to);
  to->preds=delete(from,to->preds);
  from->succs=delete(to,from->succs);
}

 /**
  * Print a human-readable dump for debugging.
  */

void G_show(FILE *out, G_nodeList p) {
  for (; p!=NULL; p=p->tail) 
  {
    G_node n = p->head;
    G_nodeList q;
    assert(n);
    /*
    if (showInfo) 
      showInfo(n->info);
    */
    fprintf(out, " (%d:%d): ", n->mykey,*(int *)G_nodeInfo(n)); 
    for(q=G_succ(n); q!=NULL; q=q->tail) 
         fprintf(out, "%d:%d ", q->head->mykey,*(int *)G_nodeInfo(q->head));
    fprintf(out,"\n");
    
    




    //Flowgraph debugging
    /*
    AS_instr inst = (AS_instr)G_nodeInfo(n);
    char *defuse;
    sprintf(defuse,"defuse: ");
    //char *flownode = checked_malloc(256);
    Temp_tempList seedef = FG_def(n);
    Temp_tempList seeuse = FG_use(n);
    switch(inst->kind)
    {
      case I_LABEL:
        fprintf(out,"assem:%s,label:%s\n",inst->u.LABEL.assem,S_name(inst->u.LABEL.label));break;
      case I_OPER:
        defuse = checked_malloc(256);
        for(Temp_tempList use = inst->u.OPER.src;use;use = use->tail)
        {
          sprintf(defuse,"use:%d ",*(int *)(use->head));
        }
        for(Temp_tempList def = inst->u.OPER.dst;def;def = def->tail)
        {
          sprintf(defuse,"def:%d ",*(int *)(def->head));
        }
      
        fprintf(out,"operassem:%s,%s\n",inst->u.OPER.assem,defuse);
        
        //fprintf(out,"operassem:%s\n",inst->u.OPER.assem);
        break;
      case I_MOVE:
        defuse = checked_malloc(256);
        for(Temp_tempList use = inst->u.MOVE.src;use;use = use->tail)
        {
          sprintf(defuse,"%s,use:%d ",defuse,*(int *)(use->head));
        }
        for(Temp_tempList def = inst->u.MOVE.dst;def;def = def->tail)
        {
          sprintf(defuse,"%s,def:%d ",defuse,*(int *)(def->head));
        }
      
        fprintf(out,"moveassem:%s,%s\n",inst->u.MOVE.assem,defuse);
        break;
    }
    */
    
    fprintf(out, "\n");
  }
}
/*
void showInfo(void *info)
{
  
}
*/
G_nodeList G_succ(G_node n) { assert(n); return n->succs; }

G_nodeList G_pred(G_node n) { assert(n); return n->preds; }

bool G_goesTo(G_node from, G_node n) {
  return G_inNodeList(n, G_succ(from));
}

/* return length of predecessor list for node n */
static int inDegree(G_node n)
{ int deg = 0;
  G_nodeList p;
  for(p=G_pred(n); p!=NULL; p=p->tail) deg++;
  return deg;
}

/* return length of successor list for node n */
static int outDegree(G_node n)
{ int deg = 0;
  G_nodeList p; 
  for(p=G_succ(n); p!=NULL; p=p->tail) deg++;
  return deg;
}

int G_degree(G_node n) {return inDegree(n)+outDegree(n);}

/* put list b at the back of list a and return the concatenated list */
static G_nodeList cat(G_nodeList a, G_nodeList b) {
  if (a==NULL) return b;
  else return G_NodeList(a->head, cat(a->tail, b));
}

/* create the adjacency list for node n by combining the successor and 
 * predecessor lists of node n */
G_nodeList G_adj(G_node n) {return cat(G_succ(n), G_pred(n));}

void *G_nodeInfo(G_node n) {return n->info;}



/* G_node table functions */

G_table G_empty(void) {
  return TAB_empty();
}

void G_enter(G_table t, G_node node, void *value)
{
  TAB_enter(t, node, value);
}

void *G_look(G_table t, G_node node)
{
  return TAB_look(t, node);
}

G_nodeList G_UnionNodeList(G_nodeList left,G_nodeList right)
{
  G_nodeList cur = left;
  for(;right;right = right->tail)
  {
    if(!G_inNodeList(right->head,left))
    {
      cur = G_NodeList(right->head,cur);
    }
  }
  return cur;
}

G_nodeList G_SubNodeList(G_nodeList left,G_nodeList right)
{
  G_nodeList cur = NULL;
  for(;left;left = left->tail)
  {
    if(!G_inNodeList(left->head,right))
    {
      cur = G_NodeList(left->head,cur);
    }
  }
  return cur;
}

G_nodeList G_IntersectNodeList(G_nodeList left,G_nodeList right)
{
  G_nodeList cur = NULL;
  for(;left ; left = left->tail)
  {
    if(G_inNodeList(left->head,right))
    {
      cur = G_NodeList(left->head,cur);
    }
  }
  return cur;
}