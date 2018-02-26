/*
 * This file is part of Goldilock.
 *
 * Goldilock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License.
 *
 * Goldilock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Goldilock.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Inspiration taken from Wiegand test v0.1 by Ben Kent 11/08/2012
 *
 * Based on an Arduino sketch by Daniel Smith: www.pagemac.com
 * Depends on the wiringPi library by Gordon Henterson:
 * https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 * The Wiegand interface has two data lines, DATA0 and DATA1.  These lines are
 * normally held high at 5V.  When a 0 is sent, DATA0 drops to 0V for a few us.
 * When a 1 is sent, DATA1 drops to 0V for a few us. There are a few ms between
 * the pulses.
 *   *************
 *   * IMPORTANT *
 *   *************
 *   The Raspberry Pi GPIO pins are 3.3V, NOT 5V. Please take appropriate
 *   precautions to bring the 5V Data 0 and Data 1 voltges down. I used a 330
 *   ohm resistor and 3V3 Zenner diode for each connection. FAILURE TO DO THIS
 *   WILL PROBABLY BLOW UP THE RASPBERRY PI!
 *
 * The wiegand reader should be powered from a separate 12V supply. Connect the
 * green wire (DATA0) to the Raspberry Pi GPIO 0(SDA) pin, and the white wire
 * (DATA1) to GPIO 1 (SCL).
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <mysql.h>
#include <time.h>

#include "parse.h"

#define RELAY_SIG 0
#define D0_PIN 2
#define D1_PIN 3

#define MAX_BITS 100                  // max number of bits
#define WIEGAND_WAIT_TIME 3000000

static unsigned char databits[MAX_BITS];    // stores all of the data bits
static unsigned char bitCount;              // bits currently captured
static unsigned int flagDone;               // low when data is being captured

static unsigned long wiegand_counter;

static char locationID[2];          // id of location
static char mysqlIP[16];            // ip of the mysql server
static char mysqlDB[16];            // db to use
static char mysqlUser[16];          // username of the mysql server
static char mysqlPass[16];          // password of the mysql server

MYSQL* db_connect(char*, char*, char*, char*);
MYSQL_RES* db_query(MYSQL*, char*);

void writeLog();

void data0_handler() {
	// Explicitly set databit to 0. This instruction could
	// be omitted since the array is zeroed.
	databits[bitCount] = 0;

	bitCount++;
	flagDone = 0;
	wiegand_counter = WIEGAND_WAIT_TIME;
}

void data1_handler() {
	databits[bitCount] = 1;

	bitCount++;
	flagDone = 0;
	wiegand_counter = WIEGAND_WAIT_TIME;
}

void setup(void)
{
	wiringPiSetup () ;
	pinMode(RELAY_SIG, OUTPUT);
	pinMode(D0_PIN, INPUT);
	pinMode(D1_PIN, INPUT);

	wiegand_counter = 0;
	bitCount = 0;
	flagDone = 0;

	// Fire off our interrupt handler
	wiringPiISR(D0_PIN, INT_EDGE_FALLING, &data0_handler);
	wiringPiISR(D1_PIN, INT_EDGE_FALLING, &data1_handler);
}

void getConfig()
{
	FILE *fr;

	fr = fopen("/etc/wiegand/wiegand.conf","r");

	if(fr == NULL)
	{
		fprintf(stderr, "Could not read configuration file\n");
		exit(1);
	}

	if(fscanf(fr,
		"location: %[^\n] "
		"mysqldb: %[^\n] "
		"mysqlip: %[^\n] "
		"mysqluser: %[^\n] "
		"mysqlpass: %[^\n] ",
		locationID, mysqlDB, mysqlIP, mysqlUser, mysqlPass) != 5)
	{
		fprintf(stderr, "Malformed configuration file\n");
		exit(1);
	}

	fclose(fr);
}

void unlockDoor() {
	digitalWrite(RELAY_SIG, HIGH);
	delay(5000);
	digitalWrite(RELAY_SIG, LOW);
}

int shouldUnlockDoor(unsigned long code, unsigned long facility) {
	MYSQL* con;

	char query[80];

	MYSQL_RES* result;
	int num_rows;
	int ret;

	snprintf(query, sizeof(query),
		"SELECT name FROM user "
		"WHERE code='%lu' AND facility='%lu' AND enabled='1'",
		code, facility);

	con = db_connect(mysqlIP, mysqlDB, mysqlUser, mysqlPass);
	result = db_query(con, query);

	num_rows = mysql_num_rows(result);

	if(num_rows == 0) {
		/* not authorized */
		ret = 0;
	} else if(num_rows == 1) {
		ret = 1;
	} else {
		/* duplicate codes */
		ret = 0;
	}

	mysql_free_result(result);
	mysql_close(con);

	return ret;
}

