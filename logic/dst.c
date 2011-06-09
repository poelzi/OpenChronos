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

#if (CONFIG_DST > 0)

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

#define N_SUN_OF_MON(n,mon) (((n)*7)-dst_day_of_week(sDate.year,(mon),((n)*7)))
#define LAST_SUN_OF_MON(mon,days) ((days)-dst_day_of_week(sDate.year,(mon),(days)))

void dst_calculate_dates(void)
{
    // This should be run whenever sDate.year gets changed.


    #if (CONFIG_DST == 1)
    // DST in US/Canada: 2nd Sun in Mar to 1st Sun in Nov.
        dst_dates[0].month = 3;
        dst_dates[0].day = N_SUN_OF_MON(2, 3);
        dst_dates[1].month = 11;
        dst_dates[1].day = N_SUN_OF_MON(1, 11);
    #endif
    #if (CONFIG_DST == 2)
    // DST in Mexico: first Sun in Apr to last Sun in Oct.
        dst_dates[0].month = 4;
        dst_dates[0].day = N_SUN_OF_MON(1, 4);
        dst_dates[1].month = 10;
        dst_dates[1].day = LAST_SUN_OF_MON(10, 31);
    #endif
    #if (CONFIG_DST == 3)
    // DST in Brazil: third Sun in Oct to third Sun in Feb.
        dst_dates[0].month = 10;
        dst_dates[0].day = N_SUN_OF_MON(3, 10);
        dst_dates[1].month = 2;
        dst_dates[1].day = N_SUN_OF_MON(3, 2);
    #endif
    #if (CONFIG_DST == 4)
        // DST in EU/UK: last Sun in Mar to last Sun in Oct.
        dst_dates[0].month = 3;
        dst_dates[0].day = LAST_SUN_OF_MON(3, 31);
        dst_dates[1].month = 10;
        dst_dates[1].day = LAST_SUN_OF_MON(10, 31);
    #endif
    #if (CONFIG_DST == 5)
        // DST in Australia: first Sun in Oct to first Sun in Apr.
        dst_dates[0].month = 10;
        dst_dates[0].day = N_SUN_OF_MON(1, 10);
        dst_dates[1].month = 4;
        dst_dates[1].day = N_SUN_OF_MON(1, 4);
    #endif
    #if (CONFIG_DST == 6)
        // DST in New Zealand: last Sun in Sep to first Sun in Apr.
        dst_dates[0].month = 9;
        dst_dates[0].day = LAST_SUN_OF_MON(9, 30);
        dst_dates[1].month = 4;
        dst_dates[1].day = N_SUN_OF_MON(1, 4);
    #endif

    // This test may be wrong if you set your watch
    // on the time-change day.
    dst_state = (dst_isDateInDST(sDate.month, sDate.day)) ?
                1 : 0;
}

#define DSTNUM(x,y) (((u16)(x)*100)+(u16)(y))

u8 dst_isDateInDST(u8 month, u8 day)
{
    if (dst_dates[0].month < dst_dates[1].month)
    {
        // Northern hemisphere
        return
            ((DSTNUM(month,day)>=DSTNUM(dst_dates[0].month,dst_dates[0].day)) &&
             (DSTNUM(month,day)<DSTNUM(dst_dates[1].month,dst_dates[1].day)));
    }
    else
    {
        // Southern hemisphere
        return (!(
            ((DSTNUM(month,day)>=DSTNUM(dst_dates[1].month,dst_dates[1].day)) &&
             (DSTNUM(month,day)<DSTNUM(dst_dates[0].month,dst_dates[0].day)))));
    }
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

#endif /* (CONFIG_DST > 0) */

