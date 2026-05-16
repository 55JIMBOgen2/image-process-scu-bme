#include "pch.h"
#include "CImageProc.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <random>
#include <vector>

namespace
{
    using Histogram = std::array<int, 256>;
    using ChannelHistograms = std::array<Histogram, 3>;
    using ByteMap = std::array<BYTE, 256>;
    using GrayBuffer = std::vector<int>;
    using Complex = std::complex<double>;
    using ComplexBuffer = std::vector<Complex>;

    enum class FrequencyFilterType
    {
        IdealLowPass,
        ButterworthLowPass,
        IdealHighPass,
        ButterworthHighPass
    };

    int ClampByte(int value)
    {
        if (value < 0) return 0;
        if (value > 255) return 255;
        return value;
    }

    double ClampUnit(double value)
    {
        if (value < 0.0) return 0.0;
        if (value > 1.0) return 1.0;
        return value;
    }

    int CalcLineBytes(int width, int bitCount)
    {
        return ((width * bitCount + 31) / 32) * 4;
    }

    int BitmapHeight(const BITMAPINFOHEADER* pBIH)
    {
        if (pBIH == nullptr) return 0;
        return pBIH->biHeight < 0 ? -pBIH->biHeight : pBIH->biHeight;
    }

    BYTE* BmpPixelPtr(const CImageProc* image, int x, int y)
    {
        if (image == nullptr || !image->m_isBmp || image->pBIH == nullptr || image->pBits == nullptr) return nullptr;
        if (x < 0 || x >= image->nWidth || y < 0 || y >= image->nHeight) return nullptr;

        const int realY = image->pBIH->biHeight < 0 ? y : image->nHeight - 1 - y;
        const int lineBytes = CalcLineBytes(image->nWidth, image->nNumColors);
        return image->pBits + realY * lineBytes + (x * image->nNumColors) / 8;
    }

    int PaletteColorCount(const CImageProc* image)
    {
        if (image == nullptr || image->pBIH == nullptr || image->pQUAD == nullptr) return 0;
        if (image->nNumColors > 8) return 0;

        const int maxColors = 1 << image->nNumColors;
        if (image->pBIH->biClrUsed == 0) return maxColors;
        if (image->pBIH->biClrUsed > static_cast<DWORD>(maxColors)) return maxColors;
        return static_cast<int>(image->pBIH->biClrUsed);
    }

