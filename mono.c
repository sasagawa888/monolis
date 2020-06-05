/* Mono (minimam Lisp interpreter)
written by kenichi sasagawa 2011/12

*/ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
#include <windows.h>
#include <signal.h>
#include "mono.h"

cell heap[HEAPSIZE];
int stack[STACKSIZE];
int argstk[STACKSIZE];
token stok = {GO,OTHER};
jmp_buf buf;
int exit_flag=0;


void main(void){
	printf("educational Lisp system Mono Ver0.24 (written by sasagawa888) \n");
    initcell();
    initsubr();
    SetConsoleCtrlHandler(NULL, FALSE ); //CTRL+Cを有効にする。
    SetConsoleCtrlHandler(CtrlHandler, TRUE );
    int ret = setjmp(buf);
    
    repl:
	
    if(ret == 0)
    	while(1){
			printf("Mono> "); fflush(stdout); fflush(stdin);
            print(eval(read ()));
            printf("\n"); fflush(stdout);
        }
    else
    	if(ret == 1){
        	ret = 0;
            goto repl;
        }
        else
    		return;
}

//-----ctrl+C,D---------------
BOOL WINAPI CtrlHandler( DWORD CtrlEvent )
{
	switch( CtrlEvent ) {
    	case CTRL_C_EVENT:
           {fprintf(stdout, "catch CTRL_C_EVENT\n" );
        	fflush(stdout);
        	exit_flag = 1;
            return TRUE;
            }
        default:
        	return FALSE;
        }
}
//-----------------------------
void initcell(void){
	int addr,addr1;
    
    for(addr=0; addr <= HEAPSIZE; addr++){
    	heap[addr].flag = FRE;
        heap[addr].cdr = addr+1;
    }
    H = 0;
    F = HEAPSIZE;
    
    //0番地はnil、環境レジスタを設定する。初期環境
    E = makeNIL();
    assocsym(makeNIL(),makeNIL());
    assocsym(makeT(),makeT());
    
    S = 0;
    A = 0;
}

int freshcell(void){
	int res;
    
    res = H;
    H = heap[H].cdr;
    SET_CDR(res,0);
	F--;
    return(res);
}

//deep-bindによる。シンボルが見つからなかったら登録。
//見つかったらそこに値をいれておく。
void bindsym(int sym, int val){
	int addr;
    
    addr= assoc(sym,E);
    if(addr == 0)
    	assocsym(sym,val);
    else
    	SET_CDR(addr,val);	
}

//ローカル変数の束縛
//ローカル変数の場合は以前の束縛に積み上げていく感じ。
//同じ変数名があったとしても書き変えない。
void assocsym(int sym, int val){
	E = cons(cons(sym,val),E);
}

    	
    
//環境はリストになっていて次のよう。
// env = ((sym1 . val1) (sym2 . val2) ...)
// assocでシンボルに対応する値を探す。
//見つからなかった場合には0
int findsym(int sym){
	int addr;
    
    addr = assoc(sym,E);
    
    if(addr == 0)
    	return(0);
    else
    	return(cdr(addr));
}

//-------デバッグ用------------------    
void cellprint(int addr){
	switch(GET_FLAG(addr)){
    	case FRE:	printf("FRE "); break;
        case USE:	printf("USE "); break;
    }
	switch(GET_TAG(addr)){
    	case EMP:	printf("EMP "); break;
        case NUM:	printf("NUM "); break;
        case SYM:	printf("SYM "); break;
        case LIS:	printf("LIS "); break;
        case SUBR:	printf("SUBR   "); break;
        case FSUBR:	printf("FSUBR  "); break;
        case FUNC:printf("FUNC "); break;
    }
    printf("car=%d ", GET_CAR(addr));
    printf("cdr=%d ", GET_CDR(addr));
    printf("bind=%d ", GET_BIND(addr));
    printf("name=%s \n", GET_NAME(addr));
}   

//ヒープダンプ	
void heapdump(int start, int end){
	int i;
    
    for(i=start; i<= end; i++){
    	printf("%d ", i);
    	cellprint(i);
    }
}

//スタックダンプ
void stackdump(int start, int end){
	int i;
    
    for(i=start; i<= end; i++){
    	printf("addr = %d  ",i);
        printf("store = %d  \n",stack[i]);
    }
}

//arglistダンプ
void argstkdump(int start, int end){
	int i;
    
    for(i=start; i<= end; i++){
    	printf("addr = %d  ",i);
        printf("store = %d  \n",argstk[i]);
    }
}




