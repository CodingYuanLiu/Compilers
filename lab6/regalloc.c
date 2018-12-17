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

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here
	struct RA_result ret;
	bool done = FALSE;
	do{
		done = TRUE;
		/*Liveness Analysis*/
		G_graph fg = FG_AssemFlowGraph(il,f);
		struct Live_graph live = Live_liveness(fg);

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

		//To be continued...
	}while(!done);
	return ret;
}
