#pragma once



// �ڽ����Ժ��Ķ���
#define BUILTIN_DEFINES_LIST(D) \
    D(include)    /*�ļ�����*/ \
    D(namespace)  /*���ֿռ�*/ \
    \
    D(let) /*���Ű�*/ \
    \
    D(type)/*���Ͷ���*/ \
    D(dcl) /*��������*/ \
    D(fun) /*��������*/ \
    D(ret) /*��������*/ \
    D(tpf) /*ģ�庯������*/ \
    D(var) /*��������*/ \
    D(set) /*������ֵ*/ \
    \
    D(adt) /*������ģʽ*/ \
    \
    D(if) /*if else*/ \
    D(while) /*while*/ \
    \
    D(refer) /*����Ϊ����*/ \
    D(array) /*����Ϊ����*/ \
    \
    D(arrget) /*�����Ա����*/ \
    D(arrset) /*�����Ա��ֵ*/ \
    \
    D(elmget) /*���Ա����*/ \
    D(elmset) /*���Ա��ֵ*/ \
    D(elmivk) /*���Ա��������*/ \
    D(elmdef) /*���Ա�����ⲿ����*/ \
    \
    D(mcrif)  /*������չ��*/ \
    D(mcrfor) /*���ظ�չ��*/ \
    D(mcrcut) /*��ֶ��п�*/ \
    

    // \
    // D(link) /*ȡ�ñ�������*/ \
    // D(load) /*�������������*/ \
  

// �ڽ���������һ������Ϊ��������ֵ���ͣ�ʣ���Ϊ�������ͣ�
#define BUILTIN_FUNCTION_LIST(B,C) \
    /* �ڽ����� */ \
    B(bool, "Bool,Bool") \
    B(bool, "Bool,Int") \
    B(add, "Int,Int,Int") \
    B(sub, "Int,Int,Int") \
    /* C ��׼�⺯�� */ \
    /* math.h */ \
    C(abs, "Int,Int") \
    /* stdio.h */ \
    C(getchar, "Int") \
    C(putchar, "Int,Int") \
    C(getchar, "Char") \
    C(putchar, "Char,Char") \
    C(gets,    "String") \
    C(puts,    "Int,String") \


