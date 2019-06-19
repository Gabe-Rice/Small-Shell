/*****************************************************************************
 *
 * Gabriel Rice
 * CS344 winter Term
 * smallsh.c
 *
 * This program is a small version of a shell environment.
 * Functionality will include: processing commands, handling signals, 
 * stdin/stdout redirection, foreground and background processes.
 * It will support three built in commands: exit, cd and status.
 *
 * Note: I apologize for the "through composed" nature of this program.
 * I fully intended on breaking it up into functions, however, I 
 * unfortunately ran out of time.
 *
 * To compile: $ gcc smallsh.c -o smallsh
 * then simply execute smallsh:  ./smallsh
 *
 * To run with the grading script:  $ p3testscript 2>&1 works fine.
 * I have also tested sending it to file: $ p3testscript > mytestresults 2>&1
 * It works as well, however there is a bit of a delay for the sleep 
 * proesses.
 *
 ****************************************************************************/