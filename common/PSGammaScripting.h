
#ifndef PSGAMMASCRIPTING_H
#define PSGAMMASCRIPTING_H

#include "PIDefines.h"
#include "PIActions.h"
#include "PITerminology.h"

#ifndef Rez
#include "PSGamma.h"
#endif

#define vendorName			"Bourt"
#define plugInName			"Adjust Gamma"
#define plugInAETEComment	"A plug-in for adjusting image gamma"
#define plugInSuiteID		'sdk1'
#define plugInClassID		plugInSuiteID
#define plugInEventID		plugInClassID
#define plugInUniqueID		"12F476CF-74EA-4C54-B2AA-1C4D3964472C"

#define plugInCopyrightYear "2012"
#define plugInDescription   "A plug-in for adjusting the gamma of an image."

#define keyPSGamma 			'gamM'
#define keyPSInvert			'invR'

#ifndef Rez
OSErr ReadScriptParameters(Boolean* displayDialog);
OSErr WriteScriptParameters(void);
#endif
#endif