/*
 * sequence.h
 *
 *  Created on: Oct 10, 2010
 *      Author: gabriel
 */

#ifndef SEQUENCE_H_
#define SEQUENCE_H_


// *************************************************************************************************
// Include section

#include "project.h"

// *************************************************************************************************
// Defines section
// Defines section
#define DOORLOCK_SEQUENCE_SIMILARITY (50u)

// Setting section
// sequence limits
#define DOORLOCK_SEQUENCE_MAX_LENGTH				(12u)
#define DOORLOCK_SEQUENCE_MIN_LENGTH				(2u)
#define DOORLOCK_SEQUENCE_PAUSE_RESOLUTION			(32768u/200u)
#define DOORLOCK_SEQUENCE_PAUSE_MAX_LENGTH			(1200u/5u)
#define DOORLOCK_SEQUENCE_PAUSE_MIN_LENGTH			(15u/5u)
#define	DOORLOCK_SEQUENCE_TAP_THRESHOLD				(120)
#define	DOORLOCK_SEQUENCE_TIMEOUT					(30u)

// error codes
#define DOORLOCK_ERROR_SUCCESS						(0u)
#define DOORLOCK_ERROR_FAILURE						(1u)
#define DOORLOCK_ERROR_TIMEOUT						(2u)
#define DOORLOCK_ERROR_INVALID						(3u)



// random bits collection interval
#define DOORLOCK_RANDOM_INTERVAL					(150u)



// *************************************************************************************************
// Prototypes section

extern u8 doorlock_sequence(u8 sequence[DOORLOCK_SEQUENCE_MAX_LENGTH]);
extern u8 sequence_compare(u8* sequence_a, u8* sequence_b);


//***************************************************************************************************

#endif /* SEQUENCE_H_ */
