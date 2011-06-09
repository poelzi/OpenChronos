/*
    Daylight Saving Time for OpenChronos on the TI ez430 chronos watch.
    Copyright 2011 Rick Miller <rdmiller3@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "project.h"

#ifdef CONFIG_DST

// driver
#include "altitude.h"
#include "display.h"
#include "vti_ps.h"
#include "ports.h"
#include "timer.h"

#include "stopwatch.h"

// logic
#include "user.h"
#include "clock.h"
#include "date.h"
#include "dst.h"

#include "menu.h"

struct dst_date_struct dst_dates[2];
u8 dst_state; // 0=ST, 1=DST

u8 dst_day_of_week(u16 year, u8 month, u8 day);

void dst_init(void)
{
    dst_calculate_dates();
}

void dst_calculate_dates(void)
{
    // This should be run whenever sDate.year gets changed.

    // DST in US: 2nd Sun in Mar to 1st Sun in Nov.

    dst_dates[0].month = 3;
    dst_dates[0].day = 15 - dst_day_of_week(sDate.year, 3, 1);
    if (dst_dates[0].day == 15)
    {
        dst_dates[0].day = 8;
    }

    dst_dates[1].month = 3;
    dst_dates[1].day = 8 - dst_day_of_week(sDate.year, 11, 1);
    if (dst_dates[1].day == 8)
    {
        dst_dates[1].day = 1;
    }

    // This test may be wrong if you set your watch
    // on the time-change day.
    dst_state = (dst_isDateInDST(sDate.month, sDate.day)) ?
                1 : 0;
}

#define DSTNUM(x,y) (((u16)(x)*100)+(u16)(y))

u8 dst_isDateInDST(u8 month, u8 day)
{
    return ((DSTNUM(month,day)>=DSTNUM(dst_dates[0].month,dst_dates[0].day)) &&
            (DSTNUM(month,day)<DSTNUM(dst_dates[1].month,dst_dates[1].day)));
}

u8 dst_day_of_week(u16 year, u8 month, u8 day)
{
    // Calculate days since 2000-01-01
    u32 tmp = ((u32)sDate.year % 200) * 365;
    tmp += ((sDate.year % 200) / 4); // leap days
    switch (sDate.month) // using lots of drop-through!
    {
        case 12:
            tmp += 30; // for nov
        case 11:
            tmp += 31; // for oct
        case 10:
            tmp += 30; // for sep
        case 9:
            tmp += 31; // for aug
        case 8:
            tmp += 31; // for jul
        case 7:
            tmp += 30; // for jun
        case 6:
            tmp += 31; // for may
        case 5:
            tmp += 30; // for apr
        case 4:
            tmp += 31; // for mar
        case 3:
            tmp += 28; // for feb
            if ((sDate.year % 4) == 0)
            {
                tmp++;
            }
        case 2:
            tmp += 31; // for jan
        case 1:
        default:
            // do nothing
            break;
    }
    tmp += sDate.day;
    tmp--; // because day-of-month is 1-based (2000-01-01 is the ZERO day).

    // day zero (2000-01-01) was a Saturday.
    return (u8)((tmp + 6) % 7);
}

#endif /* CONFIG_DST */

