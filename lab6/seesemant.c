

#include <stdio.h>

#include <assert.h>

#include <string.h>

#include "util.h"

#include "errormsg.h"

#include "symbol.h"

#include "absyn.h"

#include "types.h"

#include "env.h"

#include "semant.h"

#include "helper.h"

#include "translate.h"

#include "table.h"

#include "env.h"

/*Lab5: Your implementation of lab5.*/

struct expty
{

    Tr_exp exp;

    Ty_ty ty;
};

struct expty expTy(Tr_exp exp, Ty_ty ty)
{

    struct expty e;

    e.exp = exp;

    e.ty = ty;

    return e;
}

Ty_ty actual_ty(Ty_ty t)
{

    while (t && t->kind == Ty_name)

        t = t->u.name.ty;

    return t;
}

int hasLoopVar(S_table venv, A_var v)
{

    switch (v->kind)
    {

    case A_simpleVar:
    {

        E_enventry x = S_look(venv, v->u.simple);

        if (x->readonly)
        {

            EM_error(v->pos, "loop variable can't be assigned");

            return 1;
        }

        return 0;
    }

    case A_fieldVar:

        return hasLoopVar(venv, v->u.field.var);

    case A_subscriptVar:

        return hasLoopVar(venv, v->u.subscript.var);
    }

    assert(0); /* should have returned from some clause of the switch */
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params)
{

    if (params == NULL)
    {

        return NULL;
    }

    Ty_ty type = S_look(tenv, params->head->typ);

    return Ty_TyList(type, makeFormalTyList(tenv, params->tail));
}

U_boolList makeFormalEscList(A_fieldList params)
{

    if (params == NULL)
    {

        return NULL;
    }

    return U_BoolList(params->head->escape, makeFormalEscList(params->tail));
}

Ty_fieldList makeFieldList(S_table tenv, A_fieldList fields)
{

    // TODO: simplify recursion

    Ty_ty type = S_look(tenv, fields->head->typ);

    if (!type)
    {

        EM_error(fields->head->pos, "undefined type %s", S_name(fields->head->typ));

        type = Ty_Int();
    }

    Ty_field field = Ty_Field(fields->head->name, type);

    if (fields->tail == NULL)
    {

        return Ty_FieldList(field, NULL);
    }
    else
    {

        return Ty_FieldList(field, makeFieldList(tenv, fields->tail));
    }
}

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level curr_level, Temp_label nearest_break)
{

    // EM_error(v->pos, "made it here, var");

    switch (v->kind)
    {

    case A_simpleVar:
    {

        // check type

        E_enventry var_entry = S_look(venv, v->u.simple);

        if (!var_entry || var_entry->kind != E_varEntry)
        {

            EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));

            return expTy(NULL, Ty_Int());
        }

        // get IR node

        Tr_exp tr_exp = Tr_simpleVar(var_entry->u.var.access, curr_level);

        return expTy(tr_exp, actual_ty(var_entry->u.var.ty));
    }

    case A_fieldVar:
    {

        // check left value

        struct expty lvalue = transVar(venv, tenv, v->u.field.var, curr_level, nearest_break);

        if (lvalue.ty->kind != Ty_record)
        {

            EM_error(v->pos, "not a record type");

            return expTy(NULL, Ty_Int());
        }

        // find record field and its order

        Ty_fieldList fields = lvalue.ty->u.record;

        int order = 0;

        while (fields && fields->head->name != v->u.field.sym)
        {

            fields = fields->tail;

            order++;
        }

        if (fields == NULL)
        {

            EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));

            return expTy(NULL, Ty_Int());
        }

        // get IR node

        Tr_exp tr_exp = Tr_fieldVar(lvalue.exp, order);

        return expTy(tr_exp, actual_ty(fields->head->ty));
    }

    case A_subscriptVar:
    {

        // check left value and index

        struct expty lvalue = transVar(venv, tenv, v->u.subscript.var, curr_level, nearest_break);

        struct expty index = transExp(venv, tenv, v->u.subscript.exp, curr_level, nearest_break);

        if (lvalue.ty->kind != Ty_array)
        {

            EM_error(v->pos, "array type required");

            return expTy(NULL, Ty_Int());
        }

        if (index.ty->kind != Ty_int)
        {

            EM_error(v->pos, "index type is not int");

            return expTy(NULL, Ty_Int());
        }

        // get IR node

        Tr_exp tr_exp = Tr_arrayVar(lvalue.exp, index.exp);

        return expTy(tr_exp, actual_ty(lvalue.ty->u.array));
    }
    }

    assert(0); /* should have returned from some clause of the switch */
}

struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level curr_level, Temp_label nearest_break)
{

    // EM_error(a->pos, "made it here, exp");

    switch (a->kind)
    {

    case A_varExp:

        return transVar(venv, tenv, a->u.var, curr_level, nearest_break);

    case A_nilExp:

        return expTy(Tr_nil(), Ty_Nil());

    case A_intExp:

        return expTy(Tr_int(a->u.intt), Ty_Int());

    case A_stringExp:

        return expTy(Tr_string(a->u.stringg), Ty_String());

    case A_callExp:
    {

        // check function declaration

        E_enventry fun_entry = S_look(venv, a->u.call.func);

        if (!fun_entry || fun_entry->kind != E_funEntry)
        {

            EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));

            return expTy(NULL, Ty_Int());
        }

        // check parameter types, and build parameter list

        Ty_tyList expects = fun_entry->u.fun.formals;

        A_expList args = a->u.call.args;

        Tr_expList list = Tr_ExpList(NULL, NULL); // dummy head node

        Tr_expList tail = list;

        for (; args && expects; args = args->tail, expects = expects->tail)
        {

            // translate parameter

            struct expty var_entry = transExp(venv, tenv, args->head, curr_level, nearest_break);

            // same type or record with nil

            Ty_ty expected = actual_ty(expects->head);

            Ty_ty actual = actual_ty(var_entry.ty);

            if (expected != actual && !(expected->kind == Ty_record && actual->kind == Ty_nil))
            {

                EM_error(args->head->pos, "para type mismatch");
            }

            // append to para list

            tail->tail = Tr_ExpList(var_entry.exp, NULL);

            tail = tail->tail;
        }

        // argument number should be correct

        if (expects != NULL || args != NULL)
        {

            EM_error(a->pos, "too many params in function %s", S_name(a->u.call.func));
        }

        // skip dummy head node

        list = list->tail;

        Tr_exp tr_exp = Tr_call(fun_entry->u.fun.label, list, curr_level, fun_entry->u.fun.level);

        return expTy(tr_exp, actual_ty(fun_entry->u.fun.result));
    }

    case A_opExp:
    {

        A_oper oper = a->u.op.oper;

        struct expty left = transExp(venv, tenv, a->u.op.left, curr_level, nearest_break);

        struct expty right = transExp(venv, tenv, a->u.op.right, curr_level, nearest_break);

        // only integer allowed for arithmetic operation

        if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp)
        {

            if (actual_ty(left.ty)->kind != Ty_int)
            {

                EM_error(a->u.op.left->pos, "integer required");

                return expTy(NULL, Ty_Int());
            }

            if (actual_ty(right.ty)->kind != Ty_int)
            {

                EM_error(a->u.op.right->pos, "integer required");

                return expTy(NULL, Ty_Int());
            }

            Tr_exp tr_exp = Tr_arithmetic(oper, left.exp, right.exp);

            return expTy(tr_exp, Ty_Int());
        }
        else
        { // other ops just need the same operants' type

            Ty_ty left_ty = actual_ty(left.ty);

            Ty_ty right_ty = actual_ty(right.ty);

            if (left_ty != right_ty && !(left_ty->kind == Ty_record && right_ty->kind == Ty_nil))
            {

                EM_error(a->u.op.left->pos, "same type required");
            }

            Tr_exp tr_exp;

            if (left_ty == Ty_String())
            {

                E_enventry fun_entry = S_look(venv, S_Symbol("stringcmp"));

                Tr_expList args = Tr_ExpList(left.exp, Tr_ExpList(right.exp, NULL));

                Tr_exp cmp_call = Tr_call(fun_entry->u.fun.label, args, curr_level, fun_entry->u.fun.level);

                tr_exp = Tr_strcmp(oper, cmp_call);
            }
            else
            {

                tr_exp = Tr_condition(oper, left.exp, right.exp);
            }

            return expTy(tr_exp, Ty_Int());
        }
    }

    case A_recordExp:
    {

        // check type declaration

        Ty_ty record_type = actual_ty(S_look(tenv, a->u.record.typ));

        if (!record_type)
        {

            EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));

            return expTy(NULL, Ty_Int());
        }

        if (record_type->kind != Ty_record)
        {

            EM_error(a->pos, "not record type %s", S_name(a->u.record.typ));

            return expTy(NULL, record_type);
        }

        // check field names and types, and build field list

        Ty_fieldList expects = record_type->u.record;

        A_efieldList actuals = a->u.record.fields;

        Tr_expList list = Tr_ExpList(NULL, NULL); // dummy head node

        Tr_expList tail = list;

        for (; actuals && expects; actuals = actuals->tail, expects = expects->tail)
        {

            // field name

            if (expects->head->name != actuals->head->name)
            {

                EM_error(a->pos, "expected %s but get %s", S_name(expects->head->name),

                         S_name(actuals->head->name));
            }

            // field type

            struct expty var_entry = transExp(venv, tenv, actuals->head->exp, curr_level, nearest_break);

            // same type or record with nil

            Ty_ty expected = actual_ty(expects->head->ty);

            Ty_ty actual = actual_ty(var_entry.ty);

            if (expected != actual && !(expected->kind == Ty_record && actual->kind == Ty_nil))
            {

                EM_error(a->pos, "type not match");
            }

            // append to field list

            tail->tail = Tr_ExpList(var_entry.exp, NULL);

            tail = tail->tail;
        }

        // field number should match, too

        if (expects != NULL || actuals != NULL)
        {

            EM_error(a->pos, "field number of %s does not match", S_name(a->u.record.typ));
        }

        // skip dummy head node

        list = list->tail;

        Tr_exp tr_exp = Tr_recordCreation(list);

        return expTy(tr_exp, record_type);
    }

    case A_seqExp:
    {

        A_expList exps = a->u.seq;

        Tr_exp result_exp = Tr_nil();

        Ty_ty result_type = Ty_Void();

        while (exps)
        {

            struct expty exp = transExp(venv, tenv, exps->head, curr_level, nearest_break);

            result_exp = Tr_eseq(result_exp, exp.exp);

            result_type = exp.ty;

            exps = exps->tail;
        }

        return expTy(result_exp, result_type);
    }

    case A_assignExp:
    {

        struct expty lvalue = transVar(venv, tenv, a->u.assign.var, curr_level, nearest_break);

        struct expty exp = transExp(venv, tenv, a->u.assign.exp, curr_level, nearest_break);

        // not allow to assign to loop variable

        hasLoopVar(venv, a->u.assign.var);

        // same type or record with nil

        Ty_ty expected = actual_ty(lvalue.ty);

        Ty_ty actual = actual_ty(exp.ty);

        if (expected != actual && !(expected->kind == Ty_record && actual->kind == Ty_nil))
        {

            EM_error(a->pos, "unmatched assign exp");
        }

        Tr_exp tr_exp = Tr_assign(lvalue.exp, exp.exp);

        return expTy(tr_exp, Ty_Void());
    }

    case A_ifExp:
    {

        // check test type

        struct expty test = transExp(venv, tenv, a->u.iff.test, curr_level, nearest_break);

        if (actual_ty(test.ty)->kind != Ty_int)
        {

            EM_error(a->u.iff.test->pos, "type of test expression shoulf be int");
        }

        struct expty then = transExp(venv, tenv, a->u.iff.then, curr_level, nearest_break);

        Tr_exp tr_exp;

        if (a->u.iff.elsee)
        { // if-then-else

            struct expty elsee = transExp(venv, tenv, a->u.iff.elsee, curr_level, nearest_break);

            Ty_ty then_type = actual_ty(then.ty);

            Ty_ty else_type = actual_ty(elsee.ty);

            if (then_type != else_type && !(then_type->kind == Ty_record && else_type->kind == Ty_nil))
            {

                EM_error(a->u.iff.then->pos, "then exp and else exp type mismatch");
            }

            tr_exp = Tr_ifthenelse(test.exp, then.exp, elsee.exp);
        }
        else
        { // just if-then

            if (actual_ty(then.ty) != Ty_Void())
            {

                EM_error(a->u.iff.then->pos, "if-then exp's body must produce no value");
            }

            tr_exp = Tr_ifthen(test.exp, then.exp);
        }

        return expTy(tr_exp, then.ty);
    }

    case A_whileExp:
    {

        // new break exit

        Temp_label new_break = Temp_newNamedLabel("break");

        // check type

        struct expty test = transExp(venv, tenv, a->u.whilee.test, curr_level, nearest_break);

        struct expty body = transExp(venv, tenv, a->u.whilee.body, curr_level, new_break);

        if (actual_ty(test.ty)->kind != Ty_int)
        {

            EM_error(a->u.whilee.test->pos, "type of test expression shoulf be int");
        }

        if (actual_ty(body.ty)->kind != Ty_void)
        {

            EM_error(a->u.whilee.body->pos, "while body must produce no value");
        }

        Tr_exp tr_exp = Tr_while(test.exp, body.exp, new_break);

        return expTy(tr_exp, Ty_Void());
    }

    case A_forExp:
    {

        // new break exit

        Temp_label new_break = Temp_newNamedLabel("break");

        // check loop bounds

        struct expty start = transExp(venv, tenv, a->u.forr.lo, curr_level, nearest_break);

        struct expty end = transExp(venv, tenv, a->u.forr.hi, curr_level, nearest_break);

        if (actual_ty(start.ty)->kind != Ty_int)
        {

            EM_error(a->u.forr.lo->pos, "for exp's range type is not integer");
        }

        if (actual_ty(end.ty)->kind != Ty_int)
        {

            EM_error(a->u.forr.hi->pos, "for exp's range type is not integer");
        }

        // declare loop variable

        S_beginScope(venv);

        Tr_access access = Tr_allocLocal(curr_level, a->u.forr.escape, NULL);

        E_enventry enventry = E_ROVarEntry(access, Ty_Int());

        S_enter(venv, a->u.forr.var, enventry);

        // translate loop body (twice for if-part and while-part each)

        struct expty body_if = transExp(venv, tenv, a->u.forr.body, curr_level, new_break);

        struct expty body_while = transExp(venv, tenv, a->u.forr.body, curr_level, new_break);

        S_endScope(venv);

        if (actual_ty(body_if.ty)->kind != Ty_void)
        {

            EM_error(a->u.forr.body->pos, "type of body expression should be void");
        }

        Tr_exp tr_exp = Tr_for(access, start.exp, end.exp, body_if.exp, body_while.exp, new_break);

        return expTy(tr_exp, Ty_Void());
    }

    case A_breakExp:

        return expTy(Tr_break(nearest_break), Ty_Void());

    case A_letExp:
    {

        S_beginScope(venv);

        S_beginScope(tenv);

        A_decList d;

        for (d = a->u.let.decs; d; d = d->tail)
        {

            transDec(venv, tenv, d->head, curr_level, nearest_break);
        }

        struct expty exp = transExp(venv, tenv, a->u.let.body, curr_level, nearest_break);

        S_endScope(tenv);

        S_endScope(venv);

        return exp;
    }

    case A_arrayExp:
    {

        // check array type

        Ty_ty type = actual_ty(S_look(tenv, a->u.array.typ));

        if (!type)
        {

            EM_error(a->pos, "undefined type %s", S_name(a->u.array.typ));

            return expTy(NULL, Ty_Int());
        }

        if (type->kind != Ty_array)
        {

            EM_error(a->pos, "not array type %s", S_name(a->u.record.typ));

            return expTy(NULL, type);
        }

        // check array size

        struct expty size = transExp(venv, tenv, a->u.array.size, curr_level, nearest_break);

        if (actual_ty(size.ty)->kind != Ty_int)
        {

            EM_error(a->u.array.size->pos, "type of size expression should be int");
        }

        // check init data, same type or record with nil

        struct expty init = transExp(venv, tenv, a->u.array.init, curr_level, nearest_break);

        Ty_ty expected = actual_ty(type->u.array);

        Ty_ty actual = actual_ty(init.ty);

        if (expected != actual && !(expected->kind == Ty_record && actual->kind == Ty_nil))
        {

            EM_error(a->u.array.init->pos, "type mismatch");
        }

        Tr_exp tr_exp = Tr_arrayCreation(size.exp, init.exp);

        return expTy(tr_exp, type);
    }
    }

    assert(0); /* should have returned from some clause of the switch */
}

