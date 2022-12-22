#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;
int main(){
    string buf;
    ifstream newFile;
    newFile.open("memory.txt",ios::in);
    char command[4];
    int offset = 0;
    int rd = 0;
    int rs = 0;
    int rt = 0;
    /*
    Retrieve a line of command,
    remove comma from it, then use stringstream getline() method,
    and space character as delimeter to get different string such
    as command or register number...
    */
    for (int i = 0; i < 4; i++){
        getline(newFile, buf);
        // replace(buf.begin(), buf.end(), ',', );

        // buf.erase(remove(buf.begin(),buf.end(),','), buf.end());
        // while(sscanf(buf.c_str(),"%s" "%d" "%d",command,rd,rs,rt)){
        cout << buf;
        sscanf(buf.c_str(),"%s $%d, %d($%d)",command, &rt , &offset, &rs);
        cout << command << " " << rt << " " << offset <<" "<< rs;

        // }
        cout << endl;
    }

    string op;
    sscanf(buf.c_str(), "%s $", op.c_str());
    cout << buf << endl;
    cout << op.c_str() << endl;

    newFile.close();

    return 0;
}