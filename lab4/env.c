#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "env.h"

/*Lab4: Your implementation of lab4*/

E_enventry E_VarEntry(Ty_ty ty)
{
	E_enventry v = checked_malloc(sizeof(v));
	v->kind=E_varEntry;
	v->readonly = 0;
	v->u.var.ty=ty;
	return v;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result)
{
	E_enventry func = checked_malloc(sizeof(func));
	func->kind = E_funEntry;
	func->u.fun.formals=formals;
	func->u.fun.result=result;
	return func;
}

S_table E_base_tenv(void)
{
	S_table base = S_empty();
	S_enter(base,S_Symbol("int"),Ty_Int());
	S_enter(base,S_Symbol("string"),Ty_String());
	return base;
}

S_table E_base_venv(void)
{
	S_table base;
	/* print(string),flush(),getchar(),ord(string),
	chr(int),size(string) substring(string,int,int)
	concat(string,string) not(int) exit(int)*/
	base=S_empty()	;//Add basic functions later.

	return base;
}
