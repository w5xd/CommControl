// ComControl.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <vector>
#include "ComControl.h"
#include "Resource.h"

#ifdef UNICODE
typedef             std::wstring String_t;
#else
typedef             std::string String_t;
#endif

static const unsigned BUFF_SIZE = 32;

class MainWindow : public ATL::CDialogImpl<MainWindow>
{
public:
    enum { IDD = IDD_MAINDIALOG };

    MainWindow() : port(INVALID_HANDLE_VALUE)
    {}

    ~MainWindow()
    {
        if (port != INVALID_HANDLE_VALUE)
            ::CloseHandle(port);
    }

    BEGIN_MSG_MAP(MainWindow)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDOK, 0, OnOk)
        COMMAND_HANDLER(IDC_COMBOPORTS, CBN_SELCHANGE, OnCbnSelchangeComboports)
        COMMAND_HANDLER(IDC_CHECKTXD, BN_CLICKED, OnBnClickedChecktxd)
        COMMAND_HANDLER(IDC_CHECKRTS, BN_CLICKED, OnBnClickedCheckrts)
        COMMAND_HANDLER(IDC_CHECKDTR, BN_CLICKED, OnBnClickedCheckdtr)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HKEY key;
        if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &key))
        {
            DWORD dwI = 0;
            std::vector<TCHAR> szValueName(BUFF_SIZE);
            std::vector<TCHAR> szValue(BUFF_SIZE);
            for (;;)
            {
                DWORD nBuffSize = szValueName.size();
                auto nCode = RegEnumValue(key, dwI, &szValueName[0], &nBuffSize, NULL, NULL, NULL, NULL);
                if (nCode != ERROR_NO_MORE_ITEMS)
                {
                    DWORD valueLen = szValue.size() * sizeof(TCHAR);
                    DWORD tp;
                    RegQueryValueEx(key, &szValueName[0], 0, &tp, (unsigned char *)&szValue[0], &valueLen);
                    if (tp == REG_SZ)
                    {
                        valueLen /= sizeof(TCHAR);
                        String_t comName;
                        comName.assign(szValue.begin(), szValue.begin() + valueLen);
                        SendDlgItemMessage(IDC_COMBOPORTS, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(comName.c_str()));
                    }
                    dwI++;
                }
                else
                    break;
            }
            RegCloseKey(key);
        }
        enableDisable(false);
        return 0;
    }

    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        EndDialog(IDCANCEL);
        return 0;
    }

    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }

    void enableDisable(bool enable)
    {
        GetDlgItem(IDC_CHECKDTR).EnableWindow(enable ? TRUE : FALSE);
        GetDlgItem(IDC_CHECKTXD).EnableWindow(enable ? TRUE : FALSE);
        GetDlgItem(IDC_CHECKRTS).EnableWindow(enable ? TRUE : FALSE);

        if (!enable)
        {
            CheckDlgButton(IDC_CHECKDTR, FALSE);
            CheckDlgButton(IDC_CHECKTXD, FALSE);
            CheckDlgButton(IDC_CHECKRTS, FALSE);
        }
    }


    LRESULT OnCbnSelchangeComboports(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedChecktxd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedCheckrts(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedCheckdtr(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    HANDLE port;
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MainWindow wnd;
    wnd.DoModal();
    return 0;
}

LRESULT MainWindow::OnCbnSelchangeComboports(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
    auto sel = SendDlgItemMessage(wID, CB_GETCURSEL);
    if (sel >= 0)
    {
        if (port != INVALID_HANDLE_VALUE)
            ::CloseHandle(port);
        port = INVALID_HANDLE_VALUE;
        enableDisable(false);
        std::vector<TCHAR> selValue(BUFF_SIZE);
        GetDlgItemText(wID, &selValue[0], selValue.size());
        String_t comName = _T("\\\\.\\");

        bool alpha = true;
        for (unsigned i = 0; i < selValue.size(); i++)
        {
            if (!selValue[i])
                break;
            if (alpha)
            {
                if (_istdigit(selValue[i]))
                {
                    alpha = false;
                    comName += selValue[i];
                }
                else if (_istalpha(selValue[i]))
                    comName += selValue[i];
                else
                    break;
            }
            else if (_istdigit(selValue[i]))
                comName += selValue[i];
            else
                break;
        }

        port = ::CreateFile(comName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        if (port == INVALID_HANDLE_VALUE)
        {
            String_t msg = _T("Failed to open ");
            msg += comName.substr(4);
            ::MessageBox(*this, msg.c_str(), _T("Comm port tester"), MB_OK);
        }
        else
        {
            ::EscapeCommFunction(port, CLRBREAK);
            ::EscapeCommFunction(port, CLRRTS);
            ::EscapeCommFunction(port, CLRDTR);
        }
        enableDisable(port != INVALID_HANDLE_VALUE);

    }
    bHandled = true;
    return 0;
}


LRESULT MainWindow::OnBnClickedChecktxd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ::EscapeCommFunction(port, IsDlgButtonChecked(wID) ? SETBREAK : CLRBREAK);
    return 0;
}


LRESULT MainWindow::OnBnClickedCheckrts(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ::EscapeCommFunction(port, IsDlgButtonChecked(wID) ? SETRTS : CLRRTS);
    return 0;
}


LRESULT MainWindow::OnBnClickedCheckdtr(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ::EscapeCommFunction(port, IsDlgButtonChecked(wID) ? SETDTR : CLRDTR);
    return 0;
}
