// MyPhotoshopDoc.cpp: CMyPhotoshopDoc 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "MyPhotoshop.h"
#endif

#include "MyPhotoshopDoc.h"
#include "MainFrm.h" // 为了强刷界面，需要引入 MainFrame

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMyPhotoshopDoc

IMPLEMENT_DYNCREATE(CMyPhotoshopDoc, CDocument)

BEGIN_MESSAGE_MAP(CMyPhotoshopDoc, CDocument)
	// 将系统的打开文件命令 ID 绑定到我们重写的函数上
	ON_COMMAND(ID_FILE_OPEN, &CMyPhotoshopDoc::OnFileOpen)
	ON_COMMAND(ID_IMAGE_GRAY_HISTOGRAM, &CMyPhotoshopDoc::OnImageGrayHistogram)
	ON_COMMAND(ID_IMAGE_LINEAR_GRAY, &CMyPhotoshopDoc::OnImageLinearGrayEnhance)
	ON_COMMAND(ID_IMAGE_HIST_EQUALIZE, &CMyPhotoshopDoc::OnImageHistogramEqualization)
	ON_COMMAND(ID_IMAGE_HIST_SPEC, &CMyPhotoshopDoc::OnImageHistogramSpecification)
	ON_COMMAND(ID_IMAGE_PALETTE_WARM, &CMyPhotoshopDoc::OnImagePaletteWarm)
	ON_COMMAND(ID_IMAGE_PALETTE_COOL, &CMyPhotoshopDoc::OnImagePaletteCool)
	ON_COMMAND(ID_IMAGE_PALETTE_FALSE_COLOR, &CMyPhotoshopDoc::OnImagePaletteFalseColor)
	ON_COMMAND(ID_SPATIAL_NOISE_SALT_PEPPER, &CMyPhotoshopDoc::OnSpatialNoiseSaltPepper)
	ON_COMMAND(ID_SPATIAL_NOISE_IMPULSE, &CMyPhotoshopDoc::OnSpatialNoiseImpulse)
	ON_COMMAND(ID_SPATIAL_NOISE_GAUSSIAN, &CMyPhotoshopDoc::OnSpatialNoiseGaussian)
	ON_COMMAND(ID_SPATIAL_FILTER_MEAN, &CMyPhotoshopDoc::OnSpatialMeanFilter)
	ON_COMMAND(ID_SPATIAL_FILTER_MEDIAN, &CMyPhotoshopDoc::OnSpatialMedianFilter)
	ON_COMMAND(ID_SPATIAL_FILTER_MAX, &CMyPhotoshopDoc::OnSpatialMaxFilter)
	ON_COMMAND(ID_SPATIAL_EDGE_SOBEL, &CMyPhotoshopDoc::OnSpatialEdgeSobel)
	ON_COMMAND(ID_SPATIAL_EDGE_PREWITT, &CMyPhotoshopDoc::OnSpatialEdgePrewitt)
	ON_COMMAND(ID_SPATIAL_EDGE_LAPLACIAN, &CMyPhotoshopDoc::OnSpatialEdgeLaplacian)
	ON_COMMAND(ID_SPATIAL_ENHANCE_LAPLACIAN_SHARPEN, &CMyPhotoshopDoc::OnSpatialLaplacianSharpen)
	ON_COMMAND(ID_SPATIAL_ENHANCE_COMPOSITE, &CMyPhotoshopDoc::OnSpatialCompositeEnhance)
	ON_COMMAND(ID_SPATIAL_KERNEL_3, &CMyPhotoshopDoc::OnSpatialKernel3)
	ON_COMMAND(ID_SPATIAL_KERNEL_5, &CMyPhotoshopDoc::OnSpatialKernel5)
	ON_COMMAND(ID_SPATIAL_KERNEL_7, &CMyPhotoshopDoc::OnSpatialKernel7)
	ON_COMMAND(ID_FREQUENCY_FFT_SPECTRUM, &CMyPhotoshopDoc::OnFrequencyFftSpectrum)
	ON_COMMAND(ID_FREQUENCY_IFFT_RECONSTRUCT, &CMyPhotoshopDoc::OnFrequencyIfftReconstruct)
	ON_COMMAND(ID_FREQUENCY_IDEAL_LOW_PASS, &CMyPhotoshopDoc::OnFrequencyIdealLowPass)
	ON_COMMAND(ID_FREQUENCY_BUTTER_LOW_PASS, &CMyPhotoshopDoc::OnFrequencyButterworthLowPass)
	ON_COMMAND(ID_FREQUENCY_IDEAL_HIGH_PASS, &CMyPhotoshopDoc::OnFrequencyIdealHighPass)
	ON_COMMAND(ID_FREQUENCY_BUTTER_HIGH_PASS, &CMyPhotoshopDoc::OnFrequencyButterworthHighPass)
	ON_COMMAND(ID_FREQUENCY_HOMOMORPHIC, &CMyPhotoshopDoc::OnFrequencyHomomorphic)
	ON_COMMAND(ID_FREQUENCY_CUTOFF_5, &CMyPhotoshopDoc::OnFrequencyCutoff5)
	ON_COMMAND(ID_FREQUENCY_CUTOFF_10, &CMyPhotoshopDoc::OnFrequencyCutoff10)
	ON_COMMAND(ID_FREQUENCY_CUTOFF_20, &CMyPhotoshopDoc::OnFrequencyCutoff20)
	ON_COMMAND(ID_FREQUENCY_ORDER_1, &CMyPhotoshopDoc::OnFrequencyOrder1)
	ON_COMMAND(ID_FREQUENCY_ORDER_2, &CMyPhotoshopDoc::OnFrequencyOrder2)
	ON_COMMAND(ID_FREQUENCY_ORDER_4, &CMyPhotoshopDoc::OnFrequencyOrder4)
