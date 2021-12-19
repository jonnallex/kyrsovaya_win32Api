#pragma once
#define _CRT_SECURE_NO_WARNINGS

#define SERV_PORT 5000	// Порт сервера
#define WSA_ACCEPT   (WM_USER+0)
#define WSA_NETEVENT (WM_USER+1)
#define MAX_LOADSTRING 100

#pragma comment(lib, "WS2_32.lib")
#include "Server.h"
#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

char szBuf[512];
static HWND hwndEdit;
char mess[2048];
char* m_mess = mess;
int err = 0;
int ClientNum = -1;

WSADATA wsaData; //сведения о конкр. реализации интерфейса Windows Sockets
WORD wVersionRequested = MAKEWORD(1, 1);  //Номер требуемой версии Windows Sockets
SOCKET srv_socket = INVALID_SOCKET; // Сокет сервера
SOCKET sock[2];	// Сокеты клиентов
SOCKADDR_IN sockaddr[2];	// Адреса клиентов

DWORD cbWritten;
HINSTANCE hInst;				// current instance
TCHAR szTitle[MAX_LOADSTRING];		// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];	// the main window class name
HWND hWindow;
// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
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
	LoadString(hInstance, IDC_SERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SERVER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
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
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERV));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SERVER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SERV));
	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance;

	// Store instance handle in our global variable
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_SYSMENU,
		662, 200, 350, 575, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) return FALSE;
	hWindow = hWnd;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}

void ServerStart(HWND hWnd)
{
	//Функція WSAStartup ініціалізує WinSock
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err) {
		MessageBoxA(hWnd, "WSAStartup Error", "ERROR", MB_OK | MB_ICONSTOP);
		return;
	}

	if (srv_socket != INVALID_SOCKET) {
		MessageBoxA(hWnd, "Socket already created", "Info", MB_OK | MB_ICONINFORMATION);
		return;
	}

	//Cтворюємо сокет сервера для роботи з потоком даних
	srv_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (srv_socket == INVALID_SOCKET) {
		MessageBoxA(hWnd, "Socket creation error", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	//Встановлюємо адресу IP та номер порту
	SOCKADDR_IN srv_address;
	srv_address.sin_family = AF_INET;
	srv_address.sin_port = htons(SERV_PORT);
	srv_address.sin_addr.s_addr = INADDR_ANY; //вик. адресу за замовчуванням (тобто будь-яку)

	//Зв'язуємо адресу IP із сокетом
	if (SOCKET_ERROR == bind(srv_socket, (LPSOCKADDR)&srv_address, sizeof(srv_address))) {
		closesocket(srv_socket);
		MessageBoxA(hWnd, "Port binding error", "Error", MB_OK | MB_ICONSTOP);
		return;
	}

	//очікуємо на встановлення зв'язку
	if (listen(srv_socket, 4) == SOCKET_ERROR) {
		closesocket(srv_socket);
		MessageBoxA(hWnd, "Communication setup pending error", "Error", MB_OK);
		return;
	}

	//при спробі з'єднання головне вікно отримає повідомлення WSA_ACCEPT
	int rc = WSAAsyncSelect(srv_socket, hWnd, WSA_ACCEPT, FD_ACCEPT);
	if (rc) {
		closesocket(srv_socket);
		MessageBoxA(hWnd, "WSAAsyncSelect error", "Error", MB_OK);
		return;
	}

	// Выводим сообщение о запуске сервера
	SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)"  Server started");
}

BOOL AcceptClient(int j)
{
	int sockaddr_len = sizeof(sockaddr[j]);
	sock[j] = accept(srv_socket, (LPSOCKADDR)&sockaddr[j], (int FAR*) & sockaddr_len);

	if (sock[j] != INVALID_SOCKET)
		if (!WSAAsyncSelect(sock[j], hWindow, WSA_NETEVENT, FD_READ | FD_CLOSE))
			return TRUE;

	closesocket(sock[j]);
	return FALSE;
}

