/*
	DL600C.L
	Copyright (c) A.Sobolev 2006, 2007, 2008, 2010, 2011, 2016
	Part of project Papyrus
	Lexer of DL600 language
*/
%x incl

%{
#ifndef _DL600C_
#define _DL600C_
#endif

#include <stdio.h>
#include <stdlib.h>
#include <dl600.h>
#include "dl600c.tab.cpp.h"

long       yyline = 1;
long       yybrak = 0;
YYIN_STR * yyin_struct = 0;
long       yyin_cnt = 0;

SString MacroSubst;

#define TOKEN yylval.token
%}

EOL         "\n"
WHITE       [ \t]([ \t])*
STRING      \"[^"\n]*["\n]
REAL        [0-9]+([.][0-9]*)([Ee][+-]?[0-9]+)?
DIGIT       [0-9]
HEXDIGIT    [0-9A-Fa-f]
DECINT      {DIGIT}+
HEXINT      0[xX][0-9A-Fa-f]+
OCTCHAR     \\0[0-9][0-9]
INT         [\+\-]?(([0-9]+)|(0[xX][0-9A-Fa-f]+))
DATE        [0-9][0-9][/\-\.][0-9][0-9][/\-\.][0-9][0-9][0-9][0-9]
TIME        [0-9][0-9]:[0-9][0-9]:[0-9][0-9]
NDIGIT      [A-Z_a-z@]
IDENT       [A-Za-z\_][0-9A-Za-z\_]*
AT_IDENT    \@{IDENT}
COMMENT     "//"(.)*"\n"
COMMENT_C   "/*"
R_FMT       ~[<>|]?[*]?{DIGIT}*([.][0-9]{DIGIT}*)?([@#$&]{NDIGIT}({NDIGIT}{NDIGIT}?)?)?
UUID        {HEXDIGIT}+\-{HEXDIGIT}+\-{HEXDIGIT}+\-{HEXDIGIT}+\-{HEXDIGIT}+

%%
{EOL}       { ++yyline; }
{COMMENT}   { ++yyline; }
{COMMENT_C} {
	int    c;
	for( ; ; ) {
		while((c = yyinput()) != '*' && c != EOF)
			if(c == '\n')
				yyline++;
		;    /* eat up text of comment */
		if(c == '*') {
			while((c = yyinput()) == '*')
				;
			if(c == '/')
				break; /* found the end */
		}
		if(c == EOF) {
			yyerror("EOF in comment");
			break;
		}
	}
}
{WHITE}     { ; }
{STRING}    { yytext[yyleng-1] = 0; return TOKEN.Create(T_CONST_STR, yytext+1); }
{DECINT}    { return TOKEN.Create(T_CONST_INT, yytext);  }
{HEXINT}    { return TOKEN.Create(T_CONST_INT, yytext);  }
{REAL}      { return TOKEN.Create(T_CONST_REAL, yytext); }
{UUID}      { return TOKEN.Create(T_CONST_UUID, yytext); }
{R_FMT}     { return TOKEN.Create(T_FMT, yytext+1); }
"["         { return '['; }
"]"         { return ']'; }
"&&"        { return T_AND; }
"||"        { return T_OR;  }
"=="        { return T_EQ;  }
"!="        { return T_NEQ; }
">="        { return T_GE;  }
"<="        { return T_LE;  }
"{"         { yybrak++; return T_LBR; }
"}"         { yybrak--; return T_RBR; }
"IMPORT"    BEGIN(incl);
"import"    BEGIN(incl);
"data"      { return TOKEN.Create(T_EXPORTSTR); }
"interface" { return TOKEN.Create(T_INTERFACE); }
"struct"    { return TOKEN.Create(T_ISTRUCT);   }
"enum"      { return TOKEN.Create(T_ENUM);      }
"iclass"    { return TOKEN.Create(T_ICLASS);    }
"declare"   { return TOKEN.Create(T_DECLARE);   }
"iteration" { return TOKEN.Create(T_ITERATION); }
"link"      { return TOKEN.Create(T_LINK);      }
"if"        { return TOKEN.Create(T_IF);        }
"else"      { return TOKEN.Create(T_ELSE);      }
"format"    { return TOKEN.Create(T_FORMAT_FUNC); }
"in"        { return TOKEN.Create(T_IN);        }
"out"       { return TOKEN.Create(T_OUT);       }
"inout"     { return TOKEN.Create(T_INOUT);     }
"default"   { return TOKEN.Create(T_DEFAULT);   }
"version"   { return TOKEN.Create(T_VERSION);   }
"library"   { return TOKEN.Create(T_LIBRARY);   }
"uuid"      { return TOKEN.Create(T_UUID);      }
"abstract"  { return TOKEN.Create(T_ABSTRACT);  }
"property"  { return TOKEN.Create(T_PROPERTY);  }
"table"     { return TOKEN.Create(T_TABLE);     }
"index"     { return TOKEN.Create(T_INDEX);     }
"file"      { return TOKEN.Create(T_FILE);      }
"handler"   { return TOKEN.Create(T_HANDLER);   } // @v8.9.0
"dialog"          { return TOKEN.Create(T_DIALOG); }
"input"           { return TOKEN.Create(T_INPUT); }
"statictext"      { return TOKEN.Create(T_STATICTEXT); }
"framebox"        { return TOKEN.Create(T_FRAME); }
"combobox"        { return TOKEN.Create(T_COMBOBOX); }
"button"          { return TOKEN.Create(T_BUTTON); }
"checkbox"        { return TOKEN.Create(T_CHECKBOX); }
"checkcluster"    { return TOKEN.Create(T_CHECKBOXCLUSTER); }
"radiocluster"    { return TOKEN.Create(T_RADIOCLUSTER); }
"listbox"         { return TOKEN.Create(T_LISTBOX); }
"treelistbox"     { return TOKEN.Create(T_TREELISTBOX); }
"columns"         { return TOKEN.Create(T_COLUMNS); }
"descript"        { return TOKEN.Create(T_DESCRIPT); }
{IDENT}     {
	DLSYMBID symb_id = 0;
	if(DCtx.GetMacro(yytext, MacroSubst))
		return TOKEN.Create(T_IDENT, MacroSubst);
	else if(DCtx.SearchSymb(yytext, '@', &symb_id) > 0)
		return TOKEN.Create(T_TYPE, symb_id);
	else {
		yytext[yyleng] = 0;
		return TOKEN.Create(T_IDENT, yytext);
	}
}
{AT_IDENT}  {
	yytext[yyleng] = 0;
	return TOKEN.Create(T_AT_IDENT, yytext+1);
}
[-,;+*/.:()?<>=@&|!] { return yytext[0]; }
<incl>[ \t]+
<incl>[^ \t\n]+	{
		yyin_cnt++;
		yyin_struct = (YYIN_STR *)realloc(yyin_struct, yyin_cnt*sizeof(YYIN_STR));
		if(!yyin_struct)
			yyerror("Not enough memory");
		yyin_struct[yyin_cnt-1].yyin_buf = YY_CURRENT_BUFFER;
		yyin_struct[yyin_cnt-1].yyin_line = yyline;

		SString file_path;
		if(DCtx.FindImportFile(yytext, file_path)) {
			strnzcpy(yyin_struct[yyin_cnt-1].fname, file_path, _MAX_PATH);
			yyin = fopen(file_path, "r");
			if(!yyin) {
				SString msg_buf;
				yyerror(msg_buf.Printf("Could not open include file '%s'", yytext));
			}
			yyline = 1;
			yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
			BEGIN(INITIAL);
		}
		else
			DCtx.Error(0, 0, DlContext::erfExit | DlContext::erfLog);
	}
<<EOF>> {
		if(--yyin_cnt < 0 ) {
			free(yyin_struct);
			yyterminate();
		}
		else {
			yy_delete_buffer(YY_CURRENT_BUFFER);
			yy_switch_to_buffer(yyin_struct[yyin_cnt].yyin_buf);
			yyline = yyin_struct[yyin_cnt].yyin_line;
		}
	}

%%

int yywrap(void)
{
    return 1;
}
