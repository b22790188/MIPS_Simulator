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

    string op;
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

    string op;
    int Possible_PC = 0;
    int Zero = 0;
    int ALUResult = 0;
    int ReadData2 = 0;
    int TargetReg = 0;
};

struct MEM_WB{
    int RegWrite = 0;
    int MemtoReg = 0;

    string op;
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

string insbuf;
int stall = 0;
int line = 1;
bool PC_Write = true;
bool branch_outcome = false;
bool branch_equal = false;
bool beq_stall = false;

bool IF_Off = true;
bool ID_Off = true;
bool EX_Off= true;
bool MEM_Off = true;
bool WB_Off = true;

void initialization();
void IF();
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
void IF(){
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
    outFile << "    " <<op.c_str() << ":IF" << endl;
    outFile.close();
    
    /*
    當遇到beq正常stall的時候，需要先將暫存器值更新，使得beq在EX階段
    計算出not branch的結果時，ID做decode的時候不會直接重複decode同
    一個指令。
    */
    if(beq_stall){
        IF_ID_Reg.instruction = instruction;
        beq_stall = false;
        IF_Off = true;
        ID_Off = false;
        return;
    }
    /*
    stall condition, do not update PC and IF_ID_Reg.
    */

    if(!PC_Write){
        IF_Off = false;
        ID_Off = false;
        /*
        here
        */        
        return;
    }


    /*
    Normal condition
    Write to IF/ID Register.
    */
    IF_ID_Reg.instruction = instruction;

    /*
    line = line + 1 , simulate as PC+4
    */
    line = line + 1;
    IF_Off = true;
    ID_Off = false;
}

void ID(){
    /*
    if branch outcome has been confirmed, redo IF stage instead of passing 
    instrunction to ID stage.
    */

    /*
    branch結果出來後，branch equal不進ID階段 , not equal直接decode
    */
    if(branch_outcome && branch_equal){
        branch_outcome = false;
        branch_equal = false;
        /*
        IF_ID initial;
        IF_ID_Reg = initial;
        */
        return;
    }


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
        ID_EX_Reg.op = string(op.c_str());
        /*
        store data source.
        */
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.ReadData2 = reg[rt];
        ID_EX_Reg.SignExtend = imm;
        ID_EX_Reg.rt = rt;
        ID_EX_Reg.rd = rd;
    }
    else if(IF_ID_Reg.instruction[0] == 's' && IF_ID_Reg.instruction[1] == 'w'){
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
        ID_EX_Reg.op = string(op.c_str());
        /*
        store data source.
        */
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.ReadData2 = reg[rt];
        ID_EX_Reg.SignExtend = imm;
        ID_EX_Reg.rt = rt;
        ID_EX_Reg.rd = rd;
    }
    /*
    R-format
    */
    else if(IF_ID_Reg.instruction[0] == 'a' || (IF_ID_Reg.instruction[0] == 's' && IF_ID_Reg.instruction[1] == 'u')){
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
        ID_EX_Reg.op = string(op.c_str());
        /*
        store data source.
        */
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.ReadData2 = reg[rt];
        ID_EX_Reg.rt = rt;
        ID_EX_Reg.rd = rd;
    }
    /*
    Branch
    */
    else{
        sscanf(IF_ID_Reg.instruction.c_str(), "%s $%d, $%d, %d", op.c_str(), &rs, &rt, &imm);
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
        ID_EX_Reg.op = string(op.c_str());

        /*
        Store data source.
        */
        cout << reg[rs] << " " << reg[rt] << endl;//======================================
        cout << rs << " " << rt << endl;//==============================================
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.ReadData2 = reg[rt];
        ID_EX_Reg.SignExtend = imm;
        ID_EX_Reg.rt = rt;
        ID_EX_Reg.rd = rd;

    }
    cout << ID_EX_Reg.op << endl;//=================================================
    /*
    Output to file.
    */
    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " <<string(op.c_str()) << ":ID" << endl;
    outFile.close();

    /*
    keep ID stage.
    */
    if(stall > 0){
        ID_Off = false;
        ID_EX initial;
        ID_EX_Reg = initial;

        return;
    }

