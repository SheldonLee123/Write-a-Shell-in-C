#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <readline/readline.h>

#define normal			0  /* 一般的命令 */
#define out_redirect	1  /* 输出重定向 */
#define in_redirect		2  /* 输入重定向 */
#define have_pipe		3  /* 命令中有管道 */
#define BUFFER_SIZE 256
#define MAX_HIS 50			//number of stored history commands

//declarations
char history[MAX_HIS][BUFFER_SIZE];    //history array to store history commands
int count = 0;

void print_prompt();						/* 打印提示符 */
void get_input(char *);						/* 得到输入的命令 */
//二维数组作为形参，必须指明列的数字
void explain_input(char *, int *, char a[ ][BUFFER_SIZE]);		/* 对输入命令进行解析 */
void do_cmd(int, char a[ ][BUFFER_SIZE]);					/* 执行命令 */
int  find_command(char *);					/* 查找命令中的可执行程序 */
int ex_cmd(char **args);
int search_cmd(char **args);

/*
  Function Declarations for builtin shell commands:
 */
int lsh_info(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);
int lsh_cd(char **args);
int lsh_his(char **args);
int lsh_ls(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] =
{
    "info",
    "exit",
    "pwd",
    "cd",
    "history",
    "ls"
};

int (*builtin_func[]) (char **) =
{
    &lsh_info,
    &lsh_exit,
    &lsh_pwd,
    &lsh_cd,
    &lsh_his,
    &lsh_ls
};

int lsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("lsh");
        }
    }
    return 1;
}

/**
   @brief Builtin command: print info.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_info(char **args)
{
    printf("XJCO2211 Simplified Shell by sc16x2l\n");

    return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return terminate execution.
 */
int lsh_exit(char **args)
{
    printf("The shell was terminated!");
    exit(0);
}

/**
   @brief Builtin command: pwd.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_pwd(char **args)
{
    char buf[80];
    getcwd(buf, sizeof(buf));
    printf("current working directory: %s\n", buf);

    return 1;
}

/**
   @brief Builtin command: history.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_his(char **args)
{
    int i;
    int j = 0, flag;
    int histCount = 0;

    histCount = count > MAX_HIS ? MAX_HIS : count;
    if(count > MAX_HIS)
    {
        flag = count - MAX_HIS;
    }
    else
    {
        flag = 0;
    }
    printf("Shell command history:\n");

    //loop for iterating through commands
    for (i = 0; i < histCount; i++)
    {
        //command index
        printf("%d.  ", ++flag);
        while (history[i][j] != '\n' && history[i][j] != '\0')
        {
            //printing command
            printf("%c", history[i][j]);
            j++;
        }
        printf("\n");
        j = 0;
    }
    printf("\n");
    return 1;
}

/**
   @brief Builtin command: ls.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_ls(char **args)
{
    DIR *dir_ptr;
    int i = 0;
    struct stat s_buf;
    struct dirent *direntp;  /*each entry*/
    if((dir_ptr = opendir(".")) == NULL)
        perror("opendir fails");
    while((direntp = readdir(dir_ptr)) != NULL)
    {
    	stat(direntp->d_name,&s_buf);
        if(S_ISDIR(s_buf.st_mode))
        {
            printf("\033[36m%-13s\033[0m", direntp->d_name);
        }
        else
        {
            if(access(direntp->d_name, X_OK) != -1)
            {
                printf("\033[32m%-13s\033[0m", direntp->d_name);
            }
            else
            {
                printf("%-13s", direntp->d_name);
            }
        }
        i++;
        if(i % 6 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
    closedir(dir_ptr);
    return 1;
}

int main(int argc, char **argv)
{
    int    i;
    int    argcount = 0;
    char   arglist[100][BUFFER_SIZE];
    char   **arg = NULL;
    char   *buf  = NULL;

    buf = (char *)malloc(BUFFER_SIZE);
    if( buf == NULL )
    {
        perror("malloc failed");
        exit(-1);
    }

    while(1)
    {
        /* 将buf所指向的空间清零 */
        memset(buf, 0, BUFFER_SIZE);
        print_prompt();
        get_input(buf); 
        if(count < MAX_HIS)
        {
            strcpy(history[count], buf);        // this will be your 50 th(last) command
            count++;
        }
        else
        {
            for(i = MAX_HIS - 1; i > 0; i--)
            {
                strcpy(history[i - 1], history[i]);
            }
            strcpy(history[MAX_HIS - 1], buf);      // this will be your 50 th(last) command
            count++;
        }
        for (i = 0; i < 100; i++)
        {
            arglist[i][0] = '\0';
        }
        argcount = 0;
        explain_input(buf, &argcount, arglist);
        do_cmd(argcount, arglist);
    }

    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }

    exit(0);
}

void print_prompt()
{
    printf("sc16x2l@shell:~$ ");
}

