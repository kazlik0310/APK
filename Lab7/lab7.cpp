#include <iostream>
#include <Windows.h>
#include <string.h>

using namespace std;

HANDLE hCom_1 = NULL, hCom_2 = NULL;

OVERLAPPED overCom_1, overCom_2;

DCB dcbCom_1, dcbCom_2;

char* input();

bool Init_COM1();
bool Init_COM2();

bool setEventCom1();
bool setEventCom2();

void closehandle();

DWORD ReadData_COM();
bool WriteData_COM(char*);

int main()
{
	char* buf;
	int i;

	ZeroMemory(&dcbCom_1, sizeof(DCB));//чистка памяти
	ZeroMemory(&dcbCom_2, sizeof(DCB));

	ZeroMemory(&overCom_1, sizeof(OVERLAPPED));
	ZeroMemory(&overCom_2, sizeof(OVERLAPPED));

	Init_COM1();
	Init_COM2();

	while (true)
	{
		cout << "<<< ";
		buf = input();
		if (strcmp(buf, "quit") == 0)
			break;
		if (!WriteData_COM(buf))
		{
			cout << "FATAL ERROR" << endl;
			system("pause");
			exit(1);
		}
		ReadData_COM();
		delete[] buf;
	}

	closehandle();
	return 0;
}

bool Init_COM1()
{
	COMMTIMEOUTS CommTimeOuts;
	hCom_1 = CreateFile(TEXT("COM3"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);//открываем com как файл
	if (hCom_1 == INVALID_HANDLE_VALUE)//создался файл или нет 
	{
		cout << "No create port COM3" << endl;
		return false;
	}
	if (!GetCommState(hCom_1, &dcbCom_1))//получаем текущие настройки для ув-ва 
	{
		CloseHandle(hCom_1);
		cout << "getting state error\n";
		return false;
	}

	dcbCom_1.BaudRate = CBR_115200;                         //задаём скорость передачи 115200 бод
	dcbCom_1.fBinary = TRUE;                              //включаем двоичный режим обмена
	dcbCom_1.ByteSize = 8;                                //задаём 8 бит в байте
	dcbCom_1.Parity = NOPARITY;                                  //отключаем проверку чётности
	dcbCom_1.StopBits = ONESTOPBIT;                                //задаём один стоп-бит
	if (!SetCommState(hCom_1, &dcbCom_1)) //запоминаем изменения
	{
		cout << "error setting serial port state\n";
		CloseHandle(hCom_1);
		return false;
	}
	//стуктура временных параметров последовательного порта
	CommTimeOuts.ReadIntervalTimeout = 0; //временной промежуток между передачей 2 бит
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(hCom_1, &CommTimeOuts))
	{
		cout << "error setting serial port state\n";
		CloseHandle(hCom_1);
		return false;
	}

	return true;
}

bool Init_COM2()
{
	COMMTIMEOUTS CommTimeOuts;
	hCom_2 = CreateFile(TEXT("COM8"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hCom_2 == INVALID_HANDLE_VALUE)
	{
		cout << "No create port COM8" << endl;
		return false;
	}
	if (!GetCommState(hCom_2, &dcbCom_2))
	{
		CloseHandle(hCom_2);
		cout << "getting state error\n";
		return false;
	}

	dcbCom_2.BaudRate = CBR_9600;                         //задаём скорость передачи 115200 бод
	dcbCom_2.fBinary = TRUE;                              //включаем двоичный режим обмена
	dcbCom_2.ByteSize = 8;                                //задаём 8 бит в байте
	dcbCom_2.Parity = NOPARITY;                                  //отключаем проверку чётности
	dcbCom_2.StopBits = ONESTOPBIT;                                //задаём один стоп-бит
	if (!SetCommState(hCom_2, &dcbCom_2))
	{
		cout << "error setting serial port state\n";
		CloseHandle(hCom_2);
		return false;
	}

	CommTimeOuts.ReadIntervalTimeout = 0;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(hCom_1, &CommTimeOuts))
	{
		cout << "error setting serial port state\n";
		CloseHandle(hCom_1);
		return false;
	}

	return true;
}

DWORD ReadData_COM()
{
	char* buf;
	COMSTAT comstat;
	DWORD btr, temp, mask = 0, signal;

	overCom_2.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

	SetCommMask(hCom_2, EV_RXCHAR);

	WaitCommEvent(hCom_2, &mask, &overCom_2);

	ClearCommError(hCom_2, &temp, &comstat);		//нужно заполнить структуру COMSTAT
	btr = comstat.cbInQue;                          	//и получить из неё количество принятых байтов
	if (btr)                         			//если действительно есть байты для чтения
	{
		buf = new char[btr + 1];
		ReadFile(hCom_2, buf, btr, &temp, &overCom_2);     //прочитать байты из порта в буфер программы
		WaitForSingleObject(overCom_2.hEvent, INFINITE);
		buf[btr] = '\0';
		cout << ">>> " << buf << endl;
	}

	CloseHandle(overCom_2.hEvent);		//перед выходом из потока закрыть объект-событие
	return 0;
}

bool WriteData_COM(char* buf)
{
	bool fl;
	DWORD temp, signal;	//temp - переменная-заглушка

	overCom_1.hEvent = CreateEvent(NULL, true, true, NULL);   	  //создать событие

	//записать байты в порт (перекрываемая операция!)
	WriteFile(hCom_1, buf, strlen(buf), &temp, &overCom_1);

	signal = WaitForSingleObject(overCom_1.hEvent, INFINITE);	//приостановить поток, пока не завершится
	//перекрываемая операция WriteFile
	//если операция завершилась успешно, установить соответствующий флажок
	if ((signal == WAIT_OBJECT_0) && (GetOverlappedResult(hCom_1, &overCom_1, &temp, true)))
	{
		CloseHandle(overCom_1.hEvent);
		return true;
	}
	CloseHandle(overCom_1.hEvent);
	return false;
}

void closehandle()
{
	if (hCom_1)
		CloseHandle(hCom_1);
	if (hCom_2)
		CloseHandle(hCom_2);
}

char* input()
{
	char c, * str, * s;
	int i = 0;
	str = new char[2];
	*(str + 1) = '\0';
	cin.sync();
	while ((c = cin.get()) != '\n')
	{
		if (!i)
		{
			*(str + i) = c;
		}
		else
		{
			s = new char[i + 2];
			strcpy_s(s, sizeof(s), str);
			delete[] str;
			*(s + i) = c;
			*(s + i + 1) = '\0';
			str = s;
		}
		i++;
	}
	return str;
}
