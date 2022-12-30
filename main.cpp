#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

void initialization();
void IF();
void ID();
void EX();
void MEM();
void WB();
/**
定義pipeline register結構
*/
struct IF_ID
{
    string instruction = "";
    int line = 0;
    bool IF_ID_Write = true;
};
struct ID_EX
{
    /*
    控制信號
    */
    int RegDst = 0;
    int ALUSrc = 0;
    int Branch = 0;
    int MemRead = 0;
    int MemWrite = 0;
    int RegWrite = 0;
    int MemtoReg = 0;

    /*
    進到下一個階段所需資料，ID_EX暫存器中原本應有的ALU op0 op1改用string op來替代
    */
    string op;
    int ReadData1 = 0;
    int ReadData2 = 0;
    int SignExtend = 0;
    int rt = 0;
    int rd = 0;
};

struct EX_MEM
{
    int Branch = 0;
    int MemRead = 0;
    int MemWrite = 0;
    int RegWrite = 0;
    int MemtoReg = 0;

    /*
    possibe_PC及zero在EX階段中皆在EX階段中進行，沒有被使用到
    */
    string op;
    int Possible_PC = 0;
    int Zero = 0;
    int ALUResult = 0;
    int ReadData2 = 0;
    int TargetReg = 0;
};

struct MEM_WB
{
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

/**
@param stall: 做為stall計數的變數
@param line : 指當前指令所在的行數，模擬MIPS Pipeline CPU中的PC
@param branch_outcome: 用來確定branch指令的計算結果是否已經出來
@param branch_equal: 判斷branch結果為equal or not equal
@param beq_stall: beq正常執行時需要的變數
*/
int stall = 0;
int line = 1;
bool PC_Write = true;
bool branch_outcome = false;
bool branch_equal = false;
bool beq_stall = false;

/*
對各個階段使用bool變數做為階段啟動的開關
*/
bool IF_Off = true;
bool ID_Off = true;
bool EX_Off = true;
bool MEM_Off = true;
bool WB_Off = true;

/*
初始化暫存器及記憶體
*/
void initialization()
{
    for (int i = 0; i < 32; i++)
    {
        reg[i] = 1;
        mem[i] = 1;
    }
    reg[0] = 0;
}

/*
IF階段實作
*/
void IF()
{
    string op;
    string instruction;
    /*
    指令讀取
    */
    fstream inFile;
    inFile.open("memory.txt", ios::in);
    for (int i = 1; i <= line; i++)
    {
        getline(inFile, instruction);
    }
    inFile.close();

    /*
    輸出到檔案
    */
    sscanf(instruction.c_str(), "%s $", op.c_str());
    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " << op.c_str() << ":IF" << endl;
    outFile.close();

    /*
    當遇到beq在ID階段正常decode的時候(beq的ID階段沒遇到stall)，則需要先將IF_ID_Reg值更新，
    使得當beq進入EX階段，計算出not branch的結果後，ID做decode的時候不會直接重複decode同
    一個指令。(作圖解釋later)
    */
    if (beq_stall)
    {
        IF_ID_Reg.instruction = instruction;
        beq_stall = false;
        IF_Off = true;
        ID_Off = false;
        return;
    }

    /*
    用來處理其他一般的stall狀況，也就是不更新PC及IF_ID暫存器，直接return。
    */
    if (!PC_Write)
    {
        IF_Off = false;
        ID_Off = false;
        return;
    }

    /*
    以下則為正常的IF階段會做的操作
    1.更新IF_ID暫存器
    2.PC=PC+4(即更新PC到下一行指令，同 line=line+1 )
    */
    IF_ID_Reg.instruction = instruction;
    line = line + 1;
    IF_Off = true;
    ID_Off = false;
}

void ID()
{

    /*
    這個判斷主要是在處理當beq指令進到EX階段後，ID階段所做的處理，因為當beq進到EX階段後，同cycle的
    ID階段可能會遇到以下兩種情況:
    1.branch結果出來，結果為equal，表示這個ID階段需要做stall，因為指令還沒IF。
    2.branch結果出來，結果為not equal，直接將IF_ID暫存器中存放的指令拿出來做decode。
    */

    if (branch_outcome && branch_equal)
    {
        branch_outcome = false;
        branch_equal = false;
        /*
        IF_ID initial;
        IF_ID_Reg = initial;
        */
        return;
    }

    /*
    各指令decode，並將各項參數傳給ID_EX暫存器。
    */

    string op;
    int rs = 0;
    int rt = 0;
    int rd = 0;
    int imm = 0;

    /*
    lw decode
    */
    if (IF_ID_Reg.instruction[0] == 'l')
    {
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

    /*
    sw decode
    */
    else if (IF_ID_Reg.instruction[0] == 's' && IF_ID_Reg.instruction[1] == 'w')
    {
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
    R-format decode
    */
    else if (IF_ID_Reg.instruction[0] == 'a' || (IF_ID_Reg.instruction[0] == 's' && IF_ID_Reg.instruction[1] == 'u'))
    {
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
    Branch decode
    */
    else
    {
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
        ID_EX_Reg.ReadData1 = reg[rs];
        ID_EX_Reg.ReadData2 = reg[rt];
        ID_EX_Reg.SignExtend = imm;
        ID_EX_Reg.rt = rt;
        ID_EX_Reg.rd = rd;
    }

    /*
    Output to file.
    */
    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " << string(op.c_str()) << ":ID" << endl;
    outFile.close();

    /*
    檢查ID階段是不是還在stall，將暫存器ID_EX Reg的值全部更新為0，後面的EX,MEM,WB stage做NOP。
    */
    if (stall > 0)
    {   
        ID_Off = false;
        ID_EX initial;
        ID_EX_Reg = initial;

        return;
    }

    /*
    Hazard check
    All instruction hazard check except branch have same hazard check condition.
    Branch instruction hazard check needs to be handled in other way.

    判斷現在指令有沒有跟前指令衝突
    若有衝突表示需要stall，清空暫存器，令後面階段做NOP。
    而因為需要等到前指令在WB階段將值寫回暫存器後，才可以讀到正確的值，所以至少需要等待兩個cycle(EX,MEM stage)，
    也就是下方程式碼將stall設為2的原因。
    */
    if (op.c_str()[0] == 'b')
    {
        if ((EX_MEM_Reg.TargetReg == rs || EX_MEM_Reg.TargetReg == rt) && EX_MEM_Reg.RegWrite == 1)
        {
            stall = 2;
            PC_Write = false;

            /*
            Clear ID_EX_Reg
            */
            ID_EX initial;
            ID_EX_Reg = initial;
        }
        else
        {
            /*
            Wait until branch outcome determined before fetching next instruction.
            */

            beq_stall = true;
            /*
            stall = 1;
            PC_Write = false;
            */
        }
    }
    else
    {
        if ((EX_MEM_Reg.TargetReg == rs || EX_MEM_Reg.TargetReg == rt) && EX_MEM_Reg.RegWrite == 1)
        {
            stall = 2;
            PC_Write = false;

            ID_EX initial;
            ID_EX_Reg = initial;
        }
    }

    /*
    ID階段結束，EX階段開始。
    */
    ID_Off = true;
    EX_Off = false;
}

void EX()
{
    /*
    用來判斷是否做NOP，若ID_EX暫存器中的控制信號皆為0的時候，表示需做nop，所以接下來的
    EX運算、MEM階段、WB階段都不需要執行了，所以直接return。
    */
    if (!ID_EX_Reg.RegDst && !ID_EX_Reg.ALUSrc && !ID_EX_Reg.Branch && !ID_EX_Reg.MemRead &&
        !ID_EX_Reg.MemWrite && !ID_EX_Reg.RegWrite && !ID_EX_Reg.MemtoReg)
    {
        return;
    }

    /*
    檔案輸入
    */
    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " << ID_EX_Reg.op << ":EX ";
    if (ID_EX_Reg.RegWrite)
    {
        outFile << ID_EX_Reg.RegDst << ID_EX_Reg.ALUSrc << " " << ID_EX_Reg.Branch
                << ID_EX_Reg.MemRead << ID_EX_Reg.MemWrite << " " << ID_EX_Reg.RegWrite << ID_EX_Reg.MemtoReg << endl;
    }
    else
    {
        outFile << "X" << ID_EX_Reg.ALUSrc << " " << ID_EX_Reg.Branch
                << ID_EX_Reg.MemRead << ID_EX_Reg.MemWrite << " " << ID_EX_Reg.RegWrite << "X" << endl;
    }
    outFile.close();

    /*
    下面的部分依照傳過來的op決定ALU應該要做那些運算。
    */

    /*
    lw及sw因為在EX階段做的事情都是計算要存取/寫入的記憶體位址，所以位址計算放在同一個if block中。
    */
    if (ID_EX_Reg.op[0] == 'l' || (ID_EX_Reg.op[0] == 's' && ID_EX_Reg.op[1] == 'w'))
    {
        int MEM_Addr = ID_EX_Reg.ReadData1 + ID_EX_Reg.SignExtend / 4;
        EX_MEM_Reg.ALUResult = MEM_Addr;
    }

    /*
    add將傳過來的ReadData做相加運算
    */
    else if (ID_EX_Reg.op[0] == 'a')
    {
        int result = ID_EX_Reg.ReadData1 + ID_EX_Reg.ReadData2;

        EX_MEM_Reg.ALUResult = result;
    }

    /*
    sub將傳過來的ReadData做相減運算
    */
    else if (ID_EX_Reg.op[0] == 's' && ID_EX_Reg.op[1] == 'u')
    {
        int result = ID_EX_Reg.ReadData1 - ID_EX_Reg.ReadData2;

        EX_MEM_Reg.ALUResult = result;
    }

    /*
    branch藉由減法判斷ReadData是否equal，equal則更新PC，not equal則PC=PC+4(在這支程式中為line=line+1);
    */
    else if (ID_EX_Reg.op[0] == 'b')
    {
        int result = ID_EX_Reg.ReadData1 - ID_EX_Reg.ReadData2;

        /*
        if result equal zero,branch.
        */
        if (result == 0)
        {

            line = line + ID_EX_Reg.SignExtend;
            branch_equal = true;
        }
        /*
        not equal
        */
        else
        {
            line = line + 1;
        }

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

    /*
    ID_EX initial;
    ID_EX_Reg = initial;
    */

    EX_Off = true;
    MEM_Off = false;
}

void MEM()
{

    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " << EX_MEM_Reg.op << ":MEM ";
    if (EX_MEM_Reg.RegWrite)
    {
        outFile << EX_MEM_Reg.Branch << EX_MEM_Reg.MemRead << EX_MEM_Reg.MemWrite
                << " " << EX_MEM_Reg.RegWrite << EX_MEM_Reg.MemtoReg << endl;
    }
    else
    {
        outFile << EX_MEM_Reg.Branch << EX_MEM_Reg.MemRead << EX_MEM_Reg.MemWrite
                << " " << EX_MEM_Reg.RegWrite << "X" << endl;
    }
    outFile.close();

    // load from memory
    if (EX_MEM_Reg.MemRead)
    {
        MEM_WB_Reg.ReadData = mem[EX_MEM_Reg.ALUResult];
    }
    // store to memory
    if (EX_MEM_Reg.MemWrite)
    {
        mem[EX_MEM_Reg.ALUResult] = EX_MEM_Reg.ReadData2;
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

void WB()
{
    fstream outFile;
    outFile.open("result.txt", ios::out | ios::app);
    outFile << "    " << MEM_WB_Reg.op << ":WB ";
    if (MEM_WB_Reg.RegWrite)
    {
        outFile << MEM_WB_Reg.RegWrite << MEM_WB_Reg.MemtoReg << endl;
        /*
        Write back to register
        */
        if (!MEM_WB_Reg.MemtoReg)
        {
            reg[MEM_WB_Reg.TargetReg] = MEM_WB_Reg.ALUResult;
        }
        else
        {
            reg[MEM_WB_Reg.TargetReg] = MEM_WB_Reg.ReadData;
        }
    }
    else
    {
        outFile << MEM_WB_Reg.RegWrite << "X" << endl;
    }
    outFile.close();

    /*
    MEM_WB initial;
    MEM_WB_Reg = initial;    
    */

    WB_Off = true;
}

int main()
{
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
    while (getline(inFile, buf))
    {
        numOfIns++;
    }
    buf = "";

    while (true)
    {
        fstream outFile;
        outFile.open("result.txt", ios::out | ios::app);
        outFile << "Cycle: " << cycle << endl;
        outFile.close();

        if (stall == 0)
        {
            PC_Write = true;
        }

        if (!WB_Off)
        {
            WB();
        }

        if (!MEM_Off)
        {
            MEM();
        }

        if (!EX_Off)
        {
            EX();
        }

        if (!ID_Off || stall > 0)
        {
            ID();
        }

        if (line == numOfIns + 1)
        {
            IF_Off = true;
        }
        else
        {
            IF();
        }

        stall--;

        if (IF_Off && ID_Off && EX_Off && MEM_Off && WB_Off)
        {
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

    for (int i = 0; i < 32; i++)
    {
        outFile << "$" << i << " ";
    }
    outFile << endl;
    for (int i = 0; i < 32; i++)
    {
        outFile << reg[i] << "  ";
    }
    outFile << endl;
    for (int i = 0; i < 31; i++)
    {
        outFile << "W" << i << " ";
    }
    outFile << endl;
    for (int i = 0; i < 32; i++)
    {
        outFile << mem[i] << "  ";
    }

    return 0;
}