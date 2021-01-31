/**
 * @file serial.h
 * @author Shinichiro Nakamura
 * @brief シリアルポートドライバのインターフェース定義。
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

#ifndef SERIAL_H
#define SERIAL_H

#include <stdio.h>

typedef struct serial SERIAL;

/**
 * @brief ボーレート.
 */
enum SerialBaud {
	SerialBaud2400,
	SerialBaud4800,
	SerialBaud9600,
	SerialBaud19200,
	SerialBaud38400,
	SerialBaud115200
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * シリアルポートをオープンする.
 *
 * @param devfile シリアルポートのデバイスファイル名.
 * @param baud ボーレート.
 *
 * @retval !NULL ハンドラ。
 * @retval NULL 失敗。
 */
SERIAL *serial_open(const char *devfile, const enum SerialBaud baud);

/**
 * シリアルポートをクローズする.
 *
 * @param s シリアルデスクリプタへのポインタ.
 *
 * @return 成功したら0を返す.
 */
int serial_close(SERIAL * s);

/**
 * シリアルポートから指定バイト数の読み込みを実行する.
 *
 * @param s シリアルデスクリプタへのポインタ.
 * @param buf バッファへのポインタ.
 * @param size 読み込みバイト数.
 *
 * @return 成功したら0を返す.
 */
int serial_read(SERIAL * s, unsigned char *buf, const size_t size);

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
int serial_read_with_timeout(
		SERIAL * s, unsigned char *buf, const size_t size, const int ms);

/**
 * シリアルポートへ指定バイト数の書き込みを実行する.
 *
 * @param s シリアルデスクリプタへのポインタ.
 * @param buf バッファへのポインタ.
 * @param size 書き込みバイト数.
 *
 * @return 成功したら0を返す.
 */
int serial_write(SERIAL * s, const unsigned char *buf, const size_t size);

/**
 * シリアルポートへ指定バイト数の書き込みを実行する.
 *
 * @param s シリアルデスクリプタへのポインタ.
 *
 * @return 受け取っていないデータのバイト数を返す.
 */
int serial_avaiable(SERIAL * s);

#ifdef __cplusplus
}
#endif

#endif
