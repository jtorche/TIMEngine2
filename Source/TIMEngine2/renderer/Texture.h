#ifndef TEXTURE_H_RENDERER
#define TEXTURE_H_RENDERER

#include "GLState.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace renderer
{
    class Texture : public NonCopyable
    {
    public:
        enum Type
        {
            TEXTURE_2D=0,
            TEXTURE_3D,
            CUBE_MAP,
            ARRAY_2D,
        };

        enum Format
        {
            R=0, R8, R16, R16F, R32F,
            RGB, RGB8, RGB16, RGB16F, RGB32F,
            RGBA, RGBA8, RGBA16, RGBA16F, RGBA32F,
            RGB10_A2, RG3_B2, RG11F_B10F,
            DEPTHCOMPONENT,
        };

        static GLenum toGLType(Type);
        static GLenum toGLFormat(Format);

        struct GenTexParam
        {
            uivec3 size;
            Format format = Format::RGBA8;
            int nbLevels = 1; // 0 or less means maximum levels of mipmap

            bool repeat=false, linear=false, trilinear=false, depthMode=false;
            int anisotropy = 0;

            bool operator<(const GenTexParam& p) const
            {
                if(size < p.size) return true; else if(size > p.size) return false;
                if(format < p.format) return true; else if(format > p.format) return false;
                if(nbLevels < p.nbLevels) return true; else if(nbLevels > p.nbLevels) return false;
                if(anisotropy < p.anisotropy) return true; else if(anisotropy > p.anisotropy) return false;
                bvec4 my = bvec4(repeat,linear,trilinear,depthMode);
                bvec4 your = bvec4(p.repeat,p.linear,p.trilinear,p.depthMode);
                if(my < your) return true; else return false;
            }
        };

        static Texture* genTexture2D(const GenTexParam&, const ubyte* data=nullptr, uint nbC=0);
        static Texture* genTexture2D(const GenTexParam&, const float* data, uint nbC);

        static Texture* genTextureCube(const GenTexParam&, const vector<ubyte*>& data={}, uint nbC=0);

        static Texture* genTextureArray2D(const GenTexParam&, const ubyte* data=nullptr, uint nbC=0);
        static Texture* genTextureArray2D(const GenTexParam&, const float* data, uint nbC);

        static void exportTexture(Texture*, std::string, int nbMipmap = 1);
        static Texture* genTextureFromRawData(ubyte*, GenTexParam);

        static uint genTextureSampler(bool repeat, bool linear, bool mipmapLinear,bool depthTest, int anisotropy=0);
        static void removeTextureSampler(uint sampler);

        Texture() = default;
        ~Texture();

        void bind(uint) const;
        void makeBindless() const;

        uint id() const;
        uint64_t handle() const;
        Type type() const;
        Format format() const;

        uivec2 resolution() const;
        uivec3 size() const;

    private:
        uint _id=0;
        uivec3 _size;
        Type _type=TEXTURE_2D;
        Format _format=Format::RGB;

        mutable uint64_t _handle = 0;
        mutable bool _isBindless = false;

        static Texture* genTexture2D(uint, const GenTexParam&, const void*, uint);
        static Texture* genTextureArray2D(uint, const GenTexParam&, const void*, uint);
        static void setupParameter(GLenum type, bool repeat, bool linear, bool mipmapLinear,bool depthTest, int anisotropy=0);

        static uint bytePerPixel(Format);
        static bool isFloatFormat(Format);
        static GLenum toExternalFormat(Format);

    };

    inline uint Texture::id() const { return _id; }
    inline uint64_t Texture::handle() const { return _handle; }
    inline Texture::Type Texture::type() const { return _type; }
    inline Texture::Format Texture::format() const { return _format; }
    inline uivec2 Texture::resolution() const { return _size.to<2>(); }
    inline uivec3 Texture::size() const { return _size; }

    inline GLenum Texture::toGLType(Type t)
    {
        static const GLenum glType[] = { GL_TEXTURE_2D,
                                         GL_TEXTURE_3D,
                                         GL_TEXTURE_CUBE_MAP,
                                         GL_TEXTURE_2D_ARRAY,
                                       };
        return glType[t];
    }

    inline GLenum Texture::toGLFormat(Format f)
    {
        static const GLenum glFormat[] = { GL_R, GL_R8, GL_R16, GL_R16F, GL_R32F,
                                           GL_RGB, GL_RGB8, GL_RGB16, GL_RGB16F, GL_RGB32F,
                                           GL_RGBA, GL_RGBA8, GL_RGBA16, GL_RGBA16F, GL_RGBA32F,
                                           GL_RGB10_A2, GL_R3_G3_B2, GL_R11F_G11F_B10F,
                                           GL_DEPTH_COMPONENT32 };
        return glFormat[f];
    }

    inline GLenum Texture::toExternalFormat(Format f)
    {
        static const GLenum exFormat[] = { GL_R, GL_R, GL_R, GL_R, GL_R,
                                           GL_RGB, GL_RGB, GL_RGB, GL_RGB, GL_RGB,
                                           GL_RGBA, GL_RGBA, GL_RGBA, GL_RGBA, GL_RGBA,
                                           GL_RGB, GL_RGB, GL_RGB,
                                           GL_DEPTH_COMPONENT };
        return exFormat[f];
    }

    inline bool Texture::isFloatFormat(Format f)
    {
        static const bool t[] = { false, false, false, true, true,
                                  false, false, false, true, true,
                                  false, false, false, true, true,
                                  false, false, true,
                                  false };
        return t[f];
    }

    inline uint Texture::bytePerPixel(Format f)
    {
        static const GLenum bps[] = { 1, 1, 2, 4, 4,
                                      3, 3, 6, 12, 12,
                                      4, 4, 8, 16, 16,
                                      4, 1, 12,
                                      4 };
        return bps[f];
    }

}
}
#include "MemoryLoggerOff.h"

#endif // TEXTURE_H
