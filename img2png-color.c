#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>

// Definición del ancho de la imagen
#define IMAGE_WIDTH 512

void write_png_file(const char* filename, uint8_t* buffer, int width, int height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error al abrir el archivo de salida");
        exit(EXIT_FAILURE);
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        perror("Error al crear la estructura PNG");
        exit(EXIT_FAILURE);
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        perror("Error al crear la información PNG");
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        perror("Error durante la escritura de la imagen PNG");
        exit(EXIT_FAILURE);
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    png_bytep row_pointers[height];
    for (int y = 0; y < height; y++) {
        row_pointers[y] = buffer + y * width * 3;  // Cada píxel tiene 3 bytes (RGB)
    }

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_dump> <archivo_salida>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        perror("Error al abrir el archivo de entrada");
        return EXIT_FAILURE;
    }

    // Obtener el tamaño del archivo
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Calcular las dimensiones de la imagen
    int width = IMAGE_WIDTH;
    int height = file_size / (width * 3);  // Cada píxel tiene 3 bytes (RGB)

    // Asegurar que la imagen tenga suficientes datos
    long required_size = (long)width * height * 3;  // Multiplicar por 3 para RGB
    uint8_t *buffer = (uint8_t*)malloc(required_size);
    if (!buffer) {
        perror("Error al asignar memoria");
        fclose(fp);
        return EXIT_FAILURE;
    }

    size_t read_size = fread(buffer, 1, required_size, fp);
    if (read_size != required_size) {
        fprintf(stderr, "Advertencia: la lectura no completó todos los datos, leídos %zu bytes\n", read_size);
    }

    fclose(fp);

    write_png_file(output_file, buffer, width, height);
    free(buffer);

    printf("Imagen guardada como %s\n", output_file);
    return EXIT_SUCCESS;
}
