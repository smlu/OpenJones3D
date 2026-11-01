#include <std/Win95/stdSysTagsWin32.h>
#include <Jones3D/Main/JonesMain.h>

// Lives further down in this file.
INT_PTR CALLBACK JonesMain_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Both live in Jones3D/Main/JonesMain.c: they need its file-local startup display data.
int J3DAPI JonesMain_InitDevDialog(HWND hDlg, WPARAM wParam, JonesState* pConfig);
void J3DAPI JonesMain_DevDialogHandleCommand(HWND hWnd, int controlId, LPARAM lParam, int notifyCode);

int J3DAPI JonesMain_ShowDevDialog(HWND hWnd, JonesState* pConfig)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    return DialogBoxParam(hInstance, (LPCSTR)101, hWnd, JonesMain_DevDialogProc, (LPARAM)pConfig);
}


int J3DAPI JonesMain_ShowDisplayOptions(HWND hWnd, JonesState* pState)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    return DialogBoxParam(hInstance, (LPCSTR)106, hWnd, JonesMain_DevDialogProc, (LPARAM)pState);
}


INT_PTR CALLBACK JonesMain_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            return JonesMain_InitDevDialog(hDlg, wParam, (JonesState*)lParam);

        case WM_COMMAND:
            JonesMain_DevDialogHandleCommand(hDlg, (uint16_t)wParam, lParam, HIWORD(wParam));
            return 1;
    };

    return 0;
}


void J3DAPI JonesMain_DevDialogUpdateRadioButtons(HWND hDlg, const JonesState* pState)
{
    switch ( pState->outputMode )
    {
        case JONES_OUTPUTMODE_NONE:
            CheckRadioButton(hDlg, 1013, 1015, 1013); // Normal
            break;
        case JONES_OUTPUTMODE_CONSOLE:
            CheckRadioButton(hDlg, 1013, 1015, 1014); // Console
            break;
        case JONES_OUTPUTMODE_LOGFILE:
            CheckRadioButton(hDlg, 1013, 1015, 1015); // Log file
            break;
    };

    switch ( pState->logLevel )
    {
        case JONES_LOGLEVEL_ERROR:
            CheckRadioButton(hDlg, 1016, 1018, 1018); // Quite
            break;
        case JONES_LOGLEVEL_NORMAL:
            CheckRadioButton(hDlg, 1016, 1018, 1016); // Normal
            break;
        case JONES_LOGLEVEL_VERBOSE:
            CheckRadioButton(hDlg, 1016, 1018, 1017); // Verbose
            break;
    };

    switch ( pState->performanceLevel )
    {
        case 0:
            CheckRadioButton(hDlg, 1176, 1180, 1176);
            break;

        case 1:
            CheckRadioButton(hDlg, 1176, 1180, 1177);
            break;

        case 2:
            CheckRadioButton(hDlg, 1176, 1180, 1178);
            break;

        case 3:
            CheckRadioButton(hDlg, 1176, 1180, 1179);
            break;

        case 4:
            CheckRadioButton(hDlg, 1176, 1180, 1180);
            break;

        default:
            return;
    }
}

