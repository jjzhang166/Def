

#include "./type.h"


using namespace std;
using namespace def::core;


/**
 * ��ȡ�򻺴����� 
 */
Type* Type::get(const string & name)
{
    // ��ѯ����
    auto ty = types.find(name);
    if (ty!=types.end()) {
        return ty->second;
    }
    // ����
    Type* tp = nullptr;
    if (name=="Nil") {
        tp = new TypeNil();
    } else if (name=="Bool") {
        tp = new TypeBool();
    } else if (name=="Int") {
        tp = new TypeInt();
    } else if (name=="Float") {
        tp = new TypeFloat();
    } else if (name=="Char") {
        tp = new TypeChar();
    } else if (name=="String") {
        tp = new TypeString();
    }
    // ���� 
    if (tp) {
        types[name] = tp;
    }
    return tp;
}

// ��̬��ʼ��
map<string, Type*> Type::types;

// ��̬��ʼ��
map<int, TypeArray*> TypeArray::typtrs;
map<int, TypeRefer*> TypeRefer::typtrs;
map<int, TypePointer*> TypePointer::typtrs;


long TypeStruct::auto_idx = 0;
