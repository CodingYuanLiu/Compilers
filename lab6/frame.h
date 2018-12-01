
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"

typedef struct F_frame_ *F_frame;

typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;

struct F_accessList_ {F_access head; F_accessList tail;};


/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ 
{
	F_frag head; 
	F_fragList tail;
};

F_fragList F_FragList(F_frag head, F_fragList tail);

/* declaration for frames*/

static const int wordsize = 8;
//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

struct F_frame_{
	Temp_label name;
	F_accessList formals;
	F_accessList locals;
	T_stmList view_shift;
	int s_offset;//Which is commonly a minus number.
};

F_frame F_newFrame(Temp_label name,U_boolList escapes);

F_accessList F_formals(F_frame f);

F_access F_allocLocal(F_frame f,bool escape);

static F_access InFrame(int offset);

static F_access InReg(Temp_temp reg);

Temp_label F_name(F_frame f);

F_accessList F_AccessList(F_access head,F_accessList tail);

T_exp F_exp(F_access access,T_exp frame_ptr);

T_exp F_externalCall(string s,T_expList args);

Temp_temp F_FP(void);

Temp_temp F_RV(void);

Temp_temp F_RDI();

Temp_temp F_RSI();

Temp_temp F_RDX();

Temp_temp F_RCX();

Temp_temp F_R8();

Temp_temp F_R9();

T_stm F_procEntryExit1(F_frame frame,T_stm stm);

#endif
