/* DAIM Stage 1
 * 
 * Receive OpenFlow messages from the switch and send it to the controller
 * and vice-versa
 * 
 * Copyright (C) 2015
 * Pakawat Pupatwibul  PhD project
 * All rights reserved.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>

#define Controller_PORT 6633
#define XAPP_PORT 2000


// Variables for XApp
int app_sockfd = -1;
struct sockaddr_in app_addr;
int app_port = XAPP_PORT;

// Variables for the switch
int sw_sockfd = -1;
struct sockaddr_in sw_addr;
socklen_t sw_addl;

// Variables for remote controller
int con_sockfd = -1;
struct sockaddr_in con_addr;
int con_port = Controller_PORT;
struct hostent *con_server;

// Variables for XApp exit control
struct sigaction sigIntHandler;
pid_t cproc = -1;
int cstat;

int create_app_socket (void);
int create_controller_socket (void);
void close_sockets (void);

void exit_handler (int sig);
void child_exit_handler (int sig);

int main (int argc, char *argv[])
{
    fprintf (stdout, "XApp version 1\n");
    if (argc < 2) fprintf (stdout, "XApp port is 2000\n");
    else {
        app_port = atoi (argv[1]);
        fprintf (stdout, "XApp port is %d\n", app_port);
    }
    
    fprintf (stdout, "XApp: Creating XApp server socket...\n");
    if (create_app_socket () == -1)
    {
        fprintf (stderr, "XApp: Can not create server socket\nExiting\n");
        close_sockets ();
        exit (EXIT_FAILURE);
    }
    fprintf (stdout, "XApp: Established XApp server!\n");
    
    fprintf (stdout, "XApp: Creating controller socket...\n");
    if (create_controller_socket () == -1)
    {
        fprintf (stderr, "XApp: Can not create controller socket\nExiting\n");
        close_sockets ();
        exit (EXIT_FAILURE);
    }
    fprintf (stdout, "XApp: Established connection to the remote controller!\n");
    
    // Set main process exit handler
    memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
    sigIntHandler.sa_handler = &exit_handler;
    sigaction (SIGINT, &sigIntHandler, NULL);
    
    // Create a child process
    cproc = fork ();
    
    if (cproc == -1)
    {
        fprintf (stderr, "XApp: Can not fork child process\nExiting\n");
        close_sockets ();
        exit (EXIT_FAILURE);
    }
    else if (cproc == 0)
    {
        char buffer[10240];
        int buffer_len;
        int sw_write_len;
        
        // Set child process exit handler
        memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
        sigIntHandler.sa_handler = SIG_IGN;
        sigaction (SIGINT, &sigIntHandler, NULL);
        memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
        sigIntHandler.sa_handler = &child_exit_handler;
        sigaction (SIGTERM, &sigIntHandler, NULL);
        
        while (1)
        {
            memset (buffer, '\0', sizeof (buffer));
            buffer_len = 0;
            buffer_len = read (con_sockfd, buffer, sizeof (buffer));
            if (buffer_len < 0)
            {
                fprintf (stderr, "XApp: Can not read from the controller: %s\nExiting\n", strerror(errno));
                close_sockets ();
                _exit (EXIT_FAILURE);
            }
            else if (buffer_len > 0)
            {
                sw_write_len = write (sw_sockfd, buffer, buffer_len);
                if (sw_write_len < 0)
                {
                    fprintf (stderr, "XApp: Can not write to the switch: %s\nExiting\n", strerror(errno));
                    close_sockets ();
                    _exit (EXIT_FAILURE);
                }
            }
        }
        _exit (EXIT_SUCCESS);
    }
    else
    {
        char buffer[10240];
        int buffer_len;
        int con_write_len;
                
        while (1)
        {
            memset (buffer, '\0', sizeof (buffer));
            buffer_len = 0;
            buffer_len = read (sw_sockfd, buffer, sizeof (buffer));
            if (buffer_len < 0)
            {
                fprintf (stderr, "XApp: Can not read from the switch: %s\n", strerror(errno));
                fprintf (stdout, "XApp: Waiting for child process to exit\n");
                // Send terminate signal to the child process
                kill (cproc, SIGTERM);
                waitpid (cproc, &cstat, WUNTRACED | WCONTINUED);
                close_sockets ();
                exit (EXIT_FAILURE);
            }
            else if (buffer_len > 0)
            {
                con_write_len = write (con_sockfd, buffer, buffer_len);
                if (con_write_len < 0)
                {
                    fprintf (stderr, "XApp: Can not write to the controller: %s\n", strerror(errno));
                    fprintf (stdout, "XApp: Waiting for child process to exit\n");
                    // Send terminate signal to the child process
                    kill (cproc, SIGTERM);
                    waitpid (cproc, &cstat, WUNTRACED | WCONTINUED);
                    close_sockets ();
                    exit (EXIT_FAILURE);
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

int create_app_socket (void)
{
    // socket for server mode
    app_sockfd = socket (AF_INET, SOCK_STREAM, 0);

    if (app_sockfd < 0)
    {
        fprintf (stderr, "XApp: Error creating server socket: %s\n", strerror (errno));
        return -1;
    }
   
    // Define server address
    memset (&app_addr, '\0', sizeof (app_addr));
   
    app_addr.sin_family = AF_INET;
    app_addr.sin_addr.s_addr = INADDR_ANY;
    app_addr.sin_port = htons (app_port);
    
    // Bind server socket to the main process
    if (bind (app_sockfd, (struct sockaddr *) &app_addr, sizeof (struct sockaddr)) < 0)
    {
        fprintf (stderr, "XApp: Error binding server socket: %s\n", strerror (errno));
        return -1;
    }
    
    // Set clients listening queue size
    if (listen (app_sockfd, 10) == -1)
    {
        fprintf (stderr, "XApp: Error establishing listening: %s\n", strerror (errno));
        return -1;
    }
    
    // Accept connection from the switch
    sw_addl = sizeof (sw_addr);
    memset (&sw_addr, '\0', sizeof (sw_addr));
    sw_sockfd = accept (app_sockfd, (struct sockaddr *) &sw_addr, &sw_addl);
    if (sw_sockfd < 0)
    {
         fprintf (stderr, "XApp: Error accepting connection from switch: %s\n", strerror (errno));
         return -1;
    }
    
    return 0;
}

int create_controller_socket (void)
{
    con_sockfd = socket (AF_INET, SOCK_STREAM, 0);
    
    if (con_sockfd < 0)
    {
        fprintf (stderr, "XApp: can not create socket for the controller: %s\n", strerror (errno));
        return -1;
    }
    
    con_server = gethostbyname ("127.0.0.1");
    
    if (con_server == NULL)
    {
        fprintf (stderr, "XApp: can not get host address for the controller: %s\n", strerror (errno));
        return -1;
    }
    
    memset (&con_addr, '\0', sizeof (con_addr));
    
    // Define Address for the controller
    con_addr.sin_family = AF_INET;
    bcopy ((char *) con_server->h_addr, (char *) &con_addr.sin_addr.s_addr, con_server->h_length);
    con_addr.sin_port = htons (con_port);
    
    // Connect to the controller
    if (connect (con_sockfd, (struct sockaddr *) &con_addr, sizeof (con_addr)) < 0)
    {
        fprintf (stderr, "XApp: can not connect to the controller: %s\n", strerror (errno));
        return -1;
    }
    
    return 0;
}

void close_sockets (void)
{
    if (app_sockfd < 0) close (app_sockfd);
    if (sw_sockfd < 0) close (sw_sockfd);
    if (con_sockfd < 0) close (con_sockfd);
}

void child_exit_handler (int sig)
{
    close_sockets ();
    fprintf (stdout, "\nXApp: Child Caught TERM signal: %d\n", sig);
    _exit (EXIT_SUCCESS);        
}

void exit_handler (int sig)
{
    // Send termination signal to the child process
    kill (cproc, SIGTERM);
    close_sockets ();
    fprintf (stdout, "\nXApp: Caught close end signal, Ctrl + C: %d\n", sig);
    exit (EXIT_SUCCESS);
}
