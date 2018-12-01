#ifndef __SEMANT_H_
#define __SEMANT_H_

#include "absyn.h"
#include "symbol.h"
#include "temp.h"
#include "frame.h"
#include "translate.h"
#include <stdlib.h>
struct expty;

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label);
struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label);
Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label);
Ty_ty		 transTy (              S_table tenv, A_ty a);

struct expty transNilexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transIntexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transStringexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transCallexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transOpexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transRecordexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transSeqexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transAssignexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transIfexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transWhileexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transForexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transBreakexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transLetexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);
struct expty transArrayexp(S_table venv, S_table tenv, A_exp a,Tr_level l,Temp_label label);

Tr_exp transVarDec(S_table venv,S_table tenv,A_dec d,Tr_level l,Temp_label label);
Tr_exp transTypeDec(S_table venv,S_table tenv,A_dec d,Tr_level l,Temp_label label);
Tr_exp transFunctionDec(S_table venv,S_table tenv,A_dec d,Tr_level l,Temp_label label);

Ty_tyList makeFormalTyList(S_table tenv,A_fieldList params);

//void SEM_transProg(A_exp exp);
F_fragList SEM_transProg(A_exp exp);
Ty_ty actual_ty(Ty_ty ty);
bool cmpty(Ty_ty ty1,Ty_ty ty2);

#endif
