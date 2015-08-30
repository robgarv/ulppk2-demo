/*
 *****************************************************************

<GPL>

Copyright: © 2001-2015 Robert C Garvey

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
X-Comment: On Debian systems, the complete text of the GNU General Public
 License can be found in `/usr/share/common-licenses/GPL-3'.

</GPL>
*********************************************************************
*/

/**
 * @file demoserver.c
 *
 * The demo server accepts socket connections (see demosocketclient.c)
 * and receives "events". It demostrates use of
 * <ul>
 * <li>
 * cmdargs.c -- command argument processing (see demosocketclient.c for
 *   more elaborate use.
 * </li>
 * <li>
 * statemachine.c -- an engine for implementing a finite state machine
 *   described with ward/mellor notation
 * </li>
 * <li>
 * logging
 * </li>
 * </ul>
 *
 * Command line arguments and switches:
 * <ol>
 * <li>-h --- help</li>
 * <li>-p < port number > to define listen port</li>
 * <li>-l < processing delay in msec > ... for demonstrating flow control features >/li>
 * </ol>
 */
/*
 *  Created on: Oct 29, 2012
 *      Author: robgarv
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cmdargs.h>
#include <democonfig.h>
#include <dqacc.h>
#include <statemachine.h>
#include <ulppk_log.h>
#include <urlcoder.h>
#include <diagnostics.h>
#include <sysconfig.h>
#include <msgdeque.h>

extern FILE* stdout;
FILE* fdemolog;

int server_latency;
MSGCELL* recmsgcellp = NULL;

// Declare the machine. We could allocate space
// from the heap but this makes debugging easier.
SM_MACHINE sm_machine;
SM_MACHINE* machinep = NULL;

// Declare the state table.
SM_STATE_TABLE_DEF state_table;

// event names. Define using the SMDEFNAME macro
SMDEFNAME(DEMO_EVENT1)
SMDEFNAME(DEMO_EVENT2)
SMDEFNAME(DEMO_EVENT3)
SMDEFNAME(DEMO_EVENT4)

// Action list names.
SMDEFNAME(DEMO_AL1)
SMDEFNAME(DEMO_AL2)
SMDEFNAME(DEMO_AL3)
SMDEFNAME(DEMO_ALSHUTDOWN)

// Action handler names
SMDEFNAME(DEMO_AH1)
SMDEFNAME(DEMO_AH2)
SMDEFNAME(DEMO_AH3)
SMDEFNAME(DEMO_AHSHUTDOWN)

// State names
SMDEFNAME(DEMO_STATE1)
SMDEFNAME(DEMO_STATE2)
SMDEFNAME(DEMO_STATE3)
SMDEFNAME(DEMO_STATE_TERMINATED)

// Transition names
SMDEFNAME(DEMO_TX1)
SMDEFNAME(DEMO_TX2)
SMDEFNAME(DEMO_TX3)

// Forward declarations for state machine action handlers
SM_EVENT_HANDLE demo_actionhandler1(SM_MACHINE* machinep, void* datap);
SM_EVENT_HANDLE demo_actionhandler2(SM_MACHINE* machinep, void* datap);
SM_EVENT_HANDLE demo_actionhandler3(SM_MACHINE* machinep, void* datap);
SM_EVENT_HANDLE demo_actionhandler_shutdown(SM_MACHINE* machinep, void* datap);

/**
 * @brief Action handler 1 will return EV_NULL_HANDLE, the null
 * event. No transition will be triggered.
 *
 * @param machinep Pointer to state machine data structure
 * @param datap Pointer to data transmitted by the URL encoded event (the message)
 * @return The event EV_NULL_HANDLE
 */
SM_EVENT_HANDLE demo_actionhandler1(SM_MACHINE* machinep, void* datap) {
	fprintf(fdemolog, "ActionHandler1: from state: %s message [%s] return EV_NULL\n", sm_curr_state(machinep), (char*)datap);
	return EV_NULL_HANDLE;
}

