#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>
#include "parser.h"
#include <dirent.h>

//DECLARACION DEL TIPO BOOL
typedef int bool;
#define true 1		//Significara que el proceso esta en ejecucion
#define false 0		//Significara que el proceso esta hecho

//DECLARACION DEL TIPO NODOBG: esta estructura nos servira para guardar los procesos que se encuentren en segundo plano o ejecutandose
typedef struct nodoBG
{
	pid_t pid;		//En esta variable guardaremos el pid
	char line[1024];	//Aqui guardaremos la linea de la instruccion
	bool dentro;		//Estado en el que se encuentra el proceso
} nodoBG;

nodoBG e = {0 , "none",false}; //Declaracion de valores en estructuras por defecto

//DECLARACION DEL TIPO VECTOR: esta estructura sera la que usemos como lista para los procesos en segundo plano
typedef struct vector
{
	nodoBG datos[1024];	//Esta sera la lista en si donde guardaremos los procesos
	int longitud;		//En esta variable tenemos la posicion del ultimo insertado mas uno
} vector;
//Declaracion del array listaBG que usaremos para la lista de procesos en segundo plano
vector listaBG[1024];
vector aux[1024];

//REDECLARACION DEL TIPO PASSWD: esta estructura es usada para obtener datos del usuario e imprimirlos por el prompt
struct passwd *datos;

//DEFINICION DE LAS CABECERAS DE LAS FUNCIONES
void cd_cd(char *cd, int argc);
int  fg_fg(int argc, char* eleccion);
void foreground(vector* listaBG, int indice);
void actualizacionListaBG(vector* listaBG);

