/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef _IIOD_PARSER_H_INCLUDED
# define _IIOD_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

#include "../iio/iio-config.h"
#include "../iio/debug.h"

#include <stdbool.h>
#include <sys/socket.h>

int yylex();
int yylex_init_extra(void *d, yyscan_t *scanner);
int yylex_destroy(yyscan_t yyscanner);

void * yyget_extra(yyscan_t scanner);
ssize_t yy_input(yyscan_t scanner, char *buf, size_t max_size);

#define ECHO do { \
		struct parser_pdata *pdata = yyget_extra(yyscanner); \
		write_all(pdata, yytext, yyleng); \
	} while (0)

#define YY_INPUT(buf,result,max_size) do { \
		ssize_t res = yy_input(yyscanner, buf, max_size); \
		result = res <= 0 ? YY_NULL : (size_t) res; \
	} while (0)


/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    SPACE = 258,                   /* SPACE  */
    END = 259,                     /* END  */
    VERSION = 260,                 /* VERSION  */
    EXIT = 261,                    /* EXIT  */
    HELP = 262,                    /* HELP  */
    OPEN = 263,                    /* OPEN  */
    CLOSE = 264,                   /* CLOSE  */
    PRINT = 265,                   /* PRINT  */
    ZPRINT = 266,                  /* ZPRINT  */
    READ = 267,                    /* READ  */
    READBUF = 268,                 /* READBUF  */
    WRITEBUF = 269,                /* WRITEBUF  */
    WRITE = 270,                   /* WRITE  */
    SETTRIG = 271,                 /* SETTRIG  */
    GETTRIG = 272,                 /* GETTRIG  */
    TIMEOUT = 273,                 /* TIMEOUT  */
    DEBUG_ATTR = 274,              /* DEBUG_ATTR  */
    BUFFER_ATTR = 275,             /* BUFFER_ATTR  */
    IN_OUT = 276,                  /* IN_OUT  */
    CYCLIC = 277,                  /* CYCLIC  */
    SET = 278,                     /* SET  */
    BUFFERS_COUNT = 279,           /* BUFFERS_COUNT  */
    WORD = 280,                    /* WORD  */
    DEVICE = 281,                  /* DEVICE  */
    CHANNEL = 282,                 /* CHANNEL  */
    VALUE = 283                    /* VALUE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{

	char *word;
	struct iio_device *dev;
	struct iio_channel *chn;
	long value;


};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int yyparse (yyscan_t scanner);


#endif /* !_IIOD_PARSER_H_INCLUDED  */
