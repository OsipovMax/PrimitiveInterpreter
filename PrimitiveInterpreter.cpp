// PrimitiveInterpreter.cpp: ���������� ����� ����� ��� ����������� ����������.
//

#include "stdafx.h"

#pragma warning(disable : 4996)

using namespace std;

#include <string.h> 
#include <ctype.h>  
#include <stdlib.h>  
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>

#define PROG_SIZE       10000
#define _SIZE           100


enum reservs {
	ARG, pCHAR, pINT, IF, ELSE,
	FOR, DO, WHILE, RETURN,
	STRUCT, CLASS, FINISHED
};

enum internal_token {
	DELIMITER, IDENTIFIER, NUMBER, KEYWORD,
	TEMP, STRING, BLOCK
};

enum comp_ops { LT = 1, LTE, RT, RTE, EQ, NE };


enum error_msg
{
	SYNTAX, NO_EXP, NOT_VAR, SEMI_EXPECTED,
	FUNC_UNDEF, TYPE_EXPECTED, BRACKETS_EXPECTED,
	BROKE_BALANCE, NOT_STRING,
	TOO_MUCH_LVARS, DIV_BY_ZERO
};

struct func_type {
	string func_name;
	int ret_type;
	char *loc;
} func_table[_SIZE];

struct struct_type {
	string struct_name;
	int st_type;
	char *struct_loc;
	vector <string> char_param;
	vector <string> int_param;
} struct_table[_SIZE];

struct class_type {
	string class_name;
	int cl_type;
	char *class_loc;
	vector <string> cl_char_param;
	vector <string> cl_int_param;
	vector <string> f_name;
	char** f_loc = new char*[10];
	vector <int> cl_fc_ret_type;
}class_table[_SIZE];

struct reserved_words { // ������� ����������������� ����
	string command;
	int ltok;
} table[30] = {
	"if",    IF,
	"else",  ELSE,
	"for",   FOR,
	"do",    DO,
	"while", WHILE,
	"char",  pCHAR,
	"int",   pINT,
	"return",RETURN,
	"struct", STRUCT,
	"class", CLASS
};

struct var_type {
	string var_name;
	int v_type;
	int value;
	char *struct_lc;
}  global_vars[_SIZE];

struct var_type local_vars[_SIZE];
//---------------------------------------------------------------------------
char     *p_buf;        //��������� �� ������ ������ ���������
char     *ptr_program; //������� ������� � �������� ������ ���������



int  class_index;
int  call_stack[_SIZE];
int  functos;   //������ ������� ����� ������ ������� 
string classObj;
int  gb_var_index;  //������ � ������� ���������� ����������
int  func_index;   //������ � ������� ������� 
int  struct_index;

int  return_value; // ������������ �������� ������� 
int  l_var_s;   //������ � ����� ��������� ���������� 
char token[80];				 //��������
char token_type;			//��� �������
char intren_tok_type;       //���������� ����������

							//������� �����������
int   get_token();
void  recover();
void  sntx_err(int error);
int   search_reserv(char *s);
void  expression(int *value);
void  expression0(int *value);
void  expression1(int *value);
void  expression2(int *value);
void  expression3(int *value);
void  expression4(int *value);
void  expression5(int *value);
void  res_expr(int *value);
int   choice_lib_func(char *s);


//������� ��������������
int   load_program(char *p, char *fname);
void  first_scan();
void  interp_block();
char *search_function(char *name);
void  decl_global();
void  decl_local();
void  call();
void  get_args();
void  get_params();
void  return_func();
void  local_push(struct var_type i);
int   func_pop();
void  func_push(int i);
void  assign_var(char *var_name, int value);
int   find_var(char *s);
int   is_var(char *s);
void  exec_if();
void  exec_while();
void  exec_do();
void  serach_endBlock();
void  exec_for();
void  abra();
void  abra1();



//������������ ������� 
int   scan_char();
int   print_char();
int   print();
int   scan_number();