//Esta variable la usamos al no poder modificar la variable longitud
int total=0;
//FUNCION PRINCIPAL
int main(void) {

	system("clear"); //Borramos la pantalla

	//Ignoramos las señales CTRL+C "CTRL+\"
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	//DECLARACION DE VARIABLES
	FILE* fichero;
	datos = getpwuid(geteuid());
	char *exit = "exit\n";
	char buf[1024];
	tline * line;
	int i;
	char * jobs = "jobs\n";
	char cwd[1024];
	char hostname[1024];

	//BACKUP DE LOS DESCRIPTORES DE FICHERO PARA PODER RECUPERARLOS AL FINAL
	int backup_salida = dup(fileno(stdout));
	int backup_entrada = dup(fileno(stdin));
	int backup_error = dup(fileno(stderr));

	//FUNCIONES PARA LA OBTENCION DE DATOS DEL USUARIO PARA EL PROMPT
	gethostname(hostname,sizeof(hostname));		//Hostname
	getcwd(cwd,sizeof(cwd));			//Directorio

	printf("\e[1;32m%s@%s\e[0m:\e[1;34m%s \e[0mmsh> ",(char *) datos->pw_name, hostname, cwd);	//Prompt

	while ((fgets(buf, 1024, stdin)) && (strcmp(buf,exit)!=0))
	{
		line = tokenize(buf);

		pid_t hijos[line->ncommands];	//Declaracion del array para los pid

		//Comprobamos si ha finalizado algun proceso
		actualizacionListaBG(listaBG);

		if (line==NULL)		//En caso de no se introduzca nada por pantalla, volvera a imprimir el prompt
		{
			goto continuar;
		}
		else			//En caso de que si escriba algo
		{
		//Los siguientes tres if son para la redireccion de entrada, salida y error
		if(line->redirect_input!=NULL)		//ENTRADA
		{
			fichero = fopen(line->redirect_input, "r");
			if(fichero == NULL)
			{
				fprintf(stderr, "\e[1;31m%s: Error: %s\n", line->redirect_input, strerror(errno));
				goto continuar;
			}
			else
			{
				int entrada = fileno(fichero);
				dup2(entrada, fileno(stdin));
				fclose(fichero);
			}
		}
		if(line->redirect_output!=NULL)		//SALIDA
			{
			fichero = fopen(line->redirect_output, "a");
			if(fichero == NULL)
			{
				fprintf(stderr, "\e[1;31m%s: Error: %s\n", line->redirect_output, strerror(errno));
				goto continuar;
			}
			else
			{
				int salida = fileno(fichero);
				dup2(salida, fileno(stdout));
				fclose(fichero);
			}
		}
		if(line->redirect_error!=NULL)		//ERROR
		{
			fichero = fopen(line->redirect_error, "a");
			if(fichero == NULL)
			{
				fprintf(stderr, "\e[1;31m%s: Error: %s\n", line->redirect_output, strerror(errno));
			}
			else
			{
				int error = fileno(fichero);
				dup2(error, fileno(stderr));
				fclose(fichero);
			}
		}
		//Si solo hay una intruccion entra en este if, sino en el siguiente que es para 2 o mas
			if(line->ncommands==1)
			{
				//Primero comprobamos las instrucciones especificas que no tienen nombre, que son todo argumentos
				if((char *) line->commands[0].filename==NULL)
				{
					if(buf[0]=='c' && buf[1]=='d' && (buf[2]=='\n' || buf[2]==' '))
					{
						cd_cd((char *) line->commands[0].argv[1], line->commands[0].argc);
					}
					else if(strcmp(buf, jobs)==0)
					{
						line->background=1;
						if (line->ncommands != 1)
						{
							fprintf(stderr, "\e[1;31mjobs: solo funciona con un comando\n");
							goto continuar;
						}
						else
						{
						        for(i=0;i< listaBG->longitud;i++)
							{
							nodoBG nodo = listaBG->datos[i];
								if(nodo.dentro==1)
								{
									printf("[%i]+ \t Ejecutando \t %s\n", i+1, nodo.line);
									aux->datos[aux->longitud]=listaBG->datos[i];
									aux->longitud++;
								}
								else
								{
									printf("[%i]+ \t Hecho      \t %s\n", i+1, nodo.line);
								}
							}
							for(i=0;i<aux->longitud;i++)
							{
								listaBG->datos[i]=aux->datos[i];
							}
							listaBG->longitud=aux->longitud;
							 aux->longitud=0;
						}
						if(total==0) //En el caso de que no quede nada en jobs se pone a cero el vector
						{
							for(i=0;i< listaBG->longitud;i++)
							{
								nodoBG nodoaux;
								listaBG->datos[i]=nodoaux;
							}
							listaBG->longitud = 0;
						}
					}
					else if(buf[0]=='f'&&buf[1]=='g'&&(buf[2]=='\n'||buf[2]==' '))
					{
						if(fg_fg(line->commands[0].argc,(char *) line->commands[0].argv[1])==2)
						goto continuar;
					}
					else
					{
						fprintf(stdout, "\e[1;31m%s: No se encuentra el mandato\n", line->commands[0].argv[0]);
						goto continuar;	
					}
				}
				else
				{
					//En este else hacemos las instrucciones que si tienen nombre
					hijos[0]=fork();	//Aqui hacemos el unico hijo necesario
					if(hijos[0]==0)
					{
						/**Aqui entra si es el hijo que debe hacer la señal por defecto y despues lanzamos
						 * 	el execvp para hacer la instruccion y en caso de que no se realice se escribe
						 *	por pantalla que se ha producido un error y vuelve al prompt
						 */
						signal(SIGINT, SIG_DFL);
						execvp(line->commands[0].filename, line->commands[0].argv);
						fprintf(stderr,"\e[1;31m%s: No se encuentra el mandato", line->commands[0].filename);
						goto continuar;
					}
					else
					{
						//Este es el padre que debe ignorar la señal
						signal(SIGINT, SIG_IGN);
					}
				}
				//El siguiente if-else es para el caso de que haya background
				if(line->background==0)
				{
					//Si no hay background, los procesos deben esperar a que terminen otros
					wait(NULL);
				}
				else
				{
					/**Este es el caso de sea en background, primero configuramos un nodo (poniendo su pid y su
					 *	informacion) y luego lo añadimos a la lista
					 */
					nodoBG* nodo;
					nodo = (nodoBG *)malloc(sizeof(nodoBG));
					nodo->pid=hijos[0];
					strcpy(nodo->line, strtok(buf, "&")); 	//En esta linea eliminamos el & de final de linea
					if(strcmp(nodo->line, "jobs\n")) 	//Este if es para evitar que añada el jobs a la lista
					{
						nodo->dentro=true;
						listaBG->datos[listaBG->longitud]=*nodo;
						printf("[%i] %i\t %s\n", listaBG->longitud, nodo->pid, nodo->line);
						listaBG->longitud++;
						total++;
					}
					goto continuar;
				}
			}

		//Este es el if para varios comandos
		if(line->ncommands >= 2)
		{
			int i, fd[2], fd2[2];		//Declaracion de los arrays para los pipes y la que sera la que usemos para el for de los hijos
			for(i=0; i< line->ncommands; i++)	//Este es el for que marca el numero de hijos
			{
				if(i != line->ncommands-1)	//Este if es para que no cree un pipe despues del ultimo hijo
				{
					pipe(fd);
				}

				hijos[i] = fork();		//Hacemos el fork para crear los hijos y guardamos su pid
				if(hijos[i]<0)
				{
					/**Si entra aqui significa que ha fallado el fork y se ha producido un error,
					 *	imprime dicho error y vuelve a mostrar el prompt
					 */
					fprintf(stderr,"\e[1;31mFallo en el fork. %s\n", strerror(errno));
					goto continuar;
				}

				if(hijos[i]==0)
				{
					/**En este if entra si es el hijo:
					 *	el hijo debe ignorar la señal, a continuacion dependiendo de en que hijo nos encontremos
					 *	habilitar el pipe correspondiente, es decir, si es cualquiera menos el primero debe habilitar
					 *	los pipes de salida y es cualquiera menos el ultimo, los de entrada.
					 *	A continuacion, debe hacer el execvp() para largar la instruccion e igual que antes,
					 *	en caso de que falle se imprimira el error y se volvera al prompt
					 */
					signal(SIGINT, SIG_DFL);
					if(i != 0)
					{
						if(dup2(fd2[0], 0) < 0)
						{
							goto continuar;
						}
						close(fd2[0]);
						close(fd2[1]);
					}
					if(i != line->ncommands-1)
					{
						if(dup2(fd[1], 1) < 0)
						{
							goto continuar;
						}
						close(fd[1]);
						close(fd[0]);
					}

					execvp(line->commands[i].filename, line->commands[i].argv);
					fprintf(stderr, "\e[1;31m%s: No se encuentra el mandato\n", line->commands[i].argv[0]);
					goto continuar;
				}
				else
				{
					/**Este es el padre:
					 *	Debe ignorar la señales y a continuacion en caso de que no sea el primero cerrar ambos pipes
					 *	ya que nos los va a usar el (de esta manera evita que actuen como bloqueantes) y en caso
					 *	de que no se el ultimo debe redirigir los pipes para que cuando se vuelva a hacer el fork()
					 *	toda la informacion fluya
					 */
					signal(SIGINT, SIG_IGN);
					if(i!=0)
					{
						close(fd2[0]);
						close(fd2[1]);
					}
					if(i != line->ncommands-1)
					{
						fd2[0]=fd[0];
						fd2[1]=fd[1];
					}
				}
			}
			//Este if else, es identico al de una sola instruccion de la linea 220
			if(line->background==0)
			{
				for(i=0; i< line->ncommands; i++)
				{
					wait(NULL);
				}
			}
			else
			{
				nodoBG* nodo;
				nodo = (nodoBG *)malloc(sizeof(nodoBG));
				nodo->pid=hijos[0];
				strcpy(nodo->line, strtok(buf, "&"));
				listaBG->datos[listaBG->longitud]=*nodo;
				nodo->dentro=true;
				printf("[%i] %i\t %s\n", listaBG->longitud, nodo->pid, nodo->line);
				listaBG->longitud++;
				total++;
				goto continuar;
			}

		}

		}
		//En los siguientes tres dup2(), lo que hacemos es devolverles el valor original a los descriptores.
continuar:	dup2(backup_salida, fileno(stdout));
		dup2(backup_entrada, fileno(stdin));
		dup2(backup_error, fileno(stderr));

		getcwd(cwd,sizeof(cwd));		//Actualizar la direccion

		printf("\e[1;32m%s@%s\e[0m:\e[1;34m%s \e[0mmsh> ",(char *) datos->pw_name, hostname, cwd);
	}
	return 0;
}


