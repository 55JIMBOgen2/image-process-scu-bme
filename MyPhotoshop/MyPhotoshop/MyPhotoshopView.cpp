
// MyPhotoshopView.cpp: CMyPhotoshopView 类的实现
//

#include "pch.h"
#include "framework.h"
#include "MainFrm.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "MyPhotoshop.h"
#endif

#include "MyPhotoshopDoc.h"
#include "MyPhotoshopView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	bool CalcAdaptiveDrawRect(const CImageProc* pImage, const CRect& clientRect, CRect& outRect)
	{
		if (pImage == nullptr) return false;
		if (pImage->nWidth <= 0 || pImage->nHeight <= 0) return false;
		if (clientRect.Width() <= 0 || clientRect.Height() <= 0) return false;

		const double scaleX = static_cast<double>(clientRect.Width()) / static_cast<double>(pImage->nWidth);
		const double scaleY = static_cast<double>(clientRect.Height()) / static_cast<double>(pImage->nHeight);
		const double scale = (scaleX < scaleY) ? scaleX : scaleY; // 等比缩放，完整显示

		int drawW = static_cast<int>(pImage->nWidth * scale + 0.5);
		int drawH = static_cast<int>(pImage->nHeight * scale + 0.5);
		if (drawW <= 0) drawW = 1;
		if (drawH <= 0) drawH = 1;

		const int left = clientRect.left + (clientRect.Width() - drawW) / 2;
		const int top = clientRect.top + (clientRect.Height() - drawH) / 2;
		outRect.SetRect(left, top, left + drawW, top + drawH);
		return true;
	}

	bool ClientToImagePoint(const CImageProc* pImage, const CRect& drawRect, const CPoint& clientPoint, CPoint& imagePoint)
	{
		if (pImage == nullptr || pImage->nWidth <= 0 || pImage->nHeight <= 0) return false;
		if (drawRect.Width() <= 0 || drawRect.Height() <= 0) return false;
		if (!drawRect.PtInRect(clientPoint)) return false;

		const double normX = static_cast<double>(clientPoint.x - drawRect.left) / static_cast<double>(drawRect.Width());
		const double normY = static_cast<double>(clientPoint.y - drawRect.top) / static_cast<double>(drawRect.Height());

		int imgX = static_cast<int>(normX * pImage->nWidth);
		int imgY = static_cast<int>(normY * pImage->nHeight);

		if (imgX < 0) imgX = 0;
		else if (imgX >= pImage->nWidth) imgX = pImage->nWidth - 1;

		if (imgY < 0) imgY = 0;
		else if (imgY >= pImage->nHeight) imgY = pImage->nHeight - 1;

		imagePoint = CPoint(imgX, imgY);
		return true;
	}
}


// CMyPhotoshopView

IMPLEMENT_DYNCREATE(CMyPhotoshopView, CView)

BEGIN_MESSAGE_MAP(CMyPhotoshopView, CView)
	// 标准打印命令
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
END_MESSAGE_MAP()

// CMyPhotoshopView 构造/析构

CMyPhotoshopView::CMyPhotoshopView() noexcept
{
	// TODO: 在此处添加构造代码

}

CMyPhotoshopView::~CMyPhotoshopView()
{
}

BOOL CMyPhotoshopView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}

// CMyPhotoshopView 绘图

void CMyPhotoshopView::OnDraw(CDC* pDC)
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	CRect clientRect;
	GetClientRect(&clientRect);
	pDC->FillSolidRect(clientRect, RGB(245, 245, 245)); // 响应式留白背景

	if (pDoc->pImage != nullptr)
	{
		CRect drawRect;
		if (CalcAdaptiveDrawRect(pDoc->pImage, clientRect, drawRect))
		{
			pDoc->pImage->ShowImage(pDC, drawRect); // 等比自适应显示（支持放大）
		}
	}
}


// CMyPhotoshopView 打印

BOOL CMyPhotoshopView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 默认准备
	return DoPreparePrinting(pInfo);
}

void CMyPhotoshopView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加额外的打印前进行的初始化过程
}

void CMyPhotoshopView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加打印后进行的清理过程
}


// CMyPhotoshopView 诊断

#ifdef _DEBUG
void CMyPhotoshopView::AssertValid() const
{
	CView::AssertValid();
}

void CMyPhotoshopView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMyPhotoshopDoc* CMyPhotoshopView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMyPhotoshopDoc)));
	return (CMyPhotoshopDoc*)m_pDocument;
}
#endif //_DEBUG


// CMyPhotoshopView 消息处理程序


void CMyPhotoshopView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (nFlags & MK_SHIFT)
	{
		CClientDC dc(this);
		CMyPhotoshopDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);

		if (pDoc && pDoc->pImage)
		{
			// 1. 物理身份查验：确保内存中确实有数据存在
			bool hasBmp = pDoc->pImage->m_isBmp && pDoc->pImage->m_hDib != NULL;
			bool hasExt = !pDoc->pImage->m_isBmp && !pDoc->pImage->m_extImage.IsNull();

			if (hasBmp || hasExt)
			{
				CRect clientRect;
				GetClientRect(&clientRect);
				CRect drawRect;
				if (!CalcAdaptiveDrawRect(pDoc->pImage, clientRect, drawRect))
				{
					CView::OnLButtonDown(nFlags, point);
					return;
				}

				// 将屏幕点击坐标反算到原图坐标，确保缩放后取色仍准确
				CPoint imagePoint;
				if (!ClientToImagePoint(pDoc->pImage, drawRect, point, imagePoint))
				{
					CMainFrame* pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
					if (pFrame)
					{
						CString msg;
						msg.Format(L"屏幕:(%d,%d) | 当前点击不在图像区域", point.x, point.y);
						pFrame->SetMessageText(msg);
					}
					CView::OnLButtonDown(nFlags, point);
					return;
				}

				// 2. 向底层工厂索要数据（工厂会根据自身状态决定是手撕掩码，还是呼叫 API）
				int myR, myG, myB;
				pDoc->pImage->GetColor(imagePoint.x, imagePoint.y, myR, myG, myB);

				// 3. 屏幕显存探针：调用操作系统最底层的 CDC 屏幕渲染 API 作为对照组
				COLORREF sysColor = dc.GetPixel(point.x, point.y);
				int sysR = GetRValue(sysColor);
				int sysG = GetGValue(sysColor);
				int sysB = GetBValue(sysColor);

				// 4. 诚实的 UI 分流逻辑
				CString strOutput;
				if (pDoc->pImage->m_isBmp)
				{
					// BMP 轨道：彰显我们徒手跨越位移与内存对齐陷阱的硬核计算
					strOutput.Format(L"屏幕:(%d,%d) 图像:(%d,%d) | BMP底层强解:(%d,%d,%d) | 屏幕API校验:(%d,%d,%d)",
						point.x, point.y, imagePoint.x, imagePoint.y, myR, myG, myB, sysR, sysG, sysB);
				}
				else
				{
					// 扩展格式轨道：坦诚地承认这是调用系统黑盒的产物
					strOutput.Format(L"屏幕:(%d,%d) 图像:(%d,%d) | 扩展格式(API提取):(%d,%d,%d) | 屏幕API校验:(%d,%d,%d)",
						point.x, point.y, imagePoint.x, imagePoint.y, myR, myG, myB, sysR, sysG, sysB);
				}

				// 5. 暴力劫持主框架，将结果拍到状态栏上
				CMainFrame* pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
				if (pFrame)
				{
					pFrame->SetMessageText(strOutput);
				}
			}
		}
	}

	CView::OnLButtonDown(nFlags, point);
}
