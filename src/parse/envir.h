#pragma once


/** 
 * Def �﷨��������
 */


#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "../global.h"

#include "./element.h"
#include "../core/ast.h"




namespace def {
namespace parse {

using namespace std;
using namespace def::core;
// using namespace def::util;
// using namespace def::parse;
// using namespace def::compile;



#define ElementStack map<string, Element*>



/**
 * Def �﷨��������
 */

struct Envir
{
    Envir* parent = nullptr; // ������
    
    ElementStack stack;     // ���������������ͣ�����ģ�����ջ
    
    ASTFunctionDefine* wrap = nullptr; // ��㺯��


};





}

}