/**
 * @brief Action handler 2 will return EV_NULL_HANDLE, the null
 * event. No transition will be triggered.
 *
 * @param machinep Pointer to state machine data structure
 * @param datap Pointer to data transmitted by the URL encoded event (the message)
 * @return The event EV_NULL_HANDLE
 */

SM_EVENT_HANDLE demo_actionhandler2(SM_MACHINE* machinep, void* datap) {
	fprintf(fdemolog, "ActionHandler2: from state: %s message [%s] return EV_NULL\n", sm_curr_state(machinep), (char*)datap);
	return EV_NULL_HANDLE;
}

/**
 * @brief Action handler 3 will return DEMO_EVENT2, which should force
 * a transition from DEMO_STATE2 to DEMO_STATE1
 *
 * @param machinep Pointer to state machine data structure
 * @param datap Pointer to data transmitted by the URL encoded event (the message)
 * @return The event DEMO_EVENT2
 */
SM_EVENT_HANDLE demo_actionhandler3(SM_MACHINE* machinep, void* datap) {
	fprintf(fdemolog, "ActionHandler3: from state: %s message: [%s] return event DEMO_EVENT2\n", sm_curr_state(machinep), (char*)datap);
	return sm_event_handle(machinep, DEMO_EVENT2);
}

SM_EVENT_HANDLE demo_actionhandler_shutdown(SM_MACHINE* machinep, void* datap) {
	fprintf(fdemolog, "ActionHandler SHUTDOWN ... exiting the demoserver from state %s\n", sm_curr_state(machinep));
	return EV_NULL_HANDLE;
}

/**
 * register command line arguments.
 *
 * @param argc Argument count as passed to main
 * @param argv The argument string vector as passed to main
 * @return Non-zero on error.
 */
int register_cmdline(int argc, char* argv[]) {
	int status;

	// cmdarg_init and cmdarg_parse are handled by the
	// socket server shroud ssrvr_start.
	// We'll just add the ones peculiar to our purpose.

	// Define the simulated processing latency (in msec). Default is 0.
	status |= cmdarg_register_option("l", "latency", CA_DEFAULT_ARG,
			"Simulated processing latency in msec (default is 0)", "0", NULL);
	return status;
}

/**
 * @brief Statemachine initialization.
 *
 * Here, we initialize application
 * specific data and stuff. In this case, we're going to define a
 * simple state machine.
 */
int init_server() {
	int retval;

	// Set our output stream
	fdemolog = stdout;

	// Get a new state machine
	machinep = sm_new_machine(&sm_machine, &state_table, "demomachine");

	// Register stock events and definitions
	sm_register_stock_defs(machinep);

	// Define the events.
	sm_register_event(machinep, DEMO_EVENT1);
	sm_register_event(machinep, DEMO_EVENT2);
	sm_register_event(machinep, DEMO_EVENT3);
	sm_register_event(machinep, DEMO_EVENT4);

	// Define the action lists.

	sm_register_action_list(machinep, DEMO_AL1);
	sm_register_action_list(machinep, DEMO_AL2);
	sm_register_action_list(machinep, DEMO_AL3);
	sm_register_action_list(machinep, DEMO_ALSHUTDOWN);

	// Now register the action handlers
	// Action list 1: Just calls demo_actionhandler1
	sm_register_action(machinep, DEMO_AL1, DEMO_AH1, demo_actionhandler1);

	// Action List 2: Calls demo_actionhandler_2 and 3
	sm_register_action(machinep, DEMO_AL2, DEMO_AH2, demo_actionhandler2);
	sm_register_action(machinep, DEMO_AL2, DEMO_AH3, demo_actionhandler3);

	// Action list 3: Just calls demo_actionhandler3
	sm_register_action(machinep, DEMO_AL3, DEMO_AH3, demo_actionhandler3);

	// This action list is used by the "global transition" for shutdown.
	// WE could use any action list for that purpose, in principal.
	sm_register_action(machinep, DEMO_ALSHUTDOWN, DEMO_AHSHUTDOWN, demo_actionhandler_shutdown);

	// Define the machine states
	sm_register_state(machinep, DEMO_STATE1);
	sm_register_state(machinep, DEMO_STATE2);
	sm_register_state(machinep, DEMO_STATE3);
	sm_register_state(machinep, DEMO_STATE_TERMINATED);

	// Now register the state transitions
	sm_register_transition(machinep, DEMO_STATE1, DEMO_STATE2, DEMO_EVENT1, DEMO_AL1);
	sm_register_transition(machinep, DEMO_STATE1, DEMO_STATE3, DEMO_EVENT3, DEMO_AL2);
	sm_register_transition(machinep, DEMO_STATE2, DEMO_STATE1, DEMO_EVENT2, DEMO_AL2);
	sm_register_transition(machinep, DEMO_STATE2, DEMO_STATE3, DEMO_EVENT1, DEMO_AL1);
	sm_register_transition(machinep, DEMO_STATE3, DEMO_STATE1, DEMO_EVENT1, DEMO_AL1);
	sm_register_transition(machinep, DEMO_STATE3, DEMO_STATE2, DEMO_EVENT3, DEMO_AL3);
	sm_register_transition(machinep, DEMO_STATE3, DEMO_STATE2, DEMO_EVENT2, DEMO_AL2);

	// here's a global transition. When DEMO_EVENT4 is detected from any state, transition
	// to shut down and transition to state DEMO_TERMINATED (dead) state.

	sm_register_global_transition(machinep, DEMO_STATE_TERMINATED, DEMO_EVENT4, DEMO_ALSHUTDOWN);

	// Mark the state machine definition as being complete
	sm_set_definition_complete(machinep);

	// Now set up the input message deque
	recmsgcellp = msgdeque_create_byte_stream("demo-server", (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP), (1024*10));
	if (NULL == recmsgcellp) {
		ULPPK_CRASH("Unable to create stream message deque: demo-server");
	}
	return 0;
}

