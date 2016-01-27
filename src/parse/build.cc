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

using namespace std;
using namespace def::core;
using namespace def::util;
using namespace def::sys;
using namespace def::compile;


namespace def {
namespace parse {

    
#define Word Tokenizer::Word 
#define State Tokenizer::State
    
#define ISWS(TS) word.state==State::TS
#define NOTWS(TS) !(ISWS(TS))
#define ISSIGN(S) ISWS(Sign)&&word.value==S
#define NOTSIGN(S) !(ISSIGN(S))
#define ISCHA(S) ISWS(Character)&&word.value==S

    
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
        
        // ���͹��� ��
        if (auto gr = dynamic_cast<ElementType*>(res)) {

            auto *claty = (TypeStruct*)gr->type;
            auto *cst = new ASTTypeConstruct(claty);
            int plen = claty->len();
            while (plen--) { // ������͹������
                cst->add(build());
            }
            return cst;
        }

        // �������� ��
        if (auto gr = dynamic_cast<ElementGroup*>(res)) {
            try {
                return buildFunctionCall(chara, gr);
            } catch (...) {
                // do nothing
                // cout << chara+": no function match" << endl;
            }
        }

        // ģ�庯�����ã�
        res = stack->find(DEF_SPLITFIX_TPFPREFIX+chara, true);
        if (res) {
            return buildTemplateFuntion(chara, (ElementTemplateFuntion*)res);
        }

        // ���Ժ��Ķ��壿
        AST *core = buildCoreDefine(chara);
        if (core) {
            return core;
        }

