#ifndef CODEGEN_H
#define CODEGEN_H

AS_instrList F_codegen(F_frame f, T_stmList stmList);

//Save or restore callee registers to temp.
void savecalleeregs();
void restorecalleeregs();

static void emit(AS_instr inst);
#endif
