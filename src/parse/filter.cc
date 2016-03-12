
#include "filter.h"

using namespace std;
using namespace def::parse;


/**
 * ��������ɸѡ��
 */



filterLet::filterLet(Stack*stk, const string & name)
{
    while (stk) { // ѭ�����
        initElementGroup(stk, name);
        stk = stk->parent;
    }
}


// ɸѡ������
void filterLet::initElementGroup(Stack*stk, const string & name)
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
size_t filterLet::size()
{
    return lets.size();
}

// ɸѡ��ƥ�� ����ƥ������
size_t filterLet::match(const string & name)
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




/**
 * ����ɸѡ��
 */




filterFunction::filterFunction(Stack*stk, const string & fname, bool up)
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
void filterFunction::initElementGroup(Stack*stk, const string & fname)
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
size_t filterFunction::size()
{
    return funcs.size();
}
// ɸѡ��ƥ�� ����ƥ������
size_t filterFunction::match(TypeFunction* fty) 
{
    bool is_empty_arr = 1;
    unique = nullptr; // ��λ
    vector<ElementFunction*> new_funcs;
    string tmpname = fty->getIdentify(false); // ���ϸ����������С�����
    int tmplen = tmpname.size();
    for (auto &p : funcs) {
        string mname = p->fndef->ftype->getIdentify(false);
        if (tmpname == mname.substr(0, tmplen)) {
            if (tmpname==mname) {
                if(!unique) unique = p->fndef;
            }
            // ����������Ͳ����Ƿ�ϸ���������βγ���Ϊ0����ʵ����ȣ�
            bool atyok = true;
            int i(0);
            for (auto ty : fty->types) {
                if (auto *aty = dynamic_cast<TypeArray*>(p->fndef->ftype->types[i])) {
                    if (aty->len != 0 && aty->len != ((TypeArray*)ty)->len) {
                        atyok = false;
                    }
                }
                i++;
            }
            if(atyok) new_funcs.push_back(p);
        }
    }
    funcs = new_funcs;
    return new_funcs.size();
}

