/*
 * ============================================================
 *  Virtual Basic Operating System Simulator - Phase 1 (C++ Version)
 *  Multiprogramming Operating System (MOS)
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <cstring>

class VirtualOS {
private:
    // CPU Registers
    char R[4];          // General Purpose Register (4 bytes)
    char IR[4];         // Instruction Register (4 bytes)
    int IC;             // Instruction Counter (2-digit, 00-99)
    int C;              // Comparison flag (0 or 1)
    int SI;             // Service Interrupt (1=Read, 2=Write, 3=Halt)

    // Memory: 100 words, each 4 bytes
    char M[100][4];

    // Files
    std::ifstream fin;
    std::ofstream fout;

    // Buffer for reading input
    std::string buffer;

public:
    VirtualOS() {
        init();
    }

    void init() {
        // Clear memory - fill with spaces
        for (int i = 0; i < 100; i++)
            for (int j = 0; j < 4; j++)
                M[i][j] = ' ';

        // Clear CPU registers
        for (int i = 0; i < 4; i++) {
            R[i] = ' ';
            IR[i] = ' ';
        }

        IC = 0;
        C = 0;
        SI = 0;
    }

    void printMemory() {
        std::cout << "\n========== MEMORY DUMP ==========\n";
        std::cout << "Addr | Content\n";
        std::cout << "-----+---------\n";

        for (int i = 0; i < 100; i++) {
            bool empty = true;
            for (int j = 0; j < 4; j++) {
                if (M[i][j] != ' ') {
                    empty = false;
                    break;
                }
            }

            if (!empty) {
                std::cout << "  " << std::setw(2) << std::setfill('0') << i << " | ";
                for (int j = 0; j < 4; j++)
                    std::cout << M[i][j];
                std::cout << "\n";
            }
        }

        std::cout << "=================================\n\n";
    }

    void load(const std::string& inputFilename, const std::string& outputFilename) {
        fin.open(inputFilename);
        if (!fin.is_open()) {
            std::cerr << "[ERROR] Cannot open " << inputFilename << "\n";
            return;
        }

        fout.open(outputFilename);
        if (!fout.is_open()) {
            std::cerr << "[ERROR] Cannot open " << outputFilename << " for writing\n";
            fin.close();
            return;
        }

        std::cout << "==============================================\n";
        std::cout << "   VIRTUAL BASIC OS - Phase 1 Simulator (C++)\n";
        std::cout << "   Multiprogramming Operating System (MOS)\n";
        std::cout << "==============================================\n\n";

        int m = 0; // Memory pointer
        while (std::getline(fin, buffer)) {
            // Remove trailing \r if present (Windows compatibility)
            if (!buffer.empty() && buffer.back() == '\r') {
                buffer.pop_back();
            }

            if (buffer.substr(0, 4) == "$AMJ") {
                init();
                m = 0;
                std::cout << "[LOAD] New Job Detected: " << buffer << "\n";
                std::cout << "       Job ID       : " << buffer.substr(4, 4) << "\n";
                std::cout << "       Time Limit   : " << buffer.substr(8, 4) << "\n";
                std::cout << "       Line Limit   : " << buffer.substr(12, 4) << "\n";
            } 
            else if (buffer.substr(0, 4) == "$DTA") {
                std::cout << "[LOAD] Data section reached ($DTA)\n";
                std::cout << "[EXEC] Starting program execution...\n\n";
                printMemory();
                executeUserProgram();
            } 
            else if (buffer.substr(0, 4) == "$END") {
                std::cout << "\n[LOAD] End of Job: " << buffer.substr(4, 4) << "\n";
                std::cout << "----------------------------------------------\n\n";
            } 
            else {
                // Program/Data Card
                int len = buffer.length();
                for (int i = 0; i < len; i++) {
                    M[m][i % 4] = buffer[i];
                    if (i % 4 == 3) m++;
                }
                if (len % 4 != 0) m++;
            }
        }

        fin.close();
        fout.close();
        std::cout << "[DONE] All jobs processed. Check " << outputFilename << " for results.\n";
    }

    void executeUserProgram() {
        IC = 0; // Start from addr 0
        while (true) {
            // FETCH
            for (int i = 0; i < 4; i++)
                IR[i] = M[IC][i];
            
            int currentIC = IC;
            IC++;

            // DECODE & EXECUTE
            int address = (IR[2] - '0') * 10 + (IR[3] - '0');

            if (IR[0] == 'G' && IR[1] == 'D') {
                std::cout << "  [CPU] IC=" << std::setw(2) << std::setfill('0') << currentIC 
                          << " | Instruction: GD" << std::setw(2) << address 
                          << " (Get Data -> M[" << std::setw(2) << address << "])\n";
                SI = 1;
                MOS();
            } 
            else if (IR[0] == 'P' && IR[1] == 'D') {
                std::cout << "  [CPU] IC=" << std::setw(2) << std::setfill('0') << currentIC 
                          << " | Instruction: PD" << std::setw(2) << address 
                          << " (Put Data <- M[" << std::setw(2) << address << "])\n";
                SI = 2;
                MOS();
            } 
            else if (IR[0] == 'H') {
                std::cout << "  [CPU] IC=" << std::setw(2) << std::setfill('0') << currentIC 
                          << " | Instruction: H    (Halt)\n";
                SI = 3;
                MOS();
                break;
            } 
            else if (IR[0] == 'L' && IR[1] == 'R') {
                std::cout << "  [CPU] IC=" << std::setw(2) << std::setfill('0') << currentIC 
                          << " | Instruction: LR" << std::setw(2) << address 
                          << " (Load R <- M[" << std::setw(2) << address << "])\n";
                for (int i = 0; i < 4; i++) R[i] = M[address][i];
            } 
            else if (IR[0] == 'S' && IR[1] == 'R') {
                std::cout << "  [CPU] IC=" << std::setw(2) << std::setfill('0') << currentIC 
                          << " | Instruction: SR" << std::setw(2) << address 
                          << " (Store R -> M[" << std::setw(2) << address << "])\n";
                for (int i = 0; i < 4; i++) M[address][i] = R[i];
            } 
            else if (IR[0] == 'C' && IR[1] == 'R') {
                std::cout << "  [CPU] IC=" << std::setw(2) << std::setfill('0') << currentIC 
                          << " | Instruction: CR" << std::setw(2) << address 
                          << " (Compare R with M[" << std::setw(2) << address << "])\n";
                C = 1;
                for (int i = 0; i < 4; i++) {
                    if (R[i] != M[address][i]) {
                        C = 0;
                        break;
                    }
                }
                std::cout << "         C flag = " << C << " (" << (C ? "EQUAL" : "NOT EQUAL") << ")\n";
            } 
            else if (IR[0] == 'B' && IR[1] == 'T') {
                std::cout << "  [CPU] IC=" << std::setw(2) << std::setfill('0') << currentIC 
                          << " | Instruction: BT" << std::setw(2) << address 
                          << " (Branch if C=1)\n";
                if (C == 1) {
                    IC = address;
                    std::cout << "         Branching to IC=" << std::setw(2) << IC << "\n";
                } else {
                    std::cout << "         C=0, no branch taken\n";
                }
            } 
            else {
                std::cout << "  [CPU] IC=" << std::setw(2) << currentIC 
                          << " | Unknown instruction: " << IR[0] << IR[1] << IR[2] << IR[3] << "\n";
            }
        }
    }

    void MOS() {
        int address = (IR[2] - '0') * 10 + (IR[3] - '0');

        if (SI == 1) { // READ (GD)
            std::cout << "  [MOS] SI=1 (READ) -> Loading data card into M[" 
                      << std::setw(2) << std::setfill('0') << address << ".." 
                      << std::setw(2) << address + 9 << "]\n";
            
            if (std::getline(fin, buffer)) {
                if (!buffer.empty() && buffer.back() == '\r') buffer.pop_back();

                if (buffer.substr(0, 4) == "$END") {
                    std::cout << "  [MOS] Unexpected $END during GD. Halting.\n";
                    return;
                }

                int k = 0;
                int len = buffer.length();
                for (int i = 0; i < len; i++) {
                    M[address][k++] = buffer[i];
                    if (k == 4) {
                        address++;
                        k = 0;
                    }
                }
                std::cout << "  [MOS] Data loaded: \"" << buffer << "\"\n";
            } else {
                std::cout << "  [MOS] WARNING: No more data in input.txt for GD\n";
            }
        } 
        else if (SI == 2) { // WRITE (PD)
            std::cout << "  [MOS] SI=2 (WRITE) -> Writing M[" 
                      << std::setw(2) << std::setfill('0') << address << ".." 
                      << std::setw(2) << address + 9 << "] to output.txt\n";
            
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 4; j++)
                    fout.put(M[address + i][j]);
            }
            fout.put('\n');
            fout.flush();

            std::cout << "  [MOS] Output: \"";
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 4; j++)
                    std::cout << M[address + i][j];
            std::cout << "\"\n";
        } 
        else if (SI == 3) { // TERMINATE (H)
            std::cout << "  [MOS] SI=3 (TERMINATE) -> Job completed.\n";
            fout << "\n\n";
            fout.flush();
        }
    }
};

int main() {
    VirtualOS os;
    os.load("input.txt", "output.txt");
    return 0;
}
