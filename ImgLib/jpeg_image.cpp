#include "ppm_image.h"

#include <array>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <setjmp.h>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void)
my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

void ScanlineFromImage(std::vector<JSAMPLE>& row, int y, const Image& image) {
    const Color* line = image.GetLine(y);
    const Color* line_end = line + image.GetWidth(); // попытка вставить это выражение в цикл вызывает segfault
    for (int i = 0; line < line_end; i += 3, ++line) {
        row[i] = static_cast<JSAMPLE>(line->r);
        row[i + 1] = static_cast<JSAMPLE>(line->g);
        row[i + 2] = static_cast<JSAMPLE>(line->b);
    }
}

bool SaveJPEG(const Path& file, const Image& image) {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    
    FILE * outfile;          // target file
    JSAMPROW row_pointer[1]; // pointer to JSAMPLE row[s]
    int row_stride;          // physical row width in image buffer
    
    /* Step 1: allocate and initialize JPEG compression object */
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    
    /* Step 2: specify data destination (eg, a file) */
#ifdef _MSC_VER
    if ((outfile = _wfopen(file.string().c_str(), "wb")) == NULL) {
#else
    if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) {
#endif
        std::cerr << "can't open " << file.string().c_str() << std::endl;
        return false;
    }
    jpeg_stdio_dest(&cinfo, outfile);
    
    /* Step 3: set parameters for compression */
    cinfo.image_width = image.GetWidth();
    cinfo.image_height = image.GetHeight();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    
    /* Step 4: Start compressor */
    jpeg_start_compress(&cinfo, TRUE);
    
    /* Step 5: while (scan lines remain to be written) */
    row_stride = image.GetWidth() * 3; // JSAMPLEs per row in image_buffer
    
    std::vector<JSAMPLE> buffer(row_stride);
    while (cinfo.next_scanline < cinfo.image_height) {
        int y = cinfo.next_scanline;
        ScanlineFromImage(buffer, y, image);
        
        row_pointer[0] = buffer.data();
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    
    /* Step 6: Finish compression */
    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    
    /* Step 7: release JPEG compression object */
    jpeg_destroy_compress(&cinfo);
    
    /* And we're done! */
    return true;
}

// тип JSAMPLE фактически псевдоним для unsigned char
void SaveSсanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255}};
    }
}

Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    
    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;
    
    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
        return {};
    }
    
    /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }
    
    jpeg_create_decompress(&cinfo);
    
    /* Шаг 2: устанавливаем источник данных */
    jpeg_stdio_src(&cinfo, infile);
    
    /* Шаг 3: читаем параметры изображения через jpeg_read_header() */
    (void) jpeg_read_header(&cinfo, TRUE);
    
    /* Шаг 4: устанавливаем параметры декодирования */
    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;
    
    /* Шаг 5: начинаем декодирование */
    (void) jpeg_start_decompress(&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;
    
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    
    /* Шаг 5a: выделим изображение ImgLib */
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());
    
    /* Шаг 6: while (остаются строки изображения) */
    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);
        
        SaveSсanlineToImage(buffer[0], y, result);
    }
    
    /* Шаг 7: Останавливаем декодирование */
    (void) jpeg_finish_decompress(&cinfo);
    
    /* Шаг 8: Освобождаем объект декодирования */
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    
    return result;
}

} // of namespace img_lib
