#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"
#include "helper.h"
#include "translate.h"

/*Lab5: Your implementation of lab5.*/

struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}

F_fragList SEM_transProg(A_exp exp){

	Tr_level mainframe = Tr_outermost();
	Temp_label mainlabel = Temp_newlabel();

	struct expty mainexp = transExp(E_base_venv(),E_base_tenv(),exp,mainframe,mainlabel);

	Tr_procEntryExit(mainframe,mainexp.exp);

	return Tr_getresult();
}

bool cmpty(Ty_ty ty1,Ty_ty ty2)
{
	Ty_ty actual1=actual_ty(ty1);
	Ty_ty actual2=actual_ty(ty2);
	
	if(actual1->kind == actual2->kind)
	{
		if(actual1->kind == Ty_record)
		{
			if(actual1->u.record == actual2->u.record)
				return 1;
			else
				return 0;
		}
		else if(actual1->kind == Ty_array)
		{
			if(actual1->u.array == actual2->u.array)
				return 1;
			else
				return 0;
		}
		else
			return 1;
	}
	else 
	{
		if((actual1->kind == Ty_nil && actual2->kind == Ty_record)
		|| (actual1->kind == Ty_record && actual2->kind == Ty_nil))
		{
			return 1;
		}
		else return 0;
	}
}

