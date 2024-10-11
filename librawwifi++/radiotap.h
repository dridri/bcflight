/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once

#include "ieee80211_radiotap.h"

/* Radiotap header iteration
 *   implemented in net/wireless/radiotap.c
 *   docs in Documentation/networking/radiotap-headers.txt
 */
/**
 * struct ieee80211_radiotap_iterator - tracks walk thru present radiotap args
 * @rtheader: pointer to the radiotap header we are walking through
 * @max_length: length of radiotap header in cpu byte ordering
 * @this_arg_index: IEEE80211_RADIOTAP_... index of current arg
 * @this_arg: pointer to current radiotap arg
 * @arg_index: internal next argument index
 * @arg: internal next argument pointer
 * @next_bitmap: internal pointer to next present u32
 * @bitmap_shifter: internal shifter for curr u32 bitmap, b0 set == arg present
 */

struct ieee80211_radiotap_iterator {
	struct ieee80211_radiotap_header *rtheader;
	int max_length;
	int this_arg_index;
	uint8_t *this_arg;

	int arg_index;
	uint8_t *arg;
	__le32 *next_bitmap;
	uint32_t bitmap_shifter;
};

#ifdef __cplusplus
extern "C"
#endif
int ieee80211_radiotap_iterator_init(
   struct ieee80211_radiotap_iterator *iterator,
   struct ieee80211_radiotap_header *radiotap_header,
   int max_length);

#ifdef __cplusplus
extern "C"
#endif
 int ieee80211_radiotap_iterator_next(
   struct ieee80211_radiotap_iterator *iterator);

