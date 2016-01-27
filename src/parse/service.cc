/**
 *
 */

#include "../core/type.h"
#include "../core/error.h"
#include "./service.h"

#include "../sys/debug.h"


using namespace std;
using namespace def::core;
using namespace def::parse;
using namespace def::sys;


#define Type def::parse::Type


/**
 * ����
 */
Service::Service(Tokenizer * t)
    : tkz(t)
{
    // ��ʼ������ջ
    stack = new Stack(nullptr);
}
    


/**
 * �ȽϺ�������ֵ����
 */
void Service::verifyFunctionReturnType(Type* ret)
{
    auto *fndef = stack->fundef;
    if ( ! fndef) {
        FATAL("Non existence function cannot return value !")
    }
    if ( ret && ! fndef->ftype->ret) {
        fndef->ftype->ret = ret;
        return;
    }
    if (ret && ! ret->is(fndef->ftype->ret)) {
        // ��������ֵ������ƥ��
        FATAL("Function '"+fndef->ftype->name+"' return value type not match !")
    }
}



/**
 * ��ѯ����
 * up: �Ƿ����ϲ���
 */
string Service::fixNamespace(const string & name)
{
    if (defspace=="") {
        return name;
    }

    return defspace + "_" + name;
}





/**
 * ��鲢���� include �ļ��ľ���·��
 */
bool Service::checkSetInclude(const string& path)
{
    if (includes.find(path) == includes.end()) {
        includes.insert(path);
        return false;
    }
    // �Ѱ���
    return true;
}


    
/**
 * ��ȡһ������
 */
Tokenizer::Word Service::getWord()
{
    // ����Ԥ��
    if (!prepare_words.empty()) {
        auto rt = prepare_words.front();
        prepare_words.pop_front();
        return rt;
    }

    // ��ȡ�´�
    Tokenizer::Word word  = tkz->gain();
    // ���Դ�ӡ token list
    DEBUG_WITH("tok_list", cout << word.value << " , ";)

    return word;
}


/**
 * ����һ�����ʵ�Ԥ���б�
 */
void Service::prepareWord(const Tokenizer::Word & wd)
{
    prepare_words.push_front(wd);
}


/**
 * ����һ���б�Ԥ���б�
 */
void Service::prepareWord(list<Tokenizer::Word> wds)
{
    // ���� �� ���µ�Ԥ������뵽��ͷ
    prepare_words.splice(prepare_words.begin(), wds);
}



/**
 * �����ж�
 */
bool Service::checkType(Type* type, AST* ast)
{
    return type->is(getType(ast));
}


/**
 * �����ж�
 */
Type* Service::getType(AST* ast)
{
#define ISAST(T) AST##T* con = dynamic_cast<AST##T*>(ast)
    
    // ��������
    if (ISAST(Constant)) {
        return con->type;
    }

    // ��������
    if (ISAST(Variable)) {
        return con->type;
    }

    // ��������
    if (ISAST(VariableDefine)) {
        return getType( con->value );
    }

    // ������ֵ
    if (ISAST(VariableAssign)) {
        return getType( con->value );
    }

    // ���͹���
    if (ISAST(TypeConstruct)) {
        return con->type;
    }
    
    // ��������
    if (ISAST(FunctionCall)) {
        return con->fndef->ftype->ret;
    }

    // ��������ֵ
    if (ISAST(Ret)) {
        return getType( con->value );
    }

    // ��Ա��������
    if (ISAST(MemberFunctionCall)) {
        return getType( con->call );
    }

    // ��Ա����
    if (ISAST(MemberVisit)) {
        TypeStruct* scty = (TypeStruct*)getType( con->instance );
        return scty->types[con->index];
    }

    // ��Ա��ֵ
    if (ISAST(MemberAssign)) {
        return getType( con->value );
    }


    return nullptr;
#undef ISAST
}