struct library_functions {// ������� ������������ �������
	string f_name;
	int(*p)();   // ��������� �� �������
} library_func[5] = {
	"scan_char", scan_char,
	"print_char",  print_char,
	"print",  print,
	"scan_number", scan_number
};
//---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "russian");

	//��������� ������ ��� ���������
	if ((p_buf = new (std::nothrow) char[PROG_SIZE]) == nullptr) {
		//�������� ������ �� �������
		cout << "�� ������� �������� ������";
		exit(1);
	}

	// �������� ��������� ��� ����������
	if (!load_program(p_buf, argv[1]))
		exit(1);

	for (int i = 0; i < 3; i++) {
		struct_table[i].int_param.resize(10);
		struct_table[i].char_param.resize(10);

		class_table[i].cl_int_param.resize(10);
		class_table[i].cl_char_param.resize(10);
		class_table[i].f_name.resize(10);
		class_table[i].cl_fc_ret_type.resize(10);
	}

	gb_var_index = 0;  //������������� ������� ���������� ����������

					   //��������� ��������� ��������� �� ������ ������ ���������
	ptr_program = p_buf;
	first_scan(); // ����������� ������� ���� �������
				  // � ���������� ���������� ���������

	l_var_s = 0;// ������������� ������� ����� ��������� ����������
	functos = 0;// ������������� ������� ����� ������ (CALL)

				// ������ ���������� main()
	ptr_program = search_function("main"); // ����� ����� ����� ���������

	if (!ptr_program) { // ������� main() ����������� ��� �����������

		cout << "main() - �� �������" << endl;
		exit(1);
	}

	ptr_program--; // ������� � ������������� ������ (
	strcpy(token, "main");
	call(); // ������ ������������� main() ;
	system("pause");
	return 0;
}

//---------------------- �������������

// �������� ���������.
int load_program(char *p, char *fname)
{

	std::ifstream fin(fname, std::ios::binary);
	if (!fin) return 0;
	fin.read(p, PROG_SIZE).gcount();
	p[fin.gcount()] = 0;

	return 1;
}

//����� ������ ���� ������� � ��������� � ��������� ���������� ����������.
void first_scan()
{
	char *p, *tp;
	char temp[32], cl_fc_temp[32];
	int datatype;
	int cl_fc_type;
	int brace = 0;

	p = ptr_program;
	func_index = 0;
	struct_index = 0;

	do {
		while (brace) { //����� ���� �������
			get_token();
			if (*token == '{') brace++;
			if (*token == '}') brace--;
		}

		tp = ptr_program; //����������� ������� �������
		get_token();
		// ��� ���������� ���������� ��� ������������� �������� �������
		if (strcmp(token, "class") == 0) {

			datatype = CLASS;
			get_token();
			if (token_type == IDENTIFIER) {
				strcpy(temp, token);
				get_token();
				if (*token != '(') {
					abra1();

					class_table[class_index].cl_type = datatype;
					class_table[class_index].class_name = temp;
					class_index++;
					table[20 + class_index].command = temp;
					table[20 + class_index].ltok = datatype;

				}

			}
			else recover();
		}
		else if (strcmp(token, "struct") == 0) {
			datatype = STRUCT;
			get_token();
			if (token_type == IDENTIFIER) {
				strcpy(temp, token);
				get_token();
				if (*token != '(') {
					struct_table[struct_index].struct_loc = ptr_program;
					abra();
					struct_table[struct_index].st_type = datatype;
					struct_table[struct_index].struct_name = temp;
					struct_index++;
					table[13 + struct_index].command = temp;
					table[13 + struct_index].ltok = datatype;

				}
			}
			else recover();
		}
		else if (intren_tok_type == pCHAR || intren_tok_type == pINT
			|| intren_tok_type == STRUCT || intren_tok_type == CLASS) {
			datatype = intren_tok_type; // ����������� ���� ������
			get_token();
			if (token_type == IDENTIFIER) {
				strcpy(temp, token);
				get_token();
				if (*token != '(') { // ��� ������ ���� ���������� ����������
					ptr_program = tp; // ������� � ������ ����������
					decl_global();
				}
				else if (*token == '(') {  // ��� ������ ���� �������
					func_table[func_index].loc = ptr_program;
					func_table[func_index].ret_type = datatype;
					func_table[func_index].func_name = temp;
					func_index++;
					while (*ptr_program != ')') ptr_program++;
					ptr_program++;
				}
				else recover();
			}
		}
		else if (*token == '{') brace++;
	} while (intren_tok_type != FINISHED);
	ptr_program = p;
}



void abra() {
	char temp[32];
	int cnt_int = 0, cnt_char = 0;
	do {

		get_token();
		if (intren_tok_type == pINT) {
			get_token();
			do {

				if (token_type == IDENTIFIER) {
					struct_table[struct_index].int_param[cnt_int] = token;
					cnt_int++;
					get_token();
					if (*token == ',')
						get_token();
				}
			} while (*token != ';');
		}

		if (intren_tok_type == pCHAR) {
			get_token();
			do {

				if (token_type == IDENTIFIER) {

					struct_table[struct_index].char_param[cnt_char] = token;
					cnt_char++;
					get_token();
					if (*token == ',')
						get_token();
				}
			} while (*token != ';');
		}

	} while (*token != '}');
}

