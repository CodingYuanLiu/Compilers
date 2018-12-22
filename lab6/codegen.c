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
#define caller_saved L(F_RAX(),L(F_RDI(),L(F_RSI(),L(F_RDX(),L(F_RCX(),L(F_R8(),L(F_R9(),L(F_R10(),L(F_R11(),NULL)))))))))
#define callee_saved L(F_RBX(),L(F_RBP(),L(F_R12(),L(F_R13(),L(F_R14(),L(F_R15(),NULL))))))
static Temp_temp savedrbx,savedrbp,savedr12,savedr13,savedr14,savedr15;

static AS_instrList iList = NULL,last = NULL;
//static AS_relList rList = NULL;
//Lab 6: put your code here

/*
AS_rel AS_Rel(AS_instr* inst, int offset)
{
    AS_rel relocate = checked_malloc(sizeof(relocate));
    relocate->instr = inst;
    relocate->offset = offset;
    return relocate;
}

AS_relList AS_RelList(AS_instr head, AS_relList tail)
{
    AS_relList relocate_list = checked_malloc(sizeof(relocate_list));
    relocate_list->head = head;
    relocate_list->tail = tail;
    return relocate_list;
}

void addrel(AS_rel rel)
{
    if(!rList)
    {
        rList = AS_RelList(rel,NULL);
    }
    else
    {
        rList = AS_RelList(rel,rList);
    }
}


// Named relocate because it act like relocation.
void relocate(AS_relList rlist,int framesize)
{
    AS_rel rel;
    for(AS_relList relist=rlist;relist;relist = relist->tail)
    {
        rel = relist->head;
        AS_instr inst = *rel->instr;
        //movq (framesize+offset)(%rsp),temp
        string assem = checked_malloc(MAXLEN);
        sprintf(assem,"movq %d(`s0),`d0",framesize+rel->offset);
        free(inst->u.OPER.assem);
        inst->u.OPER.assem = assem;
        free(rel);
    }
    rlist = NULL;
}
*/
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
    //emit(AS_Label(Temp_labelstring(f->name),f->name));
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
                Temp_temp right = munchExp(dst->u.MEM);
                //temps are both live here.
                emit(AS_Oper("movq `s0,(`s1)",NULL,L(left, L(right,NULL)),AS_Targets(NULL)));
                return;
            }
            printf("Wrong T_MOVE stm.");
            break;
        }

        case T_LABEL:
        {
            Temp_label label = s->u.LABEL;
            char *lab = checked_malloc(MAXLEN);
            sprintf(lab,"%s",Temp_labelstring(label)); //".L2:". Colon will be added at AS_print()
            emit(AS_Label(lab,label));
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
            emit(AS_Oper("cmp `s0,`s1",NULL,L(left,L(right,NULL)),AS_Targets(NULL)));
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
    Temp_temp r = Temp_newtemp();
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
                T_exp right = addr->u.BINOP.right;
                if(left->kind == T_TEMP && left->u.TEMP == F_FP())
                {
                    //handle fp here.
                    if(right->kind != T_CONST)
                    {
                        printf("FP's right must be a const\n");
                        return r;
                    }
                    int offset = right->u.CONST;
                    
                    /* movq offset(%fp),%temp; */

                    string instr = checked_malloc(MAXLEN);

                    //AS_instr inst = AS_Oper("movq offset(`s0),`d0",L(r,NULL),L(F_SP(),NULL),AS_Targets(NULL));
                    //addrel(AS_Rel(&inst,offset));
                    //emit(inst);
                    
                    sprintf(instr,"movq (%s-%#x)(`s0),`d0",fs,-offset);
                    emit(AS_Oper(instr,L(r,NULL),L(F_SP(),NULL),AS_Targets(NULL)));
                    return r;
                }
                else
                {
                    Temp_temp lefttemp = munchExp(left);
                    Temp_temp righttemp = munchExp(right);
                    emit(AS_Oper("addq `s0,`d0",L(righttemp,NULL),L(lefttemp,L(righttemp,NULL)),AS_Targets(NULL)));
                    emit(AS_Oper("movq(`s0),`d0",L(r,NULL),L(righttemp,NULL),AS_Targets(NULL)));
                    return r;
                }
            }
            else //addr->kind != T_BINOP
            {
                Temp_temp r1= munchExp(addr);
                emit(AS_Oper("movq (`s0),`d0", L(r, NULL), L(r1, NULL), AS_Targets(NULL)));
                return r;
            }
        }

        case T_BINOP:
        {
            switch(e->u.BINOP.op)
            {
                //Templist may be useful for register allocation.Here may lost some of them.
                case T_plus:
                {
                    Temp_temp left = munchExp(e->u.BINOP.left);
                    Temp_temp right = munchExp(e->u.BINOP.right);
                    emit(AS_Move("movq `s0, `d0", L(r, NULL), L(left, NULL)));
			        emit(AS_Oper("addq `s0, `d0", L(r, NULL), L(right, L(r,NULL)), AS_Targets(NULL)));
                    return r;
                }
                case T_minus:
                {
                    Temp_temp left = munchExp(e->u.BINOP.left);
                    Temp_temp right = munchExp(e->u.BINOP.right);
                    emit(AS_Move("movq `s0, `d0", L(r, NULL), L(left, NULL)));
			        emit(AS_Oper("subq `s0, `d0", L(r, NULL), L(right, L(r,NULL)), AS_Targets(NULL)));
                    return r;
                }
                //Need to consider 128 bits long result.
                case T_mul:
                {
                    Temp_temp left = munchExp(e->u.BINOP.left);
                    Temp_temp right = munchExp(e->u.BINOP.right);
                    emit(AS_Move("movq `s0, `d0", Temp_TempList(F_RAX(), NULL), Temp_TempList(left, NULL)));
                    emit(AS_Oper("imul  s0",L(F_RAX(), L(F_RDX(),NULL)),L(right, L(F_RAX(),NULL)),AS_Targets(NULL)));
                    emit(AS_Move("movq `s0, `d0", Temp_TempList(r, NULL), Temp_TempList(F_RAX(), NULL)));
                    return r;
                }
                case T_div:
                {
                    Temp_temp left = munchExp(e->u.BINOP.left);
			        Temp_temp right = munchExp(e->u.BINOP.right);
                    emit(AS_Move("movq `s0, `d0", Temp_TempList(F_RAX(), NULL), Temp_TempList(left, NULL)));
                    emit(AS_Oper("clto",L(F_RAX(),L(F_RDX(),NULL)),L(F_RAX(),NULL),AS_Targets(NULL)));
                    emit(AS_Oper("idivq `s0",L(F_RAX(),L(F_RDX(),NULL)),
                        L(F_RAX(), L(F_RDX(), L(right,NULL))),AS_Targets(NULL)));
                    emit(AS_Move("movq `s0, `d0", Temp_TempList(r, NULL), Temp_TempList(F_RAX(), NULL)));
                    return r;
                }
            }
        }

        case T_TEMP:
        {
            Temp_temp temp = e->u.TEMP;
            if(temp == F_FP())
            {
                //leaq fs(%rsp),temp. use temp to replace fp
                char *inst = checked_malloc(MAXLEN);
                temp = Temp_newtemp();
                sprintf(inst,"leaq %s(`s0),`d0",fs);
                emit(AS_Move(inst,L(temp,NULL),L(F_SP(),NULL)));
            }
            return temp;
        }
        
        //movq NAME Temp
        case T_NAME:
        {
            string inst = checked_malloc(MAXLEN);
            sprintf(inst,"movq $%s,`d0",Temp_labelstring(e->u.NAME));
            emit(AS_Move(inst,L(r,NULL),NULL));
            return r;
        }
        case T_CONST:
        {
            string inst = checked_malloc(MAXLEN);
            sprintf(inst,"movq $%d,`d0",e->u.CONST);
            emit(AS_Move(inst,L(r,NULL),NULL));
            return r;
        }
        case T_CALL:
        {
            Temp_label func = e->u.CALL.fun->u.NAME;
            string inst=checked_malloc(MAXLEN);
            
            munchArgs(e->u.CALL.args);

            sprintf(inst, "call %s", Temp_labelstring(func));
	        emit(AS_Oper(inst, caller_saved, NULL, AS_Targets(NULL)));
	        emit(AS_Move("movq `s0, `d0", L(r, NULL), L(F_RAX(), NULL)));

            return r;
        }
    }   
}

void munchArgs(T_expList list)
{
    int num = 1;
    Temp_temp arg;
    for(T_expList l = list;l;l = l->tail,num++)
    {
        arg = munchExp(l->head);
        switch(num)
        {
            case 1: emit(AS_Move("movq `s0,`d0",L(F_RDI(),NULL),L(arg,NULL)));break;
            case 2: emit(AS_Move("movq `s0,`d0",L(F_RSI(),NULL),L(arg,NULL)));break;
            case 3: emit(AS_Move("movq `s0,`d0",L(F_RDX(),NULL),L(arg,NULL)));break;
            case 4: emit(AS_Move("movq `s0,`d0",L(F_RCX(),NULL),L(arg,NULL)));break;
            case 5: emit(AS_Move("movq `s0,`d0",L(F_R8(),NULL),L(arg,NULL)));break;
            case 6: emit(AS_Move("movq `s0,`d0",L(F_R9(),NULL),L(arg,NULL)));break;
            default:
                emit(AS_Oper("pushq `s0",NULL,L(arg,NULL),AS_Targets(NULL)));break;
        }
    }
    return;
}