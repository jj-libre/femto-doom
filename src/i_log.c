/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2026 Jakob Jungreuthmayer
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Log output and fatal errors, formatted for I_Log and I_Fatal.
 *
 *-----------------------------------------------------------------------------*/

#include <stdarg.h>

#include "doomtype.h"
#include "i_platform.h"


#define LOG_LENGTH 96

typedef struct
{
	char* at;
	char* end; // room for the terminating zero
} logtext_t;


static void I_Put(logtext_t* text, char c)
{
	if (text->at < text->end)
		*text->at++ = c;
}


static void I_PutNumber(logtext_t* text, unsigned long value, unsigned base, int digits)
{
	char reversed[sizeof(value) * 3];
	int n = 0;

	do
	{
		reversed[n++] = "0123456789abcdef"[value % base];
		value /= base;
	} while (value != 0);

	for (; digits > n; digits--)
		I_Put(text, '0');
	while (n > 0)
		I_Put(text, reversed[--n]);
}


static void I_Format(char* buffer, const char* format, va_list args)
{
	logtext_t text = { buffer, buffer + LOG_LENGTH - 1 };

	for (; *format != '\0'; format++)
	{
		if (*format != '%')
		{
			I_Put(&text, *format);
			continue;
		}

		int precision = -1;
		if (*++format == '.')
		{
			precision = 0;
			while (*++format >= '0' && *format <= '9')
				precision = precision * 10 + (*format - '0');
		}

		boolean islong = *format == 'l';
		if (islong)
			format++;

		switch (*format)
		{
			case 'd':
			case 'i':
			{
				long value = islong ? va_arg(args, long) : va_arg(args, int);
				unsigned long magnitude = (unsigned long)value;
				if (value < 0)
				{
					I_Put(&text, '-');
					magnitude = 0 - magnitude;
				}
				I_PutNumber(&text, magnitude, 10, precision);
				break;
			}
			case 'u':
			case 'x':
			{
				unsigned long value = islong ? va_arg(args, unsigned long) : va_arg(args, unsigned);
				I_PutNumber(&text, value, *format == 'u' ? 10 : 16, precision);
				break;
			}
			case 'c':
				I_Put(&text, (char)va_arg(args, int));
				break;
			case 's':
				for (const char* s = va_arg(args, const char*); *s != '\0' && precision != 0; s++, precision--)
					I_Put(&text, *s);
				break;
			case '\0':
				format--;
				break;
			default:
				I_Put(&text, *format);
				break;
		}
	}

	*text.at = '\0';
}


void I_Printf(const char* format, ...)
{
	char text[LOG_LENGTH];
	va_list args;

	va_start(args, format);
	I_Format(text, format, args);
	va_end(args);

	I_Log(text);
}


void I_Error(const char* error, ...)
{
	char text[LOG_LENGTH];
	va_list args;

	va_start(args, error);
	I_Format(text, error, args);
	va_end(args);

	I_Fatal(text);
}
