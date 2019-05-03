%{
#include <fxos.h>
#include <errors.h>
#include <util.h>

/* Text value for parser */
static char *yylval;

%}

%option prefix="asm"
%option noyywrap
%option nounput

code      ^[01nmdi]{16}
literal   [^ ,\t\n]+|[^ ,\t\n(]*"("[^")"\n]*")"[^ ,\t\n]*

space     [ \t]+

%%

^#[^\n]* ;
{space}  ;
,        ;
[\n]     yylineno++;

{code}               { yylval = strdup(yytext); return 0; }
^.{0,16}             { err("%d: invalid opcode at start of line", yylineno); }

"#imm"               { return IMM; }
"rn"                 { return RN; }
"rm"                 { return RM; }
"jump8"              { return JUMP8; }
"jump12"             { return JUMP12; }
"pcdisp"             { return PCDISP; }
"@rn"                { return AT_RN; }
"@rm"                { return AT_RM; }
"@rm+"               { return AT_RMP; }
"@rn+"               { return AT_RNP; }
"@-rn"               { return AT_MRN; }
"@(disp,"[ ]*"rn)"   { return AT_DRN; }
"@(disp,"[ ]*"rm)"   { return AT_DRM; }
"@(r0,"[ ]*"rn)"     { return AT_R0RN; }
"@(r0,"[ ]*"rm)"     { return AT_R0RM; }
"@(disp",[ ]*"gbr)"  { return AT_DGBR; }
{literal}            { yylval = strdup(yytext); return LITERAL; }

<<EOF>>              { return -1; }

%%

#include <stdio.h>

/* set_code(): Build an efficient representation of an opcode
   Takes a 16-byte string as argument, representing the parameterized opcode,
   and computes a bit-based representation inside the assembly structure.

   @code  16-bit opcode made of '0', '1', 'm', 'n', 'd' and 'i'
   @insn  Instruction object */
void set_code(char const *code, struct asm_insn *insn)
{
	insn->bits = insn->arg_mask = 0;
	insn->n_sh = insn->n_mask = 0;
	insn->m_sh = insn->m_mask = 0;
	insn->d_sh = insn->d_mask = 0;
	insn->i_sh = insn->i_mask = 0;

	for(int i = 0; i < 16; i++)
	{
		int c = code[i];

		/* Constant bits */
		if(c == '0' || c == '1')
		{
			insn->bits = (insn->bits << 1) | (c - '0');
			insn->arg_mask <<= 1;
			continue;
		}

		/* Argument bits */
		insn->bits <<= 1;
		insn->arg_mask = (insn->arg_mask << 1) | 1;

		if(c == 'n')
		{
			insn->n_sh = 15 - i;
			insn->n_mask = (insn->n_mask << 1) | 1;
		}
		if(c == 'm')
		{
			insn->m_sh = 15 - i;
			insn->m_mask = (insn->m_mask << 1) | 1;
		}
		if(c == 'd')
		{
			insn->d_sh = 15 - i;
			insn->d_mask = (insn->d_mask << 1) | 1;
		}
		if(c == 'i')
		{
			insn->i_sh = 15 - i;
			insn->i_mask = (insn->i_mask << 1) | 1;
		}
	}

	insn->arg_mask = ~insn->arg_mask;
}

/* lex_asm(): Assembly table lexer and parser */
struct asm_insn *lex_asm(void *data, size_t length, int *count)
{
	/* First count the number of instruction codes */

	YY_BUFFER_STATE buf = yy_scan_bytes(data, length);
	yylineno = 1;

	int total = 0, t;
	while((t = yylex()) != -1)
	{
		total += (t == 0);
		if(t == 0 || t == LITERAL) free(yylval);
	}

	yy_delete_buffer(buf);

	/* Allocate a large enough instruction array */

	struct asm_insn *table = calloc(total, sizeof *table);
	if(!table)
	{
		errf(ERR_ERRNO, "cannot allocate memory for database");
		return 0;
	}

	/* Lex all instructions and fill in the array */

	buf = yy_scan_bytes(data, length);
	yylineno = 1;

	struct asm_insn *insn = table - 1;
	int line = -1;
	int named = 1;

	while(1)
	{
		t = yylex();

		if(yylineno != line || t == 0 || t == -1)
		{
			/* Finalize current instruction */
			if(!named) err("%d: unnamed instruction", line);
			insn++;
		}
		if(t == -1) break;

		if(t == 0)
		{
			set_code(yylval, insn);
			free(yylval);
			line = yylineno;
			named = 0;
		}
		else if(t == LITERAL && !named)
		{
			insn->mnemonic = yylval;
			named = 1;
		}
		else if(!named)
		{
			err("%d: missing mnemonic", line);
		}
		else if(!insn->arg1)
		{
			insn->arg1 = t;
			if(t == LITERAL) insn->literal1 = yylval;
		}
		else if(!insn->arg2)
		{
			insn->arg2 = t;
			if(t == LITERAL) insn->literal2 = yylval;
		}
	}

	yy_delete_buffer(buf);
	if(count) *count = insn - table;
	return table;
}