void abra1() {
	char temp[32], cl_fc_temp[32];
	int cl_fc_type = 0;
	int cnt_int = 0, cnt_char = 0, cnt_fn_int = 0, cnt_fn_char = 1;
	int brace = 1;
	do {

		get_token();

		if (*token == '{') brace++;
		else if (*token == '}') brace--;

		if (intren_tok_type == pINT) {
			get_token();
			if (token_type == IDENTIFIER) {
				cl_fc_type = intren_tok_type;
				strcpy(cl_fc_temp, token);
				get_token();
				if (*token == '(') {  // ��� ������ ���� �������
					class_table[class_index].f_loc[cnt_fn_int] = ptr_program;
					class_table[class_index].cl_fc_ret_type[cnt_fn_int] = cl_fc_type;
					class_table[class_index].f_name[cnt_fn_int] = cl_fc_temp;
					cnt_fn_int = cnt_fn_int + 2;
					while (*ptr_program != ')') ptr_program++;
					ptr_program++;
				}
				else {
					while (*ptr_program != ' ') ptr_program--;
					get_token();
					do {
						class_table[class_index].cl_int_param[cnt_int] = token;
						cnt_int++;
						get_token();
						if (*token == ',')
							get_token();
					} while (*token != ';');
				}
			}
		}

		if (intren_tok_type == pCHAR) {
			get_token();
			if (token_type == IDENTIFIER) {
				cl_fc_type = intren_tok_type;
				strcpy(cl_fc_temp, token);
				get_token();
				if (*token == '(') {  // ��� ������ ���� �������
					class_table[class_index].f_loc[cnt_fn_char] = ptr_program;
					class_table[class_index].cl_fc_ret_type[cnt_fn_char] = cl_fc_type;
					class_table[class_index].f_name[cnt_fn_char] = cl_fc_temp;
					cnt_fn_char = cnt_fn_char + 2;
					while (*ptr_program != ')') ptr_program++;
					ptr_program++;
				}
				else {

					while (*ptr_program != ' ') ptr_program--;
					get_token();

					do {
						class_table[class_index].cl_char_param[cnt_char] = token;
						cnt_char++;
						get_token();
						if (*token == ',')
							get_token();
					} while (*token != ';');
				}
			}
		}
	} while (brace != 0);
}

// ������������� ������ ��������� ��� �����. 
void interp_block()
{
	int value;
	int block = 0;

	do {
		token_type = get_token();

		// ����������� ���� �������
		if (token_type == IDENTIFIER) {
			// ��� �� ������������������ �����,
			// �������������� ���������.
			recover();  // ������� ��������� ������� �����
						//  ��� ���������� ��������� �������� expression()
			expression(&value);  // ��������� ���������
			if (*token != ';') sntx_err(SEMI_EXPECTED);
		}
		else if (token_type == BLOCK) {
			// ���� ��� ����������� �����
			if (*token == '{') // ����
				block = 1; // ������������� �����, � �� ���������
			else return; // ��� }, �������
		}
		else // ����������������� �����
			switch (intren_tok_type) {
			case pCHAR:
			case CLASS:
			case STRUCT:
			case pINT:
				recover();
				decl_local();
				break;
			case RETURN:
				return_func();
				return;
			case IF:
				exec_if();
				break;
			case ELSE:
				serach_endBlock();

				break;
			case WHILE:
				exec_while();
				break;
			case DO:
				exec_do();
				break;
			case FOR:
				exec_for();
				break;
			}
	} while (intren_tok_type != FINISHED && block);
}

// ������� ������ ����� ����� ������ �������
char *search_function(char *name)
{

	for (int i = 0; i < func_index; i++)
		if (name == func_table[i].func_name)
			return func_table[i].loc;

	return 0;
}

