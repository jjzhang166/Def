#pragma once


#include <set>
#include <list>
#include <vector>
#include <string>

#include "../core/ast.h"
#include "../core/type.h"
#include "./element.h"
#include "./stack.h"
#include "./tokenizer.h"


namespace def {
namespace parse {
    
using namespace std;
using namespace def::core;


#define Namespace vector<string>


/**
 * �﷨����������
 */
class Service
{
    

public:

    Service(Tokenizer * t);
    
    Tokenizer * tkz; // �ʷ�������
    
    Stack * stack; // ��ǰ����ջ
    map<TypeStruct*, Stack*> type_member_stack; // ���Աջ
    map<TypeStruct*, ASTTypeDefine*> type_define;  // ���Աջ

    // ��ȡһ������
    Tokenizer::Word getWord();
    void prepareWord(const Tokenizer::Word &); // Ԥ��
    void prepareWord(list<Tokenizer::Word>); // ����Ԥ��


public: // �����ͺ���
    
    static Type* getType(AST*); // ���ͻ�ȡ
    static bool checkType(Type*, AST*); // �����ж�

public:

    string fixNamespace(const string &); // ׷�����ֿռ�
    // ��鲢���� include �ļ��ľ���·��
    bool checkSetInclude(const string&);

    // �ȽϺ�������ֵ����
    void verifyFunctionReturnType(Type*); 
    
public:
    
    // �Ƿ�Ϊ���캯��״̬
    bool status_construct = false;
    
public:
    // ����/Ԥ���ĵ���
    list<Tokenizer::Word> prepare_words; 

    set<string> includes; // �Ѿ� include �������ļ�

    string defspace;       // ��ǰ��������ֿռ�
    Namespace usespaces;   // ����ʹ�õ����ֿռ�
    
};



}
}

