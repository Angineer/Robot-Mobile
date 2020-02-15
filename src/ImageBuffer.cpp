#include "ImageBuffer.h"

#include <fstream>

ImageBuffer::ImageBuffer ( const std::string & imagePath )
{
    std::tie ( m_Width, m_Height ) = readImageParameters ( imagePath );

    m_WriteBuff = image_u8_create ( m_Width, m_Height );
    m_ReadBuff = image_u8_create ( m_Width, m_Height );
}

ImageBuffer::~ImageBuffer()
{
    image_u8_destroy ( m_WriteBuff );
    image_u8_destroy ( m_ReadBuff );
}

std::tuple<long, long> ImageBuffer::readImageParameters (
    const std::string & imagePath )
{
    // I make some assumptions about the format here, specifically
    // that we're working with the BITMAPINFOHEADER format
    std::ifstream reader { imagePath, std::ios_base::in | std::ios_base::binary };

    // Read in the bmp and dib headers
    char header[54];
    reader.read ( header, 54 );

    // Read the size from the header
    long width, height;
    memcpy ( &width, &header[18], sizeof ( width ) );
    memcpy ( &height, &header[22], sizeof ( height ) );

    return std::make_tuple ( width, height );
}

void ImageBuffer::readImage ( const std::string& imagePath )
{
    std::ifstream reader { imagePath, std::ios_base::in | std::ios_base::binary };

    // Read in the data one pixel at a time and convert to grayscale as we do
    std::lock_guard<std::mutex> lock ( m_Mutex );
    char pixel_data[3];
    for ( unsigned long y = 0; y < m_Height; ++y ) {
        for ( unsigned long x = 0; x < m_Width; ++x ) {
            reader.read ( pixel_data, 3 );
            double r = pixel_data[0];
            double g = pixel_data[1];
            double b = pixel_data[2];

            // This is from the Wikipedia grayscale article
            unsigned char gray = 0.2126 * r + 0.7152 * g + 0.0722 * b;

            m_WriteBuff->buf[( m_Height - y - 1)*m_WriteBuff->stride + x] = gray;
        }
    }
}

image_u8_t* ImageBuffer::getImage()
{
    std::lock_guard<std::mutex> lock ( m_Mutex );
    std::swap ( m_WriteBuff, m_ReadBuff );
    return m_ReadBuff;
}