    int GrayFromRgb(int r, int g, int b)
    {
        return ClampByte(static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b + 0.5));
    }

    COLORREF StyleColor(int gray, int style)
    {
        gray = ClampByte(gray);

        if (style == 0)
        {
            const int r = ClampByte(static_cast<int>(gray * 1.15 + 45));
            const int g = ClampByte(static_cast<int>(gray * 0.74 + 20));
            const int b = ClampByte(static_cast<int>(gray * 0.35));
            return RGB(r, g, b);
        }

        if (style == 1)
        {
            const int r = ClampByte(static_cast<int>(gray * 0.35));
            const int g = ClampByte(static_cast<int>(gray * 0.86 + 24));
            const int b = ClampByte(static_cast<int>(gray * 1.18 + 42));
            return RGB(r, g, b);
        }

        const double t = gray / 255.0;
        const int r = ClampByte(static_cast<int>(255.0 * ClampUnit(1.5 - std::fabs(4.0 * t - 3.0)) + 0.5));
        const int g = ClampByte(static_cast<int>(255.0 * ClampUnit(1.5 - std::fabs(4.0 * t - 2.0)) + 0.5));
        const int b = ClampByte(static_cast<int>(255.0 * ClampUnit(1.5 - std::fabs(4.0 * t - 1.0)) + 0.5));
        return RGB(r, g, b);
    }

    void SetIndexedGrayPalette(CImageProc* image)
    {
        const int count = PaletteColorCount(image);
        if (count <= 0) return;

        for (int i = 0; i < count; ++i)
        {
            const BYTE value = static_cast<BYTE>(count <= 1 ? 0 : (count == 256 ? i : (i * 255) / (count - 1)));
            image->pQUAD[i].rgbRed = value;
            image->pQUAD[i].rgbGreen = value;
            image->pQUAD[i].rgbBlue = value;
            image->pQUAD[i].rgbReserved = 0;
        }
    }

    bool SetGrayPixel(CImageProc* image, int x, int y, int gray)
    {
        if (image == nullptr || !image->HasImage()) return false;
        const BYTE value = static_cast<BYTE>(ClampByte(gray));

        if (image->m_isBmp)
        {
            BYTE* pixel = BmpPixelPtr(image, x, y);
            if (pixel == nullptr) return false;

            switch (image->nNumColors)
            {
            case 8:
                *pixel = value;
                return true;
            case 16:
            {
                const WORD gray16 = (image->pBIH != nullptr && image->pBIH->biCompression == BI_BITFIELDS)
                    ? static_cast<WORD>(((value >> 3) << 11) | ((value >> 2) << 5) | (value >> 3))
                    : static_cast<WORD>(((value >> 3) << 10) | ((value >> 3) << 5) | (value >> 3));
                *reinterpret_cast<WORD*>(pixel) = gray16;
                return true;
            }
            case 24:
                pixel[0] = value;
                pixel[1] = value;
                pixel[2] = value;
                return true;
            case 32:
                pixel[0] = value;
                pixel[1] = value;
                pixel[2] = value;
                return true;
            default:
                return false;
            }
        }

        image->m_extImage.SetPixel(x, y, RGB(value, value, value));
        return true;
    }

    bool SetColorPixel(CImageProc* image, int x, int y, int r, int g, int b)
    {
        if (image == nullptr || !image->HasImage()) return false;

        const BYTE red = static_cast<BYTE>(ClampByte(r));
        const BYTE green = static_cast<BYTE>(ClampByte(g));
        const BYTE blue = static_cast<BYTE>(ClampByte(b));

        if (image->m_isBmp)
        {
            BYTE* pixel = BmpPixelPtr(image, x, y);
            if (pixel == nullptr) return false;

            switch (image->nNumColors)
            {
            case 16:
            {
                const WORD color16 = (image->pBIH != nullptr && image->pBIH->biCompression == BI_BITFIELDS)
                    ? static_cast<WORD>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3))
                    : static_cast<WORD>(((red >> 3) << 10) | ((green >> 3) << 5) | (blue >> 3));
                *reinterpret_cast<WORD*>(pixel) = color16;
                return true;
            }
            case 24:
                pixel[0] = blue;
                pixel[1] = green;
                pixel[2] = red;
                return true;
            case 32:
                pixel[0] = blue;
                pixel[1] = green;
                pixel[2] = red;
                return true;
            default:
                return false;
            }
        }

        image->m_extImage.SetPixel(x, y, RGB(red, green, blue));
        return true;
    }

    bool BuildGrayHistogram(const CImageProc* image, Histogram& hist)
    {
        hist.fill(0);
        if (image == nullptr || !image->HasImage()) return false;

        for (int y = 0; y < image->nHeight; ++y)
        {
            for (int x = 0; x < image->nWidth; ++x)
            {
                int r = 0;
                int g = 0;
                int b = 0;
                image->GetColor(x, y, r, g, b);
                ++hist[GrayFromRgb(r, g, b)];
            }
        }

        return true;
    }

    bool BuildChannelHistograms(const CImageProc* image, ChannelHistograms& hist)
    {
        for (int i = 0; i < 3; ++i) hist[i].fill(0);
        if (image == nullptr || !image->HasImage()) return false;

        for (int y = 0; y < image->nHeight; ++y)
        {
            for (int x = 0; x < image->nWidth; ++x)
            {
                int r = 0;
                int g = 0;
                int b = 0;
                image->GetColor(x, y, r, g, b);
                ++hist[0][ClampByte(r)];
                ++hist[1][ClampByte(g)];
                ++hist[2][ClampByte(b)];
            }
        }

        return true;
    }

    ByteMap BuildEqualizationMap(const Histogram& hist, int total)
    {
        ByteMap map{};
        int cdf = 0;
        int cdfMin = 0;

        for (int i = 0; i < 256; ++i)
        {
            cdf += hist[i];
            if (cdfMin == 0 && cdf > 0) cdfMin = cdf;
        }

        cdf = 0;
        for (int i = 0; i < 256; ++i)
        {
            cdf += hist[i];
            if (total <= cdfMin)
            {
                map[i] = static_cast<BYTE>(i);
            }
            else
            {
                map[i] = static_cast<BYTE>(ClampByte(static_cast<int>((cdf - cdfMin) * 255.0 / (total - cdfMin) + 0.5)));
            }
        }

        return map;
    }

    ByteMap BuildSpecificationMap(const Histogram& srcHist, const Histogram& refHist, int srcTotal, int refTotal)
    {
        ByteMap map{};
        std::array<double, 256> srcCdf{};
        std::array<double, 256> refCdf{};

        int srcRun = 0;
        int refRun = 0;
        for (int i = 0; i < 256; ++i)
        {
            srcRun += srcHist[i];
            refRun += refHist[i];
            srcCdf[i] = srcTotal > 0 ? srcRun / static_cast<double>(srcTotal) : 0.0;
            refCdf[i] = refTotal > 0 ? refRun / static_cast<double>(refTotal) : 0.0;
        }

        int refValue = 0;
        for (int i = 0; i < 256; ++i)
        {
            while (refValue < 255 && refCdf[refValue] < srcCdf[i])
            {
                ++refValue;
            }

            if (refValue > 0)
            {
                const double prevDiff = std::fabs(refCdf[refValue - 1] - srcCdf[i]);
                const double currDiff = std::fabs(refCdf[refValue] - srcCdf[i]);
                map[i] = static_cast<BYTE>(prevDiff < currDiff ? refValue - 1 : refValue);
            }
            else
            {
                map[i] = static_cast<BYTE>(refValue);
            }
        }

        return map;
    }

    int NormalizeKernelSize(int kernelSize)
    {
        if (kernelSize < 3) return 3;
        if (kernelSize > 7) return 7;
        if (kernelSize % 2 == 0) ++kernelSize;
        return kernelSize;
    }

    int ClampIndex(int value, int maxValue)
    {
        if (value < 0) return 0;
        if (value >= maxValue) return maxValue - 1;
        return value;
    }

    bool BuildGrayBuffer(const CImageProc* image, GrayBuffer& out)
    {
        if (image == nullptr || !image->HasImage()) return false;

        out.assign(image->nWidth * image->nHeight, 0);
        for (int y = 0; y < image->nHeight; ++y)
        {
            for (int x = 0; x < image->nWidth; ++x)
            {
                int r = 0;
                int g = 0;
                int b = 0;
                image->GetColor(x, y, r, g, b);
                out[y * image->nWidth + x] = GrayFromRgb(r, g, b);
            }
        }

        return true;
    }

    bool WriteGrayBuffer(CImageProc* image, const GrayBuffer& buffer)
    {
        if (image == nullptr || !image->HasImage()) return false;
        if (buffer.size() != static_cast<size_t>(image->nWidth * image->nHeight)) return false;
        if (image->m_isBmp && image->nNumColors == 8) SetIndexedGrayPalette(image);

        for (int y = 0; y < image->nHeight; ++y)
        {
            for (int x = 0; x < image->nWidth; ++x)
            {
                if (!SetGrayPixel(image, x, y, buffer[y * image->nWidth + x])) return false;
            }
        }

        return true;
    }

    GrayBuffer ApplyMeanToBuffer(const GrayBuffer& src, int width, int height, int kernelSize)
    {
        kernelSize = NormalizeKernelSize(kernelSize);
        const int radius = kernelSize / 2;
        const int area = kernelSize * kernelSize;
        GrayBuffer dst(width * height, 0);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int sum = 0;
                for (int ky = -radius; ky <= radius; ++ky)
                {
                    const int yy = ClampIndex(y + ky, height);
                    for (int kx = -radius; kx <= radius; ++kx)
                    {
                        const int xx = ClampIndex(x + kx, width);
                        sum += src[yy * width + xx];
                    }
                }
                dst[y * width + x] = ClampByte((sum + area / 2) / area);
            }
        }

        return dst;
    }

    GrayBuffer ApplyMedianToBuffer(const GrayBuffer& src, int width, int height, int kernelSize)
    {
        kernelSize = NormalizeKernelSize(kernelSize);
        const int radius = kernelSize / 2;
        GrayBuffer dst(width * height, 0);
        std::vector<int> values;
        values.reserve(kernelSize * kernelSize);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                values.clear();
                for (int ky = -radius; ky <= radius; ++ky)
                {
                    const int yy = ClampIndex(y + ky, height);
                    for (int kx = -radius; kx <= radius; ++kx)
                    {
                        const int xx = ClampIndex(x + kx, width);
                        values.push_back(src[yy * width + xx]);
                    }
                }

                const size_t mid = values.size() / 2;
                std::nth_element(values.begin(), values.begin() + mid, values.end());
                dst[y * width + x] = values[mid];
            }
        }

        return dst;
    }

    GrayBuffer ApplyMaxToBuffer(const GrayBuffer& src, int width, int height, int kernelSize)
    {
        kernelSize = NormalizeKernelSize(kernelSize);
        const int radius = kernelSize / 2;
        GrayBuffer dst(width * height, 0);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int maxValue = 0;
                for (int ky = -radius; ky <= radius; ++ky)
                {
                    const int yy = ClampIndex(y + ky, height);
                    for (int kx = -radius; kx <= radius; ++kx)
                    {
                        const int xx = ClampIndex(x + kx, width);
                        if (src[yy * width + xx] > maxValue) maxValue = src[yy * width + xx];
                    }
                }
                dst[y * width + x] = maxValue;
            }
        }

        return dst;
    }

    GrayBuffer ApplyGradientEdgeToBuffer(const GrayBuffer& src, int width, int height, bool useSobel)
    {
        static const int sobelX[3][3] = {
            {-1, 0, 1},
            {-2, 0, 2},
            {-1, 0, 1}
        };
        static const int sobelY[3][3] = {
            {-1, -2, -1},
            { 0,  0,  0},
            { 1,  2,  1}
        };
        static const int prewittX[3][3] = {
            {-1, 0, 1},
            {-1, 0, 1},
            {-1, 0, 1}
        };
        static const int prewittY[3][3] = {
            {-1, -1, -1},
            { 0,  0,  0},
            { 1,  1,  1}
        };

        GrayBuffer dst(width * height, 0);
        const int(*kx)[3] = useSobel ? sobelX : prewittX;
        const int(*ky)[3] = useSobel ? sobelY : prewittY;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int gx = 0;
                int gy = 0;
                for (int j = -1; j <= 1; ++j)
                {
                    const int yy = ClampIndex(y + j, height);
                    for (int i = -1; i <= 1; ++i)
                    {
                        const int xx = ClampIndex(x + i, width);
                        const int value = src[yy * width + xx];
                        gx += value * kx[j + 1][i + 1];
                        gy += value * ky[j + 1][i + 1];
                    }
                }

                dst[y * width + x] = ClampByte(static_cast<int>(std::sqrt(static_cast<double>(gx * gx + gy * gy)) + 0.5));
            }
        }

        return dst;
    }

    GrayBuffer ApplyLaplacianEdgeToBuffer(const GrayBuffer& src, int width, int height)
    {
        GrayBuffer dst(width * height, 0);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int center = src[y * width + x] * 8;
                int around = 0;
                for (int j = -1; j <= 1; ++j)
                {
                    const int yy = ClampIndex(y + j, height);
                    for (int i = -1; i <= 1; ++i)
                    {
                        if (i == 0 && j == 0) continue;
                        const int xx = ClampIndex(x + i, width);
                        around += src[yy * width + xx];
                    }
                }

                dst[y * width + x] = ClampByte(std::abs(center - around));
            }
        }

        return dst;
    }

    GrayBuffer ApplyLaplacianSharpenToBuffer(const GrayBuffer& src, int width, int height)
    {
        GrayBuffer dst(width * height, 0);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int center = src[y * width + x] * 8;
                int around = 0;
                for (int j = -1; j <= 1; ++j)
                {
                    const int yy = ClampIndex(y + j, height);
                    for (int i = -1; i <= 1; ++i)
                    {
                        if (i == 0 && j == 0) continue;
                        const int xx = ClampIndex(x + i, width);
                        around += src[yy * width + xx];
                    }
                }

                const int laplacian = center - around;
                dst[y * width + x] = ClampByte(src[y * width + x] + laplacian);
            }
        }

        return dst;
    }

    int RoundToByte(double value)
    {
        if (value <= 0.0) return 0;
        if (value >= 255.0) return 255;
        return static_cast<int>(value + 0.5);
    }

    int NextPowerOfTwo(int value)
    {
        int result = 1;
        while (result < value && result < (1 << 30))
        {
            result <<= 1;
        }
        return result;
    }

    double NormalizeFrequencyCutoff(double cutoffRatio)
    {
        if (cutoffRatio <= 0.0) return 0.10;
        if (cutoffRatio > 0.45) return 0.45;
        return cutoffRatio;
    }

    int NormalizeButterworthOrder(int order)
    {
        if (order < 1) return 1;
        if (order > 8) return 8;
        return order;
    }

    void Fft1D(std::vector<Complex>& data, bool inverse)
    {
        const int n = static_cast<int>(data.size());
        int j = 0;
        for (int i = 1; i < n; ++i)
        {
            int bit = n >> 1;
            for (; (j & bit) != 0; bit >>= 1)
            {
                j ^= bit;
            }
            j ^= bit;

            if (i < j)
            {
                std::swap(data[i], data[j]);
            }
        }

        const double pi = std::acos(-1.0);
        for (int len = 2; len <= n; len <<= 1)
        {
            const double angle = (inverse ? 2.0 : -2.0) * pi / len;
            const Complex wlen(std::cos(angle), std::sin(angle));

            for (int i = 0; i < n; i += len)
            {
                Complex w(1.0, 0.0);
                for (int k = 0; k < len / 2; ++k)
                {
                    const Complex even = data[i + k];
                    const Complex odd = data[i + k + len / 2] * w;
                    data[i + k] = even + odd;
                    data[i + k + len / 2] = even - odd;
                    w *= wlen;
                }
            }
        }

        if (inverse)
        {
            for (int i = 0; i < n; ++i)
            {
                data[i] /= static_cast<double>(n);
            }
        }
    }

    void Fft2D(ComplexBuffer& data, int width, int height, bool inverse)
    {
        std::vector<Complex> row(width);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                row[x] = data[y * width + x];
            }

            Fft1D(row, inverse);

            for (int x = 0; x < width; ++x)
            {
                data[y * width + x] = row[x];
            }
        }

        std::vector<Complex> column(height);
        for (int x = 0; x < width; ++x)
        {
            for (int y = 0; y < height; ++y)
            {
                column[y] = data[y * width + x];
            }

            Fft1D(column, inverse);

            for (int y = 0; y < height; ++y)
            {
                data[y * width + x] = column[y];
            }
        }
    }

    bool BuildCenteredFourier(const GrayBuffer& src, int width, int height, bool useLogInput,
        ComplexBuffer& freq, int& fftWidth, int& fftHeight)
    {
        if (width <= 0 || height <= 0) return false;
        if (src.size() != static_cast<size_t>(width * height)) return false;

        fftWidth = NextPowerOfTwo(width);
        fftHeight = NextPowerOfTwo(height);
        freq.assign(static_cast<size_t>(fftWidth) * fftHeight, Complex(0.0, 0.0));

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                double value = static_cast<double>(src[y * width + x]);
                if (useLogInput)
                {
                    value = std::log(value + 1.0);
                }
                if (((x + y) & 1) != 0)
                {
                    value = -value;
                }
                freq[y * fftWidth + x] = Complex(value, 0.0);
            }
        }

        Fft2D(freq, fftWidth, fftHeight, false);
        return true;
    }

    std::vector<double> InverseCenteredFourierToValues(ComplexBuffer freq,
        int fftWidth, int fftHeight, int width, int height)
    {
        Fft2D(freq, fftWidth, fftHeight, true);

        std::vector<double> values(static_cast<size_t>(width) * height, 0.0);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                double value = freq[y * fftWidth + x].real();
                if (((x + y) & 1) != 0)
                {
                    value = -value;
                }
                values[y * width + x] = value;
            }
        }

        return values;
    }

    GrayBuffer MakeSpectrumBuffer(const ComplexBuffer& freq, int fftWidth, int fftHeight, int width, int height)
    {
        std::vector<double> magnitudes(freq.size(), 0.0);
        double maxMagnitude = 0.0;

        for (size_t i = 0; i < freq.size(); ++i)
        {
            const double value = std::log(1.0 + std::abs(freq[i]));
            magnitudes[i] = value;
            if (value > maxMagnitude)
            {
                maxMagnitude = value;
            }
        }

        GrayBuffer dst(static_cast<size_t>(width) * height, 0);
        if (maxMagnitude <= 0.0) return dst;

        for (int y = 0; y < height; ++y)
        {
            const int fy = y * fftHeight / height;
            for (int x = 0; x < width; ++x)
            {
                const int fx = x * fftWidth / width;
                const double scaled = magnitudes[fy * fftWidth + fx] * 255.0 / maxMagnitude;
                dst[y * width + x] = RoundToByte(scaled);
            }
        }

        return dst;
    }

    void ApplyFrequencyMask(ComplexBuffer& freq, int fftWidth, int fftHeight,
        double cutoffRatio, int order, FrequencyFilterType type)
    {
        const int baseSize = fftWidth < fftHeight ? fftWidth : fftHeight;
        double cutoff = NormalizeFrequencyCutoff(cutoffRatio) * baseSize;
        if (cutoff < 1.0) cutoff = 1.0;
        const int normalizedOrder = NormalizeButterworthOrder(order);
        const double centerX = fftWidth / 2.0;
        const double centerY = fftHeight / 2.0;
        const bool isButterworth = type == FrequencyFilterType::ButterworthLowPass ||
            type == FrequencyFilterType::ButterworthHighPass;
        const bool isHighPass = type == FrequencyFilterType::IdealHighPass ||
            type == FrequencyFilterType::ButterworthHighPass;

        for (int v = 0; v < fftHeight; ++v)
        {
            const double dy = v - centerY;
            for (int u = 0; u < fftWidth; ++u)
            {
                const double dx = u - centerX;
                const double distance = std::sqrt(dx * dx + dy * dy);
                double lowPass = 0.0;

                if (isButterworth)
                {
                    lowPass = 1.0 / (1.0 + std::pow(distance / cutoff, 2.0 * normalizedOrder));
                }
                else
                {
                    lowPass = distance <= cutoff ? 1.0 : 0.0;
                }

                const double mask = isHighPass ? 1.0 - lowPass : lowPass;
                freq[v * fftWidth + u] *= mask;
            }
        }
    }

    GrayBuffer ApplyFrequencyFilterToBuffer(const GrayBuffer& src, int width, int height,
        double cutoffRatio, int order, FrequencyFilterType type)
    {
        ComplexBuffer freq;
        int fftWidth = 0;
        int fftHeight = 0;
        if (!BuildCenteredFourier(src, width, height, false, freq, fftWidth, fftHeight))
        {
            return GrayBuffer();
        }

        ApplyFrequencyMask(freq, fftWidth, fftHeight, cutoffRatio, order, type);
        const std::vector<double> values = InverseCenteredFourierToValues(freq, fftWidth, fftHeight, width, height);
        const bool isHighPass = type == FrequencyFilterType::IdealHighPass ||
            type == FrequencyFilterType::ButterworthHighPass;

        GrayBuffer dst(static_cast<size_t>(width) * height, 0);
        for (size_t i = 0; i < dst.size(); ++i)
        {
            const double value = isHighPass ? static_cast<double>(src[i]) + values[i] : values[i];
            dst[i] = RoundToByte(value);
        }

        return dst;
    }

    GrayBuffer ApplyHomomorphicToBuffer(const GrayBuffer& src, int width, int height, double cutoffRatio)
    {
        ComplexBuffer freq;
        int fftWidth = 0;
        int fftHeight = 0;
        if (!BuildCenteredFourier(src, width, height, true, freq, fftWidth, fftHeight))
        {
            return GrayBuffer();
        }

        const int baseSize = fftWidth < fftHeight ? fftWidth : fftHeight;
        double cutoff = NormalizeFrequencyCutoff(cutoffRatio) * baseSize;
        if (cutoff < 1.0) cutoff = 1.0;
        const double gammaLow = 0.55;
        const double gammaHigh = 1.75;
        const double c = 1.0;
        const double centerX = fftWidth / 2.0;
        const double centerY = fftHeight / 2.0;

        for (int v = 0; v < fftHeight; ++v)
        {
            const double dy = v - centerY;
            for (int u = 0; u < fftWidth; ++u)
            {
                const double dx = u - centerX;
                const double distance2 = dx * dx + dy * dy;
                const double mask = (gammaHigh - gammaLow) *
                    (1.0 - std::exp(-c * distance2 / (cutoff * cutoff))) + gammaLow;
                freq[v * fftWidth + u] *= mask;
            }
        }

        const std::vector<double> logValues = InverseCenteredFourierToValues(freq, fftWidth, fftHeight, width, height);
        std::vector<double> values(logValues.size(), 0.0);
        double minValue = 0.0;
        double maxValue = 0.0;
        bool hasValue = false;

        for (size_t i = 0; i < logValues.size(); ++i)
        {
            double value = std::exp(logValues[i]) - 1.0;
            if (!std::isfinite(value))
            {
                value = 0.0;
            }
            values[i] = value;

            if (!hasValue)
            {
                minValue = value;
                maxValue = value;
                hasValue = true;
            }
            else
            {
                if (value < minValue) minValue = value;
                if (value > maxValue) maxValue = value;
            }
        }

        GrayBuffer dst(values.size(), 0);
        if (!hasValue || maxValue <= minValue)
        {
            for (size_t i = 0; i < values.size(); ++i)
            {
                dst[i] = RoundToByte(values[i]);
            }
            return dst;
        }

        for (size_t i = 0; i < values.size(); ++i)
        {
            dst[i] = RoundToByte((values[i] - minValue) * 255.0 / (maxValue - minValue));
        }

        return dst;
    }
}

