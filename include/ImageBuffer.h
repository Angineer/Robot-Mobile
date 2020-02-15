#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H

#include <mutex>
#include <string>

#include <apriltag.h>

// A class that runs a polling thread to read in the newest image from
// the camera and check it for an apriltag. If one is found, it lets the
// MobileManager know.
class ImageBuffer {
public:
    // Constructor
    // @param imagePath The location of the image on disk that this object
    //        will check
    ImageBuffer ( const std::string & imagePath );

    // Destructor
    ~ImageBuffer();

    // Read image data into the write buffer
    void readImage ( const std::string& imagePath );

    // Update the read buffer to use the latest data, then get unprotected access
    // to the read buffer. If a write is actively occurring, it will wait for the
    // write to finish before opening the buffer.
    image_u8_t* getImage();

private:
    // Read the height and width off the file
    std::tuple<long, long> readImageParameters (
        const std::string & imagePath );

    // File size parameters
    long m_Width;
    long m_Height;

    // Internal buffers
    image_u8_t* m_WriteBuff;
    image_u8_t* m_ReadBuff;

    // Mutex to protect the image data buffers
    std::mutex m_Mutex;
};

#endif
