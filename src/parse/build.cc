/**
 * �﷨�����ռ�
 */


#include "build.h"
#include "../core/error.h"

#include "../util/path.h"
#include "../util/fs.h"
#include "../sys/debug.h"

#include "../core/splitfix.h"
#include "../core/ast.h"
#include "../core/builtin.h"

#include "../parse/filter.h"


using namespace std;
using namespace def::core;
using namespace def::util;
using namespace def::sys;
using namespace def::compile;
using namespace def::parse;


#define Word Tokenizer::Word 
#define State Tokenizer::State

#define ISWS(TS) word.state==State::TS
#define NOTWS(TS) !(ISWS(TS))
#define ISSIGN(S) ISWS(Sign)&&word.value==S
#define NOTSIGN(S) !(ISSIGN(S))
#define ISCHA(S) ISWS(Character)&&word.value==S

// ������
/*
#define CHECKSIGN(S,F)
    auto word = getWord(); 
    if(NOTSIGN(S)){
        FATAL(F)
    } 
    */

    
/**
 * ����
 */
Build::Build(Tokenizer * t)
    : Service(t)
{
}

    
/**
 * �����Ԥ���������ڵ�
 */
void Build::prepareBuild(AST* p)
{
    if (p) {
        prepare_builds.push_back(p);
    }
}
void Build::prepareBuild(const list<AST*> & asts)
{
    for (auto &li : asts) {
        prepareBuild(li);
    }
}


/**
 * ��������
 */
ASTGroup* Build::createAST()
{
    return buildGroup();
}

/**
 * ���� Group
 */
ASTGroup* Build::buildGroup()
{
    // �½�������
    ASTGroup* block = new ASTGroup();
    while (1) {
        AST* li = build();
        if (nullptr==li) {
            break;
        }
        block->add(li);
    }
    return block;
}

/**
 * �������ʽ
 * spread �Ƿ���Ҫ��� let ���Ű�
 */
AST* Build::build(bool spread)
{
    
    // ���Դ�ӡ
    DEBUG_WITH("prepare_words", \
        if(prepare_words.size()){ \
        cout << "prepare��"; \
        for (auto &p : prepare_words) { \
            cout << " " << p.value; \
        } \
        cout << "��" << endl; \
        } \
        )


    if (!prepare_builds.empty()) {
        auto rt = prepare_builds.front();
        prepare_builds.pop_front();
        return rt;
    }

    // ������չ��
    if (spread) {
        if (AST* res = buildOperatorBind()) {
            return res;
        }
    }

    Error::snapshot();

    Word word = getWord();
    
    if ( ISWS(End) ) {
        return nullptr; // �ʷ�ɨ�����
    }


    // ������
    if (ISSIGN(";")) {
        return build();
        
    // block
    } else if(ISSIGN("(")){
        ASTGroup* grp = buildGroup();
        word = getWord();
        if (NOTSIGN(")")) {
            FATAL("Group build not right end !")
        }
        if (grp->childs.size()==1) {
            AST* res = grp->childs[0];
            delete grp;
            return res;
        }
        return grp;

    // Number
    } else if (ISWS(Number)) {
        Type *nty = Type::get("Int");
        if (Tokenizer::isfloat(word.value)) {
            nty = Type::get("Float");
        }
        return new ASTConstant(nty, word.value);
        
    // Char
    } else if (ISWS(Char)) {
        return new ASTConstant(Type::get("Char"), word.value);
     
    // String
    } else if (ISWS(String)) {
        return new ASTConstant(Type::get("String"), word.value);
        
    // Character
    } else if ( ISWS(Character) ) {
        
        string chara = word.value;

        // ��ѯ��ʶ���Ƿ���
        Element* res = stack->find(chara, true);
        
        // �������� ��
        if (auto val = buildVaribale(res, chara)) {
            return val;
        }

        // ��� ��
        if (auto gr = dynamic_cast<ElementLet*>(res)) {
            return buildMacro(gr, chara);
        }
        
        // ���͹��� ��
        if (auto gr = dynamic_cast<ElementType*>(res)) {

            return buildConstruct(gr, chara);
        }

        // �������� ��
        if (auto gr = dynamic_cast<ElementGroup*>(res)) {
            
            auto fncall = _functionCall(chara ,stack);
            if (fncall) {
                return fncall;
            }
        }

        // ģ�庯�����ã�
        res = stack->find(DEF_PREFIX_TPF+chara, true);
        if (res) {
            return buildTemplateFuntion(chara, (ElementTemplateFuntion*)res);
        }

        // ���Ժ��Ķ��壿
        AST *core = buildCoreDefine(chara);
        if (core) {
            return core;
        }

        // Ϊ Nil �������泣��
        if (chara=="nil") {
            return new ASTConstant(Type::get("Nil"), word.value);
        }

        // Ϊ Bool �������泣��
        if (chara=="true"||chara=="false") {
            return new ASTConstant(Type::get("Bool"), word.value);
        }

        // ����
        Error::exit("Undefined identifier: " + chara);
    }
    

    Error::backspace(); // ��λ

    // ���浥��  �����ϲ㴦��
    // cacheWord(word);
    prepareWord(word);

    return nullptr; // �ʷ�ɨ�����


}

/**
 * ���������Ա����
 */
AST* Build::buildVaribale(Element* elm, const string & name)
{
    if (auto gr = dynamic_cast<ElementVariable*>(elm)) {
        return new ASTVariable( name, gr->type );
    }

    // �Ƿ�Ϊ���Ա����
    if(stack->tydef && stack->fndef){
        AST* instance = new ASTVariable( // �����
            DEF_MEMFUNC_ISTC_PARAM_NAME,
            stack->tydef->type
        );
        // �����౾��
        if (name==DEF_MEMFUNC_ISTC_PARAM_NAME) {
            stack->fndef->is_static_member = false; // �г�Ա����
            return instance;
        }
        int i = 0;
        for (auto &p : stack->tydef->type->tabs) {
            if (p==name) { // �ҵ�
                // �������Ա����
                AST* elmget = new ASTMemberVisit(instance, i);
                stack->fndef->is_static_member = false; // �г�Ա����
                return elmget;
            }
            i++;
        }
    }


    return nullptr;
}

