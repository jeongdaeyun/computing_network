#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define HTML_TYPE 0
#define GIF_TYPE 1
#define JPEG_TYPE 2
#define MP3_TYPE 3
#define PDF_TYPE 4
#define UNKNOWN_TYPE -1

#define MAX_BUFF 2048

// the Content-Type function (choosing the content type and parsing the object name)
char* parse_html(char *buff, int* obj_type);

int main(int argc, char *argv[])
{
  // valid usage check
  if (argc < 2){
    fprintf(stderr, "usage: http_server [port_num]\n");
    return 0;
  }

  // HTTP header info. http header부분에 정보를 입력하는데 저는 html, gif, jpg, mp3, pdf와 error메세지를 포함해서 실행합니다.
  char html_header[] = "HTTP/1.1 200 Ok\r\nContent-Type: text/html\r\n\r\n"; // html 형태의 경우 header가 html입니다.
  char gif_header[] = "HTTP/1.1 200 Ok\r\nContent-Type: image/gif\r\n\r\n"; // gif 형태의 경우 header가 gif입니다.
  char jpg_header[] = "HTTP/1.1 200 Ok\r\nContent-Type: image/jpeg\r\n\r\n"; // jpg 형태의 경우 header가 jpg입니다.
  char mp3_header[] = "HTTP/1.1 200 Ok\r\nContent-Type: audio/mp3\r\n\r\n"; // mp3 형태의 경우 header가 mp3입니다.
  char pdf_header[] = "HTTP/1.1 200 Ok\r\nContent-Type: application/pdf\r\n\r\n"; // pdf 형태의 경우 header가 pdf입니다.
  char _404_header[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"; // 마지막은 오류가 날경우 어떡게 처리하는 지를 알려줍니다.

  // create a socket, sockat을 생성합니다.
  int server_socket;
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  // define the address, 각각의 주소 값을 지정합니다.
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(atoi(argv[1])); //주소 값을 정수형태로 바꿔주는 atoi를 사용했습니다.
  server_address.sin_addr.s_addr = INADDR_ANY;

  // receive ready
  bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  listen(server_socket, 5);

  // # read bytes
  int n;

  char cbuff[MAX_BUFF]; // client msg buff
  char fbuff[MAX_BUFF]; // file buffer

  // open a file to serve, 파일을 열기위해 변수 선언을 합니다.
  int fd;

  int object_type;
  int client_socket;

  while(1) {  // process keep alive until it receives SIGKILL, 프로세스가 SIGKILL을 수신할 때까지 활성 유지합니다.
    bzero(cbuff, MAX_BUFF);
    bzero(fbuff, MAX_BUFF);

    // get the client msg.
    client_socket = accept(server_socket, NULL, NULL);
    read(client_socket, cbuff, MAX_BUFF);
    printf("%s\n", cbuff);

    // parse and open the object file
    if ((fd = open(parse_html(cbuff, &object_type), O_RDONLY)) != -1) {  //현재 작업중인 디렉토리에서 파일을 찾는데 -1이 아닌 경우, 즉 매핑이 잘 되었습니다.
      if (object_type == HTML_TYPE) // 실행한 형태가 html형태인 경우에는 웹 페이지에 같은 경로상에 존재하는 html파일을 엽니다.
        write(client_socket, html_header, sizeof(html_header)-1); // 저는 index.html을 선언했습니다.
      else if (object_type == GIF_TYPE) // 실행한 형태가 git형태인 경우, 움직이는 사진에는 웹 페이지에 경로상에 gif파일을 열여줍니다.
        write(client_socket, gif_header, sizeof(gif_header)-1); // 저는 easy.gif을 선언했습니다.
      else if (object_type == JPEG_TYPE) //위와 같은 말로 jpg파일일 경우를 나타냅니다.
        write(client_socket, jpg_header, sizeof(jpg_header)-1); // 저는 1.jpg을 선언했습니다.
      else if (object_type == MP3_TYPE) // mp3파일의 형태를 입력한 경우입니다.
        write(client_socket, mp3_header, sizeof(mp3_header)-1); // 저는 lili.mp3을 선언했습니다.
      else if (object_type == PDF_TYPE) // 마지막으로 pdf를 받았을 때 입니다.
        write(client_socket, pdf_header, sizeof(pdf_header)-1); // 저는 comnet.pdf을 선언했습니다.
    }
    else {  // could not open a file, 파일을 찾을 수 없는 경우 에러를 나타내서 404.html이라고 했습니다.
      fd = open("404.html", O_RDONLY);
      write(client_socket, _404_header, sizeof(_404_header)-1);
    }

    // response to client, 클라이언트에 대응 하는 값들 입니다.
    while((n=read(fd, fbuff, MAX_BUFF))>0)
      write(client_socket, fbuff, n);

    close(client_socket); //파일을 엵고 닫기를 해야해서 write함수를 썼고, 열린 웹을 닫아주기 위해서 close함수를 이용했습니다.
  }

  return 0;
}

char* parse_html(char *buff, int* obj_type)
{
  // parsed object name
  char *html_object;

  // type of the object
  char type[5];

  if((html_object = (char*)malloc(sizeof(char)*25)) == NULL)
  {
    printf("error on creating html obj. name...\n");
    exit(1);
  }

  // parse the object name
  char* runner = buff+5;  // object name comes from buff+5 location
  int obj_len = 0;  // length of the object file name
  while(*runner != ' ') { //file이름을 모두 입력받는 과정입니다. 그래서 ' '를 가점으로 받는 다는 것 입니다..
    strncat(html_object, runner, 1);
    obj_len++;
    runner++;
  }

  if (*runner == ' ' && obj_len == 0) { // quit right away with default html
    *obj_type = HTML_TYPE;
    strcpy(html_object, "index.html"); //아무것도 못받았을때 index.html파일을 실행합니다
    return html_object;
  }

  // choose the type
  int i;
  runner = html_object;
  while(*runner != '.') runner++;
  runner++; // we don't need '.'

  for(i=0; i<4; i++) {
    type[i] = *runner;
    runner++;
  }
  type[4] = '\0'; // null at the end, 마지막 값에 NULL값을 넣어줍니다.

  if(strcmp(type, "html") == 0) *obj_type = HTML_TYPE; //타입을 받고나서 이 파일이 먼지 strcmp를 이용하고있습니다 처음은 html
  else if(strcmp(type, "jpg") == 0) *obj_type = JPEG_TYPE; // 두번째는 jpg파일
  else if(strcmp(type, "gif") == 0) *obj_type = GIF_TYPE; // 4번째는 움직이는 그림 gif파일
  else if(strcmp(type, "mp3") == 0) *obj_type = MP3_TYPE; // 5째는 그림이 아니라 음성파일이 들어왔을 때 형태입니다.
  else if(strcmp(type, "pdf") == 0) *obj_type = PDF_TYPE; // 6번째는 pdf를 가져오는 작업인데 컴퓨터 기본코드등에 합쳐서 점수를 판별합니다.
  else *obj_type = UNKNOWN_TYPE;

  return html_object;
}