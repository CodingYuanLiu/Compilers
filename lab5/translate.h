#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "util.h"
#include "absyn.h"
#include "temp.h"
#include "frame.h"

/* Lab5: your code below */
static F_fragList frags = NULL;

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;

typedef struct patchList_ *patchList;
struct patchList_ 
{
	Temp_label *head; 
	patchList tail;
};

typedef struct Tr_expList_ *Tr_expList;
struct Tr_expList_
{
	Tr_exp head;
	Tr_expList tail;
};

Tr_expList Tr_ExpList(Tr_exp head,Tr_expList tail);

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);

Tr_access Tr_allocLocal(Tr_level level, bool escape);

void Tr_procEntryExit(Tr_level level,Tr_exp func_body);

static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues,patchList falses,T_stm stm); 

static T_exp Tr_unEx(Tr_exp e);
static T_stm Tr_unNx(Tr_exp e);
static struct Cx Tr_unCx(Tr_exp e);


Tr_exp Tr_simpleVar(Tr_access access,Tr_level level);
Tr_exp Tr_fieldVar(Tr_exp loc, int order);
Tr_exp Tr_subscriptVar(Tr_exp loc,Tr_exp subscript);
Tr_exp Tr_Nil();
Tr_exp Tr_Int(int value);
Tr_exp Tr_String(string str);
Tr_exp Tr_Call(Temp_label label,Tr_expList args,Tr_level caller,Tr_level callee);
Tr_exp Tr_Calculate(A_oper op,Tr_exp left,Tr_exp right);
Tr_exp Tr_Condition(A_oper op,Tr_exp left,Tr_exp right);
Tr_exp Tr_Recordexp(Tr_expList fields);
T_stm Tr_mk_record_array(Tr_expList fields,Temp_temp r,int offset,int size);
Tr_exp Tr_Assign(Tr_exp var,Tr_exp exp);
Tr_exp Tr_If(Tr_exp test,Tr_exp then,Tr_exp elsee);
Tr_exp Tr_While(Tr_exp test,Tr_exp body,Temp_label done);
Tr_exp Tr_For(Tr_access loopv,Tr_exp lo, Tr_exp hi, Tr_exp body,Tr_level l ,Temp_label done);
Tr_exp Tr_Break(Temp_label done);
F_fragList Tr_getresult(){ return frags;}

#endif
