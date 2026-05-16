// MyPhotoshopDoc.h: CMyPhotoshopDoc 类的接口
//

#pragma once

#include "CImageProc.h" // 引入自定义类的头文件 [cite: 7]

class CMyPhotoshopDoc : public CDocument
{
protected: // 仅从序列化创建
	CMyPhotoshopDoc() noexcept;
	DECLARE_DYNCREATE(CMyPhotoshopDoc)

	// 特性
public:
	CImageProc* pImage; // 定义指向数据处理类的指针 [cite: 9]

	// 操作
public:

	// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

	// 实现
public:
	virtual ~CMyPhotoshopDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnFileOpen(); // 声明“打开”菜单项的消息响应函数 [cite: 21-22]
	afx_msg void OnImageGrayHistogram();
	afx_msg void OnImageLinearGrayEnhance();
	afx_msg void OnImageHistogramEqualization();
	afx_msg void OnImageHistogramSpecification();
	afx_msg void OnImagePaletteWarm();
	afx_msg void OnImagePaletteCool();
	afx_msg void OnImagePaletteFalseColor();
	afx_msg void OnSpatialNoiseSaltPepper();
	afx_msg void OnSpatialNoiseImpulse();
	afx_msg void OnSpatialNoiseGaussian();
	afx_msg void OnSpatialMeanFilter();
	afx_msg void OnSpatialMedianFilter();
	afx_msg void OnSpatialMaxFilter();
	afx_msg void OnSpatialEdgeSobel();
	afx_msg void OnSpatialEdgePrewitt();
	afx_msg void OnSpatialEdgeLaplacian();
	afx_msg void OnSpatialLaplacianSharpen();
	afx_msg void OnSpatialCompositeEnhance();
	afx_msg void OnSpatialKernel3();
	afx_msg void OnSpatialKernel5();
	afx_msg void OnSpatialKernel7();
	afx_msg void OnFrequencyFftSpectrum();
	afx_msg void OnFrequencyIfftReconstruct();
	afx_msg void OnFrequencyIdealLowPass();
	afx_msg void OnFrequencyButterworthLowPass();
	afx_msg void OnFrequencyIdealHighPass();
	afx_msg void OnFrequencyButterworthHighPass();
	afx_msg void OnFrequencyHomomorphic();
	afx_msg void OnFrequencyCutoff5();
	afx_msg void OnFrequencyCutoff10();
	afx_msg void OnFrequencyCutoff20();
	afx_msg void OnFrequencyOrder1();
	afx_msg void OnFrequencyOrder2();
	afx_msg void OnFrequencyOrder4();

private:
	bool EnsureImageLoaded() const;
	void RefreshViewsAndStatus(const CString& message);
	void SetSpatialKernelSize(int kernelSize);
	void SetFrequencyCutoffPercent(int percent);
	void SetButterworthOrder(int order);

	int m_spatialKernelSize;
	int m_frequencyCutoffPercent;
	int m_butterworthOrder;

#ifdef SHARED_HANDLERS
	// 用于为搜索处理程序设置搜索内容的 Helper 函数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
