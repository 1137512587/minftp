#include "privsock.h"
#include "common.h"
#include "sysutil.h"

void priv_sock_init(session_t *sess)
{
  int sockfd[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) < 0)
    ERR_EXIT("socketpair");
  sess->parent_fd = sockfd[0];
  sess->child_fd = sockfd[1];
}
void priv_sock_close(session_t *sess)
{
  if (sess->parent_fd != -1) {
    close(sess->parent_fd);
  }
  
  if (sess->child_fd != -1) {
    close(sess->child_fd);
  }
}
void priv_sock_set_parent_context(session_t *sess)
{
  if (sess->child_fd != -1) {
    close(sess->child_fd);
  }
}
void priv_sock_set_child_context(session_t *sess)
{
  if (sess->parent_fd != -1) {
    close(sess->parent_fd);
  }
}

void priv_sock_send_cmd(int fd, char cmd)
{
  int ret;
  ret = writen(fd, &cmd, sizeof(cmd));
  if (ret!= sizeof(cmd)) {
    fprintf(stderr,"priv_sock_send_cmd\n");
    exit(EXIT_FAILURE);
  }
}

int priv_sock_get_cmd(int fd)
{
  char cmd;
  int ret;
  ret = readn(fd, &cmd, sizeof(cmd));
  if (ret == 0) {
      printf("ftp process exit.\n");
      exit(EXIT_SUCCESS);
  }
  if (ret != sizeof(cmd)) {
    fprintf(stderr,"priv_sock_get_cmd error\n");
    exit(EXIT_FAILURE);
  }
  return cmd;
}

void priv_sock_send_result(int fd, char res)
{
  int ret;
  ret = writen(fd, &res, sizeof(res));
  if (ret != sizeof(res)) {
    fprintf(stderr,"priv_sock_send_result error\n");
    exit(EXIT_FAILURE);
  }
}

int priv_sock_get_result(int fd)
{
  char res;
  int ret;
  ret = readn(fd, &res, sizeof(res));
  if (ret != sizeof(res)) {
    fprintf(stderr,"priv_sock_get_reult error\n");
    exit(EXIT_FAILURE);
  }
  return res;
}

void priv_sock_send_int(int fd, int the_int)
{
  int ret;
  ret = writen(fd, &the_int, sizeof(the_int));
  if (ret != sizeof(the_int)) {
    fprintf(stderr,"priv_sock_send_int\n");
    exit(EXIT_FAILURE);
  }
}
int priv_sock_get_int(int fd)
{
  int the_int;
  int ret;
  ret = readn(fd, &the_int, sizeof(the_int));
  if (ret!=sizeof(the_int)) {
    fprintf(stderr,"priv_sock_get_int error\n");
    exit(EXIT_FAILURE);
  }
  return the_int;
}
void priv_sock_send_buf(int fd, char *buf, unsigned int len)
{
  priv_sock_send_int(fd, (int)len);
  int ret = writen(fd, buf, len);
  if (ret != (int)len) {
    fprintf(stderr,"priv_sock_send_buf error\n");
    exit(EXIT_FAILURE);
  }
}
void priv_sock_recv_buf(int fd,char *buf, unsigned int len)
{
  int ret;
  unsigned int recv_len = priv_sock_get_int(fd);
  if (recv_len > len) {
    fprintf(stderr,"priv_sock_get_int error\n");
    exit(EXIT_FAILURE);
  }
  ret = readn(fd, buf, recv_len);
  if (ret!=recv_len) {
    fprintf(stderr,"priv_sock_recv_buf error\n");
    exit(EXIT_FAILURE);
  }
  
}
void priv_sock_send_fd(int sock_fd, int fd)
{
  send_fd(sock_fd, fd);
}
int priv_sock_recv_fd(int sock_fd)
{
  return recv_fd(sock_fd);
}