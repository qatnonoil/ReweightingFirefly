#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>
#include <glm/glm.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
//
#include <vector>
#include <array>
#include <algorithm>

//
class Image
{
public:
    Image() = default;
    void resize(int32_t w, int32_t h)
    {
        width_ = w;
        height_ = h;
        pixels_.resize(w * h);
    }
    bool load(const char* filename)
    {
        float* out = nullptr;
        int32_t width;
        int32_t height;
        const char* err = nullptr;
        int ret = LoadEXR(&out, &width, &height, filename, &err);
        if (ret != TINYEXR_SUCCESS)
        {
            return false;
        }
        //
        width_ = width;
        height_ = height;
        pixels_.resize(width * height);
        for (size_t pi = 0; pi < width * height; ++pi)
        {
            const auto nanToZero = [](float v)
            {
                return std::isnan(v) ? 0.0f : v;
            };
            pixels_[pi] = glm::vec4(
                nanToZero(*(out + pi * 4 + 0)),
                nanToZero(*(out + pi * 4 + 1)),
                nanToZero(*(out + pi * 4 + 2)),
                nanToZero(*(out + pi * 4 + 3)));
        }
        free(out);
        return true;
    }
    void saveHdr(const char* filename)
    {
        stbi_write_hdr(filename, width_, height_, 4, (const float*)pixels_.data());
    }
    //
    int32_t width() const
    {
        return width_;
    }
    //
    int32_t height() const
    {
        return height_;
    }
    const glm::vec4& operator[](size_t idx) const
    {
        return pixels_[idx];
    }
    glm::vec4& operator[](size_t idx)
    {
        return pixels_[idx];
    }
    const glm::vec4& operator()(size_t x, size_t y) const
    {
        return pixels_[x + y * width_];
    }
    glm::vec4& operator()(size_t x, size_t y)
    {
        return pixels_[x + y * width_];
    }

private:
    int32_t width_ = 0;
    int32_t height_ = 0;
    std::vector<glm::vec4> pixels_;
};

//
void naive(const std::array<Image, 6>& images)
{
    const int32_t width = images[0].width();
    const int32_t height = images[0].height();
    Image outImage;
    outImage.resize(width, height);
    for (int32_t pi = 0; pi < width * height; ++pi)
    {
        for(int32_t layer=0; layer < images.size();++layer)
        {
            outImage[pi] += images[layer][pi];
        }
    }
    outImage.saveHdr("out_naive.hdr");
}

//
void reweightingFirefly(const std::array<Image, 6>& images)
{
    //
    const float kappa = 64.0f;
    const float cascadeBase = 8.0f;
    const float oneOverK = 1.0f / kappa;
    const float N = 2048.0f;
    //
    const auto luminance = [](glm::vec4 c)
    {
        return glm::dot(c, glm::vec4(0.212671f, 0.715160f, 0.072169f, 0.0f));
    };
    // average of <r>-radius pixel block in <layer> at <coord> (only using r=0 and r=1)
    const auto sampleLayer = [&](int32_t layer, glm::ivec2 xy, int r, float scale) -> glm::vec4
    {
        glm::vec4 val(0.0f);
        auto& img = images[layer];
        int y = -r;
        for (int i = 0; i < 3; ++i) {
            int x = -r;
            const int32_t iy = std::clamp(xy.y + y, 0, img.height() - 1);
            for (int j = 0; j < 3; ++j)
            {
                const int32_t ix = std::clamp(xy.x + x, 0, img.width() - 1);
                glm::vec4 c = img[ix + iy * img.width()];
                c *= scale;
                val += c;
                if (++x > r) break;
            }
            if (++y > r) break;
        }
        return val / float((2 * r + 1) * (2 * r + 1));
    };
    // sum of reliabilities in <curr> layer, <prev> layer and <next> layer
    const auto sampleReliability = [&](int32_t layer, glm::ivec2 xy, int r, float currScale)
    {
        //
        glm::vec4 rel = sampleLayer(layer, xy, r, currScale);
        if (layer != 0)
            rel += sampleLayer(layer - 1, xy, r, currScale * cascadeBase);
        if (layer + 1 != images.size())
            rel += sampleLayer(layer + 1, xy, r, currScale / cascadeBase);
        return luminance(rel);
    };
    //
    const auto lerp = [](float a, float b, float t)
    {
        return a * (1.0f - t) + b * t;
    };
    //
    const int32_t width = images[0].width();
    const int32_t height = images[0].height();
    Image out;
    out.resize(width, height);
    for (int32_t layer = 0; layer < images.size(); ++layer)
    {
        for (int32_t y = 0; y < height; ++y)
        {
            for (int32_t x = 0; x < width; ++x)
            {
                // N/kappa / b^i_<curr>;
                float currScale = N / (kappa * std::pow(cascadeBase, layer));
                //
                const glm::ivec2 xy(x, y);
                /* sample counting-based reliability estimation */
                // reliability in 3x3 pixel block (see robustness)
                float globalReliability = sampleReliability(layer, xy, 1, currScale);
                // reliability of curent pixel
                float localReliability = sampleReliability(layer, xy, 0, currScale);

                float reliability = globalReliability - oneOverK;
                // check if above minimum sampling threshold
                if (reliability >= 0.)
                    // then use per-pixel reliability
                    reliability = localReliability - oneOverK;

                /* color-based reliability estimation */

                float colorReliability = luminance(out[x+ out.width()* y]) * currScale;

                // a minimum image brightness that we always consider reliable
                colorReliability = std::max(colorReliability, 0.05f * currScale);

                // if not interested in exact expected value estimation, can usually accept a bit
                // more variance relative to the image brightness we already have
                float optimizeForError = std::max(0.0f, std::min(1.0f, oneOverK));
                // allow up to ~<cascadeBase> more energy in one sample to lessen bias in some cases
                colorReliability *= lerp(lerp(1.0f, cascadeBase, 0.6f), 1.0f, optimizeForError);

                reliability = (reliability + colorReliability) * 0.5f;
                reliability = std::clamp(reliability, 0.0f, 1.0f);

                // allow re-weighting to be disabled esily for the viewer demo
                if (!(oneOverK < 1.e6))
                    reliability = 1.0f;
                //
                out[x + y * width] += reliability * images[layer][x + y * width];
            }
        }
    }
    out.saveHdr("out_rw.hdr");
}

//
void main()
{
    //
    std::array<Image, 6> images;
    images[0].load("../assets/8.exr");
    images[1].load("../assets/64.exr");
    images[2].load("../assets/512.exr");
    images[3].load("../assets/4096.exr");
    images[4].load("../assets/32768.exr");
    images[5].load("../assets/262144.exr");
    //
    naive(images);
    reweightingFirefly(images);
}
