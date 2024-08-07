#include "ftpproto.h"
#include "session.h"
#include "sysutil.h"
#include "str.h"
#include "common.h"
#include "tunable.h"


int list_common(session_t *sess);
static void ftp_reply(session_t *sess, int status,  const char *text);
int port_active(session_t *sess);
int pasv_active(session_t *sess);
static void do_user(session_t *sess);
static void do_pass(session_t *sess);
static void do_feat(session_t *sess);
static void do_syst(session_t *sess);
static void do_cwd(session_t *sess);
static void do_cdup(session_t *sess);
static void do_quit(session_t *sess);
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_type(session_t *sess);
static void do_stru(session_t *sess);
static void do_node(session_t *sess);
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);
static void do_appe(session_t *sess);
static void do_list(session_t *sess);
static void do_nlst(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);
static void do_nkd(session_t *sess);
static void do_rnd(session_t *sess);
static void do_dele(session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_site(session_t *sess);
static void do_size(session_t *sess);
static void do_stat(session_t *sess);
static void do_noop(session_t *sess);
static void do_help(session_t *sess);

typedef struct ftpcmd
{
    const char *cmd;
    void (*cmd_handler)(session_t *sess);
}ftpcmd_t;

ftpcmd_t ctrl_cmds[] = 
{
    {"USER", do_user},
    {"PASS", do_pass},
    {"FEAT", do_feat},
    {"PWD",do_pwd},
    {"TYPE",do_type},
    {"PASV",do_pasv},
    {"PORT",do_port},
    {"LIST",do_list}
};


void handle_child(session_t *sess)
{
    int ret;
    writen(sess->ctrl_fd, "220 HelloWord\r\n", strlen("220 HelloWord\r\n"));
    while (1)
    {
        memset(sess->cmdline, 0, sizeof(sess->cmdline));
        memset(sess->cmd, 0,sizeof(sess->cmd));
        memset(sess->arg, 0,sizeof(sess->arg));

        //解析FTP命令和参数
        //处理FTP命令
        ret = readline(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE);
        if (ret == -1)
            ERR_EXIT("readline");
        else if (ret == 0)
            exit(EXIT_SUCCESS);
        str_trim_crlf(sess->cmdline);
        str_split(sess->cmdline, sess->cmd, sess->arg, ' ');
        int i;
        int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
        for (i = 0; i < size; i++)
        {
            if (strcmp(ctrl_cmds[i].cmd, sess->cmd) == 0)
            {
                if (ctrl_cmds[i].cmd_handler != NULL)
                {
                    ctrl_cmds[i].cmd_handler(sess);
                }
                else
                {
                    ftp_reply(sess, 502, "Unimplement command.");
                }
                break;
            }
            
        }

        if (i == size)
        {
            ftp_reply(sess, 500, "unknown command.");
        }
        
    }
    
}

int list_common(session_t *sess)
{
    DIR *dir = opendir(".");
    if(dir == NULL)
    {
        return 0;
    }
    
    
    struct dirent *dt = NULL;
    struct stat sbuf = {0};
    while((dt = readdir(dir)) != NULL)
    {
        if(lstat(dt->d_name, &sbuf) < 0 )
        {
            continue;
        }
        if(dt->d_name == NULL)
        {
            continue;
        }

        if (strcmp(dt->d_name,".") == 0 || strcmp(dt->d_name,"..") == 0)
        {
            continue;
        }
        
        char buf[1024] = {0};
        int off = 0;
        char perm[] = "----------";
        perm[0] = '?';
        mode_t mode = sbuf.st_mode;
        switch (mode & S_IFMT)
        {
            case S_IFSOCK:
                perm[0] = 's';
                break;
            case S_IFLNK:
                perm[0] = 'l';
                break;
            case S_IFREG:
                perm[0] = '-';
                break;
            case S_IFBLK:
                perm[0] = 'b';
                break;
            case S_IFDIR:
                perm[0] = 'd';
                break;
            case S_IFCHR:
                perm[0] = 'c';
                break;
            case S_IFIFO:
                perm[0] = 'p';
                break;
        }
        
        if (mode & S_IRUSR)
        {
            perm[1] = 'r';
        }
        if (mode & S_IWUSR)
        {
            perm[2] = 'w';
        }
        if (mode & S_IXUSR)
        {
            perm[3] = 'x';
        }
        
        if (mode & S_IRGRP)
        {
            perm[4] = 'r';
        }
        if (mode & S_IWGRP)
        {
            perm[5] = 'w';
        }
        if (mode & S_IXGRP)
        {
            perm[6] = 'x';
        }
        
        if (mode & S_IROTH)
        {
            perm[7] = 'r';
        }
        if (mode & S_IWOTH)
        {
            perm[8] = 'w';
        }
        if (mode & S_IXOTH)
        {
            perm[9] = 'x';
        }
        if (mode & S_ISUID)
        {
            perm[3] = (perm[3] == 'x') ? ('s') : ('S');
        }
        if (mode & S_ISGID)
        {
            perm[6] = (perm[6] == 'x') ? ('s') : ('S');
        }
        if (mode & S_ISVTX)
        {
            perm[9] = (perm[6] == 'x') ? ('t') : ('T');
        }
        off += sprintf(buf + off, "%s ", perm);
        off += sprintf(buf + off, "%3ld %-8d %-8d ",sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid);
        off += sprintf(buf + off, "%8lu ", (unsigned long)sbuf.st_size);
        const char *p_date_format = "%b %e %H:%M";
        struct timeval tv = {0};
        gettimeofday(&tv, NULL);
        time_t local_time = tv.tv_sec;
        if(sbuf.st_mtime > local_time || (local_time - sbuf.st_mtime) > 60*60*24*182)
        {
            p_date_format = "%b %e %Y";
        }
        char datebuf[64] = {0};
        struct tm *p_tm = localtime(&local_time);
        strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);
        off += sprintf(buf + off, "%s ", datebuf);
        if (S_ISLNK(mode))
        {
            char tmp[124] = {0};
            readlink(dt->d_name, tmp, sizeof(tmp));
            sprintf(buf + off, "%s -> %s\r\n", dt->d_name, tmp);
        }
        else
        {
            sprintf(buf + off,"%s\r\n", dt->d_name);
        }
        
        printf("%s",buf);
        writen(sess->data_fd,buf,strlen(buf));
    }
    closedir(dir);
    return 1;
}