// ���������� ���������� ����������.
void decl_global()
{
	int vartype;

	get_token();  // ����������� ����

	vartype = intren_tok_type; // ����������� ���� ����������

	do { // ��������� ������
		if (vartype == STRUCT) {
			for (int i = 0; i <= 10; i++)
				if (token == struct_table[i].struct_name) {
					global_vars[gb_var_index].struct_lc = struct_table[i].struct_loc;
					global_vars[gb_var_index].value = 0;  // ������������� �����
					get_token();  // ����������� �����
					for (int j = 0; j<struct_index; j++)
						for (int i = 0; !(struct_table[j].int_param[i].empty()); i++) {
							global_vars[gb_var_index].var_name = token;
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + '.';
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + struct_table[j].int_param[i];
							global_vars[gb_var_index].v_type = pINT;
							gb_var_index++;
						}
					for (int j = 0; j<struct_index; j++)
						for (int i = 0; !(struct_table[j].char_param[i].empty()); i++) {
							global_vars[gb_var_index].var_name = token;
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + '.';
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + struct_table[j].char_param[i];
							global_vars[gb_var_index].v_type = pCHAR;
							gb_var_index++;
						}

					get_token();
				}
		}
		else if (vartype == CLASS) {
			for (int i = 0; i <= 10; i++)
				if (class_table[i].class_name == token) {
					global_vars[gb_var_index].struct_lc = class_table[i].class_loc;
					global_vars[gb_var_index].value = 0;
					get_token();
					for (int j = 0; j<class_index; j++)
						for (int i = 0; !(class_table[j].cl_int_param[i].empty()); i++) {
							global_vars[gb_var_index].var_name = token;
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + '.';
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + class_table[j].cl_int_param[i];
							global_vars[gb_var_index].v_type = pINT;
							gb_var_index++;
						}
					for (int j = 0; j<class_index; j++)
						for (int i = 0; !(class_table[j].cl_char_param[i].empty()); i++) {
							global_vars[gb_var_index].var_name = token;
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + '.';
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + class_table[j].cl_char_param[i];
							global_vars[gb_var_index].v_type = pCHAR;
							gb_var_index++;
						}
					for (int j = 0; j<class_index; j++)
						for (int i = 0; !(class_table[j].f_name[i].empty()); i++) {
							func_table[func_index].func_name = token;
							func_table[func_index].func_name = func_table[func_index].func_name + ".";
							func_table[func_index].func_name = func_table[func_index].func_name + class_table[j].f_name[i];
							func_table[func_index].ret_type = class_table[j].cl_fc_ret_type[i];
							func_table[func_index].loc = class_table[j].f_loc[i];
							func_index++;
						}
					get_token();

				}
		}
		else {
			global_vars[gb_var_index].v_type = vartype;
			global_vars[gb_var_index].value = 0;
			get_token();

			global_vars[gb_var_index].var_name = token;
			get_token();
			gb_var_index++;
		}
	} while (*token == ',');
	if (*token != ';') sntx_err(SEMI_EXPECTED);
}



// ���������� ��������� ����������.
void decl_local()
{
	struct var_type i;

	get_token();  // ����������� ����
	i.v_type = intren_tok_type;
	i.value = 0;  // ������������� �����

	do { // ��������� ������
		if (i.v_type == STRUCT) {
			for (int i = 0; i <= 10; i++)
				if (token == struct_table[i].struct_name) {
					global_vars[gb_var_index].struct_lc = struct_table[i].struct_loc;
					global_vars[gb_var_index].value = 0;  // ������������� �����
					get_token();  // ����������� �����
					for (int j = 0; j<struct_index; j++)
						for (int i = 0; !(struct_table[j].int_param[i].empty()); i++) {
							global_vars[gb_var_index].var_name = token;
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + '.';
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + struct_table[j].int_param[i];
							global_vars[gb_var_index].v_type = pINT;
							gb_var_index++;
						}
					for (int j = 0; j<struct_index; j++)
						for (int i = 0; !(struct_table[j].char_param[i].empty()); i++) {
							global_vars[gb_var_index].var_name = token;
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + '.';
							global_vars[gb_var_index].var_name = global_vars[gb_var_index].var_name + struct_table[j].char_param[i];
							global_vars[gb_var_index].v_type = pCHAR;
							gb_var_index++;
						}

					get_token();
				}
		}
		else {
			get_token(); // ����������� ����� ���������
			i.var_name = token;
			local_push(i);
			get_token();
		}
	} while (*token == ',');
	if (*token != ';') sntx_err(SEMI_EXPECTED);
}

// ����� �������.
void call()
{

	char *loc, *temp;
	int lvartemp;

	loc = search_function(token); // ����� ����� ����� �������
	if (loc == 0)
		sntx_err(FUNC_UNDEF); // ������� �� ����������
	else {
		lvartemp = l_var_s;  // ����������� ������� �����
							 //  ��������� ����������
		get_args();  // ��������� ��������� �������
		temp = ptr_program; // ����������� ������ ��������
		func_push(lvartemp);  // ����������� ������� �����
							  //  ��������� ����������
		ptr_program = loc;  // ������������� prog � ������ �������
		get_params(); // �������� ���������� �������
					  // ���������� ����������
		interp_block(); // ������������� �������
		ptr_program = temp; // �������������� prog
		l_var_s = func_pop(); // �������������� �����
							  // ��������� ����������
	}
}

