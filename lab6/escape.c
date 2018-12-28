#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "escape.h"
#include "table.h"
#include "env.h"

void Esc_findEscape(A_exp exp) {
	S_table env= S_empty();
	traverseExp(env,1,exp);	
}

static void traverseExp(S_table env,int depth,A_exp exp)
{
	switch(exp->kind)
	{
		case A_varExp:
		{
			traverseVar(env,depth,exp->u.var);
			break;
		}
		case A_callExp:
		{
			A_expList args = exp->u.call.args;
			for(;args;args=args->tail)
			{
				traverseExp(env,depth,args->head);
			}
			break;
		}
		case A_opExp:
		{
			traverseExp(env,depth,exp->u.op.left);
			traverseExp(env,depth,exp->u.op.right);
			break;
		}
		case A_recordExp:
		{
			A_efieldList fields = exp->u.record.fields;
			for(;fields;fields = fields->tail)
			{
				traverseExp(env,depth,fields->head->exp);
			}
			break;
		}
		case A_seqExp:
		{
			A_expList seq = exp->u.seq;
			for(;seq;seq = seq->tail)
			{
				traverseExp(env,depth,seq->head);
			}
			break;
		}
		case A_assignExp:
		{
			traverseVar(env,depth,exp->u.assign.var);
			traverseExp(env,depth,exp->u.assign.exp);
			break;
		}
		case A_ifExp:
		{
			traverseExp(env,depth,exp->u.iff.test);
			traverseExp(env,depth,exp->u.iff.then);
			if(exp->u.iff.elsee)
				traverseExp(env,depth,exp->u.iff.elsee);
			break;
		}
		case A_whileExp:
		{
			traverseExp(env,depth,exp->u.whilee.test);
			traverseExp(env,depth,exp->u.whilee.body);
			break;
		}
		case A_forExp:
		{
			//need to analyse.
			exp->u.forr.escape = FALSE;
			S_enter(env,exp->u.forr.var,E_EscEntry(depth, &exp->u.forr.escape));

			traverseExp(env,depth,exp->u.forr.lo);
			traverseExp(env,depth,exp->u.forr.hi);
			traverseExp(env,depth,exp->u.forr.body);
			break;
		}
		case A_letExp:
		{
			for(A_decList decs = exp->u.let.decs;decs;decs = decs->tail)
			{
				traverseDec(env,depth,decs->head);
			}
			traverseExp(env,depth,exp->u.let.body);
			break;
		}
		case A_arrayExp:
		{
			traverseExp(env,depth,exp->u.array.init);
			traverseExp(env,depth,exp->u.array.size);
			break;
		}
		case A_nilExp:
		case A_intExp:
		case A_breakExp:
		case A_stringExp:break;
	}
}

static void traverseVar(S_table env,int depth,A_var var)
{
	switch(var->kind)
	{
		case A_simpleVar:
		{
			E_enventry entry = S_look(env,var->u.simple);
			if(depth > entry->u.esc.depth)
			{
				*(entry->u.esc.escape) = TRUE;
			}
			break;
		}
		case A_fieldVar:
		{
			traverseVar(env,depth,var->u.field.var);
			break;
		}
		case A_subscriptVar:
		{
			traverseVar(env,depth,var->u.subscript.var);
			traverseExp(env,depth,var->u.subscript.exp);
			break;
		}
	}
}

static void traverseDec(S_table env,int depth,A_dec dec)
{
	switch(dec->kind)
	{
		case A_varDec:
		{
			dec->u.var.escape = FALSE;
			S_enter(env,dec->u.var.var,E_EscEntry(depth,&dec->u.var.escape));
			traverseExp(env,depth,dec->u.var.init);
			break;
		}
		case A_functionDec:
		{
			A_fundecList decs= dec->u.function;
			for(;decs;decs = decs->tail)
			{
				A_fundec fundec = decs->head;
				S_beginScope(env);
				A_fieldList formals = fundec->params;
				for(;formals;formals = formals->tail)
				{
					A_field formal = formals->head;
					formal->escape = FALSE;
					S_enter(env,formal->name,E_EscEntry(depth+1 ,&formal->escape));
				}
				traverseExp(env,depth+1,fundec->body);
				S_endScope(env);
			}
			break;
		}
		case A_typeDec:
			break;
	}
}