//---------ガベージコレクション-----------
void gbc(void){
	int addr;
	
    printf("enter GBC f=%d\n", F); fflush(stdout);
	gbcmark();
    
    //printf("only mark\n"); return;
    
    gbcsweep();
    F = 0;
    for(addr=0; addr <= HEAPSIZE; addr++)
    	if(IS_EMPTY(addr))
        	F++;
	printf("exit GBC f=%d\n", F); fflush(stdout);
}


void markoblist(void){
	int addr;
    
    addr = E;
    while(!(nullp(addr))){
    	MARK_CELL(addr);
        addr = cdr(addr);
    }
    MARK_CELL(0);
}

void markcell(int addr){
	if(USED_CELL(addr))
    	return;
 
	MARK_CELL(addr); 
    if(car(addr) != 0)
    	markcell(car(addr));

	if(cdr(addr) != 0)
    	markcell(cdr(addr));
    
    if((GET_BIND(addr) != 0) && (IS_FUNC(addr)))
    	markcell(GET_BIND(addr));
     
}

void gbcmark(void){
	int addr, i;
    
    //oblistをマークする。
	markoblist();
    //oblistからつながっているcellをマークする。
    addr = E;
    while(!(nullp(addr))){
    	markcell(car(addr));
        addr = cdr(addr);
    }
    //argstackからbindされているcellをマークする。
    for(i = 0; i < A; i++)
    	markcell(argstk[i]);
    
}

void gbcsweep(void){
	int addr,maxaddr;
    
    addr = 0; 
    while(addr < HEAPSIZE){
    	if(USED_CELL(addr))
        	NOMARK_CELL(addr);
        else{
        	clrcell(addr);
            SET_CDR(addr,H);
            H = addr;
        }
        addr++;
    }
}

void clrcell(int addr){
	SET_TAG(addr,EMP);
    free(heap[addr].name);
    heap[addr].name = NULL;
    SET_CAR(addr,0);
    SET_CDR(addr,0);
    SET_BIND(addr,0);
}

//自由セルが一定数を下回った場合にはgbcを起動する。
void checkgbc(void){
	if(F < FREESIZE)
    	gbc();
}
   
//--------------リスト操作---------------------

int car(int addr){
	return(GET_CAR(addr));
}

int caar(int addr){
	return(car(car(addr)));
}

int cdar(int addr){
	return(cdr(car(addr)));
}

int cdr(int addr){
	return(GET_CDR(addr));
}

int cadr(int addr){
	return(car(cdr(addr)));
}

int caddr(int addr){
	return(car(cdr(cdr(addr))));
}


int cons(int car, int cdr){
	int addr;
    
    addr = freshcell();
   	SET_TAG(addr,LIS);
    SET_CAR(addr,car);
    SET_CDR(addr,cdr);
    return(addr);
}

int assoc(int sym, int lis){
	if(nullp(lis))
    	return(0);
    else
    if(eqp(sym, caar(lis)))
    	return(car(lis));
    else
    	assoc(sym,cdr(lis));
}

int length(int addr){
	int len = 0;
    
    while(!(nullp(addr))){
    	len++;
        addr = cdr(addr);
    }
    return(len);
} 

int list(int arglist){
	if(nullp(arglist))
    	return(makeNIL());
    else
    	return(cons(car(arglist),list(cdr(arglist))));    
}

//----------------------------------------
        			
int makenum(int num){
	int addr;
    
    addr = freshcell();
    SET_TAG(addr,NUM);
    SET_NUMBER(addr,num);
    return(addr);
}

int makesym(char *name){
	int addr;
    
    addr = freshcell();
    SET_TAG(addr,SYM);
    SET_NAME(addr,name);
    return(addr);
}


//空リストを作る。シンボルnilを空リストとも解釈している。
int makeNIL(void){
	int addr;
    
    addr = freshcell();
    SET_TAG(addr,SYM);
    SET_NAME(addr,"nil");
    return(addr);
}
//シンボルＴを返す。
int makeT(void){
	int addr;
    
    addr = freshcell();
    SET_TAG(addr,SYM);
    SET_NAME(addr,"t");
    return(addr);
}

//スタック。Ｅレジスタの保存用
void push(int reg){
	stack[S++] = reg;
    if(S > C) 
    	C = S;
}
int pop(void){
	return(stack[--S]);
}

