# SIC-ASSEMBLER

這是一個使用 C++ 實作的簡易版 SIC (Simplified Instructional Computer) 組譯器。此專案是根據課程練習 `exer3`, `exer4`, `exer5` 組合而成，展示了一個經典的 two-pass assembler 如何將 SIC 組合語言原始碼轉換為目的檔 (Object File)。

## 專案結構

這個組譯器由三個獨立的程式串聯而成，分別代表了前處理、Pass 1 和 Pass 2。

* **`exer3.cpp` (Normalizer / Splitter)**

  * **功能：** 組譯器的「前處理器」。

  * **輸入：** `stdin` (原始 SIC 組合語言)

  * **輸出：** `stdout` (正規化、已移除註解、固定欄位的組合語言)

  * **核心：** 負責清理原始碼，統一大小寫，並根據固定欄位 (Label, Opcode, Operand) 拆分指令，使 Pass 1 更容易解析。

* **`exer4.cpp` (Assembler Pass 1)**

  * **功能：** 組譯器的「階段一」。

  * **輸入：** `stdin` (來自 `exer3` 的正規化程式碼)

  * **輸出：** `SYMTAB` 和 `INTFILE`

  * **核心：** 建立符號表 (Symbol Table)，將所有 Label 與其記憶體位址 (LOCCTR) 對應起來。同時產生 `INTFILE`，其中包含每行原始碼及其分配到的位址。

* **`exer5.cpp` (Assembler Pass 2)**

  * **功能：** 組譯器的「階段二」。

  * **輸入：** `SYMTAB` 和 `INTFILE` (由 `exer4` 產生)

  * **輸出：** `stdout` (標準 SIC 格式的目的檔 H-T-E Records)

  * **核心：** 讀取 `INTFILE`，並利用 `SYMTAB` 將符號 (Label) 解析為實際位址，最終產生機器可讀的目的碼 (Object Code)。

## 如何編譯

你可以使用 `g++` 來編譯這三個程式。建議使用 `-o` 參數為它們指定有意義的執行檔名稱。

```bash
# 編譯 exer3 (Normalizer)
g++ -o my-split exer3.cpp

# 編譯 exer4 (Pass 1)
g++ -o pass1 exer4.cpp

# 編譯 exer5 (Pass 2)
g++ -o pass2 exer5.cpp
```

如何執行
這三個程式需要像pipeline一樣依序執行。

假設有一個 SIC 原始碼檔案叫做 my_program.asm：
```bash
# 步驟 1 & 2: 執行 Normalizer (my-split) 和 Pass 1 (pass1)
# my-split 會讀取 .asm 檔，並將其輸出透過 pipe 傳給 pass1
# pass1 執行完後會自動產生 SYMTAB 和 INTFILE 兩個檔案
cat my_program.asm | ./my-split | ./pass1

# 步驟 3: 執行 Pass 2 (pass2)
# pass2 會自動讀取 SYMTAB 和 INTFILE
# 它的輸出 (目的檔) 會顯示在畫面上，所以我們將它重導向到一個 .obj 檔案
./pass2 > my_program.obj
```
執行完畢後，你就會得到 my_program.obj，這就是可以被 SIC 模擬器 (simulator) 載入並執行的目的檔。
