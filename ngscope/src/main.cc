#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>

#include <string>

#include "srsran/common/crash_handler.h"
#include "srsran/srsran.h"

#include "ngscope/hdr/main.h"
#include "ngscope/hdr/dciLib/radio.h"
#include "ngscope/hdr/dciLib/task_scheduler.h"
#include "ngscope/hdr/dciLib/dci_decoder.h"
#include "ngscope/hdr/dciLib/load_config.h"
#include "ngscope/hdr/dciLib/ngscope_main.h"
// #include "ngscope/hdr/dciLib/asn_decoder.h"


std::string DEFAULT_CELLCFG_OUTPUT = "cell_cfg"; // global variable, default cell configuration output file
std::string DEFAULT_SIB_OUTPUT = "decoded_sibs"; // global variable, default sib output file
std::string DEFAULT_DCI_OUTPUT = "dci_output"; // global variable, default dci output file

bool go_exit = false; // global variable for signaling
bool have_sib1 = false; // global variable for sib1 decoding
bool have_sib2 = false; // global variable for sib2 decoding


/*********************************************
 * Function name: sig_int_handler
 * Return value type: void
 * Description: handle signal to modify the 
 *     global variable "go_exit".
 * Author: PAWS (https://paws.princeton.edu/)
*********************************************/
void sig_int_handler(int signo)
{
  printf("SIGINT received. Exiting...\n");

  if (signo == SIGINT) {
    go_exit = true;
  } else if (signo == SIGSEGV) {
    exit(1);
  }
}


/*********************************************
 * Function name: print_help
 * Return value type: void
 * Description: print help info for command 
 *     line inputs.
 * Author: PAWS (https://paws.princeton.edu/)
*********************************************/
void print_help()
{
  printf("NG-Scope usage: ngscope [OPTIONS]\n");
  printf("  -c <Config File>\t\t[Mandatory] NG-Scope configuration file.\n");
  printf("  -b <Cell Basic Configuration Output File>\t\t[Optional] Output file where the basic cell configuration information will be stored.\n");
  printf("  -s <SIB Output File>\t\t[Optional] Ouput file where the decoded SIB messages will be stored.\n");
  printf("  -o <DCI Output Folder>\t[Optional] Ouput folder where DCI logs will be stored.\n");
  printf("  -h\t\t\t\t[Optional] Show this menu.\n");
}


/*********************************************
 * Function name: main
 * Return value type: int
 * Description: the main function of ngscope,
 *     which takes command line inputs.
 * Author: PAWS (https://paws.princeton.edu/)
*********************************************/
int main(int argc, char** argv)
{
    ngscope_config_t config;
    int c;
    /* Variables tahtw ill hold the command line arguments */
    char config_path[64] = {};
    char cellcfg_path[64] = {};
    char sib_path[64] = {};
    char out_path[64] = {};
    std::string argvstr;

    /* Parsing command line arguments */
    while ((c = getopt (argc, argv, "c:s:b:o:h")) != -1) {
      switch (c) {
        case 'c':
          argvstr = optarg;
          // strcpy(config_path, optarg);
          strcpy(config_path, argvstr.c_str());
          // config_path = argvstr.c_str();
          break;
        case 'b':
          // strcpy(cellcfg_path, optarg);
          // cellcfg_path = optarg;
          argvstr = optarg;
          strcpy(cellcfg_path, argvstr.c_str());
          break;
        case 's':
          // strcpy(sib_path, optarg);
          // sib_path = optarg;
          argvstr = optarg;
          strcpy(sib_path, argvstr.c_str());
          break;
        case 'o':
          // strcpy(out_path, optarg);
          // out_path = optarg;
          argvstr = optarg;
          strcpy(out_path, argvstr.c_str());
          break;
        case 'h':
          print_help();
          return 0;
        case '?':
          if (optopt == 'c') {
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          }
          if (optopt == 'b') {
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          }
          if (optopt == 's') {
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          }
          if (optopt == 'o') {
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          } else {
            print_help();
            return 1;
          }
        default:
          print_help();
          return 1;
        }
    }
    /* Check that the config file has been provided */
    if(config_path == NULL) {
      print_help();
      return 1;
    }
    printf("Configuration file: %s\n", config_path);
    /* Check SIB output */
    if(!strcmp(sib_path, "")) {
      strcpy(sib_path, "sib_output");
      // sib_path = DEFAULT_SIB_OUTPUT;
      printf("SIB output file not specified (using '%s')\n", sib_path);
    } else {
      printf("Decoded SIB file: %s\n", sib_path);
    }
    /* Check DCI output */
    if(!strcmp(out_path, "")) {
      strcpy(out_path, "dci_output");
      // out_path = DEFAULT_DCI_OUTPUT;
      printf("DCI logs folder not specified (using '%s')\n", out_path);
    } else {
      printf("DCI logs folder: %s\n", out_path);
    }

    /* Signal handlers */
    srsran_debug_handle_crash(argc, argv);
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
    signal(SIGINT, sig_int_handler);


    /* Load the configurations */
    ngscope_read_config(&config, config_path);
    /* Set DCI logs output folder path  */
    strcpy(config.dci_logs_path, out_path);
    strcpy(config.sib_logs_path, sib_path);
    // config.dci_logs_path = out_path;
    // config.sib_logs_path = sib_path;

    ngscope_main(&config);
    return 1;
}