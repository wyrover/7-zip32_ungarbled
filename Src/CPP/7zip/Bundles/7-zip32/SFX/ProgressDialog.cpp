// ProgressDialog.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"

#include "resource.h"

#include "ProgressDialog.h"

#include "Windows/ResourceString.h"		// �ǉ�

using namespace NWindows;

extern HINSTANCE g_hInstance;

static const UINT_PTR kTimerID = 3;
static const UINT kTimerElapse = 100;

#ifdef LANG
#include "LangUtils.h"
#endif

#ifdef LANG
static CIDLangPair kIDLangPairs[] =
{
  { IDCANCEL, 0x02000711 }
};
#endif

HRESULT CProgressSync::ProcessStopAndPause()
{
  for (;;)
  {
    if (GetStopped())
      return E_ABORT;
    if (!GetPaused())
      break;
    ::Sleep(100);
  }
  return S_OK;
}

#ifndef _SFX
CProgressDialog::~CProgressDialog()
{
  AddToTitle(L"");
}
void CProgressDialog::AddToTitle(LPCWSTR s)
{
  if (MainWindow != 0)
    MySetWindowText(MainWindow, UString(s) + MainTitle);
}
#endif


bool CProgressDialog::OnInit()
{
  _range = (UInt64)-1;
  _prevPercentValue = -1;

  _wasCreated = true;
  _dialogCreatedEvent.Set();

  #ifdef LANG
  // LangSetWindowText(HWND(*this), 0x02000C00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif

  m_ProgressBar.Attach(GetItem(IDC_PROGRESS1));

  if (IconID >= 0)
  {
    HICON icon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IconID));
    SetIcon(ICON_BIG, icon);
  }

  _timer = SetTimer(kTimerID, kTimerElapse);
  SetText(_title);
  CheckNeedClose();
  return CModalDialog::OnInit();
}

void CProgressDialog::OnCancel() { Sync.SetStopped(true); }
void CProgressDialog::OnOK() { }

void CProgressDialog::SetRange(UInt64 range)
{
  _range = range;
  _peviousPos = (UInt64)(Int64)-1;
  _converter.Init(range);
  m_ProgressBar.SetRange32(0 , _converter.Count(range)); // Test it for 100%
}

void CProgressDialog::SetPos(UInt64 pos)
{
  bool redraw = true;
  if (pos < _range && pos > _peviousPos)
  {
    UInt64 posDelta = pos - _peviousPos;
    if (posDelta < (_range >> 10))
      redraw = false;
  }
  if (redraw)
  {
    m_ProgressBar.SetPos(_converter.Count(pos));  // Test it for 100%
    _peviousPos = pos;
  }
}

bool CProgressDialog::OnTimer(WPARAM /* timerID */, LPARAM /* callback */)
{
  if (Sync.GetPaused())
    return true;

  CheckNeedClose();

  UInt64 total, completed;
  Sync.GetProgress(total, completed);
  if (total != _range)
    SetRange(total);
  SetPos(completed);

  if (total == 0)
    total = 1;

  int percentValue = (int)(completed * 100 / total);
  if (percentValue != _prevPercentValue)
  {
    wchar_t s[64];
    ConvertUInt64ToString(percentValue, s);
    UString title = s;
    title += L"% ";
    SetText(title + _title);
    #ifndef _SFX
    AddToTitle(title + MainAddTitle);
    #endif
    _prevPercentValue = percentValue;
  }
  return true;
}

bool CProgressDialog::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
    case kCloseMessage:
    {
      KillTimer(_timer);
      _timer = 0;
      if (_inCancelMessageBox)
      {
        _externalCloseMessageWasReceived = true;
        break;
      }
      return OnExternalCloseMessage();
    }
    /*
    case WM_SETTEXT:
    {
      if (_timer == 0)
        return true;
    }
    */
  }
  return CModalDialog::OnMessage(message, wParam, lParam);
}

bool CProgressDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDCANCEL:
    {
      bool paused = Sync.GetPaused();
      Sync.SetPaused(true);
      _inCancelMessageBox = true;
      int res = ::MessageBoxW(HWND(*this), NWindows::MyLoadString(IDS_CONFIRM_CANSEL), _title, MB_YESNOCANCEL);	// �ύX
      _inCancelMessageBox = false;
      Sync.SetPaused(paused);
      if (res == IDCANCEL || res == IDNO)
      {
        if (_externalCloseMessageWasReceived)
          OnExternalCloseMessage();
        return true;
      }
      break;
    }
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CProgressDialog::CheckNeedClose()
{
  if (_needClose)
  {
    PostMessage(kCloseMessage);
    _needClose = false;
  }
}

bool CProgressDialog::OnExternalCloseMessage()
{
  End(0);
  return true;
}
