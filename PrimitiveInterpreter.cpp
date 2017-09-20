// PrimitiveInterpreter.cpp: определяет точку входа для консольного приложения.
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

struct reserved_words { // таблица зарезервированных слов
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
char     *p_buf;        //указывает на начало буфера программы
char     *ptr_program; //текущая позиция в исходном тексте программы



int  class_index;
int  call_stack[_SIZE];
int  functos;   //индекс вершины стека вызова функции 
string classObj;
int  gb_var_index;  //индекс в таблице глобальных переменных
int  func_index;   //индекс в таблице функций 
int  struct_index;

int  return_value; // возвращаемое значение функции 
int  l_var_s;   //индекс в стеке локальных переменных 
char token[80];				 //лексемыа
char token_type;			//тип лексемы
char intren_tok_type;       //внутренний типлексемы

							//Функции анализатора
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


//Функции интерпритатора
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



//Библиотечные функции 
int   scan_char();
int   print_char();
int   print();
int   scan_number();

struct library_functions {// таблица библиотечных функций
	string f_name;
	int(*p)();   // указатель на функцию
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

	//выделение памяти для программы
	if ((p_buf = new (std::nothrow) char[PROG_SIZE]) == nullptr) {
		//Выделить память не удалось
		cout << "Не удалось выделить память";
		exit(1);
	}

	// загрузка программы для выполнения
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

	gb_var_index = 0;  //инициализация индекса глобальных переменных

					   //установка указателя программы на начало буфера программы
	ptr_program = p_buf;
	first_scan(); // определение адресов всех функций
				  // и глобальных переменных программы

	l_var_s = 0;// инициализация индекса стека локальных переменных
	functos = 0;// инициализация индекса стека вызова (CALL)

				// первой вызывается main()
	ptr_program = search_function("main"); // поиск точки входа программы

	if (!ptr_program) { // функция main() неправильна или отсутствует

		cout << "main() - не найдена" << endl;
		exit(1);
	}

	ptr_program--; // возврат к открывающейся скобке (
	strcpy(token, "main");
	call(); // начало интерпритации main() ;
	system("pause");
	return 0;
}

//---------------------- Интерпретатор

// Загрузка программы.
int load_program(char *p, char *fname)
{

	std::ifstream fin(fname, std::ios::binary);
	if (!fin) return 0;
	fin.read(p, PROG_SIZE).gcount();
	p[fin.gcount()] = 0;

	return 1;
}

//Найти адреса всех функций в программе и запомнить глобальные переменные.
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
		while (brace) { //обход кода функции
			get_token();
			if (*token == '{') brace++;
			if (*token == '}') brace--;
		}

