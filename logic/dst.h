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

#ifndef DST_H_
#define DST_H_

struct dst_date_struct
{
    u8 month;
    u8 day;
};

extern struct dst_date_struct dst_dates[];
extern u8 dst_state; // 0=ST, 1=DST

void dst_init(void);
void dst_calculate_dates(void);
u8 dst_isDateInDST(u8 month, u8 day);

#endif
