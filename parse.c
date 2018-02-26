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
 */

#include "parse.h"

int wiegand_26bit_nofacility(
	unsigned char *databits, unsigned char bitCount,
	unsigned long *cardCode, unsigned long *facilityCode)
{
	int i;

	if(bitCount != 26)
	{
		return PARSE_METHOD_NOMATCH;
	}

	// card code = bits 2 to 25
	for (i=1; i<25; i++)
	{
		*cardCode <<=1;
		*cardCode |= databits[i];
	}

	return PARSE_METHOD_MATCH;
}

int wiegand_26bit_facility(
	unsigned char *databits, unsigned char bitCount,
	unsigned long *cardCode, unsigned long *facilityCode)
{
	int i;

	if(bitCount != 26)
	{
		return PARSE_METHOD_NOMATCH;
	}

	// facility code = bits 2 to 9
	for (i=1; i<9; i++)
	{
		*facilityCode <<=1;
		*facilityCode |= databits[i];
	}

	// card code = bits 10 to 23
	for (i=9; i<25; i++)
	{
		*cardCode <<=1;
		*cardCode |= databits[i];
	}

	return PARSE_METHOD_MATCH;
}

// 35 bit HID Corporate 1000 format
int wiegand_35bit(
	unsigned char *databits, unsigned char bitCount,
	unsigned long *cardCode, unsigned long *facilityCode)
{
	int i;

	if(bitCount != 35)
	{
		return PARSE_METHOD_NOMATCH;
	}

	// facility code = bits 2 to 14
	for (i=2; i<14; i++)
	{
		*facilityCode <<=1;
		*facilityCode |= databits[i];
	}

	// card code = bits 15 to 34
	for (i=14; i<34; i++)
	{
		*cardCode <<=1;
		*cardCode |= databits[i];
	}

	return PARSE_METHOD_MATCH;
}

//HID Proprietary 37 Bit Format with Facility Code: H10304
int wiegand_37bit(
	unsigned char *databits, unsigned char bitCount,
	unsigned long *cardCode, unsigned long *facilityCode)
{
	int i;

	if(bitCount != 37)
	{
		return PARSE_METHOD_NOMATCH;
	}

	// facility code = bits 2 to 17
	for (i=2; i<17; i++)
	{
		facilityCode <<=1;
		facilityCode |= databits[i];
	}

	// card code = bits 18 to 36
	for (i=18; i<36; i++)
	{
		cardCode <<=1;
		cardCode |= databits[i];
	}

	return PARSE_METHOD_MATCH;
}

int default_match(
	unsigned char *databits, unsigned char bitCount,
	unsigned long *cardCode, unsigned long *facilityCode)
{
	printf("---\n");
	printf("Unknown format\n");
	printf("bitCount: %d\n", bitCount);
	printf("facilityCode: %lu\n", facilityCode);
	printf("cardCode: %lu\n", cardCode);
	printf("---\n");
	fflush(stdout);

	return PARSE_METHOD_NOMATCH;
}

int parseCode(
	unsigned char databits, unsigned char bitCount,
	unsigned long *cardCode, unsigned long *facilityCode)
{
	int status = PARSE_METHOD_NOMATCH;

	status |= wiegand_26bit_facility(databits, bitCount, cardCode, facilityCode);
	status |= wiegand_35bit(databits, bitCount, cardCode, facilityCode);
	status |= wiegand_37bit(databits, bitCount, cardCode, facilityCode);

	if(status == PARSE_METHOD_NOMATCH)
	{
		default_match(databits, bitCount, cardCode, facilityCode);
	}

	return status;
}
