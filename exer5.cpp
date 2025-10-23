#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <iomanip>
#include <set>
#include <string>

using namespace std;

// 操作碼表 (opcodeTable)
map<string, string> opcodeTable = {
    {"LDA", "00"}, {"LDL", "08"}, {"STA", "0C"}, {"ADD", "18"},
    {"LDX", "04"}, {"SUB", "1C"}, {"MUL", "20"}, {"DIV", "24"},
    {"COMP", "28"}, {"J", "3C"}, {"JLT", "38"}, {"JEQ", "30"},
    {"JGT", "34"}, {"JSUB", "48"}, {"RSUB", "4C"}, {"LDCH", "50"},
    {"STCH", "54"}, {"STX", "10"}, {"STL", "14"}, {"TIX", "2C"},
    {"RD", "D8"}, {"WD", "DC"}, {"TD", "E0"}
};

// 符號表 (從 SYMTAB 載入)
map<string, int> symbolTable;
vector<string> textRecords;  // 儲存 Text Record
int programStart = 0;        // 程式開始地址
int programEnd = 0;          // 程式結束地址
string codename;             // 程式名

// 將整數轉換為指定寬度的十六進位字串
string intToHex(int value, int width) {
    stringstream ss;
    ss << setw(width) << setfill('0') << hex << uppercase << value;
    return ss.str();
}

// 從 SYMTAB 檔案中讀取符號表
void loadSYMTAB(const string& filename){
    ifstream symtab_file(filename);
    if (!symtab_file) {
        cerr << "Error: Could not open SYMTAB file.\n";
        exit(1);
    }
    string line;
    // 讀取第一行，獲取程式名、起始地址和程式长度
    getline(symtab_file, line);
    istringstream iss(line);
    string staddr_str, codelen_str;
    iss >> codename >> staddr_str >> codelen_str;
    // 將起始地址和程式長度轉換為整數
    int staddr = stoi(staddr_str, nullptr, 16);
    int codelen = stoi(codelen_str, nullptr, 16);
    programStart = staddr;
    programEnd = staddr + codelen;

    // 讀取符號表
    string label, address_str;
    while(symtab_file >> label >> address_str){
        int address = stoi(address_str, nullptr, 16);
        symbolTable[label] = address;
    }
    symtab_file.close();
}

// 移除字串前後空白
string trim(const string& str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return (first == string::npos || last == string::npos) ? "" : str.substr(first, last - first + 1);
}


// 生成對象碼
string generateObjectCode(const string& opcode, const string& operand) {
    string objectCode;
    if (opcode == "WORD") {
        int value = stoi(operand);
        objectCode = intToHex(value, 6);
    } else if (opcode == "BYTE") {
        if (operand[0] == 'C' && operand[1] == '\'' && operand.back() == '\'') {
            string chars = operand.substr(2, operand.size() - 3);
            stringstream ss;
            for (char c : chars) {
                ss << setw(2) << setfill('0') << hex << uppercase << (int)c;
            }
            objectCode = ss.str();
        } else if (operand[0] == 'X' && operand[1] == '\'' && operand.back() == '\'') {
            objectCode = operand.substr(2, operand.size() - 3);
            // 確保是偶數長度，若需要可補0 (根據 SIC 規格)
            if (objectCode.length() % 2 != 0) {
                objectCode = "0" + objectCode;
            }
        } else {
            cerr << "Error: Invalid BYTE operand '" << operand << "'\n";
        }
    } else if (opcodeTable.find(opcode) != opcodeTable.end()) {
        objectCode = opcodeTable[opcode];
        int address = 0;

        if (!operand.empty()) {
            bool indexed = false;
            string tempOperand = operand;

            // 檢查是否為索引定址 ,X
            if (tempOperand.size() > 2 && tempOperand.substr(tempOperand.size() - 2) == ",X") {
                indexed = true;
                tempOperand = tempOperand.substr(0, tempOperand.size() - 2);
            }

            if (symbolTable.find(tempOperand) != symbolTable.end()) {
                address = symbolTable[tempOperand];
                if (indexed) {
                    address |= 0x8000;  // 設置索引位 (X bit)
                }
            } else {
                cerr << "Error: Undefined symbol '" << tempOperand << "'\n";
                address = 0; // 錯誤處理
            }
        } else if (opcode == "RSUB") {
            address = 0; // RSUB 的 operand address 為 0
        }
        // 其它需要 operand 但 operand 為空的指令 (錯誤)
        else if (!operand.empty()) {
             cerr << "Error: Missing operand for " << opcode << "\n";
             address = 0;
        }

        objectCode += intToHex(address, 4);
    } else {
        // 對於 RESW、RESB、START、END 等指令，不生成對象碼
        objectCode = "";
    }
    return objectCode;
}

// 添加 Text Record
void addTextRecord(int startAddress, const string& objectCodes) {
    if (objectCodes.empty()) return;
    int length = objectCodes.size() / 2; // 長度 (bytes)
    stringstream ss;
    ss << "T" << intToHex(startAddress, 6)
       << intToHex(length, 2)
       << objectCodes;
    textRecords.push_back(ss.str());
}

