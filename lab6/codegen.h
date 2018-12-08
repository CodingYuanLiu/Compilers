#ifndef CODEGEN_H
#define CODEGEN_H

AS_instrList F_codegen(F_frame f, T_stmList stmList);

//Save or restore callee registers to temp.
static void savecalleeregs();
static void restorecalleeregs();

static void emit(AS_instr inst);

static void munchStm(T_stm stm);
static Temp_temp munchExp(T_exp exp);
Temp_tempList L(Temp_temp h,Temp_tempList t);


/* To handle frame ptr, save all the instructions with fp and emit them after reg_allocation*/
typedef struct AS_rel_* AS_rel;
typedef struct AS_relList_* AS_relList;
struct AS_rel_//To handle frame ptr
{
    AS_instr* instr; // The instruction with virtual frame ptr
    int offset;//offset off frame ptr;
};

struct AS_relList_
{
    AS_rel head;
    AS_relList tail;
};

// Constructor
AS_rel AS_Rel(AS_instr* inst, int offset);
AS_relList AS_RelList(AS_instr head, AS_relList tail);
void addrel(AS_rel rel);
void relocate(AS_relList rlist,int framesize);

#endif
