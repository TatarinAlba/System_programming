#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"

struct DynamicArray
{
    int capacity;
    int size;
    int *head;
};

struct my_context
{
    char *name;
    struct DynamicArray* my_array;
};

struct DynamicArray
merge(struct DynamicArray *left_arr, struct DynamicArray *right_arr)
{
    int left_idx = 0;
    int right_idx = 0;
    int res_idx = 0;
    int *resulted_array = (int *)malloc((left_arr->size + right_arr->size) * sizeof(int));
    while (left_idx < left_arr->size && right_idx < right_arr->size)
    {
        if (*(left_arr->head + left_idx) < *(right_arr->head + right_idx))
        {
            resulted_array[res_idx++] = *(left_arr->head + left_idx++);
        }
        else
        {
            resulted_array[res_idx++] = *(right_arr->head + right_idx++);
        }
    }
    while (left_idx < left_arr->size)
    {
        resulted_array[res_idx++] = *(left_arr->head + left_idx++);
    }
    while (right_idx < right_arr->size)
    {
        resulted_array[res_idx++] = *(right_arr->head + right_idx++);
    }
    struct DynamicArray res_arr;
    res_arr.head = resulted_array;
    res_arr.size = (left_arr->size + right_arr->size);
    res_arr.capacity = (left_arr->size + right_arr->size);
    return res_arr;
}

struct DynamicArray 
mergeSort(struct DynamicArray array_to_be_sorted, struct my_context* ctx, struct coro* c)
{
    if (array_to_be_sorted.size <= 1)
    {
        struct DynamicArray to_return;
        to_return.size = to_return.capacity = array_to_be_sorted.size;
        if (to_return.size != 0)
        {
            to_return.head = (int *)malloc(sizeof(int));
            to_return.head[0] = array_to_be_sorted.head[0];
        }
        return to_return;
    }
    struct DynamicArray left_arr;
    struct DynamicArray right_arr;
    int *container = (int *)malloc((int)(array_to_be_sorted.size / 2) * sizeof(int));
    for (int i = 0; i < (int)array_to_be_sorted.size / 2; i++)
    {
        container[i] = array_to_be_sorted.head[i];
    }
    left_arr.head = container;
    left_arr.size = left_arr.capacity = (int)array_to_be_sorted.size / 2;
    container = (int *)malloc((array_to_be_sorted.size - (int)(array_to_be_sorted.size / 2)) * sizeof(int));
    for (int i = 0; i < array_to_be_sorted.size - (int)(array_to_be_sorted.size / 2); i++)
    {
        container[i] = array_to_be_sorted.head[(int)array_to_be_sorted.size / 2 + i];
    }
    right_arr.head = container;
    right_arr.size = right_arr.capacity = array_to_be_sorted.size - (int)(array_to_be_sorted.size / 2);
    printf("%s: yield\n", ctx->name);
    coro_yield();
    printf("%s: switch count %lld\n", ctx->name, coro_switch_count(c));
    struct DynamicArray new_left_arr = mergeSort(left_arr, ctx, c);
    printf("%s: yield\n", ctx->name);
    coro_yield();
    printf("%s: switch count %lld\n", ctx->name, coro_switch_count(c));
    struct DynamicArray new_right_arr = mergeSort(right_arr, ctx, c);
    free(left_arr.head);
    free(right_arr.head);
    printf("%s: yield\n", ctx->name);
    coro_yield();
    printf("%s: switch count %lld\n", ctx->name, coro_switch_count(c));
    struct DynamicArray sorted_array = merge(&new_left_arr, &new_right_arr);
    free(new_left_arr.head);
    free(new_right_arr.head);
    return sorted_array;
}

