/*
 * This file is part of Foobar.
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

#ifndef _PARSE_H
#define _PARSE_H
#define PARSE_METHOD_NOMATCH 0
#define PARSE_METHOD_MATCH 1

int parseCode(unsigned char databits, unsigned char bitCount, unsigned long *cardCode, unsigned long *facilityCode);
#endif
