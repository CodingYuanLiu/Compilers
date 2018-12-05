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

#define MAXLEN 256

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
        /* need to munch src exp first */
        case T_MOVE:
        {
            T_exp dst = s->u.MOVE.dst;
            T_exp src = s->u.MOVE.src;
            /* movq mem,reg,movq imm,reg, movq reg,reg*/
            if(dst->kind == T_TEMP)
            {
                Temp_temp left = munchExp(src);
                emit(AS_Move("movq `s0,`d0",Temp_TempList(dst->u.TEMP,NULL),Temp_TempList(left,NULL)));
                return;
            }

            /* movq reg,mem*/
            if(dst->kind == T_MEM)
            {
                Temp_temp left = munchExp(src);
                Temp_temp right = munchExp(dst);
                emit(AS_Move("movq `s0,(`d0)",Temp_TempList(right,NULL),Temp_TempList(left,NULL)));
                return;
            }
            printf("Wrong T_MOVE stm.");
            break;
        }

        case T_LABEL:
        {
            Temp_label label = s->u.LABEL;
            char *lab = checked_malloc(MAXLEN);
            sprintf(lab,"%s:",Temp_labelstring(label));
            emit(AS_Label(lab,label));//The second argument of AS_Label() seems useless.
            return;
        }

        case T_JUMP:
        {
            Temp_label label = s->u.JUMP.exp->u.NAME;
            char *jmp = checked_malloc(MAXLEN);
            sprintf(jmp,"jmp %s",Temp_labelstring(label));
            emit(AS_Oper(jmp,NULL,NULL,AS_Targets(s->u.JUMP.jumps)));
            return;
        }

        case T_CJUMP:
        {
            char* cjmp = checked_malloc(MAXLEN);
            char *op;
            Temp_temp left = munchExp(s->u.CJUMP.left);
            Temp_temp right = munchExp(s->u.CJUMP.right);
            switch(s->u.CJUMP.op)
            {
                case T_eq: op = String("je");break;
                case T_ne: op = String("jne");break;
                case T_lt: op = String("jl");break;
                case T_gt: op = String("jg");break;
                case T_le: op = String("jle");break;
                case T_ge: op = String("jge");break;
            }
            emit(AS_Oper("cmp `s0,`s1",NULL,L(left,right),AS_Targets(NULL)));
            sprintf(cjmp,"%s %s",op,Temp_labelstring(s->u.CJUMP.true));
            emit(AS_Oper(cjmp,NULL,NULL,AS_Targets(Temp_LabelList(s->u.CJUMP.true,NULL))));
            return;
        }

        case T_EXP:
        {
            munchExp(s->u.EXP);
            return;
        }
    }
}

static Temp_temp munchExp(T_exp e)
{
    switch(e->kind)
    {
        //May contain F_FP() in it: MEM(+(FP,EXP)).
        case T_MEM:
        {
            T_exp addr = e->u.MEM;
            //May contain F_FP() in it.
            if(addr->kind == T_BINOP)
            {
                T_exp left = addr->u.BINOP.left;
                if(left->kind == T_TEMP && left->u.TEMP == F_FP())
                {
                    //handle fp here.
                }
                
            }
        }
    }
}