void transDec(S_table venv, S_table tenv, A_dec d, Tr_level level, Temp_label nearest_break)
{

    // EM_error(d->pos, "made it here, dec");

    switch (d->kind)
    {

    case A_varDec:
    {

        // translate init data

        struct expty init = transExp(venv, tenv, d->u.var.init, level, nearest_break);

        // check variable type

        if (d->u.var.typ)
        { // if type is specified

            Ty_ty type = S_look(tenv, d->u.var.typ);

            if (!type)
            {

                EM_error(d->u.var.init->pos, "type not exist %s", S_name(d->u.var.typ));
            }

            Ty_ty expected = actual_ty(type);

            Ty_ty actual = actual_ty(init.ty);

            if (expected != actual && !(expected->kind == Ty_record && actual->kind == Ty_nil))
            {

                EM_error(d->u.var.init->pos, "type mismatch");
            }

            S_enter(venv, d->u.var.var, E_VarEntry(Tr_allocLocal(level, d->u.var.escape, init.exp), type));
        }
        else
        { // otherwise, infer type from init data

            if (actual_ty(init.ty)->kind == Ty_nil)
            { // can not be nil then

                EM_error(d->u.var.init->pos, "init should not be nil without type specified");

                break;
            }

            S_enter(venv, d->u.var.var, E_VarEntry(Tr_allocLocal(level, d->u.var.escape, init.exp), init.ty));
        }

        break;
    }

    case A_typeDec:
    {

        // check if there are same type names in a batch declarations

        TAB_table name_table = TAB_empty();

        A_nametyList types = d->u.type;

        while (types)
        {

            if (TAB_look(name_table, types->head->name) != NULL)
            {

                EM_error(d->pos, "two types with the same name %s in batch declaration\n",

                         S_name(types->head->name));
            }

            TAB_enter(name_table, types->head->name, (void *)1);

            types = types->tail;
        }

        // put type declarations to enviroment

        types = d->u.type;

        while (types)
        {

            S_enter(tenv, types->head->name, Ty_Name(types->head->name, NULL));

            types = types->tail;
        }

        // resolve references

        types = d->u.type;

        while (types)
        {

            Ty_ty type = S_look(tenv, types->head->name);

            type->u.name.ty = transTy(tenv, types->head->ty);

            types = types->tail;
        }

        // cycle detection

        types = d->u.type;

        while (types)
        {

            Ty_ty init = S_look(tenv, types->head->name);

            Ty_ty type = init;

            while ((type = type->u.name.ty)->kind == Ty_name)
            {

                if (type == init)
                {

                    EM_error(d->pos, "illegal type cycle");

                    init->u.name.ty = Ty_Int();

                    break;
                }
            }

            types = types->tail;
        }

        break;
    }

    case A_functionDec:
    {

        // check if there are same type names in a batch declarations

        TAB_table name_table = TAB_empty();

        A_fundecList funcs = d->u.function;

        while (funcs)
        {

            if (TAB_look(name_table, funcs->head->name) != NULL)
            {

                EM_error(d->pos, "two functions with the same name %s in batch declaration\n",

                         S_name(funcs->head->name));
            }

            TAB_enter(name_table, funcs->head->name, (void *)1);

            funcs = funcs->tail;
        }

        // put function declarations to environment

        funcs = d->u.function;

        while (funcs)
        {

            // get result type

            Ty_ty result_ty = Ty_Void();

            if (funcs->head->result)
            {

                result_ty = S_look(tenv, funcs->head->result);

                if (!result_ty)
                {

                    EM_error(funcs->head->pos, "undefined result type %s", S_name(funcs->head->result));

                    result_ty = Ty_Void();
                }
            }

            // create new level

            Temp_label func_label = Temp_newNamedLabel(S_name(funcs->head->name));

            U_boolList formal_escapes = makeFormalEscList(funcs->head->params);

            Tr_level new_level = Tr_newLevel(level, func_label, formal_escapes);

            // add declaration

            Ty_tyList formal_tys = makeFormalTyList(tenv, funcs->head->params);

            E_enventry entry = E_FunEntry(new_level, func_label, formal_tys, result_ty);

            S_enter(venv, funcs->head->name, entry);

            funcs = funcs->tail;
        }

        // then check function bodies

        funcs = d->u.function;

        while (funcs)
        {

            // get formal accesses

            E_enventry fun_entry = S_look(venv, funcs->head->name);

            Tr_accessList formal_accesses = Tr_formals(fun_entry->u.fun.level);

            // add parameter declaration

            A_fieldList fields = funcs->head->params;

            S_beginScope(venv);

            while (fields)
            {

                Ty_ty type = S_look(tenv, fields->head->typ);

                E_enventry var_entry = E_VarEntry(formal_accesses->head, type);

                S_enter(venv, fields->head->name, var_entry);

                fields = fields->tail;

                formal_accesses = formal_accesses->tail;
            }

            // translate function body

            struct expty body_exp = transExp(venv, tenv, funcs->head->body, fun_entry->u.fun.level, nearest_break);

            S_endScope(venv);

            Ty_ty expected = actual_ty(fun_entry->u.fun.result);

            Ty_ty actual = actual_ty(body_exp.ty);

            if (expected->kind == Ty_void && actual->kind != Ty_void)
            {

                EM_error(funcs->head->pos, "procedure returns value");
            }
            else if (expected != actual && !(expected->kind == Ty_record && actual->kind == Ty_nil))
            {

                EM_error(funcs->head->pos, "procedure returns unexpected type");
            }

            // end current program fragment

            Tr_procEntryExit(fun_entry->u.fun.level, body_exp.exp, formal_accesses);

            funcs = funcs->tail;
        }

        break;
    }
    }
}