		tp = ptr_program; //запоминание текущей позиции
		get_token();
		// тип глобальной переменной или возвращаемого значения функции
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
			datatype = intren_tok_type; // запоминание типа данных
			get_token();
			if (token_type == IDENTIFIER) {
				strcpy(temp, token);
				get_token();
				if (*token != '(') { // это должна быть глобальная переменная
					ptr_program = tp; // возврат в начало объявления
					decl_global();
				}
				else if (*token == '(') {  // это должна быть функция
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
				if (*token == '(') {  // это должна быть функция
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
				if (*token == '(') {  // это должна быть функция
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

// Интерпритация одного оператора или блока. 
void interp_block()
{
	int value;
	int block = 0;

	do {
		token_type = get_token();

		// определение типа лексемы
		if (token_type == IDENTIFIER) {
			// Это не зарегестрированное слово,
			// обрабатывается выражение.
			recover();  // возврат лексемыво входной поток
						//  для дальнейшей обработки функцией expression()
			expression(&value);  // обработка выражения
			if (*token != ';') sntx_err(SEMI_EXPECTED);
		}
		else if (token_type == BLOCK) {
			// если это граничитель блока
			if (*token == '{') // блок
				block = 1; // интерпритация блока, а не оператора
			else return; // это }, возврат
		}
		else // зарезервированное слово
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

// Возврат адреса точки входа данной функции
char *search_function(char *name)
{

	for (int i = 0; i < func_index; i++)
		if (name == func_table[i].func_name)
			return func_table[i].loc;

	return 0;
}

// Объявление глобальной переменной.
void decl_global()
{
	int vartype;

	get_token();  // определение типа

	vartype = intren_tok_type; // запоминание типа переменной

	do { // обработка списка
		if (vartype == STRUCT) {
			for (int i = 0; i <= 10; i++)
				if (token == struct_table[i].struct_name) {
					global_vars[gb_var_index].struct_lc = struct_table[i].struct_loc;
					global_vars[gb_var_index].value = 0;  // инициализация нулем
					get_token();  // определение имени
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



// Объявление локальной переменной.
void decl_local()
{
	struct var_type i;

	get_token();  // определение типа
	i.v_type = intren_tok_type;
	i.value = 0;  // инициализация нулем

	do { // обработка списка
		if (i.v_type == STRUCT) {
			for (int i = 0; i <= 10; i++)
				if (token == struct_table[i].struct_name) {
					global_vars[gb_var_index].struct_lc = struct_table[i].struct_loc;
					global_vars[gb_var_index].value = 0;  // инициализация нулем
					get_token();  // определение имени
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
			get_token(); // определение имени пременной
			i.var_name = token;
			local_push(i);
			get_token();
		}
	} while (*token == ',');
	if (*token != ';') sntx_err(SEMI_EXPECTED);
}

// Вызов функции.
void call()
{

	char *loc, *temp;
	int lvartemp;

	loc = search_function(token); // найти точку входа функции
	if (loc == 0)
		sntx_err(FUNC_UNDEF); // функция не определена
	else {
		lvartemp = l_var_s;  // запоминание индекса стека
							 //  локальных переменных
		get_args();  // получение аргумента функции
		temp = ptr_program; // запоминание адреса возврата
		func_push(lvartemp);  // запоминание индекса стека
							  //  локальных переменных
		ptr_program = loc;  // переустановка prog в начало функции
		get_params(); // загрузка параметров функции
					  // значениями аргументов
		interp_block(); // интерпретация функции
		ptr_program = temp; // восстановление prog
		l_var_s = func_pop(); // восстановление стека
							  // локальных переменных
	}
}

// Заталкивание аргументов функций в стек локальных переменных.
void get_args()
{
	int value, count, temp[32];
	struct var_type i;

	count = 0;
	get_token();
	if (*token != '(') sntx_err(BRACKETS_EXPECTED);

	// обработка списка значений
	do {

		expression(&value);
		temp[count] = value;
		get_token();
		count++;
	} while (*token == ',');
	count--;
	// затолкиваем обратном порядке
	for (; count >= 0; count--) {
		i.value = temp[count];
		i.v_type = ARG;
		local_push(i);
	}
}

// Получение параметров функции.
void get_params()
{
	struct var_type *p;
	int i;

	i = l_var_s - 1;
	do { // обработка списка параметров
		get_token();

		p = &local_vars[i];
		if (*token != ')') {
			if (intren_tok_type != pINT && intren_tok_type != pCHAR) {

				sntx_err(TYPE_EXPECTED);
			}

			p->v_type = token_type;
			get_token();

			// связывание имени пераметров с аргументом,
			//  уже находящимся в стеке локальных переменных
			p->var_name = token;
			get_token();
			i--;
		}
		else break;
	} while (*token == ',');
	if (*token != ')') sntx_err(BRACKETS_EXPECTED);
}

// Возврат из функции.
void return_func()
{
	int value;

	value = 0;
	// получение возвращаемого значения, если оно есть
	expression(&value);

	return_value = value;
}

// Затолкнуть локальную переменную.
void local_push(struct var_type i)
{
	if (l_var_s > _SIZE)
		sntx_err(TOO_MUCH_LVARS);

	local_vars[l_var_s] = i;
	l_var_s++;
}

// Выталкивание индекса в стеке локальных переменных.
int func_pop()
{
	functos--;
	return call_stack[functos];
}

// Запись индекса в стек локальных переменных.
void func_push(int i)
{
	call_stack[functos] = i;
	functos++;
}

// Присваивание переменной значения.
void assign_var(char *var_name, int value)
{
	int i = 0;
	string str;
	//проверка наличия локальной переменной
	for (i = l_var_s - 1; i >= call_stack[functos - 1]; i--) {
		if (local_vars[i].var_name == var_name) {
			local_vars[i].value = value;
			return;
		}
	}

	if (i < call_stack[functos - 1])
		// если переменная нелокальная,
		// ищем ее в таблице глобальных переменных
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


	sntx_err(NOT_VAR); //переменная не найдена
}

// Получение значения переменной.
int find_var(char *s)
{
	int i;

	// проверка наличия лок переменной
	for (i = l_var_s - 1; i >= call_stack[functos - 1]; i--)
		if (local_vars[i].var_name == token)
			return local_vars[i].value;

	// может  глобальная переменная
	for (i = 0; i < _SIZE; i++)
		if (global_vars[i].var_name == s)
			return global_vars[i].value;
	sntx_err(NOT_VAR); // переменная не найдена
	return -1;
}
// Если переменная -1 , иначе - 0
int is_var(char *s)
{


	//  локальная переменная 
	for (int i = l_var_s - 1; i >= call_stack[functos - 1]; i--)
		if (local_vars[i].var_name == token)
			return 1;

	//  глобальная переменная
	for (int i = 0; i < _SIZE; i++)
		if (global_vars[i].var_name == s)
			return 1;
	if (!classObj.empty())
		return 1;

	return 0;
}

// Выполнение оператора if.
void exec_if()
{
	int condition;

	expression(&condition); // проверка условия

	if (condition) { //  интерпретация
		interp_block();
	}
	else { // пропуск - if

		serach_endBlock(); // поиск конца блока
		get_token();

		if (intren_tok_type != ELSE) {
			recover();  // восстановление лексемы,
						// если else-предложение отсутсвует
			return;
		}
		interp_block();
	}
}

// Выполнение цикла while.
void exec_while()
{
	int condition;
	char *temp;

	recover();
	temp = ptr_program;  // запоминание адреса начала цикла while
	get_token();
	expression(&condition);  // вычисление управляющего выражения
	if (condition) interp_block();  // если оно истинно, то выполнить
									// интерпритацию
	else {  // в противном случае цикл пропускается
		serach_endBlock();
		return;
	}
	ptr_program = temp;  // возврат к началу цикла
}

// Выполнение цикла do.
void exec_do()
{
	int condition;
	char *temp;

	recover();
	temp = ptr_program;  // запоминание адреса начала цикла

	get_token(); // найти начало цикла
	interp_block(); // интерпритация цикла
	get_token();

	expression(&condition); // проверка условия цикла
	if (condition) ptr_program = temp; // если условие истинно,
									   //то цикл выполняется, в противном случае происходит
									   //выход из цикла
}

// Поиск конца блока.
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

// Выполнение цикла for.
void exec_for()
{
	int condition;
	char *temp, *temp2;
	int brace;

	get_token();
	expression(&condition);  // инициализирующее выражение
	if (*token != ';') sntx_err(SEMI_EXPECTED);
	ptr_program++; // пропуск ; */
	temp = ptr_program;
	for (;;) {
		expression(&condition);  // проверка условия
		if (*token != ';') sntx_err(SEMI_EXPECTED);
		ptr_program++; // пропуск ;
		temp2 = ptr_program;

		// поиск начала тела цикла
		brace = 1;
		while (brace) {
			get_token();
			if (*token == '(') brace++;
			if (*token == ')') brace--;
		}

		if (condition) interp_block();  // если условие выполнено,
										// то выполнить интерпритацию
		else {  // в противном случае обойти цикл
			serach_endBlock();
			return;
		}
		ptr_program = temp2;
		expression(&condition); // вполнение инкремента
		ptr_program = temp;  // возврат в начало цикла
	}
}

//синтаксический анализатор-----------------------------------------------

// Точка входа в синтаксический анализатор выражений.
void expression(int *value)
{
	get_token();
	if (!*token) {
		sntx_err(NO_EXP);
		return;
	}
	if (*token == ';') {
		*value = 0; // пустое выражение
		return;
	}
	expression0(value);
	recover(); // возврат последней лексемы во входной поток
}

//Обработка выражения в присваивании
void expression0(int *value)
{
	char temp[32];  // содержит имя переменной, которой присваивается значение
	int temp_tok;

	if (token_type == IDENTIFIER) {
		if (is_var(token)) {  // если эта переменная,
							  // посмотреть, присваивается ли ей значение
			strcpy(temp, token);
			temp_tok = token_type;
			get_token();
			if (*token == '=') {  // это присваивание
				get_token();
				expression0(value);  // вычислить присваемое значение
				assign_var(temp, *value);  // присвоить значение
				return;
			}
			else {  // не присваивание
				recover();  // востановление лексемы
				strcpy(token, temp);
				token_type = temp_tok;
			}
		}
	}
	expression1(value);
}

// Обработка операций сравнения.
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
		switch (operation) {  // вычисление результата операции сравнения
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

//  Суммирование или вычисление двух термов.
void expression2(int *value)
{
	char  operation;
	int partial_value;

	expression3(value);
	while ((operation = *token) == '+' || operation == '-') {
		get_token();
		expression3(&partial_value);
		switch (operation) { // суммирование или вычитание
		case '-':
			*value = *value - partial_value;
			break;
		case '+':
			*value = *value + partial_value;
			break;
		}
	}
}

// Умножение или деление двух множителей.
void expression3(int *value)
{
	char  operation;
	int partial_value, t;

	expression4(value);
	while ((operation = *token) == '*' || operation == '/' || operation == '%') {
		get_token();
		expression4(&partial_value);
		switch (operation) { // умножение, деление или деление целых
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

// Унарный + или -. 
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

// Обработка выражения в скобках.
void expression5(int *value)
{
	if ((*token == '(')) {
		get_token();
		expression0(value);   // вычисление подвыражения
		if (*token != ')') sntx_err(BRACKETS_EXPECTED);
		get_token();
	}
	else
		res_expr(value);
}

// Получение значения числа, переменной или функции.
void res_expr(int *value)
{
	int i = 0;
	switch (token_type) {
	case IDENTIFIER:
		i = choice_lib_func(token);
		if (i != -1) {  // вызов функции из "стандартной билиотеки"
			*value = (*library_func[i].p)();
		}
		else
			if (search_function(token)) { // вызов функции,
										  // определенной пользователем
				if (strchr(token, '.')) {
					for (int i = 0; token[i] != '.'; i++) {
						classObj = classObj + token[i];
					}
				}

				call();
				classObj.clear();
				*value = return_value;
			}
			else *value = find_var(token); // получение значения переменной
			get_token();
			return;
	case NUMBER: // числовая константа
		*value = atoi(token);
		get_token();
		return;
	case DELIMITER: // это символьная константа?
		if (*token == '\'') {
			*value = *ptr_program;
			ptr_program++;
			if (*ptr_program != '\'') sntx_err(BROKE_BALANCE);
			ptr_program++;
			get_token();
			return;
		}
		if (*token == ')') return; // обработка пустого выражения
		else sntx_err(SYNTAX); // синтаксическая ошибка
	default:
		sntx_err(SYNTAX); // синтаксическая ошибка
	}
}

// Вывод сообщения об ошибке.
void sntx_err(int error)
{
	char  *temp;
	int line_cnt = 1;
	int i;

	vector <string> message = {
		"-===Синтаксическая ошибка===-",
		"-===Отсутствует выражение===-",
		"-===Символ не является переменной===-",
		"-===Ожидалась точка с запятой===-",
		"-===Функция не была определена===-",
		"-===Ожидался тип переменной===-",
		"-===Ожидались скобки===-",
		"-===Нарушен баланс \'===-",
		"-===Ожидалась строка===-",
		"-===Слишком много локальных переменных===-",
		"-===Деление на нуль!===-"
	};
	cout << endl;
	cout << setw(10) << message[error];
	temp = p_buf;
	while (temp != ptr_program) {  //поиск номера строки с ошибкой
		temp++;
		if (*temp == '\n') {
			line_cnt++;
		}
	}
	cout << endl;
	cout << setw(100) << "строка - " << line_cnt;
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

// Считывание лексемы из входного потока.
int get_token()
{

	char *temp_token;

	token_type = 0; intren_tok_type = 0;

	temp_token = token;
	*temp_token = '\0';

	// пропуск пробелов, символов табуляции и пустой строки
	while (isspace(*ptr_program) && *ptr_program) ++ptr_program;


	if (*ptr_program == '\0') { // конец файла
		*token = '\0';
		intren_tok_type = FINISHED;
		return (token_type = DELIMITER);
	}

	if (strchr("{}", *ptr_program)) { // ограничение блока
		*temp_token = *ptr_program;
		temp_token++;
		*temp_token = '\0';
		ptr_program++;
		return (token_type = BLOCK);
	}

	// поиск комментариев
	if (*ptr_program == '/')
		if (*(ptr_program + 1) == '*') { // это комментарий
			ptr_program += 2;
			do { // найти конец комментария
				while (*ptr_program != '*') ptr_program++;
				ptr_program++;
			} while (*ptr_program != '/');
			ptr_program++;
		}

	if (strchr("!<>=", *ptr_program)) { // возможно, это
										// оператор сравнения
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

	if (strchr("+-*^/%=;(),'", *ptr_program)) { // разделитель
		*temp_token = *ptr_program;
		ptr_program++; // продвижение на следующую позицию
		temp_token++;
		*temp_token = '\0';
		return (token_type = DELIMITER);
	}

	if (*ptr_program == '"') { // строка в кавычках
		ptr_program++;
		while (*ptr_program != '"'&& *ptr_program != '\r') *temp_token++ = *ptr_program++;
		if (*ptr_program == '\r') sntx_err(SYNTAX);
		ptr_program++; *temp_token = '\0';
		return (token_type = STRING);
	}

	if (isdigit(*ptr_program)) { // число
		while (!(strchr(" !;,+-<>'/*%^=()", *ptr_program))) *temp_token++ = *ptr_program++;
		*temp_token = '\0';
		return (token_type = NUMBER);
	}

	if (isalpha(*ptr_program)) { // переменная или оператор
		while (!strchr(" !;,+-<>'/*%^=()", *ptr_program)) *temp_token++ = *ptr_program++;
		token_type = TEMP;
	}

	*temp_token = '\0';

	//переменная или строка
	if (token_type == TEMP) {
		intren_tok_type = search_reserv(token); // преобразовать во внутренее представление
		if (intren_tok_type) token_type = KEYWORD; // это зарезервированное слово
		else token_type = IDENTIFIER;
	}
	return token_type;
}

// Возврат лексемы 
void recover()
{
	char *t;

	t = token;
	for (; *t; t++) ptr_program--;
}

// Поиск внутреннего представления лексемы в таблице лексем
int search_reserv(char *s)
{

	for (int i = 0; i<25; i++) {
		if (table[i].command == s) {
			return table[i].ltok;
		}
	}
	return 0; // незнакомый оператор
}

// Возвращает индекс библиотечной функции
int choice_lib_func(char *s)
{
	for (int i = 0; i<4; i++) {
		if (library_func[i].f_name == s) return i;
	}
	return -1;
}



// Библиотечные функции ------------------------------
int scan_char()
{
	char str;
	str = getchar();
	while (*ptr_program != ')') ptr_program++;
	ptr_program++; // продвижение к концу строки
	return str;
}
// Вывод символа на экран.
int print_char()
{
	int value;

	expression(&value);
	printf("%c\n", value);
	return value;
}

// Считывание целого числа с клавиатуры.
int scan_number()
{
	string str;
	getline(cin, str);
	while (*ptr_program != ')') ptr_program++;
	ptr_program++;
	return stoi(str);
}

//  функция вывода
int print()
{
	int i;
	get_token();
	if (*token != '(')  sntx_err(BRACKETS_EXPECTED);
	get_token();

	if (token_type == STRING) { // вывод строки
		cout << token << endl;
	}
	else {  // вывод числа
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