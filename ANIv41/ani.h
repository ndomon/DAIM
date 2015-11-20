/* 
 * File:   ani.h
 * Author: imam
 *
 * Created on 25 July 2015, 8:19 AM
 */

#ifndef ANI_H
#define	ANI_H

#include <getopt.h>

#include <signal.h>

#include "con.h"

#define ANI_VERSION 0x04

using namespace std;

extern void exit_handler (int sig);
extern void exit_func ();

#endif	/* ANI_H */

