#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

using namespace std;

// 移除字串前後空白
string trim(const string& str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return (first == string::npos || last == string::npos) ? "" : str.substr(first, last - first + 1);
}

// 字串轉大寫
string to_upper_str(string str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

// 儲存中介檔每行資訊的結構
struct LineData {
    int loc;
    string label;
    string operation;
    string operand;
};

// 輔助函式：整數轉十六進位字串
string i2hex(int value, int width = 4) {
    stringstream ss;
    ss << setfill('0') << setw(width) << hex << uppercase << value;
    return ss.str();
}

// 檢查是否在 C' 或 c' 引號內部 (同 exer3)
bool in_c(string& str, int index) {
    bool c_quotes = false;
    for (int i = 0; i <= index; ++i) {
        if ((str[i] == 'C' || str[i] == 'c') && (i + 1 < str.size() && str[i + 1] == '\'') && !c_quotes) {
            c_quotes = true;
            str[i] = 'C';
            i++;  // 跳過單引號
        } else if (c_quotes && str[i] == '\'') {
            c_quotes = false;
        }
    }
    return c_quotes;
}

int main() {
    string code;
    // 打開輸出文件
    ofstream intfile("INTFILE");
    ofstream symtabfile("SYMTAB");

    // 檢查檔案是否成功開啟
    if (!intfile || !symtabfile) {
        cerr << "error opening output files" << endl;
        return 1;
    }

    // 符號表 (Symbol Table)
    unordered_map<string, int> symtab;
    // 暫存所有行的資訊，以便最後統一寫入 INTFILE
    vector<LineData> lines;

    int LOCCTR = 0;
    string program_name = "UNKNOWN";
    int start_address = 0;
    int program_length = 0;
    bool start_found = false;
    bool end_found = false;

    // 逐行讀取 stdin (來自 exer3 的輸出)
    while (getline(cin, code)) {
        if (code.empty() || code[0] == '.') continue; // 雖然 exer3 應該清掉了，但做個保險

        // 雖然 exer3 處理過了，但再次解析欄位
        // 這是因為 exer3 的輸出格式是固定的
        string label, operation, operand;
        stringstream ss(code);
        
        // 讀取第一個 token (可能是 Label 或 Opcode)
        string first_token;
        ss >> first_token;

        if (code[0] == ' ') {
            // 沒有 Label
            label = "";
            operation = first_token;
        } else {
            // 有 Label
            label = first_token;
            ss >> operation;
        }
        
        // 讀取剩餘部分作為 Operand
        string temp_operand;
        getline(ss, temp_operand);
        operand = trim(temp_operand);

        // 處理 START 指令
        if (operation == "START") {
            if (start_found) {
                cerr << "Error: Multiple START directives." << endl;
                return 1;
            }
            if (!operand.empty()) {
                stringstream ss_hex;
                ss_hex << hex << operand;
                ss_hex >> start_address;
                LOCCTR = start_address;
            } else {
                LOCCTR = 0;
            }
            program_name = label.empty() ? "UNKNOWN" : label;
            start_found = true;
            lines.push_back({LOCCTR, label, operation, operand});
            continue;
        }

        // 處理 END 指令
        if (operation == "END") {
            end_found = true;
            lines.push_back({LOCCTR, label, operation, operand});
            break; // Pass 1 結束
        }

        // 如果有標籤，將其加入符號表
        if (!label.empty()) {
            if (symtab.count(label)) {
                cerr << "Error: Duplicate symbol '" << label << "'" << endl;
                // 根據需求看是否要中斷
            }
            symtab[label] = LOCCTR;
        }

        // 將此行加入 INTFILE (暫存)
        lines.push_back({LOCCTR, label, operation, operand});

        // 根據 Operation 更新 LOCCTR
        if (operation == "WORD") {
            LOCCTR += 3;
        } else if (operation == "RESW") {
            LOCCTR += 3 * stoi(operand);
        } else if (operation == "RESB") {
            LOCCTR += stoi(operand);
        } else if (operation == "BYTE") {
            if (operand[0] == 'C') { // C'...'
                LOCCTR += operand.substr(2, operand.size() - 3).length();
            } else if (operand[0] == 'X') { // X'...'
                LOCCTR += (operand.substr(2, operand.size() - 3).length() + 1) / 2;
            }
        } else {
            // 假設所有其他指令長度為 3
            LOCCTR += 3;
        }
    }

    program_length = LOCCTR - start_address;

    // 寫入 INTFILE
    for (const auto& ld : lines) {
        intfile << i2hex(ld.loc, 6) << " " << left << setw(8) << ld.label << " ";
        if (!ld.operand.empty()) {
            intfile << setw(6) << ld.operation << "  " << ld.operand << "\n";
        } else {
            // 處理像 RSUB 這樣沒有 operand 的指令
            intfile << ld.operation << "\n";
        }
    }

    // 寫入 SYMTAB
    symtabfile << left << setw(6) << program_name << " "
               << i2hex(start_address, 6) << " "
               << i2hex(program_length, 6) << "\n";

    for (const auto& entry : symtab) {
        symtabfile << left << setw(6) << entry.first << " "
                   << i2hex(entry.second, 6) << "\n";
    }

    intfile.close();
    symtabfile.close();

    return 0;
}
