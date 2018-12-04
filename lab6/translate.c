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

static struct F_fragList_ startfrag = {NULL,NULL};

static F_fragList frags = &startfrag, fragtail = &startfrag;

static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues,patchList falses,T_stm stm); 

static T_exp Tr_unEx(Tr_exp e);
static T_stm Tr_unNx(Tr_exp e);
static struct Cx Tr_unCx(Tr_exp e);



F_fragList Tr_getresult()
{
	frags = frags->tail;
	return frags;
}

Tr_expList Tr_ExpList(Tr_exp head,Tr_expList tail)
{
	Tr_expList list= checked_malloc(sizeof(*list));
	list->head = head;
	list->tail = tail;
	return list;
}

Tr_access Tr_Access(F_access faccess,Tr_level level)
{
	Tr_access access = checked_malloc(sizeof(*access));
	access->access=faccess;
	access->level = level;
	return access;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail)
{
	Tr_accessList lst = checked_malloc(sizeof(*lst));
	lst->head = head;
	lst->tail = tail;
	return lst;
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

static T_exp Tr_unEx(Tr_exp e)
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
			T_Eseq(T_Move(T_Temp(r),T_Const(1)),
				T_Eseq(e->u.cx.stm,
					T_Eseq(T_Label(f),
						T_Eseq(T_Move(T_Temp(r),T_Const(0)),
							T_Eseq(T_Label(t),
								T_Temp(r))))));
		}
	}
}

static T_stm Tr_unNx(Tr_exp e)
{
	switch(e->kind)
	{
		case Tr_ex:
			return T_Exp(e->u.ex);
		case Tr_nx:
			return e->u.nx;
		case Tr_cx:
		{
			Temp_label label = Temp_newlabel();
			doPatch(e->u.cx.trues,label);
			doPatch(e->u.cx.falses,label);
			return T_Seq(e->u.cx.stm,T_Label(label));
		}
	}
	assert(0);
}

static struct Cx Tr_unCx(Tr_exp e)
{
	switch(e->kind)
	{
		case Tr_cx:
			return e->u.cx;
		case Tr_ex:
		{
			struct Cx uncx;
			uncx.stm = T_Cjump(T_ne,e->u.ex,0,NULL,NULL);
			uncx.trues = PatchList(&(uncx.stm->u.CJUMP.true),NULL);
			uncx.falses = PatchList(&(uncx.stm->u.CJUMP.false),NULL );
			return uncx;
		}
		case Tr_nx:
			printf("Error: nx can't be a test exp.");
	}
	assert(0);
}

Tr_level Tr_outermost(void)
{
	static struct Tr_level_ outermost;
	outermost.frame = F_newFrame(Temp_namedlabel("main"),NULL);
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
	T_stm stm = T_Move(T_Temp(F_RV()),Tr_unEx(func_body));
	F_frag head = F_ProcFrag(stm,level->frame);
	//The added frag is the head of the new frags. 
	fragtail->tail = F_FragList(head,NULL);
	fragtail = fragtail->tail;
}

Tr_access Tr_allocLocal(Tr_level level,bool escape)
{
	Tr_access access = checked_malloc(sizeof(*access));
	access->access = F_allocLocal(level->frame,escape);
	access->level = level;
	return access;
}

