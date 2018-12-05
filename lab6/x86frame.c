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
	F_accessList formals = F_AccessList(NULL,NULL);
	F_accessList ftail = formals;
	T_stmList view_shift = T_StmList(NULL,NULL);
	T_stmList tail = view_shift;
	
	newframe->s_offset = 0;
	
	bool escape;
	Temp_temp temp;

	int formal_off = wordsize;// The seventh arg was located at 8(%rbp)

	int num=1; //Marks the sequence of the formals.
	/*If the formal is escape, then allocate it on the frame.
	  Else,allocate it on the temp.*/
	for(;escapes;escapes=escapes->tail,num++)
	{
		escape = escapes->head;
		if(escape)
		{	
			switch(num)
			{
				case 1: 
				{
					tail->tail = T_StmList(T_Move(T_Mem(T_Const(-num * wordsize)),T_Temp(F_RDI())),NULL);
					newframe->s_offset -= wordsize;
					ftail->tail = F_AccessList(InFrame(-num * wordsize),NULL);
					ftail = ftail->tail; tail = tail->tail;
					break;
				}
				case 2:
				{ 
					tail->tail = T_StmList(T_Move(T_Mem(T_Const(-num*wordsize)),T_Temp(F_RSI())),NULL);
					newframe->s_offset -= wordsize;
					ftail->tail = F_AccessList(InFrame(-num * wordsize),NULL);
					ftail = ftail->tail; tail = tail->tail;
					break;
				}
				case 3: 
				{
					tail->tail = T_StmList(T_Move(T_Mem(T_Const(-num*wordsize)),T_Temp(F_RDX())),NULL);
					newframe->s_offset -= wordsize;
					ftail->tail = F_AccessList(InFrame(-num * wordsize),NULL);
					ftail = ftail->tail; tail = tail->tail;
					break;
				}
				case 4: 
				{
					tail->tail = T_StmList(T_Move(T_Mem(T_Const(-num*wordsize)),T_Temp(F_RCX())),NULL);
					newframe->s_offset -= wordsize;
					ftail->tail = F_AccessList(InFrame(-num * wordsize),NULL);
					ftail = ftail->tail; tail = tail->tail;
					break;
				}
				case 5: 
				{
					tail->tail = T_StmList(T_Move(T_Mem(T_Const(-num*wordsize)),T_Temp(F_R8())),NULL);
					newframe->s_offset -= wordsize;
					ftail->tail = F_AccessList(InFrame(-num * wordsize),NULL);
					ftail = ftail->tail; tail = tail->tail;
					break;
				}
				case 6: 
				{
					tail->tail = T_StmList(T_Move(T_Mem(T_Const(-num*wordsize)),T_Temp(F_R9())),NULL);
					newframe->s_offset -= wordsize;
					ftail->tail = F_AccessList(InFrame(-num * wordsize),NULL);
					ftail = ftail->tail; tail = tail->tail;
					break;
				}
				default:
				{
					ftail->tail = F_AccessList(InFrame(formal_off),NULL);//sequence of formals here is reversed.
					ftail = ftail->tail;
					formal_off += wordsize;
				}
			}
		}
		else
		{
			temp = Temp_newtemp();
			switch(num)
			{
				case 1: tail->tail=T_StmList(T_Move(T_Temp(temp),T_Temp(F_RDI())),NULL);tail = tail->tail;break;
				case 2: tail->tail=T_StmList(T_Move(T_Temp(temp),T_Temp(F_RSI())),NULL);tail = tail->tail;break;
				case 3: tail->tail=T_StmList(T_Move(T_Temp(temp),T_Temp(F_RDX())),NULL);tail = tail->tail;break;
				case 4: tail->tail=T_StmList(T_Move(T_Temp(temp),T_Temp(F_RCX())),NULL);tail = tail->tail;break;
				case 5: tail->tail=T_StmList(T_Move(T_Temp(temp),T_Temp(F_R8())),NULL);tail = tail->tail;break;
				case 6: tail->tail=T_StmList(T_Move(T_Temp(temp),T_Temp(F_R9())),NULL);tail = tail->tail;break;
				default: printf("Frame: the 7-nth formal should be passed on frame.");
			}
			formals = F_AccessList(InReg(temp),formals);
		}
	}
	formals = formals->tail;
	view_shift = view_shift->tail;
	newframe->formals = formals;
	newframe->locals = NULL;
	newframe->view_shift = view_shift;
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
	T_stmList l = frame->view_shift;
	T_stm bind = l->head;
	for(l = l->tail;l;l = l->tail)
	{
		bind = T_Seq(bind,l->head);
	}
	bind = T_Seq(bind,stm);
	return bind;
}

AS_instrList F_procEntryExit2(AS_instrList body)
{
	static Temp_tempList returnSink = NULL;
	if(!returnSink)
		returnSink = Temp_TempList(rsp,NULL);
	return AS_splice(body,AS_InstrList(AS_Oper("",NULL,returnSink,NULL),NULL));
}

T_exp F_exp(F_access access,T_exp frame_ptr)
{
	if(access->kind = inFrame)
	{
		return T_Mem(T_Binop(T_plus,frame_ptr,T_Const(access->u.offset)));
	}
	else
	{
		return T_Temp(access->u.reg);
	}
}

T_exp F_externalCall(string s,T_expList args)
{
	return T_Call(T_Name(Temp_namedlabel(s)),args);
}

Temp_temp F_FP()
{
	if(!fp)
		fp = Temp_newtemp();
	return fp;	
}

Temp_temp F_RV()
{
	if(!rax)
		rax = Temp_newtemp();
	return rax;	
}

Temp_temp F_SP()
{
	if(!rsp)
		rsp = Temp_newtemp();
	return rsp;	
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

Temp_temp F_R10()
{
	if(!r10)
		r10 = Temp_newtemp();
	return r10;
}

Temp_temp F_R11()
{
	if(!r11)
		r11 = Temp_newtemp();
	return r11;
}

Temp_temp F_R12()
{
	if(!r12)
		r12 = Temp_newtemp();
	return r12;
}

Temp_temp F_R13()
{
	if(!r13)
		r13 = Temp_newtemp();
	return r13;
}

Temp_temp F_R14()
{
	if(!r14)
		r14 = Temp_newtemp();
	return r14;
}

Temp_temp F_R15()
{
	if(!r15)
		r15 = Temp_newtemp();
	return r15;
}

Temp_temp F_RBX()
{
	if(!rbx)
		rbx = Temp_newtemp();
	return rbx;
}

Temp_temp F_RBP()
{
	if(!rbp)
		rbp = Temp_newtemp();
	return rbp;
}