/*
 * democonfig.c
 *
 *  Created on: Oct 29, 2012
 *      Author: robgarv
 */

#include <stdio.h>

#include <sysconfig.h>
#include <ifile.h>
#include <ulppk-properties.h>

/*
 * Convenience function for setting up an application.
 * Don't use for daemons ... presumes console
 */
void app_init(char* appname, int argc, char* argv[]) {

	// Read environment variables. Using
	// that info, open and parse the ini file.
	sysconfig_parse_inifile(appname);

	log_app_start(argc, argv);


}
