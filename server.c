#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/sendfile.h>

char *getword(char *text, char *word){
    char *ptr = text;
    char *qtr = word;
    word[0] = '\0';
    char *textend = text + strlen(text);
    while(*ptr == '\r' || *ptr == '\n'){
        ptr++;
    }
    while(*ptr != '\r' && *ptr != '\n' && ptr < textend){
        *qtr = *ptr;
        ptr++;
        qtr++;
    }
    *qtr = '\0';
    if(word[0] == '\0'){
        return NULL;
    }
    return ptr;
}// 取字串

void get_backgroud(char *tmp, char *type){
    char *ptr = strstr(tmp, "/h");
    ptr +=1;
    while(*ptr != ' '){
        *type = *ptr;
        ptr++;
        type++;
    }
    type = '\0';
}

void conn_handler(int fd){
    int ret, cnt = 0;
    char buffer[80960], style[1024], tmp[1024][1024], word[1024];
    ret = recv(fd, buffer, 80960, 0);
    char *ptr = buffer;
    if(ret == -1 || ret == 0){
        exit(-1);
    }
    while((ptr = getword(ptr, word)) != NULL){
        strcpy(tmp[cnt], word);
        cnt++;
    }
    printf("====================================================\n");
    for(int i = 0; i < cnt; i++){
        printf("%s\n", tmp[i]);
    }
    printf("====================================================\n");

    if(strncmp(tmp[0], "GET", 3) == 0){
        if(strstr(tmp[0], ".jpg")){
            printf("tmp[0]: %s\n", tmp[0]);
            get_backgroud(tmp[0], style);
            printf("%s\n", style);
            char img_header[] =
                "HTTP/1.1 200 OK\r\n"
                "Content-type: image/jpeg\r\n"
                "\r\n"
                ;
            write(fd, img_header, sizeof(img_header) - 1);
            int img_fd = open(style, O_RDONLY);
            if(img_fd == -1){
                perror("open");
            }
            int res;
            char buffer3[8192];
            while((res = read(img_fd, buffer3, 1024)) > 0){
                write(fd, buffer3, res);
            }
            shutdown(fd, SHUT_RDWR);
            close(fd);
            close(img_fd);
        }
        else{
            char header[] =  
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                ;
            write(fd, header, sizeof(header) - 1);

            int html_fd = open("index.html", O_RDONLY);
            int res;
            char buffer2[8192];
            while((res = read(html_fd, buffer2, 1024)) > 0){
                write(fd, buffer2, res);
            }
            shutdown(fd, SHUT_RDWR);
            close(fd);
            close(html_fd);
        }
    }

    if(strncmp(tmp[0], "POST", 4) == 0){
        int length; 
        char *num = strchr(tmp[3], ' ') + 1;
        char boundary[1024];
        char *bound_ptr;
        length = atoi(num);
        char **data_storage = malloc(length * sizeof(char *)+1);
        int idx = 0;
        while(strstr(tmp[idx], "boundary=") == NULL){
            idx++;
        }
        bound_ptr = strchr(tmp[idx], '=') + 1;
        strcpy(boundary, bound_ptr);
        idx++;
        while(strstr(tmp[idx], boundary) == NULL){
            idx++;
        }
        idx += 3;
        int cnt = 0;
        while(strstr(tmp[idx], boundary) == NULL){
            *(data_storage + cnt) = malloc(strlen(tmp[idx]));
            strcpy(*(data_storage + cnt), tmp[idx]);
            cnt++;
            idx++;
        }
        //printf("==========================================\n");
        /*for(int i = 0; i < cnt; i++){
            printf("%s\n", *(data_storage + i));
        }*/
        //printf("==========================================\n");/
        int data_fd = open("client_data.txt", O_RDWR|O_CREAT);
        for(int i = 0; i < cnt; i++){
            write(data_fd, *(data_storage + i), strlen(*(data_storage + i)));
            write(data_fd, "\n", 1);
        }
        char post_header[] =  
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                ;
            write(fd, post_header, sizeof(post_header) - 1);
 
            int html_fd = open("index.html", O_RDONLY);
            int res;
            char buffer4[8192];
            while((res = read(html_fd, buffer4, 1024)) > 0){
                write(fd, buffer4, res);
            }
            shutdown(fd, SHUT_RDWR);
            close(fd);
            close(html_fd);
    } 
}
 
int main() {
    int listenfd, connfd;
    unsigned val = 1;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    socklen_t length;

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
 
    memset(&servaddr, '\0', sizeof(servaddr));
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(80);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0){ 
        perror("bind"); 
        exit(-1);
    }

    if (listen(listenfd, SOMAXCONN) < 0) { 
        perror("listen");
        exit(-1); 
    }

    signal(SIGCHLD, SIG_IGN); // prevent child zombie
    
    while(1){
        length = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &length);
        fprintf(stderr, "accept!\n");
        if (connfd < 0) {
            perror("accept");
            continue;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }
        //child
        if (pid == 0) {
            close(listenfd);
            conn_handler(connfd);
            exit(0);
        }
        close(connfd);
    }
}
