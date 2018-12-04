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

AS_instrList iList = NULL,last = NULL;

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


AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList list;
    
}

void savecalleeregs()
{
    
}