END_MESSAGE_MAP()


// CMyPhotoshopDoc 构造/析构

CMyPhotoshopDoc::CMyPhotoshopDoc() noexcept
{
	// 实例化对象，将数据掌控权交接给文档类 [cite: 14]
	pImage = new CImageProc();
	m_spatialKernelSize = 3;
	m_frequencyCutoffPercent = 10;
	m_butterworthOrder = 2;
}

CMyPhotoshopDoc::~CMyPhotoshopDoc()
{
	// 防止内存泄漏 [cite: 18]
	delete pImage;
}

BOOL CMyPhotoshopDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: 在此添加重新初始化代码
	// (SDI 文档将重用该文档)

	return TRUE;
}

// CMyPhotoshopDoc 序列化

void CMyPhotoshopDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 在此添加存储代码
	}
	else
	{
		// TODO: 在此添加加载代码
	}
}

#ifdef SHARED_HANDLERS

// 缩略图的支持
void CMyPhotoshopDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// 修改此代码以绘制文档数据
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// 搜索处理程序的支持
void CMyPhotoshopDoc::InitializeSearchContent()
{
	CString strSearchContent;
	SetSearchContent(strSearchContent);
}

void CMyPhotoshopDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl* pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CMyPhotoshopDoc 诊断

#ifdef _DEBUG
void CMyPhotoshopDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMyPhotoshopDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMyPhotoshopDoc 命令

void CMyPhotoshopDoc::OnFileOpen()
{
	// 修改过滤器，把主流格式全部加进白名单
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("所有支持图像(*.bmp;*.jpg;*.jpeg;*.tif;*.png)|*.bmp;*.jpg;*.jpeg;*.tif;*.tiff;*.png|All Files(*.*)|*.*||"), NULL);
	fileDlg.m_ofn.lpstrTitle = _T("上传/打开图片");

	if (fileDlg.DoModal() == IDOK)
	{
		CString stpathname = fileDlg.GetPathName();
		pImage->LoadFile(stpathname); // 名字已经改成了 LoadFile

		if (pImage == nullptr || !pImage->HasImage())
		{
			AfxMessageBox(_T("图片打开失败，请确认文件格式和路径是否正确。"));
			return;
		}

		SetTitle(fileDlg.GetFileName());
		SetModifiedFlag(FALSE);

		CMainFrame* pMainFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
		if (pMainFrame) {
			CString msg;
			msg.Format(_T("已上传图片：%s  尺寸：%d x %d  位深：%d"), fileDlg.GetFileName().GetString(), pImage->nWidth, pImage->nHeight, pImage->nNumColors);
			pMainFrame->SetMessageText(msg);
			CView* pView = pMainFrame->GetActiveView();
			if (pView) pView->Invalidate(TRUE);
		}
	}
}

bool CMyPhotoshopDoc::EnsureImageLoaded() const
{
	if (pImage == nullptr || !pImage->HasImage())
	{
		AfxMessageBox(_T("请先上传/打开一张图片。"));
		return false;
	}
	return true;
}