/*获取用户输入*/
void get_input(char *buf)
{
    int len = 0;
    int ch;

    ch = getchar();
    while (len < BUFFER_SIZE && ch != '\n')
    {
        buf[len++] = ch;
        ch = getchar();
    }

    if(len == BUFFER_SIZE)
    {
        printf("command is too long \n");
        exit(-1); /* 输入的命令过长则退出程序 */
    }

    buf[len] = '\n';
    len++;
    buf[len] = '\0';
}

/* 解析buf中的命令，将结果存入arglist中，命令以回车符号\n结束 */
/* 例如输入命令为"ls -l /tmp"，则arglist[0]、arglist[1]、arglsit[2]分别为ls、-l和/tmp */
void explain_input(char *buf, int *argcount, char arglist[100][BUFFER_SIZE])
{
    char	*p	= buf;
    char	*q	= buf;
    int	number	= 0;

    while (1)
    {
        if ( p[0] == '\n' )
            break;

        if ( p[0] == ' '  )
            p++;
        else
        {
            q = p;
            number = 0;
            while( (q[0] != ' ') && (q[0] != '\n') )
            {
                number++;
                q++;
            }
            strncpy(arglist[*argcount], p, number + 1);
            arglist[*argcount][number] = '\0';
            *argcount = *argcount + 1;
            p = q;
        }
    }
}

void do_cmd(int argcount, char arglist[100][BUFFER_SIZE])
{
    int	flag = 0;
    int	how = 0;        /* 用于指示命令中是否含有>、<、|   */
    int	background = 0; /* 标识命令中是否有后台运行标识符& */
    int	status, status1;
    int	i;
    int	fd;
    char	*arg[argcount + 1];
    char	*argnext[argcount + 1];
    char	*file;
    pid_t	pid;

    /*将命令取出*/
    for (i = 0; i < argcount; i++)
    {
        arg[i] = (char *) arglist[i];
    }
    arg[argcount] = NULL;

    for (i = 0; i < lsh_num_builtins(); i++)
    {
        if(argcount == 0){
            return ;
        }
        else{
            if (strcmp(arg[0], builtin_str[i]) == 0)
            {
                return (*builtin_func[i])(arg);
            }
        }
    }

    status1 = search_cmd(arg);

    if(status1 == 1)
    {
        return ;
    }

    /*查看命令行是否有后台运行符*/
    for (i = 0; i < argcount; i++)
    {
        if (strncmp(arg[i], "&", 1) == 0)
        {
            if (i == argcount - 1)
            {
                background = 1;
                arg[argcount - 1] = NULL;
                break;
            }
            else
            {
                printf("wrong command\n");
                return;
            }
        }
    }

    for (i = 0; arg[i] != NULL; i++)
    {
        if (strcmp(arg[i], ">") == 0 )
        {
            flag++;
            how = out_redirect;
            if (arg[i + 1] == NULL)
                flag++;
        }
        if ( strcmp(arg[i], "<") == 0 )
        {
            flag++;
            how = in_redirect;
            if(i == 0)
                flag++;
        }
        if ( strcmp(arg[i], "|") == 0 )
        {
            flag++;
            how = have_pipe;
            if(arg[i + 1] == NULL)
                flag++;
            if(i == 0 )
                flag++;
        }
    }
    /* flag大于1，说明命令中含有多个> ,<,|符号，本程序是不支持这样的命令的
       或者命令格式不对，如"ls －l /tmp >" */
    if (flag > 1)
    {
        printf("wrong command\n");
        return;
    }

    if (how == out_redirect)    /*命令只含有一个输出重定向符号> */
    {
        for (i = 0; arg[i] != NULL; i++)
        {
            if (strcmp(arg[i], ">") == 0)
            {
                file   = arg[i + 1];
                arg[i] = NULL;
            }
        }
    }

    if (how == in_redirect)      /*命令只含有一个输入重定向符号< */
    {
        for (i = 0; arg[i] != NULL; i++)
        {
            if (strcmp (arg[i], "<") == 0)
            {
                file   = arg[i + 1];
                arg[i] = NULL;
            }
        }
    }

    if (how == have_pipe)    /* 命令只含有一个管道符号| */
    {
        /* 把管道符号后门的部分存入argnext中，管道后面的部分是一个可执行的shell命令 */
        for (i = 0; arg[i] != NULL; i++)
        {
            if (strcmp(arg[i], "|") == 0)
            {
                arg[i] = NULL;
                int j;
                for (j = i + 1; arg[j] != NULL; j++)
                {
                    argnext[j - i - 1] = arg[j];
                }
                argnext[j - i - 1] = arg[j];
                break;
            }
        }
    }

    if ( (pid = fork()) < 0 )
    {
        printf("fork error\n");
        return;
    }

    switch(how)
    {
    case 0:
        /* pid为0说明是子进程，在子进程中执行输入的命令 */
        /* 输入的命令中不含>、<和| */
        if (pid == 0)
        {
            if ( !(find_command(arg[0])) )
            {
                printf("%s : command not found\n", arg[0]);
                exit (0);
            }
            ex_cmd(arg);
            execvp(arg[0], arg);
            exit(0);
        }
        break;
    case 1:
        /* 输入的命令中含有输出重定向符> */
        if (pid == 0)
        {
            if ( !(find_command(arg[0])) )
            {
                printf("%s : command not found\n", arg[0]);
                exit(0);
            }
            fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0644);
            dup2(fd, 1);
            ex_cmd(arg);
            execvp(arg[0], arg);
            exit(0);
        }
        break;
    case 2:
        /* 输入的命令中含有输入重定向符< */
        if (pid == 0)
        {
            if ( !(find_command (arg[0])) )
            {
                printf("%s : command not found\n", arg[0]);
                exit(0);
            }
            fd = open(file, O_RDONLY);
            dup2(fd, 0);
            ex_cmd(arg);
            execvp(arg[0], arg);
            exit(0);
        }
        break;
    case 3:
        /* 输入的命令中含有管道符| */
        if(pid == 0)
        {
            int  pid2;
            int  status2;
            int  fd2;

            if ( (pid2 = fork()) < 0 )
            {
                printf("fork2 error\n");
                return;
            }
            else if (pid2 == 0)
            {
                if ( !(find_command(arg[0])) )
                {
                    printf("%s : command not found\n", arg[0]);
                    exit(0);
                }
                fd2 = open("./tmpfile",
                           O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd2, 1);
                ex_cmd(arg);
                execvp(arg[0], arg);
                exit(0);
            }

            if (waitpid(pid2, &status2, 0) == -1)
                printf("wait for child process error\n");

            if ( !(find_command(argnext[0])) )
            {
                printf("%s : command not found\n", argnext[0]);
                exit(0);
            }
            fd2 = open("./tmpfile", O_RDONLY);
            dup2(fd2, 0);
            ex_cmd(argnext);
            execvp (argnext[0], argnext);

            if ( remove("./tmpfile") )
                printf("remove error\n");
            exit(0);
        }
        break;
    default:
        break;
    }

    /* 若命令中有&，表示后台执行，父进程直接返回不等待子进程结束 */
    if ( background == 1 )
    {
        printf("[process id %d]\n", pid);
        return ;
    }

    /* 父进程等待子进程结束 */
    if (waitpid (pid, &status, 0) == -1)
        printf("wait for child process error\n");
}