void WndProc_OnWSAAccept(HWND hWnd, LPARAM lParam)
{
	// при помилці скасовуємо надходження повідомлень у головне вікно програми
	//if (WSAGETSELECTERROR(lParam)) {
	///	MessageBoxA(hWnd, "Accept error", "Error", MB_OK);
	//	WSAAsyncSelect(srv_socket, hWnd, 0, 0);
	//	return;
	//}
	///if (ClientNum == 1) {
	//	MessageBoxA(hWnd, "Number of clients >2\r\n", "Connection is invalid", MB_OK);
	//return;
	//}

	ClientNum++;

	if (!AcceptClient(ClientNum)) {
		MessageBoxA(hWnd, "Client connection error", "Error", MB_OK);
		return;
	}

	//додаємо клієнта
	//sprintf_s(szBuf, "  Added client %i\r\n  Address: IP=%s  Port=%u\r\n \0", ClientNum + 1,
		//inet_ntoa(sockaddr[ClientNum].sin_addr), htons(sockaddr[ClientNum].sin_port));
	//SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)szBuf);
}

void SendToClient(int j)
{
	if (j > ClientNum) return;

	cbWritten = SendMessageA(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
	SendMessageA(hwndEdit, WM_GETTEXT, (WPARAM)cbWritten, (LPARAM)szBuf);
	szBuf[cbWritten] = '\0';

	if (send(sock[j], szBuf, strlen(szBuf), 0) != SOCKET_ERROR) {
		sprintf_s(szBuf,szBuf);
		SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)szBuf);
	} else
		SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)"  Error sending message \r\n");	
}

void WndProc_OnWSANetEvent(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	char szTemp[256], szMess[256];
	int Number;

	//дізнаємося від якого клієнта надійшло повідомлення, => Number
	if (sock[0] == (SOCKET)wParam) Number = 0;
	else if (sock[1] == (SOCKET)wParam) Number = 1;

	//якщо на сокеті виконується передача даних, приймаємо та відображаємо їх
	if (WSAGETSELECTEVENT(lParam) == FD_READ) {
		int rc = recv((SOCKET)wParam, szTemp, 256, 0);
		if (rc) {
			szTemp[rc] = '\0';
			sprintf(m_mess, "%s \r\n  Client data %i:\r\n%s", m_mess, Number + 1, szTemp);
			SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
		}
	}

	//если соединение завершено, выводим сообщение об этом
	if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) {
		WSAAsyncSelect(sock[Number], hWindow, 0, 0);
		closesocket(sock[Number]);
		sprintf_s(szTemp, "  Client %i finished", Number + 1);
		SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)szTemp);
	}
}

void ClientOff(HWND hWnd, int j)
{ 
	if (j > ClientNum) return;

	sprintf_s(szBuf, "  Disable Client %i?", j + 1);
	if (IDYES == MessageBoxA(hWnd, szBuf, "Question", MB_YESNO | MB_ICONQUESTION)) {
		WSAAsyncSelect(sock[j], hWindow, 0, 0);
		closesocket(sock[j]);
		return;
	}
}

void ServerStop(HWND hWnd)
{
	WSAAsyncSelect(srv_socket, hWnd, 0, 0);

	srv_socket = INVALID_SOCKET;
	closesocket(srv_socket);
	WSACleanup();

	SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)"  Server stopped");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
	case WM_CREATE: {
		hwndEdit = CreateWindow( // Створюємо доч. вікно для виведення даних від процесів
			TEXT("EDIT"), NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 330, 518, hWnd, NULL, hInst, NULL);

			err = WSAStartup(wVersionRequested, &wsaData);
			if (err) {
				MessageBoxA(hWnd, "WSAStartup Error", "ERROR", MB_OK | MB_ICONSTOP);
				return FALSE;
			}
			WSACleanup();

			sprintf(m_mess, "  Used by %s\r\n  Status: %s\r\n",
				wsaData.szDescription, wsaData.szSystemStatus);
			SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
	}
	break;

	case WM_COMMAND: {
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case ID_START:
			ClientNum = -1;
			ServerStart(hWnd);
			break;
		case ID_STOP:
			ServerStop(hWnd);
			break;
		case ID_SEND_CLIENTONE:
			SendToClient(0);
			break;
		case ID_SEND_CLIENTTWO:
			SendToClient(1);
			break;
		case ID_OFF_CLIENTONE:
			ClientOff(hWnd, 0);
			break;
		case ID_OFF_CLIENTTWO:
			ClientOff(hWnd, 1);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		WSACleanup();
		PostQuitMessage(0);
		break;
	case WSA_ACCEPT:
		WndProc_OnWSAAccept(hWnd, lParam);
		break;
	case WSA_NETEVENT:
		WndProc_OnWSANetEvent(hWnd, wParam, lParam);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}