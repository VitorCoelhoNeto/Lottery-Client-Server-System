/*
	Simple winsock client
	Código por Vítor Neto 68717, João Leal 68719, Hugo Anes 68571
*/

//Inclusão das bibliotecas necessárias
#include<stdio.h> 
#include<winsock2.h>
#include<string.h>
#include <time.h>

#pragma comment(lib,"ws2_32.lib") 
#pragma warning(disable : 4996)

//Programa principal
int main(int argc, char* argv[])
{
	//Inicialização das variáveis
	WSADATA wsa;						   //Variável que armazena as informações do socket
	SOCKET s;							   //Socket utilizada para a conexão
	struct sockaddr_in server;			   //Estrutura que vai armazenar os dados do servidor ao qual o cliente se vai conectar, como o IP e o porto
	char* message = (char*)malloc(4096);   //Mensagem que vai ser enviada ao servidor
	char server_reply[4096];			   //Variável que armazena a resposta do servidor
	int recv_size;						   //Tamanho da resposta do servidor, irá ser também verificada para resolver problemas de troca de informações
	int ws_result;						   //Resultado da tentativa de conexão com o servidor, se for menor que 0 dá erro
	char ip[35];						   //Vetor que irá armazenar o IP ao qual o utilizador se tentará conectar
	int connected = 0;					   //Estado da conexão (0 = desconectado, 1 = conectado)
	char* codeString = (char*)malloc(4096);//Vetor de caracteres que irá armazenar a resposta do servidor para futura comparação dos status codes
	int codeInt = 0;					   //Variável que armazena os status codes passados pela codeString, para o cliente decidir a ação necessária a tomar

	//Inicializar a socket
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) //Caso haja problemas na inicialização da socket, dá erro
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}
	printf("Initialised.\n");

	//Criação da socket
	s = socket(AF_INET, SOCK_STREAM, 0); 
	if (s == INVALID_SOCKET) //Caso a socket seja inválida, dá erro
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}
	printf("Socket created.\n");

	//Loop que pede continuamente ao utilizador do cliente que forneça um IP para que este se conecte com um servidor, que só acaba quando a variável connected passa a 1
	//ou seja, quando o cliente encontra um servidor válido com o qual se pode conectar
	while (connected == 0) {
		
		here:
		printf("Insert server IP to which you would like to connect => ");
		//scanf("%s", &ip); //Scanf não estava a funcionar então usamos o fgets
		fgets(ip, 35, stdin);

		//Criar o endereço da socket, atribuindo o IP e o porto (68000)
		server.sin_addr.s_addr = inet_addr(ip);
		server.sin_family = AF_INET;
		server.sin_port = htons(68000);

		//Conectar ao servidor remoto
		ws_result = connect(s, (struct sockaddr*)&server, sizeof(server));
		if (ws_result < 0) //Se o resultado da conexão for menor que 0, significa que o IP inserido é unreachable, e dá o seguinte erro:
		{
			puts("Connection error");
			goto here; //Ao dar este erro, salta-mos para o ponto do código "here", voltando ao início do ciclo while
		}

		//Receber a resposta do servidor
		recv_size = recv(s, server_reply, 4096, 0);
		if (recv_size == SOCKET_ERROR) //Verificação de erros no recebimento da resposta do servidor
		{
			puts("recv failed");
		}

		//Se o IP fornecido for válido, o servidor irá responder com o status code "100 OK", fazendo com que o estado da conexão, o connected, passe a 1, fazendo
		//com que o ciclo quebre
		if (strcmp(server_reply, "100 OK") == 0) {
			connected = 1;
			printf("Connected\n");
		}
	}

	//Ciclo infinito para que o cliente consiga enviar comandos ao servidor continuamente
	while (1)
	{
		//Limpeza das variáveis que armazenam as mensagens que são enviadas ao e recebidas do servidor
		ZeroMemory(message, 4096);
		ZeroMemory(server_reply, 4096);
		ZeroMemory(codeString, 4096);

		//Informa o utilizador que comandos pode utilizar e aguarda que este os insira
		fputs("\nCommands: CHAVE X => Generates random keys\nQUIT => Terminates the connection\nWrite something =>", stdout);
		fgets(message, 4096, stdin);

		ws_result = send(s, message, strlen(message) - 1, 0); //Envia a mensagem ao servidor e verifica foi enviado com sucesso
		if (ws_result < 0)
		{
			puts("Send failed");
			return 1;
		}

		recv_size = recv(s, server_reply, 4096, 0); //Recebe a resposta do servidor e verifica se foi recebido com sucesso
		if (recv_size == SOCKET_ERROR)
		{
			puts("recv failed");
		}

		//Nestas 3 linhas de código, pegamos na resposta do servidor e dividimos a resposta em 2 partes.
		//Visto que a resposta do servidor será sempre um status code deste género: NNN MSSG, dividimos a mensagem em 2 partes, delimitada pelo espaço em branco.
		//Com isto, ficamos só com o número do código de estado, convertendo-o posteriormente para inteiro, visto que este estava armazenado num vetor de carateres.
		codeString = server_reply;
		codeString = strtok(codeString, " ");
		codeInt = atoi(codeString);

		//Nesta sequência de ifs encadeados, verifica-se o status code recebido pelo servidor, para que o cliente apresente uma resposta ao utilizador conforme o status code recebido
		//Caso o status code seja 200, significa que o servidor enviou a resposta com sucesso e irá apresentá-la no ecrã, neste caso a chave
		if (codeInt == 200) { 
			recv_size = recv(s, server_reply, 4096, 0);
			server_reply[recv_size] = '\0';
			printf("\nServer => \n%s", server_reply);
			time_t now = time(0); //Apresenta também a data e hora em que a chave foi recebida
			char* dt = ctime(&now);
			printf("Key received on: %s\n", dt);
		}
		else if (codeInt == 421) //Com o status code 421, o servidor informa o cliente de que o comando enviado não é reconhecido pelo mesmo
		{

			printf("Server=> Error: Unrecognised command\n");

		}
		else if (codeInt == 404) //O status code 404 ocorre quando o número de chaves a ser gerado é inválido
		{

			printf("Server=> Error: Can't generate N number of keys\n");

		}
		else if (codeInt == 400) //O código 400 ocorre quando o cliente se desconecta do servidor
		{

			printf("Server=> Connection terminated\n");
			break;

		}
	}

	//Fecha a socket do cliente
	closesocket(s);
	//Limpa a socket
	WSACleanup();

	system("pause");
	return 0;
}