CImageProc::CImageProc()
{
    m_hDib = NULL;
    pDib = nullptr;
    pBFH = nullptr;
    pBIH = nullptr;
    pQUAD = nullptr;
    pBits = nullptr;
    m_isBmp = true;
    nWidth = 0;
    nHeight = 0;
    nNumColors = 0;
}

CImageProc::~CImageProc()
{
    ReleaseImage();
}

void CImageProc::ReleaseImage()
{
    if (m_hDib != NULL)
    {
        ::GlobalUnlock(m_hDib);
        ::GlobalFree(m_hDib);
    }

    if (!m_extImage.IsNull())
    {
        m_extImage.Destroy();
    }

    m_hDib = NULL;
    pDib = nullptr;
    pBFH = nullptr;
    pBIH = nullptr;
    pQUAD = nullptr;
    pBits = nullptr;
    m_isBmp = true;
    nWidth = 0;
    nHeight = 0;
    nNumColors = 0;
}

void CImageProc::LoadFile(CString stFileName)
{
    ReleaseImage();

    int dotPos = stFileName.ReverseFind(_T('.'));
    CString ext;
    if (dotPos >= 0)
    {
        ext = stFileName.Mid(dotPos);
        ext.MakeLower();
    }

    if (ext == _T(".bmp"))
    {
        CFile file;
        if (!file.Open(stFileName, CFile::modeRead | CFile::shareDenyWrite)) return;

        const ULONGLONG fileSize64 = file.GetLength();
        if (fileSize64 < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) || fileSize64 > 0xFFFFFFFFULL)
        {
            file.Close();
            return;
        }

        const UINT fileSize = static_cast<UINT>(fileSize64);
        m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, fileSize);
        if (m_hDib == NULL)
        {
            file.Close();
            return;
        }

        pDib = static_cast<BYTE*>(::GlobalLock(m_hDib));
        if (pDib == nullptr)
        {
            file.Close();
            ReleaseImage();
            return;
        }

        const UINT bytesRead = file.Read(pDib, fileSize);
        file.Close();

        if (bytesRead != fileSize)
        {
            ReleaseImage();
            return;
        }

        pBFH = reinterpret_cast<BITMAPFILEHEADER*>(pDib);
        pBIH = reinterpret_cast<BITMAPINFOHEADER*>(pDib + sizeof(BITMAPFILEHEADER));
        if (pBFH->bfType != 0x4D42 || pBFH->bfOffBits >= fileSize || pBIH->biSize < sizeof(BITMAPINFOHEADER))
        {
            ReleaseImage();
            return;
        }

        pBits = pDib + pBFH->bfOffBits;
        pQUAD = pBIH->biBitCount <= 8
            ? reinterpret_cast<RGBQUAD*>(pDib + sizeof(BITMAPFILEHEADER) + pBIH->biSize)
            : nullptr;

        m_isBmp = true;
        nWidth = pBIH->biWidth;
        nHeight = BitmapHeight(pBIH);
        nNumColors = pBIH->biBitCount;
        return;
    }

    m_isBmp = false;
    if (FAILED(m_extImage.Load(stFileName)) || m_extImage.IsNull())
    {
        ReleaseImage();
        return;
    }

    nWidth = m_extImage.GetWidth();
    nHeight = m_extImage.GetHeight();
    nNumColors = m_extImage.GetBPP();
}

