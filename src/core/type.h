#pragma once


/** 
 * Def ��������
 */


#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "../global.h"

#include "./splitfix.h"
#include "../util/str.h"


// def ��ԭ�������б�
#define DEF_AOTM_TYPE_LIST(T) \
    T(Nil) \
    T(Bool) \
    T(Int) \
    T(Float) \
    T(Char) \
    T(String) \


//    T(Quote) /* ���� */ \




namespace def {
namespace core {
    
using namespace std;
using namespace def::util;


/**
 * Def �������� 
 */
struct Type
{
    virtual string str() = 0;    // �ַ�����ʾ
    virtual void set(Type*) {};  // ���� type
    virtual void add(Type*) {};  // ���� type
    virtual size_t len() { return 1; }; // type ����
    virtual Type* copy(const string& n="") { // ���� type
        return nullptr;
    };
    virtual bool is(Type*t) = 0; // �ж����������Ƿ����
    virtual string getIdentify(bool strict=true) { // ���Ψһ��ʶ
        return str();
    }
// static function
    static map<string, Type*> types; // �����ԭ������
    static Type* get(const string &);

};


// ԭ������
#define AOTM_TYPE(N) \
struct Type##N : Type \
{ \
    virtual string str() { \
        return #N; \
    } \
    virtual bool is(Type*t){ /* ԭ�����Ͳ�������ʱ�ж� */ \
        return dynamic_cast<Type##N*>(t); \
    } \
};

#define AT(T) AOTM_TYPE(T)
DEF_AOTM_TYPE_LIST(AT)
#undef AT

#undef AOTM_TYPE


// ��������
struct TypeRefer : Type
{
    // size_t len;
    Type* type = nullptr; // ����ֵ������
    TypeRefer(Type*t)
        : type(t)
    {}
    virtual bool is(Type*t){
        if (auto *ty = dynamic_cast<TypeRefer*>(t)) {
            return ty->type->is(type); // ��������һ��
        }
        return false;
    }
    virtual void set(Type* t) { // ���� type
        type = t;
    }
    virtual string str() {
        return getIdentify();
    }
    virtual string getIdentify(bool strict=true) { // ���Ψһ��ʶ
        return "~" + type->getIdentify(strict);
    }
};



// ��������
struct TypeArray : Type
{
    size_t len;
    Type* type;
    TypeArray(Type*t, size_t l=0)
        : type(t)
        , len(l)
    {}
    virtual string str() {
        return getIdentify();
    }
    virtual string getIdentify(bool strict=true) { 
        // ���Ψһ��ʶ��strict ��ʾ��СҲ�������
        return "["
            + (strict ? Str::l2s(len) + "*" : "")
            + type->getIdentify(strict)
            + "]";
    }
    virtual void set(Type* t) { // ���� type
        type = t;
    }
    virtual bool is(Type*t){
        if (auto *ty = dynamic_cast<TypeArray*>(t)) {
            return ty->type->is(type); // ��������һ��
        }
        return false;
    }
};



// ��չ����
#define EXTEND_TYPE(N,P) \
struct Type##N : P \
{ \
    virtual bool is(Type*t){ /* ��չ����ֱ�ӶԱȵ�ַ */ \
        return !!( ((int)this)==((int)t) ); \
    }


// �ṹ����
EXTEND_TYPE(Struct, Type)
    long idx = 0;
    static long auto_idx; // ����Ψһ��ʶ������
    string name; // ������
    vector<string> tabs; // �����ʶ��
    vector<Type*> types; // �����б�
    TypeStruct(const string&n)
        : name(n) {
        // cout << "TypeStruct(const string&n)"<< n << endl;
    }
    void increment() {
        idx = ++auto_idx;
    }
    virtual string str() {
        string s = name+"{";
        bool f = true;
        for(auto& it : types) {
            if (f) {
                f = false;
            } else {
                s += ",";
            }
            s += it->str();
        }
        return s+"}";
    }
    virtual void add(Type* t, const string&n="") { // ���� type
        tabs.push_back(n);
        types.push_back(t);
    }
    virtual size_t len() {  // type ����
        return types.size();
    };
    virtual Type* copy(const string&n="") {       // ���� type
        TypeStruct* nts = new TypeStruct(n);
        nts->types = types; // �����ṹ
        return nts; // �����½�������
    };
    int elmpos(const string&n) {  // ��Ԫ��ƫ��
        int i = 0;
        for(auto &it : tabs) {
            if (it==n) {
                return i;
            }
            i++;
        }
        return -1;
    };
    Type* elmget(const string&n) {  // ��Ԫ��ƫ��
        int i = 0;
        for(auto &it : tabs) {
            if (it==n) {
                return types[i];
            }
            i++;
        }
        return nullptr;
    };
    virtual string getIdentify(bool strict=true) { // ���Ψһ��ʶ
        if (idx == 0) {
            return name;
        }
        return name + "." + Str::l2s(idx);
    }

};


// ��������
EXTEND_TYPE(Function,TypeStruct)
    Type* ret; // ����ֵ����
    TypeFunction(const string&n, Type*t=nullptr)
        : TypeStruct(n)
        , ret(t)
    {}
    virtual void set(Type* t) { // ���÷���ֵ type
        ret = t;
    }
    virtual string str() {
        string s(name);
        if (ret) {
            s += ": " + ret->getIdentify();
        }
        s += "(";
        bool fx = false;
        for(auto& it : types) {
            if (fx) s += ",";
            fx = true;
            s += it->getIdentify();
        }
        return s+")";
    }
    virtual string getIdentify(bool strict=true) { // ���Ψһ��ʶ
        string identify(name);
        for(auto& it : types) {
            identify += DEF_SPLITFIX_FUNCARGV+it->getIdentify(strict);
        }
        return identify;
    }
};



#undef EXTEND_TYPE



}
}

