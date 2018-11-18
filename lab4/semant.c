#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/


typedef void* Tr_exp;
struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

void SEM_transProg(A_exp exp)
{
	S_table tenv = E_base_tenv();
	S_table venv = E_base_venv();
	transExp(venv,tenv,exp);
}

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
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


struct expty transVar(S_table venv,S_table tenv,A_var v)
{
	switch(v->kind)
	{
		case A_simpleVar:
		{
			E_enventry x = S_look(venv,v->u.simple);
			if (x && x->kind == E_varEntry)
			{
				return expTy(NULL,actual_ty(x->u.var.ty));
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
			struct expty check_var = transVar(venv,tenv,v_var);
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
				while(rec_fieldlist != NULL)
				{
					field = rec_fieldlist->head;
					if(field->name == v_sym)
					{
						return expTy(NULL,actual_ty(field->ty));
					}
					rec_fieldlist = rec_fieldlist->tail;
				}
				EM_error(v->pos,"field %s doesn't exist",S_name(v_sym));
				return expTy(NULL,Ty_Int());
			}
		}

		case A_subscriptVar:
		{
			A_var sub_v = v->u.subscript.var;
			A_exp subscript= v->u.subscript.exp;
			struct expty check_var = transVar(venv,tenv,sub_v);
			if(actual_ty(check_var.ty)->kind != Ty_array)
			{
				EM_error(v->pos,"array type required");
				return expTy(NULL,Ty_Int());
			}
			else
			{
				struct expty check_sub=transExp(venv,tenv,subscript);
				if(actual_ty(check_sub.ty)->kind != Ty_int)
				{
					EM_error(v->pos,"index type is not int");
					return expTy(NULL,Ty_Int());
				}	
			}
			return expTy(NULL,actual_ty(check_var.ty->u.array));
		}
	}
}

struct expty transExp(S_table venv,S_table tenv,A_exp a)
{
	switch(a->kind)
	{
		case A_varExp:{return transVarexp(venv,tenv,a);}
		case A_nilExp:{return transNilexp(venv,tenv,a);}
		case A_intExp:{return transIntexp(venv,tenv,a);}
		case A_stringExp:{return transStringexp(venv,tenv,a);}
		case A_callExp:{return transCallexp(venv,tenv,a);}
		case A_opExp:{return transOpexp(venv,tenv,a);}
		case A_recordExp:{return transRecordexp(venv,tenv,a);}
		case A_seqExp:{return transSeqexp(venv,tenv,a);}
		case A_assignExp:{return transAssignexp(venv,tenv,a);}
		case A_ifExp:{return transIfexp(venv,tenv,a);}
		case A_whileExp:{return transWhileexp(venv,tenv,a);}
		case A_forExp:{return transForexp(venv,tenv,a);}
		case A_breakExp:{return transBreakexp(venv,tenv,a);}
		case A_letExp:{return transLetexp(venv,tenv,a);}
		case A_arrayExp:{return transArrayexp(venv,tenv,a);}

	}
}


struct expty transVarexp(S_table venv,S_table tenv,A_exp a)
{
	return transVar(venv,tenv,a->u.var);
}

struct expty transNilexp(S_table venv,S_table tenv,A_exp a)
{
	return expTy(NULL,Ty_Nil());
}

struct expty transIntexp(S_table venv,S_table tenv,A_exp a)
{
	return expTy(NULL,Ty_Int());
}

struct expty transStringexp(S_table venv,S_table tenv,A_exp a)
{
	return expTy(NULL,Ty_String());
}

struct expty transCallexp(S_table venv,S_table tenv,A_exp a)
{
	S_symbol func_name=a->u.call.func;
	A_expList args=a->u.call.args;
	E_enventry x = S_look(venv,func_name);
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
			struct expty exp=transExp(venv,tenv,arg);
			if(!cmpty(exp.ty,formal))
			{
				EM_error(arg->pos,"para type mismatch");
			}
		}
		if(args != NULL)
		{
			EM_error(a->pos,"too many params in function %s",S_name(a->u.call.func));
		}
		return expTy(NULL,actual_ty(x->u.fun.result));
	}
}