//arglistスタックのpush/pop
void argpush(int addr){
	argstk[A++] = addr;
}

void argpop(void){
	--A;
}

//-------read()--------

void gettoken(void){
	char c;
    int pos;
    
    if(stok.flag == BACK){
    	stok.flag = GO;
        return;
    }
    
    if(stok.ch == ')'){
    	stok.type = RPAREN;
        stok.ch = NUL;
        return;
    }
    
    if(stok.ch == '('){
    	stok.type = LPAREN;
        stok.ch = NUL;
        return;
    }
    
    c = getchar();
    while((c == SPACE) || (c == EOL) || (c == TAB))
    	c=getchar();
    
    switch(c){
    	case '(':	stok.type = LPAREN; break;
        case ')':	stok.type = RPAREN; break;
        case '\'':	stok.type = QUOTE; break;
        case '.':	stok.type = DOT; break;
        default: {
        	pos = 0; stok.buf[pos++] = c;
        	while(((c=getchar()) != EOL) && (pos < BUFSIZE) && 
            		(c != SPACE) && (c != '(') && (c != ')'))
            	stok.buf[pos++] = c;
                
            stok.buf[pos] = NUL;
            stok.ch = c;
            if(numbertoken(stok.buf)){
            	stok.type = NUMBER;
                break;
            }
            if(symboltoken(stok.buf)){
            	stok.type = SYMBOL;
                break;
            }
            stok.type = OTHER;  
        }
    }
}

int numbertoken(char buf[]){
	int i;
    char c;
    
    if(((buf[0] == '+') || (buf[0] == '-'))){
		if(buf[1] == NUL)
        	return(0); // case {+,-} => symbol
    	i = 1;
    	while((c=buf[i]) != NUL)
        	if(isdigit(c))
            	i++;  // case {+123..., -123...}
            else
            	return(0); 
    }
    else {       
    	i = 0;    // {1234...}
    	while((c=buf[i]) != NUL)
    		if(isdigit(c))
        		i++;
        	else 
        		return(0);
    }
    return(1);
}

int symboltoken(char buf[]){
	int i;
    char c;
    
    if(isdigit(buf[0]))
    	return(0);
    
    i = 0;
    while((c=buf[i]) != NUL)
    	if((isalpha(c)) || (isdigit(c)) || (issymch(c)))
        	i++;
        else 
        	return(0);
    
    return(1);
}

int issymch(char c){
	switch(c){
    	case '!':
        case '?':
        case '+':
        case '-':
        case '*':
        case '/': return(1);
        defalut:  return(0);
    }
}  
        

int read(void){
    gettoken();
    switch(stok.type){
    	case NUMBER:	return(makenum(atoi(stok.buf)));
        case SYMBOL:	return(makesym(stok.buf));
        case QUOTE:		return(cons(makesym("quote"), cons(read(),makeNIL())));
        case LPAREN:	return(readlist());
    }
    error(CANT_READ_ERR,"read",NIL);
}

int readlist(void){
    int car,cdr;
    
    gettoken();      
    if(stok.type == RPAREN) 
    	return(makeNIL());
    else
    if(stok.type == DOT){
    	cdr = read();
        if(atomp(cdr))
        	gettoken();
    	return(cdr);
    }
    else{
    	stok.flag = BACK;
    	car = read();
        cdr = readlist();
        return(cons(car,cdr));
    }
}

//-----print------------------
void print(int addr){    
	switch(GET_TAG(addr)){
    	case NUM:	printf("%d", GET_NUMBER(addr)); break;
        case SYM:	printf("%s", GET_NAME(addr)); break;
        case SUBR:	printf("<subr>"); break;
        case FSUBR:	printf("<fsubr>"); break;
        case LIS: {	printf("(");
    				printlist(addr); break;}
    }
}	
	

void printlist(int addr){
	if(IS_NIL(addr))
    	printf(")");
    else
    if((!(listp(cdr(addr)))) && (! (nullp(cdr(addr))))){
    	print(car(addr));
        printf(" . ");
        print(cdr(addr));
        printf(")");
    }
    else {
    	print(GET_CAR(addr));    
        if(! (IS_NIL(GET_CDR(addr))))
        	printf(" ");
       	printlist(GET_CDR(addr));
    }
}

