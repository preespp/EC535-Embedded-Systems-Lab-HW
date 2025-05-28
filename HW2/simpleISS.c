// Pree Simphliphan
// U01702082
// HW2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// For easiness to execute instruction
typedef struct {
    int8_t registers[6]; // 8-bit R1-R6
    uint8_t memory[256]; // 256-Byte Local Memory
    uint32_t mem_access[8]; // 256 bits = 8 x 32-bit integers
    uint16_t PC; // Program Counter
    size_t instr_count; // Total Executed Instructions
    size_t cycle_count; // Total Cycle Count
    size_t num_hit; // Total number of hit
    size_t num_LDST; // Total number of LD/ST instuction
} CPU;

typedef enum {
    MOV, ADD, LD, ST, CMP, JE, JMP, INVALID
} Opcode;

typedef struct {
    int num;
    int8_t opcode;
    char opcode_str[4];
    char operand1[10];
    char operand2[10];
} Instr;

// Convert index of register
int get_register_index(const char *reg) {
    return (reg[0] == 'R' && reg[1] >= '1' && reg[1] <= '6' && (reg[2] == '\0' || reg[2] == ',')) ? reg[1] - '1' : -1; // -1 means immediate
}

int8_t get_opcode(const char* opcode) {
    if (strcmp(opcode, "MOV") == 0) return 0;
    if (strcmp(opcode, "ADD") == 0) return 1;
    if (strcmp(opcode, "LD") == 0) return 2;
    if (strcmp(opcode, "ST") == 0) return 3;
    if (strcmp(opcode, "CMP") == 0) return 4;
    if (strcmp(opcode, "JE") == 0) return 5;
    if (strcmp(opcode, "JMP") == 0) return 6;
    return -1;
}

// execute instruction
void execute_instruction(CPU *cpu, Instr *instr, u_int8_t *flag) {
    int reg1, reg2, addr;
    char addr_str[10];

    cpu->instr_count++;

    if (instr->opcode == 0) {
        reg1 = get_register_index(instr->operand1);
        reg2 = get_register_index(instr->operand2);

        if (reg1 != -1) { 
            cpu->registers[reg1] = (reg2 != -1) ? cpu->registers[reg2] : strtol(instr->operand2, NULL, 10);;
        }
        cpu->cycle_count++;
        cpu->PC++;

    } else if (instr->opcode == 1) {
        reg1 = get_register_index(instr->operand1);
        reg2 = get_register_index(instr->operand2);

        // ADD R_ R_
        if (reg1 != -1 && reg2 != -1) {
            cpu->registers[reg1] += cpu->registers[reg2];
        } else if (reg1 != -1 && reg2 == -1) {
            cpu->registers[reg1] += strtol(instr->operand2, NULL, 10);;
        }
        cpu->cycle_count++;
        cpu->PC++;

    } else if (instr->opcode == 2) {
        reg1 = get_register_index(instr->operand1);
        size_t len = strlen(instr->operand2);
        strncpy(addr_str, instr->operand2 + 1 , len - 2);  // excluding [ and ]
        addr_str[len - 2] = '\0';
        reg2 = get_register_index(addr_str);
        addr = cpu->registers[reg2];

        if (reg1 != -1 && addr >= 0 && addr < 256) {
            int idx = addr / 32, bit = addr % 32;
            if (cpu->mem_access[idx] & (1 << bit)) { // Hit
                cpu->num_hit++;
                cpu->cycle_count += 2;
            } else { // Miss
                cpu->mem_access[idx] |= (1 << bit);
                cpu->cycle_count += 45;
            }
            cpu->registers[reg1] = cpu->memory[addr];
        }

        cpu->num_LDST++;
        cpu->PC++;

    } else if (instr->opcode == 3) {
        size_t len = strlen(instr->operand1);
        strncpy(addr_str, instr->operand1 + 1 , len - 3);  // excluding [ and ],
        addr_str[len - 3] = '\0';
        reg2 = get_register_index(addr_str);
        addr = cpu->registers[reg2];
        reg1 = get_register_index(instr->operand2);

        if (addr >= 0 && addr < 256 && reg1 != -1) {
            int idx = addr / 32, bit = addr % 32;
            if (cpu->mem_access[idx] & (1 << bit)) { // Hit
                cpu->num_hit++;
                cpu->cycle_count += 2;
            } else { // Miss
                cpu->mem_access[idx] |= (1 << bit);
                cpu->cycle_count += 45;
            }
            cpu->memory[addr] = cpu->registers[reg1];
        }

        cpu->num_LDST++;
        cpu->PC++;

    } else if (instr->opcode == 4) {
        reg1 = get_register_index(instr->operand1);
        reg2 = get_register_index(instr->operand2);

        if (reg1 != -1 && reg2 != -1) {
            *flag = (cpu->registers[reg1] == cpu->registers[reg2]) ? 1 : 0;
        }
        cpu->cycle_count += 1;
        cpu->PC++;

    } else if (instr->opcode == 5) {
        if (*flag == 1) {  // If CMP result was equal
            cpu->PC = strtol(instr->operand1, NULL, 10);;
            *flag = 0; // Reset flag
        } else {
            cpu->PC++;
        }
        cpu->cycle_count += 1;

    } else if (instr->opcode == 6) {
        cpu->PC = strtol(instr->operand1, NULL, 10);; // Jump directly
        cpu->cycle_count++;
    } else {
        printf("Unknown instruction: %s\n", instr->opcode_str);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    const char *input_file = argv[1];
    FILE *in_file = fopen(input_file, "r");

    // Define Variable to read assembly file
    size_t count = 0;
    size_t max_instr = 1000;
    Instr *instruction = malloc(max_instr * sizeof(Instr));

    char line[30];
    while (fgets(line, sizeof(line), in_file)) {
        int parsed = sscanf(line, "%d %s %s %s",
            &instruction[count].num,
            instruction[count].opcode_str,
            instruction[count].operand1,
            instruction[count].operand2);

        instruction[count].opcode = get_opcode(instruction[count].opcode_str);
    
        if (parsed < 2) break; // Stop if we didn't get a valid opcode

        count++;
    }

    // Difference between PC and index
    size_t diff = instruction[0].num;

    CPU cpu = {
        .registers = {0},
        .memory = {0},
        .mem_access = {0},
        .PC = instruction[0].num,
        .instr_count = 0,
        .cycle_count = 0,
        .num_hit = 0,
        .num_LDST = 0
    };

    u_int8_t flag = 0;

    // Iterate through all instructions
    while (cpu.PC - diff < count) {
        execute_instruction(&cpu, &instruction[cpu.PC - diff], &flag);
    }

    // Post processing (Close file + Free Memory in linked list)
    fclose(in_file);
    free(instruction);

    // Print Execution Summary
    printf("Total number of executed instructions: %zu\n", cpu.instr_count);
    printf("Total number of clock cycles: %zu\n", cpu.cycle_count);
    printf("Number of hits to local memory: %zu\n", cpu.num_hit);
    printf("Total number of executed LD/ST instructions: %zu\n", cpu.num_LDST);

    return EXIT_SUCCESS;
}