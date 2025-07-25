/**
 * @file lexer.l
 * @brief Grammar for HTTP
 *
 * This file contains grammar for HTTP packets defined in RFC 2616
 * section 2: Notational Conventions and Generic Grammar.
 *
 * @author Rajul Bhatnagar (2016)
 */

%{
#include <unistd.h>

/* This file is generated by yacc */
#include "y.tab.h"


/* Define LEXDEBUG to enable debug messages for this lex file */
//#define LEXDEBUG
#ifdef LEXDEBUG
#include <stdio.h>
#define LPRINTF(...) printf(__VA_ARGS__)
#else
#define LPRINTF(...)
#endif

#undef YY_INPUT
/*
 * yylex() by default takes input from stdin. You might be wondering how
 * to parse data from a buffer instead. This is how you do it. The way
 * it works is, when yylex() needs more input, it invokes a macro
 * called YYINPUT:
 *
 * YY_INPUT(lex_internal_buffer, number_of_bytes_read, max_number_of_bytes_to_read)
 *
 * We hack it, and we undef the macro, and redefine it to something else!
 *
 * The usage of this macro will be clear from the lex-yacc-example.
 */

/* We need some global state (must be defined in parser.y) */
extern char *parsing_buf;	/* The buffer to read the data from */
extern size_t parsing_buf_siz;	/* Size of the buffer */
extern int parsing_offset;	/* Current offset in the buffer */

#define MIN(__a, __b) (((__a) < (__b)) ? (__a) : (__b))

/* Redefine YY_INPUT to read from a buffer instead of stdin! */

#define YY_INPUT(__b, __r, __s) do {					\
		__r = MIN(__s, parsing_buf_siz - parsing_offset);	\
		memcpy(__b, parsing_buf + parsing_offset, __r);		\
		parsing_offset += __r;					\
	} while(0)



%}

/*
 * Following is a list of rules specified in RFC 2616 section 2:
 *
 * Lookup Table
 * cr              \x0d
 * lf              \x0a
 * sp              \x20
 * ht              \x09
 * quote           \"
 * digit           [0-9]
 * ctl             [\x0-\x1f\x7f]
 * upalpha         [A-Z]
 * loalpha         [a-z]
 * alpha	       [A-Za-z]
 * char            [\x0-\x7f]
 * octet           [\x0-\x1f\xff]
 * crlf            {cr}{lf}
 * lws             \x0d\x0a(\x20|\x09)*
 * hex             [ABCDEFabcdef0-9]
 * separators	   [\{\}\(\)\<\>@,;:\\\"/\[\]?=\x20\x09]
 */

/**
 * Declarations
 */

/* Matches a digit. For e.g., 0, 8. */
digit		[0-9]

/* Matches a CRLF. Carriage return - linefeed sequence */
crlf            \x0d\x0a

/* Matches a Colon */
colon           :

/* Matches a space */
sp		\x20

/* Matches a any combination of spaces and horizontal tabs */
ws		[\x20\x09]*

/* Matches a CRLF followed by a ws */
lws             \x0d\x0a(\x20|\x09)*

/*
 * matches following characters: (RFC 2616, Section 2.2)
 * ( ) < > @ , ; : \ " / [ ] ? = { } <space> <tab>
 */
separators	[\(\)\<\>@\,;:\\\"\/\[\]?=\{\}\x20\x09]

/* Matches a CTL*/
ctl	[\x0-\x1f\x7f]

/*
 * (RFC 2616, Section 2.2)
 * This rule matches _ANY_ character _EXCEPT_ separators (see above),
 * and control characters (ascii values 0x0 - 0x1F and 0x7f).
 *
 * token_char = (   char    -       ctl      -               separators             )
 *
 * Note: A token can be detected as any combination of token characters.
 */
token_char      [\x0-\x7f]{-}[\x0-\x1f\x7f]{-}[\{\}\(\)\<\>@\,;:\\\"/\[\]?=\x20\x09]

%%
%{
/*
 * Actions
 *
 * yytext: yytext is the "string" that matches a certain rule. For example,
 *         in the first rule 1: slash, you get the string that matched
 *         (in this case "/") in yytext.
 *
 * yylval: yylval is a variable used to communicate matched value in lex to
 *         yacc. yylval is a union of different types (please see parser.y)
 *         file for details.
 */
%}

"\\"	{
	/* Rule 0: Backslash */

	LPRINTF("t:backslash; \n");

	/* Copy character to yylval.i*/
	yylval.i = yytext[0];

	/*
	 * This return statement lets terminates yylex() function and lets
	 * yacc know that a slash was found!
	 */
	return t_backslash;
}

"\/"	{
	/* Rule 1: Slash */

	LPRINTF("t:slash; \n");

	/* Copy character to yylval.i*/
	yylval.i = yytext[0];

	/*
	 * This return statement lets terminates yylex() function and lets
	 * yacc know that a slash was found!
	 */
	return t_slash;
}

{crlf} {
	/* Rule 2: CRLF */

	LPRINTF("t:crlf; \n");

	/*
	 * No need to communicate the value of CRLF to yacc, so no
	 * yylval here.
	 */

	return t_crlf;
}

{sp} {
	/* Rule 3: Space */

	LPRINTF("t:sp '%s'; \n", yytext);

	yylval.i = yytext[0];

	return t_sp;
}

{ws} {
	/* Rule 4: A sequence of white spaces */

	LPRINTF("t:ht; \n");

	/* Very important to communicate the value here! */
	strcpy(yylval.str, yytext);

	return t_ws;
}

{digit} {
	/* Rule 5: A digit */

	LPRINTF("t:digit %d; \n", atoi(yytext));

	yylval.i = atoi(yytext);

	return t_digit;
}

"." {
	/* Rule 6: A dot */

	LPRINTF("t:dot; \n");
	yylval.i = '.';
	return t_dot;
}

{colon} {
	/* Rule 7: A colon */

	LPRINTF("t:colon; \n");
	yylval.i = ':';
	return t_colon;
}

{separators} {
	/* Rule 8: A separator */

	LPRINTF("t:separators \'%s\'\n", yytext);
	yylval.i = yytext[0];
	return t_separators;
}

{token_char} {
	/* Rule 9: A character allowed in a token */

	LPRINTF("t:token_char %s\n", yytext);
	/*
	 * Again, it is important to communicate the value back
	 * Otherwise, yacc has no way to know which character matched the rule
	 */
	yylval.i = yytext[0];
	return t_token_char;
}

{lws} {
	/* Rule 10: Linear white spaces */

	LPRINTF("t:lws\n");
	return t_lws;
}

{ctl} {
	LPRINTF("t:ctl\n");
	return t_ctl;
}

%%

int yywrap(void) {
    return 0;
}
