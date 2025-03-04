#pragma once

namespace Media
{

enum class Type
{
    Unspecifed,
    BMP,
    JPEG,
    PNG,
    EXIF,
    MP4,
    HEVC
};

enum class Format
{
    Unknown,
    yuv420p,
    YUV422P,
    YUV444P,
    R8G8B8
};

}

namespace sl
{

enum class Status
{
    SUCCEED = 0,
    CORRUPT_FILE,
    INCORRECT_FORMAT,
    UNABLE_TO_OEPN_FILE,
    UNSUPPORT_FORMAT,
    OUT_OF_MEMORY
};

}
