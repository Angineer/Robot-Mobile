#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H

#include <mutex>
#include <string>

#include <apriltag.h>

// A class that holds image data read in by the CameraFetcher and makes it
// available to the ImageChecker
class ImageBuffer {
public:
    // Constructor
    // @param imagePath An image on disk that has the same dimensions as the
    //        image we will be checking (used to initialize the buffer sizes)
    ImageBuffer ( const std::string & imagePath );

    // Destructor
    ~ImageBuffer();

    // Read image data into the write buffer, then save it to the swap buffer so
    // that it's available for reading
    void readImage ( const std::string& imagePath );

    // Pull the latest data from the swap buffer into the read buffer, then get
    // unprotected access to the read buffer
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
    image_u8_t* m_SwapBuff;
    image_u8_t* m_ReadBuff;

    // Mutex to protect the image data buffers when swapping
    std::mutex m_Mutex;
};

#endif