// 輔助函式：解析 INTFILE 的行
void parseIntfileLine(const string& line, string& locStr, string& label, string& opcode, string& operand) {
    locStr = line.substr(0, 6);
    string rest = trim(line.substr(6));

    stringstream ss(rest);
    string first, second, third;
    
    // 讀取第一個 token (可能是 Label 或 Opcode)
    ss >> first;
    
    if (rest[0] == ' ') {
        // 沒有 Label
        label = "";
        opcode = first;
    } else {
        // 有 Label
        label = first;
        ss >> second;
        opcode = second;
    }

    // 讀取剩餘部分作為 Operand
    string temp_operand;
    if (getline(ss, temp_operand)) {
        operand = trim(temp_operand);
    } else {
        operand = "";
    }

    // 特殊處理 RSUB (pass1 可能會漏掉 operand 欄位)
    if (opcode == "RSUB") {
        operand = "";
    }
    // 處理 END (可能有 operand)
    else if (opcode == "END") {
        if (label.empty()) { // 格式： LOC END OPERAND
            operand = first;
            opcode = "END";
        } else { // 格式： LOC LABEL END OPERAND
             operand = second;
        }
        operand = trim(operand);
    }
}


// 處理 INTFILE
void process_INTFILE(const string& filename) {
    ifstream infile(filename);
    if (!infile) {
        cerr << "Error: Could not open INTFILE file.\n";
        exit(1);
    }

    string line;
    string currentObjectCodes;
    int recordStartAddress = -1;
    int firstExecAddr = programStart; // 預設 E record 為程式起始地址

    while (getline(infile, line)) {
        if (line.empty()) continue;

        string locStr, label, opcode, operand;
        // 使用更穩健的行解析
        locStr = line.substr(0, 6);
        string rest = trim(line.substr(6));
        
        stringstream ss(rest);
        string token1, token2;
        
        ss >> token1; // 可能是 Label 或 Opcode
        ss >> token2; // 可能是 Opcode 或 Operand
        
        string restOfLine;
        getline(ss, restOfLine);
        restOfLine = trim(restOfLine);

        if (rest[0] != ' ') {
            // 有 Label
            label = token1;
            opcode = token2;
            operand = restOfLine;
        } else {
            // 沒有 Label
            label = "";
            opcode = token1;
            if (opcode == "RSUB" || opcode == "END") {
                 operand = token2; // END CSEC 或 RSUB
            } else {
                 operand = token2 + (restOfLine.empty() ? "" : " " + restOfLine);
            }
        }
        
        // 再次修剪 operand
        operand = trim(operand);
        
        // 處理 RSUB
        if(opcode == "RSUB") operand = "";


        int loc = stoi(locStr, nullptr, 16);

        if (opcode == "START") {
            recordStartAddress = loc; // 第一個 T-Record 的起始地址
            continue;
        }
        
        if (opcode == "END") {
            if (!operand.empty() && symbolTable.count(operand)) {
                firstExecAddr = symbolTable[operand];
            } else {
                firstExecAddr = programStart;
            }
            break; // 結束處理
        }

        if (opcode == "RESW" || opcode == "RESB") {
            // 遇到保留空間，結束當前的 T-Record
            if (!currentObjectCodes.empty()) {
                addTextRecord(recordStartAddress, currentObjectCodes);
                currentObjectCodes = "";
                recordStartAddress = -1;
            }
            continue;
        }

        string objCode = generateObjectCode(opcode, operand);

        if (!objCode.empty()) {
            if (recordStartAddress == -1) {
                recordStartAddress = loc; // 新 T-Record 的起始
            }

            // 檢查 T-Record 是否已滿 (30 bytes = 60 chars)
            if (currentObjectCodes.size() + objCode.size() > 60) {
                addTextRecord(recordStartAddress, currentObjectCodes);
                currentObjectCodes = objCode;
                recordStartAddress = loc;
            } else {
                currentObjectCodes += objCode;
            }
        }
    }
    infile.close();

    // 處理最後一筆 T-Record
    if (!currentObjectCodes.empty()) {
        addTextRecord(recordStartAddress, currentObjectCodes);
    }
    
    // 設定 E Record 的地址
    programStart = firstExecAddr;
}

// 生成物件檔 (Object File)
void generateObjectFile() {
    int programLength = programEnd - programStart;
    
    // 取得真正的起始地址 (SYMTAB 的第一行)
    ifstream symtab_file("SYMTAB");
    string line;
    getline(symtab_file, line);
    istringstream iss(line);
    string staddr_str, codelen_str;
    iss >> codename >> staddr_str >> codelen_str;
    int actualStart = stoi(staddr_str, nullptr, 16);
    int actualLength = stoi(codelen_str, nullptr, 16);
    symtab_file.close();


    // 輸出 H Record
    cout << "H" << setw(6) << left << setfill(' ') << codename.substr(0, 6)
         << intToHex(actualStart, 6)
         << intToHex(actualLength, 6) << endl;

    // 輸出 T Records
    for (const auto& record : textRecords) {
        cout << record << endl;
    }

    // 輸出 E Record
    // E Record 使用的是 Pass 1 在 END 指令上指定的地址，或預設的程式起始地址
    cout << "E" << intToHex(programStart, 6) << endl;
}

int main() {
    loadSYMTAB("SYMTAB");         // 從 SYMTAB 文件加載符號表
    process_INTFILE("INTFILE");   // 處理 INTFILE 並生成對象碼
    generateObjectFile();         // 生成物件檔 (輸出到 stdout)
    return 0;
}
