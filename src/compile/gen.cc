/**
 *
 */

#include "gen.h"

#include "../core/ast.h"
#include "../core/error.h"
#include "../parse/analysis.h"

using namespace std;
using namespace llvm;
using namespace def::core;
using namespace def::compile;
using namespace def::parse;



/**
 * ��ȡ����
 */
Value* Gen::getValue(const string & name)
{
    auto rel = values.find(name);
    if (rel != values.end()) {
        return rel->second;
        // FATAL("codegen: Cannot find variable '"+name+"' !")
    }
    return nullptr;
}


/**
 * ���ñ���
 * ���ؾɱ���
 */
Value* Gen::putValue(const string & name, Value* v)
{
    Value* old = getValue(name);
    values[name] = v;
    return old;
}


/**
 * �� AST ����ɿ�����Ϊ��������ʹ��
 */
Value* Gen::varyPointer(AST* ast)
{
    // AST* ast = (AST*)p;
    Value *v = ast->codegen(*this);

    Value *val = varyPointer(v);

    // ����Ǳ��������ظ�ֵ
    if (auto *vari = dynamic_cast<ASTVariable*>(ast)) {
        putValue(vari->name, val);
    }

    return val;
}
Value* Gen::varyPointer(Value* val)
{
    // Value *val = (Value *)v;
    llvm::Type* ty = val->getType();
    // �ṹ���͵�ֵ�����·����ڴ�

    if (! isa<PointerType>(ty) ){
        Value *aoc = builder.CreateAlloca(ty);
        builder.CreateStore(val, aoc); // ����
        val = aoc;
    }
    

    return val;

}




/**
 * ��������
 */
Value* Gen::createLoad(AST* p)
{
    AST* ast = (AST*)p;
    // def::parse::Type *ty = Analysis::getType(ast);
    Value *val = ast->codegen(*this);

    return createLoad(val);

}
Value* Gen::createLoad(Value* val)
{
    // Value *val = (Value*)v;
    llvm::Type *ty = val->getType();

    // Store ����
    if (isa<StoreInst>(val)) {
        auto * stis = (StoreInst*)val;
        return stis->getValueOperand();
    }

    // ָ��ڵ�
    if( isa<PointerType>(ty) ) {
        // ��Ҫ�ӵ�ַ��������
        return builder.CreateLoad(val);
    }
    
    return val;

}


/**
 * ��������
 */
Function* Gen::createFunction(AST* p)
{
    ASTFunctionCall* call = (ASTFunctionCall*)p;
    string fname = call->fndef->ftype->name;
    string idname = call->fndef->getIdentify();
    // ���һ���
    Function *func = module.getFunction(idname);
    if(func){
        // cout << "module.getFunction " << fname << endl;
        return func; // ���ػ���
    }
    
    // ����ɵı���
    auto old_values = values;
    values.clear();

    // ����ջ
    vector<string> cpt_name;
    vector<def::core::Type*> cpt_type;
    
    // ��ʵ���������Ǿ�̬��
    if (call->fndef->belong && ! call->fndef->is_static_member) {
        cpt_name.push_back(DEF_MEMFUNC_ISTC_PARAM_NAME);
        cpt_type.push_back(call->fndef->belong->type);
    }

    // ����ı���
    for (auto &p : call->fndef->cptvar) { // ���ò�������
        cpt_name.push_back(p.first);
        cpt_type.push_back(p.second);
    }

    // ������������
    FunctionType *fty = (FunctionType*)fixType( 
        call->fndef->ftype, 
        & cpt_type
    );
    
    // ��������
    func = Function::Create(fty, Function::ExternalLinkage, idname, &module);

    // ���ú�������
    int idx = 0;
    int idx2 = 0;
    int cptnum = cpt_name.size();
    // cout << "values  = = = = "+call->fndef->ftype->name+" = = = = : " << endl;
    for (auto &Arg : func->args()) { // ���ò���
        string name;
        if (idx<cptnum) {
            // �����������
            name = cpt_name[idx];
        } else {
            // ʵ�ʲ���
            name = call->fndef->ftype->tabs[idx2];
            idx2++;
        }
        Arg.setName(name);
        values[name] = &Arg;
        idx++;
        // cout << "values[name]: " << name << endl;
    }
    
    // ����������
    if (!call->fndef->body) {
        FATAL("codegen: Cannot find function '"+fname+"' body !")
    }

    // ����ɵĲ����
    BasicBlock *old_block = builder.GetInsertBlock();
    BasicBlock *new_block = BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(new_block);
    Value* last(nullptr);
    AST* tail(nullptr);
    for (auto &li : call->fndef->body->childs) {
        if (Value* v = li->codegen(*this)) {
            last = v;
            tail = li;
            //if (isa<ReturnInst>(last)) {
                // break;
            //}
        }
    }

    // �Ƿ�Ϊ���캯��
    if (! last || call->fndef->is_construct) {
        builder.CreateRetVoid();
    }else if ( ! isa<ReturnInst>(last)) {
        // ���������һ���Զ���Ϊ����ֵ������ֵ���� Load
        // cout << "! isa<ReturnInst>(last)" << endl;
        // builder.CreateRet(last);
        builder.CreateRet(
           createLoad(last)
        );
    }else {
    }

    // ����� ��λ
    builder.SetInsertPoint(old_block);
    
    // ��λ����ջ
    values = old_values;

    return func;
}



