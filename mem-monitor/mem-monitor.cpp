#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define PROCESS_ID 34029 // Reemplaza con el PID del proceso que deseas analizar

typedef struct {
    uintptr_t start;
    uintptr_t end;
} MemoryRegion;

MemoryRegion* getReadableMemoryRegions(pid_t pid, size_t* region_count) {
    char maps_filename[256];
    snprintf(maps_filename, sizeof(maps_filename), "/proc/%d/maps", pid);
    FILE* maps_file = fopen(maps_filename, "r");
    if (!maps_file) {
        perror("Error al abrir el archivo maps del proceso");
        return NULL;
    }

    MemoryRegion* regions = NULL;
    *region_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), maps_file)) {
        uintptr_t start, end;
        char perms[5];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) == 3) {
            if (perms[0] == 'r') { // Solo regiones legibles
                regions = (MemoryRegion*)realloc(regions, (*region_count + 1) * sizeof(MemoryRegion));
                if (!regions) {
                    perror("Error al asignar memoria");
                    fclose(maps_file);
                    return NULL;
                }
                regions[*region_count].start = start;
                regions[*region_count].end = end;
                (*region_count)++;
            }
        }
    }

    fclose(maps_file);
    return regions;
}

unsigned char* read_process_memory(pid_t pid, size_t* size, MemoryRegion* regions, size_t region_count) {
    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/mem", pid);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error al abrir el archivo de memoria del proceso");
        return NULL;
    }

    size_t total_size = 0;
    for (size_t i = 0; i < region_count; ++i) {
        total_size += (regions[i].end - regions[i].start);
    }

    unsigned char* buffer = (unsigned char*)malloc(total_size);
    if (!buffer) {
        perror("Error al asignar memoria");
        close(fd);
        return NULL;
    }

    unsigned char* ptr = buffer;
    for (size_t i = 0; i < region_count; ++i) {
        size_t length = regions[i].end - regions[i].start;
        if (pread(fd, ptr, length, regions[i].start) < 0) {
            perror("Error al leer el archivo de memoria del proceso");
            free(buffer);
            close(fd);
            return NULL;
        }
        ptr += length;
    }

    close(fd);
    *size = total_size;
    return buffer;
}

void render_image(const unsigned char* data, size_t size, int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    size_t rgb_data_size = size - (size % 3);
    unsigned char* rgb_data = (unsigned char*)malloc(rgb_data_size);
    if (!rgb_data) {
        perror("Error al asignar memoria para rgb_data");
        return;
    }

    memcpy(rgb_data, data, rgb_data_size);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ImGui::Text("Memory Viewer");
    ImGui::Image((void*)(intptr_t)texture, ImVec2(width, height));

    free(rgb_data);
}

int main() {
    pid_t pid = PROCESS_ID;
    int width = 512;
    int height = 512;
    size_t buffer_size;
    size_t region_count;

    MemoryRegion* regions = getReadableMemoryRegions(pid, &region_count);
    if (!regions) {
        fprintf(stderr, "No se encontraron regiones de memoria legibles para el PID %d\n", pid);
        return EXIT_FAILURE;
    }

    unsigned char* buffer = read_process_memory(pid, &buffer_size, regions, region_count);
    if (!buffer) {
        fprintf(stderr, "Error al leer la memoria del proceso\n");
        free(regions);
        return EXIT_FAILURE;
    }

    free(regions);

    if (!glfwInit()) {
        fprintf(stderr, "Error al inicializar GLFW\n");
        free(buffer);
        return EXIT_FAILURE;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Memory Viewer", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Error al crear la ventana\n");
        glfwTerminate();
        free(buffer);
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO* io = &ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, 1);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        render_image(buffer, buffer_size, width, height);

        ImGui::Render();
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    free(buffer);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
