/*
 * ============================================================
 *  Virtual Basic Operating System Simulator - Phase 1
 *  Multiprogramming Operating System (MOS)
 * ============================================================
 *
 *  4 Sections:
 *    1. CPU      - Registers: R[4], IR[4], IC (2-digit), C (1-bit flag)
 *    2. Memory   - 100 words (00-99), each 4 bytes = 400 chars total
 *                  Divided into pages of 10 rows each (0-9, 10-19, ...)
 *    3. Input    - input.txt  (Job Card Reader)
 *    4. Output   - output.txt (Line Printer)
 *
 *  Job Card Format:
 *    $AMJ<JobID 4><TTL 4><TLL 4>  - Start of job
 *    <Program Cards>                - Instructions (GD, PD, H, LR, SR, CR, BT)
 *    $DTA                           - Start of data
 *    <Data Cards>                   - Data lines
 *    $END<JobID 4>                  - End of job
 *
 *  Instruction Set:
 *    GDxx - Get Data      : Read one data card into memory starting at address xx
 *    PDxx - Put Data      : Write 10 words (1 page) from address xx to output
 *    LRxx - Load Register : Load word at address xx into R
 *    SRxx - Store Register: Store R into memory at address xx
 *    CRxx - Compare        : Compare R with word at address xx, set C flag
 *    BTxx - Branch True    : If C == true, set IC = xx
 *    H    - Halt           : Terminate the program
 *
 *  Service Interrupts (SI):
 *    SI = 1 -> GD (Read)
 *    SI = 2 -> PD (Write)
 *    SI = 3 -> H  (Halt/Terminate)
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ======================== CPU Registers ======================== */
char R[4];          /* General Purpose Register (4 bytes)         */
char IR[4];         /* Instruction Register (4 bytes)             */
int  IC;            /* Instruction Counter (2-digit, 00-99)       */
int  C;             /* Comparison flag (Toggle: 0 or 1)           */
int  SI;            /* Service Interrupt (1=Read, 2=Write, 3=Halt)*/



char M[100][4];

/* ======================== I/O Files ============================ */
FILE *fin;          /* input.txt  - Job Card Reader                */
FILE *fout;         /* output.txt - Line Printer                   */

/* ======================== Buffer =============================== */
char buffer[256];   /* Buffer to read lines from input file        */

/* ======================== Flags ================================ */
int dataFlag;       /* Flag to track if we've reached $DTA section */

/* ============ Function Prototypes ============================== */
void init();                    /* Initialize memory & registers   */
void load();                    /* Load jobs from input.txt        */
void executeUserProgram();      /* Fetch-Decode-Execute cycle      */
void MOS();                     /* Master Operating System handler */
void printMemory();             /* Debug: print memory contents    */

/* ================================================================
 *  init() - Initialize all memory and CPU registers to defaults
 * ================================================================ */
void init()
{
    int i, j;

    /* Clear memory - fill with spaces */
    for (i = 0; i < 100; i++)
        for (j = 0; j < 4; j++)
            M[i][j] = ' ';

    /* Clear CPU registers */
    for (i = 0; i < 4; i++)
    {
        R[i]  = ' ';
        IR[i] = ' ';
    }

    IC = 0;
    C  = 0;
    SI = 0;
    dataFlag = 0;
}

/* ================================================================
 *  printMemory() - Debug utility to show memory contents
 * ================================================================ */
void printMemory()
{
    int i, j;

    printf("\n========== MEMORY DUMP ==========\n");
    printf("Addr | Content\n");
    printf("-----+---------\n");

    for (i = 0; i < 100; i++)
    {
        /* Only print non-empty locations */
        int empty = 1;
        for (j = 0; j < 4; j++)
        {
            if (M[i][j] != ' ')
            {
                empty = 0;
                break;
            }
        }

        if (!empty)
        {
            printf("  %02d | ", i);
            for (j = 0; j < 4; j++)
                printf("%c", M[i][j]);
            printf("\n");
        }
    }

    printf("=================================\n\n");
}

/* ================================================================
 *  load() - Read input.txt and process job cards
 *
 *  Control Cards:
 *    $AMJ - Start a new job, reset memory pointer to 0
 *    $DTA - Data section begins, start executing program
 *    $END - End of job
 *    else - Program card, store into memory word-by-word
 * ================================================================ */