        // ���Ժ���ָ�
        /*
        AST *ins = buildInstruction(chara);
        if (ins) {
            return ins;
        }
        */

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
    if(stack->tydef && stack->fundef){
        AST* instance = new ASTVariable( // �����
            DEF_MEMFUNC_ISTC_PARAM_NAME,
            stack->tydef->type
        );
        // �����౾��
        if (name==DEF_MEMFUNC_ISTC_PARAM_NAME) {
            stack->fundef->is_static_member = false; // �г�Ա����
            return instance;
        }
        int i = 0;
        for (auto &p : stack->tydef->type->tabs) {
            if (p==name) { // �ҵ�
                // �������Ա����
                AST* elmget = new ASTMemberVisit(instance, i);
                stack->fundef->is_static_member = false; // �г�Ա����
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
 * ��������
 */
AST* Build::buildFunctionCall(const string & name, ElementGroup* grp, bool istpf)
{
    auto *fncall = new ASTFunctionCall(nullptr);
    auto elms = grp->elms;

    // �ڵ㻺��
    list<AST*> cachebuilds;
        
    // string fname(name); // ����������װ��
    // vector<Element*> matchs; // ƥ��ĺ���
    // ElementFunction* elmfn;
    ASTFunctionDefine* fundef(nullptr);

    // ��ʱ������ѯ����������
    auto *tmpfty = new TypeFunction(name);
    filterFunction filter = filterFunction(stack, name);

    // ����̰��ƥ��ģʽ
    while (true) {
        // ƥ�亯������һ���޲Σ�
        int match = filter.match(tmpfty);
        if (match==1 && filter.unique) {
            fundef = filter.unique; // �ҵ�Ψһƥ��
            string idname = fundef->ftype->getIdentify();
            break;
        }
        if (match==0) {
            prepareBuild(cachebuilds);
            throw "No macth function !"; // ��ƥ��
        }
        // �жϺ����Ƿ���ý���
        auto word = getWord();
        prepareWord(word); // �ϲ㴦������
        if (ISWS(End) || ISSIGN(")")) { // ��ǰ�ֶ����ý���
            if (filter.unique) {
                fundef = filter.unique; // �ҵ�Ψһƥ��
                break;
            }
        }
        // ��Ӳ���
        AST *exp = build();
        if (!exp) {
            if (filter.unique) {
                fundef = filter.unique; // �޸���������ҵ�ƥ��
                break;
            }
        }
        cachebuilds.push_back(exp);
        fncall->addparam(exp); // ��ʵ��
        auto *pty = getType(exp);
        // ���غ�����
        tmpfty->add("", pty);
    }

    fncall->fndef = fundef; // elmfn->fndef;
        
    return fncall;

}

/**W
 * ����ģ�庯��
 */
AST* Build::buildTemplateFuntion(const string & name, ElementTemplateFuntion* tpf)
{
    // �º�������
    TypeFunction* functy = new TypeFunction(name);
    
    // �����º���
    auto *fundef = new ASTFunctionDefine(functy);

    // ����ģ�庯������
    auto *fcall = new ASTFunctionCall(fundef);

    // �����·���ջ
    Stack* old_stack = stack;
    Stack new_stack(stack);
    fundef->wrap = stack->fundef; // wrap
    new_stack.fundef = fundef; // ��ǰ����ĺ���

    // ElementStack nsk; // ����ɱ���
    // ʵ����ջ


    //list<AST*> paramcaches;
    // ElementStack nsk; // ����ɱ���
    for (auto &pn : tpf->tpfdef->params) {
        AST* p = build(); // ��ȡ����
        //paramcaches.push_back(p); // ����
        fcall->addparam(p);
        Type *ty = getType(p);
        functy->add(pn, ty); // ��������
        new_stack.put(pn, new ElementVariable(ty)); // ��ʵ��
        //nsk[pn] = old; // ���汻���ǵ�
    }

    /*
    int i(0);
    for (auto &pty : functy->types) {
        string pn(functy->tabs[i]);
        new_stack.put(pn, new ElementVariable(pty)); // ��ʵ��
        i++;
    }
    */

    // �滻��ջ֡
    stack = & new_stack;
    
    // ���������
    // prepare_words = tpf->tpfdef->bodywords;
    prepareWord(tpf->tpfdef->bodywords);

    // ����������
    ASTGroup *body = createAST();
    auto word = getWord();
    if (NOTSIGN(")")) {
        FATAL("Error format function body !")
    }

    // ����������
    // ����º���
    Type* tyret(nullptr);
    size_t bodylen = body->childs.size();
    if(bodylen>0){
        // ��ȡ���������һ��Ϊ��������
        tyret = getType(body->childs[bodylen-1]);
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
    fundef->ftype = functy;
    // ���� Body
    fundef->body = body;

    
    // ��λ��ջ֡
    stack = old_stack;

    // ����º���
    stack->addFunction(fundef);

    // ���û���Ĳ���
    // prepareBuild(paramcaches);

    
    return fcall;


    // ��������
    /*
    if (auto res = dynamic_cast<ElementGroup*>(stack->find(name, true))) {
        return buildFunctionCall(name, res, true);
    } else {
        FATAL("Add new function fail !")
    }
    */

    // TODO:: ������������
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
    stack->put(name, new ElementVariable(getType(value)));

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
                stack->fundef->is_static_member = false; // �г�Ա����
                return instance;
            }
            Type* elmty = stack->tydef->type->elmget(name);
            if (elmty) { // �ҵ���Ա
                AST* value = build();
                // ���ͼ��
                if ( ! getType(value)->is(elmty)) {
                    FATAL("member assign type not match !")
                }
                stack->fundef->is_static_member = false; // �г�Ա����
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
    Type* vty = getType(value);

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
    if (stack->fundef) {
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
    new_stk->fundef = nullptr; // ���Ա�����������κΰ�������
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
        AST* fundef(nullptr);
        if("fun"==word.value){
            fundef = build_fun();
        } else if ("dcl" == word.value) {
            fundef = build_dcl();
        }
        if (fundef) {
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
    ASTFunctionDefine* fundef = stack->findFunction(functy);

    // �жϺ����Ƿ��Ѿ�����
    if (!fundef) {
        fundef = new ASTFunctionDefine(functy, nullptr);
        // ����º���
        stack->addFunction(fundef);
    }

    // ���غ�������
    return new ASTFuntionDeclare(fundef->ftype);
}

/**
 * fun ���庯��
 */
AST* Build::build_fun()
{
    Word word = getWord();
    if (NOTWS(Character)) {
        FATAL("function define need a type name to belong !")
    }

    // ��������ֵ����
    Type *rty(nullptr); // nullptr ʱ��Ҫ�ƶ�����
    Element* elmret = stack->find(word.value);
    if (auto *ety = dynamic_cast<ElementType*>(elmret)) {
        rty = ety->type;
    } else {
        prepareWord(word);
    }

    // ��������
    word = getWord();
    if (NOTWS(Character)) {
        FATAL("function define need a legal name !")
    }

    // �½���������
    string funcname = word.value;
    auto *functy = new TypeFunction(funcname);

    // ����
    word = getWord();
    if (NOTSIGN("(")) {
        FATAL("function define need a sign ( to belong !")
    }

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
        } else {
            FATAL("Parameter format is not valid !")
        }
        word = getWord();
        if (NOTWS(Character)) {
            FATAL("function parameter need a legal name !")
        }
        pnm = word.value;
        if (stack->tydef && pnm==DEF_MEMFUNC_ISTC_PARAM_NAME) {
            // ���Ա�������������Ƴ�ͻ
            FATAL("can't give a parameter name '"+pnm+"' in type member function !")
        }
        functy->add(pnm, pty);
    }

    // �����Ƿ��Ѿ�����
    ASTFunctionDefine* aldef = stack->findFunction(functy);
    ASTFunctionDefine *fundef;

    // �жϺ����Ƿ��Ѿ�����
    if (aldef){
        if (aldef->body) {
            // �������������� body �Ѷ���
            FATAL("function define repeat: " + functy->str());
        }
        fundef = aldef;
    } else {
        // �½���������
        fundef = new ASTFunctionDefine(functy);
    }

    // ���ÿ��ܱ�ǵķ���ֵ
    functy->ret = rty;
    fundef->ftype = functy;

    // ����������

    // �����·���ջ
    Stack* old_stack = stack;
    Stack new_stack(stack);
    new_stack.fundef = fundef; // ��ǰ����ĺ���
    // ��ǰ ��Ӻ��� ֧�ֵݹ�
    new_stack.addFunction(fundef);

    // �������廷��
    fundef->wrap = stack->fundef; // wrap
    fundef->belong = stack->tydef; // belong

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

    // ����ֵ��֤���ƶ�
    size_t bodylen = body->childs.size();
    Type *lastChildTy(nullptr);
    if(bodylen>0){
        // ��ȡ���������һ��Ϊ��������
        AST* last = body->childs[bodylen - 1];
        lastChildTy = getType(last);
    }

    // ��鷵��ֵ����һ����
    verifyFunctionReturnType(lastChildTy);

    if ( ! functy->ret) {
        cout << "! functy->ret" << endl;
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
    fundef->body = body;

    // �����Ѿ��������ˣ���� body
    if (aldef) {
        // �滻 ftype�������������
        // delete aldef->ftype;
        // aldef->ftype = functy;

    // ��������º���
    } else {
        stack->addFunction(fundef);
    }

    // ��ӵ����Ա����
    if(stack->tydef){
        stack->tydef->members.push_back(fundef);
    }

    // ����ֵ
    return fundef;
}

/**
 * ret ����ֵ����
 */
AST* Build::build_ret()
{
    AST *ret = build();
    // ��֤����ֵ
    verifyFunctionReturnType( getType(ret) );
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
    if (stack->find(DEF_SPLITFIX_TPFPREFIX + tpfName)) {
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
    stack->put(DEF_SPLITFIX_TPFPREFIX + tpfName, new ElementTemplateFuntion(tpfdef));

    return tpfdef;
}

/**
 * if ������֧
 */
AST* Build::build_if()
{

#define DO_FATAL FATAL("No match bool function Type<"+idn+"> when be used as a if condition")
    
    AST* cond = build();
    Type* cty = getType(cond);
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
    Type* cty = getType(cond);
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
    auto word = getWord();
    if (NOTSIGN("(")) {
        FATAL("let binding need a sign ( to belong !")
    }

    // Ψһ����
    string idname("");
    bool sign = false;

    auto *let = new ElementLet();
    auto *relet = new ASTLet();

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
            idname += "_";
        } else if (ISWS(Operator)) {
            idname += str;
            sign = true;
        } else {
            FATAL("let unsupported the symbol type '"+str+"' !")
        }
    }

    if (!sign) { // �����ṩһ������
        FATAL("let binding must provide at least one symbol !")
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
 * let ���Ű�չ��
 *
AST* Build::spreadLetBind(list<Word>*pwds)
{
    string idname("");

    // ����
    vector<Word> params;
    Word placeholder;
    if (pwds) { // �ϲ���ڣ�����������
        idname = "_";
        params.push_back(placeholder);
    }

    list<Word> caches;
    
    ElementLet* let(nullptr);
    //ElementLet* macth(nullptr);
    //string stkn("");

    filterLet* filter(nullptr);

    while (true) {
        auto word = getWord();
        caches.push_back(word);
        if (ISWS(Operator)) {
            idname += word.value;
        }
        else {
            idname += "_";
            params.push_back(word);
        }
        if (!filter) {
            filter = new filterLet(stack, idname);
            continue;
        }
        int num = filter->match(idname);
        if (filter->unique) {
            let = filter->unique;
            goto macth_success; // �ҵ�Ψһƥ��
        }
        if (num==0) {
            FATAL("let bind can't find !")
        }
    }

    FATAL("let bind can't find until the end !")


    macth_success:

    // ��������
    map<string, Word> pmstk;
    int i = 0;
    for (auto &p : let->params) {
        pmstk[p] = params[i];
        i++;
    }

    // չ������
    list<Word> results;
    for (auto &word : let->bodywords) {
        string str(word.value);
        if (ISWS(Character) && pmstk.find(str) != pmstk.end()) {
            auto wd = pmstk[str];
            if (wd == placeholder) {
                for (auto &p : *pwds) {
                    // �ϲ�����������滻
                    results.push_back(p);
                }
            }
            else {
                results.push_back(wd); // �����滻
            }
        }
        else {
            results.push_back(word);
        }
    }

    auto word = getWord();

    // �鿴�������Ƿ���Ҫ�����󶨷���
    if (ISWS(Operator)) {
        // ������
        prepareWord(word);
        return spreadLetBind(&results);
    } else {

        cout << "spreadLetBind��";
        for (auto &p : results) {
            cout << " " << p.value;
        }
        cout << "��" << endl;


        // ���Ű󶨽���
        prepareWord(word);
        // Ԥ��
        prepareWord(results);
        // ���¿�ʼ
        return build();

    }









}
*/
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
        tyclass  = dynamic_cast<TypeStruct*>(getType(val));
    }

    if( ! tyclass){
        FATAL("elmivk must belong a struct value '"+word.value+"' !")
    }

    // ��ѯĿ�����Ա����
    word = getWord();
    auto target_stack = type_member_stack[tyclass];
    // ��Ա�������ã������ϲ���
    auto filter = filterFunction(target_stack, word.value, false);
    if (!filter.size()) {
        FATAL("can't find member function '"
            +word.value+"' in class '"+tyclass->name+"'")
    }


    string name = word.value;
    auto *fncall = new ASTFunctionCall(nullptr);
    // auto elms = grp->elms;

    // �ڵ㻺��
    //list<AST*> cachebuilds;
        
    // string fname(name); // ����������װ��
    // vector<Element*> matchs; // ƥ��ĺ���
    // ElementFunction* elmfn;
    ASTFunctionDefine* fundef(nullptr);

    // ��ʱ������ѯ����������
    auto *tmpfty = new TypeFunction(name);


    // ������Ա��������

        // ����̰��ƥ��ģʽ
    while (true) {
        // ƥ�亯������һ���޲Σ�
        int match = filter.match(tmpfty);
        if (match==1 && filter.unique) {
            fundef = filter.unique; // �ҵ�Ψһƥ��
            string idname = fundef->ftype->getIdentify();
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
                fundef = filter.unique; // �ҵ�Ψһƥ��
                break;
            }
        }
        // ��Ӳ���
        AST *exp = build();
        if (!exp) {
            if (filter.unique) {
                fundef = filter.unique; // �޸���������ҵ�ƥ��
                break;
            }
            FATAL("No macth function '"+tyclass->name+"."+tmpfty->getIdentify()+"' !");
        }
        //cachebuilds.push_back(exp);
        fncall->addparam(exp); // ��ʵ��
        auto *pty = getType(exp);
        // ���غ�����
        tmpfty->add("", pty);
    }



    /*
    while (1) {
        AST *exp;
        auto word = getWord();
        if (ISSIGN(";")) { // �������ý���
            int match = filter.match(tmpfty);
            if(filter.unique){
                fundef = filter.unique;
                break;
            }
            if (match==0) {
                prepareBuild(cachebuilds); // ��λ����ڵ�
                throw "No macth function !";
                // FATAL("No macth function !")
            }
        } else {
            prepareWord(word);
        }
        // ƥ�亯������һ���޲Σ�
        int match = filter.match(tmpfty);
        if(filter.unique){
            fundef = filter.unique;
            break;
        }
        if (match==0) {
            prepareBuild(cachebuilds); // ��λ����ڵ�
            throw "No macth function !";
            // FATAL("No macth function !")
        }
        // ��Ӳ���
        exp = build();
        cachebuilds.push_back(exp);
        fncall->addparam(exp); // ��ʵ��
        auto *pty = getType(exp);
        // ���غ�����
        tmpfty->add("", pty);
    }
    */

    fncall->fndef = fundef; // elmfn->fndef;
        
    // return fncall;





    /*


    // ������ջ
    auto old_stack = stack;
    stack = type_member_stack[tyclass];

    // ���ú���
    AST* res = build();
    auto* fcall  = dynamic_cast<ASTFunctionCall*>(res);
    if( ! fcall){
        FATAL("elmivk must belong a member function call !")
    }
    // ��λջ
    stack = old_stack;

    
    */


    // ��̬��Ա������֤
    if (is_static && ! fundef->is_static_member) {
        FATAL("'"+fundef->ftype->name+"' is not a static member function !")
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
    auto* scty = dynamic_cast<TypeStruct*>(getType(sctval));
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
    auto* scty = dynamic_cast<TypeStruct*>(getType(sctval));
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
    Type* putty = getType(putv);
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
        idname = "_";
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
            idname += "_";
        }else if (ISWS(Operator)) { // ������
            idname += word.value;
        } else {
            idname += "_";
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
        return spreadOperatorBind(&results);
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





}
}