Ty_ty actual_ty(Ty_ty ty)
{
	if(!ty || ty->kind != Ty_name)
	{
		return ty;
	}
	else
		return actual_ty(ty->u.name.ty);
}

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label label)
{
	switch(v->kind)
	{
		case A_simpleVar:
		{
			E_enventry x = S_look(venv,v->u.simple);
			if (x && x->kind == E_varEntry)
			{
				Tr_exp simpvar=Tr_simpleVar(x->u.var.access,l);
				return expTy(simpvar,actual_ty(x->u.var.ty));
			}
			else {
				EM_error(v->pos,"undefined variable %s",S_name(v->u.simple));
				return expTy(NULL,Ty_Int());
			}
		}

		case A_fieldVar:
		{
			A_var v_var = v->u.field.var;
			S_symbol v_sym=v->u.field.sym;
			struct expty check_var = transVar(venv,tenv,v_var,l,label);
			Ty_ty rec = actual_ty(check_var.ty);
			if(rec->kind!=Ty_record)
			{
				EM_error(v->pos,"not a record type");
				return expTy(NULL,Ty_Int());
			}
			else
			{
				Ty_fieldList rec_fieldlist = rec->u.record;
				Ty_field field;
				int order=0;
				while(rec_fieldlist != NULL)
				{
					field = rec_fieldlist->head;
					if(field->name == v_sym)
					{
						Tr_exp fieldvar = Tr_fieldVar(check_var.exp,order);
						return expTy(fieldvar,actual_ty(field->ty));
					}
					rec_fieldlist = rec_fieldlist->tail;
					order++;
				}
				EM_error(v->pos,"field %s doesn't exist",S_name(v_sym));
				return expTy(NULL,Ty_Int());
			}
		}

		case A_subscriptVar:
		{
			A_var sub_v = v->u.subscript.var;
			A_exp subscript= v->u.subscript.exp;
			struct expty check_var = transVar(venv,tenv,sub_v,l,label);
			if(actual_ty(check_var.ty)->kind != Ty_array)
			{
				EM_error(v->pos,"array type required");
				return expTy(NULL,Ty_Int());
			}
			else
			{
				struct expty check_sub=transExp(venv,tenv,subscript,l,label);
				if(actual_ty(check_sub.ty)->kind != Ty_int)
				{
					EM_error(v->pos,"index type is not int");
					return expTy(NULL,Ty_Int());
				}	
				Tr_exp subscriptv = Tr_subscriptVar(check_var.exp,check_sub.exp);
				return expTy(subscriptv,actual_ty(check_var.ty->u.array));
			}
		}
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label)
{
	switch(a->kind)
	{
		case A_varExp:{return transVar(venv,tenv,a,l,label);}
		case A_nilExp:{return transNilexp(venv,tenv,a,l,label);}
		case A_intExp:{return transIntexp(venv,tenv,a,l,label);}
		case A_stringExp:{return transStringexp(venv,tenv,a,l,label);}
		case A_callExp:{return transCallexp(venv,tenv,a,l,label);}
		case A_opExp:{return transOpexp(venv,tenv,a,l,label);}
		case A_recordExp:{return transRecordexp(venv,tenv,a,l,label);}
		case A_seqExp:{return transSeqexp(venv,tenv,a,l,label);}
		case A_assignExp:{return transAssignexp(venv,tenv,a,l,label);}
		case A_ifExp:{return transIfexp(venv,tenv,a,l,label);}
		case A_whileExp:{return transWhileexp(venv,tenv,a,l,label);}
		case A_forExp:{return transForexp(venv,tenv,a,l,label);}
		case A_breakExp:{return transBreakexp(venv,tenv,a,l,label);}
		case A_letExp:{return transLetexp(venv,tenv,a,l,label);}
		case A_arrayExp:{return transArrayexp(venv,tenv,a,l,label);}
	}
}

struct expty transNilexp(S_table venv,S_table tenv,A_exp a,Tr_level l,Temp_label label)
{
	return expTy(Tr_Nil(),Ty_Nil());
}

struct expty transIntexp(S_table venv,S_table tenv,A_exp a,Tr_level l,Temp_label label)
{
	return expTy(Tr_Int(a->u.intt),Ty_Int());
}

struct expty transStringexp(S_table venv,S_table tenv,A_exp a,Tr_level l,Temp_label label)
{
	return expTy(Tr_String(a->u.stringg),Ty_String());
}

struct expty transCallexp(S_table venv,S_table tenv,A_exp a,Tr_level l,Temp_label label)
{
	S_symbol func_name=a->u.call.func;
	A_expList args=a->u.call.args;
	E_enventry x = S_look(venv,func_name);
	Tr_expList list = Tr_ExpList(NULL,NULL);
	Tr_expList tail = list;
	if(x==NULL || x->kind != E_funEntry)
	{
		EM_error(a->pos,"undefined function %s",S_name(func_name));
		return expTy(NULL,Ty_Void());
	}
	else
	{
		Ty_tyList formals= x->u.fun.formals;
		Ty_ty formal;
		A_exp arg;
		for(;args && formals;args=args->tail,formals=formals->tail)
		{
			arg = args->head;
			formal = formals->head;
			struct expty exp=transExp(venv,tenv,arg,l,label);
			if(!cmpty(exp.ty,formal))
			{
				EM_error(arg->pos,"para type mismatch");
			}
			tail->tail=Tr_ExpList(exp.exp,NULL);
			tail = tail->tail;
		}
		if(args != NULL)
		{
			EM_error(a->pos,"too many params in function %s",S_name(a->u.call.func));
		}
		list = list->tail;
		Tr_exp trcall = Tr_Call(x->u.fun.label,list,l,x->u.fun.level);
		return expTy(trcall,actual_ty(x->u.fun.result));
	}
}

struct expty transOpexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label)
{
	A_oper oper = a->u.op.oper;
	struct expty left = transExp(venv,tenv,a->u.op.left,l,label);
	struct expty right = transExp(venv,tenv,a->u.op.right,l,label);
	Tr_exp tropexp;
	if(oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp)
	{
		if(actual_ty(left.ty)->kind != Ty_int)
			EM_error(a->u.op.left->pos,"integer required");
		if(actual_ty(right.ty)->kind !=Ty_int)
		{
			EM_error(a->u.op.right->pos,"integer required");
		}
		tropexp = Tr_Calculate(oper,left.exp,right.exp);
	}
	else
	{
		if(!cmpty(left.ty,right.ty))
		{
			EM_error(a->u.op.left->pos,"same type required");
		}
		tropexp = Tr_Condition(oper,left.exp,right.exp);
	}
	return expTy(tropexp,Ty_Int());
}

