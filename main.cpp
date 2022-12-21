#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;
/**
define pipeline register using struct
*/
struct IF_ID{
    string instruction = "";
    int line = 0;
    bool IF_ID_Write = true;
};

struct ID_EX{
    int RegDst = 0;
    int ALUSrc = 0;
    int Branch = 0;
    int MemRead = 0;
    int MemWrite = 0;
    int RegWrite = 0;
    int MemtoReg = 0;
    
    int ReadData1 = 0;
    int ReadData2 = 0;
    int SignExtend = 0;
    int rt = 0;
    int rd = 0;
};

struct EX_MEM{
    int Branch = 0;
    int MemRead = 0;
    int MemWrite = 0;
    int RegWrite = 0;
    int MemtoReg = 0;

    int Possible_PC = 0;
    int Zero = 0;
    int ALUResult = 0;
    int ReadData2 = 0;
    int TargetReg = 0;
};

struct MEM_WB{
    int RegWrite = 0;
    int MemtoReg = 0;

    int ReadData = 0;
    int ALUResult = 0;
    int TargetReg = 0;
};
/*
declare pipeline reg
*/
static IF_ID IF_ID_Reg;
static ID_EX ID_EX_Reg;
static EX_MEM EX_MEM_Reg;
static MEM_WB MEM_WB_Reg;

static int reg[32] = {0};
static int mem[32] = {0};

string insBuf = "";
bool PC_Write = true;

bool IF_Off = true;
bool ID_Off = true;
bool EX_Off= true;
bool MEM_Off = true;
bool WB_Off = true;
int stall = 0;

void initialization();
void IF(int &line);
void ID();
void EX();
void MEM();
void WB();

void initialization(){
    for (int i = 0; i < 32; i++){
        reg[i] = 1;
        mem[i] = 1;
    }
    reg[0] = 0;
}
/**
@param line is upcoming instruction to be fetched.
*/
void IF(int &line){
    string op;
    string instruction;
    
    fstream inFile;
    inFile.open("memory.txt",ios::in);
    for (int i = 1; i <= line; i++){
        getline(inFile, instruction);
    }
    inFile.close();
    
    //Get name of operation by format string.
    sscanf(instruction.c_str(), "%s $", op.c_str());

    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << op.c_str() << ":IF" << endl;
    outFile.close();
    
    /*
    stall condition, do not update PC.
    */
    if(!PC_Write){
        IF_Off = false;
        ID_Off = false;
        return;
    }

    /*
    Write to IF/ID Register.
    */
    IF_ID_Reg.instruction = instruction;
    IF_ID_Reg.line = line;

    /*
    Normal condition
    line = line + 1 , simulate as PC+4
    */
    line = line + 1;
    IF_Off = true;
    ID_Off = false;
}

