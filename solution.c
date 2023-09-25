#include <stdio.h>
#include <stdlib.h>

struct DynamicArray {
    int capacity;
    int size;
    int* head;
};

struct DynamicArray merge(struct DynamicArray* left_arr, struct DynamicArray* right_arr) {
    int left_idx = 0;
    int right_idx = 0;
    int res_idx = 0;
    int* resulted_array = (int*) malloc((left_arr -> size + right_arr -> size) * sizeof(int));
    while (left_idx < left_arr -> size && right_idx < right_arr -> size) {
        if (*(left_arr -> head + left_idx) < *(right_arr -> head + right_idx)) {
            resulted_array[res_idx++] = *(left_arr -> head + left_idx++);
        } else {
            resulted_array[res_idx++] = *(right_arr -> head + right_idx++);
        }
    }
    while (left_idx < left_arr -> size) {
        resulted_array[res_idx++] = *(left_arr -> head + left_idx++);
    }
    while (right_idx < right_arr -> size) {
        resulted_array[res_idx++] = *(right_arr -> head + right_idx++);
    }
    struct DynamicArray res_arr;
    res_arr.head = resulted_array;
    res_arr.size = (left_arr -> size + right_arr -> size);
    res_arr.capacity = (left_arr -> size + right_arr -> size);
    return res_arr;
}

struct DynamicArray mergeSort(struct DynamicArray array_to_be_sorted) {
    if (array_to_be_sorted.size <= 1) {
        struct DynamicArray to_return;
        to_return.size = to_return.capacity = array_to_be_sorted.size;
        if (to_return.size != 0) {
            to_return.head = (int *) malloc(sizeof(int));
            to_return.head[0] = array_to_be_sorted.head[0];
        }
        return to_return;
    }
    struct DynamicArray left_arr;
    struct DynamicArray right_arr;
    int *container = (int*) malloc((int) (array_to_be_sorted.size / 2) * sizeof (int));
    for (int i = 0; i < (int) array_to_be_sorted.size / 2; i++) {
        container[i] = array_to_be_sorted.head[i];
    }
    left_arr.head = container;
    left_arr.size = left_arr.capacity = (int) array_to_be_sorted.size / 2;
    container = (int*) malloc((array_to_be_sorted.size - (int) (array_to_be_sorted.size / 2)) * sizeof (int));
    for (int i = 0; i < array_to_be_sorted.size - (int) (array_to_be_sorted.size / 2); i++) {
        container[i] = array_to_be_sorted.head[(int) array_to_be_sorted.size / 2 + i];
    }
    right_arr.head = container;
    right_arr.size = right_arr.capacity = array_to_be_sorted.size - (int) (array_to_be_sorted.size / 2);
    struct DynamicArray new_left_arr = mergeSort(left_arr);
    struct DynamicArray new_right_arr = mergeSort(right_arr);
    free(left_arr.head);
    free(right_arr.head);
    struct DynamicArray sorted_array = merge(&new_left_arr, &new_right_arr);
    free(new_left_arr.head);
    free(new_right_arr.head);
    return sorted_array;
}

struct DynamicArray file_to_array(const char* file_name) {
    FILE *ptr = fopen(file_name, "r");
    if (ptr == NULL) {
        printf("File does not exists\n");
        struct DynamicArray empty_array;
        empty_array.capacity = 0;
        empty_array.size = 0;
        empty_array.head = (int*) 0;
        return empty_array;
    }
    int* numbers;
    int size = 0, capacity = 2;
    numbers = (int*) malloc(capacity * sizeof (int));
    if (numbers == NULL) {
        printf("Unable to allocate memory\n");
        struct DynamicArray empty_array;
        empty_array.capacity = 0;
        empty_array.size = 0;
        empty_array.head = (int*) 0;
        return empty_array;
    }
    int num;
    while (fscanf(ptr, "%d", &num) != EOF) {
        numbers[size++] = num;
        if (size == capacity) {
            capacity *= 2;
            int* new_numbers = (int*) realloc(numbers, capacity * sizeof (int));
            if (new_numbers == NULL) {
                printf("Unable to reallocate memory\n");
                free(numbers);
                struct DynamicArray empty_array;
                empty_array.capacity = 0;
                empty_array.size = 0;
                empty_array.head = (int*) 0;
                return empty_array;
            }
            numbers = new_numbers;
        }
    }
    fclose(ptr);
    struct DynamicArray return_array;
    if (size == 0) {
        free(numbers);
        return_array.capacity = 0;
        return_array.size = size;
        return_array.head = (int*) 0;
        return return_array;
    }
    else {
        return_array.capacity = capacity;
        return_array.size = size;
        return_array.head = numbers;
        return return_array;
    }
}

int main () {

    return 0;
}