void CMyPhotoshopDoc::RefreshViewsAndStatus(const CString& message)
{
	SetModifiedFlag(TRUE);
	UpdateAllViews(nullptr);

	CMainFrame* pMainFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	if (pMainFrame)
	{
		pMainFrame->SetMessageText(message);
		CView* pView = pMainFrame->GetActiveView();
		if (pView) pView->Invalidate(TRUE);
	}
}

void CMyPhotoshopDoc::OnImageGrayHistogram()
{
	if (!EnsureImageLoaded()) return;

	if (pImage->MakeGrayHistogramImage())
	{
		RefreshViewsAndStatus(_T("已生成当前图像的灰度直方图。"));
	}
	else
	{
		AfxMessageBox(_T("灰度直方图生成失败。"));
	}
}

void CMyPhotoshopDoc::OnImageLinearGrayEnhance()
{
	if (!EnsureImageLoaded()) return;

	if (pImage->ApplyLinearGrayEnhance())
	{
		RefreshViewsAndStatus(_T("已完成灰度线性增强。"));
	}
	else
	{
		AfxMessageBox(_T("灰度线性增强失败；请确认当前图像不是空图，并优先使用 8 位灰度或 24/32 位图像。"));
	}
}

void CMyPhotoshopDoc::OnImageHistogramEqualization()
{
	if (!EnsureImageLoaded()) return;

	if (pImage->ApplyHistogramEqualization())
	{
		RefreshViewsAndStatus(_T("已完成直方图均衡增强。"));
	}
	else
	{
		AfxMessageBox(_T("直方图均衡失败；请确认当前图像不是空图，并优先使用 8 位灰度或 24/32 位图像。"));
	}
}

void CMyPhotoshopDoc::OnImageHistogramSpecification()
{
	if (!EnsureImageLoaded()) return;

	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("所有支持图像(*.bmp;*.jpg;*.jpeg;*.tif;*.png)|*.bmp;*.jpg;*.jpeg;*.tif;*.tiff;*.png|All Files(*.*)|*.*||"), NULL);
	fileDlg.m_ofn.lpstrTitle = _T("选择直方图规格化参考图像（图4）");

	if (fileDlg.DoModal() != IDOK) return;

	if (pImage->ApplyHistogramSpecification(fileDlg.GetPathName()))
	{
		CString msg;
		msg.Format(_T("已按参考图 %s 完成直方图规格化。"), fileDlg.GetFileName().GetString());
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("直方图规格化失败；请先打开图3，再选择图4作为参考图。当前图像建议使用 24/32 位彩色图。"));
	}
}

void CMyPhotoshopDoc::OnImagePaletteWarm()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyPaletteStyle(0)) RefreshViewsAndStatus(_T("已应用暖色调调色板风格。"));
	else AfxMessageBox(_T("调色板风格变换失败。"));
}

void CMyPhotoshopDoc::OnImagePaletteCool()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyPaletteStyle(1)) RefreshViewsAndStatus(_T("已应用冷色调调色板风格。"));
	else AfxMessageBox(_T("调色板风格变换失败。"));
}

void CMyPhotoshopDoc::OnImagePaletteFalseColor()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyPaletteStyle(2)) RefreshViewsAndStatus(_T("已应用伪彩色调色板风格。"));
	else AfxMessageBox(_T("调色板风格变换失败。"));
}

void CMyPhotoshopDoc::SetSpatialKernelSize(int kernelSize)
{
	if (kernelSize != 3 && kernelSize != 5 && kernelSize != 7)
	{
		kernelSize = 3;
	}

	m_spatialKernelSize = kernelSize;

	CString msg;
	msg.Format(_T("空域滤波模板大小已设置为 %d x %d。"), m_spatialKernelSize, m_spatialKernelSize);

	CMainFrame* pMainFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	if (pMainFrame)
	{
		pMainFrame->SetMessageText(msg);
	}
}

void CMyPhotoshopDoc::OnSpatialNoiseSaltPepper()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->AddSaltPepperNoise()) RefreshViewsAndStatus(_T("已加入椒盐噪声。"));
	else AfxMessageBox(_T("加入椒盐噪声失败。"));
}

