#pragma once

/** 
 * Def �﷨������
 */


#include <string>
#include <vector>
#include <map>

#include "../global.h"
#include "../util/str.h"

#include "../core/builtin.h"
#include "../core/type.h"
#include "../core/ast.h"
#include "./tokenizer.h"
#include "./service.h"
#include "./stack.h"


namespace def {
namespace parse {
    
using namespace std;
using namespace def::core;
using namespace def::util;





/**
 * Def ast ������
 */
class Build : public Service
{
    
public:

    // ����������
    ASTGroup* createAST();

protected:
    
    Build(Tokenizer * t);

    // ������״̬
    // bool ctx_type = false;  // ���Ͷ���
    // bool ctx_fun  = false;  // ��������
    
protected:

    list<AST*> prepare_builds; // ����Ľڵ�

    void prepareBuild(AST*);
    void prepareBuild(const list<AST*> &);

    // ������������ cache ����ȥ����Ľڵ���
    AST* build(bool spread=true); // spread = ����� let ��
    // ���� Group
    ASTGroup* buildGroup();
    // ��������
    // AST* buildFunctionCall(const string &, ElementGroup*, bool istpf=false);
    // ����ģ�庯��
    AST* buildTemplateFuntion(const string &, ElementTemplateFuntion*);
    // ���Ķ��崦��
    AST* buildCoreDefine(const string &);
    // ���������Ա����
    AST* buildVaribale(Element*, const string &n="");
    // �������캯������
    AST* buildConstruct(ElementType*, const string &n="");
    AST* buildMacro(ElementLet*, const string &);

protected:

    // let ���Ű�չ��
    // AST* spreadLetBind(list<Tokenizer::Word>*pwds=nullptr);
    AST* buildOperatorBind();
    bool forecastOperatorBind(); // Ԥ���Ƿ���Ҫ�⿪���Ű�
    list<Tokenizer::Word> spreadOperatorBind(list<Tokenizer::Word>*pwds=nullptr);
    
protected:

    // �����������ã�up=�Ƿ����ϲ���
    ASTFunctionCall* _functionCall(const string &, Stack*, bool up=true);
    // �Ӻ���ͷ���������壩�����������ͣ�declare=�Ƿ�Ϊ������ʽ
    TypeFunction* _functionType(bool declare=false);

    // ���浥�ʶΣ����������ڲ��������ݣ�
    void cacheWordSegment(list<Tokenizer::Word>&);


protected:

    /** def ���Ķ����б� **/
#define T(N) AST* build_##N();
    BUILTIN_DEFINES_LIST(T)
#undef T

    
};


}
}