/**
 * ���Ķ��崦��
 */
AST* Build::buildCoreDefine(const string & name)
{
    /** def ���Ķ����б� **/
#define T(N) if(#N==name) return build_##N();
    BUILTIN_DEFINES_LIST(T)
#undef T

    // δƥ��
    return nullptr;

}


/**
 * ����ģ�庯��
 */
AST* Build::buildTemplateFuntion(const string & name, ElementTemplateFuntion* tpf)
{
    // �º�������
    TypeFunction* functy = new TypeFunction(name);
    
    // �����º���
    auto *fndef = new ASTFunctionDefine(functy);

    // ����ģ�庯������
    auto *fncall = new ASTFunctionCall(fndef);

    // �����·���ջ
    Stack* old_stack = stack;
    Stack new_stack(stack);
    fndef->wrap = stack->fndef; // wrap
    new_stack.fndef = fndef; // ��ǰ����ĺ���

    // ʵ����ջ
    for (auto &pn : tpf->tpfdef->params) {
        AST* p = build();
        fncall->addparam(p);
        Type *ty = p->getType();
        functy->add(pn, ty); // ��������
        new_stack.put(pn, new ElementVariable(ty)); // ��ʵ��
    }

    // �滻��ջ֡
    stack = & new_stack;
    
    // Ԥ�����������
    prepareWord(tpf->tpfdef->bodywords);

    // ����������
    ASTGroup *body = createAST();
    auto word = getWord(); 
    if(NOTSIGN(")")){
        FATAL("Error format function body !)")
    } 

    // ����º���
    Type* tyret(nullptr);
    size_t bodylen = body->childs.size();
    if(bodylen>0){
        // ��ȡ���������һ��Ϊ��������
        tyret = body->childs.back()->getType();
    }

    // ��鷵��ֵ����һ����
    verifyFunctionReturnType(tyret);

    if ( ! functy->ret) {
        cout << "! functy->ret" << endl;
        functy->ret = Type::get("Nil");
    }

    // ��λ��ջ֡
    stack = old_stack;
    // ��ӡ�﷨����ջ
    DEBUG_WITH("als_stack", \
        cout << endl << endl << "==== Analysis stack ( template function "+name+" ) ===" << endl << endl; \
        new_stack.print(); \
        cout << endl << "====== end ======" << endl << endl; \
        )


    // ���ÿ��ܱ�ǵķ���ֵ
    functy->ret = tyret;
    fndef->ftype = functy;
    // ���� Body
    fndef->body = body;

    // ��λ��ջ֡
    stack = old_stack;

    // ����º���
    stack->addFunction(fndef);

    // ���غ�������
    return fncall;
}

/**
 * �������캯������
 */
AST* Build::buildConstruct(ElementType* ety, const string & name)
{
    // ���캯������
    string fname = DEF_PREFIX_CSTC + name;

    auto *tyclass = (TypeStruct*)ety->type;
    // �����Ƿ���ڹ��캯��
    auto fd = stack->find(fname);

    // �޹��캯��ʱ
    if (!fd) {
        auto *cst = new ASTTypeConstruct(tyclass);
        int plen = tyclass->len();
        while (plen--) { // ������͹������
            cst->add(build());
        }
        return cst;
    }

    // ���ù��캯��
    auto *val = new ASTTypeConstruct(tyclass, true); // �չ���

    // �����ڲ�ջ
    auto target_stack = type_member_stack[tyclass];

    // ��������
    ASTFunctionCall* fncall =
        _functionCall(fname ,target_stack, false);

    // δ�ҵ����ʵĹ��캯��
    if ( ! fncall) {
        FATAL("can't match class '"+tyclass->name
            +"' construct function '"
            +fname+"' !")
    }


    /*
    // ��ѯĿ�����Ա����
    // auto word = getWord();
    auto target_stack = type_member_stack[tyclass];
    // ��Ա�������ã������ϲ���
    auto filter = filterFunction(target_stack, funname, false);
    if (!filter.size()) {
        FATAL("can't find member function '"
            +funname+"' in class '"+tyclass->name+"'")
    }


    // string name = word.value;
    auto *fncall = new ASTFunctionCall(nullptr);
    // auto elms = grp->elms;
        
    ASTFunctionDefine* fndef(nullptr);

    // ��ʱ������ѯ����������
    auto *tmpfty = new TypeFunction(funname);


    // ������Ա��������

        // ����̰��ƥ��ģʽ
    while (true) {
        // ƥ�亯������һ���޲Σ�
        int match = filter.match(tmpfty);
        if (match==1 && filter.unique) {
            fndef = filter.unique; // �ҵ�Ψһƥ��
            string idname = fndef->ftype->getIdentify();
            break;
        }
        if (match==0) {
            //prepareBuild(cachebuilds);
            FATAL("No macth function '"+tyclass->name+"."+tmpfty->getIdentify()+"' !");
            // throw ""; // ��ƥ��
        }
        // �жϺ����Ƿ���ý���
        auto word = getWord();
        prepareWord(word); // �ϲ㴦������
        if (ISWS(End) || ISSIGN(")")) { // ��ǰ�ֶ����ý���
            if (filter.unique) {
                fndef = filter.unique; // �ҵ�Ψһƥ��
                break;
            }
        }
        // ��Ӳ���
        AST *exp = build();
        if (!exp) {
            if (filter.unique) {
                fndef = filter.unique; // �޸���������ҵ�ƥ��
                break;
            }
            FATAL("No macth function '"+tyclass->name+"."+tmpfty->getIdentify()+"' !");
        }
        //cachebuilds.push_back(exp);
        fncall->addparam(exp); // ��ʵ��
        auto *pty = exp->getType();
        // ���غ�����
        tmpfty->add("", pty);
    }

    fncall->fndef = fndef; // elmfn->fndef;
        

    delete tmpfty;



    // ��̬��Ա������֤
    //if (is_static && ! fndef->is_static_member) {
    //    FATAL("'"+fndef->ftype->name+"' is not a static member function !")
    //}
    */
    
    auto * mfc = new ASTMemberFunctionCall(val, fncall);

    return mfc;
}

