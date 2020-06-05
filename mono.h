/* Mono (minimam Lisp interpreter)
written by kenichi sasagawa 2011/12
ver 0.24
*/ 

#define HEAPSIZE 10000000
#define FREESIZE       50
#define STACKSIZE  30000
#define SYMSIZE	256
#define BUFSIZE 256
#define NIL	1
#define T 5

typedef enum tag {EMP,NUM,SYM,LIS,SUBR,FSUBR,FUNC} tag;
typedef enum flag {FRE,USE} flag;


struct cell {
	tag tag;
    flag flag;
	char *name;
	union{
    	int num;
    	int bind;
		int ( *subr) ();
    } val;
    int car;
    int cdr;
};

typedef struct cell cell;

typedef enum toktype {LPAREN,RPAREN,QUOTE,DOT,NUMBER,SYMBOL,OTHER} toktype;
typedef enum backtrack {GO,BACK} backtrack;

struct token {
	char ch;
    backtrack flag;
	toktype type;
    char buf[BUFSIZE];
};

typedef struct token token;

#define GET_CAR(addr)		heap[addr].car
#define GET_CDR(addr)		heap[addr].cdr
#define GET_NUMBER(addr)	heap[addr].val.num
#define GET_NAME(addr)		heap[addr].name
#define GET_TAG(addr)		heap[addr].tag
#define GET_BIND(addr)		heap[addr].val.bind
#define GET_SUBR(addr)		heap[addr].val.subr
#define GET_FLAG(addr)		heap[addr].flag
#define SET_TAG(addr,x)		heap[addr].tag = x
#define SET_CAR(addr,x)		heap[addr].car = x
#define SET_CDR(addr,x)		heap[addr].cdr = x
#define SET_NUMBER(addr,x)	heap[addr].val.num = x
#define	SET_BIND(addr,x)	heap[addr].val.bind = x
#define SET_NAME(addr,x)	heap[addr].name = (char *)malloc(SYMSIZE); strcpy(heap[addr].name,x);
#define SET_SUBR(addr,x)	heap[addr].val.subr = (int (*)())x
#define IS_SYMBOL(addr)		heap[addr].tag == SYM
#define IS_NUMBER(addr)		heap[addr].tag == NUM
#define IS_LIST(addr)		heap[addr].tag == LIS
#define IS_NIL(addr)		heap[addr].tag == SYM && HAS_NAME(addr,"nil")
#define IS_SUBR(addr)		heap[addr].tag == SUBR
#define IS_FSUBR(addr)		heap[addr].tag == FSUBR
#define IS_FUNC(addr)		heap[addr].tag == FUNC
#define IS_EMPTY(addr)		heap[addr].tag	== EMP
#define HAS_NAME(addr,x)	strcmp(heap[addr].name,x) == 0
#define SAME_NAME(addr1,addr2) strcmp(heap[addr1].name, heap[addr2].name) == 0
#define EQUAL_STR(x,y)		strcmp(x,y) == 0
#define MARK_CELL(addr)		heap[addr].flag = USE
#define NOMARK_CELL(addr)	heap[addr].flag = FRE
#define USED_CELL(addr)		heap[addr].flag == USE
#define FREE_CELL(addr)		heap[addr].flag == FRE


//------register----
int E; //environment pointer
int H; //heap pointer 
int S; //stack pointer
int F; //free count
int C; //consume of S
int A; //arglist pointer
int P; //address on CTRL+C event

//-------read--------
#define EOL		'\n'
#define TAB		'\t'
#define SPACE	' '
#define ESCAPE	033
#define NUL		'\0'

//-------error code---
#define CANT_FIND_ERR	1
#define ARG_SYM_ERR		2
#define ARG_NUM_ERR		3
#define ARG_LIS_ERR		4
#define ARG_LEN0_ERR	5
#define ARG_LEN1_ERR	6
#define ARG_LEN2_ERR	7
#define ARG_LEN3_ERR	8
#define MALFORM_ERR		9
#define CANT_READ_ERR	10

//-------arg check code--
#define NUMLIST_TEST	1
#define SYMBOL_TEST		2
#define NUMBER_TEST		3
#define LIST_TEST		4
#define LEN0_TEST		5
#define LEN1_TEST		6
#define LEN2_TEST		7
#define LEN3_TEST		8
#define LENS1_TEST		9
#define LENS2_TEST		10
#define COND_TEST		11	


BOOL WINAPI CtrlHandler( DWORD CtrlEvent );
void initcell(void);
int freshcell(void);
void bindsym(int sym, int val);
void assocsym(int sym, int val);
int findsym(int sym);
void cellprint(int addr);
void heapdump(int start, int end);
void stackdump(int start, int end);
void argstkdump(int start, int end);
void markoblist(void);
void markcell(int addr);
void gbcmark(void);
void gbcsweep(void);
void clrcell(int addr);
void gbc(void);
void checkgbc(void);
int car(int addr);
int cdr(int addr);
int cons(int car, int cdr);
int caar(int addr);
int cdar(int addr);
int cadr(int addr);
int caddr(int addr);
int assoc(int sym, int lis);
int length(int addr);
int list(int addr);
int makenum(int num);
int makesym(char *name);
int makeNIL(void);
int makeT(void);
void gettoken(void);
int numbertoken(char buf[]);
int symboltoken(char buf[]);
int issymch(char c);
int read(void);
int readlist(void);
void print(int addr);
void printlist(int addr);
int eval(int addr);
void bindarg(int lambda, int arglist);
void unbind(void);
int atomp(int addr);
int numberp(int addr);
int symbolp(int addr);
int listp(int addr);
int nullp(int addr);
int symnamep(int addr, char *name);
int evlisp(int addr);
int apply(int func, int arg);
int subrp(int addr);
int fsubrp(int addr);
void initsubr(void);
void bindfunc(char *name, tag tag, int func);
void push(int reg);
int pop(void);
void argpush(int addr);
void argpop(void);
void error(int errnum, char *fun, int arg);
void checkarg(int test, char *fun, int arg);
int isnumlis(int arg);

//---subr-------
int f_plus(int addr);
int f_minus(int addr);
int	f_mult(int addr);
int f_div(int addr);
int f_evenp(int addr);
int f_oddp(int addr);
int f_exit(int addr);
int f_stackdump(int addr);
int f_heapdump(int addr);
int f_argstkdump(int addr);
int f_car(int addr);
int f_cdr(int addr);
int f_cons(int addr);
int f_length(int addr);
int f_list(int addr);
int f_rplaca(int arglist);
int f_rplacd(int arglist);
int f_nullp(int addr);
int f_atomp(int addr);
int f_eq(int addr);
int f_setq(int addr);
int f_oblist(int addr);
int f_defun(int addr);
int f_if(int addr);
int f_cond(int addr);
int f_numeqp(int addr);
int f_numberp(int addr);
int f_symbolp(int addr);
int f_listp(int addr);
int f_greater(int addr);
int f_eqgreater(int addr);
int f_smaller(int addr);
int f_eqsmaller(int addr);
int f_gbc(int addr);
int f_register(int addr);
int f_reset(int addr);
int f_eval(int addr);
int f_apply(int addr);
int f_read(int addr);
int f_print(int addr);
int f_princ(int addr);
int f_begin(int addr);
int f_addr(int addr);
int f_resest(int addr);
int f_regsetq(int arglist);



