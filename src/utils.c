#include "utils.h"

double 
timespec_diff(struct timespec first, struct timespec second)
{
    return (first.tv_sec - second.tv_sec) + (first.tv_nsec - second.tv_nsec) / 1e9;
}

char* 
read_file_content(const char* file_name) {
    FILE *file_ptr = fopen(file_name, "r");
    if (file_ptr == NULL)
    {
        return NULL;
    }

    // Compute the length of the file
    fseek(file_ptr, 0, SEEK_END);
    long fsize = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);

    // Read the whole content
    char *content = malloc(fsize + 1);
    fread(content, fsize, 1, file_ptr);
    //TODO: fread might have read less than fsize I think?
    fclose(file_ptr);

    //Make sure the string is null terminated.
    content[fsize] = 0;
    return content;
}