/**
 * �����
 */
AST* Build::buildMacro(ElementLet* let, const string & name)
{
    // ��������
    map<string, list<Word>> pmstk;
    for (auto pm : let->params) {
        list<Word> pmws;
        auto word = getWord();
        if(ISSIGN("(")){ // �����������
            cacheWordSegment(pmws);
        } else {
            pmws.push_back(word);
        }
        pmstk[pm] = pmws;
    }

    // չ������
    list<Word> bodys;
    for (auto wd : let->bodywords) {
        auto fd = pmstk.find(wd.value);
        if (fd != pmstk.end()) {
            bodys.splice(bodys.end(), fd->second);
        } else {
            bodys.push_back(wd);
        }
    }

    // Ԥ��
    prepareWord(bodys);

    // ���¿�ʼ����
    return build();
}



/**
 * namespace �������ֿռ�
 */
AST* Build::build_namespace()
{

    
    Word word = getWord();

    if (NOTWS(Character)) {
        FATAL("namespace define need a legal name to belong !")
    }

    // ��ʷ����
    int oldlen = defspace.size();

    // ��������ռ�
    defspace += "";// (oldlen == 0 ? "" : "_") + word.value;

    word = getWord();
    if (NOTSIGN("(")) {
        FATAL("namespace define need a sign ( to belong !")
    }

    // ѭ���������
    AST *block = createAST();

    word = getWord();
    if (NOTSIGN(")")) {
        FATAL("namespace define need a sign ) to end !")
    }

    // ���ֿռ临λ
    defspace = defspace.substr(0, oldlen);

    return block;
}

/**
 * include ������չ���ļ�
 */
AST* Build::build_include()
{
    Error::snapshot();
    AST* path = build(false);
    auto *p = dynamic_cast<ASTConstant*>(path);
    if (!p || !p->type->is(Type::get("String"))) {
        // ������Ч��·��
        Error::exit("Behind 'include' is not a valid file path ! "\
            "(need a type<String> constant value)");
    }

    // �ӵ�ǰ�������ļ���λ����һ���ļ�
    string absfile = Path::join(tkz->file, p->value);
    // cout << "absfile:" << absfile << endl;
    if (!Fs::exist(absfile)) {
        Error::exit("Cannot find the file '"+p->value+"' at current and library path ! ");
    }
    
    Error::backspace(); // ����

    // include Ψһ��
    if (checkSetInclude(absfile)) {
        // ���Դ�ӡ
        DEBUG_COUT("repeat_include", "[repeat include] "+absfile)

        delete path;
        return new ASTGroup();
    }


    // ���𻺴�ĵ���
    list<Word> cache = prepare_words;
    prepare_words.clear();

    // �½����滻�ʷ�������
    auto *old_tkz = tkz;
    Tokenizer new_tkz(absfile, false);
    tkz = & new_tkz;

    // ִ���﷨����
    AST* tree = createAST();

    // ��λ�ʷ�������
    tkz = old_tkz;

    // ��λ����ĵ���
    prepare_words = cache;

    // ��λ���󱨸�
    Error::update(old_tkz);

    delete p;
    return tree;
}

/**
 * ��ʼ������
 */
AST* Build::build_var()
{
    Word word = getWord();

    if (NOTWS(Character)) {
        FATAL("var define need a legal name to belong !")
    }

    string name = word.value;

    // �����Ƿ����
    auto fd = stack->find(name);
    if (fd) {
        FATAL("Cannot repeat define var '"+name+"' !")
    }
    
    AST* value = build();

    auto vardef = new ASTVariableDefine( name, value );

    // ��ӱ�����ջ
    stack->put(name, new ElementVariable(value->getType()));

    return vardef;
}

/**
 * set ������ֵ
 */
AST* Build::build_set()
{
    Word word = getWord();

    if (NOTWS(Character)) {
        FATAL("var assignment need a legal name to belong !")
    }

    string name = word.value;

    // ���ұ����Ƿ����
    auto fd = stack->find(name);
    ElementVariable * ev = dynamic_cast<ElementVariable*>(fd);
    if ( !fd || !ev ) {
        // ���������ڣ������Ƿ�Ϊ��Ա����
        if (stack->tydef) {
            AST* instance = new ASTVariable( // �����
                DEF_MEMFUNC_ISTC_PARAM_NAME,
                stack->tydef->type
            );
            // ��ֵ�౾��
            if (name==DEF_MEMFUNC_ISTC_PARAM_NAME) {
                stack->fndef->is_static_member = false; // �г�Ա����
                return instance;
            }
            Type* elmty = stack->tydef->type->elmget(name);
            if (elmty) { // �ҵ���Ա
                AST* value = build();
                // ���ͼ��
                if ( ! value->getType()->is(elmty)) {
                    FATAL("member assign type not match !")
                }
                stack->fndef->is_static_member = false; // �г�Ա����
                // �������Ա��ֵ
                return new ASTMemberAssign(
                    instance,
                    stack->tydef->type->elmpos(name),
                    value
                );
            }
        }

        FATAL("var '" + name + "' does not exist can't assignment  !")
    }

    // ����
    Type* ty = ev->type;

    // ֵ
    AST* value = build();
    Type* vty = value->getType();

    // ���ͼ��
    if (!vty->is(ty)) {
        FATAL("can't assignment <"
            +vty->getIdentify()+"> to var "
            +name+"<"+ty->getIdentify()+">  !")
    }

    // ��ӱ�����ջ
    // stack->set(name, new ElementVariable(vty));

    return new ASTVariableAssign( name, value );

}

