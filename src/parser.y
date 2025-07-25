/**
 * @file parser.y
 * @brief Grammar for HTTP
 * @author Rajul Bhatnagar (2016)
 */

%{
#include "parse.h"

/* Define YACCDEBUG to enable debug messages for this lex file */
//#define YACCDEBUG
#define YYERROR_VERBOSE
#ifdef YACCDEBUG
#include <stdio.h>
#define YPRINTF(...) printf(__VA_ARGS__)
#else
#define YPRINTF(...)
#endif

/* yyparse() calls yyerror() on error */
void yyerror (const char *s);

void set_parsing_options(char *buf, size_t siz, Request *parsing_request);

/* yyparse() calls yylex() to get tokens */
extern int yylex();


/*
** Global variables required for parsing from buffer
** instead of stdin:
*/

/* Pointer to the buffer that contains input */
char *parsing_buf;

/* Current position in the buffer */
int parsing_offset;

/* Buffer size */
size_t parsing_buf_siz;

/* Current parsing_request Header Struct */
Request *parsing_request;

%}


/* Various types values that we can get from lex */
%union {
	char str[8192];
	int i;
}

%start request

/*
 * Tokens that yacc expects from lex, essentially these are the tokens
 * declared in declaration section of lex file.
 */
%token t_crlf
%token t_backslash
%token t_slash
%token t_digit
%token t_dot
%token t_token_char
%token t_lws
%token t_colon
%token t_separators
%token t_sp
%token t_ws
%token t_ctl


/* Type of value returned for these tokens */
%type<str> t_crlf
%type<i> t_backslash
%type<i> t_slash
%type<i> t_digit
%type<i> t_dot
%type<i> t_token_char
%type<str> t_lws
%type<i> t_colon
%type<i> t_separators
%type<i> t_sp
%type<str> t_ws
%type<i> t_ctl

/*
 * Followed by this, you should have types defined for all the intermediate
 * rules that you will define. These are some of the intermediate rules:
 */
%type<i> allowed_char_for_token
%type<i> allowed_char_for_text
%type<str> ows
%type<str> token
%type<str> text

%type<str> method

%%

/*
** The following 2 rules define a token.
*/

/*
 * Rule 1: Allowed characters in a token
 *
 * An excerpt from RFC 2616:
 * --
 * token          = 1*<any CHAR except CTLs or separators>
 * --
 */
allowed_char_for_token: t_token_char;
			| t_digit { $$ = '0' + $1; }; /* parsing to text */
			| t_dot;

/*
 * Rule 2: A token is a sequence of all allowed token chars.
 */
token:
allowed_char_for_token {
	YPRINTF("token: Matched rule 1.\n");
	snprintf($$, 8192, "%c", $1);
}; |
token allowed_char_for_token {
	YPRINTF("token: Matched rule 2.\n");
	memcpy($$, $1, strlen($1));
	$$[strlen($1)] = $2;
	$$[strlen($1) + 1] = 0;
    // snprintf($$, 8192, "%s%c", $1, $2);
};

/*
** The following 2 rules define text.
*/
/*
 *
 * Rule 3: Allowed characters in text
 *
 * An excerpt from RFC 2616, section 2.2:
 * --
 * The TEXT rule is only used for descriptive field contents and values
 * that are not intended to be interpreted by the message parser. Words
 * of *TEXT MAY contain characters from character sets other than ISO-
 * 8859-1 [22] only when encoded according to the rules of RFC 2047
 * [14].
 *
 * TEXT = <any OCTET except CTLs, but including LWS>
 * --
 *
 */

allowed_char_for_text: allowed_char_for_token;
			| t_separators 	{ $$ = $1; };
			| t_colon	{ $$ = $1; };
			| t_slash	{ $$ = $1; };

/*
 * Rule 4: Text is a sequence of characters allowed in text as per RFC. May
 * 	   also contains spaces.
 */
text: allowed_char_for_text {
		YPRINTF("text: Matched rule 1.\n");
		snprintf($$, 8192, "%c", $1);
	};
	| text ows allowed_char_for_text {
		YPRINTF("text: Matched rule 2.\n");
		memcpy($$, $1, strlen($1));
		memcpy($$ + strlen($1), $2, strlen($2));
		$$[strlen($1) + strlen($2)] = $3;
		$$[strlen($1) + strlen($2) + 1] = 0;
		// snprintf($$, 8192, "%s%s%c", $1, $2, $3);
	};

/*
 * Rule 5: Optional white spaces
 */
ows: 	{
		YPRINTF("OWS: Matched rule 1\n");
		$$[0]=0;
	};
	| t_sp	{
			YPRINTF("OWS: Matched rule 2\n");
			snprintf($$, 8192, "%c", $1);
		};
	| t_ws 	{
			YPRINTF("OWS: Matched rule 3\n");
			snprintf($$, 8192, "%s", $1);
		};



request_line: token t_sp text t_sp text t_crlf	{
	YPRINTF("request_Line:\n%s\n%s\n%s\n",$1,$3,$5);
	strcpy(parsing_request->http_method, $1);
	strcpy(parsing_request->http_uri, $3);
	strcpy(parsing_request->http_version, $5);
};

request_header: token ows t_colon ows text ows t_crlf 	{
	YPRINTF("request_Header:\n%s\n%s\n",$1,$5);
	// default for 1 header, resize if needed
	parsing_request->headers = (Request_header *)realloc(parsing_request->headers, sizeof(Request_header) * (parsing_request->header_count + 1));
	strcpy(parsing_request->headers[parsing_request->header_count].header_name, $1);
	strcpy(parsing_request->headers[parsing_request->header_count].header_value, $5);
	parsing_request->header_count++;
};

request_header: request_header request_header
	|;


/*
 * You need to fill this rule, and you are done! You have all the assembly
 * needed. You may wish to define your own rules. Please read RFC 2616.
 * All the best!
 *
 */
request: request_line request_header t_crlf {
	YPRINTF("parsing_request: Matched Success.\n");
	return SUCCESS;
};

%%

/* C code */

// let lex know parsing from the buffer
void set_parsing_options(char *buf, size_t siz, Request *request)
{
	parsing_buf = buf;
	parsing_offset = 0;
	parsing_buf_siz = siz;
	parsing_request = request;
}

void yyerror (const char *s) {fprintf (stderr, "%s\n", s);}
