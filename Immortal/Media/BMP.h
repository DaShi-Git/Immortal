#pragma once

#include <cstdint>
#include <cstdlib>

#include <string>
#include <memory>

#include "Decoder.h"

namespace Media
{

class BMPDecoder : public Decoder
{
public:
    using Super = Decoder;

public:
    BMPDecoder() :
        Super{ Type::BMP, Format::R8G8B8 }
    {
        memset(&identifer, 0, HeaderSize());
    }

    virtual ~BMPDecoder()
    {
    
    }

    virtual uint8_t *Data() const override
    {
        return data.get();
    }

    size_t HeaderSize()
    {
        return reinterpret_cast<uint8_t *>(&importantColours) - reinterpret_cast<uint8_t *>(&identifer);
    }

    Error Read(const std::string &filename);

private:
    #pragma pack(push, 1)
    uint16_t identifer;
    uint32_t fileSize;
    uint16_t RESERVER_ST;
    uint16_t RESERVER_ND;
    uint32_t offset;
    uint32_t informationSize;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compressionType;
    uint32_t imageSize;
    int32_t  horizontalResolution;
    int32_t  verticalResolution;
    uint32_t coloursNum;
    uint32_t importantColours;
    #pragma pack(pop)

    int depth{ 3 };
    std::unique_ptr<uint8_t> data;
};

}