void handleCode(unsigned long code, unsigned long facility)
{
	printf("Read %d bits\n", bitCount);
	printf("Facility Code: %lu\n", facility);
	printf("TagID: %lu\n", code);

	if(shouldUnlockDoor(code, facility) == 1) {
		printf("Response: unlock_door\n");
		unlockDoor();
	} else {
		printf("Response: nop\n");
	}

	fflush(stdout);
	return;
}

MYSQL* db_connect(char *ip, char *db, char *user, char *pass)
{
	assert(ip != NULL);
	assert(db != NULL);
	assert(user != NULL);
	assert(pass != NULL);

	MYSQL *con = mysql_init(NULL);

	if (con == NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		exit(1);
	}

	if (mysql_real_connect(con, ip, user, pass, db, 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
		exit(1);
	}

	return con;
}

MYSQL_RES* db_query(MYSQL *con, char* query) {
	assert(con != NULL);
	assert(query != NULL);

	if (mysql_query(con, query))
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
	}

	MYSQL_RES *result = mysql_store_result(con);

	if (result == NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
	}

	return result;
}

void writeLog() {
	return;

#if 0
	time_t rawtime;
	struct tm *info;

	time( &rawtime );

	info = localtime( &rawtime );

	char timestamp[80];
	char sqlinsert[80];

	// work out the current timestamp in the format YYYY-mm-dd hh:mm:ss
	// remember that the rpi has no hwc, so the time is utc if connected to
	// the internet, random if not.

	strftime(timestamp,80,"%Y-%m-%d %X", info);
	printf("Timestamp: %s\n", timestamp );

	snprintf(sqlinsert, sizeof sqlinsert,
		"INSERT INTO pdm_logs VALUES(null,'%lu','%lu','%s','%c','%s')"
		facilityCode, cardCode, timestamp, accresult, locationID);

	if (mysql_query(con, sqlinsert))
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
	}

	mysql_free_result(result);
	mysql_close(con);

	delay(1000);

	return;
#endif
}

int main(void)
{
getConfig();
setup();

unsigned long code ;
unsigned long facility;

code = 0;
facility = 0;

printf("\n"
	"Ready.\n"
	"Present card:\n"
	"\n");
fflush(stdout);

for (;;)
{
	assert(wiegand_counter >= 0);
	assert(wiegand_counter <= WIEGAND_WAIT_TIME);

	// This waits to make sure that there have been no more data pulses
	// before processing data
	if (flagDone == 0) {
		if (wiegand_counter == 0) {
			flagDone = 1;
		} else {
			wiegand_counter--;
		}
	}

	// if we have bits and the wiegand counter reached 0
	if ( (bitCount > 0) && (flagDone == 1) )
	{
		if(parseCode(
			databits, bitCount, 
			&code, &facility) == PARSE_METHOD_MATCH)
		{
			handleCode(code, facility);
			writeLog();
		}

		// cleanup and get ready for the next card
		bitCount = 0;
		code = 0;
		facility = 0;
		flagDone = 0;

		for (i=0; i<MAX_BITS; i++)
		{
			databits[i] = 0;
		}
	}
}

return 0;
}
