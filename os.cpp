#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <cstring>

using namespace std;

class VirtualOS {
private:
    char R[4];
    char IR[4];
    int IC;
    int C;
    int SI;
    char M[100][4];
    ifstream fin;
    ofstream fout;
    string buffer;

public:
    VirtualOS() {
        init();
    }

    void init() {
        for (int i = 0; i < 100; i++)
            for (int j = 0; j < 4; j++)
                M[i][j] = ' ';

        for (int i = 0; i < 4; i++) {
            R[i] = ' ';
            IR[i] = ' ';
        }

        IC = 0;
        C = 0;
        SI = 0;
    }

    void printMemory() {
        cout << "\n========== MEMORY DUMP ==========\n";
        cout << "Addr | Content\n";
        cout << "-----+---------\n";

        for (int i = 0; i < 100; i++) {
            bool empty = true;
            for (int j = 0; j < 4; j++) {
                if (M[i][j] != ' ') {
                    empty = false;
                    break;
                }
            }

            if (!empty) {
                cout << "  " << setw(2) << setfill('0') << i << " | ";
                for (int j = 0; j < 4; j++)
                    cout << M[i][j];
                cout << "\n";
            }
        }
        cout << "=================================\n\n";
    }

    void load(const string& inputFilename, const string& outputFilename) {
        fin.open(inputFilename);
        if (!fin.is_open()) {
            cerr << "[ERROR] Cannot open " << inputFilename << "\n";
            return;
        }

        fout.open(outputFilename);
        if (!fout.is_open()) {
            cerr << "[ERROR] Cannot open " << outputFilename << " for writing\n";
            fin.close();
            return;
        }

        cout << "==============================================\n";
        cout << "   VIRTUAL BASIC OS - Phase 1 Simulator (C++)\n";
        cout << "   Multiprogramming Operating System (MOS)\n";
        cout << "==============================================\n\n";

        int m = 0;
        while (getline(fin, buffer)) {
            if (!buffer.empty() && buffer.back() == '\r') {
                buffer.pop_back();
            }

            if (buffer.substr(0, 4) == "$AMJ") {
                init();
                m = 0;
                cout << "[LOAD] New Job Detected: " << buffer << "\n";
                cout << "       Job ID       : " << buffer.substr(4, 4) << "\n";
                cout << "       Time Limit   : " << buffer.substr(8, 4) << "\n";
                cout << "       Line Limit   : " << buffer.substr(12, 4) << "\n";
            } 
            else if (buffer.substr(0, 4) == "$DTA") {
                cout << "[LOAD] Data section reached ($DTA)\n";
                cout << "[EXEC] Starting program execution...\n\n";
                printMemory();
                executeUserProgram();
            } 
            else if (buffer.substr(0, 4) == "$END") {
                cout << "\n[LOAD] End of Job: " << buffer.substr(4, 4) << "\n";
                cout << "----------------------------------------------\n\n";
            } 
            else {
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
        cout << "[DONE] All jobs processed. Check " << outputFilename << " for results.\n";
    }

    void executeUserProgram() {
        IC = 0;
        while (true) {
            for (int i = 0; i < 4; i++)
                IR[i] = M[IC][i];
            
            int currentIC = IC;
            IC++;

            int address = (IR[2] - '0') * 10 + (IR[3] - '0');

            if (IR[0] == 'G' && IR[1] == 'D') {
                cout << "  [CPU] IC=" << setw(2) << setfill('0') << currentIC 
                     << " | Instruction: GD" << setw(2) << address 
                     << " (Get Data -> M[" << setw(2) << address << "])\n";
                SI = 1;
                MOS();
            } 
            else if (IR[0] == 'P' && IR[1] == 'D') {
                cout << "  [CPU] IC=" << setw(2) << setfill('0') << currentIC 
                     << " | Instruction: PD" << setw(2) << address 
                     << " (Put Data <- M[" << setw(2) << address << "])\n";
                SI = 2;
                MOS();
            } 
            else if (IR[0] == 'H') {
                cout << "  [CPU] IC=" << setw(2) << setfill('0') << currentIC 
                     << " | Instruction: H    (Halt)\n";
                SI = 3;
                MOS();
                break;
            } 
            else if (IR[0] == 'L' && IR[1] == 'R') {
                cout << "  [CPU] IC=" << setw(2) << setfill('0') << currentIC 
                     << " | Instruction: LR" << setw(2) << address 
                     << " (Load R <- M[" << setw(2) << address << "])\n";
                for (int i = 0; i < 4; i++) R[i] = M[address][i];
            } 
            else if (IR[0] == 'S' && IR[1] == 'R') {
                cout << "  [CPU] IC=" << setw(2) << setfill('0') << currentIC 
                     << " | Instruction: SR" << setw(2) << address 
                     << " (Store R -> M[" << setw(2) << address << "])\n";
                for (int i = 0; i < 4; i++) M[address][i] = R[i];
            } 
            else if (IR[0] == 'C' && IR[1] == 'R') {
                cout << "  [CPU] IC=" << setw(2) << setfill('0') << currentIC 
                     << " | Instruction: CR" << setw(2) << address 
                     << " (Compare R with M[" << setw(2) << address << "])\n";
                C = 1;
                for (int i = 0; i < 4; i++) {
                    if (R[i] != M[address][i]) {
                        C = 0;
                        break;
                    }
                }
                cout << "         C flag = " << C << " (" << (C ? "EQUAL" : "NOT EQUAL") << ")\n";
            } 
            else if (IR[0] == 'B' && IR[1] == 'T') {
                cout << "  [CPU] IC=" << setw(2) << setfill('0') << currentIC 
                     << " | Instruction: BT" << setw(2) << address 
                     << " (Branch if C=1)\n";
                if (C == 1) {
                    IC = address;
                    cout << "         Branching to IC=" << setw(2) << IC << "\n";
                } else {
                    cout << "         C=0, no branch taken\n";
                }
            } 
            else {
                cout << "  [CPU] IC=" << setw(2) << currentIC 
                     << " | Unknown instruction: " << IR[0] << IR[1] << IR[2] << IR[3] << "\n";
            }
        }
    }

    void MOS() {
        int address = (IR[2] - '0') * 10 + (IR[3] - '0');

        if (SI == 1) {
            cout << "  [MOS] SI=1 (READ) -> Loading data card into M[" 
                 << setw(2) << setfill('0') << address << ".." 
                 << setw(2) << address + 9 << "]\n";
            
            if (getline(fin, buffer)) {
                if (!buffer.empty() && buffer.back() == '\r') buffer.pop_back();

                if (buffer.substr(0, 4) == "$END") {
                    cout << "  [MOS] Unexpected $END during GD. Halting.\n";
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
                cout << "  [MOS] Data loaded: \"" << buffer << "\"\n";
            }
        } 
        else if (SI == 2) {
            cout << "  [MOS] SI=2 (WRITE) -> Writing M[" 
                 << setw(2) << setfill('0') << address << ".." 
                 << setw(2) << address + 9 << "] to output.txt\n";
            
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 4; j++)
                    fout.put(M[address + i][j]);
            }
            fout.put('\n');
            fout.flush();

            cout << "  [MOS] Output: \"";
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 4; j++)
                    cout << M[address + i][j];
            cout << "\"\n";
        } 
        else if (SI == 3) {
            cout << "  [MOS] SI=3 (TERMINATE) -> Job completed.\n";
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
