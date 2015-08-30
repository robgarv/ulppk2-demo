/*
 *****************************************************************

<GPL>

Copyright: © 2001-2012 Robert C Garvey

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
 * @mainpage
 *
 * @author Rob Garvey
 *
 * <h1>Introduction</h1>
 * The 'ulppk2-demo package is provides a simple example
 * of a system of communicating processes that are built on
 * ulppk2 facilities.
 * <p>
 * demosocketclient is a command line program that connects
 * to demosocketserver and issues event messages. The
 * demosocketserver delivers these event messages to
 * demoserver using a memory mapped deque managed as a message
 * queue. The demoserver implements a simple statemachine.
 * Receipt of event messages causes the state machine to
 * take actions and transition to a new state.
 * <p>
 * <h1>Defined Event Messages</h1>
 * <ul>
 * <li>DEMO_EVENT1</li>
 * <li>DEMO_EVENT2</li>
 * <li>DEMO_EVENT3</li>
 * <li>DEMO_EVENT4 -- the shutdown command shutsdown demoserver</li>
 * </ul>
 *
 * <h1>Running the Demo</h1>
 *
 * From a terminal:
 *
 * demoserver
 * demosocketserver
 *
 * Then issue event commands like
 *
 * demosocketclient -e DEMO_EVENT1 -m "This is a random message"
 */

/*
 * democlient.c
 *
 *  Created on: Oct 30, 2012
 *      Author: robgarv
 */

/**
 * @file demosocketclient.c
 * @brief Client program for sending formatted events to demosocketserver.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmdargs.h>
#include <socketio.h>
#include <urlcoder.h>
#include <democonfig.c>
#include <ulppk_log.h>
#include <sysconfig.h>


char hostname[128];
char event[64];
char message[256];
int npackets;
int sockfd;

/**
 * @brief Command argument registration/definition
 *
 * register_cmdline is where you define the syntax of your command
 * line arguments. For this simple client, we have  the following
 * options:
 *
 * -H --host : hostname or IP of target server. Default is localhost
 * -n --packet-count: packet count to send (0 = infinite).
 * -e --event: event to send. The events are strings identifying the event (like "myevent1")
 * 		events carry a message string.
 * -m -- message: message to send along with the event
 *
 * @return Returns non-zero on error.
 */
static int register_cmdline() {
	int status = 0;


	// Define the help menu switch
	status |= cmdarg_register_option("h", "help", CA_SWITCH, "Get help on this program", NULL, NULL);

	// Define the target (server)host name argument. If not provided, the default of
	// localhost will be enforced.
	status |= cmdarg_register_option("H","hostname", CA_DEFAULT_ARG,
			"Hostname of target (default is localhost)", "localhost", NULL);

	// Define the target port.
	status |= cmdarg_register_option("p","port", CA_DEFAULT_ARG,
			"Hostname of target (default is 49152)", "49152", NULL);

	// Define the packet count argument. If 0, the client will continuously resend the
	// packet until the client is killed. If not provided a default packet
	// count of 1 is enforced.
	status |= cmdarg_register_option("n", "packet-count", CA_DEFAULT_ARG,
			"Number of packets to send (0 => infite, default is 1", "1", "H");

	// Define the event argument. This is required, and a value must be provided.
	status |= cmdarg_register_option("e","event",CA_REQUIRED_ARG, "Event identifier string (required)", NULL, "H");

	// Define the message argument. If not provided, a default value of
	// "eventmessage=demosocketclient" will be sent.
	status |= cmdarg_register_option("m", "message", CA_DEFAULT_ARG,
			"Event message string (default is \"eventmessage=demosocketclient)\"", NULL, "H" );

	return status;
}


/**
 * @brief Format an event. The formatted  event is written to the buffer.
 *
 * Events are written as URL encoded strings.
 * @param buffer Pointer to memory area to receive formatted event string
 * @param buffsize Size of the buffer in bytes
 * @param serialnumber Event serial number.
 * @param event Event name string
 * @param message Message to include with the event.
 * @return Pointer to the formatted event. (Same as input buffer.)
 */
char* fmt_event(char* buffer, size_t buffsize, int serialnumber, char* event, char* message) {
	char *evp;
	char* urlargs;
	char snbuff[16];

	if (buffsize < (strlen(event) + 6 + strlen(message) + 9 + 12 + 3)) {
		fprintf(stderr, "Increase eventbuff size in event_generator ... aborting\n");
		exit(1);
	}
	// sprintf(buffer, "event=\"%s\"&message=\"%s\"&serialnumber=\"%d\"\n", event, message, serialnumber);
	sprintf(snbuff, "%d", serialnumber);
	urlargs = url_encode_arguments(NULL, "event", event);
	urlargs = url_encode_arguments(urlargs, "message", message);
	urlargs = url_encode_arguments(urlargs, "serialnumber", snbuff);
	strncpy(buffer, urlargs, buffsize);
	free(urlargs);
	evp = buffer;
	return evp;
}

/**
 * @brief Generates and sends one of more events to demosocketserver
 *
 * This function fires URL encoded event strings at the demosocketserver.
 * The npackets parameters allows the transmission of copies of the
 * event at high rates of speed. Good for bench marking.
 *
 * @param hostname IP or hostname of the destination host
 * @param event Pointer to event name string.
 * @param message Pointer to message string
 * @param npackets Number of encoded packets to send. Useful for
 * firing a stream of copies of the same event at the demosocketserver.
 *
 */
int event_generator(char* hostname, char* event, char* message, int npackets) {
	char eventbuff[256];
	int i;
	int eventlen;

	for (i=0; i < npackets; i++) {
		fmt_event(eventbuff, sizeof(eventbuff), i, event, message);
		strcat(eventbuff, "\n");
		eventlen = strlen(eventbuff);
		sio_writen(sockfd, eventbuff, eventlen);
	}
	return 0;
}

/**
 * @brief Main program
 *
 */
int main(int argc, char* argv[]) {
	int status;
	int helpflag;
	char* hostnamep;
	char* eventp;
	char* messagep;
	int port_number;
	int exit_status;

	sysconfig_set_logging(ULPPK_LOGDEST_ALL, "demosocketclient", LOG_PID, LOG_LOCAL1);
	app_init("demosocketclient", argc, argv);

	cmdarg_init(argc, argv);
	status = register_cmdline(argc, argv);
	if (status) {
		fprintf(stdout, "Registration error was reported! Parsing should cause abort\n");
	} else {
		fprintf(stdout, "Registration successful\n");
	}

	if (cmdarg_parse(argc, argv)) {
		cmdarg_show_help(NULL);
		return 1;
	}

	cmdarg_print_args(NULL);

	helpflag = cmdarg_fetch_switch(NULL, "h");
	if (helpflag) {
		cmdarg_show_help(NULL);
		return 1;
	}

	// Fetch values from command line arguments.
	cmdarg_load_string(hostname, sizeof(hostname), NULL, "H");
	port_number = cmdarg_fetch_int(NULL, "p");
	cmdarg_load_string(event, sizeof(event), NULL, "e");
	cmdarg_load_string(message, sizeof(message), NULL, "m");
	npackets = cmdarg_fetch_int(NULL, "n");

	// Connect to the server
	sockfd = sio_connectbyhostname(hostname, port_number);
	if (sockfd < 0) {
		fprintf(stderr, "Connection attempt to host %s: %d fails!\n", hostname, port_number);
		exit(1);
	}

	// Now call the event generator
	exit_status = event_generator(hostname, event, message, npackets);

	return exit_status;
}
