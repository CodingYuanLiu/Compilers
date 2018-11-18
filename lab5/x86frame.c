#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/

F_access InFrame(int offset)
{
	F_access new_access = checked_malloc(sizeof(*new_access));
	new_access->kind = inFrame;
	new_access->u.offset=offset;
	return new_access;
}

F_access InReg(Temp_temp temp)
{
	F_access new_access = checked_malloc(sizeof(*new_access));
	new_access->kind = inReg;
	new_access->u.reg = temp;
	return new_access;
}

F_access F_allocLocal(F_frame f,bool escape)
{
	F_access local = checked_malloc(sizeof(*local));
	if(escape)
	{
		local->kind = inFrame;
		local->u.offset = f->s_offset;
		f->s_offset -= wordsize;
	}
	else
	{
		local->kind = inReg;
		local->u.reg = Temp_newtemp();
	}
	return local;
}

Temp_label F_name(F_frame f)
{
	return f->name;
}

F_accessList F_formals(F_frame f)
{
	return f->formals;
}

F_accessList F_AccessList(F_access head,F_accessList tail)
{
	F_accessList accesslist = checked_malloc(sizeof(*accesslist));
	accesslist->head = head;
	accesslist->tail = tail;
	return accesslist;
}                                                  

F_frame F_newFrame(Temp_label name,U_boolList escapes)
{
	F_frame newframe=checked_malloc(sizeof(*newframe));
	newframe->name=name;
	F_accessList formals = NULL;
	T_stm view_shift = NULL;
	bool escape;
	Temp_temp temp;

	//The first formal is the static link.
	int formal_off = 2 * wordsize;//16(%rbp) stores the static link.
	//(%rbp) is old rbp value.8(%rbp) is the return address.

	int num=1; //Marks the sequence of the formals.
	/*If the formal is escape, then allocate it on the frame.
	  Else,allocate it on the temp.*/
	for(;escapes;escapes=escapes->tail)
	{
		escape = escapes->head;
		if(escape)
		{
			formals = F_AccessList(InFrame(formal_off),formals);//sequence of formals here is reversed.
			formal_off += wordsize;
		}
		else
		{
			temp = Temp_newtemp();
			switch(num)
			{
				case 1: view_shift=T_StmList(T_Move(T_Temp(temp),T_Temp(F_RDI())),view_shift);break;
				case 2: view_shift=T_StmList(T_Move(T_Temp(temp),T_Temp(F_RSI())),view_shift);break;
				case 3: view_shift=T_StmList(T_Move(T_Temp(temp),T_Temp(F_RDX())),view_shift);break;
				case 4: view_shift=T_StmList(T_Move(T_Temp(temp),T_Temp(F_RCX())),view_shift);break;
				case 5: view_shift=T_StmList(T_Move(T_Temp(temp),T_Temp(F_R8())),view_shift);break;
				case 6: view_shift=T_StmList(T_Move(T_Temp(temp),T_Temp(F_R9())),view_shift);break;
				default: printf("Frame: the 7-nth formal should be passed on frame.");
			}
			num += 1;
			formals = F_AccessList(InReg(temp),formals);
		}
	}
	newframe->formals = formals;
	newframe->locals = NULL;
	newframe->s_offset = 0;
	return newframe;
}

F_frag F_StringFrag(Temp_label label, string str) 
{   
	F_frag strfrag = checked_malloc(sizeof(*strfrag));
	strfrag->kind = F_stringFrag;
	strfrag->u.stringg.label=label;
	strfrag->u.stringg.str=str;
	return strfrag;                                      
}                                                     
                                                      
F_frag F_ProcFrag(T_stm body, F_frame frame) 
{        
	F_frag procfrag = checked_malloc(sizeof(*procfrag));
	procfrag->kind = F_procFrag;
	procfrag->u.proc.body=body;
	procfrag->u.proc.frame=frame;
	return procfrag;                                      
}                                                     
                                                      
F_fragList F_FragList(F_frag head, F_fragList tail) 
{ 
	F_fragList fraglist = checked_malloc(sizeof(*fraglist));
	fraglist->head = head;
	fraglist->tail=tail;
	return fraglist;                                      
}

T_stm F_procEntryExit1(F_frame frame,T_stm stm)
{
	return stm;
}


T_exp F_exp(F_access access,T_exp frame_ptr)
{
	if(access->kind = inFrame)
	{
		return T_Mem(T_Binop(T_plus,frame_ptr,access->u.offset));
	}
	else
	{
		return T_Temp(access->u.reg);
	}
}



static Temp_temp rbp = NULL;
static Temp_temp rax = NULL;
static Temp_temp rdi = NULL;
static Temp_temp rsi = NULL;
static Temp_temp rdx = NULL;
static Temp_temp rcx = NULL;
static Temp_temp r8 = NULL;
static Temp_temp r9 = NULL;

Temp_temp F_FP()
{
	if(!rbp)
		rbp = Temp_newtemp();
	return rbp;	
}

Temp_temp F_RV()
{
	if(!rax)
		rax = Temp_newtemp();
	return rax;	
}

Temp_temp F_RDI()
{
	if(!rdi)
		rdi = Temp_newtemp();
	return rdi;	
}

Temp_temp F_RSI()
{
	if(!rsi)
		rsi = Temp_newtemp();
	return rsi;	
}

Temp_temp F_RDX()
{
	if(!rdx)
		rdx = Temp_newtemp();
	return rdx;	
}

Temp_temp F_RCX()
{
	if(!rcx)
		rcx = Temp_newtemp();
	return rcx;	
}

Temp_temp F_R8()
{
	if(!r8)
		r8 = Temp_newtemp();
	return r8;	
}

Temp_temp F_R9()
{
	if(!r9)
		r9 = Temp_newtemp();
	return r9;	
}