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
    auto *fndef = stack->fndef;
    if ( ! fndef) {
        FATAL("Non existence function cannot return value !")
    }
    if (ret && status_construct) { // ���캯�������з���ֵ
        FATAL("class construct function cannot have any return value !")
    }
    if ( ret && ! fndef->ftype->ret) {
        fndef->ftype->ret = ret;
        return;
    }
    if ( ret && ! ret->is(fndef->ftype->ret)) {
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
 * ����һ���б���Ԥ���б�
 */
void Service::prepareWord(list<Tokenizer::Word> wds)
{
    // ���� �� ���µ�Ԥ�������뵽��ͷ
    prepare_words.splice(prepare_words.begin(), wds);
}



/**
 * �����ж�
 */
bool Service::checkType(Type* type, AST* ast)
{
    return type->is(ast->getType());
}