// ������������ ���������� ������� � ���� ��������� ����������.
void get_args()
{
	int value, count, temp[32];
	struct var_type i;

	count = 0;
	get_token();
	if (*token != '(') sntx_err(BRACKETS_EXPECTED);

	// ��������� ������ ��������
	do {

		expression(&value);
		temp[count] = value;
		get_token();
		count++;
	} while (*token == ',');
	count--;
	// ����������� �������� �������
	for (; count >= 0; count--) {
		i.value = temp[count];
		i.v_type = ARG;
		local_push(i);
	}
}

// ��������� ���������� �������.
void get_params()
{
	struct var_type *p;
	int i;

	i = l_var_s - 1;
	do { // ��������� ������ ����������
		get_token();

		p = &local_vars[i];
		if (*token != ')') {
			if (intren_tok_type != pINT && intren_tok_type != pCHAR) {

				sntx_err(TYPE_EXPECTED);
			}

			p->v_type = token_type;
			get_token();

			// ���������� ����� ���������� � ����������,
			//  ��� ����������� � ����� ��������� ����������
			p->var_name = token;
			get_token();
			i--;
		}
		else break;
	} while (*token == ',');
	if (*token != ')') sntx_err(BRACKETS_EXPECTED);
}

// ������� �� �������.
void return_func()
{
	int value;

	value = 0;
	// ��������� ������������� ��������, ���� ��� ����
	expression(&value);

	return_value = value;
}

// ���������� ��������� ����������.
void local_push(struct var_type i)
{
	if (l_var_s > _SIZE)
		sntx_err(TOO_MUCH_LVARS);

	local_vars[l_var_s] = i;
	l_var_s++;
}

// ������������ ������� � ����� ��������� ����������.
int func_pop()
{
	functos--;
	return call_stack[functos];
}

// ������ ������� � ���� ��������� ����������.
void func_push(int i)
{
	call_stack[functos] = i;
	functos++;
}

// ������������ ���������� ��������.
void assign_var(char *var_name, int value)
{
	int i = 0;
	string str;
	//�������� ������� ��������� ����������
	for (i = l_var_s - 1; i >= call_stack[functos - 1]; i--) {
		if (local_vars[i].var_name == var_name) {
			local_vars[i].value = value;
			return;
		}
	}

	if (i < call_stack[functos - 1])
		// ���� ���������� �����������,
		// ���� �� � ������� ���������� ����������
		for (i = 0; i < _SIZE; i++)
			if (global_vars[i].var_name == var_name) {
				global_vars[i].value = value;
				return;
			}
	if (!classObj.empty()) {
		str = classObj;
		str = str + '.';
		str = str + var_name;
		for (int i = 0; i< 15; i++)
			if (global_vars[i].var_name == str)
				global_vars[i].value = value;
		return;
	}


	sntx_err(NOT_VAR); //���������� �� �������
}

// ��������� �������� ����������.
int find_var(char *s)
{
	int i;

	// �������� ������� ��� ����������
	for (i = l_var_s - 1; i >= call_stack[functos - 1]; i--)
		if (local_vars[i].var_name == token)
			return local_vars[i].value;

	// �����  ���������� ����������
	for (i = 0; i < _SIZE; i++)
		if (global_vars[i].var_name == s)
			return global_vars[i].value;
	sntx_err(NOT_VAR); // ���������� �� �������
	return -1;
}
// ���� ���������� -1 , ����� - 0
int is_var(char *s)
{


	//  ��������� ���������� 
	for (int i = l_var_s - 1; i >= call_stack[functos - 1]; i--)
		if (local_vars[i].var_name == token)
			return 1;

	//  ���������� ����������
	for (int i = 0; i < _SIZE; i++)
		if (global_vars[i].var_name == s)
			return 1;
	if (!classObj.empty())
		return 1;

	return 0;
}

// ���������� ��������� if.
void exec_if()
{
	int condition;

	expression(&condition); // �������� �������

	if (condition) { //  �������������
		interp_block();
	}
	else { // ������� - if

		serach_endBlock(); // ����� ����� �����
		get_token();

		if (intren_tok_type != ELSE) {
			recover();  // �������������� �������,
						// ���� else-����������� ����������
			return;
		}
		interp_block();
	}
}