    /*
    Hazard check
    All instruction hazard check except branch have same hazard check condition.
    Branch instruction hazard check needs to be handled in other way.
    */
    if(op.c_str()[0] == 'b'){
        if((EX_MEM_Reg.TargetReg == rs || EX_MEM_Reg.TargetReg == rt) && EX_MEM_Reg.RegWrite == 1){
            stall = 2;
            PC_Write = false;
            /*
            Clear ID_EX_Reg
            */
            ID_EX initial;
            ID_EX_Reg = initial;
        }
        else{
            /*
            Wait until branch outcome determined before fetching next instruction.
            */

            /*
            當收到beq指令ID 時(非stall)，使得beq stall為true，令PC可以寫入
            */

            beq_stall = true;
            /*

            stall = 1;
            PC_Write = false;
            */
        } 
    }
    else{
        
        if((EX_MEM_Reg.TargetReg == rs || EX_MEM_Reg.TargetReg == rt) && EX_MEM_Reg.RegWrite == 1){
            stall = 2;
            PC_Write = false;

            ID_EX initial;
            ID_EX_Reg = initial;
            cout << "test" << endl;//=========================================================
        }
    }
    ID_Off = true;
    EX_Off = false;
}

void EX(){
    
    if(!ID_EX_Reg.RegDst && !ID_EX_Reg.ALUSrc && !ID_EX_Reg.Branch && !ID_EX_Reg.MemRead &&
    !ID_EX_Reg.MemWrite && !ID_EX_Reg.RegWrite && !ID_EX_Reg.MemtoReg){
        return;
    }

    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " << ID_EX_Reg.op << ":EX ";
    if(ID_EX_Reg.RegWrite){
        outFile << ID_EX_Reg.RegDst << ID_EX_Reg.ALUSrc << " " << ID_EX_Reg.Branch
            << ID_EX_Reg.MemRead << ID_EX_Reg.MemWrite << " " << ID_EX_Reg.RegWrite << ID_EX_Reg.MemtoReg << endl;
    }
    else{
        outFile << "X" << ID_EX_Reg.ALUSrc << " " << ID_EX_Reg.Branch
            << ID_EX_Reg.MemRead << ID_EX_Reg.MemWrite << " " << ID_EX_Reg.RegWrite << "X" << endl;
    }
    outFile.close();
    

    if(ID_EX_Reg.op[0] == 'l' || (ID_EX_Reg.op[0] == 's' && ID_EX_Reg.op[1] == 'w')){
        int MEM_Addr = ID_EX_Reg.ReadData1 + ID_EX_Reg.SignExtend / 4;
        cout << "lw or sw EX" << endl;
        EX_MEM_Reg.ALUResult = MEM_Addr;
    }
    else if(ID_EX_Reg.op[0] == 'a'){
        int result = ID_EX_Reg.ReadData1 + ID_EX_Reg.ReadData2;

        EX_MEM_Reg.ALUResult = result;
    }
    else if(ID_EX_Reg.op[0] == 's' && ID_EX_Reg.op[1] == 'u'){
        int result = ID_EX_Reg.ReadData1 - ID_EX_Reg.ReadData2;

        EX_MEM_Reg.ALUResult = result;
    }
    else if(ID_EX_Reg.op[0] == 'b'){
        int result = ID_EX_Reg.ReadData1 - ID_EX_Reg.ReadData2;
        /*
        if result equal zero, need branch.
        */
        cout << ID_EX_Reg.ReadData1 << endl;
        cout << ID_EX_Reg.ReadData2 << endl;
        cout << result << endl; //=============================================
        cout << ID_EX_Reg.SignExtend << endl;//=========================================
        if(result == 0){
    
            line = line + ID_EX_Reg.SignExtend;
            branch_equal = true;
        }
        
        else{
            line = line + 1;
        }
        
        /*
        */
        branch_outcome = true;
    }

    /*
    Pass control signal to EX/MEM register.
    */

    EX_MEM_Reg.Branch = ID_EX_Reg.Branch;
    EX_MEM_Reg.MemRead = ID_EX_Reg.MemRead;
    EX_MEM_Reg.MemWrite = ID_EX_Reg.MemWrite;
    EX_MEM_Reg.RegWrite = ID_EX_Reg.RegWrite;
    EX_MEM_Reg.MemtoReg = ID_EX_Reg.MemtoReg;

    EX_MEM_Reg.op = ID_EX_Reg.op;
    EX_MEM_Reg.ReadData2 = ID_EX_Reg.ReadData2;
    EX_MEM_Reg.TargetReg = (ID_EX_Reg.RegDst) ? ID_EX_Reg.rd : ID_EX_Reg.rt;
    cout << "ID:readdata" << ID_EX_Reg.ReadData2 << endl;//================================================

    ID_EX initial;
    ID_EX_Reg = initial;
    

    EX_Off = true;
    MEM_Off = false;
}