/**
 * @brief Convenience function for accessing URL argument name/value pairs
 *
 * @param listp Pointer to linked list of name/value pairs.
 * @param namep The name of the item of interest.
 * @return Pointer to the value string retrieved.  Note that
 * the memory address returned is still on the linked list
 * and that the caller does not have the authority to
 * return this memory to the heap.
 *
 */
char* get_urlarg(PLL_HEAD listp, char* namep) {
	char* valp;

	valp = url_get_arg_value(listp, namep);
	if (valp == NULL) {
		APP_ERR(stderr, "Error retrieving URL parameter named %s", namep);
	}

	// Safe to return to valp because it points into the buffers
	// stored in linked list listp
	return valp;
}

/**
 * @brief Loop obtains requests and pushes them into the state machine
 * as events.
 */
int demoserver()  {
	int retstatus = 0;
	ssize_t nread;
	char* buff;
	LL_HEAD arglist;
	char* event;
	char* message;
	char* serialnumber;
	size_t bytes_received;

	// TSTRACE("MPF Executes ... CONNECTION ESTABLISHED");

	while (1) {
		buff = msgdeque_rec_byte_stream(recmsgcellp, &bytes_received);
		if (NULL == buff) {
			ULPPK_LOG(ULPPK_LOG_WARN, "Received NULL data on input queue");
		} else {
			fprintf(stdout, "LINE [bytes = %u]: %s\n", (unsigned int)bytes_received, buff);
			fflush(stdout);
		}

		// Decode the URL encoded arguments
		url_decode_arguments(&arglist, buff);
		event = get_urlarg(&arglist, "event");
		message = get_urlarg(&arglist, "message");
		serialnumber = get_urlarg(&arglist, "serialnumber");
		free(buff);
		// Pass the event to the state machine. Data is the incoming message
		sm_transition(machinep, event, message);
	}
	return 0;
}

int main(int argc, char* argv[]) {
	int tstate;
	char* label;

	printf("LOG_LOCAL0 = %d LOG_LOCAL1 = %d\n", LOG_LOCAL0, LOG_LOCAL1);
	sysconfig_set_logging(ULPPK_LOGDEST_SYSLOG, "demoserver", LOG_PID, LOG_LOCAL0);
	app_init("demoserver", argc, argv);
	register_cmdline(argc, argv);
	init_server();
	demoserver();
	return 0;
}