Ty_ty transTy(S_table tenv, A_ty a)
{

    switch (a->kind)
    {

    case A_nameTy:
    {

        Ty_ty type = S_look(tenv, a->u.name);

        if (!type)
        {

            EM_error(a->pos, "undefined type %s", S_name(a->u.name));

            return Ty_Int();
        }

        return Ty_Name(a->u.name, type);
    }

    case A_recordTy:

        return Ty_Record(makeFieldList(tenv, a->u.record));

    case A_arrayTy:
    {

        Ty_ty type = S_look(tenv, a->u.array);

        if (!type)
        {

            EM_error(a->pos, "undefined type %s", S_name(a->u.array));

            return Ty_Int();
        }

        return Ty_Array(type);
    }
    }

    return NULL;
}

F_fragList SEM_transProg(A_exp exp)
{

    // create main level

    Temp_label func_label = Temp_namedlabel("tigermain");

    Tr_level main_level = Tr_newLevel(Tr_outermost(), func_label, NULL);

    E_enventry fun_entry = E_FunEntry(main_level, func_label, NULL, Ty_Void()); // result type is void

    // translate main body

    struct expty body_exp = transExp(E_base_venv(), E_base_tenv(), exp, main_level, NULL);

    // end main fragment

    Tr_procEntryExit(fun_entry->u.fun.level, body_exp.exp, NULL);

    return Tr_getResult();
}