void CMyPhotoshopDoc::OnSpatialNoiseImpulse()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->AddImpulseNoise()) RefreshViewsAndStatus(_T("已加入脉冲噪声。"));
	else AfxMessageBox(_T("加入脉冲噪声失败。"));
}

void CMyPhotoshopDoc::OnSpatialNoiseGaussian()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->AddGaussianNoise()) RefreshViewsAndStatus(_T("已加入高斯噪声。"));
	else AfxMessageBox(_T("加入高斯噪声失败。"));
}

void CMyPhotoshopDoc::OnSpatialMeanFilter()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyMeanFilter(m_spatialKernelSize))
	{
		CString msg;
		msg.Format(_T("已完成 %d x %d 均值滤波。"), m_spatialKernelSize, m_spatialKernelSize);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("均值滤波失败。"));
	}
}

void CMyPhotoshopDoc::OnSpatialMedianFilter()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyMedianFilter(m_spatialKernelSize))
	{
		CString msg;
		msg.Format(_T("已完成 %d x %d 中值滤波。"), m_spatialKernelSize, m_spatialKernelSize);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("中值滤波失败。"));
	}
}

void CMyPhotoshopDoc::OnSpatialMaxFilter()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyMaxFilter(m_spatialKernelSize))
	{
		CString msg;
		msg.Format(_T("已完成 %d x %d 最大值滤波。"), m_spatialKernelSize, m_spatialKernelSize);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("最大值滤波失败。"));
	}
}

void CMyPhotoshopDoc::OnSpatialEdgeSobel()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplySobelEdge(m_spatialKernelSize))
	{
		CString msg;
		msg.Format(_T("已完成 Sobel 边缘检测，预平滑模板为 %d x %d。"), m_spatialKernelSize, m_spatialKernelSize);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("Sobel 边缘检测失败。"));
	}
}

void CMyPhotoshopDoc::OnSpatialEdgePrewitt()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyPrewittEdge(m_spatialKernelSize))
	{
		CString msg;
		msg.Format(_T("已完成 Prewitt 边缘检测，预平滑模板为 %d x %d。"), m_spatialKernelSize, m_spatialKernelSize);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("Prewitt 边缘检测失败。"));
	}
}

void CMyPhotoshopDoc::OnSpatialEdgeLaplacian()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyLaplacianEdge(m_spatialKernelSize))
	{
		CString msg;
		msg.Format(_T("已完成 Laplacian 边缘检测，预平滑模板为 %d x %d。"), m_spatialKernelSize, m_spatialKernelSize);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("Laplacian 边缘检测失败。"));
	}
}

void CMyPhotoshopDoc::OnSpatialLaplacianSharpen()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyLaplacianSharpen()) RefreshViewsAndStatus(_T("已完成 Laplacian 锐化增强。"));
	else AfxMessageBox(_T("Laplacian 锐化增强失败。"));
}

void CMyPhotoshopDoc::OnSpatialCompositeEnhance()
{
	if (!EnsureImageLoaded()) return;
	if (pImage->ApplyCompositeSpatialEnhancement(m_spatialKernelSize))
	{
		CString msg;
		msg.Format(_T("已完成综合空域增强，平滑模板为 %d x %d。"), m_spatialKernelSize, m_spatialKernelSize);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("综合空域增强失败。"));
	}
}

void CMyPhotoshopDoc::OnSpatialKernel3()
{
	SetSpatialKernelSize(3);
}

void CMyPhotoshopDoc::OnSpatialKernel5()
{
	SetSpatialKernelSize(5);
}

void CMyPhotoshopDoc::OnSpatialKernel7()
{
	SetSpatialKernelSize(7);
}

void CMyPhotoshopDoc::SetFrequencyCutoffPercent(int percent)
{
	if (percent != 5 && percent != 10 && percent != 20)
	{
		percent = 10;
	}

	m_frequencyCutoffPercent = percent;

	CString msg;
	msg.Format(_T("变换域滤波截止半径已设置为频谱宽度的 %d%%。"), m_frequencyCutoffPercent);

	CMainFrame* pMainFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	if (pMainFrame)
	{
		pMainFrame->SetMessageText(msg);
	}
}

void CMyPhotoshopDoc::SetButterworthOrder(int order)
{
	if (order != 1 && order != 2 && order != 4)
	{
		order = 2;
	}

	m_butterworthOrder = order;

	CString msg;
	msg.Format(_T("巴特沃斯滤波器阶数已设置为 %d 阶。"), m_butterworthOrder);

	CMainFrame* pMainFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	if (pMainFrame)
	{
		pMainFrame->SetMessageText(msg);
	}
}

