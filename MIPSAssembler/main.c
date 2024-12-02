#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define MAX_LINES 1000
#define MAX_LABELS 100
#define BASE_ADDRESS 0x00400000

// structure to store labels and their addresses
typedef struct {
    char label[MAX_LINE_LENGTH];
    int address;
} Label;

int labelCount = 0;
Label labels[MAX_LABELS];

// file read
int readFile(const char *filename, char lines[MAX_LINES][MAX_LINE_LENGTH]) {
    FILE *file = fopen(filename, "r");

    int lineCount = 0;
    while (fgets(lines[lineCount], MAX_LINE_LENGTH, file)) {
        // Remove trailing newline
        lines[lineCount][strcspn(lines[lineCount], "\r\n")] = '\0';
        lineCount++;
    }

    fclose(file);
    return lineCount;
}

// Separate labels and assign addresses
void labelSeparation(char lines[MAX_LINES][MAX_LINE_LENGTH], int lineCount) {
    int address = BASE_ADDRESS;

    for (int i = 0; i < lineCount; i++) {
        // if line ends with ':' its a label
        if (lines[i][strlen(lines[i]) - 1] == ':') {
            // Add label
            strncpy(labels[labelCount].label, lines[i], strlen(lines[i]) - 1);
            labels[labelCount].label[strlen(lines[i]) - 1] = '\0'; // null terminate
            labels[labelCount].address = address;
            labelCount++;
        } else if (lines[i][0] != '\0') {
            address += 4;
        }
    }
}

// Get label address for jump inst.
int getLabelAddress(const char *label) {
    for (int i = 0; i < labelCount; i++) {
        if (strcmp(labels[i].label, label) == 0) { //strcmp compares both strings compared with ASCII
            return labels[i].address;
        }
    }
}
// atoi is ASCII to integer.
// Parse register
int parseRegister(const char *reg) {
    return atoi(reg + 1); // Skip '$' and convert to integer
}

// Parse immediate value for future use
int parseImmediate(const char *imm) {
    return atoi(imm);
}
//FORMAT SECTION
//"0x%08X" used to format result as size of 8 hexa string
// Format R-type instruction
char *formatRType(int funct, int rd, int rs, int rt) {
    static char formatted[11];
    unsigned int result = (rs << 21) | (rt << 16) | (rd << 11) | funct;
    snprintf(formatted,
    sizeof(formatted), "0x%08X", result);
    return formatted;
}

// Format I-type instruction
char *formatIType(int opcode, int rs, int rt, int immediate) {
    static char formatted[11];
    unsigned int result = opcode | (rs << 21) | (rt << 16) | (immediate & 0xFFFF);
    snprintf(formatted, sizeof(formatted),"0x%08X", result);
    return formatted;
}

//format shift type
char *formatShiftType(int funct,int rd,int rt, int shamt) {
    static  char formatted[11];
    unsigned int result = (rt << 16) | (rd << 11) | (shamt << 6) | funct;
    snprintf(formatted, sizeof(formatted),"0x%08X", result);
    return formatted;
}

//format branch type
char *formatBranchType(int opcode,int rs,int rt, int labelAddress,int currentAddress) {
    static  char formatted[11];
    int offset = (labelAddress - currentAddress - 4) / 4; // calculate the offset for the branch
    unsigned int result = opcode | (rs << 21) | (rt << 16) | (offset & 0xFFFF);
    snprintf(formatted, sizeof(formatted),"0x%08X", result);
    return formatted;
}

// Translate instruction to machine code
char *translateInstruction(const char *instruction, int currentAddress) {
    static char machineCode[11];
    char parts[4][MAX_LINE_LENGTH];
    int partCount = 0;

    char *token = strtok((char *)instruction, " ,\t"); //to separate every instruction into parts based on tab space, space and ",".
    while (token) {
        strncpy(parts[partCount++], token, MAX_LINE_LENGTH);
        token = strtok(NULL, " ,\t");
    }

    // Process instruction
    if (strcmp(parts[0], "add") == 0) {
        return formatRType(0x20, parseRegister(parts[1]), parseRegister(parts[2]), parseRegister(parts[3]));
    }
    else if (strcmp(parts[0], "sub") == 0) {
        return formatRType(0x22, parseRegister(parts[1]), parseRegister(parts[2]), parseRegister(parts[3]));
    }
    else if (strcmp(parts[0], "and") == 0) {
        return formatRType(0x24, parseRegister(parts[1]), parseRegister(parts[2]), parseRegister(parts[3]));
    }
    else if (strcmp(parts[0], "or") == 0) {
        return formatRType(0x25, parseRegister(parts[1]), parseRegister(parts[2]), parseRegister(parts[3]));
    }
    else if (strcmp(parts[0], "sll") == 0) {
        return formatShiftType(0x00, parseRegister(parts[1]), parseRegister(parts[2]), parseImmediate(parts[3]));
    }
    else if (strcmp(parts[0], "addi") == 0) {
        return formatIType(0x20000000, parseRegister(parts[2]), parseRegister(parts[1]), parseImmediate(parts[3]));
    }
    else if (strcmp(parts[0], "andi") == 0) {
        return formatIType(0x30000000, parseRegister(parts[2]), parseRegister(parts[1]), parseImmediate(parts[3]));
    }
    else if (strcmp(parts[0], "beq") == 0) {
        return formatBranchType(0x10000000, parseRegister(parts[1]), 0, parseImmediate(parts[3]),currentAddress);
    }
    else if (strcmp(parts[0], "bne") == 0) {
        return formatBranchType(0x14000000, parseRegister(parts[1]), 0, parseImmediate(parts[3]),currentAddress);
    }
     else if (strcmp(parts[0], "j") == 0) {
        unsigned int address = getLabelAddress(parts[1]);
        unsigned int result = 0x08000000 | ((address >> 2) & 0x3FFFFFF);
        snprintf(machineCode, sizeof(machineCode), "0x%08X", result);
        return machineCode;
    }
}

// Write output file
void writeOutputFile(const char *filename, char machineCode[MAX_LINES][MAX_LINE_LENGTH], int codeCount) {
    FILE *file = fopen(filename, "w");

    fprintf(file, "Address     Code\n");
    int address = BASE_ADDRESS;
    for (int i = 0; i < codeCount; i++) {
        fprintf(file, "0x%08X %s\n", address, machineCode[i]);
        address += 4;
    }
    fclose(file);
}

int main() {

    const char *inputFilename = "mipsmars.asm";
    const char *outputFilename = "mipsmars.obj";

    char lines[MAX_LINES][MAX_LINE_LENGTH];
    int lineCount = readFile(inputFilename, lines);

    labelSeparation(lines, lineCount);

    char machineCode[MAX_LINES][MAX_LINE_LENGTH];
    int codeCount = 0;

    int address = BASE_ADDRESS;
    for (int i = 0; i < lineCount; i++) {
        // Skip label lines and empty lines
        if (!strchr(lines[i], ':') && lines[i][0] != '\0') {
            strcpy(machineCode[codeCount++], translateInstruction(lines[i], address));
            address += 4;
        }
    }

    writeOutputFile(outputFilename, machineCode, codeCount);
  }