bool CImageProc::HasImage() const
{
    if (nWidth <= 0 || nHeight <= 0) return false;
    if (m_isBmp) return m_hDib != NULL && pDib != nullptr && pBits != nullptr && pBIH != nullptr;
    return !m_extImage.IsNull();
}

void CImageProc::ShowImage(CDC* pDC)
{
    ShowImage(pDC, CRect(0, 0, nWidth, nHeight));
}

void CImageProc::ShowImage(CDC* pDC, const CRect& targetRect)
{
    if (pDC == nullptr || targetRect.Width() <= 0 || targetRect.Height() <= 0 || !HasImage()) return;

    if (m_isBmp)
    {
        ::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
        ::StretchDIBits(
            pDC->m_hDC,
            targetRect.left, targetRect.top,
            targetRect.Width(), targetRect.Height(),
            0, 0, nWidth, nHeight,
            pBits, reinterpret_cast<BITMAPINFO*>(pBIH), DIB_RGB_COLORS, SRCCOPY
        );
    }
    else
    {
        m_extImage.Draw(
            pDC->m_hDC,
            targetRect.left, targetRect.top,
            targetRect.Width(), targetRect.Height()
        );
    }
}

void CImageProc::GetColor(int x, int y, int& r, int& g, int& b) const
{
    r = 0;
    g = 0;
    b = 0;

    if (x < 0 || x >= nWidth || y < 0 || y >= nHeight || !HasImage()) return;

    if (m_isBmp)
    {
        BYTE* pixel = BmpPixelPtr(this, x, y);
        if (pixel == nullptr) return;

        switch (nNumColors)
        {
        case 1:
        {
            const int index = (*pixel & (0x80 >> (x % 8))) ? 1 : 0;
            if (index < PaletteColorCount(this))
            {
                r = pQUAD[index].rgbRed;
                g = pQUAD[index].rgbGreen;
                b = pQUAD[index].rgbBlue;
            }
            break;
        }
        case 4:
        {
            const int index = (x % 2 == 0) ? ((*pixel >> 4) & 0x0F) : (*pixel & 0x0F);
            if (index < PaletteColorCount(this))
            {
                r = pQUAD[index].rgbRed;
                g = pQUAD[index].rgbGreen;
                b = pQUAD[index].rgbBlue;
            }
            break;
        }
        case 8:
        {
            const int index = *pixel;
            if (index < PaletteColorCount(this))
            {
                r = pQUAD[index].rgbRed;
                g = pQUAD[index].rgbGreen;
                b = pQUAD[index].rgbBlue;
            }
            else
            {
                r = index;
                g = index;
                b = index;
            }
            break;
        }
        case 16:
        {
            const WORD color16 = *reinterpret_cast<WORD*>(pixel);
            if (pBIH->biCompression == BI_BITFIELDS)
            {
                const int r5 = (color16 >> 11) & 0x1F;
                const int g6 = (color16 >> 5) & 0x3F;
                const int b5 = color16 & 0x1F;
                r = (r5 << 3) | (r5 >> 2);
                g = (g6 << 2) | (g6 >> 4);
                b = (b5 << 3) | (b5 >> 2);
            }
            else
            {
                const int r5 = (color16 >> 10) & 0x1F;
                const int g5 = (color16 >> 5) & 0x1F;
                const int b5 = color16 & 0x1F;
                r = (r5 << 3) | (r5 >> 2);
                g = (g5 << 3) | (g5 >> 2);
                b = (b5 << 3) | (b5 >> 2);
            }
            break;
        }
        case 24:
            b = pixel[0];
            g = pixel[1];
            r = pixel[2];
            break;
        case 32:
            b = pixel[0];
            g = pixel[1];
            r = pixel[2];
            break;
        default:
            break;
        }
    }
    else
    {
        const COLORREF color = m_extImage.GetPixel(x, y);
        r = GetRValue(color);
        g = GetGValue(color);
        b = GetBValue(color);
    }
}

