#ifndef CODEGEN_H
#define CODEGEN_H

AS_instrList F_codegen(F_frame f, T_stmList stmList);

//Save or restore callee registers to temp.
static void savecalleeregs();
static void restorecalleeregs();

static void emit(AS_instr inst);

static void munchStm(T_stm stm);
static Temp_temp munchExp(T_exp exp);
#endif
