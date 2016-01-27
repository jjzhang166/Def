#pragma once


#include <vector>
#include <set>
#include <string>

#include "./element.h"


namespace def {
namespace parse {

using namespace std;


#define ElementStack map<string, Element*>


/**
 * Def block ������
 */
class Stack
{
public:
public:
    
    Stack(Stack*p=nullptr);

    // ��ʼ����������ջ
    void Initialize();


    
public:

    // ��ӡ����ջ
    void print();
    
    // ��������ջ
    Element* put(const string &, Element*); // ���뵱ǰջ
    Element* set(const string &, Element*, bool up = true);
    Element* find(const string &, bool up = true);
    // ��ѯ�����Ƿ��壬���ض���
    ASTFunctionDefine* findFunction(TypeFunction*);
    
    // ������û����庯��
    void addFunction(TypeFunction*, ASTGroup*);
    void addFunction(ASTFunctionDefine*);
    void addBuiltinFunction(const string &); // ͨ���ַ�������ڽ�����

public:
    
    Stack* parent = nullptr; // ������ջ

    ElementStack stack;     // ��ǰ����ջ
    
    ASTFunctionDefine* fundef = nullptr; // ��ǰ���ڶ���ĺ���
    ASTTypeDefine*    tydef  = nullptr; // ��ǰ���ڶ��������


};

#undef Namespace

// ��������ɸѡ��
struct filterLet
{
    map<string, ElementLet*> lets;
    ElementLet* unique  = nullptr; // Ψһƥ��
    filterLet(Stack*stk, const string & name)
    {
        while (stk) { // ѭ�����
            initElementGroup(stk, name);
            stk = stk->parent;
        }
    }
    // ɸѡ������
    void initElementGroup(Stack*stk, const string & name)
    {
        int len = name.size();
        for (auto &p : stk->stack) {
            if (auto *let = dynamic_cast<ElementLet*>(p.second)) {
                if (name == p.first.substr(0,len)) {
                    if (lets.end()==lets.find(name)) {
                        lets[p.first] = let;
                    }
                    if (name == p.first) {
                        if (!unique) unique = let;
                    }
                }
            }
        }
    }
    // ɸѡ��ƥ�� ����ƥ������
    size_t size()
    {
        return lets.size();
    }
    // ɸѡ��ƥ�� ����ƥ������
    size_t match(const string & name)
    {
        unique = nullptr; // ��λ
        map<string, ElementLet*> new_lets;
        int len = name.size();
        for (auto &p : lets) {
            if (name == p.first.substr(0, len)) {
                if (name == p.first) {
                    if (!unique) unique = p.second;
                }
                new_lets[p.first] = p.second;
            }
        }
        lets = new_lets;
        return new_lets.size();
    }
};

// ����ɸѡƥ��
struct filterFunction
{
    vector<ElementFunction*> funcs;
    ASTFunctionDefine* unique  = nullptr; // Ψһƥ��
    // up = �Ƿ����ϲ���׼��ջ
    filterFunction(Stack*stk, const string & fname, bool up=true)
    {
        if(!up){
            initElementGroup(stk, fname);
        } else {
            while (stk) { // ѭ�����
                initElementGroup(stk, fname);
                stk = stk->parent;
            }
        }
    }
    // ɸѡ������
    void initElementGroup(Stack*stk, const string & fname)
    {
        if (auto *fngr = dynamic_cast<ElementGroup*>(stk->find(fname, false))) {
            for(auto &p: fngr->elms){
                if (auto fun=dynamic_cast<ElementFunction*>(p.second)) {
                    funcs.push_back(fun);
                }
            }
        }
    }
    // ɸѡ��ƥ�� ����ƥ������
    size_t size()
    {
        return funcs.size();
    }
    // ɸѡ��ƥ�� ����ƥ������
    size_t match(TypeFunction* fty) 
    {
        unique = nullptr; // ��λ
        vector<ElementFunction*> new_funcs;
        string tmpname = fty->getIdentify();
        int tmplen = tmpname.size();
        for (auto &p : funcs) {
            string mname = p->fndef->ftype->getIdentify();
            if (tmpname == mname.substr(0, tmplen)) {
                if (tmpname==mname) {
                    if(!unique) unique = p->fndef;
                }
                new_funcs.push_back(p);
            }
        }
        funcs = new_funcs;
        return new_funcs.size();
    }


};

}
}
