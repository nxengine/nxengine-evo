/************************************************************************
 * $Id$
 *
 * ------------
 * Description:
 * ------------
 * This is an implemention of Unicode's Bidirectional Algorithm
 * (known as UAX #9).
 *
 *   http://www.unicode.org/reports/tr9/
 * 
 * Author: Ahmad Khalifa
 *
 * -----------------
 * Revision Details:    (Updated by Revision Control System)
 * -----------------
 *  $Date$
 *  $Author$
 *  $Revision$
 *  $Source$
 *
 * (www.arabeyes.org - under MIT license)
 *
 ************************************************************************/
#include <cstdint>

#define BLOCKTYPE uint32_t*
#define CHARTYPE uint32_t

int doBidi(BLOCKTYPE line, int count, bool applyShape, bool reorderCombining);