struct expty transOpexp(S_table venv,S_table tenv,A_exp a)
{
	A_oper oper = a->u.op.oper;
	struct expty left = transExp(venv,tenv,a->u.op.left);
	struct expty right = transExp(venv,tenv,a->u.op.right);
	if(oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp)
	{
		if(actual_ty(left.ty)->kind != Ty_int)
			EM_error(a->u.op.left->pos,"integer required");
		if(actual_ty(right.ty)->kind !=Ty_int)
		{
			EM_error(a->u.op.right->pos,"integer required");
		}
	}
	else
	{
		if(!cmpty(left.ty,right.ty))
		{
			EM_error(a->u.op.left->pos,"same type required");
		}
	}
	return expTy(NULL,Ty_Int());
}


struct expty transRecordexp(S_table venv,S_table tenv,A_exp a)
{
	S_symbol ty_name = a->u.record.typ;
	Ty_ty x = actual_ty(S_look(tenv,ty_name));
	if(!x)
	{
		EM_error(a->pos,"undefined type %s",S_name(ty_name));
		return expTy(NULL,Ty_Int());
	}
	if(actual_ty(x)->kind != Ty_record)
	{
		EM_error(a->pos,"not a record type");
		return expTy(NULL,x);
	}
	else
	{
		A_efieldList efields = a->u.record.fields;
		Ty_fieldList fields = x->u.record;
		A_efield efield;
		Ty_field field;
		while(efields != NULL)
		{
			
			if(fields == NULL)
			{
				EM_error(a->pos,"Too many efields in %s",S_name(ty_name));
				break;
			}
			
			efield = efields->head;
			field = fields->head;
			struct expty exp = transExp(venv,tenv,efield->exp);
			if(!cmpty(exp.ty,field->ty) )
			{
				EM_error(a->pos,"record type unmatched");
			}
			efields=efields->tail;
			fields=fields->tail;
		}

		if(fields != NULL)
		{
			EM_error(a->pos,"Too little efields in %s",S_name(ty_name));
		}
	}
	return expTy(NULL,x);
}

struct expty transSeqexp(S_table venv,S_table tenv,A_exp a)
{
	A_expList exps=a->u.seq;
	if(!exps)
	{
		return expTy(NULL,Ty_Void());
	}
	else
	{
		struct expty result;
		while(exps)
		{
			result = transExp(venv,tenv,exps->head);
			exps = exps->tail;
		}
		return result;
	}
}

struct expty transAssignexp(S_table venv,S_table tenv,A_exp a)
{
	
	A_var v=a->u.assign.var;
	if(v->kind == A_simpleVar)
	{
		E_enventry x = S_look(venv,v->u.simple);
		if(x->readonly == 1)
		{
			EM_error(a->pos,"loop variable can't be assigned");
		}
	}
	A_exp exp=a->u.assign.exp;
	Ty_ty var_ty = transVar(venv,tenv,v).ty;
	Ty_ty exp_ty = transExp(venv,tenv,exp).ty;
	if(!cmpty(var_ty,exp_ty))
	{
		EM_error(a->pos,"unmatched assign exp");
	}
	return expTy(NULL,actual_ty(var_ty));
}

struct expty transIfexp(S_table venv,S_table tenv,A_exp a)
{
	struct expty test = transExp(venv,tenv,a->u.iff.test);
	struct expty then = transExp(venv,tenv,a->u.iff.then);
	if(a->u.iff.elsee->kind != A_nilExp)
	{
		struct expty elsee=transExp(venv,tenv,a->u.iff.elsee);
		if(!cmpty(then.ty,elsee.ty))
		{
			EM_error(a->pos,"then exp and else exp type mismatch");
		}
	}
	else if(then.ty->kind != Ty_void)
	{
		EM_error(a->pos,"if-then exp's body must produce no value");
	}
	return expTy(NULL,actual_ty(then.ty));
}

struct expty transWhileexp(S_table venv,S_table tenv,A_exp a)
{
	struct expty test = transExp(venv,tenv,a->u.whilee.test);
	struct expty body = transExp(venv,tenv,a->u.whilee.body);
	if(body.ty->kind != Ty_void)
	{
		EM_error(a->pos,"while body must produce no value");
	}
	return expTy(NULL,actual_ty(body.ty));
}

struct expty transForexp(S_table venv,S_table tenv,A_exp a)
{
	S_beginScope(venv);
	struct expty lo=transExp(venv,tenv,a->u.forr.lo);
	struct expty hi=transExp(venv,tenv,a->u.forr.hi);
	if(!cmpty(lo.ty,Ty_Int()) || !cmpty(hi.ty,Ty_Int()))
	{
		EM_error(a->u.forr.lo->pos,"for exp's range type is not integer");
	}
	E_enventry rovar=E_VarEntry(lo.ty);
	rovar->readonly=1;
	S_enter(venv,a->u.forr.var,rovar);
	struct expty body = transExp(venv,tenv,a->u.forr.body);
	if(body.ty->kind != Ty_void)
	{
		EM_error(a->pos,"for body must produce no value");
	}
	S_endScope(venv);
	return expTy(NULL,actual_ty(body.ty));
}