/**
 * type ��������
 */
AST* Build::build_type()
{
    if (stack->tydef) { // Ƕ�׶���
        FATAL("can't define type in type define !")
    }
    
    Word word = getWord();

    if (NOTWS(Character)) {
        FATAL("type define need a legal name to belong !")
    }

    string typeName = fixNamespace( word.value );

    // ��ѯ�����Ƿ���
    if (auto *fd = dynamic_cast<ElementType*>(stack->find(typeName))) {
        FATAL("can't repeat type '"+typeName+"' !")
    }

    word = getWord();
    if (NOTSIGN("(")) {
        FATAL("namespace define need a sign ( to belong !")
    }

    // �½�����
    TypeStruct* tyclass = new TypeStruct(typeName);
    if (stack->fndef) {
        tyclass->increment(); // �����������Ƽ�����������
    }

    // ����AST
    ASTTypeDefine* tydef = new ASTTypeDefine();
    tydef->type = tyclass;

    auto *elmty = new ElementType(tyclass);

    // ��ջ
    Stack *old_stk = stack;
    Stack *new_stk = new Stack(stack);
    // ��ӵ���ǰջ֡ ֧�ֺ�������
    new_stk->put(typeName, elmty);
    new_stk->tydef = tydef;
    new_stk->fndef = nullptr; // ���Ա�����������κΰ�������
    stack = new_stk;


#define TCADD(N) if(it) tyclass->add(N, it);

    while (1) { // ����Ԫ�ض���
        Type* it = nullptr;
        word = getWord();
        if (ISSIGN(")")) {
            TCADD("")
            break; // ������������
        }
        // �ຯ������
        AST* fndef(nullptr);
        if("fun"==word.value){
            fndef = build_fun();
        } else if ("dcl" == word.value) {
            fndef = build_dcl();
        }
        if (fndef) {
            continue; // ��Ա��������
        }
        // ���ͱ�ʾ
        Element* res = stack->find(word.value);
        if (ElementType* dco = dynamic_cast<ElementType*>(res)) {
            it = dco->type;
        } else {
            FATAL("Type declare format error: not find Type <"<<word.value<<"> !")
        }
        // ���ͱ��
        word = getWord();
        res = stack->find(word.value);
        if (ElementType* dco = dynamic_cast<ElementType*>(res)) {
            prepareWord(word); // Ԥ��
            TCADD("")
        } else if(ISWS(Character)) {
            TCADD(word.value)
        } else {
            FATAL("Type declare format error !")
        }
    }

#undef TCADD

    // ��鿽����Ա�����б�
    type_member_stack[tyclass] = new_stk;
    type_define[tyclass] = tydef;


    // ��ӵ�����ջ
    stack = old_stk;
    stack->put(typeName, elmty);

    // ����
    return tydef;
}

/**
 * dcl ��������
 */
AST* Build::build_dcl()
{
    Word word = getWord();
    if (NOTWS(Character)) {
        FATAL("function declare need a type name to belong !")
    }

    // ��������ֵ����
    Type *rty;
    Element* elmret = stack->find(word.value);
    if (auto *ety = dynamic_cast<ElementType*>(elmret)) {
        rty = ety->type;
    }
    else {
        FATAL("function declare need a type name to belong !")
    }

    // ��������
    word = getWord();
    if (NOTWS(Character)) {
        FATAL("function declare need a legal name !")
    }

    // �½���������
    string funcname = word.value;
    auto *functy = new TypeFunction(funcname, rty);

    // ����
    word = getWord();
    if (NOTSIGN("(")) {
        FATAL("function declare need a sign ( to belong !")
    }

    // ������������
    while (true) {
        word = getWord();
        if (ISSIGN(")")) {
            break; // ���������б����
        }
        Type *pty;  // ��������
        if (auto *ety = dynamic_cast<ElementType*>(stack->find(word.value))) {
            functy->add("", ety->type);
        } else {
            FATAL("function declare parameter format is not valid !")
        }
    }
    
    // �����Ƿ��Ѿ�����
    ASTFunctionDefine* fndef = stack->findFunction(functy);

    // �жϺ����Ƿ��Ѿ�����
    if (!fndef) {
        fndef = new ASTFunctionDefine(functy, nullptr);
        // ����º���
        stack->addFunction(fndef);
    }

    // ���غ�������
    return new ASTFuntionDeclare(fndef->ftype);
}

/**
 * fun ���庯��
 */
