#pragma once
#define _CRT_SECURE_NO_WARNINGS

#define SERV_PORT 5000	// Порт сервера
#define WSA_NETEVENT (WM_USER+1)
#define MAX_LOADSTRING 100

#pragma comment(lib, "WS2_32.lib")
#include "framework.h"
#include "ClientTwo.h"
#include <winsock.h>
#include <stdio.h>
#include <string>

static PHOSTENT phe;
char szBuf[1024];
int flag = 0;
static HWND hwndEdit;
char mess[2048];
char* m_mess = mess;
char szHostName[128] = "localhost"; //имя хоста
int err = 0;

WSADATA wsaData; //сведения о конкретной реализации интерфейса Windows Sockets
WORD wVersionRequested = MAKEWORD(1, 1);  //Номер требуемой версии Windows Sockets
SOCKET cln_socket = INVALID_SOCKET; // Сокет сервера
SOCKADDR_IN dest_sin; // Адрес сервера

DWORD cbWritten;
HINSTANCE hInst;						// current instance
TCHAR szTitle[MAX_LOADSTRING];				// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CLIENTTWO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENTTWO));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_CLIENTTWO);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_CLIENT));
	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance;

	// Store instance handle in our global variable
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_SYSMENU,
		1000, 485, 350, 290, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)  return FALSE;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}

BOOL SetConnection(HWND hWnd)
{
	// створюємо сокет
	cln_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (cln_socket == INVALID_SOCKET) {
		MessageBoxA(hWnd, "Socket error", "Error", MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	// Визначаємо адресу вузла
	phe = gethostbyname(szHostName);
	if (phe == NULL) {
		closesocket(cln_socket);
		MessageBoxA(hWnd, "Host address not defined", "Error", MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	dest_sin.sin_family = AF_INET;	// Задаємо тип адреси
	dest_sin.sin_port = htons(SERV_PORT);	// Встановлюємо номер порту

	memcpy((char FAR*) & (dest_sin.sin_addr), phe->h_addr, phe->h_length); // Копіюємо адресу вузла

	// Встановлюємо з'єднання
	if (connect(cln_socket, (PSOCKADDR)&dest_sin, sizeof(dest_sin)) == SOCKET_ERROR) {
		closesocket(cln_socket);
		MessageBoxA(hWnd, "Connection error", "Error", MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	// при спробі з'єднання головне вікно отримає повідомлення WSA_ACCEPT
	if (WSAAsyncSelect(cln_socket, hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE)) {
		MessageBoxA(hWnd, "WSAAsyncSelect error", "Error", MB_OK);
		return FALSE;
	}

	// Виводимо повідомлення про встановлення з'єднання з вузлом
	SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)"  Connection is set\r\n  ");

	return TRUE;
}

void SendMsg(HWND hWnd)
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	char buffer[2][_MAX_U64TOSTR_BASE10_COUNT];
	sprintf(buffer[0], "%llu", status.ullTotalPageFile);
	sprintf(buffer[1], "%llu", status.ullAvailPageFile);
	
	sprintf(szBuf, "    Pagefile size: %s byte. \r\n    Pagefile free size: %s byte.\r\n",
		buffer[0], buffer[1]);

	if (send(cln_socket, szBuf, strlen(szBuf), 0) != SOCKET_ERROR) {
		sprintf(m_mess, "\r\n  Data has been sent to the server: \r\n%s", szBuf);
		SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
	} else {
		sprintf(m_mess, "%s \r\n  Error sending message \r\n  ", m_mess);
		SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
	case WM_CREATE: {
		hwndEdit = CreateWindow( // Создаем доч.окно для вывода данных от процессов
			TEXT("EDIT"), NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 330, 230, hWnd, NULL, hInst, NULL);

		err = WSAStartup(wVersionRequested, &wsaData);
		if (err) {
			MessageBoxA(hWnd, "WSAStartup Error", "ERROR", MB_OK | MB_ICONSTOP);
			return FALSE;
		}

		sprintf(m_mess, "  Used by %s \r\n  Status: %s\r\n",
			wsaData.szDescription, wsaData.szSystemStatus);
		SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
	}
	break;

	case WM_COMMAND: {
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case ID_SET:
			SetConnection(hWnd);
			break;
		case ID_SENDMESSAGE:
			SendMsg(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;

	case WM_PAINT: {
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	break;

	case WM_DESTROY: {
		WSACleanup();
		PostQuitMessage(0);
	}
	break;

	case WSA_NETEVENT: {
		// если на сокете выполняется передача данных, принимаем и отображаем их
		if (WSAGETSELECTEVENT(lParam) == FD_READ) {
			int rc = recv(cln_socket, szBuf, sizeof(szBuf), 0);

			if (rc) {
				szBuf[rc] = '\0';
				sprintf(m_mess, "%s\r\n__________Server data___________________\r\n%s\r\n", m_mess, szBuf);
				SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
			}
		}
		// если соединение завершено, выводим сообщение об этом
		if (WSAGETSELECTEVENT(lParam) == FD_CLOSE)
			MessageBoxA(hWnd, "Server closed", "Server", MB_OK);
	}
	break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}