//--------eval---------------        
int eval(int addr){
	int res;
    
    //ctrl+cによる割り込みがあった場合
    if(exit_flag == 1){
    	exit_flag = 0;
        P = addr; //後で調べられるように退避
        printf("exit eval by CTRL_C_EVENT\n"); fflush(stdout);
        longjmp(buf,1);
    }
    
    if(atomp(addr)){
		if(numberp(addr))
    		return(addr);
    	if(symbolp(addr)){
    		res = findsym(addr);
            if(res == 0)
            	error(CANT_FIND_ERR, "eval", addr);
            else
            	switch(GET_TAG(res)){
                	case NUM:	return(res);
                	case SYM:	return(res);
                    case LIS:	return(res);
                	case SUBR:	return(res);
                    case FSUBR:	return(res);
                    case FUNC:return(GET_BIND(res));
                }	
        }
    }
    else 
    if(listp(addr)){
    	if((symbolp(car(addr))) &&(HAS_NAME(car(addr),"quote")))
    		return(cadr(addr));
        if(numberp(car(addr)))
        	error(ARG_SYM_ERR, "eval", addr);
    	if(subrp(car(addr)))
    		return(apply(car(addr),evlis(cdr(addr))));
    	if(fsubrp(car(addr)))
            return(apply(car(addr),cdr(addr)));
    	if(lambdap(car(addr)))
    		return(apply(car(addr),evlis(cdr(addr))));	  
    }
    error(CANT_FIND_ERR, "eval", addr);
}

int apply(int func, int args){
	int symaddr,lamlis,body,res;
      
    symaddr = findsym(func);
    if(symaddr == 0)
      	error(CANT_FIND_ERR, "apply", func);
    else {
    	switch(GET_TAG(symaddr)){
        	case SUBR: 	return((GET_SUBR(symaddr))(args));
            case FSUBR:	return((GET_SUBR(symaddr))(args)); 			
            case FUNC: {	lamlis = car(GET_BIND(symaddr));
            				body = cdr(GET_BIND(symaddr));
                            bindarg(lamlis,args);
                            while(!(IS_NIL(body))){
                            	res = eval(car(body));
                                body = cdr(body);
                            }
                            unbind();
                            return(res); }      
        }
    }
}

void bindarg(int lambda, int arglist){
	int arg1,arg2;

	push(E);
    while(!(IS_NIL(lambda))){
    	arg1 = car(lambda);
        arg2 = car(arglist);
        assocsym(arg1,arg2);
        lambda = cdr(lambda);
        arglist = cdr(arglist);
    }
}

void unbind(void){
	E = pop();
}


int evlis(int addr){
	int car_addr,cdr_addr;
    
    argpush(addr);
    checkgbc();
    if(IS_NIL(addr)){
    	argpop();
        return(addr);
    }
	else{
    	car_addr = eval(car(addr));
        argpush(car_addr);
        cdr_addr = evlis(cdr(addr));
        argpop();
        argpop();
        return(cons(car_addr,cdr_addr));
    }
}	


int atomp(int addr){
    if((IS_NUMBER(addr)) || (IS_SYMBOL(addr)))
    	return(1);
    else
    	return(0);
}

int numberp(int addr){	
    if(IS_NUMBER(addr))
    	return(1);
    else
    	return(0);
}

int symbolp(int addr){	
    if(IS_SYMBOL(addr))
    	return(1);
    else
    	return(0);
}

int listp(int addr){	
    if(IS_LIST(addr))
    	return(1);
    else
    	return(0);
}

int nullp(int addr){
	if(IS_NIL(addr))
    	return(1);
    else
    	return(0);
}


int eqp(int addr1, int addr2){
	if((numberp(addr1)) && (numberp(addr2))
    	&& ((GET_NUMBER(addr1)) == (GET_NUMBER(addr2))))
        return(1);
    else if ((symbolp(addr1)) && (symbolp(addr2))
    	&& (SAME_NAME(addr1,addr2)))
        return(1);
    else
    	return(0);
}



int subrp(int addr){
	int val;
    
    val = findsym(addr);
    if(val != 0)
		return(IS_SUBR(val));
    else
    	return(0);
}

int fsubrp(int addr){
	int val;
    
    val = findsym(addr);
    if(val != 0)
		return(IS_FSUBR(val));
    else
    	return(0);
}

int lambdap(int addr){
	int val;
    
    val = findsym(addr);
    if(val != 0)
		return(IS_FUNC(val));
    else
    	return(0);
}

