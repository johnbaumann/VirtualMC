/**
 * @file serial.c
 * @author Shinichiro Nakamura
 * @brief シリアルポートドライバの実装。(Linuxプラットフォーム用)
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
#if !defined(__MINGW32__) && !defined(_WIN32) && !defined(__CYGWIN__)

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <limits.h>
#include <pthread.h>
#include "serial.h"

#define MUTEX_INIT()	pthread_mutex_init(&(s->mutex), NULL)
#define MUTEX_LOCK()	pthread_mutex_lock(&(s->mutex))
#define MUTEX_UNLOCK()	pthread_mutex_unlock(&(s->mutex))

struct serial {
	char devfile[BUFSIZ];
	int fd;
	struct termios termios_old;
	struct termios termios_new;
	pthread_mutex_t mutex;
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
	int baudrate = 0;

	/*
	 * シリアルデスクリプタの管理領域を確保する.
	 */
	SERIAL *s = (SERIAL *) malloc(sizeof(SERIAL));
	if (s == NULL) {
		return NULL;
	}

	/*
	 * ミューテックスを初期化する.
	 */
	MUTEX_INIT();

	/*
	 * ポートを開く.
	 */
	strcpy(s->devfile, devfile);
	s->fd = open(devfile, O_RDWR | O_NOCTTY | O_NDELAY);
	if (s->fd < 0) {
		free(s);
		return NULL;
	}

	/*
	 * ポート設定を退避する.
	 */
	tcgetattr(s->fd, &(s->termios_old));

	/*
	 * ポート設定を初期化する.
	 */
	memset(&(s->termios_new), 0x00, sizeof(s->termios_new));

	/*
	 * Bxxxxx : ボーレートの設定．cfsetispeed と cfsetospeed も使用できる
	 * CS8	  : 8n1 (8 ビット，ノンパリティ，ストップビット 1)
	 * CLOCAL : ローカル接続，モデム制御なし
	 * CREAD  : 受信文字(receiving characters)を有効にする
	 */
	s->termios_new.c_cflag = CS8 | CLOCAL | CREAD;
	switch (baud) {
	case SerialBaud2400:
		s->termios_new.c_cflag |= B2400;
		baudrate = 2400;
		break;
	case SerialBaud4800:
		s->termios_new.c_cflag |= B4800;
		baudrate = 4800;
		break;
	case SerialBaud9600:
		s->termios_new.c_cflag |= B9600;
		baudrate = 9600;
		break;
	case SerialBaud19200:
		s->termios_new.c_cflag |= B19200;
		baudrate = 19200;
		break;
	case SerialBaud38400:
		s->termios_new.c_cflag |= B38400;
		baudrate = 38400;
		break;
	default:
		s->termios_new.c_cflag |= B9600;
		baudrate = 9600;
		break;
	}
	cfsetispeed(&(s->termios_new), baudrate);
	cfsetospeed(&(s->termios_new), baudrate);

	/*
	 * IGNPAR : パリティエラーのデータは無視する
	 */
	s->termios_new.c_iflag = IGNPAR;

	/*
	 * Raw モードでの出力
	 */
	s->termios_new.c_oflag = 0;

	/*
	 * 入力モードをノンカノニカル、ノンエコーに設定する
	 */
	s->termios_new.c_lflag = 0;

	/*
	 * モデムラインをクリアし，ポートの設定を有効にする
	 */
	tcflush(s->fd, TCIFLUSH);
	tcsetpgrp(s->fd, getpgrp());
	tcsetattr(s->fd, TCSANOW, &(s->termios_new));

	return s;
}

/**
 * シリアルポートをクローズする.
 *
 * @param s シリアルデスクリプタへのポインタ.
 *
 * @return 成功したら0を返す.
 */
int serial_close(SERIAL * s)
{
	/*
	 * ポート設定を元に戻す.
	 */
	tcsetattr(s->fd, TCSANOW, &(s->termios_old));

	/*
	 * ポートを閉じる.
	 */
	close(s->fd);

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
int serial_read(SERIAL * s, unsigned char *buf, const size_t size)
{
	int e = 0;

	MUTEX_LOCK();

	/*
	 * 読み込みを実行する.
	 */
	if (read(s->fd, buf, size) != size) {
		e = 1;
	}

	MUTEX_UNLOCK();

	return e;
}

/**
 * シリアルポートから指定バイト数の読み込みを実行する.
 *
 * @param s シリアルデスクリプタへのポインタ.
 * @param buf バッファへのポインタ.
 * @param size 読み込みバイト数.
 * @param ms ミリ秒単位のタイムアウト時間.
 *
 * @return 成功したら0を返す.
 */
int serial_read_with_timeout(SERIAL * s,
		unsigned char *buf, const size_t size, const int ms)
{
	int e = 0;
	int i;

	MUTEX_LOCK();

	/*
	 * タイムアウトを設定して読み込みを実行する.
	 */
	for (i = 0; i < (int) size; i++) {
		struct pollfd fds;
		fds.fd = s->fd;
		fds.events = POLLIN;
		poll(&fds, 1, ms);
		if (fds.revents & POLLIN) {
			/*
			 * 1バイトの読み込みを実行.
			 */
			if (read(s->fd, buf + i, 1) != 1) {
				e = 1;
			}
		} else {
			/*
			 * タイムアウトが発生した.
			 */
			e = 2;
		}
	}

	MUTEX_UNLOCK();

	return e;
}

/**
 * シリアルポートへ指定バイト数の書き込みを実行する.
 *
 * @param s シリアルデスクリプタへのポインタ.
 * @param buf バッファへのポインタ.
 * @param size 書き込みバイト数.
 *
 * @return 成功したら0を返す.
 */
int serial_write(SERIAL * s,
		const unsigned char *buf, const size_t size)
{
	int e = 0;

	MUTEX_LOCK();

	if (write(s->fd, buf, size) != size) {
		e = 1;
	}

	MUTEX_UNLOCK();

	return e;
}

/**
 * シリアルポートへ指定バイト数の書き込みを実行する.
 *
 * @param s シリアルデスクリプタへのポインタ.
 *
 * @return 受け取っていないデータのバイト数を返す.
 */
int serial_avaiable(SERIAL * s)
{
	int e = 0;

	MUTEX_LOCK();

	ioctl(s->fd, FIONREAD, &e);

	MUTEX_UNLOCK();

	return e;
}

#endif