AST* Build::build_fun()
{
    auto word = getWord();
    if (NOTWS(Character)) {
        FATAL("function define need a type name to belong !")
    }

    // ��������ֵ����
    Type *rty(nullptr); // nullptr ʱ��Ҫ�ƶ�����
    
    // �Ƿ�Ϊ���͹��캯��
    if(stack->tydef
        && word.value==stack->tydef->type->name)
    {
        auto word = getWord();
        if(ISSIGN("(")){
            // ������������ͬ�����޷���ֵ��Ϊ���캯��
            status_construct = true;
        } else {
            prepareWord(word);
        }

    }
    
    // ��ͨ��������
    if (!status_construct) {
        Element* elmret = stack->find(word.value);
        if (auto *ety = dynamic_cast<ElementType*>(elmret)) {
            rty = ety->type;
        }
        else {
            prepareWord(word);
        }

        // ��������
        word = getWord();
        if (NOTWS(Character)) {
            FATAL("function define need a legal name !")
        }

        // ������֤
        auto word = getWord();
        if (NOTSIGN("(")) {
            FATAL("function  define need a sign ( to belong !")
        }
    }

    // �½���������
    string funcname = word.value;
    string cstc_funcname = funcname;
    if (status_construct) {
        cstc_funcname = DEF_PREFIX_CSTC + funcname;
    }
    TypeFunction *functy = new TypeFunction(cstc_funcname);
    TypeFunction *constructfuncty = new TypeFunction(cstc_funcname);


    // ������������
    while (true) {
        word = getWord();
        if (ISSIGN(")")) {
            break; // ���������б����
        }
        Type *pty;  // ��������
        string pnm; // ��������
        if (auto *ety = dynamic_cast<ElementType*>(
            stack->find(word.value))) {
            pty = ety->type;
        }
        else {
            FATAL("Parameter format is not valid !")
        }
        word = getWord();
        if (NOTWS(Character)) {
            FATAL("function parameter need a legal name !")
        }
        pnm = word.value;
        if (stack->tydef && pnm == DEF_MEMFUNC_ISTC_PARAM_NAME) {
            // ���Ա�������������Ƴ�ͻ
            FATAL("can't give a parameter name '" + pnm + "' in type member function !")
        }
        functy->add(pnm, pty);
        // ���캯����ջ
        if (status_construct) {
            constructfuncty->add(pnm, pty);
        }
    }

    // �����Ƿ��Ѿ�����
    ASTFunctionDefine* aldef = stack->findFunction(functy);
    ASTFunctionDefine *fndef;

    // �жϺ����Ƿ��Ѿ�����
    if (aldef){
        if (aldef->body) {
            // �������������� body �Ѷ���
            FATAL("function define repeat: " + functy->str());
        }
        fndef = aldef;
    } else {
        // �½���������
        fndef = new ASTFunctionDefine(functy);
    }

    // ���ÿ��ܱ�ǵķ���ֵ
    functy->ret = rty;
    fndef->ftype = functy;

    // �����·���ջ
    Stack* old_stack = stack;
    Stack new_stack(stack);
    new_stack.fndef = fndef; // ��ǰ����ĺ���
    // ��ǰ ��Ӻ��� ֧�ֵݹ�
    new_stack.addFunction(fndef);

    // �������廷��
    fndef->wrap = stack->fndef; // wrap
    fndef->belong = stack->tydef; // belong

    // ElementStack nsk; // ����ɱ���
    int i(0);
    for (auto &pty : functy->types) {
        string pn(functy->tabs[i]);
        new_stack.put(pn, new ElementVariable(pty)); // ��ʵ��
        i++;
    }
    // �滻��ջ֡
    stack = & new_stack;

    // �½�������
    ASTGroup *body;
    word = getWord();
    // �����
    if (ISSIGN("(")) {
        body = buildGroup();
        word = getWord();
        if (NOTSIGN(")")) {
            FATAL("Error format function body !")
        }
    // �����
    } else {
        body = new ASTGroup();
        body->add( build() );
    }

    
    /*
    // ���캯��������ʵ������ֵ
    if (status_construct) {
        status_construct = false;
        prepareWord(Word(State::Character,"this"));
        prepareWord(Word(State::Character,"ret"));
        body->add( build() );
        status_construct = true;
    }*/

    
    if ( ! status_construct) {
        // ����ֵ��֤���ƶ�
        size_t len = body->childs.size();
        Type *lastChildTy(nullptr);
        while (len--) {
            AST* li = body->childs[len];
            if (li->isValue()) {
                lastChildTy = li->getType();
                break;
            }
        }
        if ( ! lastChildTy) {
            lastChildTy = Type::get("Nil");
        }
        // ��鷵��ֵ����һ����
        verifyFunctionReturnType(lastChildTy);
    }

    if ( ! functy->ret) {
        // cout << "! functy->ret" << endl;
        functy->ret = Type::get("Nil");
    }

    // ��λ��ջ֡
    stack = old_stack;
    // ��ӡ�﷨����ջ
    DEBUG_WITH("als_stack", \
        cout << endl << endl << "==== Analysis stack ( function "+funcname+" ) ===" << endl << endl; \
        new_stack.print(); \
        cout << endl << "====== end ======" << endl << endl; \
        )


    // ���� Body
    fndef->body = body;

    // ���캯����ջ
    if (status_construct) {
        auto * main_stack = old_stack->parent;
        constructfuncty->ret = stack->tydef->type;
        auto *constrct = new ASTFunctionDefine(
            constructfuncty, body);
        constrct->is_construct = true;
        main_stack->addFunction(constrct);
        // 
        fndef->is_construct = true;
        fndef->is_static_member = false;
    } else {
        delete constructfuncty;
    }

    // �����Ѿ��������ˣ���� body
    if (aldef) {
        // �滻 ftype�������������
        // delete aldef->ftype;
        // aldef->ftype = functy;

    // ��������º���
    } else {
        // �Ƿ�Ϊ���캯��
        fndef->is_construct = status_construct;
        fndef->is_construct = status_construct;
        stack->addFunction(fndef);
    }

    // ��ӵ����Ա����
    if(stack->tydef){
        stack->tydef->members.push_back(fndef);
    }
    
    // ��λ
    status_construct = false;

    // ����ֵ
    return fndef;
}

/**
 * ret ����ֵ����
 */
