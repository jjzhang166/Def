#pragma once
/**
 * ջɸѡ��
 */

#include <vector>
#include <set>
#include <string>

#include "./element.h"
#include "./stack.h"


namespace def {
namespace parse {

using namespace std;



// ��������ɸѡ��
struct filterLet
{
    map<string, ElementLet*> lets;
    ElementLet* unique  = nullptr; // Ψһƥ��
    filterLet(Stack*, const string &);
    // ɸѡ������
    void initElementGroup(Stack*, const string & );
    // ɸѡ��ƥ�� ����ƥ������
    size_t size();
    // ɸѡ��ƥ�� ����ƥ������
    size_t match(const string &);
};



// ����ɸѡƥ��
struct filterFunction
{
    vector<ElementFunction*> funcs;
    ASTFunctionDefine* unique  = nullptr; // Ψһƥ��
    // up = �Ƿ����ϲ���׼��ջ
    filterFunction(Stack*, const string &, bool up=true);
    // ɸѡ������
    void initElementGroup(Stack*, const string &);
    // ɸѡ��ƥ�� ����ƥ������
    size_t size();
    // ɸѡ��ƥ�� ����ƥ������
    size_t match(TypeFunction*);


};



}
}