/**
 * ����ת��
 */
llvm::Type* Gen::fixType(def::core::Type* ty, vector<def::core::Type*>* append)
{

#define ISTY(T) def::core::Type##T* obj = dynamic_cast<def::core::Type##T*>(ty)

    // ԭ������
    if (ISTY(Nil)) {
        return builder.getVoidTy();
    } else if (ISTY(Bool)) {
        return builder.getInt1Ty();
    } else if (ISTY(Int)) {
        return builder.getInt32Ty();
    } else if (ISTY(Float)) {
        return builder.getFloatTy();
    } else if (ISTY(Char)) {
        return builder.getInt32Ty();
    } else if (ISTY(String)) {
        // return PointerType::get(builder.getInt8Ty(), 0);
        return builder.getInt8PtrTy();
    }
    
    // ��������
    if (ISTY(Quote)) {
        return builder.getInt32Ty();
    }

    // ��������
    if (ISTY(Function)) {
        std::vector<llvm::Type*> ptys;
        // �����׷�ӵ�
        if (append) {
            for (auto &p : *append) {
                auto *pty = fixType(p);
                // ׷�ӵ�����ʼ����ָ�뷽ʽ����
                if ( ! isa<PointerType>(pty)) {
                    pty = PointerType::get(pty, 0);
                }
                ptys.push_back( pty ); // ��������
            }
        }
        for (auto &p : obj->types) {
            auto *pty = fixType(p);
            // ����ָ�봫�ݣ���������ֵ����
            if (dynamic_cast<TypeStruct*>(p)) {
                if (!isa<PointerType>(pty)) {
                    pty = PointerType::get(pty, 0);
                }
            }
            ptys.push_back( pty ); // ��������
        }
        // ���ؽṹ����ֵ��Ҳ���ص�ַ
        auto rty = fixType(obj->ret);
        // rty = rty ? rty : builder.getVoidTy();
        /*
        if (dynamic_cast<TypeClass*>(obj->ret)) {
            rty = PointerType::get(rty, 0);
        }*/
        return FunctionType::get(rty, ptys, false);
    }

    // Struct ������
    if (ISTY(Struct)) {

        // �ÿ�ͷ�ĵ�ű�ʶ����
        string name = obj->getIdentify();

        // �����Ƿ��Ѿ�����
        auto* scty = module.getTypeByName(name);
        if(scty){
            return scty;
        }

        // ����������
        scty = StructType::create(context, name);
        // ������
        std::vector<llvm::Type*> ptys;
        for (auto &p : obj->types) {
            ptys.push_back( fixType(p) ); // ��������
        }
        scty->setBody(ptys);
        // auto* llty = StructType::get(context, ptys, false);
        // scty->setName(obj->name);

        return scty;

        //return llty;
    }
    


#undef ISTY

}


// �ڽ���������ת��
llvm::Type* Gen::fixBuiltinFunctionType(def::core::TypeFunction* fty)
{
    std::vector<llvm::Type*> ptys;
    for (auto &p : fty->types) {
        auto *pty = fixType(p);
        // if (1 || dynamic_cast<TypeStruct*>(p)) {
        // if ( ! isa<PointerType>(pty)) {
            // ͨ��ָ�봫�ݽṹ���Ͳ���
            // pty = PointerType::get(pty, 0);
        // }
        ptys.push_back( pty ); // ��������
    }
    // ���ؽṹ����ֵ��Ҳ���ص�ַ
    auto rty = fixType(fty->ret);
    // rty = rty ? rty : builder.getVoidTy();
    /*
    if (dynamic_cast<TypeClass*>(obj->ret)) {
        rty = PointerType::get(rty, 0);
    }*/
    return FunctionType::get(rty, ptys, false);
}