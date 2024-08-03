#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void dump_memory(pid_t pid, const char *output_file) {
    char maps_path[256];
    char mem_path[256];
    char line[256];

    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);

    FILE *maps_file = fopen(maps_path, "r");
    if (maps_file == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", maps_path, strerror(errno));
        return;
    }

    FILE *mem_file = fopen(mem_path, "rb");
    if (mem_file == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", mem_path, strerror(errno));
        fclose(maps_file);
        return;
    }

    FILE *output = fopen(output_file, "wb");
    if (output == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", output_file, strerror(errno));
        fclose(maps_file);
        fclose(mem_file);
        return;
    }

    while (fgets(line, sizeof(line), maps_file)) {
        unsigned long start, end;
        char perms[5];

        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) == 3) {
            // Check if the memory region has read permissions
            if (strchr(perms, 'r')) {
                char *buffer = malloc(end - start);
                if (buffer == NULL) {
                    fprintf(stderr, "Memory allocation error\n");
                    break;
                }

                if (fseek(mem_file, start, SEEK_SET) == 0) {
                    size_t bytes_read = fread(buffer, 1, end - start, mem_file);
                    if (bytes_read == (size_t)(end - start)) {
                        fwrite(buffer, 1, bytes_read, output);
                    } else {
                        fprintf(stderr, "Error reading memory at %lx-%lx\n", start, end);
                    }
                } else {
                    fprintf(stderr, "Error seeking in memory file at %lx\n", start);
                }

                free(buffer);
            }
        }
    }

    fclose(maps_file);
    fclose(mem_file);
    fclose(output);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <PID> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = (pid_t)atoi(argv[1]);
    const char *output_file = argv[2];

    dump_memory(pid, output_file);
    printf("Memory dump saved to %s\n", output_file);

    return EXIT_SUCCESS;
}