void CMyPhotoshopDoc::OnFrequencyFftSpectrum()
{
	if (!EnsureImageLoaded()) return;

	if (pImage->MakeFourierSpectrumImage())
	{
		RefreshViewsAndStatus(_T("已完成二维 FFT，并显示移中后的对数幅度谱。"));
	}
	else
	{
		AfxMessageBox(_T("二维 FFT 幅度谱生成失败。"));
	}
}

void CMyPhotoshopDoc::OnFrequencyIfftReconstruct()
{
	if (!EnsureImageLoaded()) return;

	if (pImage->ApplyInverseFftReconstruction())
	{
		RefreshViewsAndStatus(_T("已完成二维 FFT 后的 IFFT 重建。"));
	}
	else
	{
		AfxMessageBox(_T("IFFT 重建失败。"));
	}
}

void CMyPhotoshopDoc::OnFrequencyIdealLowPass()
{
	if (!EnsureImageLoaded()) return;

	const double cutoffRatio = m_frequencyCutoffPercent / 100.0;
	if (pImage->ApplyIdealLowPassFilter(cutoffRatio))
	{
		CString msg;
		msg.Format(_T("已完成理想低通滤波，截止半径为 %d%%。"), m_frequencyCutoffPercent);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("理想低通滤波失败。"));
	}
}

void CMyPhotoshopDoc::OnFrequencyButterworthLowPass()
{
	if (!EnsureImageLoaded()) return;

	const double cutoffRatio = m_frequencyCutoffPercent / 100.0;
	if (pImage->ApplyButterworthLowPassFilter(cutoffRatio, m_butterworthOrder))
	{
		CString msg;
		msg.Format(_T("已完成 %d 阶巴特沃斯低通滤波，截止半径为 %d%%。"), m_butterworthOrder, m_frequencyCutoffPercent);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("巴特沃斯低通滤波失败。"));
	}
}

void CMyPhotoshopDoc::OnFrequencyIdealHighPass()
{
	if (!EnsureImageLoaded()) return;

	const double cutoffRatio = m_frequencyCutoffPercent / 100.0;
	if (pImage->ApplyIdealHighPassFilter(cutoffRatio))
	{
		CString msg;
		msg.Format(_T("已完成理想高通锐化，截止半径为 %d%%。"), m_frequencyCutoffPercent);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("理想高通滤波失败。"));
	}
}

void CMyPhotoshopDoc::OnFrequencyButterworthHighPass()
{
	if (!EnsureImageLoaded()) return;

	const double cutoffRatio = m_frequencyCutoffPercent / 100.0;
	if (pImage->ApplyButterworthHighPassFilter(cutoffRatio, m_butterworthOrder))
	{
		CString msg;
		msg.Format(_T("已完成 %d 阶巴特沃斯高通锐化，截止半径为 %d%%。"), m_butterworthOrder, m_frequencyCutoffPercent);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("巴特沃斯高通滤波失败。"));
	}
}

void CMyPhotoshopDoc::OnFrequencyHomomorphic()
{
	if (!EnsureImageLoaded()) return;

	const double cutoffRatio = m_frequencyCutoffPercent / 100.0;
	if (pImage->ApplyHomomorphicFilter(cutoffRatio))
	{
		CString msg;
		msg.Format(_T("已完成同态滤波，截止半径为 %d%%。"), m_frequencyCutoffPercent);
		RefreshViewsAndStatus(msg);
	}
	else
	{
		AfxMessageBox(_T("同态滤波失败。"));
	}
}

void CMyPhotoshopDoc::OnFrequencyCutoff5()
{
	SetFrequencyCutoffPercent(5);
}

void CMyPhotoshopDoc::OnFrequencyCutoff10()
{
	SetFrequencyCutoffPercent(10);
}

void CMyPhotoshopDoc::OnFrequencyCutoff20()
{
	SetFrequencyCutoffPercent(20);
}

void CMyPhotoshopDoc::OnFrequencyOrder1()
{
	SetButterworthOrder(1);
}

void CMyPhotoshopDoc::OnFrequencyOrder2()
{
	SetButterworthOrder(2);
}

void CMyPhotoshopDoc::OnFrequencyOrder4()
{
	SetButterworthOrder(4);
}
