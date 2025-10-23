#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>

using namespace std;

// 移除字串前後空白
string trim(const string& str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return (first == string::npos || last == string::npos) ? "" : str.substr(first, last - first + 1);
}

// 檢查是否在 C'...' 字元常數引號內部
bool in_c(string& str, int index) {
    bool c_quotes = false;
    for (int i = 0; i <= index; ++i) {
        // 檢查 C' 或 c' 引號的起點
        if ((str[i] == 'C' || str[i] == 'c') && (i + 1 < str.size() && str[i + 1] == '\'') && !c_quotes) {
            c_quotes = true;
            str[i] = 'C'; // 統一將 c' 轉為 C'
            i++;  // 跳過單引號
        }
        else if (c_quotes && str[i] == '\'') {
            c_quotes = false;
        }
    }
    return c_quotes;
}

int main() {
    string code;
    // 逐行讀取 stdin
    while (getline(cin, code)) {
        // 忽略空行或註解行
        if (code.empty() || code[0] == '.') continue;

        // 根據 SIC 格式，只處理到第 35 欄位 (忽略註解)
        if (code.length() > 35) {
            code = code.substr(0, 35);
        }

        string label, operation, operand;

        // 解析 Label (第 1-8 字節)
        if (code.length() >= 8 && code.substr(0, 8).find_first_not_of(' ') != string::npos) {
            label = trim(code.substr(0, 8));
        } else {
            label = "";  // 沒有標籤，設為空字串
        }

        // 解析 Operation (第 10-15 字節)
        if (code.length() >= 15 && code.substr(9, 6).find_first_not_of(' ') != string::npos) {
            operation = trim(code.substr(9, 6));
        }
        // 處理 Operation 欄位不足但有內容的情況
        else if (code.length() >= 9 && code.substr(9).find_first_not_of(' ') != string::npos) {
            operation = trim(code.substr(9));
        } else {
            operation = "";
        }

        // 處理 Operand (第 18-35 字節)
        if (code.length() > 17) {
            operand = trim(code.substr(17));
        } else {
            operand = "";  // 沒有 operand
        }

        // 轉換 Operand，保留 C'...' 內部
        for (int i = 0; i < operand.size(); ++i) {
            if (in_c(operand, i)) {
                // 引號內部字符已經處理(in_c會順便轉大寫C)，不進行其他處理
            } else {
                operand[i] = toupper(operand[i]);  // 將非引號內的字符轉為大寫
            }
        }

        // 將 Label 和 Operation 轉為大寫
        transform(label.begin(), label.end(), label.begin(), ::toupper);
        transform(operation.begin(), operation.end(), operation.begin(), ::toupper);

        // 根據正規化格式輸出到 stdout
        if (!operand.empty()) {
            cout << left << setw(8) << label << ' '
                 << setw(6) << operation << "  " << operand << '\n';
        }
        else{
            cout << left << setw(8) << label << ' '
                 << operation << '\n';
        }
    }

    return 0;
}