Tr_exp Tr_simpleVar(Tr_access access,Tr_level level)
{
	T_exp frame = T_Temp(F_FP());
	while(level != access->level)
	{
		frame = T_Mem(T_Binop(T_plus,frame,T_Const(-wordsize)));//static link is the first escaped arg;
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
	return Tr_Ex(T_Mem(T_Binop(T_plus,Tr_unEx(loc),T_Const(order * wordsize))));
}

Tr_exp Tr_subscriptVar(Tr_exp loc,Tr_exp subscript)
{
	if(loc->kind != Tr_ex || subscript->kind != Tr_ex)
	{
		printf("Error: subscriptVar's loc or subscript must be an expression");
	}
	return Tr_Ex(T_Mem(T_Binop(T_plus,Tr_unEx(loc),
		T_Binop(T_mul,Tr_unEx(subscript),T_Const(wordsize)))));
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
	F_frag head = F_StringFrag(label,str);
	//frags = F_FragList(head,frags);//New String on the head of the frags.
	fragtail->tail = F_FragList(head,NULL);
	fragtail = fragtail->tail;
	return Tr_Ex(T_Name(label));
}

Tr_exp Tr_Call(Temp_label label,Tr_expList args,Tr_level caller,Tr_level callee)
{
	T_exp staticlink = T_Temp(F_FP());
	while(caller != callee->parent)
	{
		staticlink = T_Mem(T_Binop(T_plus,staticlink,T_Const(-wordsize)));
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
	T_stm stm;
	switch(op)
	{
		case A_eqOp:
		{
			stm = T_Cjump(T_eq,Tr_unEx(left),Tr_unEx(right),NULL,NULL);
			break;
		}
		case A_neqOp:
		{
			stm = T_Cjump(T_ne,Tr_unEx(left),Tr_unEx(right),NULL,NULL);
			break;
		}
		case A_ltOp:
		{
			stm = T_Cjump(T_lt,Tr_unEx(left),Tr_unEx(right),NULL,NULL);
			break;
		}
		case A_leOp:
		{
			stm = T_Cjump(T_le,Tr_unEx(left),Tr_unEx(right),NULL,NULL);
			break;
		}
		case A_gtOp:
		{
			stm = T_Cjump(T_gt,Tr_unEx(left),Tr_unEx(right),NULL,NULL);
			break;
		}
		case A_geOp:
		{
			stm = T_Cjump(T_ge,Tr_unEx(left),Tr_unEx(right),NULL,NULL);
			break;
		}
		default:
		{
			assert(0);
		}
	}
	patchList trues = PatchList(&stm->u.CJUMP.true,NULL);
	patchList falses = PatchList(&stm->u.CJUMP.false,NULL);
	return Tr_Cx(trues,falses,stm);
}

Tr_exp Tr_Recordexp(Tr_expList fields)
{
	int count = 0;
	for(Tr_expList cnt_field = fields;cnt_field;cnt_field = cnt_field->tail,count++);
	Temp_temp r = Temp_newtemp();
	T_stm stm = 
	T_Move(
		T_Temp(r),
		F_externalCall("malloc"
			,T_ExpList(T_Const(count*wordsize),NULL)
		)
	);
	stm = T_Seq(stm,Tr_mk_record_array(fields,r,0,count));
	return Tr_Ex(T_Eseq(stm,T_Temp(r)));
}

T_stm Tr_mk_record_array(Tr_expList fields,Temp_temp r,int offset,int size)
{
	if(size > 1)
	{
		if(offset < size-2)
		{
			return 
			T_Seq(
				T_Move(T_Binop(T_plus,T_Temp(r),T_Const(offset*wordsize)),Tr_unEx(fields->head)),
				Tr_mk_record_array(fields->tail,r,offset+1,size)
			);
		}
		else
		{
			return
			T_Seq(
				T_Move(T_Binop(T_plus,T_Temp(r),T_Const(offset*wordsize)),Tr_unEx(fields->head)),
				T_Move(T_Binop(T_plus,T_Temp(r),T_Const((offset+1)*wordsize)),Tr_unEx(fields->tail->head))
			);
		}
	}
	else{
		return T_Move(T_Binop(T_plus,T_Temp(r),T_Const(offset*wordsize)),Tr_unEx(fields->head));
	}
}

Tr_exp Tr_Assign(Tr_exp var,Tr_exp exp)
{
	return Tr_Nx(T_Move(Tr_unEx(var),Tr_unEx(exp)));
}

Tr_exp Tr_If(Tr_exp test,Tr_exp then,Tr_exp elsee)
{
	struct Cx test_ = Tr_unCx(test);
	Temp_temp r = Temp_newtemp();
	Temp_label truelabel = Temp_newlabel();
	Temp_label falselabel = Temp_newlabel();
	doPatch(test_.trues,truelabel);
	doPatch(test_.falses,falselabel);
	if(elsee)
	{
		Temp_label meeting = Temp_newlabel();
		T_exp e = 
		T_Eseq(test_.stm,
			T_Eseq(T_Label(truelabel),
				T_Eseq(T_Move(T_Temp(r),Tr_unEx(then)),
					T_Eseq(T_Jump(T_Name(meeting),Temp_LabelList(meeting,NULL)),
						T_Eseq(T_Label(falselabel),
							T_Eseq(T_Move(T_Temp(r),Tr_unEx(elsee)),
								T_Eseq(T_Jump(T_Name(meeting),Temp_LabelList(meeting,NULL)),
									T_Eseq(T_Label(meeting),
										T_Temp(r)))))))));
		return Tr_Ex(e);
	}
	else
	{
		T_stm s = 
		T_Seq(test_.stm,
			T_Seq(T_Label(truelabel),
				T_Seq(Tr_unNx(then),T_Label(falselabel))));
		return Tr_Nx(s);
	}
}

Tr_exp Tr_While(Tr_exp test,Tr_exp body,Temp_label done)
{
	Temp_label bodyy = Temp_newlabel(), tst = Temp_newlabel();
	struct Cx test_ = Tr_unCx(test);
	doPatch(test_.trues,bodyy);
	doPatch(test_.falses,done);
	T_stm s = T_Seq(T_Label(tst),
	T_Seq(test_.stm,
		T_Seq(T_Label(bodyy),
			T_Seq(Tr_unNx(body),
				T_Seq(T_Jump(T_Name(tst),Temp_LabelList(tst,NULL)),
					T_Label(done))))));
	return Tr_Nx(s);
}

Tr_exp Tr_For(Tr_access loopv,Tr_exp lo,Tr_exp hi,Tr_exp body,Tr_level l,Temp_label done)
{
	//Caution:check lo<=hi FIRST!
	//check if i<hi before i++;
	Temp_label bodylabel = Temp_newlabel();
	Temp_label incloop_label = Temp_newlabel();

	//Stm makes i++
	T_stm incloop;//Stm that makes i++;
	T_exp loopvar = F_exp(loopv->access,T_Temp(F_FP()));
	incloop = T_Move(loopvar,
		T_Binop(T_plus,loopvar,T_Const(1)));

	/*if(i < hi) {i++; goto body;}*/
	T_stm test = T_Seq(T_Cjump(T_le,Tr_unEx(Tr_simpleVar(loopv,l)),Tr_unEx(hi),incloop_label,done),
	T_Seq(T_Label(incloop_label),
		T_Seq(incloop,
			T_Jump(T_Name(bodylabel),Temp_LabelList(bodylabel,NULL)))));

	//Test if lo<=hi;
	T_stm checklohi = T_Cjump(T_le,Tr_unEx(lo),Tr_unEx(hi),bodylabel,done);

	//Concatenate together.
	T_stm forr = T_Seq(checklohi,
	T_Seq(T_Label(bodylabel),
		T_Seq(Tr_unNx(body),
			T_Seq(test,T_Label(done)))));
	
	return Tr_Nx(forr);
}

Tr_exp Tr_Break(Temp_label done)
{
	return Tr_Nx(T_Jump(T_Name(done),Temp_LabelList(done,NULL)));
}

Tr_exp Tr_Array(Tr_exp size,Tr_exp init)
{
	//Call initArray to create an array.
	T_exp callinitArray = F_externalCall("initArray",
	T_ExpList(Tr_unEx(size),
		T_ExpList(Tr_unEx(init),NULL)));
	return Tr_Ex(callinitArray);
}


Tr_accessList Tr_get_formal_access(Tr_level level)
{
	Tr_accessList accesslst = Tr_AccessList(NULL,NULL);
	Tr_accessList tail = accesslst;
	for(F_accessList faccesslst= level->frame->formals;faccesslst;faccesslst=faccesslst->tail)
	{
		tail->tail = Tr_AccessList(Tr_Access(faccesslst->head,level),NULL);
		tail = tail->tail;
	}
	accesslst = accesslst->tail;
	return accesslst;
}

Tr_exp Tr_Seq(Tr_exp left,Tr_exp right)
{
	/*Don't handle the situation where left is NULL*/
	T_exp e;
	if(right)
	{
		e= T_Eseq(Tr_unNx(left),Tr_unEx(right));
	}
	else
	{
		e = T_Eseq(Tr_unNx(left),T_Const(0));
	}
	return Tr_Ex(e);
}