struct DynamicArray 
fileToArray(const char *file_name)
{
    FILE *ptr = fopen(file_name, "r");
    if (ptr == NULL)
    {
        printf("File does not exists\n");
        struct DynamicArray empty_array;
        empty_array.capacity = 0;
        empty_array.size = 0;
        empty_array.head = (int *)0;
        return empty_array;
    }
    int *numbers;
    int size = 0, capacity = 2;
    numbers = (int *)malloc(capacity * sizeof(int));
    if (numbers == NULL)
    {
        printf("Unable to allocate memory\n");
        struct DynamicArray empty_array;
        empty_array.capacity = 0;
        empty_array.size = 0;
        empty_array.head = (int *)0;
        return empty_array;
    }
    int num;
    while (fscanf(ptr, "%d", &num) != EOF)
    {
        numbers[size++] = num;
        if (size == capacity)
        {
            capacity *= 2;
            int *new_numbers = (int *)realloc(numbers, capacity * sizeof(int));
            if (new_numbers == NULL)
            {
                printf("Unable to reallocate memory\n");
                free(numbers);
                struct DynamicArray empty_array;
                empty_array.capacity = 0;
                empty_array.size = 0;
                empty_array.head = (int *)0;
                return empty_array;
            }
            numbers = new_numbers;
        }
    }
    fclose(ptr);
    struct DynamicArray return_array;
    if (size == 0)
    {
        free(numbers);
        return_array.capacity = 0;
        return_array.size = size;
        return_array.head = (int *)0;
        return return_array;
    }
    else
    {
        return_array.capacity = capacity;
        return_array.size = size;
        return_array.head = numbers;
        return return_array;
    }
}

static struct my_context *
my_context_new(const char *name, struct DynamicArray* my_array)
{
    struct my_context *ctx = malloc(sizeof(*ctx));
    ctx->name = strdup(name);
    ctx->my_array = my_array;
    return ctx;
}

static void
my_context_delete(struct my_context *ctx)
{
    free(ctx->name);
    free(ctx);
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
coroutine_func_f(void *context)
{
    /* IMPLEMENT SORTING OF INDIVIDUAL FILES HERE. */

    struct coro *this = coro_this();
    struct my_context *ctx = context;
    char *name = ctx->name;
    printf("Started coroutine %s\n", name);
    printf("%s: switch count %lld\n", name, coro_switch_count(this));
    struct DynamicArray resulted_array = mergeSort(*(ctx->my_array), ctx, this);
    free(ctx->my_array->head);
    *(ctx->my_array) = resulted_array;
    my_context_delete(ctx);
    return 0;
}

int main(const int argc, const char **argv)
{
    struct DynamicArray files[argc - 1];
    /* Initialize our coroutine global cooperative scheduler. */
    coro_sched_init();
    for (int i = 0; i < argc - 1; i++)
    { 
        files[i] = fileToArray(argv[i + 1]);
    }
    /* Start several coroutines. */
    
    for (int i = 0; i < argc - 1; ++i)
    {
        /*
         * The coroutines can take any 'void *' interpretation of which
         * depends on what you want. Here as an example I give them
         * some names.
         */
        char name[16];
        sprintf(name, "coro_%d", i);
        /*
         * I have to copy the name. Otherwise all the coroutines would
         * have the same name when they finally start.
         */
        coro_new(coroutine_func_f, my_context_new(name, &files[i]));
    }
    /* Wait for all the coroutines to end. */
    struct coro *c;
    while ((c = coro_sched_wait()) != NULL)
    {
        /*
         * Each 'wait' returns a finished coroutine with which you can
         * do anything you want. Like check its exit status, for
         * example. Don't forget to free the coroutine afterwards.
         */
        printf("Finished %d\n", coro_status(c));
        coro_delete(c);
    }
    /* All coroutines have finished. */

    /* IMPLEMENT MERGING OF THE SORTED ARRAYS HERE. */
    // Making merge for all files (merging algorithm)
    int anchor = 0;
    int number_of_arrays = argc - 1;
    int first_array = 0, second_array = 1;
    while (number_of_arrays > 1)
    {
        struct DynamicArray resulted_array = merge(&files[first_array], &files[second_array]);
        free(files[first_array].head);
        free(files[second_array].head);
        files[anchor++] = resulted_array;
        first_array += 2;
        second_array += 2;
        if (first_array == number_of_arrays)
        {
            number_of_arrays /= 2;
            first_array = 0;
            second_array = 1;
            anchor = 0;
        }
        else if (second_array == number_of_arrays)
        {
            resulted_array = merge(&files[anchor - 1], &files[first_array]);
            free(files[anchor - 1].head);
            free(files[first_array].head);
            files[anchor - 1] = resulted_array;
            number_of_arrays /= 2;
            first_array = 0;
            second_array = 1;
            anchor = 0;
        }
    }
    if (number_of_arrays == 1)
    {
        FILE *output_file = fopen("answer.txt", "w");
        for (int i = 0; i < files[0].size; i++)
        {
            fprintf(output_file, "%d ", files[0].head[i]);
        }
        fclose(output_file);
    }
    free(files[0].head);
    return 0;
}