AST* Build::build_ret()
{
    AST *ret = build();
    // ��֤����ֵ
    verifyFunctionReturnType( ret->getType() );
    return new ASTRet(ret);
}

/**
 * tpf ģ�庯���ƶ�
 */
AST* Build::build_tpf()
{
    Word word = getWord();

    if (NOTWS(Character)) {
        FATAL("template function define need a legal name to belong !")
    }

    string tpfName = word.value; // fixNamespace(word.value);
    if (stack->find(DEF_PREFIX_TPF + tpfName)) {
        FATAL("template function duplicate definition '"+tpfName+"' !");
    }
    
    word = getWord();
    if (NOTSIGN("(")) {
        FATAL("template function define need a sign ( to belong !")
    }

    // �½�����
    auto *tpfdef = new ASTTemplateFuntionDefine();
    tpfdef->name = tpfName;

    // �����β��б�
    while (1) {
        word = getWord();
        if (ISSIGN(")")) { // �β��б����
            break;
        }
        if (NOTWS(Character)) {
            FATAL("template function define parameter name is not valid !")
        }
        tpfdef->params.push_back( word.value );
    }

    word = getWord();
    if (NOTSIGN("(")) {
        FATAL("template function define body need a sign ( to belong !")
    }
    
    // ����������
    cacheWordSegment(tpfdef->bodywords); // �������Ŷ�����
    
    // ��ӵ�����ջ
    stack->put(DEF_PREFIX_TPF + tpfName, new ElementTemplateFuntion(tpfdef));

    return tpfdef;
}

/**
 * if ������֧
 */
AST* Build::build_if()
{

#define DO_FATAL FATAL("No match bool function Type<"+idn+"> when be used as a if condition")
    
    AST* cond = build();
    Type* cty = cond->getType();
    Type* boolty = Type::get("Bool");

    string idn = cty->getIdentify();

    bool isboolty = cty->is(boolty);
    if ( ! isboolty) { // �� Bool ���ͣ����� bool ����
        auto *bools = dynamic_cast<ElementGroup*>(stack->find("bool"));
        if (!bools) { 
            DO_FATAL
        }
        auto fd = bools->elms.find("bool" DEF_SPLITFIX_FUNCARGV + idn);
        if (fd == bools->elms.end()) {
            DO_FATAL
        }
        auto *ef = dynamic_cast<ElementFunction*>(fd->second);
        if (!ef || !ef->fndef->ftype->ret->is(boolty)) {
            DO_FATAL
        }
        // �Զ����� bool ��������
        auto fcall = new ASTFunctionCall(ef->fndef);
        fcall->addparam(cond);
        cond = fcall;
    }

    // �½� if �ڵ�
    ASTIf *astif = new ASTIf(cond);

    // if ���
    astif->pthen = build();

    // ��� else
    auto word = getWord();
    if (ISCHA("else")) {
        astif->pelse = build();
    } else {
        prepareWord(word); // ��λ
    }


    // ���� if �ڵ�
    return astif;
}

/**
 * while ѭ���ṹ
 */
AST* Build::build_while()
{
#define DO_FATAL FATAL("No match bool function Type<"+idn+"> when be used as a while flow !")
    
    AST* cond = build();
    Type* cty = cond->getType();
    Type* boolty = Type::get("Bool");

    string idn = cty->getIdentify();

    bool isboolty = cty->is(boolty);
    if ( ! isboolty) { // �� Bool ���ͣ����� bool ����
        auto *bools = dynamic_cast<ElementGroup*>(stack->find("bool"));
        if (!bools) { 
            DO_FATAL
        }
        auto fd = bools->elms.find("bool" DEF_SPLITFIX_FUNCARGV + idn);
        if (fd == bools->elms.end()) {
            DO_FATAL
        }
        auto *ef = dynamic_cast<ElementFunction*>(fd->second);
        if (!ef || !ef->fndef->ftype->ret->is(boolty)) {
            DO_FATAL
        }
        // �Զ����� bool ��������
        auto fcall = new ASTFunctionCall(ef->fndef);
        fcall->addparam(cond);
        cond = fcall;
    }

    // �½� while �ڵ�
    ASTWhile *astwhile = new ASTWhile(cond);

    // while ���
    astwhile->body = build();

    // ���� if �ڵ�
    return astwhile;

}

/**
 * let ���Ű�
 */
AST* Build::build_let()
{
    auto *let = new ElementLet();
    auto *relet = new ASTLet();

    auto word = getWord();
    
    // Ψһ����
    string idname("");
    bool sign = false;

    // �������
    if (ISWS(Character)) {
        idname = word.value;
        relet->head.push_back(idname);
        auto word = getWord();
        if (NOTSIGN("(")) {
            FATAL("let macro binding need a sign ( to belong !")
        }
        // ����
        while (true) {
            word = getWord();
            if (ISSIGN(")")) {
                break; // ��������
            }
            string str(word.value);
            let->params.push_back(str);
            relet->head.push_back(str);
        }

    // ��������
    } else if (ISSIGN("(")) {
        
        // ����
        while (true) {
            word = getWord();
            if (ISSIGN(")")) {
                break; // ��������
            }
            string str(word.value);
            relet->head.push_back(str);
            if (ISWS(Character)) {
                let->params.push_back(str);
                idname += DEF_SPLITFIX_OPRT_BIND;
            } else if (ISWS(Operator)) {
                idname += str;
                sign = true;
            } else {
                FATAL("let unsupported the symbol type '"+str+"' !")
            }
        }
        
        if (!sign || relet->head.size()<2
            || idname.substr(0,1)!=DEF_SPLITFIX_OPRT_BIND 
        ) { // �����ṩһ������ �Ҳ���Ϊǰ׺����
            FATAL("let operator binding format error !")
        }

    } 
   
    auto *fd = stack->find(idname);
    if (fd) { // �����ظ���
        FATAL("let can't repeat binding '"+idname+"' !")
    }
    
    word = getWord();
    if (NOTSIGN("(")) {
        FATAL("let binding need a sign ( to belong !")
    }

    // ����
    while (true) {
        word = getWord();
        if (ISSIGN(")")) {
            break; // �����
        }
        relet->body.push_back(word.value);
        let->bodywords.push_back(word);
    }

    // ���ջ����
    stack->put(idname, let);

    // ����
    return relet;
}

