#pragma once
#include <afxwin.h>

class CImageProc
{
public:
    CImageProc();
    ~CImageProc();

    void LoadFile(CString stFileName);
    bool HasImage() const;
    void ShowImage(CDC* pDC);
    void ShowImage(CDC* pDC, const CRect& targetRect);

    void GetColor(int x, int y, int& r, int& g, int& b) const;

    bool MakeGrayHistogramImage();
    bool ApplyLinearGrayEnhance();
    bool ApplyHistogramEqualization();
    bool ApplyHistogramSpecification(const CString& referenceFileName);
    bool ApplyPaletteStyle(int style);
    bool AddSaltPepperNoise(double amount = 0.08);
    bool AddImpulseNoise(double amount = 0.06);
    bool AddGaussianNoise(double sigma = 20.0);
    bool ApplyMeanFilter(int kernelSize);
    bool ApplyMedianFilter(int kernelSize);
    bool ApplyMaxFilter(int kernelSize);
    bool ApplySobelEdge(int smoothKernelSize = 3);
    bool ApplyPrewittEdge(int smoothKernelSize = 3);
    bool ApplyLaplacianEdge(int smoothKernelSize = 3);
    bool ApplyLaplacianSharpen();
    bool ApplyCompositeSpatialEnhancement(int smoothKernelSize);
    bool MakeFourierSpectrumImage();
    bool ApplyInverseFftReconstruction();
    bool ApplyIdealLowPassFilter(double cutoffRatio);
    bool ApplyButterworthLowPassFilter(double cutoffRatio, int order);
    bool ApplyIdealHighPassFilter(double cutoffRatio);
    bool ApplyButterworthHighPassFilter(double cutoffRatio, int order);
    bool ApplyHomomorphicFilter(double cutoffRatio);

    HGLOBAL m_hDib;
    BYTE* pDib;

    BITMAPFILEHEADER* pBFH;
    BITMAPINFOHEADER* pBIH;
    RGBQUAD* pQUAD;
    BYTE* pBits;

    CImage m_extImage;
    bool m_isBmp;

    int nWidth;
    int nHeight;
    int nNumColors;

private:
    void ReleaseImage();
};