struct expty transBreakexp(S_table venv, S_table tenv, A_exp a)
{
	return expTy(NULL,Ty_Void());
}

struct expty transLetexp(S_table venv, S_table tenv, A_exp a)
{
	struct expty exp;
	A_decList d;
	S_beginScope(venv);
	S_beginScope(tenv);
	for(d=a->u.let.decs ;d ; d=d->tail)
	{
		transDec(venv,tenv,d->head);
	}
	exp=transExp(venv,tenv,a->u.let.body);
	S_endScope(venv);
	S_endScope(tenv);
	return expTy(NULL,actual_ty(exp.ty));
}

struct expty transArrayexp(S_table venv,S_table tenv, A_exp a)
{
	Ty_ty ty_array = actual_ty(S_look(tenv,a->u.array.typ));
	if(!ty_array)
	{
		EM_error(a->pos,"undefined type %s",S_name(a->u.array.typ));
		return expTy(NULL,Ty_Int());
	}
	if(ty_array->kind!=Ty_array)
	{
		EM_error(a->pos,"not array type");
	}
	struct expty size = transExp(venv,tenv,a->u.array.size);
	if(size.ty->kind != Ty_int)
	{
		EM_error(a->u.array.size->pos,"type of size expression should be int");
	}
	struct expty init = transExp(venv,tenv,a->u.array.init);
	if(!cmpty(init.ty,ty_array->u.array))
	{
		EM_error(a->u.array.size->pos,"type mismatch");
	}
	return expTy(NULL,ty_array->u.array);
}


void transDec(S_table venv,S_table tenv,A_dec d)
{
	switch(d->kind)
	{
		case A_functionDec:{transFunctionDec(venv,tenv,d);break;}
		case A_varDec:{transVarDec(venv,tenv,d);break;}
		case A_typeDec:{transTypeDec(venv,tenv,d);break;}
	}
}

void transVarDec(S_table venv,S_table tenv,A_dec d)
{
	struct expty e = transExp(venv,tenv,d->u.var.init);
	S_symbol type = d->u.var.typ;
	if(!type)
	{
		if(e.ty == Ty_Nil())
		{
			EM_error(d->pos,"init should not be nil without type specified");
		}
		S_enter(venv,d->u.var.var,E_VarEntry(e.ty));
		return;
	}
	else
	{
		Ty_ty ty_var= S_look(tenv,type);
		if(!ty_var)
		{
			EM_error(d->pos,"undefined type %s",S_name(type));
			return ;
		}
		else
		{
			if(e.ty == Ty_Nil() && actual_ty(ty_var)->kind !=Ty_record)
			{
				EM_error(d->pos,"init should not be nil without type specified");
			}

			else if(!cmpty(ty_var,e.ty) && e.ty !=Ty_Nil())
			{
				EM_error(d->pos,"type mismatch");
			}
		}
		S_enter(venv,d->u.var.var,E_VarEntry(actual_ty(e.ty)));
	}
}


Ty_ty transTy(S_table tenv,A_ty a)
{
	switch(a->kind)
	{
		case A_nameTy:
		{
			Ty_ty namety = S_look(tenv,a->u.name);
			if(!namety)
			{
				EM_error(a->pos,"undefined type %s",S_name(a->u.name));
			}
			return Ty_Name(a->u.name,namety);
		}
		case A_recordTy:
		{
			
			Ty_fieldList Ty_recordty=Ty_FieldList(NULL,NULL);
			Ty_fieldList ty_tail=Ty_recordty;
			A_fieldList A_recordty = a->u.record;
			while(A_recordty)
			{
				Ty_ty fieldtyp = S_look(tenv,A_recordty->head->typ);
				if(!fieldtyp)
				{
					EM_error(a->pos,"undefined type %s",S_name(A_recordty->head->typ));
					fieldtyp = Ty_Int();
				}
				
				Ty_field ty_field= Ty_Field(A_recordty->head->name,fieldtyp);
				ty_tail->tail = Ty_FieldList(ty_field,NULL);
				ty_tail=ty_tail->tail;

				A_recordty = A_recordty->tail;
			}
			Ty_fieldList nullhead = Ty_recordty;
			Ty_recordty=Ty_recordty->tail;
			free(nullhead);
			return Ty_Record(Ty_recordty);
			
		}
		case A_arrayTy:
		{
			Ty_ty Ty_array = S_look(tenv,a->u.array);
			if(!Ty_array)
			{
				EM_error(a->pos,"undefined type %s",S_name(a->u.array));
				return Ty_Array(NULL);
			}
			else
				return Ty_Array(Ty_array);
		}
	}
}

