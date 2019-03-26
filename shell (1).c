#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
 
//Declaracion variables globales
static char* args[512];//arreglo de comandos
static int n = 0; 
static int error=0;
int espera = 1; // determina si un proceso debe correr en background
int output=0;
int input=0;
int bg=0;
int contBG=0;
int nVar=0;
int contVar=0;
char* nombreArchivo;

#define WRITE 1
#define READ  0

//Cuenta el # de pipes en una linea de comando
int contarComandos(char *auxiliar2){
	int i,cont=0;
	for(i=0;i<strlen(auxiliar2);i++){
		if(auxiliar2[i]=='|'){
			cont++;		
		}
	}
	return cont;
}
//Recorta la linea que ingresa el usuario para obtener unicamente el comando requerido
char * arreglarComandoDoblesCompuesto(char comando[100]){
	char* sig;
	int i=0,j=0;
	char* arreglo = malloc(100);
	sig=strtok(comando, " ");
	while(sig!=NULL){
		sig=strtok(NULL, " ");
		if(strstr(sig,"INPUTFILE=")!=NULL){
			sig=strstr(sig,"=")+1;
			strcat(arreglo," ");
			strcat(arreglo,sig);
			break;
		}
		else if(strstr(sig,"REDIR")==NULL){
			if(i==0){
				strcpy(arreglo,sig);
			}else{
				strcat(arreglo," ");
				strcat(arreglo,sig);
			}
			i++;
		}
	}
	return arreglo;
}


//Recorta la linea que ingresa el usuario para obtener unicamente el comando requerido
char * arreglarComandoDobleSimple(char comando[100]){
	char* sig;
	int i=0,j=0;
	char* arreglo = malloc(100);
	sig=strtok(comando, " ");
	while(sig!=NULL){
		sig=strtok(NULL, " ");
		if(strstr(sig,"INPUTFILE=")!=NULL){
			sig=strstr(sig,"=")+1;
			strcat(arreglo," ");
			strcat(arreglo,sig);
			break;
		}
		else if(strstr(sig,"REDIR")==NULL){
			if(i==0){
				strcpy(arreglo,sig);
			}else{
				strcat(arreglo," ");
				strcat(arreglo,sig);
			}
			i++;
		}
	}
	return arreglo;
}

//Recorta la linea que ingresa el usuario para obtener unicamente el comando requerido
char *arreglarComandoInputCompuesto(char comando[100]){
	char* sig;
	int i=0,j=0;
	char* arreglo = malloc(100);
	sig=strtok(comando, "|");
	int b=0;
	while(sig!=NULL){
		if(b==0){
			do{
				sig=strtok(NULL, "|");
				if(strstr(sig,"INPUTFILE")!=NULL){
					b=1;
					char* nueva=sig;
					sig=strtok(nueva, " ");
					if(strstr(sig,"INPUTFILE=")!=NULL){
							sig=strstr(sig,"=")+1;
							strcat(arreglo," ");
							strcat(arreglo,sig);
							break;
						}
						else if(strstr(sig,"REDIR")==NULL){
							if(i==0){
								strcpy(arreglo,sig);
							}else{
								strcat(arreglo," ");
								strcat(arreglo,sig);
							}
							i++;
						}
				}
			}while(strstr(sig,"INPUTFILE")==NULL&&b==0);
		}
		sig=strtok(NULL, " ");
		if(strstr(sig,"INPUTFILE=")!=NULL){
			sig=strstr(sig,"=")+1;
			strcat(arreglo," ");
			strcat(arreglo,sig);
			break;
		}
		else if(strstr(sig,"REDIR")==NULL){
			if(i==0){
				strcpy(arreglo,sig);
			}else{
				strcat(arreglo," ");
				strcat(arreglo,sig);
			}
			i++;
		}
	}
	return arreglo;
}

