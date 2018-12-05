#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

//caller_saved register: rax rdi rsi rdx rcx r8 r9 r10 r11
//callee_saved register: rbx rbp r12 r13 r14 r15 

static Temp_temp savedrbx,savedrbp,savedr12,savedr13,savedr14,savedr15;

static AS_instrList iList = NULL,last = NULL;

//Lab 6: put your code here
static void emit(AS_instr inst)
{
    if(iList)
    {
        last->tail = AS_InstrList(inst,NULL);
        last = last->tail;
    }
    else
    {
        iList = AS_InstrList(inst,NULL);
        last = iList;
    }
}

static char fs[256];
AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList list;
    T_stmList s;
    savecalleeregs();
    
    sprintf(fs,"%s_framesize",S_name(f->name));
    for(s=stmList;s;s=s->tail)
    {
        munchStm(s->head);
    }
    restorecalleeregs();
    list = iList;
    iList = NULL;
    return F_procEntryExit2(list);
}

static void savecalleeregs()
{
    savedrbx = Temp_newtemp();
    savedrbp = Temp_newtemp(); //Saved rbp here.
    savedr12 = Temp_newtemp();
    savedr13 = Temp_newtemp();
    savedr14 = Temp_newtemp();
    savedr15 = Temp_newtemp();
    emit(AS_Move("movq `s0,`d0",Temp_TempList(savedrbx,NULL),Temp_TempList(F_RBX(),NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(savedrbp,NULL),Temp_TempList(F_RBP(),NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(savedr12,NULL),Temp_TempList(F_R12(),NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(savedr13,NULL),Temp_TempList(F_R13(),NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(savedr14,NULL),Temp_TempList(F_R14(),NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(savedr15,NULL),Temp_TempList(F_R15(),NULL)));
}

static void restorecalleeregs()
{
    emit(AS_Move("movq `s0,`d0",Temp_TempList(F_RBX(),NULL),Temp_TempList(savedrbx,NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(F_RBP(),NULL),Temp_TempList(savedrbp,NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(F_R12(),NULL),Temp_TempList(savedr12,NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(F_R13(),NULL),Temp_TempList(savedr13,NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(F_R14(),NULL),Temp_TempList(savedr14,NULL)));
    emit(AS_Move("movq `s0,`d0",Temp_TempList(F_R15(),NULL),Temp_TempList(savedr15,NULL)));
}

Temp_tempList L(Temp_temp h,Temp_tempList t){
    return Temp_TempList(h,t);
}

static void munchStm(T_stm s)
{
    switch (s->kind)
    {
        /*movq mem,reg; movq reg,mem; movq irr,mem; movq reg,reg*/
        case T_MOVE:
        {
            T_exp dst = s->u.MOVE.dst;
            T_exp src = s->u.MOVE.src;
            /* movq reg,reg */
            if(dst->kind == T_TEMP && src->kind == T_TEMP)
            {
                emit(AS_Move("movq `s0,`d0",Temp_TempList(dst->u.TEMP,NULL),Temp_TempList(src->u.TEMP,NULL)));
                return;
            }

            /* movq imm,reg; or movq mem,reg; */
            Temp_temp left = munchExp(src);
            {
                if(dst->kind == T_TEMP)
                {
                    emit(AS_Move("movq `s0,`d0",Temp_TempList(dst->u.TEMP,NULL),Temp_TempList(left,NULL)));
                }
                
            }

        }
    }
}