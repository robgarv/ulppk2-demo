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
 * @file demosocketserver.c
 *
 * @brief A simple socket server implementation using the socketserver framework of ULPPK.
 *
 */
/*
 * demosocketserver.c
 *
 *  Created on: Dec 26, 2012
 *      Author: robgarv
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <cmdargs.h>
#include <democonfig.h>
#include <dqacc.h>
#include <process_control.h>
#include <socketserver.h>
#include <statemachine.h>
#include <ulppk_log.h>
#include <urlcoder.h>
#include <diagnostics.h>
#include <sysconfig.h>
#include <msgdeque.h>

static MSGCELL* xmtmsgcellp = NULL;		// send data to the server on this deque
static MSGCELL* recmsgcellp = NULL;		// receive data from the server on this deque

/**
 * The command line argument personality function. This simple
 * server has no arguments not already fielded by the socketserver.c
 * framework.
 * @param argc Argument count as passed to main.
 * @param argv Vector of command line argument strings
 * @param datap Pointer to custom application data.
 * @return 0 on success.
 */
int pf_register_cmdline(int argc, char* argv[], void* datap) {
	return 0;
}

/**
 * @brief Initialization personality function. On socket server initialization, we'll attach to the message
 * dequeue of the state machine process.
 *
 * @param datap Pointer to custom application data.
 * @return 0 on success.
 */
int pf_init_server(void* datap) {
	recmsgcellp = msgdeque_create_byte_stream("demo-socketserver", (S_IRWXU | S_IRWXG), (1024 * 4));
	xmtmsgcellp = msgdeque_attach("demo-server");
	if (NULL == xmtmsgcellp) {
		ULPPK_CRASH("Unable to attach to demoserver input deque");
	}
	return 0;
}

/**
 * @brief Main personality function for demosocketserver.
 *
 * Reads socket input and sends the received URL encoded event command to demoserver via
 * means of a message dequeue send.
 *
 * @param connfd The socket connection file descriptor opened by socketserver
 * @param datap Pointer to custom application data.
 * @return 0 on success.
 *
 */
int pf_demoserver(int connfd, void* datap)  {
	int retstatus = 0;
	ssize_t nread;
	static char buff[1024];
	LL_HEAD arglist;
	char* event;
	char* message;
	char* serialnumber;

	// TSTRACE("MPF Executes ... CONNECTION ESTABLISHED");

	memset(buff, 0, sizeof(buff));
	while ((nread = sio_readline(connfd, buff, sizeof(buff))) > 0) {
		fprintf(stdout, "LINE: %s\n", buff);
		fflush(stdout);

		// Send the entire encoded request (line of text) to the
		// message deque server/statemachine
		if (msgdeque_send_byte_stream(xmtmsgcellp, buff, strlen(buff))) {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Error sending to demoserver error code: [%d]", xmtmsgcellp->errcode);
		}
	}
	if (nread == 0) {
		fprintf(stdout, "EOF Detected\n");
	} else if (nread < 0 ) {
		fprintf(stdout, "Read error: errno = %d | %s\n", errno, strerror(errno));
	}
	fflush(stdout);
	return 0;
}

/**
 * @brief Register server personality functions.
 *
 * @param ssrvrhp Pointer to socket server handle structure
 */
void register_server_pf(SSRVR_HANDLE* ssrvrhp) {
	ssrvr_register_clpf(ssrvrhp, pf_register_cmdline);
	ssrvr_register_ipf(ssrvrhp, pf_init_server);
	ssrvr_register_mpf(ssrvrhp, pf_demoserver);
}

int main(int argc, char* argv[]) {
	int tstate;
	char* label;
	SSRVR_HANDLE* ssrvrhp;


	printf("LOG_LOCAL0 = %d LOG_LOCAL1 = %d\n", LOG_LOCAL0, LOG_LOCAL1);

	// Set up logging.
	sysconfig_set_logging(ULPPK_LOGDEST_SYSLOG, "demosocketserver", LOG_PID, LOG_LOCAL0);

	// Parse the INI file
	sysconfig_parse_inifile("demosocketserver");

	// Log start of application
	log_app_start(argc, argv);

	// Create a new socket server handle.
	ssrvrhp = ssrvr_new();

	// Now register the "personality functions" of the socket
	// server.
	register_server_pf(ssrvrhp);

	// Start the socket server. Now we start accepting
	// connections.
	ssrvr_start(ssrvrhp, NULL, argc, argv);

	return 0;
}