//Recorta la linea que ingresa el usuario para obtener unicamente el comando requerido
char* arreglarComandoBG(char comando [100]){
	char* sig;
	int i=0,j=0;
	char* arreglo = malloc(100);
	sig=strtok(comando, " ");
	while(sig!=NULL){
		if (i > 0){
			if(i>1){
				strcat(arreglo," ");	
			}
			strcat(arreglo,sig);
			sig=strtok(NULL, " ");
		}
		else{
			sig=strtok(NULL, " ");

		}
		
		i++;
	}
	return arreglo;
}

//Recorta la linea que ingresa el usuario para obtener unicamente el comando requerido
char* arreglarComandoOut(char comando [100]){
	char* sig;
	int i=0,j=0;
	char* arreglo = malloc(100);
	sig=strtok(comando, " ");
	while(sig!=NULL){
		sig=strtok(NULL, " ");
		if(strstr(sig,"OUTPUTFILE")!=NULL){
			break;
		}
		else if(strstr(sig,"REDIR")==NULL){
			if(i==0){
				strcpy(arreglo,sig);
			}else{
				strcat(arreglo," ");
				strcat(arreglo,sig);
			}
			i++;
		}
	}
	
	return arreglo;
}

//Recorta la linea que ingresa el usuario para obtener unicamente el comando requerido
char* arreglarComandoIn(char comando [100]){
	char* sig;
	int i=0,j=0;
	char* arreglo = malloc(100);
	sig=strtok(comando, " ");
	while(sig!=NULL){
		sig=strtok(NULL, " ");
		if(strstr(sig,"INPUTFILE=")!=NULL){
			sig=strstr(sig,"=")+1;
			strcat(arreglo," ");
			strcat(arreglo,sig);
			break;
		}
		else if(strstr(sig,"REDIR")==NULL){
			if(i==0){
				strcpy(arreglo,sig);
			}else{
				strcat(arreglo," ");
				strcat(arreglo,sig);
			}
			i++;
		}
	}
	int le = strlen(arreglo);
	arreglo[le]='\0';
	return arreglo;
}

//Funcion encargada de crear los procesos y ejecutar los comandos dependiendo del tipo de comando
static int command(int entrada, int first, int last){
	
	int status;
	pid_t childpid;
	int fd[2];
	pipe(fd);	
	if ((childpid = fork()) == 0) {
		if(bg&&output==0){
				printf("[%d] %d \n",contBG,getpid());
				waitpid(childpid,NULL,0);
		}
		if(bg&&contVar==nVar-1&&output==1){
				printf("[%d] %d \n",contBG,getpid());
				waitpid(childpid,NULL,0);
		}
		
		if (first == 1 && last == 0 && entrada == 0) {
			// Primer comando
			dup2( fd[WRITE], STDOUT_FILENO );
		} else if (first == 0 && last == 0 && entrada != 0) {
			// Segundo comando
			dup2(entrada, STDIN_FILENO);
			dup2(fd[WRITE], STDOUT_FILENO);
		} else {
			// Último comando
			dup2(entrada, STDIN_FILENO );
			
		}
		
		if(output==1&&contVar==nVar-1){//solo para output
			int fs = open(nombreArchivo, O_CREAT | O_TRUNC | O_WRONLY, 0600);
	      		dup2(fs, STDOUT_FILENO);
	      	 	close(fs);
			nVar=0;
			contVar=0;
		}
		wait(0);
		
		
		if (execvp( args[0], args)){
				error=-1;	
				perror("main");
		}
	
				
		
	}
	
	if (entrada != 0) {
		close(entrada);
	}
	close(fd[WRITE]);//Cierro la salida para no escribir mas
	if (last == 1){
		close(fd[READ]);//Si es el ultimo comando cierro la entrada para no leer mas	
	}
	return fd[READ];
}
 
//Espera a los procesos cuantas veces fueron llamados.
static void esperar(int llamadas){
	int i;
	for (i = 0; i < llamadas; ++i){
		wait(NULL); 
	} 
		
}
//Quita los espacios de la cadena ingresada
static char* quitarEspacios(char* s)
{
	while (isspace(*s)) ++s;
	return s;
}