bool CImageProc::MakeGrayHistogramImage()
{
    Histogram hist;
    if (!BuildGrayHistogram(this, hist)) return false;

    const int maxCount = *std::max_element(hist.begin(), hist.end());
    if (maxCount <= 0) return false;

    const int width = 512;
    const int height = 320;
    const int bitCount = 24;
    const int lineBytes = CalcLineBytes(width, bitCount);
    const DWORD imageBytes = static_cast<DWORD>(lineBytes * height);
    const DWORD headerBytes = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    const DWORD fileBytes = headerBytes + imageBytes;

    ReleaseImage();

    m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, fileBytes);
    if (m_hDib == NULL) return false;

    pDib = static_cast<BYTE*>(::GlobalLock(m_hDib));
    if (pDib == nullptr)
    {
        ReleaseImage();
        return false;
    }

    pBFH = reinterpret_cast<BITMAPFILEHEADER*>(pDib);
    pBIH = reinterpret_cast<BITMAPINFOHEADER*>(pDib + sizeof(BITMAPFILEHEADER));
    pBits = pDib + headerBytes;
    pQUAD = nullptr;

    pBFH->bfType = 0x4D42;
    pBFH->bfSize = fileBytes;
    pBFH->bfReserved1 = 0;
    pBFH->bfReserved2 = 0;
    pBFH->bfOffBits = headerBytes;

    pBIH->biSize = sizeof(BITMAPINFOHEADER);
    pBIH->biWidth = width;
    pBIH->biHeight = height;
    pBIH->biPlanes = 1;
    pBIH->biBitCount = bitCount;
    pBIH->biCompression = BI_RGB;
    pBIH->biSizeImage = imageBytes;
    pBIH->biXPelsPerMeter = 0;
    pBIH->biYPelsPerMeter = 0;
    pBIH->biClrUsed = 0;
    pBIH->biClrImportant = 0;

    m_isBmp = true;
    nWidth = width;
    nHeight = height;
    nNumColors = bitCount;

    auto putPixel = [&](int x, int y, BYTE r, BYTE g, BYTE b)
    {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        const int realY = height - 1 - y;
        BYTE* pixel = pBits + realY * lineBytes + x * 3;
        pixel[0] = b;
        pixel[1] = g;
        pixel[2] = r;
    };

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            putPixel(x, y, 255, 255, 255);
        }
    }

    const int left = 42;
    const int top = 24;
    const int right = 494;
    const int bottom = 282;
    const int plotWidth = right - left;
    const int plotHeight = bottom - top;

    for (int x = left; x <= right; ++x) putPixel(x, bottom, 30, 30, 30);
    for (int y = top; y <= bottom; ++y) putPixel(left, y, 30, 30, 30);

    for (int i = 0; i < 256; ++i)
    {
        int x0 = left + i * plotWidth / 256;
        int x1 = left + (i + 1) * plotWidth / 256;
        if (x1 <= x0) x1 = x0 + 1;

        const int barHeight = static_cast<int>(static_cast<long long>(hist[i]) * plotHeight / maxCount);
        const BYTE shade = static_cast<BYTE>(ClampByte(40 + i * 180 / 255));
        for (int x = x0; x < x1; ++x)
        {
            for (int y = bottom - barHeight; y < bottom; ++y)
            {
                putPixel(x, y, shade, shade, 220);
            }
        }
    }

    return true;
}

