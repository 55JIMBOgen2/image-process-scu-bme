
// MyPhotoshopView.h: CMyPhotoshopView 类的接口
//

#pragma once


class CMyPhotoshopView : public CView
{
protected: // 仅从序列化创建
	CMyPhotoshopView() noexcept;
	DECLARE_DYNCREATE(CMyPhotoshopView)

// 特性
public:
	CMyPhotoshopDoc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// 实现
public:
	virtual ~CMyPhotoshopView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // MyPhotoshopView.cpp 中的调试版本
inline CMyPhotoshopDoc* CMyPhotoshopView::GetDocument() const
   { return reinterpret_cast<CMyPhotoshopDoc*>(m_pDocument); }
#endif