//Funcion para hacer el cd
void cd_cd (char *cd, int argc)
{
	if(argc==2)		//Si son dos argumentos se cambia a la direccion que se le pasa
	{
		DIR* dir = opendir(cd);
		if (dir)
		{
		chdir(cd);
		}
		else
		{
		fprintf(stderr, "\e[1;31m%s: No es un directorio valido\n",cd);
		}
	}
	else if(argc==1)	//Si es un argumento significa que hay que volver a home
	{
		chdir(getenv("HOME"));
	}
	else
	{
		fprintf(stderr, "\e[1;31mcd: solo funciona con 0 o 1 argumento\n");
	}
}

//Funcion para hacer el fg
int fg_fg (int argc, char* eleccion)
{
	if(argc==1)		//Si tiene un argumento coge el ultimo de la lista
	{
		if(listaBG->longitud != 0)
		{
			int cantidad = listaBG->longitud;
			cantidad--;
			foreground(listaBG, cantidad);
		}
	}
	else if(argc==2)	//Si son dos argumentos coge el seleccionado mediante el segundo argumento
	{
		int cantidad = atoi(eleccion);
		if(cantidad <= listaBG->longitud && cantidad>0)
		{
			foreground(listaBG, cantidad-1);
		}
		else
		{
			fprintf(stderr, "\e[1;31mfg: ese trabajo no existe\n");
			return 2;
		}
	}
	else			//Si hay mas argumentos devuelve un error
	{
		fprintf(stderr, "\e[1;31mfg: solo funciona con 0 o 1 argumento\n");
		return 2;
	}
	return 0;
}

