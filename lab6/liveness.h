#ifndef LIVENESS_H
#define LIVENESS_H

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	G_node src, dst;
	Live_moveList tail;
};

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);
bool Live_isinMoveList(G_node src,G_node dst,Live_moveList tail);

struct Live_graph {
	G_graph graph;
	Live_moveList moves;
};
Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

// A && B for set A,B
Temp_tempList UnionSets(Temp_tempList left,Temp_tempList right);
// A - B for set A,B
Temp_tempList SubSets(Temp_tempList left,Temp_tempList right);
bool tempequal(Temp_tempList old,Temp_tempList neww);
bool intemp(Temp_tempList left,Temp_temp right);

Live_moveList Live_IntersectMoveList(Live_moveList left,Live_moveList right);
Live_moveList Live_UnionMoveList(Live_moveList left, Live_moveList right);
Live_moveList Live_SubMoveList(Live_moveList left,Live_moveList right);

#endif
