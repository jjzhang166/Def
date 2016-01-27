#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "../parse/tokenizer.h"


namespace def {
namespace core {
    
using namespace std;
using namespace def::parse;


/**
* ���󱨸�
*/
class Error
{

public:
    // ����ջ�ṹ
    struct Position {
        size_t line;   // ������
        size_t cursor; // �α�λ��
    };

public:
    
    static Tokenizer* tkz;  // ��ǰ������
    static void update(Tokenizer* t) { 
        tkz = t;
    }

    // ���󱨸沢��ֹ����
    static void exit(const string &, const string & f="");
    static string getFoucsLine(); // �����м��α�λ��

    // ����ջ����
    static Position createPosition(); // ��������
    static Position curPosition(); // ��ǰ����
    static void snapshot();  // �������������
    static void backspace(int n=1); // ����

private:

    // ���ռ�¼
    static vector<Position> positions;

};


}
}



// ��ͨ����
#define ERR(str) cerr<<str<<endl;
// ��������
#define FATAL(str) cerr<<endl<<endl<<str<<endl; exit(1);