/* 查找命令中的可执行程序 */
int find_command (char *command)
{
    DIR *dp;
    struct dirent   *dirp;
    char	*path[] = { "./", "/bin", "/usr/bin", NULL};

    /* 使当前目录下的程序可以被运行，如命令"./fork"可以被正确解释和执行 */
    if( strncmp(command, "./", 2) == 0 )
    {
        command = command + 2;
    }

    /* 分别在当前目录、/bin和/usr/bin目录查找要可执行程序 */
    int i = 0;
    while (path[i] != NULL)
    {
        if ( (dp = opendir(path[i]) ) == NULL)
            printf ("can not open /bin \n");
        while ( (dirp = readdir(dp)) != NULL)
        {
            if (strcmp(dirp->d_name, command) == 0)
            {
                closedir(dp);
                return 1;
            }
        }
        closedir (dp);
        i++;
    }
    return 0;
}

int ex_cmd(char **args)
{
    int i;
    char *exec_argv[BUFFER_SIZE];

    if(strcmp(args[0], "ex") == 0)
    {
        for(i = 1; args[i] != NULL; i++)
        {
            exec_argv[i - 1] = (char *)args[i];
        }
        execvp(args[1], exec_argv);
        exit(0);
    }
    return 0;
}

int search_cmd(char **args)
{
    if(strcmp(args[0], "search") == 0)
    {
        DIR *dir;
        struct dirent *ptr;
        char *q, *p, *f;
        dir = opendir("./");
        if(args[1][0] == '*')
        {
            while((ptr = readdir(dir)) != NULL)
            {
                p = ptr->d_name;
                f = args[1] + 1;
                q = strstr(p, f);
                if(q != NULL)
                {
                    if(strcmp(q, args[1] + 1) == 0)
                    {
                        printf("d_name : %s\n", ptr->d_name);
                    }
                }
            }
        }
        else
        {
            while((ptr = readdir(dir)) != NULL)
            {
                if(strcmp(ptr->d_name, args[1]) == 0)
                {
                    printf("d_name : %s\n", ptr->d_name);
                    break;
                }
            }
        }
        closedir(dir);
        return 1;
    }
    return 0;
}
