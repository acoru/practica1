/*******************************************************
Protocolos de Transporte
Grado en Ingeniería Telemática
Dpto. Ingeníería de Telecomunicación
Univerisdad de Jaén

Fichero: servidor.c
Versión: 1.0
Fecha: 23/09/2012
Descripción:
	Servidor de eco sencillo TCP.

Autor: Juan Carlos Cuevas Martínez

*******************************************************/
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <Winsock2.h>

#include "protocol.h"




main()
{

	WORD wVersionRequested;
	WSADATA wsaData;
	SOCKET sockfd,nuevosockfd;
	struct sockaddr_in  local_addr,remote_addr;
	char buffer_out[1024],buffer_in[1024], cmd[10], usr[10], pas[10];
	int err,tamanio;
	int fin=0, fin_conexion=0;
	int recibidos=0,enviados=0;
	int estado=0;
	//NEW VARIABLES FOR SUM FROM HERE
	//cSum fSpace and sSpace will be use for comparing the correct format of the SUM functionality
	char cSum[4] = "", fSpace[2] = "", sSpace[2] = "", cCRLF[3] = "";
	//cNum1 and cNum2 same as the other chars written before, but this will be use for the numbers
	char cNum1[5] = "", cNum2[5] = "";
	//here is where the numbers will be saved before the SUM
	int num1 = 0, num2 = 0;
	int suma = 0;
	//END OFF NEW VARIABLES

	/** INICIALIZACION DE BIBLIOTECA WINSOCK2 **
	 ** OJO!: SOLO WINDOWS                    **/
	wVersionRequested=MAKEWORD(1,1);
	err=WSAStartup(wVersionRequested,&wsaData);
	if(err!=0){
		return(-1);
	}
	if(LOBYTE(wsaData.wVersion)!=1||HIBYTE(wsaData.wVersion)!=1){
		WSACleanup() ;
		return(-2);
	}
	/** FIN INICIALIZACION DE BIBLIOTECA WINSOCK2 **/


	sockfd=socket(AF_INET,SOCK_STREAM,0);//Creación del socket

	if(sockfd==INVALID_SOCKET)	{
		return(-3);
	}
	else {
		local_addr.sin_family		=AF_INET;			// Familia de protocolos de Internet
		local_addr.sin_port			=htons(TCP_SERVICE_PORT);	// Puerto del servidor
		local_addr.sin_addr.s_addr	=htonl(INADDR_ANY);	// Direccion IP del servidor Any cualquier disponible
													// Cambiar para que conincida con la del host
	}
	
	// Enlace el socket a la direccion local (IP y puerto)
	if(bind(sockfd,(struct sockaddr*)&local_addr,sizeof(local_addr))<0)
		return(-4);
	
	//Se prepara el socket para recibir conexiones y se establece el tamaño de cola de espera
	if(listen(sockfd,5)!=0)
		return (-6);
	
	tamanio=sizeof(remote_addr);

	do
	{
		printf ("SERVIDOR> ESPERANDO NUEVA CONEXION DE TRANSPORTE\r\n");
		
		nuevosockfd=accept(sockfd,(struct sockaddr*)&remote_addr,&tamanio);

		if(nuevosockfd==INVALID_SOCKET) {
			
			return(-5);
		}

		printf ("SERVIDOR> CLIENTE CONECTADO\r\nSERVIDOR [IP CLIENTE]> %s\r\nSERVIDOR [CLIENTE PUERTO TCP]>%d\r\n",
					inet_ntoa(remote_addr.sin_addr),ntohs(remote_addr.sin_port));

		//Mensaje de Bienvenida
		sprintf_s (buffer_out, sizeof(buffer_out), "%s Bienvenindo al servidor de ECO%s",OK,CRLF);
		
		enviados=send(nuevosockfd,buffer_out,(int)strlen(buffer_out),0);
		//TODO Comprobar error de envío
		if(enviados == SOCKET_ERROR)
		{
			//-7 value chosen for send error code
			return(-7);
			//provisional solution for error sending welcome message to the client, just close the socket
			closesocket(nuevosockfd);
		}
		//Se reestablece el estado inicial
		estado = S_USER;
		fin_conexion = 0;

		printf ("SERVIDOR> Esperando conexion de aplicacion\r\n");
		do
		{
			//Se espera un comando del cliente
			recibidos = recv(nuevosockfd,buffer_in,1023,0);
			//TODO Comprobar posible error de recepción
			if(recibidos == SOCKET_ERROR)
			{
				//-8 value chosen for recv(reception) error code
				return(-8);
				//provisional solution for reciving error from client, just close the socket
				closesocket(nuevosockfd);
			}
			
			buffer_in[recibidos] = 0x00;
			printf ("SERVIDOR [bytes recibidos]> %d\r\nSERVIDOR [datos recibidos]>%s", recibidos, buffer_in);
			
			switch (estado)
			{
				case S_USER:    /*****************************************/
					strncpy_s ( cmd, sizeof(cmd),buffer_in, 4);
					cmd[4]=0x00; // en C los arrays finalizan con el byte 0000 0000

					if ( strcmp(cmd,SC)==0 ) // si recibido es solicitud de conexion de aplicacion
					{
						sscanf_s (buffer_in,"USER %s\r\n",usr,sizeof(usr));
						
						// envia OK acepta todos los usuarios hasta que tenga la clave
						sprintf_s (buffer_out, sizeof(buffer_out), "%s%s", OK,CRLF);
						
						estado = S_PASS;
						printf ("SERVIDOR> Esperando clave\r\n");
					} else
					if ( strcmp(cmd,SD)==0 )
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Fin de la conexión%s", OK,CRLF);
						fin_conexion=1;
					}
					else
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Comando incorrecto%s",ER,CRLF);
					}
				break;

				case S_PASS: /******************************************************/

					
					strncpy_s ( cmd, sizeof(cmd), buffer_in, 4);
					cmd[4]=0x00; // en C los arrays finalizan con el byte 0000 0000

					if ( strcmp(cmd,PW)==0 ) // si comando recibido es password
					{
						sscanf_s (buffer_in,"PASS %s\r\n",pas,sizeof(usr));

						if ( (strcmp(usr,USER)==0) && (strcmp(pas,PASSWORD)==0) ) // si password recibido es correcto
						{
							// envia aceptacion de la conexion de aplicacion, nombre de usuario y
							// la direccion IP desde donde se ha conectado
							sprintf_s (buffer_out, sizeof(buffer_out), "%s %s IP(%s)%s", OK, usr, inet_ntoa(remote_addr.sin_addr),CRLF);
							estado = S_DATA;
							printf ("SERVIDOR> Esperando comando\r\n");
						}
						else
						{
							sprintf_s (buffer_out, sizeof(buffer_out), "%s Autenticación errónea%s",ER,CRLF);
						}
					} else
					if ( strcmp(cmd,SD)==0 )
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Fin de la conexión%s", OK,CRLF);
						fin_conexion=1;
					}
					else
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Comando incorrecto%s",ER,CRLF);
					}
				break;

				case S_DATA: /***********************************************************/
					
					buffer_in[recibidos] = 0x00;
					
					strncpy_s(cmd,sizeof(cmd), buffer_in, 4);

					printf ("SERVIDOR [Comando]>%s\r\n",cmd);
					
					if ( strcmp(cmd,SD)==0 )
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Fin de la conexión%s", OK,CRLF);
						fin_conexion=1;
					}
					else if (strcmp(cmd,SD2)==0)
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Finalizando servidor%s", OK,CRLF);
						fin_conexion=1;
						fin=1;
					}
					//NEW CODE ADDED from here for SUM
					else if(strcmp(cmd, "SUM ") == 0)
					{
						//sscanf_s (buffer_in,"PASS %s\r\n",pas,sizeof(usr));
						strncpy(cSum, buffer_in, 3);
						strncpy(fSpace, buffer_in + 3, 1);
						strncpy(sSpace, buffer_in + 8, 1);
						strncpy(cNum1, buffer_in + 4, 4);
						strncpy(cNum2, buffer_in + 9, 4);
						strncpy(cCRLF, buffer_in + 13, 2);
						sscanf_s(cNum1, "%d", &num1);
						sscanf_s(cNum1, "%d", &num2);
						if(strcmp(cSum, SUM) == 0 && strcmp(fSpace, " ") == 0 && strcmp(sSpace, " ") == 0 && strcmp(cCRLF, CRLF) == 0 && num1 > 0 && num1 < 9999 && num2 > 0 && num2 < 9999)
						{
							suma = num1 + num2;
							printf("%d", suma);
							sprintf_s(buffer_out, sizeof(buffer_out), "%s %d%s", OK, suma, CRLF);
						}
						else
						{
							sprintf_s(buffer_out, sizeof(buffer_out), "%s%s", ER, CRLF);
						}
					}
					//END OF NEW CODE
					else
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Comando incorrecto%s",ER,CRLF);
					}
					break;

				default:
					break;
					
			} // switch

			enviados=send(nuevosockfd,buffer_out,(int)strlen(buffer_out),0);
			//TODO 
			if(enviados == SOCKET_ERROR)
			{
				//if an error occurs sending data from the state machine, it will return as error code "-9"
				return(-9);
				//provisional solution for sending error, just close the socket created for the current client conection
				closesocket(nuevosockfd);
			}


		} while (!fin_conexion);
		printf ("SERVIDOR> CERRANDO CONEXION DE TRANSPORTE\r\n");
		shutdown(nuevosockfd,SD_SEND);
		closesocket(nuevosockfd);

	}while(!fin);

	printf ("SERVIDOR> CERRANDO SERVIDOR\r\n");

	return(0);
} 
