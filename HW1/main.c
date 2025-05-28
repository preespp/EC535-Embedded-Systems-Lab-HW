// Pree Simphliphan
// U01702082

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bits.h"

int main(int argc, char *argv[]) {

    // Reading Inputs
    const char *input = argv[1];
    const char *output = argv[2];
    const char *key = argv[3];

    FILE *in_file = fopen(input, "r");

    FILE *out_file = fopen(output, "w");

    Node *head = NULL;

    // Convert input (Part 2)
    uint32_t number;
    while (fscanf(in_file, "%u", &number) == 1) {
        uint32_t big_endian = ConvertEndianness(number);
        uint32_t encrypt = EncryptXOR(big_endian, key);

        Node *new_node = create_node(number, big_endian, encrypt);
        sort_list(&head, new_node);
    }

    // Write sorted data to output file (Part 3) First column decimal, then hexadecimal the ascii (one-by-one character)
    for (Node *current = head; current != NULL; current = current->next) {
        fprintf(out_file, "%u\t%08x\t%c%c%c%c\r\n\r\n", 
                current->decimal, 
                current->hex,
        // For ASCII
                (current->ascii >> 24) & 0xFF, 
                (current->ascii >> 16) & 0xFF, 
                (current->ascii >> 8) & 0xFF, 
                current->ascii & 0xFF);
    }

    // Post processing (Close file + Free Memory in linked list)
    fclose(in_file);
    fclose(out_file);
    free_list(head);

    return EXIT_SUCCESS;
}
