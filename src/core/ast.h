#pragma once

/** 
 * Def �﷨������
 */


#include <string>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <iostream>

#include "../global.h"

#include "./type.h"
#include "./error.h"

#include "../compile/gen.h"
#include "../parse/tokenizer.h"
#include "../util/str.h"

#include "llvm/IR/IRBuilder.h"

namespace def {
namespace core {
    
using namespace std;
using namespace def::util;
using namespace def::parse;
using namespace def::compile;


/**
 * Def ��������� 
 */
struct AST
{
    virtual llvm::Value* codegen(Gen &) { return nullptr;  };
    virtual void print(string pre="", string ind="") {};
    virtual def::core::Type* getType() {
        FATAL("cannot call getType() , this AST is not a <value> !")
    };
    // �Ƿ�Ϊ value ֵ�����˺��������͵�����������ȣ�
    virtual bool isValue() { return false; }; 
};


class ASTTypeDefine;
class ASTFunctionDefine;



#define AST_HEAD(N) \
struct AST##N : AST \
{ \
    virtual void print(string, string); \


#define AST_CODE_HEAD(N) \
AST_HEAD(N) \
    virtual llvm::Value* codegen(Gen &); \
    virtual def::core::Type* getType(); \
    virtual bool isValue() { return true; }; \


// ��
AST_CODE_HEAD(Group)
    vector<AST*> childs; // �б�
    void add(AST*a); // ����Ӿ�
};


// ����
AST_CODE_HEAD(Constant)
    Type* type; // ��������
    string value; // ��������ֵ
    ASTConstant(Type*t, const string&v)
        : type(t)
        , value(v)
    {}
};



// ��������ֵ
AST_CODE_HEAD(Ret)
    AST* value;
    ASTRet(AST*v)
        : value(v)
    {}
};


// if ������֧
AST_CODE_HEAD(If)
    bool canphi=false; // IF��֧������һ�£��ɽ��� PHI �ڵ�
    AST* cond;
    AST* pthen=nullptr;
    AST* pelse=nullptr;
    ASTIf(AST*c)
        : cond(c)
    {}
};


// while ѭ��
AST_CODE_HEAD(While)
    AST* cond;
    AST* body=nullptr;
    ASTWhile(AST*c)
        : cond(c)
    {}
};


// ���Ͷ���
AST_HEAD(TypeDefine)
    TypeStruct* type;
    vector<ASTFunctionDefine*> members; // ��Ա����
    // bool checkMember(ASTFuntionDefine* fdef);
    // void addMember(ASTFuntionDefine* fdef);
};


// �ⲿ��Ա��������
AST_HEAD(ExternalMemberFunctionDefine)
    TypeStruct* type;
    AST* defs;
    ASTExternalMemberFunctionDefine(TypeStruct*t=nullptr,AST*c=nullptr)
        : type(t)
        , defs(c)
    {}
};



// ���͹���
AST_CODE_HEAD(TypeConstruct)
    TypeStruct* type;
    vector<AST*> childs; //
    bool bare = false; // �չ���
    ASTTypeConstruct(TypeStruct*t=nullptr, bool b=false)
        : type(t)
        , bare(b)
    {}
    void add(AST*);
};


// ����
AST_CODE_HEAD(Variable)
    string name; // ����
    Type* type;  // ����
    ASTVariable(const string&n, Type*t)
        : name(n)
        , type(t)
    {}
};


// ��������
AST_CODE_HEAD(VariableDefine)
    string name;
    AST* value; // 
    ASTVariableDefine(const string &n = "", AST*v = nullptr)
        : name(n)
        , value(v)
    {}
};


// ������ֵ
AST_CODE_HEAD(VariableAssign)
    string name;
    AST* value; // 
    ASTVariableAssign(const string &n = "", AST*v = nullptr)
        : name(n)
        , value(v)
    {}
};


// ��������
AST_HEAD(FuntionDeclare)
    TypeFunction* ftype; // ��������
    ASTFuntionDeclare(TypeFunction*ft)
        : ftype(ft)
    {}
};


// ��������
AST_HEAD(FunctionDefine)
    TypeFunction* ftype; // ��������
    ASTGroup* body; // ������
    ASTFunctionDefine* wrap = nullptr; // ��㺯��
    ASTTypeDefine*  belong = nullptr; // ������
    bool is_static_member  = true;    // �Ƿ�Ϊ��̬��Ա����
    bool is_construct  = false;    // �Ƿ�Ϊ���캯��
    set<string> cptmbr;        // ����ʹ�õ����Ա����
    map<string, Type*> cptvar;  // �������������ı���
    ASTFunctionDefine(TypeFunction*ft, ASTGroup *bd=nullptr)
        : ftype(ft)
        , body(bd)
    {}
    string getWrapPrefix(); // ��ȡ��㺯��ǰ׺
    string getIdentify();   // ��ȡΨһ�ĺ�������
};

// ��������
AST_CODE_HEAD(FunctionCall)
    ASTFunctionDefine* fndef; // ��������
    vector<AST*> params; // ʵ��ֵ��
    ASTFunctionCall(ASTFunctionDefine*fd=nullptr)
        : fndef(fd)
    {}
    void addparam(AST*);// ���ʵ��
};

// ��Ա��������
AST_CODE_HEAD(MemberFunctionCall)
    ASTFunctionCall* call; // ��������
    AST* value; // ��ʵ��
    ASTMemberFunctionCall(AST*v=nullptr, ASTFunctionCall*c=nullptr)
        : call(c)
        , value(v)
    {}
};

// ��Ա����
AST_CODE_HEAD(MemberVisit)
    size_t index; // ��Ԫ������
    AST* instance; // ��ʵ��
    ASTMemberVisit(AST*v=nullptr, size_t i=0)
        : index(i)
        , instance(v)
    {}
};

// ��Ա��ֵ
AST_CODE_HEAD(MemberAssign)
    size_t index; // ��Ԫ������
    AST* instance; // ��ʵ��
    AST* value; // ��ֵ
    ASTMemberAssign(AST*m=nullptr, size_t i=0, AST*c=nullptr)
        : index(i)
        , instance(m)
        , value(c)
    {}
};



// ģ�庯������
AST_HEAD(TemplateFuntionDefine)
    string name;
    vector<string> params;
    list<Tokenizer::Word> bodywords; // �����嵥�ʱ�
    ASTTemplateFuntionDefine()
    {}
    void addword(string);
};


// let ���Ű�
AST_HEAD(Let)
    vector<string> head;
    vector<string> body;
    ASTLet()
    {}
};






#undef AST_HEAD



}
}

