#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <mysql/mysql.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define ARR_CNT 8

void* send_msg(void* arg);
void* recv_msg(void* arg);
void error_handling(char* msg);

char name[NAME_SIZE] = "[Default]";
char msg[BUF_SIZE];

int doorstate=0;

void finish_with_error(MYSQL *con)
{
	fprintf(stderr, "%s\n", mysql_error(con));
	mysql_close(con);
	exit(1);        
}

int main(int argc, char* argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread, mysql_thread;
	void* thread_return;

	if (argc != 4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "%s", argv[3]);

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");

	sprintf(msg, "[%s:PASSWD]", name);
	write(sock, msg, strlen(msg));
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);

	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);

	close(sock);
	return 0;
}


void* send_msg(void* arg)
{
	int* sock = (int*)arg;
	int str_len;
	int ret;
	fd_set initset, newset;
	struct timeval tv;
	char name_msg[NAME_SIZE + BUF_SIZE + 2];

	FD_ZERO(&initset);
	FD_SET(STDIN_FILENO, &initset);

	fputs("Input a message! [ID]msg (Default ID:ALLMSG)\n", stdout);
	while (1) {
		memset(msg, 0, sizeof(msg));
		name_msg[0] = '\0';
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		newset = initset;
		ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);
		if (FD_ISSET(STDIN_FILENO, &newset))
		{
			fgets(msg, BUF_SIZE, stdin);
			if (!strncmp(msg, "quit\n", 5)) {
				*sock = -1;
				return NULL;
			}
			else if (msg[0] != '[')
			{
				strcat(name_msg, "[ALLMSG]");
				strcat(name_msg, msg);
			}
			else
				strcpy(name_msg, msg);
			if (write(*sock, name_msg, strlen(name_msg)) <= 0)
			{
				*sock = -1;
				return NULL;
			}
		}
		if (ret == 0)
		{
			if (*sock == -1)
				return NULL;
		}
	}
}