bool CImageProc::ApplyLinearGrayEnhance()
{
    Histogram hist;
    if (!BuildGrayHistogram(this, hist)) return false;

    int minGray = 0;
    while (minGray < 256 && hist[minGray] == 0) ++minGray;

    int maxGray = 255;
    while (maxGray >= 0 && hist[maxGray] == 0) --maxGray;

    if (minGray >= maxGray) return false;
    if (m_isBmp && nNumColors == 8) SetIndexedGrayPalette(this);

    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            int r = 0;
            int g = 0;
            int b = 0;
            GetColor(x, y, r, g, b);
            const int gray = GrayFromRgb(r, g, b);
            const int enhanced = (gray - minGray) * 255 / (maxGray - minGray);
            if (!SetGrayPixel(this, x, y, enhanced)) return false;
        }
    }

    return true;
}

bool CImageProc::ApplyHistogramEqualization()
{
    Histogram hist;
    if (!BuildGrayHistogram(this, hist)) return false;

    const int total = nWidth * nHeight;
    const ByteMap map = BuildEqualizationMap(hist, total);
    if (m_isBmp && nNumColors == 8) SetIndexedGrayPalette(this);

    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            int r = 0;
            int g = 0;
            int b = 0;
            GetColor(x, y, r, g, b);
            const int gray = GrayFromRgb(r, g, b);
            if (!SetGrayPixel(this, x, y, map[gray])) return false;
        }
    }

    return true;
}

