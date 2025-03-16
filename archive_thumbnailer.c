#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <archive.h>
#include <archive_entry.h>
#include <jpeglib.h>
#include <png.h>
#include <webp/decode.h>

#define THUMBNAIL_SIZE 256

// Supported image file extensions
const char *SUPPORTED_EXTENSIONS[] = {".jpg", ".jpeg", ".png", ".webp"};
#define NUM_SUPPORTED_EXTENSIONS (sizeof(SUPPORTED_EXTENSIONS) / sizeof(SUPPORTED_EXTENSIONS[0]))

// Check if filename has a supported image extension
int has_supported_extension(const char *filename) {
    for (size_t i = 0; i < NUM_SUPPORTED_EXTENSIONS; i++) {
        if (strstr(filename, SUPPORTED_EXTENSIONS[i])) {
            return 1;
        }
    }
    return 0;
}

// Extract first image file from archive
int get_first_image_from_archive(const char *archive_path, unsigned char **img_data, size_t *img_size, char **img_format) {
    struct archive *archive = archive_read_new();
    archive_read_support_format_all(archive);

    if (archive_read_open_filename(archive, archive_path, 10240) != ARCHIVE_OK) {
        fprintf(stderr, "Failed to open archive: %s\n", archive_path);
        return -1;
    }

    struct archive_entry *entry;
    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        const char *filename = archive_entry_pathname(entry);
        if (has_supported_extension(filename)) {
            *img_size = archive_entry_size(entry);
            *img_data = (unsigned char *)malloc(*img_size);
            if (!*img_data) {
                fprintf(stderr, "Memory allocation failed.\n");
                archive_read_free(archive);
                return -1;
            }

            // Read file data into memory
            archive_read_data(archive, *img_data, *img_size);

            // Get image format based on extension
            *img_format = strdup(strrchr(filename, '.'));

            archive_read_free(archive);
            return 0;
        }
    }

    archive_read_free(archive);
    return -1; // No image found
}

// Decode JPEG
unsigned char *decode_jpeg(unsigned char *data, size_t size, int *width, int *height) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, size);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    int row_stride = cinfo.output_width * cinfo.output_components;

    unsigned char *image = (unsigned char *)malloc(*width * *height * 3);
    unsigned char *buffer_array[1];

    while (cinfo.output_scanline < cinfo.output_height) {
        buffer_array[0] = image + (cinfo.output_scanline) * row_stride;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return image;
}

// Decode PNG
unsigned char *decode_png(unsigned char *data, size_t size, int *width, int *height) {
    png_image image;
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_memory(&image, data, size)) {
        return NULL;
    }

    *width = image.width;
    *height = image.height;
    image.format = PNG_FORMAT_RGB;

    unsigned char *buffer = (unsigned char *)malloc(PNG_IMAGE_SIZE(image));
    if (!buffer) {
        return NULL;
    }

    if (!png_image_finish_read(&image, NULL, buffer, 0, NULL)) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

// Decode WebP
unsigned char *decode_webp(unsigned char *data, size_t size, int *width, int *height) {
    return WebPDecodeRGB(data, size, width, height);
}
void png_write_callback(png_structp png_ptr, png_bytep data, png_size_t length) {
    FILE *fp = (FILE *)png_get_io_ptr(png_ptr);
    fwrite(data, 1, length, fp);
}
// Save image as PNG
void save_png(const char *filename, unsigned char *data, int width, int height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    setjmp(png_jmpbuf(png));

    png_set_compression_level(png, 9);
    png_set_filter(png, 0, PNG_FILTER_NONE);

    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_bytep row_pointers[height];
    for (int y = 0; y < height; y++)
        row_pointers[y] = data + y * width * 3;

    png_set_rows(png, info, row_pointers);
    png_set_filter(png, 0, PNG_FILTER_NONE);

    png_set_write_fn(png, (png_voidp)fp, png_write_callback, NULL);
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
    fclose(fp);

    png_destroy_write_struct(&png, &info);
}

// Save image as JPEG
void save_jpeg(const char *filename, unsigned char *data, int width, int height, int quality) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s for writing\n", filename);
        return;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;  // RGB
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer = &data[cinfo.next_scanline * width * 3];
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(fp);

    jpeg_destroy_compress(&cinfo);
}

// Generate thumbnail
void generate_thumbnail(const char *input_path, const char *output_path) {
    unsigned char *img_data = NULL;
    size_t img_size;
    char *img_format = NULL;
    int width, height;

    if (get_first_image_from_archive(input_path, &img_data, &img_size, &img_format) != 0) {
        fprintf(stderr, "No images found in archive.\n");
        return;
    }

    unsigned char *image = NULL;
    if (strstr(img_format, ".jpg") || strstr(img_format, ".jpeg")) {
        image = decode_jpeg(img_data, img_size, &width, &height);
    } else if (strstr(img_format, ".png")) {
        image = decode_png(img_data, img_size, &width, &height);
    } else if (strstr(img_format, ".webp")) {
        image = decode_webp(img_data, img_size, &width, &height);
    }

    if (!image) {
        fprintf(stderr, "Failed to decode image.\n");
        return;
    }

    // save_png(output_path, image, width, height);
    save_jpeg(output_path, image, width, height, 90);
    printf("Thumbnail saved to %s\n", output_path);

    free(img_data);
    free(image);
    free(img_format);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_archive> <output_thumbnail.png>\n", argv[0]);
        return 1;
    }

    generate_thumbnail(argv[1], argv[2]);
    return 0;
}