static void ftp_reply(session_t *sess, int status, const char *text)
{
    char buf[1024] = {0};
    sprintf(buf,"%d %s\r\n", status, text);
    writen(sess->ctrl_fd, buf, strlen(buf));
}

  static void ftp_lreply(session_t *sess, int status, const char *text)
 {
     char buf[1024] = {0};
     sprintf(buf, "%d-%s\r\n",status, text);
     writen(sess->ctrl_fd, buf, strlen(buf));
 }

static void do_user(session_t *sess)
{
    struct passwd *pw = getpwnam(sess->arg);
    if (pw == NULL)
    {
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    sess->uid = pw->pw_uid;
    ftp_reply(sess, 331, "Please specify password");  
}

static void do_pass(session_t *sess)
{
    struct passwd *pw = getpwuid(sess->uid);
    if (pw == NULL)
    {
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    
    struct spwd *sp = getspnam(pw->pw_name);
    if (sp == NULL)
    {
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    
    char *encrypted_pass = crypt(sess->arg, sp->sp_pwdp);
    if (strcmp(encrypted_pass, sp->sp_pwdp) != 0)
    {
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    ftp_reply(sess, 230, "Login successful");
}

static void do_feat(session_t *sess)
{
    ftp_lreply(sess, 211, "Features:");
    writen(sess->ctrl_fd,"EPRT\r\n", strlen("EPRT\r\n"));
    writen(sess->ctrl_fd,"EPSU\r\n", strlen("EPSU"));
    writen(sess->ctrl_fd,"MDTM\r\n", strlen("MDTM\r\n")); 
    writen(sess->ctrl_fd,"PASU\r\n", strlen("PASU\r\n"));
    writen(sess->ctrl_fd,"REST STREAM\r\n", strlen("REST STREAM\r\n"));
    writen(sess->ctrl_fd,"SIZE\r\n",strlen("SIZE\r\n")); 
    writen(sess->ctrl_fd,"TUFS\r\n",strlen("TUFS\r\n")); 
    writen(sess->ctrl_fd,"UTF8\r\n",strlen("UTF8\r\n")); 
    ftp_reply(sess, 211, "End");
}

static void do_syst(session_t *sess)
{
   ftp_reply(sess, 215, "SYS TYPE");
}

static void do_cwd(session_t *sess)
{
}
static void do_cdup(session_t *sess)
{
}
static void do_quit(session_t *sess)
{
}
static void do_port(session_t *sess)
{
    unsigned int v[6] = {0};
    sscanf(sess->arg,"%u,%u,%u,%u,%u,%u",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5]);
    sess->port_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(sess->port_addr, 0, sizeof(struct sockaddr_in));
    sess->port_addr->sin_family = AF_INET;
    unsigned char *p = (unsigned char*)&sess->port_addr->sin_port;
    p[0] = v[4];
    p[1] = v[5];
    
    p = (unsigned char *)&sess->port_addr->sin_addr;
    p[0] = v[0];
    p[1] = v[1];
    p[2] = v[2];  
    p[3] = v[3];
    ftp_reply(sess, 200,"PORT command successful.Consider using PASV");
}
static void do_pasv(session_t *sess)
{
  char ip[4] = {0};
  getlocalip(ip);
  sess->pasv_listen_fd = tcp_server(ip,0);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  if (getsockname(sess->pasv_listen_fd, (struct sockaddr*)&addr,&addrlen) < 0) {
    
    ERR_EXIT("getsockname");
  }
  unsigned short port = ntohs(addr.sin_port);
  unsigned int v[4];
  sscanf(ip,"%u.%u.%u.%u",&v[0],&v[1],&v[3],&v[4]);
  char text[1024] = {0};
  sprintf(text,"Entering pasv mode (%u,%u,%u,%u,%u,%u)",v[0],v[1],v[2],v[3],port>>8, port & 0xff);
  ftp_reply(sess,227,text);
}
void do_type(session_t *sess)
{
    if(strcmp(sess->arg,"A") == 0)
    {
        ftp_reply(sess, 200,"switching to ASCII mode");
        sess->is_ascii = 1;
    }
    else if(strcmp(sess->arg,"B") == 0 || strcmp(sess->arg,"I") == 0)
    {
        sess->is_ascii = 0;
        ftp_reply(sess, 200,"switching to Binary mode");
    }
    else
    {
        ftp_reply(sess, 500,"Unrecognized  TYPE command");
    }
}
static void do_stru(session_t *sess)
{
}
static void do_node(session_t *sess)
{
}
static void do_retr(session_t *sess)
{
}
static void do_stor(session_t *sess)
{
}
static void do_appe(session_t *sess)
{
}

int port_active(session_t *sess)
{
  if (sess->port_addr != NULL) {
    if (pasv_active(sess))
      return 0;
    return 1;
  }
  return 0;
}

int pasv_active(session_t *sess)
{
  if (sess->pasv_listen_fd != -1) {
    if (port_active(sess)) {
      fprintf(stderr,"botn port an pasv are active.");
      exit(EXIT_FAILURE);
    }
    return 1;
  }
  return 0;
}
int get_transfer_fd(session_t *sess)
{
  int fd;
  if (!port_active(sess) && !pasv_active(sess))
  {
    fprintf(stderr,"debug:No port or pasv mode.");
    return 0;
  }
  
  if (port_active(sess)) {
    fd = tcp_client(5177);
    if (fd < 0) {
      fprintf(stderr,"Create TCP client fail.");
      return 0;
    }
    
    if (connect_timeout(fd, &sess->port_addr, sizeof(struct sockaddr_in),tunable_connect_tineout) < 0) {
      close(fd);
      return 0;
      
    }
    sess->data_fd = fd;
  }
  
  if (sess->port_addr) {
    free(sess->port_addr);
    sess->port_addr = NULL;
  }
  
  if(pasv_active(sess)) {
    int fd = accept_timeout(sess->pasv_listen_fd,NULL,tunable_accept_tineout);
    if (fd == -1) {
      close(sess->pasv_listen_fd);
      return 0;
    }
    sess->data_fd = fd;
  }
  
  return 1;
}


static void do_list(session_t *sess)
{
  if (get_transfer_fd(sess) == 0) {
    ftp_reply(sess,425,"Use port or pasv frist");
    return;
  }
  ftp_reply(sess,150,"Here comes the directory listing");
  
  list_common(sess);
  close(sess->data_fd);
  ftp_reply(sess,226,"Directory send OK.");
}
static void do_nlst(session_t *sess)
{
}
static void do_rest(session_t *sess)
{
}
static void do_abor(session_t *sess)
{
}
static void do_pwd(session_t *sess)
{
    char text[1024+1] = {0};
    char dir[1024+1] = {0};
    getcwd(dir, 1024);
    sprintf(text, "\"%s\"",dir);
    ftp_reply(sess, 257, text);
}
static void do_nkd(session_t *sess)
{
}
static void do_rnd(session_t *sess)
{
}
static void do_dele(session_t *sess)
{
}
static void do_rnfr(session_t *sess)
{
}
static void do_rnto(session_t *sess)
{
}
static void do_site(session_t *sess)
{
}
static void do_size(session_t *sess)
{
}
static void do_stat(session_t *sess)
{
}
static void do_noop(session_t *sess)
{
}
static void do_help(session_t *sess)
{
}
