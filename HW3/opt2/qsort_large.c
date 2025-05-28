#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>  // for uint64_t //prees26 starlion

#define MAXARRAY 60000

/* 
 * [prees26 starlion] Changed from storing a double 'distance' to 
 * storing the squared distance (distSq) as a 64-bit integer (uint64_t).
 * Avoids floating-point math for improved performance.
 */
typedef struct {
    int x, y, z;
    uint64_t distSq;  // x^2 + y^2 + z^2 in 64-bit integer //prees26 starlion
} My3DVertex;

/* 
 * [prees26 starlion] Added new merge function 
 * to implement mergesort (replaces qsort + compare).
 */
void merge(My3DVertex *array, int left, int mid, int right, My3DVertex *temp) {
    int i = left;     // index for left subarray
    int j = mid + 1;  // index for right subarray
    int k = left;     // index for temp array

    while (i <= mid && j <= right) {
        /* [prees26 starlion] Compare using distSq instead of float distance */
        if (array[i].distSq <= array[j].distSq) {
            temp[k++] = array[i++];
        } else {
            temp[k++] = array[j++];
        }
    }
    // Copy any remaining elements on the left side
    while (i <= mid) {
        temp[k++] = array[i++];
    }
    // Copy any remaining elements on the right side
    while (j <= right) {
        temp[k++] = array[j++];
    }
    // Copy back to original array
    for (i = left; i <= right; i++) {
        array[i] = temp[i];
    }
}

/* 
 * [prees26 starlion] Added recursive mergesort function 
 * to replace the old qsort() call.
 */
void mergeSort(My3DVertex *array, int left, int right, My3DVertex *temp) {
    if (left < right) {
        int mid = (left + right) / 2;
        mergeSort(array, left, mid, temp);
        mergeSort(array, mid + 1, right, temp);
        merge(array, left, mid, right, temp);
    }
}

int main(int argc, char *argv[]) {
    My3DVertex array[MAXARRAY];
    FILE *fp;
    int count = 0;
    int x, y, z;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    /* [prees26 starlion] We no longer compute floating-point distance with sqrt().
     * Instead, we compute (x^2 + y^2 + z^2) in a 64-bit variable (distSq). 
     */
    while ((fscanf(fp, "%d %d %d", &x, &y, &z) == 3) && count < MAXARRAY) {
        array[count].x = x;
        array[count].y = y;
        array[count].z = z;
        
        // [prees26 starlion] Casting to 64-bit for safe multiplication
        int64_t dx = (int64_t)x;
        int64_t dy = (int64_t)y;
        int64_t dz = (int64_t)z;
        array[count].distSq = (uint64_t)(dx * dx + dy * dy + dz * dz);
        
        count++;
    }
    fclose(fp);

    printf("\nSorting %d vectors based on squared distance from the origin (64-bit integer).\n\n", count);

    /* [prees26 starlion] Allocating temporary array for mergesort 
     * (needed to merge subarrays without overwriting data).
     */
    My3DVertex *temp = malloc(count * sizeof(My3DVertex));
    if (!temp) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    // [prees26 starlion] Replaced qsort with our custom mergeSort
    mergeSort(array, 0, count - 1, temp);
    free(temp);

    // Print sorted results
    for (int i = 0; i < count; i++) {
        printf("%d %d %d\n", array[i].x, array[i].y, array[i].z);
    }

    return 0;
}