void ID(){
    string op;
    int rs = 0;
    int rt = 0;
    int rd = 0;
    int imm = 0;
    if(IF_ID_Reg.instruction[0] == 'l'){
        sscanf(IF_ID_Reg.instruction.c_str(), "%s $%d, %d($%d)", op.c_str(), &rt, &imm, &rs);
        /*
        Write Control signal to ID/EX pipeline regitser
        */
        ID_EX_Reg.RegDst = 0;
        ID_EX_Reg.ALUSrc = 1;
        ID_EX_Reg.Branch = 0;
        ID_EX_Reg.MemRead = 1;
        ID_EX_Reg.MemWrite = 0;
        ID_EX_Reg.RegWrite = 1;
        ID_EX_Reg.MemtoReg = 1;
        /*
        store data source.
        */
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.SignExtend = imm/4;
    }
    else if(IF_ID_Reg.instruction[0] == 's'){
        sscanf(IF_ID_Reg.instruction.c_str(), "%s $%d, %d($%d)", op.c_str(), &rt, &imm, &rs);
        /*
        Write Control signal to ID/EX pipeline regitser
        */
        ID_EX_Reg.RegDst = 0;
        ID_EX_Reg.ALUSrc = 1;
        ID_EX_Reg.Branch = 0;
        ID_EX_Reg.MemRead = 0;
        ID_EX_Reg.MemWrite = 1;
        ID_EX_Reg.RegWrite = 0;
        ID_EX_Reg.MemtoReg = 0;
        /*
        store data source.
        */
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.SignExtend = imm/4;
    }
    /*
    R-format
    */
    else if(IF_ID_Reg.instruction[0] == 'a' || (IF_ID_Reg.instruction[0] == 's' || IF_ID_Reg.instruction[1] == 'u')){
        sscanf(IF_ID_Reg.instruction.c_str(), "%s $%d, $%d, $%d", op.c_str(), &rd, &rs, &rt);
        /*
        Write Control signal to ID/EX pipeline regitser
        */
        ID_EX_Reg.RegDst = 1;
        ID_EX_Reg.ALUSrc = 0;
        ID_EX_Reg.Branch = 0;
        ID_EX_Reg.MemRead = 0;
        ID_EX_Reg.MemWrite = 0;
        ID_EX_Reg.RegWrite = 1;
        ID_EX_Reg.MemtoReg = 0;
        /*
        store data source.
        */
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.ReadData2 = reg[rt];
    }
    /*
    Branch
    */
    else{
        sscanf(IF_ID_Reg.instruction.c_str(), "%s $%d, $%d, $%d", op.c_str(), &rs, &rt, &imm);
        /*
        Write Control signal to ID/EX pipeline regitser
        */
        ID_EX_Reg.RegDst = 0;
        ID_EX_Reg.ALUSrc = 0;
        ID_EX_Reg.Branch = 1;
        ID_EX_Reg.MemRead = 0;
        ID_EX_Reg.MemWrite = 0;
        ID_EX_Reg.RegWrite = 0;
        ID_EX_Reg.MemtoReg = 0;
        /*
        Store data source.
        */
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.ReadData2 = reg[rt];
    }

    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << op.c_str() << ":ID" << endl;
    outFile.close();

    /*
    keep ID stage.
    */
    if(stall > 0){
        ID_Off = false;
        return;
    }

    /*
    Hazard check
    All instruction hazard check except branch have same hazard check condition.
    Branch instruction hazard check needs to be handle in other way.
    */
    if(op[0] == 'b'){
        if((MEM_WB_Reg.TargetReg = rs || MEM_WB_Reg.TargetReg == rt) && MEM_WB_Reg.RegWrite == 1){
            /*
            set stall rounds and PC_Write to false.
            */
            stall = 2;
            PC_Write = false;
            /*
            Redeclare a new ID_EX_Reg to initialize ID_EX_Reg
            */
            ID_EX initial;
            ID_EX_Reg = initial;
        }
        else{
            /*
            Wait until branch outcome determined before fetching next instruction.
            */
            stall = 1;
            PC_Write = false;
        }
    }
    else{
         if((MEM_WB_Reg.TargetReg = rs || MEM_WB_Reg.TargetReg == rt) && MEM_WB_Reg.RegWrite == 1){
            stall = 2;
            ID_EX initial;
            ID_EX_Reg = initial;
        }
    }

    ID_Off = true;
    EX_Off = false;
}

int main(){
    /*
    @param nextIns is specified to line that will be fetched in next cycle.
    @param numOfIns record number of instruction in memory.
    @param buf is used as buffer.
    @param inFile is used as inputFilestream.
    */
    int nextIns = 1;
    int numOfIns = 0;
    int cycle = 1;
    string buf;
    fstream inFile;

    /*
    Initialization regitser and memory, and also get how many instruction will be
    executed.
    */
    initialization();

    inFile.open("memory.txt", ios::in);
    while(getline(inFile,buf)){
        numOfIns++;
    }
    buf = "";
    
    while(true){
        fstream outFile;
        outFile.open("result.txt",ios::out | ios::app);
        outFile << "Cycle: " << cycle << endl;
        outFile.close();

        if(stall == 0){
            PC_Write = true;
        }
        /*
        if(!WB_Off){
            WB();
        }

        if(!MEM_Off){
            MEM();
        }

        if(!EX_Off){
            EX();
        }
        */

        if(!ID_Off){
            ID();
        }

        if(nextIns == numOfIns + 1){
            IF_Off = true;
        }
        else{
            IF(nextIns);
        }
        stall--;

        if(IF_Off && ID_Off && EX_Off && MEM_Off && WB_Off){
            break;
        }
        cycle++;
    }

    return 0;
}