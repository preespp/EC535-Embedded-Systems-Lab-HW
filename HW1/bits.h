// Pree Simphliphan
// U01702082

#ifndef BITS_H
#define BITS_H

#include <stdint.h>

// Define the structure for a node for linked list (decimal for first column, hexadecimal for second column and ascii for last one)
typedef struct Node {
    // Use unsigned 32-bit instead of unsigned interger because of compatibility with memory allocation
    uint32_t decimal;
    uint32_t hex;
    uint32_t ascii;
    struct Node *next;
} Node;

// Functions for part 2
uint32_t ConvertEndianness(uint32_t value);
uint32_t EncryptXOR(uint32_t value, const char *key);

// Functions for linked list
Node *create_node(uint32_t decimal, uint32_t hex, uint32_t ascii);
void sort_list(Node **head, Node *new_node);
void free_list(Node *head);

#endif