void transTypeDec(S_table venv,S_table tenv,A_dec d)
{
	A_nametyList cur = d->u.type;
	A_namety head;
	//first cycle:push type names into tenv to handle recursive type.
	while(cur)
	{
		head = cur->head;
		if(S_look(tenv,head->name))
		{
			EM_error(d->pos,"two types have the same name");
			cur = cur->tail;
			continue;
		}

		else
		{
			S_enter(tenv,head->name,Ty_Name(head->name,NULL));//Handle recursive type.
		}
		cur = cur->tail;
	}

	//second cycle:push actual type of the names to the tenv.
	cur = d->u.type;
	while(cur)
	{
		head = cur->head;
		Ty_ty ty = S_look(tenv,head->name);
		ty->u.name.ty = transTy(tenv,head->ty);
		cur = cur->tail;
	}

	//third cycle:Find wrong recursive typedec.
	cur = d->u.type;
	while(cur)
	{
		head = cur->head;
		Ty_ty init = S_look(tenv,head->name);
		Ty_ty type = init;
		while(type->kind == Ty_name)
		{	type = type->u.name.ty;
			if(type == init)
			{
				EM_error(d->pos,"illegal type cycle");
				init->u.name.ty = Ty_Int();
				break;
			}
		}
		cur = cur->tail;
	}
}

void transFunctionDec(S_table venv,S_table tenv,A_dec d)
{
	A_fundecList funcs = d->u.function;
	A_fundec f;

	//First loop:push function names into venv to handle recursive function
	for(;funcs;funcs = funcs->tail)
	{
		f=funcs->head;
		if(S_look(venv,f->name))
		{
			EM_error(f->pos,"two functions have the same name");
			continue;
		}
		Ty_tyList formalTys = makeFormalTyList(tenv,f->params);
		if(f->result)
		{
			Ty_ty resultTy = S_look(tenv,f->result);
			if(!resultTy)
			{
				EM_error(f->pos,"undefined return type %s",S_name(f->result));
				continue;
			}
			S_enter(venv,f->name,E_FunEntry(formalTys,resultTy));
		}
		else
		{
			S_enter(venv,f->name,E_FunEntry(formalTys,Ty_Void()));
		}
	}

	//Second loop:handle 
	for(funcs = d->u.function;funcs;funcs=funcs->tail)
	{
		f=funcs->head;
		Ty_tyList formalTys = makeFormalTyList(tenv,f->params);
		S_beginScope(venv);
		{
			A_fieldList l; 
			Ty_tyList t;
			/*If error exists in makeFormalTyList,it's likely to error in this loop*/
			for(l=f->params,t=formalTys;l;l=l->tail,t=t->tail)
			{
				S_enter(venv,l->head->name,E_VarEntry(t->head));
			}
		}
		struct expty body = transExp(venv,tenv,f->body);
		E_enventry functy = S_look(venv,f->name);
		if(!cmpty(body.ty,functy->u.fun.result))
		{
			if(actual_ty(functy->u.fun.result)->kind == Ty_void)
			{
				EM_error(f->pos,"procedure returns value");
			}
			EM_error(f->pos,"procedure returns unexpected type");
		}
		S_endScope(venv);
	}
}

Ty_tyList makeFormalTyList(S_table tenv,A_fieldList params)
{
	A_field field;
	Ty_tyList tyList = Ty_TyList(NULL,NULL);
	Ty_tyList tail = tyList;
	Ty_ty cur;
	for(;params;params = params->tail)
	{
		field = params->head;
		cur = S_look(tenv,field->typ);
		if(!cur)
		{
			EM_error(field->pos,"undefined type %s",S_name(field->typ));
			continue;
		}
		tail->tail = Ty_TyList(cur,NULL);
		tail = tail->tail;
	}
	Ty_tyList old = tyList;
	tyList = tyList->tail;
	free(old);
	return tyList;
}