//-------エラー処理------
void error(int errnum, char *fun, int arg){
	switch(errnum){
    	case CANT_FIND_ERR:{printf("%s can't find difinition of ", fun);
        					print(arg); break; }
        
        case CANT_READ_ERR:{printf("%s can't find difinition of ", fun);
        					break; }
                            
        case ARG_SYM_ERR:  {printf("%s require symbol but got ", fun);
        					print(arg); break; }
                            
        case ARG_NUM_ERR:  {printf("%s require number but got ", fun);
        					print(arg); break; }
                            
        case ARG_LIS_ERR:  {printf("%s require list but got ", fun);
        					print(arg); break; }
        
        case ARG_LEN0_ERR: {printf("%s require 0 arg but got %d", fun, length(arg));
                           	break; }
                            
        case ARG_LEN1_ERR: {printf("%s require 1 arg but got %d", fun, length(arg));
                           	break; }
        
        case ARG_LEN2_ERR: {printf("%s require 2 args but got %d", fun, length(arg));
                           	break; }
        
        case ARG_LEN3_ERR: {printf("%s require 3 args but got %d", fun, length(arg));
                           	break; }
                              
        case MALFORM_ERR:  {printf("%s got malformed args " ,fun);
        					print(arg); break; }
    }
    printf("\n");
    longjmp(buf,1);
}

void checkarg(int test, char *fun, int arg){
	switch(test){
    	case NUMLIST_TEST:	if(isnumlis(arg)) return; else error(ARG_NUM_ERR, fun, arg);
        case SYMBOL_TEST:	if(symbolp(arg)) return; else error(ARG_SYM_ERR, fun, arg);
		case NUMBER_TEST:	if(numberp(arg)) return; else error(ARG_NUM_ERR, fun, arg);
        case LIST_TEST:		if(listp(arg)) return; else  error(ARG_LIS_ERR, fun, arg);
        case LEN0_TEST:		if(length(arg) == 0) return; else error(ARG_LEN0_ERR, fun, arg);
        case LEN1_TEST:		if(length(arg) == 1) return; else error(ARG_LEN1_ERR, fun, arg);
        case LEN2_TEST:		if(length(arg) == 2) return; else error(ARG_LEN2_ERR, fun, arg);
        case LEN3_TEST:		if(length(arg) == 3) return; else error(ARG_LEN3_ERR, fun, arg);
    }
}

int isnumlis(int arg){
	while(!(IS_NIL(arg)))
    	if(numberp(car(arg)))
        	arg = cdr(arg);
        else
        	return(0);
    return(1);
}
    	


//--------組込み関数
//subrを環境に登録する。
void defsubr(char *symname, int func){
	bindfunc(symname, SUBR, func);
}
//fsubr(引数を評価しない組込関数）の登録。
void deffsubr(char *symname, int func){
	bindfunc(symname, FSUBR, func);
}

void bindfunc(char *name, tag tag, int func){
	int sym,val;
	
    sym = makesym(name);
	val = freshcell();
    SET_TAG(val,tag);
    switch(tag){
    	case SUBR:
        case FSUBR:		SET_SUBR(val,func); break;
        case FUNC:	SET_BIND(val,func); break;
    }
    SET_CDR(val,0);
    bindsym(sym,val);
}

void initsubr(void){
	defsubr("+",(int)f_plus);
    defsubr("-",(int)f_minus);
    defsubr("*",(int)f_mult);
    defsubr("/",(int)f_div);
    defsubr("evenp",(int)f_evenp);
    defsubr("oddp",(int)f_oddp);
    defsubr("exit",(int)f_exit);
    defsubr("hdmp",(int)f_heapdump);
    defsubr("sdmp",(int)f_stackdump);
    defsubr("admp",(int)f_argstkdump);
    defsubr("car",(int)f_car);
    defsubr("cdr",(int)f_cdr);
    defsubr("cons",(int)f_cons);
    defsubr("list",(int)f_list);
    defsubr("rplaca",(int)f_rplaca);
    defsubr("rplacd",(int)f_rplacd);
    defsubr("eq",(int)f_eq);
    defsubr("null",(int)f_nullp);
    defsubr("atom",(int)f_atomp);
    defsubr("oblist",(int)f_oblist);
    defsubr("gbc",(int)f_gbc);
    defsubr("reg",(int)f_register);
    defsubr("reset",(int)f_reset);
    defsubr("read",(int)f_read);
    defsubr("eval",(int)f_eval);
    defsubr("apply",(int)f_apply);
    defsubr("print",(int)f_print);
    defsubr("princ",(int)f_princ);
    defsubr("=",(int)f_numeqp);
    defsubr(">",(int)f_greater);
    defsubr(">=",(int)f_eqgreater);
    defsubr("<",(int)f_smaller);
    defsubr("<=",(int)f_eqsmaller);
    defsubr("length",(int)f_length);
    defsubr("numberp",(int)f_numberp);
    defsubr("symbolp",(int)f_symbolp);
    defsubr("listp",(int)f_listp);
    defsubr("addr",(int)f_addr);
    
    deffsubr("setq",(int)f_setq);
    deffsubr("defun",(int)f_defun);
    deffsubr("if",(int)f_if);
    deffsubr("begin",(int)f_begin);
    deffsubr("cond",(int)f_cond);
    deffsubr("regsetq",(int)f_regsetq);
}