bool CImageProc::ApplyHistogramSpecification(const CString& referenceFileName)
{
    if (!HasImage()) return false;

    CImageProc reference;
    reference.LoadFile(referenceFileName);
    if (!reference.HasImage()) return false;

    ChannelHistograms sourceHist;
    ChannelHistograms referenceHist;
    if (!BuildChannelHistograms(this, sourceHist)) return false;
    if (!BuildChannelHistograms(&reference, referenceHist)) return false;

    std::array<ByteMap, 3> maps{};
    const int sourceTotal = nWidth * nHeight;
    const int referenceTotal = reference.nWidth * reference.nHeight;
    for (int c = 0; c < 3; ++c)
    {
        maps[c] = BuildSpecificationMap(sourceHist[c], referenceHist[c], sourceTotal, referenceTotal);
    }

    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            int r = 0;
            int g = 0;
            int b = 0;
            GetColor(x, y, r, g, b);
            if (!SetColorPixel(this, x, y, maps[0][ClampByte(r)], maps[1][ClampByte(g)], maps[2][ClampByte(b)])) return false;
        }
    }

    return true;
}

bool CImageProc::ApplyPaletteStyle(int style)
{
    if (!HasImage()) return false;

    if (m_isBmp && nNumColors <= 8 && pQUAD != nullptr)
    {
        const int count = PaletteColorCount(this);
        if (count <= 0) return false;

        for (int i = 0; i < count; ++i)
        {
            const int gray = GrayFromRgb(pQUAD[i].rgbRed, pQUAD[i].rgbGreen, pQUAD[i].rgbBlue);
            const COLORREF color = StyleColor(gray, style);
            pQUAD[i].rgbRed = GetRValue(color);
            pQUAD[i].rgbGreen = GetGValue(color);
            pQUAD[i].rgbBlue = GetBValue(color);
            pQUAD[i].rgbReserved = 0;
        }
        return true;
    }

    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            int r = 0;
            int g = 0;
            int b = 0;
            GetColor(x, y, r, g, b);
            const COLORREF color = StyleColor(GrayFromRgb(r, g, b), style);
            if (!SetColorPixel(this, x, y, GetRValue(color), GetGValue(color), GetBValue(color))) return false;
        }
    }

    return true;
}

