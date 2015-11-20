#include "ani.h"

struct sigaction sigIntHandler;

int main (int argc, char *argv[])
{
    int opt;
    const char *short_options = "p:qcvh";
    const struct option long_options [] = {
        {"port", 1, NULL, 'p'},
        {"quite", 0, NULL, 'q'},
        {"cbench", 0, NULL, 'c'},
        {"version", 0, NULL, 'v'},
        {"help", 0, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };
    
    while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                ani_port = atoi (optarg);
                break;
            case 'q':
                verbose = 0;
                break;
            case 'c':
                cbench = 1;
                break;
            case 'h':
                cout << "Usage:\n" << "\t-p, --port controller port e.g. -p 2000" << endl
                     << "\t-q, --quite turn off verbose mode" << endl
                     << "\t-c, --cbench Cbench benchmarking mode" << endl
                     << "\t-v, --version show ANI version" << endl
                     << "\t-h, --help show this help" << endl;
                exit (EXIT_SUCCESS);
            case 'v':
                cout << "ANI OpenFlow Controller" << endl
                     << "Compiled on " << __DATE__ << ' ' << __TIME__ << endl
                     << "version " << ANI_VERSION << endl;
                exit (EXIT_SUCCESS);
            default:
                break;
        }
    }
    
    if (atexit (exit_func) != 0) {
        cerr << "Error: can not set exit function" << endl;
        exit (EXIT_FAILURE);
    }
    
    cout << "Info: creating ANI socket..." << endl;
    
    if (create_ani_socket () == -1) {
        cerr << "Error: can not create ANI socket" << endl << "Exiting..." << endl;
        close_sockets ();
        exit (EXIT_FAILURE);
    }
    cout << "Info: established ANI server!" << endl;

    cout << "Info: waiting for the switch connection..." << endl;
    
    if (create_switch_socket () == -1) {
        cerr << "Error: can not connect to the switch" << endl << "Exiting..." << endl;
        close_sockets ();
        exit (EXIT_FAILURE);
    }
    cout << "Info: connected to the switch!" << endl;
    
    memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
    sigIntHandler.sa_handler = SIG_IGN;
    sigaction (SIGINT, &sigIntHandler, NULL);
    
    memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
    sigIntHandler.sa_handler = &exit_handler;
    sigaction (SIGINT, &sigIntHandler, NULL);

    memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
    sigIntHandler.sa_handler = &exit_handler;
    sigaction (SIGTERM, &sigIntHandler, NULL);
    
    hosts.set_object_size (sizeof(struct switch_host));
    ports.set_object_size (sizeof(struct switch_port));
    links.set_object_size (sizeof(struct switch_link));
    sw_replys.set_object_size(sizeof(struct switch_reply));
    
    while (1) {
        if (communicate_with_switch () == -1) {
            cerr << "Error: can not communicate with the switch" << endl;
            exit (EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

void exit_handler (int sig)
{
    cout << endl << "Info: caught signal, Ctrl + C: " << sig << endl;
    exit (EXIT_SUCCESS);
}

void exit_func ()
{
    pthread_join (node_send_thread, NULL);
    close_sockets ();
    cout << "Total host entries freed: " << hosts.free_list() << endl;
    cout << "Total port entries freed: " << ports.free_list() << endl;
    cout << "Total links entries freed: " << links.free_list() << endl;
    cout << "Total reply entries freed: " << sw_replys.free_list() << endl;
}