//Funcion para devolver a foreground
void foreground(vector* listaBG,int indice)
{
	//Ignoramos las señales, buscamos el proceso que queremos volver a traer a foreground y lo devolvemos.
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	nodoBG valorNodo = listaBG->datos[indice];
	fprintf(stderr, "%s\n", valorNodo.line);
	total--;
	listaBG->datos[indice].dentro=false;
	waitpid(valorNodo.pid, NULL, 0);
}

//Funcion para comprobar si ha acabado algun proceso en background
void actualizacionListaBG(vector* listaBG)
{
	//Pasamos por todos los elementos de la lista y vamos comprobando uno a uno si ha finalizado y lo actualizamos
	int i, status;
	for(i=0; i< listaBG->longitud; i++)
	{
		nodoBG procesoBG = listaBG->datos[i];

		if(waitpid(procesoBG.pid, &status, WNOHANG))
		{
			printf("[%i]+ \t Hecho      \t %s\n", i+1, procesoBG.line);
		}
		else
		{
		aux->datos[aux->longitud]=listaBG->datos[i];
                aux->longitud++;
		}
	}
	for(i=0;i<aux->longitud;i++)
        {
               listaBG->datos[i]=aux->datos[i];
        }
               listaBG->longitud=aux->longitud;
               aux->longitud=0;
}