void MEM(){
    
    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " << EX_MEM_Reg.op << ":MEM ";
    if(EX_MEM_Reg.RegWrite){
        outFile << EX_MEM_Reg.Branch << EX_MEM_Reg.MemRead << EX_MEM_Reg.MemWrite 
                << " " << EX_MEM_Reg.RegWrite << EX_MEM_Reg.MemtoReg << endl;
    }
    else{
        outFile << EX_MEM_Reg.Branch << EX_MEM_Reg.MemRead << EX_MEM_Reg.MemWrite 
                << " " << EX_MEM_Reg.RegWrite << "X" << endl;
    }
    outFile.close();

    //load from memory
    if(EX_MEM_Reg.MemRead){
        MEM_WB_Reg.ReadData = mem[EX_MEM_Reg.ALUResult];
    }
    //store to memory
    if(EX_MEM_Reg.MemWrite){
        mem[EX_MEM_Reg.ALUResult] = EX_MEM_Reg.ReadData2;
        cout << "EX readdata" << EX_MEM_Reg.ReadData2 << endl;//==============================================================
    }

    MEM_WB_Reg.op = EX_MEM_Reg.op;
    MEM_WB_Reg.RegWrite = EX_MEM_Reg.RegWrite;
    MEM_WB_Reg.MemtoReg = EX_MEM_Reg.MemtoReg;
    MEM_WB_Reg.ALUResult = EX_MEM_Reg.ALUResult;
    MEM_WB_Reg.TargetReg = EX_MEM_Reg.TargetReg;

    
    EX_MEM initial;
    EX_MEM_Reg = initial;
    

    MEM_Off = true;
    WB_Off = false;
}

void WB(){
    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " << MEM_WB_Reg.op << ":WB ";
    if(MEM_WB_Reg.RegWrite){
        outFile << MEM_WB_Reg.RegWrite << MEM_WB_Reg.MemtoReg << endl;
        /*
        Write back to register
        */
        if(!MEM_WB_Reg.MemtoReg){
            reg[MEM_WB_Reg.TargetReg] = MEM_WB_Reg.ALUResult;
        }
        else{
            reg[MEM_WB_Reg.TargetReg] = MEM_WB_Reg.ReadData;
        }

    }
    else{
        outFile << MEM_WB_Reg.RegWrite << "X" << endl;
    }
    outFile.close();

    
    MEM_WB initial;
    MEM_WB_Reg = initial;
    

    WB_Off = true;
}

int main(){
    /*
    @param numOfIns specified to number of instruction in memory.
    @param buf is used as buffer.
    @param inFile is used as inputFilestream.
    */
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
    
        if(!WB_Off){
            WB();
        }
        
        if(!MEM_Off){
            MEM();
        }

        if(!EX_Off){
            EX();
        }

        if(!ID_Off || stall > 0){
            ID();
        }

        if(line == numOfIns + 1){
            IF_Off = true;
        }
        else{
            IF();
        }

        stall--;

        if(IF_Off && ID_Off && EX_Off && MEM_Off && WB_Off){
            break;
        }
        cycle++;
    }

    /*
    reg and mem output
    */
    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "\n\n";
    outFile << "需要花" << cycle << "個cycle" << endl;

    for (int i = 0; i < 32;i ++){
        outFile << "$" << i << " ";
    }
    outFile << endl;
    for (int i = 0; i < 32;i ++){
        outFile << reg[i] << "  ";
    }
    outFile << endl;
    for (int i = 0; i < 31;i ++){
        outFile << "W" << i << " ";
    }
    outFile << endl;
    for (int i = 0; i < 32;i ++){
        outFile << mem[i] << "  ";
    }

    return 0;
}