// ���������� ����� while.
void exec_while()
{
	int condition;
	char *temp;

	recover();
	temp = ptr_program;  // ����������� ������ ������ ����� while
	get_token();
	expression(&condition);  // ���������� ������������ ���������
	if (condition) interp_block();  // ���� ��� �������, �� ���������
									// �������������
	else {  // � ��������� ������ ���� ������������
		serach_endBlock();
		return;
	}
	ptr_program = temp;  // ������� � ������ �����
}

// ���������� ����� do.
void exec_do()
{
	int condition;
	char *temp;

	recover();
	temp = ptr_program;  // ����������� ������ ������ �����

	get_token(); // ����� ������ �����
	interp_block(); // ������������� �����
	get_token();

	expression(&condition); // �������� ������� �����
	if (condition) ptr_program = temp; // ���� ������� �������,
									   //�� ���� �����������, � ��������� ������ ����������
									   //����� �� �����
}

// ����� ����� �����.
void serach_endBlock()
{
	int brace;
	get_token();
	brace = 1;
	do {
		get_token();
		if (*token == '{') brace++;
		else if (*token == '}') brace--;
	} while (brace);
}

// ���������� ����� for.
void exec_for()
{
	int condition;
	char *temp, *temp2;
	int brace;

	get_token();
	expression(&condition);  // ���������������� ���������
	if (*token != ';') sntx_err(SEMI_EXPECTED);
	ptr_program++; // ������� ; */
	temp = ptr_program;
	for (;;) {
		expression(&condition);  // �������� �������
		if (*token != ';') sntx_err(SEMI_EXPECTED);
		ptr_program++; // ������� ;
		temp2 = ptr_program;

		// ����� ������ ���� �����
		brace = 1;
		while (brace) {
			get_token();
			if (*token == '(') brace++;
			if (*token == ')') brace--;
		}

		if (condition) interp_block();  // ���� ������� ���������,
										// �� ��������� �������������
		else {  // � ��������� ������ ������ ����
			serach_endBlock();
			return;
		}
		ptr_program = temp2;
		expression(&condition); // ��������� ����������
		ptr_program = temp;  // ������� � ������ �����
	}
}

//�������������� ����������-----------------------------------------------

// ����� ����� � �������������� ���������� ���������.
void expression(int *value)
{
	get_token();
	if (!*token) {
		sntx_err(NO_EXP);
		return;
	}
	if (*token == ';') {
		*value = 0; // ������ ���������
		return;
	}
	expression0(value);
	recover(); // ������� ��������� ������� �� ������� �����
}

//��������� ��������� � ������������
void expression0(int *value)
{
	char temp[32];  // �������� ��� ����������, ������� ������������� ��������
	int temp_tok;

	if (token_type == IDENTIFIER) {
		if (is_var(token)) {  // ���� ��� ����������,
							  // ����������, ������������� �� �� ��������
			strcpy(temp, token);
			temp_tok = token_type;
			get_token();
			if (*token == '=') {  // ��� ������������
				get_token();
				expression0(value);  // ��������� ���������� ��������
				assign_var(temp, *value);  // ��������� ��������
				return;
			}
			else {  // �� ������������
				recover();  // ������������� �������
				strcpy(token, temp);
				token_type = temp_tok;
			}
		}
	}
	expression1(value);
}

// ��������� �������� ���������.
void expression1(int *value)
{
	int partial_value;
	char operation;
	char reloperations[7] = {
		LT, LTE, RT, RTE, EQ, NE, 0
	};

	expression2(value);
	operation = *token;
	if (strchr(reloperations, operation)) {
		get_token();
		expression2(&partial_value);
		switch (operation) {  // ���������� ���������� �������� ���������
		case LT:
			*value = *value < partial_value;
			break;
		case LTE:
			*value = *value <= partial_value;
			break;
		case RT:
			*value = *value > partial_value;
			break;
		case RTE:
			*value = *value >= partial_value;
			break;
		case EQ:
			*value = *value == partial_value;
			break;
		case NE:
			*value = *value != partial_value;
			break;
		}
	}
}

//  ������������ ��� ���������� ���� ������.
void expression2(int *value)
{
	char  operation;
	int partial_value;

	expression3(value);
	while ((operation = *token) == '+' || operation == '-') {
		get_token();
		expression3(&partial_value);
		switch (operation) { // ������������ ��� ���������
		case '-':
			*value = *value - partial_value;
			break;
		case '+':
			*value = *value + partial_value;
			break;
		}
	}
}