void* recv_msg(void* arg)
{
	MYSQL* conn;
//	MYSQL_RES* res_ptr;
	MYSQL_ROW sqlrow,sqlrow1;
	int res;
	char sql_cmd[200] = { 0 };
	char sql_cmd1[200] = { 0 };

	char* host = "localhost";
	char* user = "iot";
	char* pass = "pwiot";
	char* dbname = "iotdb";


	int* sock = (int*)arg;
	int i;
	char* pToken;
	char* pArray[ARR_CNT] = { 0 };

	char name_msg[NAME_SIZE + BUF_SIZE + 1];
	int str_len;

	char Product_Name[10];
	int Count=0;
	int Goal=0;
	float Rate;
	conn = mysql_init(NULL);

	puts("MYSQL startup");
	if (!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		fprintf(stderr, "ERROR : %s[%d]\n", mysql_error(conn), mysql_errno(conn));
		exit(1);
	}
	else
		printf("Connection Successful!\n\n");

	while (1) {
		memset(name_msg, 0x0, sizeof(name_msg));
		str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE);
		if (str_len <= 0)
		{
			*sock = -1;
			return NULL;
		}
		fputs(name_msg, stdout);

		name_msg[str_len-1] = 0;  //'\n' remove

		pToken = strtok(name_msg, "[:@]");
		i = 0;
		while (pToken != NULL)
		{
			pArray[i] = pToken;
			if ( ++i >= ARR_CNT)
				break;
			pToken = strtok(NULL, "[:@]");

		}
//[KSH_SQL]GET@LAMP
//[KSH_SQL]SET@LAMP@1
	if(!strcmp(pArray[1],"GET"))
	{
		if(!strcmp(pArray[2],"PRODUCT"))
		{
			sprintf(sql_cmd, "SELECT name, box, customer, worker FROM product WHERE id=\'%s\'", pArray[3]);

			if (mysql_query(conn, sql_cmd)) 
			{
				finish_with_error(conn);
			}
			
			MYSQL_RES *result = mysql_store_result(conn);
			if (result == NULL) 
			{
				finish_with_error(conn);
			}

			MYSQL_ROW sqlrow = mysql_fetch_row(result);
			if (sqlrow == NULL) 
			{
				// 데이터가 없을 경우 대비
				sprintf(sql_cmd, "[%s]%s@%s@NULL@NULL@NULL@NULL\n", pArray[0], pArray[1], pArray[2]);
			}
			else 
			{
				// 모든 데이터를 @로 구분하여 전송
				sprintf(sql_cmd, "[%s]%s@%s@%s@BOXID=%s@CUSTOMERID=%s@WORKERID=%s\n",
						pArray[0], pArray[2], pArray[3],
						sqlrow[0] ? sqlrow[0] : "NULL", // name
						sqlrow[1] ? sqlrow[1] : "NULL", // box
						sqlrow[2] ? sqlrow[2] : "NULL", // customer
						sqlrow[3] ? sqlrow[3] : "NULL"  // worker
				);
			}

			write(*sock, sql_cmd, strlen(sql_cmd));

			mysql_free_result(result);
		}
		else if(!strcmp(pArray[2],"BOX"))
		{
			sprintf(sql_cmd, "SELECT location,empty,product,customer FROM box WHERE id=\'%s\'", pArray[3]);

			if (mysql_query(conn, sql_cmd)) 
			{
				finish_with_error(conn);
			}
			
			MYSQL_RES *result = mysql_store_result(conn);
			if (result == NULL) 
			{
				finish_with_error(conn);
			}

			MYSQL_ROW sqlrow = mysql_fetch_row(result);
			if (sqlrow == NULL) 
			{
				// 데이터가 없을 경우 대비
				sprintf(sql_cmd, "[%s]%s@%s@NULL@NULL@NULL@NULL\n", pArray[0], pArray[1], pArray[2]);
			}
			else 
			{
				// 모든 데이터를 @로 구분하여 전송
				sprintf(sql_cmd, "[%s]%s@%s@ZONE=%s@EMPTY=%s@PRODUCTID=%s@CUSTOMERID=%s\n",
						pArray[0], pArray[1], pArray[2],
						sqlrow[0] ? sqlrow[0] : "NULL", // name
						sqlrow[1] ? sqlrow[1] : "NULL", // box
						sqlrow[2] ? sqlrow[2] : "NULL", // customer
						sqlrow[3] ? sqlrow[3] : "NULL"  // worker
				);
			}

			write(*sock, sql_cmd, strlen(sql_cmd));

			mysql_free_result(result);
			}

	
		}
		else if(!strcmp(pArray[1],"SETHT")) {
			if(!strcmp(pArray[2],"BOX")) {
				double humi = atof(pArray[4]);  // 문자열을 실수형으로 변환
				double temp = atof(pArray[5]);  // 문자열을 실수형으로 변환
				
				sprintf(sql_cmd, "UPDATE box SET humi=%lf, temp=%lf WHERE id='%s'", 
						humi, temp, pArray[3]);
		
				res = mysql_query(conn, sql_cmd);
				if (!res) {
					printf("Updated %lu rows\n", (unsigned long)mysql_affected_rows(conn));
					sprintf(sql_cmd, "[%s]%s@BOXHTSET\n",pArray[0],pArray[3]);
					write(*sock, sql_cmd, strlen(sql_cmd));
				} else {
					fprintf(stderr, "ERROR: %s [%d]\n", mysql_error(conn), mysql_errno(conn));
				}
			}
		}
		else if(!strcmp(pArray[1],"SETINFO")) {
			if(!strcmp(pArray[2],"BOX")) {
				int empty = atoi(pArray[4]);  // 문자열을 실수형으로 변환
				if((!strcmp(pArray[5],"CUSTOMER"))||(!strcmp(pArray[5],"RETURN")))
				{
					sprintf(sql_cmd, "UPDATE box SET empty=%d,product=0,customer=0, date=NOW(), time=NOW() WHERE id='%s'", 
						empty, pArray[3]);
				}
				else
				{
					sprintf(sql_cmd, "UPDATE box SET empty=%d, date=NOW(), time=NOW() WHERE id='%s'", 
						empty, pArray[3]);
				}

				res = mysql_query(conn, sql_cmd);
				if (!res) {
					printf("Updated %lu rows\n", (unsigned long)mysql_affected_rows(conn));
					sprintf(sql_cmd, "[%s]%s@BOXINFOSET\n",pArray[0],pArray[3]);
					write(*sock, sql_cmd, strlen(sql_cmd));
				} else {
					fprintf(stderr, "ERROR: %s [%d]\n", mysql_error(conn), mysql_errno(conn));
				}
			}
			if(!strcmp(pArray[2],"PRODUCT")) {
				int delivery = atoi(pArray[4]);
				sprintf(sql_cmd, "UPDATE product SET delivery=%d, date=NOW(), time=NOW() WHERE id='%s'", 
						delivery, pArray[3]);
		
				res = mysql_query(conn, sql_cmd);
				if (!res) {
					printf("Updated %lu rows\n", (unsigned long)mysql_affected_rows(conn));
					sprintf(sql_cmd, "[%s]%s@PRODUCTINFOSET\n",pArray[0],pArray[3]);
					write(*sock, sql_cmd, strlen(sql_cmd));
					
				} else {
					fprintf(stderr, "ERROR: %s [%d]\n", mysql_error(conn), mysql_errno(conn));
				}

				sprintf(sql_cmd, "SELECT customer,name FROM product WHERE id=\'%s\'", pArray[3]);
				if (mysql_query(conn, sql_cmd)) 
				{
					finish_with_error(conn);
				}
				
				MYSQL_RES *result = mysql_store_result(conn);
				if (result == NULL) 
				{
					finish_with_error(conn);
				}

				MYSQL_ROW sqlrow = mysql_fetch_row(result);

				if (sqlrow == NULL) 
				{
					// 데이터가 없을 경우 대비
					sprintf(sql_cmd, "[%s]FAIL\n", pArray[0]);
					write(*sock, sql_cmd, strlen(sql_cmd));
				}
				else
				{
					if(delivery==0)
					{
						sprintf(sql_cmd, "[CENTER]%s@DELIVERYCOMPLETED\n",pArray[3]);
						write(*sock, sql_cmd, strlen(sql_cmd));
						sprintf(sql_cmd, "[%s]%s@DELIVERYCOMPLETED\n",sqlrow[0],sqlrow[1]);
						write(*sock, sql_cmd, strlen(sql_cmd));
					}
					else
					{
						sprintf(sql_cmd, "[%s]COMPLETED\n",pArray[0]);
						write(*sock, sql_cmd, strlen(sql_cmd));
					}

				}
				mysql_free_result(result);
				}
	}
	else if(!strcmp(pArray[1],"RETURNREQUEST")) {

			sprintf(sql_cmd, "SELECT product_id,password FROM customer WHERE id=\'%s\'", pArray[3]);
			if (mysql_query(conn, sql_cmd)) 
			{
				finish_with_error(conn);
			}
			
			MYSQL_RES *result = mysql_store_result(conn);
			if (result == NULL) 
			{
				finish_with_error(conn);
			}

			MYSQL_ROW sqlrow = mysql_fetch_row(result);



			if (sqlrow == NULL) 
			{
				// 데이터가 없을 경우 대비
				sprintf(sql_cmd, "[%s]REQUESTDENY\n", pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else if((!strcmp(pArray[2],sqlrow[0]))&&(!strcmp(pArray[4],sqlrow[1])))
			{
				sprintf(sql_cmd, "UPDATE product SET delivery=1, returnstate=1 WHERE id='%s'", 
					pArray[2]);
	
				res = mysql_query(conn, sql_cmd);
				if (!res) {
					printf("Updated %lu rows\n", (unsigned long)mysql_affected_rows(conn));
				} else {
					fprintf(stderr, "ERROR: %s [%d]\n", mysql_error(conn), mysql_errno(conn));
				}
	
				sprintf(sql_cmd, "[CENTER]RETURNREQUEST\n");
				write(*sock, sql_cmd, strlen(sql_cmd));
				sprintf(sql_cmd, "[%s]REQUESTCOMPLETED\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else
			{
				sprintf(sql_cmd, "[%s]REQUESTDENY\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			mysql_free_result(result);

			
		
	}

	else if(!strcmp(pArray[1],"OPENBOX")) {
		if(doorstate==0)
		{
		sprintf(sql_cmd, "SELECT customer,empty FROM box WHERE id=\'%s\'", pArray[2]);
		

		if (mysql_query(conn, sql_cmd)) 
			{
				finish_with_error(conn);
			}
			
			MYSQL_RES *result = mysql_store_result(conn);
			if (result == NULL) 
			{
				finish_with_error(conn);
			}

			MYSQL_ROW sqlrow = mysql_fetch_row(result);

			sprintf(sql_cmd, "SELECT password FROM customer WHERE id=\'%s\'", pArray[3]);

			if (mysql_query(conn, sql_cmd)) 
			{
				finish_with_error(conn);
			}
			
			MYSQL_RES *result1 = mysql_store_result(conn);
			if (result1 == NULL) 
			{
				finish_with_error(conn);
			}

			MYSQL_ROW sqlrow1 = mysql_fetch_row(result1);

			sprintf(sql_cmd, "SELECT returnstate FROM product WHERE id=\'%s\'", pArray[5]);

			if (mysql_query(conn, sql_cmd)) 
			{
				finish_with_error(conn);
			}
			
			MYSQL_RES *result2 = mysql_store_result(conn);
			if (result1 == NULL) 
			{
				finish_with_error(conn);
			}

			MYSQL_ROW sqlrow2 = mysql_fetch_row(result2);
			
			if ((sqlrow == NULL)||(sqlrow1 == NULL)||(sqlrow2 == NULL)) 
			{
				// 데이터가 없을 경우 대비
				sprintf(sql_cmd, "[%s]NONE\n", pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else if(!strcmp(sqlrow[0],"0"))
			{
				sprintf(sql_cmd, "[%s]INVAILD!\n", pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else if(strcmp(sqlrow[0],pArray[3])!=0)
			{
				sprintf(sql_cmd,"[%s]WRONGBOX\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else if(strcmp(sqlrow1[0],pArray[4])!=0)
			{
				sprintf(sql_cmd,"[%s]WRONGPW\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else if(!strcmp(sqlrow2[0],"1"))
			{
				int empty = atoi(sqlrow[1]);
				if(empty==1) empty=0;
				else if(empty==0) empty=1;
				
				sprintf(sql_cmd,"[%s]CORRECT\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
				sprintf(sql_cmd,"[BOX%s]SERVO@ON@RETURN@%d\n",pArray[2],empty);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else
			{
				int empty = atoi(sqlrow[1]);
				if(empty==1) empty=0;
				else if(empty==0) empty=1;
				
				sprintf(sql_cmd,"[%s]CORRECT\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
				sprintf(sql_cmd,"[BOX%s]SERVO@ON@CUSTOMER@%d\n",pArray[2],empty);
				write(*sock, sql_cmd, strlen(sql_cmd));
				
			}


			mysql_free_result(result);
			mysql_free_result(result1);
			mysql_free_result(result2);
			doorstate=1;
		}
		else
		{
			sprintf(sql_cmd,"[%s]alreadyopened\n",pArray[0]);
			write(*sock, sql_cmd, strlen(sql_cmd));
		}
	}
	else if(!strcmp(pArray[1],"CLOSEBOX")) {
		
		if(doorstate==1)
		{
				sprintf(sql_cmd,"[BOX%s]SERVO@OFF@aak\n",pArray[2]);
				write(*sock, sql_cmd, strlen(sql_cmd));
				doorstate=0;
		}
		else
		{
			sprintf(sql_cmd,"[%s]alreadyclosed\n",pArray[0]);
			write(*sock, sql_cmd, strlen(sql_cmd));
		}		

	}
	else if(!strcmp(pArray[1],"OPENBOXADMI")) {
		if(doorstate==0)
		{
		
		sprintf(sql_cmd, "SELECT password FROM worker WHERE id=\'%s\'", pArray[3]);
		

		if (mysql_query(conn, sql_cmd)) 
			{
				finish_with_error(conn);
			}
			
			MYSQL_RES *result = mysql_store_result(conn);
			if (result == NULL) 
			{
				finish_with_error(conn);
			}

			MYSQL_ROW sqlrow = mysql_fetch_row(result);


			if (sqlrow == NULL) 
			{
				// 데이터가 없을 경우 대비
				sprintf(sql_cmd, "[%s]INVAILD\n", pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else if(!strcmp(sqlrow[0],pArray[4]))
			{

				sprintf(sql_cmd,"[%s]CORRECT\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
				sprintf(sql_cmd,"[BOX%s]SERVO@ON@WORKER\n",pArray[2]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else
			{
				sprintf(sql_cmd,"[%s]WRONGPW\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}


			mysql_free_result(result);
			doorstate=1;
		}
		else
		{
			sprintf(sql_cmd,"[%s]alreadyopened\n",pArray[0]);
			write(*sock, sql_cmd, strlen(sql_cmd));
		}
	}
	else if(!strcmp(pArray[1],"REQUEST")) {

		sprintf(sql_cmd, "SELECT password FROM customer WHERE id=\'%s\'", pArray[2]);
		

		if (mysql_query(conn, sql_cmd)) 
			{
				finish_with_error(conn);
			}
			
			MYSQL_RES *result = mysql_store_result(conn);
			if (result == NULL) 
			{
				finish_with_error(conn);
			}

			MYSQL_ROW sqlrow = mysql_fetch_row(result);


			if (sqlrow == NULL) 
			{
				// 데이터가 없을 경우 대비
				sprintf(sql_cmd, "[%s]INVAILD\n", pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else if(!strcmp(sqlrow[0],pArray[3]))
			{
				sprintf(sql_cmd, "UPDATE customer SET product='%s', date=NOW(), time=NOW() WHERE id='%s'", 
					pArray[4], pArray[2]);

				res = mysql_query(conn, sql_cmd);
				if (!res) {
				printf("Updated %lu rows\n", (unsigned long)mysql_affected_rows(conn));
				sprintf(sql_cmd, "[%s]REQUESTCOMPLETED\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
				sprintf(sql_cmd, "[CENTER]DELIVERYREQUEST@CUSTOMERID=%s\n",pArray[2]);
				write(*sock, sql_cmd, strlen(sql_cmd));
				} else {
				fprintf(stderr, "ERROR: %s [%d]\n", mysql_error(conn), mysql_errno(conn));
				}
				
			}
			else
			{
				sprintf(sql_cmd,"[%s]WRONGPW\n",pArray[0]);
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			mysql_free_result(result);
	}
	
}
//	mysql_free_result(res_ptr);
	mysql_close(conn);

}

void error_handling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

