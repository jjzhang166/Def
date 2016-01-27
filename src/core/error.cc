/**
 * ����������󱨸�
 */


#include "./error.h"


using namespace def::core;


#define EP Error::Position


Tokenizer* Error::tkz = nullptr;
vector<EP> Error::positions;


/**
 * ������ֹ
 */
void Error::exit(const string & head, const string & foot)
{
    // ���ͷ����Ϣ
    cerr<<endl<<"[Error]: "<<head<<endl;

    // ��ӡ������
    cerr << endl << getFoucsLine() << endl;
    
    EP cur = curPosition();
    // ������
    //cerr << endl  << cur.foucs << endl;
    // �ļ�·�� (line, cursor)
    cerr << tkz->file << " (" << cur.line
         << "," << (cur.cursor+1) << ")" << endl;

    // ���β����Ϣ
    cerr << endl << foot << endl;

    std::exit(0);
}



/**
 * ��ӡ�����α�λ��
 */
string Error::getFoucsLine()
{
    ifstream fin(tkz->file);
    EP cur = curPosition();
    int l = cur.line;
    char lcs[1024];
    while (l--)
        fin.getline(lcs, 1024); // ����֮ǰ��
    string line(lcs);
    // ���� focusλ��
    int wrap = 33;
    int fcs = cur.cursor;
    // ��ض�
    int lov = cur.cursor - wrap;
    if (lov>0) {
        line = "..." + line.substr(lov);
        fcs -= lov - 3;
    }
    // �ҽض�
    int rov = line.size() - cur.cursor - wrap;
    if (rov>0) {
        line = line.substr(0, wrap*2 + 3) + "...";
    }
    // ���� 
    string foucs(fcs, ' ');
    return line + "\n" + foucs + "^"; // ��-^����^���ѡ�񨐡���*~-
}


/**
 * ��������
 */
EP Error::curPosition()
{
    if (!positions.empty()) {
        return positions[positions.size()-1];
    }
    else {
        return createPosition();
    }
}



/**
 * ��������
 */
EP Error::createPosition()
{
    tkz->jumpWhitespace(); // �����հ�
    EP ep;
    // ep.foucs = tkz->getFoucsLine();
    ep.line = tkz->curline;
    ep.cursor = tkz->cursor;
    return ep;
}



/**
 * �������������
 */
void Error::snapshot()
{
    positions.push_back(createPosition());
}


/**
 * ���˿���
 */
void Error::backspace(int n)
{
    while(n--)
        positions.pop_back();
}