bool CImageProc::AddSaltPepperNoise(double amount)
{
    if (!HasImage()) return false;
    amount = ClampUnit(amount);

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    GrayBuffer dst = src;

    for (size_t i = 0; i < dst.size(); ++i)
    {
        const double p = prob(rng);
        if (p < amount * 0.5)
        {
            dst[i] = 0;
        }
        else if (p < amount)
        {
            dst[i] = 255;
        }
    }

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::AddImpulseNoise(double amount)
{
    if (!HasImage()) return false;
    amount = ClampUnit(amount);

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    std::uniform_int_distribution<int> impulseValue(0, 255);
    GrayBuffer dst = src;

    for (size_t i = 0; i < dst.size(); ++i)
    {
        if (prob(rng) < amount)
        {
            dst[i] = impulseValue(rng);
        }
    }

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::AddGaussianNoise(double sigma)
{
    if (!HasImage()) return false;
    if (sigma < 0.0) sigma = 0.0;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    std::mt19937 rng(std::random_device{}());
    std::normal_distribution<double> gaussian(0.0, sigma);
    GrayBuffer dst(src.size(), 0);

    for (size_t i = 0; i < src.size(); ++i)
    {
        dst[i] = ClampByte(static_cast<int>(src[i] + gaussian(rng) + 0.5));
    }

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyMeanFilter(int kernelSize)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;
    GrayBuffer dst = ApplyMeanToBuffer(src, nWidth, nHeight, kernelSize);
    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyMedianFilter(int kernelSize)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;
    GrayBuffer dst = ApplyMedianToBuffer(src, nWidth, nHeight, kernelSize);
    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyMaxFilter(int kernelSize)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;
    GrayBuffer dst = ApplyMaxToBuffer(src, nWidth, nHeight, kernelSize);
    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplySobelEdge(int smoothKernelSize)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;
    if (NormalizeKernelSize(smoothKernelSize) > 3)
    {
        src = ApplyMeanToBuffer(src, nWidth, nHeight, smoothKernelSize);
    }
    GrayBuffer dst = ApplyGradientEdgeToBuffer(src, nWidth, nHeight, true);
    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyPrewittEdge(int smoothKernelSize)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;
    if (NormalizeKernelSize(smoothKernelSize) > 3)
    {
        src = ApplyMeanToBuffer(src, nWidth, nHeight, smoothKernelSize);
    }
    GrayBuffer dst = ApplyGradientEdgeToBuffer(src, nWidth, nHeight, false);
    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyLaplacianEdge(int smoothKernelSize)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;
    if (NormalizeKernelSize(smoothKernelSize) > 3)
    {
        src = ApplyMeanToBuffer(src, nWidth, nHeight, smoothKernelSize);
    }
    GrayBuffer dst = ApplyLaplacianEdgeToBuffer(src, nWidth, nHeight);
    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyLaplacianSharpen()
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;
    GrayBuffer dst = ApplyLaplacianSharpenToBuffer(src, nWidth, nHeight);
    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyCompositeSpatialEnhancement(int smoothKernelSize)
{
    if (!HasImage()) return false;

    GrayBuffer original;
    if (!BuildGrayBuffer(this, original)) return false;

    const GrayBuffer sharpened = ApplyLaplacianSharpenToBuffer(original, nWidth, nHeight);
    const GrayBuffer sobel = ApplyGradientEdgeToBuffer(original, nWidth, nHeight, true);
    const GrayBuffer smoothSobel = ApplyMeanToBuffer(sobel, nWidth, nHeight, smoothKernelSize);

    GrayBuffer dst(original.size(), 0);
    for (size_t i = 0; i < original.size(); ++i)
    {
        const int mask = sharpened[i] * smoothSobel[i] / 255;
        const int combined = ClampByte(original[i] + mask);
        const double normalized = combined / 255.0;
        dst[i] = ClampByte(static_cast<int>(std::pow(normalized, 0.5) * 255.0 + 0.5));
    }

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::MakeFourierSpectrumImage()
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    ComplexBuffer freq;
    int fftWidth = 0;
    int fftHeight = 0;
    if (!BuildCenteredFourier(src, nWidth, nHeight, false, freq, fftWidth, fftHeight)) return false;

    const GrayBuffer dst = MakeSpectrumBuffer(freq, fftWidth, fftHeight, nWidth, nHeight);
    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyInverseFftReconstruction()
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    ComplexBuffer freq;
    int fftWidth = 0;
    int fftHeight = 0;
    if (!BuildCenteredFourier(src, nWidth, nHeight, false, freq, fftWidth, fftHeight)) return false;

    const std::vector<double> values = InverseCenteredFourierToValues(freq, fftWidth, fftHeight, nWidth, nHeight);
    GrayBuffer dst(values.size(), 0);
    for (size_t i = 0; i < values.size(); ++i)
    {
        dst[i] = RoundToByte(values[i]);
    }

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyIdealLowPassFilter(double cutoffRatio)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    const GrayBuffer dst = ApplyFrequencyFilterToBuffer(src, nWidth, nHeight,
        cutoffRatio, 1, FrequencyFilterType::IdealLowPass);
    if (dst.empty()) return false;

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyButterworthLowPassFilter(double cutoffRatio, int order)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    const GrayBuffer dst = ApplyFrequencyFilterToBuffer(src, nWidth, nHeight,
        cutoffRatio, order, FrequencyFilterType::ButterworthLowPass);
    if (dst.empty()) return false;

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyIdealHighPassFilter(double cutoffRatio)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    const GrayBuffer dst = ApplyFrequencyFilterToBuffer(src, nWidth, nHeight,
        cutoffRatio, 1, FrequencyFilterType::IdealHighPass);
    if (dst.empty()) return false;

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyButterworthHighPassFilter(double cutoffRatio, int order)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    const GrayBuffer dst = ApplyFrequencyFilterToBuffer(src, nWidth, nHeight,
        cutoffRatio, order, FrequencyFilterType::ButterworthHighPass);
    if (dst.empty()) return false;

    return WriteGrayBuffer(this, dst);
}

bool CImageProc::ApplyHomomorphicFilter(double cutoffRatio)
{
    if (!HasImage()) return false;

    GrayBuffer src;
    if (!BuildGrayBuffer(this, src)) return false;

    const GrayBuffer dst = ApplyHomomorphicToBuffer(src, nWidth, nHeight, cutoffRatio);
    if (dst.empty()) return false;

    return WriteGrayBuffer(this, dst);
}
