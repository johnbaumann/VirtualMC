/**
 * @file serial.c
 * @author Shinichiro Nakamura
 * @brief シリアルポートドライバの実装。(Windowsプラットフォーム用)
 */

/*
 * ===============================================================
 *	Serial interface library
 *	Version 0.0.3
 * ===============================================================
 * Copyright (c) 2010-2011 Shinichiro Nakamura
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * ===============================================================
 */
#if defined(__MINGW32__) || defined(_WIN32) || defined(__CYGWIN__)

#include <stdio.h>
#include <Windows.h>
#include <WinBase.h>
#include "serial.h"

struct serial
{
	char devfile[BUFSIZ];
	HANDLE handle;
};

/**
 * シリアルポートをオープンする.
 *
 * @param devfile シリアルポートのデバイスファイル名.
 * @param baud ボーレート.
 *
 * @retval !NULL ハンドラ。
 * @retval NULL 失敗。
 */
SERIAL *serial_open(const char *devfile, const enum SerialBaud baud)
{
	DCB param;
	int baudrate = 0;

	/*
	 * シリアルデスクリプタの管理領域を確保する.
	 */
	SERIAL *s = (SERIAL *)malloc(sizeof(SERIAL));
	if (s == NULL)
	{
		return NULL;
	}

	/*
	 * ポート名を決定する.
	 */
	if (strstr(devfile, "\\\\.\\") == NULL)
	{
		strcpy(s->devfile, "\\\\.\\");
	}
	else
	{
		strcpy(s->devfile, "");
	}
	strcat(s->devfile, devfile);

	/*
	 * ポートを開く.
	 */
	s->handle = CreateFile(
		s->devfile,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (s->handle == INVALID_HANDLE_VALUE)
	{
		free(s);
		return NULL;
	}

	/*
	 * ポートの設定を行う。
	 */
	if (!GetCommState(s->handle, &param))
	{
		free(s);
		return NULL;
	}
	switch (baud)
	{
	case SerialBaud2400:
		param.BaudRate = CBR_2400;
		break;
	case SerialBaud4800:
		param.BaudRate = CBR_4800;
		break;
	case SerialBaud9600:
		param.BaudRate = CBR_9600;
		break;
	case SerialBaud19200:
		param.BaudRate = CBR_19200;
		break;
	case SerialBaud38400:
		param.BaudRate = CBR_38400;
		break;
	case SerialBaud115200:
		param.BaudRate = CBR_115200;
		break;
	default:
		param.BaudRate = CBR_9600;
		break;
	}
	param.ByteSize = 8;
	param.StopBits = ONESTOPBIT;
	param.Parity = NOPARITY;
	param.fInX = FALSE;
	param.fOutX = FALSE;
	param.fOutxCtsFlow = TRUE;
	param.fOutxDsrFlow = FALSE;
	param.fRtsControl = RTS_CONTROL_DISABLE;
	param.fDtrControl = DTR_CONTROL_DISABLE;

	if (!SetCommState(s->handle, &param))
	{
		free(s);
		return NULL;
	}

	/*
	 * バッファの内容を捨てる.
	 */
	PurgeComm(s->handle, PURGE_RXCLEAR);
	PurgeComm(s->handle, PURGE_TXCLEAR);
	PurgeComm(s->handle, PURGE_RXABORT);
	PurgeComm(s->handle, PURGE_TXABORT);

	return s;
}

/**
 * シリアルポートをクローズする.
 *
 * @param s シリアルデスクリプタへのポインタ.
 *
 * @return 成功したら0を返す.
 */
int serial_close(SERIAL *s)
{

	/*
	 * バッファの内容を捨てる.
	 */
	PurgeComm(s->handle, PURGE_RXCLEAR);
	PurgeComm(s->handle, PURGE_TXCLEAR);
	PurgeComm(s->handle, PURGE_RXABORT);
	PurgeComm(s->handle, PURGE_TXABORT);

	/*
	 * ポートを閉じる.
	 */
	CloseHandle(s->handle);

	/*
	 * シリアルデスクリプタの管理領域を破棄する.
	 */
	free(s);

	return 0;
}

/**
 * シリアルポートから指定バイト数の読み込みを実行する.
 *
 * @param s シリアルデスクリプタへのポインタ.
 * @param buf バッファへのポインタ.
 * @param size 読み込みバイト数.
 *
 * @return 成功したら0を返す.
 */
int serial_read(SERIAL *s, unsigned char *buf, const size_t size)
{
	int e = 0;
	DWORD cnt;

	/*
	 * 読み込みを実行する. Perform a read.
	 */
	if (!ReadFile(s->handle, buf, size, &cnt, NULL))
	{
		e = 1;
	}

	return e;
}

/**
 * シリアルポートから指定バイト数の読み込みを実行する.
 * Read the specified number of bytes from the serial port.
 *
 * @param s シリアルデスクリプタへのポインタ. Pointer to the serial descriptor.
 * @param buf バッファへのポインタ. Pointer to the buffer.
 * @param size 読み込みバイト数. Number of bytes to read.
 * @param ms ミリ秒単位のタイムアウト時間. Time-out time in milliseconds.
 *
 * @return 成功したら0を返す. Returns 0 if successful.
 */
int serial_read_with_timeout(SERIAL *s,
							 unsigned char *buf, const size_t size, const int ms)
{
	int e = 0;
	DWORD cnt = 0;
	DWORD total = 0;

	/*
	 * タイムアウトを設定して読み込みを実行する.
	 * Set a timeout and perform a read.
	 */
	COMMTIMEOUTS cto;
	cto.ReadIntervalTimeout = 0;
	cto.ReadTotalTimeoutConstant = ms;
	cto.ReadTotalTimeoutMultiplier = 0;
	if (!SetCommTimeouts(s->handle, &cto))
	{
		e = 1;
	}

	/*
	 * 読み込みを実行する.
	 * Perform a read
	 */
	while (total < size)
	{
		ReadFile(s->handle, buf, size, &cnt, NULL);
		if (cnt != size)
		{
			e = 2;
			break;
		}
		total += cnt;
		buf += cnt;
	}

	return e;
}

/**
 * シリアルポートへ指定バイト数の書き込みを実行する.
 * Write the specified number of bytes to the serial port.
 *
 * @param s シリアルデスクリプタへのポインタ. Pointer to the serial descriptor.
 * @param buf バッファへのポインタ. Pointer to the buffer.
 * @param size 書き込みバイト数. Number of bytes to write.
 *
 * @return 成功したら0を返す. Returns 0 if successful.
 */
int serial_write(SERIAL *s,
				 const unsigned char *buf, const size_t size)
{
	int e = 0;
	DWORD cnt;

	/*
	 * 書き込みを実行する.
	 * Perform a write.
	 */
	if (!WriteFile(s->handle, buf, size, &cnt, NULL))
	{
		e = 1;
	}

	return e;
}

/**
 * シリアルポートへ指定バイト数の書き込みを実行する.
 * Write the specified number of bytes to the serial port.
 *
 * @param s シリアルデスクリプタへのポインタ. Pointer to the serial descriptor.
 *
 * @return 受け取っていないデータのバイト数を返す. Returns the number of bytes of data that has not been received.
 */
int serial_avaiable(SERIAL *s)
{
	DWORD dwErrors, dwEvent;
	COMSTAT ComStat;

	ClearCommError(s->handle, &dwErrors, &ComStat);
	return ComStat.cbInQue;
}

#endif