//--subr----

int f_plus(int arglist){
	int arg,res;
    
    checkarg(NUMLIST_TEST, "+", arglist);
	res = 0;
    while(!(IS_NIL(arglist))){
    	arg = GET_NUMBER(car(arglist));
        arglist = cdr(arglist);
    	res = res + arg;
    }
    return(makenum(res));
}

int f_minus(int arglist){
	int arg,res;
    
    checkarg(NUMLIST_TEST, "-", arglist);
    res = GET_NUMBER(car(arglist));
    arglist = cdr(arglist);
    while(!(IS_NIL(arglist))){
    	arg = GET_NUMBER(car(arglist));
        arglist = cdr(arglist);
    	res = res - arg;
    }
    return(makenum(res));
}

int f_mult(int arglist){
	int arg,res;
    
    checkarg(NUMLIST_TEST, "*", arglist);
    res = GET_NUMBER(car(arglist));
    arglist = cdr(arglist);
    while(!(IS_NIL(arglist))){
    	arg = GET_NUMBER(car(arglist));
        arglist = cdr(arglist);
    	res = res * arg;
    }
    return(makenum(res));
}

int f_div(int arglist){
	int arg,res;
    
	checkarg(NUMLIST_TEST, "/", arglist);
    res = GET_NUMBER(car(arglist));
    arglist = cdr(arglist);
    while(!(IS_NIL(arglist))){
    	arg = GET_NUMBER(car(arglist));
        arglist = cdr(arglist);
    	res = res / arg;
    }
    return(makenum(res));
}  

int f_evenp(int arglist){
	int arg;
    
    checkarg(LEN1_TEST, "evenp", arglist);
    checkarg(NUMBER_TEST, "evenp", car(arglist));
    arg = GET_NUMBER(car(arglist));
    if((arg % 2) == 0)
    	return(T);
    else
    	return(NIL);
}

int f_oddp(int arglist){
	int arg;
    
    checkarg(LEN1_TEST, "oddp", arglist);
    checkarg(NUMBER_TEST, "oddp", car(arglist));
    arg = GET_NUMBER(car(arglist));
    if((arg % 2) == 1)
    	return(T);
    else
    	return(NIL);
}

int f_exit(int arglist){
	int addr;
    
    checkarg(LEN0_TEST, "exit", arglist);
    for(addr=0; addr<= HEAPSIZE; addr++)
    	free(heap[addr].name);
	
	printf("- Long live Lisp. Thank you very much Dr.McCarthy. -\n");
    longjmp(buf,2); 
}

int f_heapdump(int arglist){
	int arg1;
    
    checkarg(LEN1_TEST, "hdmp", arglist);
    arg1 = GET_NUMBER(car(arglist));
	heapdump(arg1,arg1+10);
    return(makeT());
}

int f_stackdump(int arglist){
	int arg1;
    
    checkarg(LEN1_TEST, "hdmp", arglist);
    arg1 = GET_NUMBER(car(arglist));
	stackdump(arg1,arg1+10);
    return(makeT());
}

int f_argstkdump(int arglist){
	int arg1;
    
    checkarg(LEN1_TEST, "hdmp", arglist);
    arg1 = GET_NUMBER(car(arglist));
	argstkdump(arg1,arg1+10);
    return(makeT());
}


int f_register(int arglist){
	checkarg(LEN0_TEST, "reg", arglist);
	printf("H(heap)         = %d\n", H);
    printf("F(free)         = %d\n", F);
    printf("E(environment)  = %d\n", E);
    printf("S(stack)        = %d\n", S);
    printf("C(consume of S) = %d\n", C);
    printf("A(arg-stack)    = %d\n", A);
    printf("P(on ctrl+c)    = %d\n", P);
    return(makeT());
}