//Se encarga de separar los comandos y guardarlos en el vector de argumentos 
 static void split(char* comando){
	comando=quitarEspacios(comando);
	char* sig = strchr(comando, ' ');
	int i = 0;
	while(sig != NULL) {
		sig[0] = '\0';
		args[i] = comando;
		++i;
		comando=quitarEspacios(sig+1);
		sig = strchr(comando, ' ');
	}
	if (comando[0] != '\0') {
		args[i] = comando;
		sig = strchr(comando, '\n');
		if(output==0&&input==0){
			sig[0] = '\0';
		}
		++i; 
	}
	args[i] = NULL;
}
 
//Verifica que el comando no es 'quit' y luego hace la llamada a command.
int ejecutar(char* comando, int entrada,int first, int last){
	split(comando);
	if (args[0] != NULL) {
		if (strcmp(args[0], "QUIT") == 0){//si el usuario dese salir
			exit(0);
		} 
		n += 1;
		return command(entrada, first, last);
	}
	return 0;
}



int main()
{
    char linea[2000];
    int puntoycoma=0;
	printf("MiniShell: Escriba 'QUIT' para salir.\n");
	while (1) {
		output=0;
		printf("$proyectoOperativos> ");
		fflush(NULL);
		if (!fgets(linea, 100, stdin)){
			return 0;
		}  
		int entrada = 0,first = 1;
		error=0;
		char* comando2=linea;
		char* sig;
		int len = strlen(comando2)-1;	
		char* comando = comando2;
		if(comando[0]=='B'&&comando[1]=='G'){//verifica que es en background
				contBG++;
				char aux [100];
				strcpy(aux,comando);
				comando=arreglarComandoBG(aux);
				bg=1;

		}
		if(strchr(comando, '|')!=NULL&&strstr(comando, "REDIR")==NULL){//comando compuesto |
			sig=strchr(comando, '|');
			while (sig != NULL) { 
				*sig = '\0'; //Apunta al '|'
				entrada = ejecutar(comando, entrada, first, 0);
				comando = sig + 1;
				sig = strchr(comando, '|'); // Encuentra el siguiente '|' 
				first = 0;
			}
		}else{
			if(strchr(comando, ';')!=NULL){//comando compuesto ;
				char *auxiliar=comando;
				sig=strtok(auxiliar, ";");
				while (sig != NULL) {
					entrada = ejecutar(sig, entrada, first, 1);	
					sig = strtok(NULL, ";");
					esperar(n);
				}
				puntoycoma=1;
				n=0;
			}
			if(strstr(comando, "&&")!=NULL){//comando compuesto &&
				char *auxiliar2=comando;
				sig=strtok(auxiliar2, "&&");
				while (sig != NULL&&error!=-1) {
						entrada = ejecutar(sig, entrada, first, 1);	
						sig = strtok(NULL, "&&");
						esperar(n);
						if(error==-1){
							exit(1);
						}
				}
				puntoycoma=1;
				n=0;
			}
			if(strstr(comando, "REDIR")!=NULL){//REDIRECCIONAMIENO 6 OPCIONES
				if(strchr(comando, '|')!=NULL){//compuesto
					if(strstr(comando,"OUTPUTFILE")!=NULL&&strstr(comando,"INPUTFILE")==NULL){//compuesto output
						entrada=0,first=1;
						char aux [100];
						strcpy(aux,comando);
						char* nombreAr=strtok(comando, "=");
						nombreAr=strtok(NULL, "\n");
						char* auxiliar2=arreglarComandoOut(aux);
						nombreArchivo=nombreAr;
						nVar=contarComandos(auxiliar2)+1;
						sig=strchr(auxiliar2, '|');
						int bandera=0;
						output=1;
						while (bandera<=1) { 
							if(bandera==0){
								*sig = '\0'; //Apunta al '|'
							}
					
							
							entrada = ejecutar(auxiliar2, entrada, first, 0);
							contVar++;
							if(bandera==0){
								auxiliar2 = sig + 1;
							}
							if(strstr(auxiliar2,"|")!=NULL){
								sig = strchr(auxiliar2, '|');	
							}else{
								bandera++;
							}
							first = 0;
						}
						nVar=0;
						output=0;
						contVar=0;
					}else{
						if(strstr(comando,"OUTPUTFILE")==NULL&&strstr(comando,"INPUTFILE")!=NULL){//input compuesto
							char aux [100];
							strcpy(aux,comando);
							char* auxiliar2=arreglarComandoInputCompuesto(aux);
							nVar=contarComandos(auxiliar2)+1;
							contVar=0;
							entrada = ejecutar(auxiliar2, entrada, first, 1);
							esperar(n);
							puntoycoma=1;
							n=0;
						}else{
							if(strstr(comando,"INPUTFILE")!=NULL && strstr(comando,"OUTPUTFILE") !=NULL){//input output compuesto
								char aux [100];
								strcpy(aux,comando);
								char* auxiliar2=arreglarComandoInputCompuesto(aux);
								char* nombreAr=strtok(comando, "=");
								nombreAr=strtok(NULL, "=");
								nombreAr=strtok(NULL,"\n");
								nVar=contarComandos(auxiliar2)+1;
								contVar=0;
								output=1;
								nombreArchivo=nombreAr;
								entrada = ejecutar(auxiliar2, entrada, first, 1);
								contVar++;
								esperar(n);
								output=0;
								puntoycoma=1;
								n=0;
							}
						}
					}
				}
				else{//simple
				n=0;
					if(strstr(comando,"OUTPUTFILE")!=NULL&&strstr(comando,"INPUTFILE")==NULL){//simple output
						char aux [100];
						strcpy(aux,comando);
						
						char* auxiliar2=arreglarComandoOut(aux);
						char* nombreAr=strtok(comando, "=");
						nombreAr=strtok(NULL, "\n");
						nombreArchivo=nombreAr;
						nVar=contarComandos(auxiliar2)+1;
						contVar=0;
						output=1;
						entrada = ejecutar(auxiliar2, entrada, first, 1);
						esperar(n);
						output=0;
						nVar=0;
						contVar=0;
						puntoycoma=1;
						n=0;
					}else{
						if(strstr(comando,"OUTPUTFILE")==NULL&&strstr(comando,"INPUTFILE")!=NULL){//simple input
							char aux [100];
							strcpy(aux,comando);
							char* auxiliar2=arreglarComandoIn(aux);
							nVar=contarComandos(auxiliar2)+1;
							contVar=0;
							entrada = ejecutar(auxiliar2, entrada, first, 1);
							esperar(n);
							puntoycoma=1;
							n=0;
						}else{
							if(strstr(comando,"OUTPUTFILE")!=NULL&&strstr(comando,"INPUTFILE")!=NULL){//input output simple
								char aux [100];
								strcpy(aux,comando);
								char* auxiliar2=arreglarComandoDobleSimple(aux);
								char* nombreAr=strtok(comando, "=");
								nVar=contarComandos(auxiliar2)+1;
								contVar=0;
								nombreAr=strtok(NULL, "=");
								nombreAr=strtok(NULL,"\n");
								nombreArchivo=nombreAr;
								output=1;
								entrada = ejecutar(auxiliar2, entrada, first, 1);
								esperar(n);
								puntoycoma=1;
								contVar=0;
								output=0;
								nVar=0;
								n=0;
							}
						}
						
					}
					
				}
				puntoycoma=1;
				n=0;
			}
			
		}
		if(puntoycoma!=1){//COMANDO SIMPLE
			entrada = ejecutar(comando, entrada, first, 1);
			esperar(n);
			n = 0;
		}
		puntoycoma=0;
		if(bg ==1){
		    bg==0;
	
		}
	}
	
	return 0;
}
 

