#include "QtTextureLoader.h"
#include <QImage>

#include "MemoryLoggerOn.h"
namespace tim
{

ubyte* QtTextureLoader::loadImage(const std::string& file, ImageFormat& format) const
{
    QImage textureImg(file.c_str());

    if(textureImg.isNull())
        return nullptr;

    QImage& glImg = textureImg;//QOpenGLWidgets::convertToGLFormat(textureImg);

    if(glImg.isNull())
    {
        LOG("Not enough memory to convert ", file, "to gl format (",
            textureImg.height(),"x", textureImg.width());

        return nullptr;
    }

    try{
        ubyte* b = new ubyte[glImg.sizeInBytes()];
        memcpy(b, glImg.bits(), glImg.sizeInBytes());
        format.size.x() = glImg.size().width();
        format.size.y() = glImg.size().height();
        format.nbComponent = 4;
        return b;
    }
    catch(const std::bad_alloc&)
    {
        LOG("Not enough memory to alloc ", file, " (", glImg.sizeInBytes(), " bytes)");
    }

    return nullptr;
}

}
#include "MemoryLoggerOff.h"