int f_reset(int arglist){
	E = stack[0];
    S = 0;
    C = 0;
    A = 0;
    P = 0;
    return(makeT());
}


int f_car(int arglist){
	int arg1;
    
    checkarg(LEN1_TEST, "car", arglist);
    arg1 = car(arglist);
	return(car(arg1));
}

int f_cdr(int arglist){
	int arg1;
    
    checkarg(LEN1_TEST, "cdr", arglist);
    arg1 = car(arglist);
	return(cdr(arg1));
}

int f_cons(int arglist){
	int arg1,arg2;
    
    checkarg(LEN2_TEST, "cons", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
	return(cons(arg1,arg2));
}

int f_eq(int arglist){
	int arg1,arg2;
    
    checkarg(LEN2_TEST, "eq" ,arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if(eqp(arg1,arg2))
    	return(T);
    else
    	return(NIL);
}

int f_nullp(int arglist){
	int arg;
    
    checkarg(LEN1_TEST, "null" ,arglist);
    arg = car(arglist);
    if(nullp(arg))
    	return(T);
    else
    	return(NIL);
}
         
int f_atomp(int arglist){
	int arg;
    
    checkarg(LEN1_TEST, "atom" ,arglist);
    arg = car(arglist);
    if(atomp(arg))
    	return(T);
    else
    	return(NIL);
}

int f_length(int arglist){
	checkarg(LEN1_TEST, "length", arglist);
	checkarg(LIST_TEST, "length", car(arglist));
	return(makenum(length(car(arglist))));
}

int f_list(int arglist){
	return(list(arglist));   
}

int f_rplaca(int arglist){
	int arg1,arg2;
    
    checkarg(LEN2_TEST, "rplaca", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    checkarg(LIST_TEST, "rplaca", arg1);
    SET_CAR(arg1,arg2);
    return(arg1);
}

int f_rplacd(int arglist){
	int arg1,arg2;
    
    checkarg(LEN2_TEST, "rplacd", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    checkarg(LIST_TEST, "rplacd", arg1);
    SET_CDR(arg1,arg2);
    return(arg1);
}



int f_numeqp(int arglist){
	int num1,num2;
    
    checkarg(LEN2_TEST, "=", arglist);
    checkarg(NUMLIST_TEST, "=", arglist);
    num1 = GET_NUMBER(car(arglist));
    num2 = GET_NUMBER(cadr(arglist));
    
    if(num1 == num2)
    	return(makeT());
    else
    	return(makeNIL());
}

int f_symbolp(int arglist){
	if(symbolp(car(arglist)))
    	return(makeT());
    else
    	return(makeNIL());
}

int f_numberp(int arglist){
	if(numberp(car(arglist)))
    	return(makeT());
    else
    	return(makeNIL());
}

int f_listp(int arglist){
	if(listp(car(arglist)))
    	return(makeT());
    else
    	return(makeNIL());
}

int f_smaller(int arglist){
	int num1,num2;
    
    checkarg(LEN2_TEST, "<", arglist);
    checkarg(NUMLIST_TEST, "<", arglist);
    num1 = GET_NUMBER(car(arglist));
    num2 = GET_NUMBER(cadr(arglist));
    
    if(num1 < num2)
    	return(makeT());
    else
    	return(makeNIL());
}

int f_eqsmaller(int arglist){
	int num1,num2;
    
    checkarg(LEN2_TEST, "<=", arglist);
    checkarg(NUMLIST_TEST, "<=", arglist);
    num1 = GET_NUMBER(car(arglist));
    num2 = GET_NUMBER(cadr(arglist));
    
    if(num1 <= num2)
    	return(makeT());
    else
    	return(makeNIL());
}

int f_greater(int arglist){
	int num1,num2;
    
    checkarg(LEN2_TEST, ">", arglist);
    checkarg(NUMLIST_TEST, ">", arglist);
    num1 = GET_NUMBER(car(arglist));
    num2 = GET_NUMBER(cadr(arglist));
    
    if(num1 > num2)
    	return(makeT());
    else
    	return(makeNIL());
}


int f_eqgreater(int arglist){
	int num1,num2;
    
    checkarg(LEN2_TEST, ">=", arglist);
    checkarg(NUMLIST_TEST, ">=", arglist);
    num1 = GET_NUMBER(car(arglist));
    num2 = GET_NUMBER(cadr(arglist));
    
    if(num1 >= num2)
    	return(makeT());
    else
    	return(makeNIL());
}    

int f_oblist(int arglist){
	int addr,addr1,res;
    
    checkarg(LEN0_TEST, "oblist", arglist);
    res = makeNIL();
    addr = E;
    while(!(nullp(addr))){
    	addr1 = caar(addr);
    	res = cons(addr1,res);
        addr = cdr(addr);
    }
	return(res);
}

int f_gbc(int arglist){
	gbc();
    return(makeT());
}
    	
int f_read(int arglist){
	checkarg(LEN0_TEST, "read", arglist);
	return(read());
}

int f_print(int arglist){
	checkarg(LEN1_TEST, "print", arglist);
	print(car(arglist));
    printf("\n");
    return(makeT());
}

int f_princ(int arglist){
	checkarg(LEN1_TEST, "princ", arglist);
	print(car(arglist));
    return(makeT());
}


int f_eval(int arglist){
	checkarg(LEN1_TEST, "eval", arglist);
	return(eval(car(arglist)));
}

int f_apply(int arglist){
	checkarg(LEN2_TEST, "apply", arglist);
    checkarg(SYMBOL_TEST, "apply", car(arglist));
    checkarg(LIST_TEST, "apply", cadr(arglist));
	int arg1,arg2;
    
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return(apply(arg1,arg2));
}
         
int f_addr(int arglist){
	checkarg(LEN1_TEST, "addr", arglist);
    checkarg(NUMLIST_TEST, "addr", arglist);
    return(GET_NUMBER(car(arglist)));
}
         
//--FSUBR-----------
int f_setq(int arglist){
	int arg1,arg2;
 	
    checkarg(LEN2_TEST, "setq", arglist);
    checkarg(SYMBOL_TEST, "setq", car(arglist));
    arg1 = car(arglist);
    arg2 = eval(cadr(arglist));
    bindsym(arg1,arg2);
    return(makeT());   
}

int f_regsetq(int arglist){
    int arg1,arg2;
    
    checkarg(LEN2_TEST, "regsetq", arglist);
    checkarg(SYMBOL_TEST, "regsetq", car(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if(HAS_NAME(arg1,"H"))
    	H = GET_NUMBER(arg2);
    else 
    if(HAS_NAME(arg1,"E"))
    	E = GET_NUMBER(arg2);
    else
    if(HAS_NAME(arg1,"F"))
    	F = GET_NUMBER(arg2);
    else
    if(HAS_NAME(arg1,"S"))
        S = GET_NUMBER(arg2);
    else
    if(HAS_NAME(arg1,"C"))
        C = GET_NUMBER(arg2);
    else
    if(HAS_NAME(arg1,"A"))
        A = GET_NUMBER(arg2);
    else
    if(HAS_NAME(arg1,"P"))
        P = GET_NUMBER(arg2);
    
    return(makeT());
}
    

int f_defun(int arglist){
	int arg1,arg2;
    
    checkarg(LEN3_TEST, "defun", arglist);
    checkarg(SYMBOL_TEST, "defun", car(arglist));
    checkarg(LIST_TEST, "defun", cadr(arglist));
    checkarg(LIST_TEST, "defun" ,caddr(arglist));
    arg1 = car(arglist);
    arg2 = cdr(arglist);
    bindfunc(GET_NAME(arg1),FUNC,arg2);
    return(makeT());
}



int f_if(int arglist){
	int arg1,arg2,arg3;
    
    checkarg(LEN3_TEST, "if", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = car(cdr(cdr(arglist)));
    
    if(! (nullp(eval(arg1))))
    	return(eval(arg2));
    else
    	return(eval(arg3));
}

int f_cond(int arglist){
	int arg1,arg2,arg3;
    
    if(nullp(arglist))
    	return(makeNIL());
    
    arg1 = car(arglist);
    checkarg(LIST_TEST, "cond", arg1);
    arg2 = car(arg1);
    arg3 = cdr(arg1);
    
    if(! (nullp(eval(arg2))))
    	return(f_begin(arg3));
    else
		return(f_cond(cdr(arglist)));
}

int f_begin(int arglist){
	int res;
    
    while(!(nullp(arglist))){
    	res = eval(car(arglist));
        arglist = cdr(arglist);
    }
	return(res);
}
   	

//-----------------------------
	  