// ��������� ��� ������� ���� ����������.
void expression3(int *value)
{
	char  operation;
	int partial_value, t;

	expression4(value);
	while ((operation = *token) == '*' || operation == '/' || operation == '%') {
		get_token();
		expression4(&partial_value);
		switch (operation) { // ���������, ������� ��� ������� �����
		case '*':
			*value = *value * partial_value;
			break;
		case '/':
			if (partial_value == 0) sntx_err(DIV_BY_ZERO);
			*value = (*value) / partial_value;
			break;
		case '%':
			t = (*value) / partial_value;
			*value = *value - (t * partial_value);
			break;
		}
	}
}

// ������� + ��� -. 
void expression4(int *value)
{
	char  operation;

	operation = '\0';
	if (*token == '+' || *token == '-') {
		operation = *token;
		get_token();
	}
	expression5(value);
	if (operation)
		if (operation == '-') *value = -(*value);
}

// ��������� ��������� � �������.
void expression5(int *value)
{
	if ((*token == '(')) {
		get_token();
		expression0(value);   // ���������� ������������
		if (*token != ')') sntx_err(BRACKETS_EXPECTED);
		get_token();
	}
	else
		res_expr(value);
}

// ��������� �������� �����, ���������� ��� �������.
void res_expr(int *value)
{
	int i = 0;
	switch (token_type) {
	case IDENTIFIER:
		i = choice_lib_func(token);
		if (i != -1) {  // ����� ������� �� "����������� ���������"
			*value = (*library_func[i].p)();
		}
		else
			if (search_function(token)) { // ����� �������,
										  // ������������ �������������
				if (strchr(token, '.')) {
					for (int i = 0; token[i] != '.'; i++) {
						classObj = classObj + token[i];
					}
				}

				call();
				classObj.clear();
				*value = return_value;
			}
			else *value = find_var(token); // ��������� �������� ����������
			get_token();
			return;
	case NUMBER: // �������� ���������
		*value = atoi(token);
		get_token();
		return;
	case DELIMITER: // ��� ���������� ���������?
		if (*token == '\'') {
			*value = *ptr_program;
			ptr_program++;
			if (*ptr_program != '\'') sntx_err(BROKE_BALANCE);
			ptr_program++;
			get_token();
			return;
		}
		if (*token == ')') return; // ��������� ������� ���������
		else sntx_err(SYNTAX); // �������������� ������
	default:
		sntx_err(SYNTAX); // �������������� ������
	}
}

// ����� ��������� �� ������.
void sntx_err(int error)
{
	char  *temp;
	int line_cnt = 1;
	int i;

	vector <string> message = {
		"-===�������������� ������===-",
		"-===����������� ���������===-",
		"-===������ �� �������� ����������===-",
		"-===��������� ����� � �������===-",
		"-===������� �� ���� ����������===-",
		"-===�������� ��� ����������===-",
		"-===��������� ������===-",
		"-===������� ������ \'===-",
		"-===��������� ������===-",
		"-===������� ����� ��������� ����������===-",
		"-===������� �� ����!===-"
	};
	cout << endl;
	cout << setw(10) << message[error];
	temp = p_buf;
	while (temp != ptr_program) {  //����� ������ ������ � �������
		temp++;
		if (*temp == '\n') {
			line_cnt++;
		}
	}
	cout << endl;
	cout << setw(100) << "������ - " << line_cnt;
	cout << endl;
	while (*temp != '\n') temp--;
	while (*temp != ';') {
		temp++;
		cout << *temp;
	}
	cout << endl;
	system("pause");
	exit(1);
}