void load()
{
    int m = 0;  /* Memory pointer (current word address) */
    int i, len;

    fin = fopen("input.txt", "r");
    if (fin == NULL)
    {
        printf("[ERROR] Cannot open input.txt\n");
        return;
    }

    fout = fopen("output.txt", "w");
    if (fout == NULL)
    {
        printf("[ERROR] Cannot open output.txt for writing\n");
        fclose(fin);
        return;
    }

    printf("==============================================\n");
    printf("   VIRTUAL BASIC OS - Phase 1 Simulator\n");
    printf("   Multiprogramming Operating System (MOS)\n");
    printf("==============================================\n\n");

    while (fgets(buffer, sizeof(buffer), fin) != NULL)
    {
        /* Remove trailing newline/carriage return */
        len = (int)strlen(buffer);
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
            buffer[--len] = '\0';

        /* ---- $AMJ: Start of a new Job ---- */
        if (strncmp(buffer, "$AMJ", 4) == 0)
        {
            init();     /* Reset memory and registers */
            m = 0;      /* Start loading from memory address 0 */

            printf("[LOAD] New Job Detected: %s\n", buffer);
            printf("       Job ID       : %.4s\n", buffer + 4);
            printf("       Time Limit   : %.4s\n", buffer + 8);
            printf("       Line Limit   : %.4s\n", buffer + 12);
        }
        /* ---- $DTA: Data section - begin execution ---- */
        else if (strncmp(buffer, "$DTA", 4) == 0)
        {
            printf("[LOAD] Data section reached ($DTA)\n");
            printf("[EXEC] Starting program execution...\n\n");

            /* Show memory state before execution */
            printMemory();

            /* Begin executing the loaded program */
            executeUserProgram();
        }
        /* ---- $END: End of Job ---- */
        else if (strncmp(buffer, "$END", 4) == 0)
        {
            printf("\n[LOAD] End of Job: %.4s\n", buffer + 4);
            printf("----------------------------------------------\n\n");
        }
        /* ---- Program/Data Card: Store into memory ---- */
        else
        {
            /* Store program card into memory, 4 chars per word */
            for (i = 0; i < len; i++)
            {
                M[m][i % 4] = buffer[i];

                if (i % 4 == 3)
                    m++;    /* Move to next memory word after every 4 chars */
            }

            /* If the line didn't end on a 4-char boundary, advance m */
            if (len % 4 != 0)
                m++;
        }
    }

    fclose(fin);
    fclose(fout);

    printf("[DONE] All jobs processed. Check output.txt for results.\n");
}

/* ================================================================
 *  executeUserProgram() - Fetch-Decode-Execute Cycle
 *
 *  Fetch:   IR = M[IC], IC++
 *  Decode:  Check IR[0] and IR[1] to determine instruction
 *  Execute: Perform the operation or trigger Service Interrupt
 *
 *  Instructions:
 *    GD xx -> SI=1, MOS()     (Get Data from input)
 *    PD xx -> SI=2, MOS()     (Put Data to output)
 *    H     -> SI=3, MOS()     (Halt program)
 *    LR xx -> R = M[xx]       (Load Register)
 *    SR xx -> M[xx] = R       (Store Register)
 *    CR xx -> C = (R == M[xx])(Compare Register)
 *    BT xx -> if C, IC = xx   (Branch if True)
 * ================================================================ */
