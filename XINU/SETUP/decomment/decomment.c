/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 *
 * @APPLE_LICENSE_HEADER_END@
 */
  
              
  
                                                                       
                           
   

#include <stdio.h>
#include <ctype.h>                       
#include <libc.h>

  
                          
   
typedef enum {
	IS_NORMAL,
	IS_SLASH,                                         
	IS_IN_COMMENT,                                   
	IS_STAR,                                          
	IS_IN_END_COMMENT                            
} input_state_t;

static void usage(char **argv);

int
main(int argc, char **argv)
{
	FILE *fp;
	char bufchar;
	input_state_t input_state = IS_NORMAL;
	int exit_code = 0;
	int remove_whitespace = 0;
	int arg;

	if (argc < 2) {
		usage(argv);
	}
	for (arg = 2; arg < argc; arg++) {
		switch (argv[arg][0]) {
		case 'r':
			remove_whitespace++;
			break;
		default:
			usage(argv);
		}
	}

	fp = fopen(argv[1], "r");
	if (!fp) {
		fprintf(stderr, "Error opening %s\n", argv[1]);
		perror("fopen");
		exit(1);
	}
	for (;;) {
		bufchar = getc_unlocked(fp);
		if (bufchar == EOF) {
			break;
		}

		switch (input_state) {
		case IS_NORMAL:
			if (bufchar == '/') {
				  
                                   
       
				input_state = IS_SLASH;
			} else {
				if (!(remove_whitespace && isspace(bufchar))) {
					putchar_unlocked(bufchar);
				}
			}
			break;

		case IS_SLASH:
			switch (bufchar) {
			case '*':
				  
                               
       
				input_state = IS_IN_COMMENT;
				break;

			case '/':
				  
                                         
       
				input_state = IS_IN_END_COMMENT;
				break;

			default:
				  
                                             
                                           
                                             
                    
       
				putchar_unlocked('/');
				if (!(remove_whitespace && isspace(bufchar))) {
					putchar_unlocked(bufchar);
				}
				input_state = IS_NORMAL;
				break;
			}
			break;

		case IS_IN_COMMENT:
			if (bufchar == '*') {
				  
                              
       
				input_state = IS_STAR;
			}
			break;


		case IS_STAR:
			switch (bufchar) {
			case '/':
				  
                             
       
				input_state = IS_NORMAL;
				break;

			case '*':
				  
                                            
                  
       
				break;

			default:
				  
                                             
       
				input_state = IS_IN_COMMENT;
				break;
			}
			break;

		case IS_IN_END_COMMENT:
			if (bufchar == '\n') {
				  
                                          
                   
       
				if (!remove_whitespace) {
					putchar_unlocked(bufchar);
				}
				input_state = IS_NORMAL;
			}
			break;
		}                         
	}                             

	  
         
    
	return exit_code;
}

static void
usage(char **argv)
{
	printf("usage: %s infile [r(emove whitespace)]\n", argv[0]);
	exit(1);
}