// ���������� ������� �� �������� ������.
int get_token()
{

	char *temp_token;

	token_type = 0; intren_tok_type = 0;

	temp_token = token;
	*temp_token = '\0';

	// ������� ��������, �������� ��������� � ������ ������
	while (isspace(*ptr_program) && *ptr_program) ++ptr_program;


	if (*ptr_program == '\0') { // ����� �����
		*token = '\0';
		intren_tok_type = FINISHED;
		return (token_type = DELIMITER);
	}

	if (strchr("{}", *ptr_program)) { // ����������� �����
		*temp_token = *ptr_program;
		temp_token++;
		*temp_token = '\0';
		ptr_program++;
		return (token_type = BLOCK);
	}

	// ����� ������������
	if (*ptr_program == '/')
		if (*(ptr_program + 1) == '*') { // ��� �����������
			ptr_program += 2;
			do { // ����� ����� �����������
				while (*ptr_program != '*') ptr_program++;
				ptr_program++;
			} while (*ptr_program != '/');
			ptr_program++;
		}

	if (strchr("!<>=", *ptr_program)) { // ��������, ���
										// �������� ���������
		switch (*ptr_program) {
		case '=': if (*(ptr_program + 1) == '=') {
			ptr_program++; ptr_program++;
			*temp_token = EQ;
			temp_token++; *temp_token = EQ; temp_token++;
			*temp_token = '\0';
		}
				  break;
		case '!': if (*(ptr_program + 1) == '=') {
			ptr_program++; ptr_program++;
			*temp_token = NE;
			temp_token++; *temp_token = NE; temp_token++;
			*temp_token = '\0';
		}
				  break;
		case '<': if (*(ptr_program + 1) == '=') {
			ptr_program++; ptr_program++;
			*temp_token = LTE; temp_token++; *temp_token = LTE;
		}
				  else {
					  ptr_program++;
					  *temp_token = LT;
				  }
				  temp_token++;
				  *temp_token = '\0';
				  break;
		case '>': if (*(ptr_program + 1) == '=') {
			ptr_program++; ptr_program++;
			*temp_token = RTE; temp_token++; *temp_token = RTE;
		}
				  else {
					  ptr_program++;
					  *temp_token = RT;
				  }
				  temp_token++;
				  *temp_token = '\0';
				  break;
		}
		if (*token) return(token_type = DELIMITER);
	}

	if (strchr("+-*^/%=;(),'", *ptr_program)) { // �����������
		*temp_token = *ptr_program;
		ptr_program++; // ����������� �� ��������� �������
		temp_token++;
		*temp_token = '\0';
		return (token_type = DELIMITER);
	}

	if (*ptr_program == '"') { // ������ � ��������
		ptr_program++;
		while (*ptr_program != '"'&& *ptr_program != '\r') *temp_token++ = *ptr_program++;
		if (*ptr_program == '\r') sntx_err(SYNTAX);
		ptr_program++; *temp_token = '\0';
		return (token_type = STRING);
	}

	if (isdigit(*ptr_program)) { // �����
		while (!(strchr(" !;,+-<>'/*%^=()", *ptr_program))) *temp_token++ = *ptr_program++;
		*temp_token = '\0';
		return (token_type = NUMBER);
	}

	if (isalpha(*ptr_program)) { // ���������� ��� ��������
		while (!strchr(" !;,+-<>'/*%^=()", *ptr_program)) *temp_token++ = *ptr_program++;
		token_type = TEMP;
	}

	*temp_token = '\0';

	//���������� ��� ������
	if (token_type == TEMP) {
		intren_tok_type = search_reserv(token); // ������������� �� ��������� �������������
		if (intren_tok_type) token_type = KEYWORD; // ��� ����������������� �����
		else token_type = IDENTIFIER;
	}
	return token_type;
}

// ������� ������� 
void recover()
{
	char *t;

	t = token;
	for (; *t; t++) ptr_program--;
}

// ����� ����������� ������������� ������� � ������� ������
int search_reserv(char *s)
{

	for (int i = 0; i<25; i++) {
		if (table[i].command == s) {
			return table[i].ltok;
		}
	}
	return 0; // ���������� ��������
}

// ���������� ������ ������������ �������
int choice_lib_func(char *s)
{
	for (int i = 0; i<4; i++) {
		if (library_func[i].f_name == s) return i;
	}
	return -1;
}



// ������������ ������� ------------------------------
int scan_char()
{
	char str;
	str = getchar();
	while (*ptr_program != ')') ptr_program++;
	ptr_program++; // ����������� � ����� ������
	return str;
}
// ����� ������� �� �����.
int print_char()
{
	int value;

	expression(&value);
	printf("%c\n", value);
	return value;
}

// ���������� ������ ����� � ����������.
int scan_number()
{
	string str;
	getline(cin, str);
	while (*ptr_program != ')') ptr_program++;
	ptr_program++;
	return stoi(str);
}

//  ������� ������
int print()
{
	int i;
	get_token();
	if (*token != '(')  sntx_err(BRACKETS_EXPECTED);
	get_token();

	if (token_type == STRING) { // ����� ������
		cout << token << endl;
	}
	else {  // ����� �����
		recover();
		expression(&i);
		cout << i << endl;
	}
	get_token();
	if (*token != ')') sntx_err(BRACKETS_EXPECTED);
	get_token();
	if (*token != ';') sntx_err(BRACKETS_EXPECTED);
	recover();
	return 0;
}