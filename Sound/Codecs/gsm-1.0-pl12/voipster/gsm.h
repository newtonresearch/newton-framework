

/*
  GSM voice codec, part of the HawkVoice Direct Interface (HVDI)
  cross platform network voice library
  Copyright (C) 2001-2003 Phil Frisbie, Jr. (phil@hawksoft.com)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.
    
  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA  02111-1307, USA.
      
  Or go to http://www.gnu.org/copyleft/lgpl.html
*/

/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */


#ifndef	GSM_H
#define	GSM_H

#ifdef __cplusplus
extern "C" {
#endif

#define GSM_SAMPLES_PER_FRAME       160
#define GSM_ENCODED_FRAME_SIZE      33

// #ifdef _WIN32
// #ifdef GSM0610LIB
#define DECL_FUNC
// #define DECL_FUNC		__declspec( dllexport ) 
// #else // G711LIB
// #define DECL_FUNC		__declspec( dllimport ) 
// #endif // G711LIB
// #endif //_WIN32

/* The GSM_OPT_LTP_CUT option speeds up encoding by using an alternate */
/* faster algorithm to calculate the long term predictor */
#define	GSM_OPT_LTP_CUT		3

DECL_FUNC struct gsm_state *  gsm_create(void);
DECL_FUNC void gsm_destroy(struct gsm_state *st);

DECL_FUNC int gsm_option(struct gsm_state *st, int opt, int *val);

DECL_FUNC int gsm_encode(struct gsm_state *st, const short *in, unsigned char  *out);
DECL_FUNC int gsm_decode(struct gsm_state *st, const unsigned char *in, short *out);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif	/* GSM_H */
