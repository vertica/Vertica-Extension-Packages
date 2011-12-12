/*
 * JulianDate.cpp
 *
 *  Created on: Dec 9, 2011
 *      Author: ptoole
 *
 * Vertica Analytic Database
 *
 * Makefile to build package directory
 *
 * Copyright 2011 Vertica Systems, an HP Company
 *
 *
 * Portions of this software Copyright (c) 2011 by Vertica, an HP
 * Company.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "JulianDate.h"

const int32 je2gc = 32044;              // 1/1/4713 BC - 3/1/4801 BC
const int32 days4yrs = 365*4 + 1;       // 1461 (if there is a leap day)
const int32 days100yrs = days4yrs*25 - 1; // 36524 (for the first 3)
const int32 daysgc = days100yrs*4 + 1;  // 146097 (days in gregorian cycle)


DateADT date2j(int y, int m, int d)
{
    int64       julian;

    if (++m < 4)
        --y, m += 12;

    julian = y * 365 + 1720997;         // - 32167;
    if (y < 0)
        julian += ++y / 4 - 1;          // make divide work right
    else
        julian += y / 4;                // leap year every four years,
    julian -= y / 100;                  // except centuries,
    julian += y / 400;                  // except quad-centuries
    julian += 7834LL * m / 256;         // march thru feb interpolation ~30.6
    julian += d;

    return julian;
}

void
j2date(DateADT jdn, int *year, int *month, int *day)
{
	int days4yrs = 1461;

    int64 g;                            // Gregorian quadirecentennial cycles
    int64 j = jdn + je2gc;              // shift epoch to 3/1/4801 BC
    if (j < 0)
        g = (j + 1) / daysgc - 1;       // make divide work right
    else
        g = j / daysgc;                 // days per 400-year cycle
    int32 dg = j - g * daysgc;          // days within the cycle (positive)
    int32 c = dg / days100yrs;          // century within cycle
    if (c == 4) c = 3;                  // last century can be a day longer
    int32 dc = dg - c * days100yrs;     // days within century
    int32 b = dc / days4yrs;            // quadrennial cycles within century
    int32 db = dc - b * days4yrs;       // days within quadrennial cycle
    int32 a = db / 365;                 // Roman annual cycles (years)
    if (a == 4) a = 3;                  // if the last year was a day longer
    int32 da = db - a * 365;            // days in the last year (from 3/1)
    int64 y = ((g*4 + c)*25 + b)*4 + a; // years since epoch
    int32 m = (da*5 + 308)/153 - 2 + 2; // month from January by interpolation
    int32 d = da - (m + 2)*153/5 + 122; // day of month by interpolation

    *year = y - 4800 + m/12;            // adjust for epoch and 1-based m,d
    *month = m % 12 + 1;
    *day = d + 1;

    return;
}


DateADT JulianDate::getDateADT() {
	return internalDate - date2j(2000, 1, 1);
}


void JulianDate::setDateADT(DateADT date) {
	internalDate = date + date2j(2000, 1, 1);
}


JulianDate::JulianDate(const DateADT &date) {
	internalDate = date + date2j(2000, 1, 1);
}


JulianDate::JulianDate(int year, int month, int day) {
	internalDate = date2j(year, month, day);
}


int JulianDate::getYear() {
	int year, month, day;
	j2date(internalDate, &year, &month, &day);

	return year;
}


int JulianDate::getMonth() {
	int year, month, day;
	j2date(internalDate, &year, &month, &day);

	return month;
}


int JulianDate::getDay(){
	int year, month, day;
	j2date(internalDate, &year, &month, &day);

	return day;
}


void JulianDate::setYear(int year) {
	int y, m, d;
	j2date(internalDate, &y, &m, &d);

	y = year;

	internalDate = date2j(y, m, d);
}


void JulianDate::setMonth(int month) {
	int y, m, d;
	j2date(internalDate, &y, &m, &d);

	m = month;

	internalDate = date2j(y, m, d);
}

void JulianDate::setDay(int day) {
	int y, m, d;
	j2date(internalDate, &y, &m, &d);

	d = day;

	internalDate = date2j(y, m, d);
}

string JulianDate::toString() {
	std::ostringstream dateStream;
	dateStream << getYear() << "-";

	int x = getMonth();
	if (x<10) dateStream << "0";
	dateStream << x;

	dateStream << "-";

	x = getDay();
	if (x<10) dateStream << "0";
	dateStream << x;

	return dateStream.str();
}