/**
 * member call ��Ա��������
 */
AST* Build::build_elmivk()
{
    TypeStruct* tyclass(nullptr);
    AST* val(nullptr);

    auto word = getWord();

     // ͨ���������Ƶ��þ�̬��Ա����
    bool is_static = false;
    if (auto *e = dynamic_cast<ElementType*>(
        stack->find(word.value))) {
        if (tyclass = dynamic_cast<TypeStruct*>(e->type)) {
            is_static = true;
        }
    }
    // ͨ�������Ƶ��þ�̬����
    if ( ! tyclass) {
        prepareWord(word);
        val = build();
        // varibale = dynamic_cast<ASTVariable*>(val);
        if( ! val){
            FATAL("elmivk must belong a struct value !")
        }
        // val->print();
        tyclass  = dynamic_cast<TypeStruct*>(val->getType());
    }

    if( ! tyclass){
        FATAL("elmivk must belong a struct value '"+word.value+"' !")
    }

    // ���ڲ�ջ
    auto target_stack = type_member_stack[tyclass];

    // ��Ա��������
    word = getWord();
    string fname = word.value;

    // ��������
    ASTFunctionCall* fncall =
        _functionCall(fname ,target_stack, false);

    if ( ! fncall) {
        FATAL("can't find member function '"
            +fname+"' in class '"+tyclass->name+"'")
    }


    // ��̬��Ա������֤
    if (is_static && ! fncall->fndef->is_static_member) {
        FATAL("'"+fncall->fndef->ftype->name+"' is not a static member function !")
    }

    auto * mfc = new ASTMemberFunctionCall(val, fncall);

    return mfc;

}

/**
 * member get ��Ա����
 */
AST* Build::build_elmget()
{
    auto *ins = new ASTMemberVisit();

    AST* sctval = build();
    auto* scty = dynamic_cast<TypeStruct*>(sctval->getType());
    if( ! scty ){
        FATAL("elmget must eat a Struct value !")
    }

    ins->instance = sctval;

    auto word = getWord();
    int pos;
    if (ISWS(Number)) {
        pos = Str::s2l(word.value);
    } else {
        pos = scty->elmpos(word.value);
    }

    // �������
    if (pos<0 || pos>scty->len()) {
        FATAL("class '"+scty->name+"' no element '" + word.value + "' !")
    }
    ins->index = pos;

    return ins;
}

/**
 * member set ��Ա��ֵ
 */
AST* Build::build_elmset()
{
    auto *ins = new ASTMemberAssign();

    AST* sctval = build();
    auto* scty = dynamic_cast<TypeStruct*>(sctval->getType());
    if( ! scty ){
        FATAL("elmget must eat a Struct value !")
    }

    ins->instance = sctval;

    auto word = getWord();
    int pos;
    if (ISWS(Number)) {
        pos = Str::s2l(word.value);
    } else {
        pos = scty->elmpos(word.value);
    }

    // �������
    if (pos<0 || pos>scty->len()) {
        FATAL("class '"+scty->name+"' no element '" + word.value + "' !")
    }
    ins->index = pos;
    
    AST* putv = build();
    Type* putty = putv->getType();
    // ���ͼ��
    Type* pvty = scty->types[ins->index];
    if (!putty->is(pvty)) {
        FATAL("can't member assign <"+pvty->str()+"> by <"+putty->str()+">' !")
    }

    ins->value = putv;

    return ins;
}

/**
 * member function �ⲿ����
 */
AST* Build::build_elmdef()
{
    auto word = getWord();
    // ��������ֵ����
    TypeStruct *ty(nullptr);
    if(auto *e=dynamic_cast<ElementType*>(stack->find(word.value))){
        if (auto *y = dynamic_cast<TypeStruct*>(e->type)) {
            ty = y;
        }
    } 
    // ����Ϊ����
    if (NOTWS(Character) || ! ty) {
        FATAL("member function external define need a type name to belong !")
    }

    // �滻����ջ
    Stack* old_stack = stack;
    stack = type_member_stack[ty];
    stack->tydef = type_define[ty];

    // ����
    AST* res = build();

    // ��λջ֡
    stack = old_stack;

    return new ASTExternalMemberFunctionDefine(ty, res);
}


/**
 * Ԥ���Ƿ���Ҫ�⿪���Ű�
 */
bool Build::forecastOperatorBind()
{
    bool spread = false; // �Ƿ���չ
    bool fail = false;
    list<Word> cache;
    while (true) {
        auto word = getWord();
        if (ISWS(End)) { // ����
            break; // ����
        }
        cache.push_back(word);
        if (ISSIGN("(")) { // �Ӽ�
            if (fail) break;
            cacheWordSegment(cache); // �������Ŷ�����
            fail = true;
            continue;
        }
        if (ISWS(Operator)) {
            spread = true; // ��Ҫ���
            break;
        }
        if (ISWS(Sign)) { // ��������
            break;
        }
        if (fail) {
            break; // ��
        }
        fail = true;
    }
    
    /*
    cout << "cache��";
    for (auto &p : cache) {
        cout << " " << p.value;
    }
    cout << "��" << endl;
    */

    // �ָ�
    prepareWord(cache);

    return spread;
}


/**
 * let �������󶨰�չ��
 */
