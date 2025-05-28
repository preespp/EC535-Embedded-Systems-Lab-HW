// Pree Simphliphan
// U01702082

#include "bits.h"
#include <stdlib.h>
#include <stdio.h>

// Converts a little-endian number to big-endian
uint32_t ConvertEndianness(uint32_t value) {
    return ((value >> 24) & 0xFF) | // 8 most significant bit
           ((value >> 8) & 0xFF00) |
           ((value << 8) & 0xFF0000) |
           ((value << 24) & 0xFF000000); // 8 least significant bit
}

// Encrypts a number using XOR with a 4-character key
uint32_t EncryptXOR(uint32_t value, const char *key) {
    uint32_t result = 0;

    // Because default is little-endian so we need to encrypt from left to right (q e m u)
    for (int i = 3; i >= 0; i--) {
        uint8_t value_one = (value >> (i * 8)) & 0xFF;
        uint8_t read_key = key[3-i];
        uint8_t encrypted = value_one ^ read_key;

        result |= ((uint32_t)encrypted) << (i * 8);
    }

    return result;
}

// Creates a new linked list node
Node *create_node(uint32_t decimal, uint32_t hex, uint32_t ascii) {
    Node *node = (Node *)malloc(sizeof(Node));
    // In case of fail in memory allocation
    if (!node) {
        exit(EXIT_FAILURE);
    }
    
    node->decimal = decimal;
    node->hex = hex;
    node->ascii = ascii;
    node->next = NULL; // will be implemented in inserted_sorted()
    return node;
}

// Inserts a node into the list in sorted order by ASCII value
void sort_list(Node **head, Node *new_node) {
    // This Node is header or is supposed to add before current header
    if (!*head || new_node->ascii < (*head)->ascii) {
        new_node->next = *head;
        *head = new_node;
        return;
    }

    // Find position to add
    Node *current = *head;
    while (current->next && current->next->ascii < new_node->ascii) {
        current = current->next;
    }
    new_node->next = current->next;
    current->next = new_node;
}

// Frees all nodes
void free_list(Node *head) {
    while (head) {
        Node *temp = head;
        head = head->next;
        free(temp);
    }
}