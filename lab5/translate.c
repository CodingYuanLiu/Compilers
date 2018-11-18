#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.

struct Tr_access_ {
	Tr_level level;
	F_access access;
};


struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;	
};

struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};

struct Cx 
{
	patchList trues; 
	patchList falses; 
	T_stm stm;
};

struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {T_exp ex; T_stm nx; struct Cx cx; } u;
};

Tr_expList Tr_ExpList(Tr_exp head,Tr_expList tail)
{
	Tr_expList list= checked_malloc(sizeof(*list));
	list->head = head;
	list->tail = tail;
	return list;
}

static patchList PatchList(Temp_label *head, patchList tail)
{
	patchList list;

	list = (patchList)checked_malloc(sizeof(struct patchList_));
	list->head = head;
	list->tail = tail;
	return list;
}

void doPatch(patchList tList, Temp_label label)
{
	for(; tList; tList = tList->tail)
		*(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second)
{
	if(!first) return second;
	for(; first->tail; first = first->tail);
	first->tail = second;
	return first;
}


static Tr_exp Tr_Ex(T_exp exp)
{
	Tr_exp ex = checked_malloc(sizeof(*ex));
	ex->kind = Tr_ex;
	ex->u.ex=exp;
	return ex;
}

static Tr_exp Tr_Nx(T_stm stm)
{
	Tr_exp nx = checked_malloc(sizeof(*nx));
	nx->kind = Tr_nx;
	nx->u.nx = stm;
	return nx;
}

static Tr_exp Tr_Cx(patchList trues,patchList falses,T_stm stm)
{
	Tr_exp cx = checked_malloc(sizeof(*cx));
	cx->kind = Tr_cx;
	cx->u.cx.falses = falses;
	cx->u.cx.trues = trues;
	cx->u.cx.stm = stm;
	return cx;
}

static T_exp unEx(Tr_exp e)
{
	switch(e->kind)
	{
		case Tr_ex:
			return e->u.ex;
		case Tr_nx:
			return T_Eseq(e->u.nx,T_Const(0));
		case Tr_cx:
		{
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel(), f = Temp_newlabel();
			doPatch(e->u.cx.trues,t);
			doPatch(e->u.cx.falses,f);
			return 
			T_Eseq(T_Move(r,T_Const(1)),
				T_Eseq(e->u.cx.stm,
					T_Eseq(T_label(f),
						T_Eseq(T_Move(r,T_Const(0)),
							T_Eseq(T_label(t),
								T_temp(r))))));
		}
	}
	assert(0);//Can't get here.
}

static T_stm unNx(Tr_exp e)
{
	switch(e->kind)
	{
		case Tr_ex:
			return T_Exp(e->u.ex);
		case Tr_nx:
			return e->u.nx;
		case Tr_cx:
			Temp_label label = Temp_newlabel();
			doPatch(e->u.cx.trues,label);
			doPatch(e->u.cx.falses,label);
			return T_Seq(e->u.cx.stm,T_Label(label));
	}
	assert(0);
}


Tr_level Tr_outermost(void)
{
	static struct Tr_level_ outermost;
	outermost.frame = NULL;
	outermost.parent = NULL;
	return &outermost; 
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals)
{
	Tr_level level = checked_malloc(sizeof(*level));
	U_boolList link_added_formals = U_BoolList(TRUE,formals);
	F_frame frame = F_newFrame(name,link_added_formals);
	level->frame = frame;
	level->parent = parent;
	return level;
}

void Tr_procEntryExit(Tr_level level,Tr_exp func_body)
{
	T_stm stm = T_Move(F_RAX(),unEx(func_body));
	F_frag head = F_ProcFrag(stm,level->frame);
	//The added frag is the head of the new frags. 
	frags = F_FragList(head,frags);
}

Tr_exp Tr_simpleVar(Tr_access access,Tr_level level)
{
	T_exp frame = T_Temp(F_FP());
	while(level != access->level)
	{
		frame = T_Mem(T_Binop(T_plus,frame,T_Const(2 * wordsize)));//static link is at 16(%rbp);
		level = level->parent;
	}
	frame = F_exp(access->access,frame);
	return Tr_Ex(frame);
}

Tr_exp Tr_fieldVar(Tr_exp loc,int order)
{
	if(loc->kind != Tr_ex)
	{
		printf("Error: fieldVar's loc must be an expression");
	}
	return T_Mem(T_Binop(T_plus,unEx(loc),T_Const(order * wordsize)));
}

Tr_exp Tr_subscriptVar(Tr_exp loc,Tr_exp subscript)
{
	if(loc->kind != Tr_ex || subscript->kind != Tr_ex)
	{
		printf("Error: subscriptVar's loc or subscript must be an expression");
	}
	return T_Mem(T_Binop(T_plus,unEx(loc),
		T_Binop(T_mul,unEx(subscript),T_Const(wordsize))));
}

Tr_exp Tr_Nil()
{
	return Tr_Ex(T_Const(0));
}

Tr_exp Tr_Int(int value)
{
	return Tr_Ex(T_Const(value));
}

Tr_exp Tr_String(string str)
{
	Temp_label label = Temp_newlabel();
	F_frag head = F_Stringfrag(label,str);
	frags = F_FragList(head,frags);//New String on the head of the frags.
	return Tr_Ex(T_Name(label));
}

Tr_exp Tr_Call(Temp_label label,Tr_expList args,Tr_level caller,Tr_level callee)
{
	T_exp staticlink = T_Temp(F_FP());
	while(caller != callee)
	{
		staticlink = T_Mem(T_Binop(T_plus,staticlink,T_Const(2*wordsize)));
		caller = caller->parent;
	}
	T_expList targs = T_ExpList(NULL,NULL);
	T_expList tail = targs;
	for(;args;args=args->tail)
	{
		tail->tail = T_ExpList(Tr_unEx(args->head),NULL);
		tail = tail->tail;
	}
	targs = T_ExpList(staticlink,targs);
	return Tr_Ex(T_Call(T_Name(label),targs));
}

Tr_exp Tr_Calculate(A_oper op,Tr_exp left,Tr_exp right)
{
	switch(op)
	{
		case A_plusOp:
		{
			return Tr_Ex(T_Binop(T_plus,Tr_unEx(left),Tr_unEx(right)));
		}
		case A_minusOp:
		{
			return Tr_Ex(T_Binop(T_minus,Tr_unEx(left),Tr_unEx(right)));
		}
		case A_timesOp:
		{
			return Tr_Ex(T_Binop(T_mul,Tr_unEx(left),Tr_unEx(right)));
		}
		case A_divideOp:
		{
			return Tr_Ex(T_Binop(T_div,Tr_unEx(left),Tr_unEx(right)));
		}
		assert(0);
	}
}

Tr_exp Tr_Condition(A_oper op,Tr_exp left,Tr_exp right)
{
	struct Cx cx;
	T_stm stm;
	switch(op)
	{
		
	}
}