AST* Build::buildOperatorBind()
{
    //��չ������
    if (! forecastOperatorBind()) {
        return nullptr; 
    }

    // չ����
    auto words = spreadOperatorBind();
    prepareWord(words); // չ����չ�������

    // ���ٴμ���Ƿ���չ
    return build(false);
}



/**
 * let �������󶨰�չ��
 */
list<Word> Build::spreadOperatorBind(list<Word>*pwds)
{
    string idname("");
    vector<list<Word>> params;

    if (pwds) { // ������
        idname = DEF_SPLITFIX_OPRT_BIND;
        params.push_back(*pwds);
    }

    ElementLet* let(nullptr);
    filterLet* filter(nullptr);
    bool end = false;
    list<Word> cache;
    while (true) {
        cache.clear();
        auto word = getWord();
        if (ISWS(End)) { // �ı�����
            end = true;
        }
        cache.push_back(word);
        if (ISSIGN("(")) { // �Ӽ�
            cacheWordSegment(cache); // �������Ŷ�����
            if (cache.empty()) {
                FATAL("Operator binding priority error !")
            }
            params.push_back(cache);
            idname += DEF_SPLITFIX_OPRT_BIND;
        }else if (ISWS(Operator)) { // ������
            idname += word.value;
        } else {
            idname += DEF_SPLITFIX_OPRT_BIND;
            list<Word> pms;
            pms.push_back(word);
            params.push_back(pms);
        }
        if ( ! filter) { // ��ʼ��ɸѡ��
            filter = new filterLet(stack, idname);
        }
        // ɸѡƥ��
        int match = filter->match(idname);
        if (match==1 && filter->unique) { // ̰��ƥ�����
            let = filter->unique;
            break; // �ҵ�Ψһƥ��
        }
        if(end||match==0){
            if (let) {
                prepareWord(cache); // ��λ��������
                break; // ������һ����ȫƥ��
            }
            FATAL("can't find the binding '"+idname+"' !")
        }
        // ��¼��һ����Ψһƥ��
        let = filter->unique;
    }

    delete filter;

    // ��������
    map<string, list<Word>> pmstk;
    int i = 0;
    for (auto &p : let->params) {
        pmstk[p] = params[i];
        i++;
    }

    // չ����
    list<Word> results;
    for (auto &word : let->bodywords) {
        if (ISWS(Character) && pmstk.find(word.value) != pmstk.end()) {
            // �⿪����
            auto wd = pmstk[word.value];
            for(auto &w : wd){
                results.push_back(w);
            }
        }else {
            results.push_back(word);
        }
    } 

    // ���Դ�ӡ
    DEBUG_WITH("binding_spread", \
        if(results.size()){ \
        cout << "binding spread "+idname+"��"; \
        for (auto &p : results) { \
            cout << " " << p.value; \
        } \
        cout << "��" << endl; \
        } \
    )

    // �ж���һ���Ƿ�Ϊ���Ű�
    auto word = getWord();
    prepareWord(word);
    if (ISWS(Operator)){
        filterLet filter(stack, DEF_SPLITFIX_OPRT_BIND + word.value);
        if (filter.size()>0) { // ��ѯƥ��
            return spreadOperatorBind(&results);
        }
    }


    // ���Ű󶨽���
    return results;
}



/**
 * ���浥�ʶΣ����������ڲ��������ݣ�
 */
void Build::cacheWordSegment(list<Word>& cache)
{
    int down = 1;
    while (true) {
        auto word = getWord();
        if (ISWS(End)) { // ����
            return;
        }
        cache.push_back(word);
        if (ISSIGN("(")) down++;
        if (ISSIGN(")")) down--;
        if (0 == down) break;
    }
}




/********************************************************/



/**
 * ������������
 */
TypeFunction* Build::_functionType(bool declare)
{
    return nullptr;
}

/**
 * ������������
 */
ASTFunctionCall* Build::_functionCall(const string & fname, Stack* stack, bool up)
{
    auto *fncall = new ASTFunctionCall(nullptr);
    ASTFunctionDefine *fndef(nullptr);

    // �ڵ㻺��
    list<AST*> cachebuilds;

    // ��ʱ������ѯ����������
    auto *tmpfty = new TypeFunction(fname);
    filterFunction filter = filterFunction(stack, fname, up);
    if (!filter.size()) {
        return nullptr; // �޺���
    }

    // ����̰��ƥ��ģʽ
    while (true) {
        // ƥ�亯������һ���޲Σ�
        int match = filter.match(tmpfty);
        if (match==1 && filter.unique) {
            fndef = filter.unique; // �ҵ�Ψһƥ��
            string idname = fndef->ftype->getIdentify();
            break;
        }
        // ��ƥ��
        if (match==0) {
            prepareBuild(cachebuilds); // ��λ�����Ľڵ�
            delete tmpfty;
            delete fncall;
            return nullptr;
        }
        // �жϺ����Ƿ���ý���
        auto word = getWord();
        prepareWord(word); // �ϲ㴦������
        if (ISWS(End) || ISSIGN(")")) { // ��ǰ�ֶ����ý���
            if (filter.unique) {
                fndef = filter.unique; // �ҵ�Ψһƥ��
                break;
            }
        }
        // ��Ӳ���
        AST *exp = build();
        if (!exp) {
            if (filter.unique) {
                fndef = filter.unique; // �޸���������ҵ�ƥ��
                break;
            }
        }
        cachebuilds.push_back(exp);
        fncall->addparam(exp); // ��ʵ��
        auto *pty = exp->getType();
        // ���غ�����
        tmpfty->add("", pty);
    }

    fncall->fndef = fndef; // elmfn->fndef;
       
    // �������ý����ɹ�
    delete tmpfty;
    return fncall;
}