void executeUserProgram()
{
    int address;
    int i;

    IC = 0;  /* Start execution from memory address 0 */

    while (1)
    {
        /* ---- FETCH ---- */
        for (i = 0; i < 4; i++)
            IR[i] = M[IC][i];

        IC++;   /* Increment Instruction Counter */

        /* ---- DECODE & EXECUTE ---- */

        /* Calculate operand address from IR[2] and IR[3] */
        address = (IR[2] - '0') * 10 + (IR[3] - '0');

        /* GD - Get Data (Read from input) */
        if (IR[0] == 'G' && IR[1] == 'D')
        {
            printf("  [CPU] IC=%02d | Instruction: GD%02d (Get Data -> M[%02d])\n",
                   IC - 1, address, address);
            SI = 1;
            MOS();
        }
        /* PD - Put Data (Write to output) */
        else if (IR[0] == 'P' && IR[1] == 'D')
        {
            printf("  [CPU] IC=%02d | Instruction: PD%02d (Put Data <- M[%02d])\n",
                   IC - 1, address, address);
            SI = 2;
            MOS();
        }
        /* H - Halt */
        else if (IR[0] == 'H')
        {
            printf("  [CPU] IC=%02d | Instruction: H    (Halt)\n", IC - 1);
            SI = 3;
            MOS();
            break;  /* Stop the execution loop */
        }
        /* LR - Load Register: R = M[address] */
        else if (IR[0] == 'L' && IR[1] == 'R')
        {
            printf("  [CPU] IC=%02d | Instruction: LR%02d (Load R <- M[%02d])\n",
                   IC - 1, address, address);
            for (i = 0; i < 4; i++)
                R[i] = M[address][i];
        }
        /* SR - Store Register: M[address] = R */
        else if (IR[0] == 'S' && IR[1] == 'R')
        {
            printf("  [CPU] IC=%02d | Instruction: SR%02d (Store R -> M[%02d])\n",
                   IC - 1, address, address);
            for (i = 0; i < 4; i++)
                M[address][i] = R[i];
        }
        /* CR - Compare Register: C = (R == M[address]) */
        else if (IR[0] == 'C' && IR[1] == 'R')
        {
            printf("  [CPU] IC=%02d | Instruction: CR%02d (Compare R with M[%02d])\n",
                   IC - 1, address, address);
            C = 1;  /* Assume equal */
            for (i = 0; i < 4; i++)
            {
                if (R[i] != M[address][i])
                {
                    C = 0;  /* Not equal */
                    break;
                }
            }
            printf("         C flag = %d (%s)\n", C, C ? "EQUAL" : "NOT EQUAL");
        }
        /* BT - Branch if True: if C==1, IC = address */
        else if (IR[0] == 'B' && IR[1] == 'T')
        {
            printf("  [CPU] IC=%02d | Instruction: BT%02d (Branch if C=1)\n",
                   IC - 1, address);
            if (C == 1)
            {
                IC = address;
                printf("         Branching to IC=%02d\n", IC);
            }
            else
            {
                printf("         C=0, no branch taken\n");
            }
        }
        else
        {
            printf("  [CPU] IC=%02d | Unknown instruction: %c%c%c%c\n",
                   IC - 1, IR[0], IR[1], IR[2], IR[3]);
        }
    }
}

/* ================================================================
 *  MOS() - Master Operating System (Service Interrupt Handler)
 *
 *  Handles service interrupts triggered by the CPU:
 *    SI = 1 (GD) -> Read one data card from input.txt into memory
 *                    at the address specified in IR[2..3], filling
 *                    10 words (one page).
 *    SI = 2 (PD) -> Write 10 words (one page) from memory starting
 *                    at the address specified in IR[2..3] to output.txt.
 *    SI = 3 (H)  -> Terminate the current job. Write separator to
 *                    output.txt.
 * ================================================================ */
void MOS()
{
    int address;
    int i, j, k, len;

    /* Extract memory address from IR[2] and IR[3] */
    address = (IR[2] - '0') * 10 + (IR[3] - '0');

    /* ---- SI = 1: READ (GD) ---- */
    if (SI == 1)
    {
        printf("  [MOS] SI=1 (READ) -> Loading data card into M[%02d..%02d]\n",
               address, address + 9);

        /* Read next line from input.txt (data card) */
        if (fgets(buffer, sizeof(buffer), fin) != NULL)
        {
            /* Remove trailing newline/carriage return */
            len = (int)strlen(buffer);
            while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
                buffer[--len] = '\0';

            /* Check if it's a control card instead of data */
            if (strncmp(buffer, "$END", 4) == 0)
            {
                printf("  [MOS] Unexpected $END during GD. Halting.\n");
                return;
            }

            /* Store data card into memory at given address */
            k = 0;
            for (i = 0; i < len; i++)
            {
                M[address][k++] = buffer[i];

                if (k == 4)
                {
                    address++;
                    k = 0;
                }
            }

            printf("  [MOS] Data loaded: \"%s\"\n", buffer);
        }
        else
        {
            printf("  [MOS] WARNING: No more data in input.txt for GD\n");
        }
    }
    /* ---- SI = 2: WRITE (PD) ---- */
    else if (SI == 2)
    {
        printf("  [MOS] SI=2 (WRITE) -> Writing M[%02d..%02d] to output.txt\n",
               address, address + 9);

        /* Write one page (10 words = 40 chars) to output file */
        for (i = 0; i < 10; i++)
        {
            for (j = 0; j < 4; j++)
                fputc(M[address + i][j], fout);
        }
        fputc('\n', fout);
        fflush(fout);

        /* Also print to console for visibility */
        printf("  [MOS] Output: \"");
        for (i = 0; i < 10; i++)
            for (j = 0; j < 4; j++)
                printf("%c", M[address + i][j]);
        printf("\"\n");
    }
    /* ---- SI = 3: TERMINATE (H) ---- */
    else if (SI == 3)
    {
        printf("  [MOS] SI=3 (TERMINATE) -> Job completed.\n");

        /* Write two blank lines as job separator in output */
        fprintf(fout, "\n\n");
        fflush(fout);
    }
}

/* ================================================================
 *  main() - Entry point
 * ================================================================ */
int main()
{
    load();
    return 0;
}
